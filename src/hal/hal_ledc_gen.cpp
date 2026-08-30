#include "hal/hal_ledc_gen.h"
#include <Arduino.h>
#include "driver/ledc.h"
#include "esp_log.h"

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

static ledc_timer_bit_t get_optimal_resolution(uint32_t freq) {
    if (freq <= 5000) {
        return LEDC_TIMER_14_BIT;
    } else if (freq <= 50000) {
        return LEDC_TIMER_12_BIT;
    } else if (freq <= 500000) {
        return LEDC_TIMER_10_BIT;
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

    ledc_timer_bit_t res = get_optimal_resolution(s_status.frequency_hz);

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE_DEFAULT,
        .duty_resolution  = res,
        .timer_num        = LEDC_TIMER_NUM,
        .freq_hz          = s_status.frequency_hz,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config failed: %s", esp_err_to_name(err));
        return err;
    }

    uint32_t max_duty = (1 << res);
    uint32_t duty = (max_duty * s_status.duty_cycle_pct) / 100;

    ledc_channel_config_t ledc_channel = {
        .gpio_num       = s_status.output_gpio,
        .speed_mode     = LEDC_MODE_DEFAULT,
        .channel        = LEDC_CHANNEL_NUM,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_NUM,
        .duty           = 0, // initially 0
        .hpoint         = 0
    };
    err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config failed: %s", esp_err_to_name(err));
        return err;
    }

    s_is_inited = true;
    s_status.enabled = false;
    ESP_LOGI(TAG, "LEDC reference generator initialized on GPIO %d", gpio_num);
    return ESP_OK;
}

esp_err_t hal_ledc_gen_set_frequency(uint32_t freq_hz, uint8_t duty_pct) {
    if (!s_is_inited) return ESP_ERR_INVALID_STATE;
    if (freq_hz == 0 || freq_hz > 40000000) return ESP_ERR_INVALID_ARG;
    if (duty_pct > 100) duty_pct = 100;

    s_status.frequency_hz = freq_hz;
    s_status.duty_cycle_pct = duty_pct;

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
        ESP_LOGE(TAG, "ledc_timer_config failed for freq %lu: %s", (unsigned long)freq_hz, esp_err_to_name(err));
        return err;
    }

    if (s_status.enabled) {
        uint32_t max_duty = (1 << res);
        uint32_t duty = (max_duty * duty_pct) / 100;
        ledc_set_duty(LEDC_MODE_DEFAULT, LEDC_CHANNEL_NUM, duty);
        ledc_update_duty(LEDC_MODE_DEFAULT, LEDC_CHANNEL_NUM);
    }

    return ESP_OK;
}

esp_err_t hal_ledc_gen_start(void) {
    if (!s_is_inited) return ESP_ERR_INVALID_STATE;

    ledc_timer_bit_t res = get_optimal_resolution(s_status.frequency_hz);
    uint32_t max_duty = (1 << res);
    uint32_t duty = (max_duty * s_status.duty_cycle_pct) / 100;

    ledc_set_duty(LEDC_MODE_DEFAULT, LEDC_CHANNEL_NUM, duty);
    ledc_update_duty(LEDC_MODE_DEFAULT, LEDC_CHANNEL_NUM);
    s_status.enabled = true;
    ESP_LOGI(TAG, "LEDC reference generator started (%lu Hz, %u%% duty)", (unsigned long)s_status.frequency_hz, s_status.duty_cycle_pct);
    return ESP_OK;
}

esp_err_t hal_ledc_gen_stop(void) {
    if (!s_is_inited) return ESP_ERR_INVALID_STATE;

    ledc_stop(LEDC_MODE_DEFAULT, LEDC_CHANNEL_NUM, 0);
    s_status.enabled = false;
    ESP_LOGI(TAG, "LEDC reference generator stopped");
    return ESP_OK;
}

esp_err_t hal_ledc_gen_deinit(void) {
    hal_ledc_gen_stop();
    s_is_inited = false;
    ESP_LOGI(TAG, "LEDC reference generator deinitialized");
    return ESP_OK;
}

ledc_gen_status_t hal_ledc_gen_get_status(void) {
    return s_status;
}
