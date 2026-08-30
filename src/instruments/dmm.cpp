#include "instruments/dmm.h"
#include <Arduino.h>
#include "esp_log.h"
#include "system_config.h"

static const char *TAG = "INST_DMM";

DmmInstrument::DmmInstrument()
    : m_running(false),
      m_range(DMM_RANGE_3V3),
      m_oversample_count(128),
      m_reading_count(0),
      m_stream_ready(false) {
    memset(&m_last_meas, 0, sizeof(m_last_meas));
    memset(&m_last_stream_stats, 0, sizeof(m_last_stream_stats));
}

DmmInstrument::~DmmInstrument() {
    deinit();
}

esp_err_t DmmInstrument::init() {
    esp_err_t err = hal_adc_dmm_init(ADC1_CHANNEL_6); // GPIO 34
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "hal_adc_dmm_init failed: %s", esp_err_to_name(err));
        return err;
    }
    hal_adc_dmm_set_range(m_range);
    hal_adc_dmm_set_oversample(m_oversample_count);
    return ESP_OK;
}

esp_err_t DmmInstrument::start() {
    m_running = true;
    return ESP_OK;
}

esp_err_t DmmInstrument::stop() {
    m_running = false;
    return ESP_OK;
}

esp_err_t DmmInstrument::deinit() {
    stop();
    return hal_adc_dmm_deinit();
}

uint32_t DmmInstrument::getRequiredPeripherals() const {
    return PERIPH_ADC1;
}

uint64_t DmmInstrument::getRequiredPinsMask() const {
    return (1ULL << PIN_ADC_DMM);
}

void DmmInstrument::getTelemetryJson(JsonObject &root) {
    if (m_running) {
        esp_err_t err = hal_adc_dmm_read(&m_last_meas);
        if (err == ESP_OK) {
            m_reading_count++;
        }
    }

    root["voltage_v"] = m_last_meas.voltage_v;
    root["voltage_mv"] = m_last_meas.voltage_v * 1000.0f;
    root["raw_adc"] = m_last_meas.raw_adc_avg;
    root["raw_min"] = m_last_meas.raw_adc_min;
    root["raw_max"] = m_last_meas.raw_adc_max;
    root["noise_std_dev_mv"] = m_last_meas.noise_std_dev_mv;
    root["v_min_v"] = m_last_meas.v_min_v;
    root["v_max_v"] = m_last_meas.v_max_v;
    root["v_pp_mv"] = m_last_meas.v_pp_mv;
    root["range"] = (m_range == DMM_RANGE_5V) ? "0-5V" : "0-3.3V";
    root["samples"] = m_last_meas.num_samples;
    root["efuse_cal"] = m_last_meas.efuse_cal_applied;
    root["reading_seq"] = m_reading_count;
    root["gpio"] = PIN_ADC_DMM;

    if (m_stream_ready) {
        JsonObject s = root.createNestedObject("raw_stream_stats");
        s["count"] = m_last_stream_stats.sample_count;
        s["raw_mean"] = m_last_stream_stats.raw_mean;
        s["raw_stdev"] = m_last_stream_stats.raw_stdev;
        s["raw_min"] = m_last_stream_stats.raw_min;
        s["raw_max"] = m_last_stream_stats.raw_max;
        s["cal_voltage_mean_v"] = m_last_stream_stats.cal_voltage_mean_v;
        s["cal_voltage_stdev_mv"] = m_last_stream_stats.cal_voltage_stdev_mv;
        s["cal_voltage_min_v"] = m_last_stream_stats.cal_voltage_min_v;
        s["cal_voltage_max_v"] = m_last_stream_stats.cal_voltage_max_v;
        s["interval_us"] = m_last_stream_stats.sample_interval_us;
    }
}

esp_err_t DmmInstrument::handleCommand(const JsonObject &cmd) {
    const char *action = cmd["action"] | cmd["cmd"];
    if (!action) return ESP_ERR_INVALID_ARG;

    if (strcmp(action, "set_range") == 0) {
        const char *r_str = cmd["range"] | "0-3.3V";
        if (strcmp(r_str, "0-5V") == 0 || strcmp(r_str, "5V") == 0 || cmd["range_id"] == 1) {
            m_range = DMM_RANGE_5V;
        } else {
            m_range = DMM_RANGE_3V3;
        }
        return hal_adc_dmm_set_range(m_range);
    } else if (strcmp(action, "set_oversample") == 0) {
        uint32_t samples = cmd["samples"] | 128;
        m_oversample_count = samples;
        return hal_adc_dmm_set_oversample(samples);
    } else if (strcmp(action, "calibrate") == 0) {
        float offset = cmd["offset_v"] | 0.0f;
        float gain = cmd["gain_scale"] | 1.0f;
        return hal_adc_dmm_set_calibration(offset, gain);
    } else if (strcmp(action, "get_raw_stream") == 0) {
        uint32_t count = cmd["count"] | 1000;
        uint32_t interval = cmd["interval_us"] | 50;
        dmm_raw_stream_stats_t stats;
        esp_err_t err = hal_adc_dmm_capture_raw_stream(count, interval, &stats, NULL);
        if (err == ESP_OK) {
            m_last_stream_stats = stats;
            m_stream_ready = true;
        }
        return err;
    }

    return ESP_ERR_NOT_SUPPORTED;
}
