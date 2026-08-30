#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

typedef enum {
    DMM_RANGE_3V3 = 0, // Direct 0 - 3.3V (1:1 scaling)
    DMM_RANGE_5V  = 1  // Scaled 0 - 5.0V (2:1 divider, 2.0x scaling)
} dmm_range_t;

typedef struct {
    float voltage_v;             // Calibrated DC voltage in Volts
    uint32_t raw_adc_avg;        // Average raw ADC reading (0-4095)
    uint32_t raw_adc_min;        // Minimum raw ADC reading in batch
    uint32_t raw_adc_max;        // Maximum raw ADC reading in batch
    float noise_std_dev_mv;      // Noise RMS standard deviation in mV
    float v_min_v;               // Minimum voltage observed in sample burst
    float v_max_v;               // Maximum voltage observed in sample burst
    float v_pp_mv;               // Peak-to-peak ripple/noise in mV
    uint32_t num_samples;        // Oversampling count
    dmm_range_t range;           // Active range
    bool efuse_cal_applied;      // True if ESP32 eFuse calibration active
} dmm_measurement_t;

typedef struct {
    uint32_t sample_count;
    float raw_mean;
    float raw_stdev;
    uint32_t raw_min;
    uint32_t raw_max;
    float cal_voltage_mean_v;
    float cal_voltage_stdev_mv;
    float cal_voltage_min_v;
    float cal_voltage_max_v;
    uint32_t sample_interval_us;
} dmm_raw_stream_stats_t;

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t hal_adc_dmm_init(adc1_channel_t channel);
esp_err_t hal_adc_dmm_deinit(void);
esp_err_t hal_adc_dmm_set_range(dmm_range_t range);
esp_err_t hal_adc_dmm_set_oversample(uint32_t samples);
esp_err_t hal_adc_dmm_read(dmm_measurement_t *out_meas);
esp_err_t hal_adc_dmm_set_calibration(float offset_v, float gain_scale);
esp_err_t hal_adc_dmm_get_efuse_info(bool *out_has_efuse, uint32_t *out_vref_mv);
esp_err_t hal_adc_dmm_capture_raw_stream(uint32_t count, uint32_t interval_us, dmm_raw_stream_stats_t *out_stats, uint16_t *out_raw_samples);

#ifdef __cplusplus
}
#endif
