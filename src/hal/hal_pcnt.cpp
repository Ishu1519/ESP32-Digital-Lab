#include "hal/hal_pcnt.h"
#include <Arduino.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "HAL_PCNT";

#define PCNT_UNIT_DEFAULT       PCNT_UNIT_0
#define PCNT_CHANNEL_DEFAULT    PCNT_CHANNEL_0
#define PCNT_H_LIM_VAL          20000
#define PCNT_L_LIM_VAL          0

static portMUX_TYPE pcnt_spinlock = portMUX_INITIALIZER_UNLOCKED;
static volatile int32_t s_overflow_count = 0;
static volatile uint64_t s_last_gate_time_us = 0;
static esp_timer_handle_t s_gate_timer = NULL;
static bool s_is_running = false;
static bool s_is_inited = false;
static pcnt_config_params_t s_config;

static pcnt_measurement_t s_latest_measurement = {0};
static SemaphoreHandle_t s_meas_mutex = NULL;

// Pulse width / duty cycle capture via GPIO ISR
static volatile uint64_t s_high_edge_time_us = 0;
static volatile uint64_t s_low_edge_time_us = 0;
static volatile uint32_t s_high_duration_sum_us = 0;
static volatile uint32_t s_period_sum_us = 0;
static volatile uint32_t s_edge_samples = 0;

static void IRAM_ATTR pcnt_overflow_isr(void *arg) {
    uint32_t status = 0;
    pcnt_get_event_status(PCNT_UNIT_DEFAULT, &status);
    if (status & PCNT_EVT_H_LIM) {
        portENTER_CRITICAL_ISR(&pcnt_spinlock);
        s_overflow_count++;
        portEXIT_CRITICAL_ISR(&pcnt_spinlock);
    }
}

static void IRAM_ATTR gpio_edge_isr(void *arg) {
    uint64_t now = esp_timer_get_time();
    int level = gpio_get_level((gpio_num_t)s_config.input_gpio);
    if (level == 1) {
        // Rising edge
        if (s_low_edge_time_us > 0) {
            uint32_t period = (uint32_t)(now - s_high_edge_time_us);
            if (period > 0 && period < 2000000) { // Limit to sensible audio/control pulse ranges
                uint32_t high_dur = (uint32_t)(s_low_edge_time_us - s_high_edge_time_us);
                s_period_sum_us += period;
                s_high_duration_sum_us += high_dur;
                s_edge_samples++;
            }
        }
        s_high_edge_time_us = now;
    } else {
        // Falling edge
        s_low_edge_time_us = now;
    }
}

static void gate_timer_callback(void *arg) {
    if (!s_is_running) return;

    int16_t count = 0;
    int32_t overflows = 0;
    uint64_t now_us = esp_timer_get_time();

    portENTER_CRITICAL(&pcnt_spinlock);
    pcnt_get_counter_value(PCNT_UNIT_DEFAULT, &count);
    pcnt_counter_clear(PCNT_UNIT_DEFAULT);
    overflows = s_overflow_count;
    s_overflow_count = 0;
    uint64_t elapsed_us = now_us - s_last_gate_time_us;
    s_last_gate_time_us = now_us;
    portEXIT_CRITICAL(&pcnt_spinlock);

    if (elapsed_us == 0) elapsed_us = s_config.gate_time_ms * 1000;

    int64_t total_pulses = ((int64_t)overflows * PCNT_H_LIM_VAL) + count;
    if (total_pulses < 0) total_pulses = 0;

    double elapsed_sec = (double)elapsed_us / 1000000.0;
    double freq = (double)total_pulses / elapsed_sec;

    // Period & Duty cycle
    double period_us = (freq > 0.01) ? (1000000.0 / freq) : 0.0;
    double duty_cycle = 50.0;
    double pulse_width_us = period_us * 0.5;

    if (s_edge_samples >= 5 && s_period_sum_us > 0) {
        duty_cycle = ((double)s_high_duration_sum_us / (double)s_period_sum_us) * 100.0;
        if (duty_cycle < 0.0) duty_cycle = 0.0;
        if (duty_cycle > 100.0) duty_cycle = 100.0;
        pulse_width_us = period_us * (duty_cycle / 100.0);
        s_edge_samples = 0;
        s_period_sum_us = 0;
        s_high_duration_sum_us = 0;
    }

    if (xSemaphoreTake(s_meas_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        s_latest_measurement.frequency_hz = freq;
        s_latest_measurement.period_us = period_us;
        s_latest_measurement.duty_cycle_pct = duty_cycle;
        s_latest_measurement.pulse_width_us = pulse_width_us;
        s_latest_measurement.total_pulses = total_pulses;
        s_latest_measurement.gate_time_ms = s_config.gate_time_ms;
        s_latest_measurement.signal_detected = (total_pulses > 0);
        s_latest_measurement.samples_count++;
        xSemaphoreGive(s_meas_mutex);
    }
}

esp_err_t hal_pcnt_init(const pcnt_config_params_t *config) {
    if (s_is_inited) {
        hal_pcnt_deinit();
    }

    s_config = *config;
    if (s_config.gate_time_ms == 0) s_config.gate_time_ms = 500;

    if (s_meas_mutex == NULL) {
        s_meas_mutex = xSemaphoreCreateMutex();
    }

    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = s_config.input_gpio,
        .ctrl_gpio_num = PCNT_PIN_NOT_USED,
        .lctrl_mode = PCNT_MODE_KEEP,
        .hctrl_mode = PCNT_MODE_KEEP,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_DIS,
        .counter_h_lim = PCNT_H_LIM_VAL,
        .counter_l_lim = PCNT_L_LIM_VAL,
        .unit = PCNT_UNIT_DEFAULT,
        .channel = PCNT_CHANNEL_DEFAULT
    };

    esp_err_t err = pcnt_unit_config(&pcnt_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "pcnt_unit_config failed: %s", esp_err_to_name(err));
        return err;
    }

    if (s_config.filter_val > 0) {
        pcnt_set_filter_value(PCNT_UNIT_DEFAULT, s_config.filter_val);
        pcnt_filter_enable(PCNT_UNIT_DEFAULT);
    } else {
        pcnt_filter_disable(PCNT_UNIT_DEFAULT);
    }

    pcnt_event_enable(PCNT_UNIT_DEFAULT, PCNT_EVT_H_LIM);
    pcnt_counter_pause(PCNT_UNIT_DEFAULT);
    pcnt_counter_clear(PCNT_UNIT_DEFAULT);

    err = pcnt_isr_service_install(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "pcnt_isr_service_install failed: %s", esp_err_to_name(err));
        return err;
    }

    pcnt_isr_handler_add(PCNT_UNIT_DEFAULT, pcnt_overflow_isr, NULL);

    // Configure GPIO for pulse width / edge interrupt
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << s_config.input_gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)s_config.input_gpio, gpio_edge_isr, NULL);

    // Create periodic gate timer
    const esp_timer_create_args_t timer_args = {
        .callback = &gate_timer_callback,
        .name = "pcnt_gate_timer"
    };
    err = esp_timer_create(&timer_args, &s_gate_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_timer_create failed: %s", esp_err_to_name(err));
        return err;
    }

    s_is_inited = true;
    ESP_LOGI(TAG, "PCNT initialized on GPIO %d with gate time %lu ms", s_config.input_gpio, (unsigned long)s_config.gate_time_ms);
    return ESP_OK;
}

esp_err_t hal_pcnt_start(void) {
    if (!s_is_inited) return ESP_ERR_INVALID_STATE;
    if (s_is_running) return ESP_OK;

    portENTER_CRITICAL(&pcnt_spinlock);
    s_overflow_count = 0;
    s_last_gate_time_us = esp_timer_get_time();
    pcnt_counter_clear(PCNT_UNIT_DEFAULT);
    pcnt_counter_resume(PCNT_UNIT_DEFAULT);
    portEXIT_CRITICAL(&pcnt_spinlock);

    esp_err_t err = esp_timer_start_periodic(s_gate_timer, (uint64_t)s_config.gate_time_ms * 1000);
    if (err == ESP_OK) {
        s_is_running = true;
        ESP_LOGI(TAG, "PCNT counter started");
    }
    return err;
}

esp_err_t hal_pcnt_stop(void) {
    if (!s_is_running) return ESP_OK;

    esp_timer_stop(s_gate_timer);
    pcnt_counter_pause(PCNT_UNIT_DEFAULT);
    s_is_running = false;
    ESP_LOGI(TAG, "PCNT counter stopped");
    return ESP_OK;
}

esp_err_t hal_pcnt_deinit(void) {
    hal_pcnt_stop();
    if (s_gate_timer) {
        esp_timer_delete(s_gate_timer);
        s_gate_timer = NULL;
    }
    pcnt_isr_handler_remove(PCNT_UNIT_DEFAULT);
    gpio_isr_handler_remove((gpio_num_t)s_config.input_gpio);
    s_is_inited = false;
    ESP_LOGI(TAG, "PCNT deinitialized");
    return ESP_OK;
}

esp_err_t hal_pcnt_get_measurement(pcnt_measurement_t *out_meas) {
    if (!out_meas) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(s_meas_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        *out_meas = s_latest_measurement;
        xSemaphoreGive(s_meas_mutex);
        return ESP_OK;
    }
    return ESP_ERR_TIMEOUT;
}

esp_err_t hal_pcnt_set_gate_time(uint32_t gate_time_ms) {
    if (gate_time_ms < 50 || gate_time_ms > 10000) return ESP_ERR_INVALID_ARG;
    s_config.gate_time_ms = gate_time_ms;
    if (s_is_running) {
        esp_timer_stop(s_gate_timer);
        esp_timer_start_periodic(s_gate_timer, (uint64_t)s_config.gate_time_ms * 1000);
    }
    return ESP_OK;
}
