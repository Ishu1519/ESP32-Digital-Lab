#include "hal/hal_ledc_gen.h"
#include <Arduino.h>
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "HAL_LEDC_GEN";

#define LEDC_TIMER_NUM          LEDC_TIMER_0
#define LEDC_MODE_DEFAULT       LEDC_HIGH_SPEED_MODE
#define LEDC_CHANNEL_NUM        LEDC_CHANNEL_0

static ledc_gen_status_t s_status = {
    .frequency_hz = 1000,
    .duty_cycle_pct = 50,
    .output_gpio = 19,
    .enabled = false
};

static bool s_is_inited = false;
static esp_timer_handle_t s_low_freq_timer = NULL;
static volatile int s_low_freq_pin_level = 0;
static bool s_is_low_freq_mode = false;

static void low_freq_timer_cb(void *arg) {
    s_low_freq_pin_level = !s_low_freq_pin_level;
    gpio_set_level((gpio_num_t)s_status.output_gpio, s_low_freq_pin_level);
}

static ledc_timer_bit_t get_optimal_resolution(uint32_t freq) {
    if (freq <= 5000) {
        return LEDC_TIMER_13_BIT;
    } else if (freq <= 50000) {
        return LEDC_TIMER_11_BIT;
    } else if (freq <= 500000) {
        return LEDC_TIMER_9_BIT;
    } else if (freq <= 5000000) {
        return LEDC_TIMER_6_BIT;
    } else if (freq <= 20000000) {
        return LEDC_TIMER_3_BIT;
    } else {
        return LEDC_TIMER_1_BIT;
    }
}

esp_err_t hal_ledc_gen_init(int gpio_num) {
    if (s_is_inited) {
        hal_ledc_gen_deinit();
    }
    s_status.output_gpio = gpio_num;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level((gpio_num_t)gpio_num, 0);

    const esp_timer_create_args_t timer_args = {
        .callback = &low_freq_timer_cb,
        .name = "low_freq_gen_timer"
    };
    esp_timer_create(&timer_args, &s_low_freq_timer);

    s_is_inited = true;
    s_status.enabled = false;
    return hal_ledc_gen_set_frequency(s_status.frequency_hz, s_status.duty_cycle_pct);
}

esp_err_t hal_ledc_gen_set_frequency(uint32_t freq_hz, uint8_t duty_pct) {
    if (!s_is_inited) return ESP_ERR_INVALID_STATE;
    if (freq_hz == 0 || freq_hz > 40000000) return ESP_ERR_INVALID_ARG;
    if (duty_pct > 100) duty_pct = 100;

    s_status.frequency_hz = freq_hz;
    s_status.duty_cycle_pct = duty_pct;

    // Below 50 Hz, ESP32 APB clock divider cannot reach 1 Hz or 10 Hz directly via LEDC.
    // Use high-precision 64-bit microsecond esp_timer toggle mode for sub-50Hz.
    if (freq_hz < 50) {
        if (!s_is_low_freq_mode) {
            ledc_stop(LEDC_MODE_DEFAULT, LEDC_CHANNEL_NUM, 0);
            gpio_set_direction((gpio_num_t)s_status.output_gpio, GPIO_MODE_OUTPUT);
            s_is_low_freq_mode = true;
        }

        if (s_status.enabled) {
            esp_timer_stop(s_low_freq_timer);
            uint64_t half_period_us = (1000000ULL / freq_hz) / 2;
            if (half_period_us == 0) half_period_us = 1;
            esp_timer_start_periodic(s_low_freq_timer, half_period_us);
        }
        return ESP_OK;
    }

    // Above 50 Hz, use hardware LEDC peripheral
    if (s_is_low_freq_mode) {
        esp_timer_stop(s_low_freq_timer);
        s_is_low_freq_mode = false;
    }

    ledc_timer_bit_t res = get_optimal_resolution(freq_hz);

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE_DEFAULT,
        .duty_resolution  = res,
        .timer_num        = LEDC_TIMER_NUM,
        .freq_hz          = freq_hz,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config failed for %lu Hz: %s", (unsigned long)freq_hz, esp_err_to_name(err));
        return err;
    }

    uint32_t max_duty = (1 << res);
    uint32_t duty = (max_duty * duty_pct) / 100;

    ledc_channel_config_t ledc_channel = {
        .gpio_num       = s_status.output_gpio,
        .speed_mode     = LEDC_MODE_DEFAULT,
        .channel        = LEDC_CHANNEL_NUM,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_NUM,
        .duty           = s_status.enabled ? duty : 0,
        .hpoint         = 0
    };
    err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config failed: %s", esp_err_to_name(err));
        return err;
    }

    if (s_status.enabled) {
        ledc_set_duty(LEDC_MODE_DEFAULT, LEDC_CHANNEL_NUM, duty);
        ledc_update_duty(LEDC_MODE_DEFAULT, LEDC_CHANNEL_NUM);
    }

    return ESP_OK;
}

esp_err_t hal_ledc_gen_start(void) {
    if (!s_is_inited) return ESP_ERR_INVALID_STATE;
    s_status.enabled = true;
    return hal_ledc_gen_set_frequency(s_status.frequency_hz, s_status.duty_cycle_pct);
}

esp_err_t hal_ledc_gen_stop(void) {
    if (!s_is_inited) return ESP_ERR_INVALID_STATE;

    if (s_is_low_freq_mode && s_low_freq_timer) {
        esp_timer_stop(s_low_freq_timer);
        gpio_set_level((gpio_num_t)s_status.output_gpio, 0);
    } else {
        ledc_stop(LEDC_MODE_DEFAULT, LEDC_CHANNEL_NUM, 0);
    }
    s_status.enabled = false;
    ESP_LOGI(TAG, "Reference generator stopped");
    return ESP_OK;
}

esp_err_t hal_ledc_gen_deinit(void) {
    hal_ledc_gen_stop();
    if (s_low_freq_timer) {
        esp_timer_delete(s_low_freq_timer);
        s_low_freq_timer = NULL;
    }
    s_is_inited = false;
    return ESP_OK;
}

ledc_gen_status_t hal_ledc_gen_get_status(void) {
    return s_status;
}
