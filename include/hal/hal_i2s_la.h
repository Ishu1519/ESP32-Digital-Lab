#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LA_TRIGGER_NONE = 0,
    LA_TRIGGER_RISING,
    LA_TRIGGER_FALLING,
    LA_TRIGGER_CHANGE
} la_trigger_mode_t;

typedef struct {
    uint32_t sample_rate_hz;     // e.g. 100000 to 20000000
    uint32_t sample_depth;       // Total samples to capture (e.g. 1024, 2048, 4096, 8192)
    uint8_t num_channels;        // 4 or 8 channels (default 4)
    uint8_t trigger_channel;     // 0 to 7
    la_trigger_mode_t trigger_mode;
    const int *channel_pins;     // Array of GPIO pins for channels
} la_config_t;

typedef struct {
    uint8_t *buffer;             // Raw sample buffer (each sample is 1 byte for <=8 channels)
    uint32_t buffer_len;         // Number of valid samples in buffer
    uint32_t sample_rate_hz;     // Actual sample rate used
    uint32_t trigger_index;      // Sample index where trigger occurred
    bool is_triggered;
    bool is_complete;
} la_capture_result_t;

esp_err_t hal_i2s_la_init(const la_config_t *config);
esp_err_t hal_i2s_la_arm(void);
esp_err_t hal_i2s_la_capture_sync(uint32_t timeout_ms, la_capture_result_t *out_result);
esp_err_t hal_i2s_la_stop(void);
esp_err_t hal_i2s_la_deinit(void);
esp_err_t hal_i2s_la_get_status(bool *out_armed, bool *out_complete);

#ifdef __cplusplus
}
#endif
