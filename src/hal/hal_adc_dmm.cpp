#include "hal/hal_adc_dmm.h"
#include <Arduino.h>
#include <math.h>
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "HAL_ADC_DMM";

static bool s_is_inited = false;
static adc1_channel_t s_channel = ADC1_CHANNEL_6; // GPIO 34
static esp_adc_cal_characteristics_t s_adc_chars;
static bool s_efuse_cal_present = false;
static uint32_t s_oversample_count = 128;
static dmm_range_t s_range = DMM_RANGE_3V3;

// Software fine calibration trims
static float s_cal_offset_v = 0.0f;
static float s_cal_gain_scale = 1.0f;

// Check eFuse VRef on original ESP32
static bool check_efuse(void) {
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Vref: Supported");
        return true;
    } else if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Two Point: Supported");
        return true;
    } else {
        ESP_LOGW(TAG, "eFuse Vref: Not burnt, using default reference 1100 mV");
        return false;
    }
}

esp_err_t hal_adc_dmm_init(adc1_channel_t channel) {
    s_channel = channel;

    // Configure ADC1 bit width & channel attenuation
    adc1_config_width(ADC_WIDTH_BIT_12);
    esp_err_t err = adc1_config_channel_atten(s_channel, ADC_ATTEN_DB_11);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel atten: %s", esp_err_to_name(err));
        return err;
    }

    s_efuse_cal_present = check_efuse();
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
        ADC_UNIT_1,
        ADC_ATTEN_DB_11,
        ADC_WIDTH_BIT_12,
        1100, // Default reference voltage mV if eFuse missing
        &s_adc_chars
    );

    ESP_LOGI(TAG, "ADC1 DMM initialized on Channel %d (eFuse Cal Type: %d, Oversample: %lu)",
             s_channel, val_type, (unsigned long)s_oversample_count);

    s_is_inited = true;
    return ESP_OK;
}

esp_err_t hal_adc_dmm_deinit(void) {
    s_is_inited = false;
    ESP_LOGI(TAG, "ADC1 DMM deinitialized");
    return ESP_OK;
}

esp_err_t hal_adc_dmm_set_range(dmm_range_t range) {
    s_range = range;
    return ESP_OK;
}

esp_err_t hal_adc_dmm_set_oversample(uint32_t samples) {
    if (samples < 1) samples = 1;
    if (samples > 1024) samples = 1024;
    s_oversample_count = samples;
    return ESP_OK;
}

esp_err_t hal_adc_dmm_set_calibration(float offset_v, float gain_scale) {
    s_cal_offset_v = offset_v;
    s_cal_gain_scale = gain_scale;
    ESP_LOGI(TAG, "Set DMM calibration: Offset=%.4f V, Gain=%.4f", offset_v, gain_scale);
    return ESP_OK;
}

esp_err_t hal_adc_dmm_get_efuse_info(bool *out_has_efuse, uint32_t *out_vref_mv) {
    if (out_has_efuse) *out_has_efuse = s_efuse_cal_present;
    if (out_vref_mv) *out_vref_mv = s_adc_chars.vref;
    return ESP_OK;
}

static inline float raw_to_mv(uint32_t raw) {
    if (raw == 0) return 0.0f;
    if (raw < 60) {
        float cal_at_60 = (float)esp_adc_cal_raw_to_voltage(60, &s_adc_chars);
        return (float)raw * (cal_at_60 / 60.0f);
    }
    return (float)esp_adc_cal_raw_to_voltage(raw, &s_adc_chars);
}

esp_err_t hal_adc_dmm_read(dmm_measurement_t *out_meas) {
    if (!s_is_inited || !out_meas) return ESP_ERR_INVALID_STATE;

    uint32_t raw_sum = 0;
    uint32_t raw_min = 4096;
    uint32_t raw_max = 0;
    uint32_t n = s_oversample_count;

    // Buffer to compute standard deviation
    float readings_mv[256];
    uint32_t sample_storage_count = (n <= 256) ? n : 256;

    for (uint32_t i = 0; i < n; i++) {
        uint32_t raw = adc1_get_raw(s_channel);
        raw_sum += raw;
        if (raw < raw_min) raw_min = raw;
        if (raw > raw_max) raw_max = raw;

        if (i < sample_storage_count) {
            readings_mv[i] = raw_to_mv(raw);
        }
        esp_rom_delay_us(10); // Anti-aliasing sample spacing
    }

    uint32_t raw_avg = raw_sum / n;
    float mv_avg = raw_to_mv(raw_avg);

    // Compute standard deviation (noise floor) in mV
    float sum_sq_diff = 0.0f;
    for (uint32_t i = 0; i < sample_storage_count; i++) {
        float diff = readings_mv[i] - mv_avg;
        sum_sq_diff += diff * diff;
    }
    float std_dev_mv = sqrtf(sum_sq_diff / sample_storage_count);

    // Convert to Volts and apply range multiplier
    float range_multiplier = (s_range == DMM_RANGE_5V) ? 2.0f : 1.0f;
    float voltage_v = (mv_avg / 1000.0f) * range_multiplier;

    // Apply software fine calibration
    voltage_v = (voltage_v * s_cal_gain_scale) + s_cal_offset_v;
    if (voltage_v < 0.0f) {
        voltage_v = 0.0f;
    }

    float v_min_v = (raw_to_mv(raw_min) / 1000.0f) * range_multiplier;
    float v_max_v = (raw_to_mv(raw_max) / 1000.0f) * range_multiplier;

    out_meas->voltage_v = voltage_v;
    out_meas->raw_adc_avg = raw_avg;
    out_meas->raw_adc_min = raw_min;
    out_meas->raw_adc_max = raw_max;
    out_meas->noise_std_dev_mv = std_dev_mv * range_multiplier;
    out_meas->v_min_v = v_min_v;
    out_meas->v_max_v = v_max_v;
    out_meas->v_pp_mv = (v_max_v - v_min_v) * 1000.0f;
    out_meas->num_samples = n;
    out_meas->range = s_range;
    out_meas->efuse_cal_applied = s_efuse_cal_present;

    return ESP_OK;
}

esp_err_t hal_adc_dmm_capture_raw_stream(uint32_t count, uint32_t interval_us, dmm_raw_stream_stats_t *out_stats, uint16_t *out_raw_samples) {
    if (!s_is_inited || !out_stats) return ESP_ERR_INVALID_STATE;
    if (count == 0) return ESP_ERR_INVALID_ARG;
    if (count > 2048) count = 2048;

    uint32_t raw_min = 4096;
    uint32_t raw_max = 0;
    double raw_sum = 0.0;
    double volt_sum = 0.0;
    float range_mult = (s_range == DMM_RANGE_5V) ? 2.0f : 1.0f;

    // Allocate local buffer if caller didn't supply one or for statistics calculation
    uint16_t local_samples[2048];
    uint16_t *buf = out_raw_samples ? out_raw_samples : local_samples;

    for (uint32_t i = 0; i < count; i++) {
        uint32_t raw = adc1_get_raw(s_channel);
        buf[i] = (uint16_t)raw;
        raw_sum += raw;
        if (raw < raw_min) raw_min = raw;
        if (raw > raw_max) raw_max = raw;

        float v = (raw_to_mv(raw) / 1000.0f) * range_mult;
        v = (v * s_cal_gain_scale) + s_cal_offset_v;
        if (v < 0.0f) v = 0.0f;
        volt_sum += v;

        if (interval_us > 0) {
            esp_rom_delay_us(interval_us);
        }
    }

    float raw_mean = (float)(raw_sum / count);
    float volt_mean = (float)(volt_sum / count);

    // Compute standard deviations
    double raw_sq_diff_sum = 0.0;
    double volt_sq_diff_sum = 0.0;
    for (uint32_t i = 0; i < count; i++) {
        double r_diff = (double)buf[i] - raw_mean;
        raw_sq_diff_sum += r_diff * r_diff;

        float v = (raw_to_mv(buf[i]) / 1000.0f) * range_mult;
        v = (v * s_cal_gain_scale) + s_cal_offset_v;
        if (v < 0.0f) v = 0.0f;
        double v_diff = (double)v - volt_mean;
        volt_sq_diff_sum += v_diff * v_diff;
    }

    float raw_stdev = (count > 1) ? sqrtf((float)(raw_sq_diff_sum / (count - 1))) : 0.0f;
    float volt_stdev_mv = (count > 1) ? (sqrtf((float)(volt_sq_diff_sum / (count - 1))) * 1000.0f) : 0.0f;

    float v_min = (raw_to_mv(raw_min) / 1000.0f) * range_mult;
    float v_max = (raw_to_mv(raw_max) / 1000.0f) * range_mult;

    out_stats->sample_count = count;
    out_stats->raw_mean = raw_mean;
    out_stats->raw_stdev = raw_stdev;
    out_stats->raw_min = raw_min;
    out_stats->raw_max = raw_max;
    out_stats->cal_voltage_mean_v = volt_mean;
    out_stats->cal_voltage_stdev_mv = volt_stdev_mv;
    out_stats->cal_voltage_min_v = v_min;
    out_stats->cal_voltage_max_v = v_max;
    out_stats->sample_interval_us = interval_us;

    return ESP_OK;
}

