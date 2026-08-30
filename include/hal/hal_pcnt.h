#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/pcnt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double frequency_hz;
    double period_us;
    double duty_cycle_pct;
    double pulse_width_us;
    uint64_t total_pulses;
    uint32_t gate_time_ms;
    bool signal_detected;
    uint32_t samples_count;
} pcnt_measurement_t;

typedef struct {
    int input_gpio;
    uint32_t gate_time_ms;   // Gate time in ms (e.g. 100, 200, 500, 1000)
    int16_t filter_val;      // PCNT filter (0 to 1023 APB clock cycles)
} pcnt_config_params_t;

esp_err_t hal_pcnt_init(const pcnt_config_params_t *config);
esp_err_t hal_pcnt_start(void);
esp_err_t hal_pcnt_stop(void);
esp_err_t hal_pcnt_deinit(void);
esp_err_t hal_pcnt_get_measurement(pcnt_measurement_t *out_meas);
esp_err_t hal_pcnt_set_gate_time(uint32_t gate_time_ms);

#ifdef __cplusplus
}
#endif
