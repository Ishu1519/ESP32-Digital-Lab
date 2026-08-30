#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t frequency_hz;
    uint8_t duty_cycle_pct;
    int output_gpio;
    bool enabled;
} ledc_gen_status_t;

esp_err_t hal_ledc_gen_init(int gpio_num);
esp_err_t hal_ledc_gen_set_frequency(uint32_t freq_hz, uint8_t duty_pct);
esp_err_t hal_ledc_gen_start(void);
esp_err_t hal_ledc_gen_stop(void);
esp_err_t hal_ledc_gen_deinit(void);
ledc_gen_status_t hal_ledc_gen_get_status(void);

#ifdef __cplusplus
}
#endif
