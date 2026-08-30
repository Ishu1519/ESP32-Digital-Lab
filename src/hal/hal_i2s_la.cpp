#include "hal/hal_i2s_la.h"
#include <Arduino.h>
#include <string.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_timer.h"

static const char *TAG = "HAL_I2S_LA";

static la_config_t s_config = {0};
static bool s_is_inited = false;
static bool s_is_armed = false;
static bool s_is_complete = false;

static uint8_t *s_captured_samples = NULL;
static uint32_t s_captured_count = 0;

esp_err_t hal_i2s_la_init(const la_config_t *config) {
    if (!config) return ESP_ERR_INVALID_ARG;
    if (s_is_inited) hal_i2s_la_deinit();

    s_config = *config;
    if (s_config.sample_depth == 0) s_config.sample_depth = 2048;
    if (s_config.sample_rate_hz == 0) s_config.sample_rate_hz = 1000000;
    if (s_config.num_channels == 0) s_config.num_channels = 4;

    // Configure GPIOs as inputs with pull-ups
    for (int i = 0; i < s_config.num_channels; i++) {
        int pin = s_config.channel_pins[i];
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << pin),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
    }

    s_captured_samples = (uint8_t *)malloc(s_config.sample_depth);
    if (!s_captured_samples) {
        ESP_LOGE(TAG, "Failed to allocate sample buffer for %lu samples", (unsigned long)s_config.sample_depth);
        return ESP_ERR_NO_MEM;
    }

    s_is_inited = true;
    s_is_armed = false;
    s_is_complete = false;

    ESP_LOGI(TAG, "Hardware Logic Analyzer initialized (%lu samples @ %lu Hz, %d channels)",
             (unsigned long)s_config.sample_depth, (unsigned long)s_config.sample_rate_hz, s_config.num_channels);

    return ESP_OK;
}

esp_err_t hal_i2s_la_arm(void) {
    if (!s_is_inited) return ESP_ERR_INVALID_STATE;
    s_is_complete = false;
    s_is_armed = true;
    return ESP_OK;
}

// IRAM fast precision sampling engine
static void IRAM_ATTR run_precision_capture(uint8_t *dest, uint32_t count, uint32_t rate_hz, const int *pins, int num_channels) {
    uint32_t cpu_freq_mhz = getCpuFrequencyMhz(); // Usually 240 MHz
    uint32_t cycles_per_sample = (cpu_freq_mhz * 1000000ULL) / rate_hz;

    int p0 = pins[0];
    int p1 = (num_channels > 1) ? pins[1] : 0;
    int p2 = (num_channels > 2) ? pins[2] : 0;
    int p3 = (num_channels > 3) ? pins[3] : 0;

    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);

    // If trigger configured: wait with timeout for trigger edge
    if (s_config.trigger_mode == LA_TRIGGER_FALLING) {
        uint32_t trig_mask = (1UL << pins[s_config.trigger_channel]);
        uint32_t timeout_cycles = cpu_freq_mhz * 1000000; // 1 second timeout
        uint32_t t_start = ESP.getCycleCount();
        
        while ((GPIO.in & trig_mask) == 0 && (ESP.getCycleCount() - t_start) < timeout_cycles) {}
        while ((GPIO.in & trig_mask) != 0 && (ESP.getCycleCount() - t_start) < timeout_cycles) {}
    } else if (s_config.trigger_mode == LA_TRIGGER_RISING) {
        uint32_t trig_mask = (1UL << pins[s_config.trigger_channel]);
        uint32_t timeout_cycles = cpu_freq_mhz * 1000000; // 1 second timeout
        uint32_t t_start = ESP.getCycleCount();
        
        while ((GPIO.in & trig_mask) != 0 && (ESP.getCycleCount() - t_start) < timeout_cycles) {}
        while ((GPIO.in & trig_mask) == 0 && (ESP.getCycleCount() - t_start) < timeout_cycles) {}
    }

    uint32_t next_cycle = ESP.getCycleCount() + cycles_per_sample;

    if (num_channels == 4) {
        uint32_t m0 = (1UL << p0);
        uint32_t m1 = (1UL << p1);
        uint32_t m2 = (1UL << p2);
        uint32_t m3 = (1UL << p3);

        for (uint32_t i = 0; i < count; i++) {
            while (((int32_t)(ESP.getCycleCount() - next_cycle)) < 0) {
                // Exact cycle boundary wait
            }
            uint32_t cur = ESP.getCycleCount();
            if (((int32_t)(cur - next_cycle)) > (int32_t)cycles_per_sample) {
                next_cycle = cur + cycles_per_sample;
            } else {
                next_cycle += cycles_per_sample;
            }

            uint32_t gpio_in = GPIO.in;
            uint8_t sample = ((gpio_in & m0) ? 1 : 0) |
                             ((gpio_in & m1) ? 2 : 0) |
                             ((gpio_in & m2) ? 4 : 0) |
                             ((gpio_in & m3) ? 8 : 0);
            dest[i] = sample;
        }
    } else {
        for (uint32_t i = 0; i < count; i++) {
            while (((int32_t)(ESP.getCycleCount() - next_cycle)) < 0) {
                // Exact cycle boundary wait
            }
            uint32_t cur = ESP.getCycleCount();
            if (((int32_t)(cur - next_cycle)) > (int32_t)cycles_per_sample) {
                next_cycle = cur + cycles_per_sample;
            } else {
                next_cycle += cycles_per_sample;
            }

            uint32_t gpio_in = GPIO.in;
            uint8_t sample = 0;
            for (int ch = 0; ch < num_channels; ch++) {
                if (gpio_in & (1UL << pins[ch])) {
                    sample |= (1 << ch);
                }
            }
            dest[i] = sample;
        }
    }

    portEXIT_CRITICAL(&mux);
}

esp_err_t hal_i2s_la_capture_sync(uint32_t timeout_ms, la_capture_result_t *out_result) {
    if (!s_is_inited || !out_result || !s_captured_samples) return ESP_ERR_INVALID_ARG;

    hal_i2s_la_arm();

    uint32_t count = s_config.sample_depth;
    run_precision_capture(s_captured_samples, count, s_config.sample_rate_hz, s_config.channel_pins, s_config.num_channels);

    s_captured_count = count;
    s_is_complete = true;
    s_is_armed = false;

    out_result->buffer = s_captured_samples;
    out_result->buffer_len = s_captured_count;
    out_result->sample_rate_hz = s_config.sample_rate_hz;
    out_result->trigger_index = 0;
    out_result->is_triggered = true;
    out_result->is_complete = true;

    return ESP_OK;
}

esp_err_t hal_i2s_la_stop(void) {
    if (!s_is_inited) return ESP_ERR_INVALID_STATE;
    s_is_armed = false;
    return ESP_OK;
}

esp_err_t hal_i2s_la_deinit(void) {
    hal_i2s_la_stop();

    if (s_captured_samples) {
        free(s_captured_samples);
        s_captured_samples = NULL;
    }

    s_is_inited = false;
    s_is_armed = false;
    s_is_complete = false;
    ESP_LOGI(TAG, "Logic Analyzer deinitialized");
    return ESP_OK;
}

esp_err_t hal_i2s_la_get_status(bool *out_armed, bool *out_complete) {
    if (out_armed) *out_armed = s_is_armed;
    if (out_complete) *out_complete = s_is_complete;
    return ESP_OK;
}
