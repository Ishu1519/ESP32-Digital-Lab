#include "instruments/freq_counter.h"
#include <Arduino.h>
#include <math.h>
#include "esp_log.h"
#include "system_config.h"

static const char *TAG = "INST_FREQ";

FrequencyCounterInstrument::FrequencyCounterInstrument()
    : m_running(false),
      m_gate_time_ms(500),
      m_input_gpio(PIN_FREQ_INPUT),
      m_ref_gpio(PIN_REF_GEN_OUTPUT),
      m_min_freq(0),
      m_max_freq(0),
      m_sum_freq(0),
      m_sum_sq_freq(0),
      m_stat_samples(0) {
}

FrequencyCounterInstrument::~FrequencyCounterInstrument() {
    deinit();
}

esp_err_t FrequencyCounterInstrument::init() {
    pcnt_config_params_t config = {
        .input_gpio = m_input_gpio,
        .gate_time_ms = m_gate_time_ms,
        .filter_val = 0 // 0 = Full bandwidth (DC to 40 MHz)
    };
    esp_err_t err = hal_pcnt_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "hal_pcnt_init failed: %s", esp_err_to_name(err));
        return err;
    }

    // Initialize the test signal generator on GPIO 19
    hal_ledc_gen_init(m_ref_gpio);
    hal_ledc_gen_set_frequency(10000, 50); // Default 10 kHz 50% duty
    hal_ledc_gen_start();

    resetStats();
    return ESP_OK;
}

esp_err_t FrequencyCounterInstrument::start() {
    esp_err_t err = hal_pcnt_start();
    if (err == ESP_OK) {
        m_running = true;
    }
    return err;
}

esp_err_t FrequencyCounterInstrument::stop() {
    hal_pcnt_stop();
    m_running = false;
    return ESP_OK;
}

esp_err_t FrequencyCounterInstrument::deinit() {
    stop();
    hal_pcnt_deinit();
    hal_ledc_gen_deinit();
    return ESP_OK;
}

uint32_t FrequencyCounterInstrument::getRequiredPeripherals() const {
    return PERIPH_PCNT0 | PERIPH_LEDC0;
}

uint64_t FrequencyCounterInstrument::getRequiredPinsMask() const {
    return (1ULL << m_input_gpio) | (1ULL << m_ref_gpio);
}

void FrequencyCounterInstrument::resetStats() {
    m_min_freq = 0;
    m_max_freq = 0;
    m_sum_freq = 0;
    m_sum_sq_freq = 0;
    m_stat_samples = 0;
}

void FrequencyCounterInstrument::updateStats(double freq) {
    if (freq <= 0) return;

    if (m_stat_samples == 0) {
        m_min_freq = freq;
        m_max_freq = freq;
    } else {
        if (freq < m_min_freq) m_min_freq = freq;
        if (freq > m_max_freq) m_max_freq = freq;
    }

    m_sum_freq += freq;
    m_sum_sq_freq += (freq * freq);
    m_stat_samples++;
}

void FrequencyCounterInstrument::getTelemetryJson(JsonObject &root) {
    pcnt_measurement_t meas = {0};
    hal_pcnt_get_measurement(&meas);

    if (meas.signal_detected) {
        updateStats(meas.frequency_hz);
    }

    root["freq_hz"] = meas.frequency_hz;
    root["period_us"] = meas.period_us;
    root["duty_pct"] = meas.duty_cycle_pct;
    root["pulse_width_us"] = meas.pulse_width_us;
    root["total_pulses"] = (uint32_t)meas.total_pulses;
    root["signal_detected"] = meas.signal_detected;
    root["gate_time_ms"] = m_gate_time_ms;
    root["input_gpio"] = m_input_gpio;
    root["ref_gpio"] = m_ref_gpio;

    // Stats
    JsonObject stats = root.createNestedObject("stats");
    stats["samples"] = m_stat_samples;
    if (m_stat_samples > 0) {
        double mean = m_sum_freq / m_stat_samples;
        double variance = (m_sum_sq_freq / m_stat_samples) - (mean * mean);
        double std_dev = (variance > 0) ? sqrt(variance) : 0.0;
        stats["min_hz"] = m_min_freq;
        stats["max_hz"] = m_max_freq;
        stats["mean_hz"] = mean;
        stats["std_dev_hz"] = std_dev;
    } else {
        stats["min_hz"] = 0;
        stats["max_hz"] = 0;
        stats["mean_hz"] = 0;
        stats["std_dev_hz"] = 0;
    }

    // Reference generator status
    ledc_gen_status_t gen = hal_ledc_gen_get_status();
    JsonObject genObj = root.createNestedObject("ref_gen");
    genObj["enabled"] = gen.enabled;
    genObj["freq_hz"] = gen.frequency_hz;
    genObj["duty_pct"] = gen.duty_cycle_pct;
    genObj["gpio"] = gen.output_gpio;
    genObj["res_bits"] = gen.res_bits;
    genObj["div_param"] = gen.div_param;
    genObj["calc_freq_hz"] = gen.calc_freq_hz;
}

esp_err_t FrequencyCounterInstrument::handleCommand(const JsonObject &cmd) {
    const char *action = cmd["action"] | cmd["cmd"];
    if (!action) return ESP_ERR_INVALID_ARG;

    if (strcmp(action, "set_gate_time") == 0) {
        uint32_t gate_ms = cmd["gate_time_ms"] | 500;
        m_gate_time_ms = gate_ms;
        return hal_pcnt_set_gate_time(gate_ms);
    } else if (strcmp(action, "reset_stats") == 0) {
        resetStats();
        return ESP_OK;
    } else if (strcmp(action, "set_ref_gen") == 0) {
        uint32_t freq = cmd["freq_hz"] | 1000;
        uint8_t duty = cmd["duty_pct"] | 50;
        bool enabled = cmd.containsKey("enabled") ? (bool)cmd["enabled"] : true;

        hal_ledc_gen_set_frequency(freq, duty);
        if (enabled) {
            hal_ledc_gen_start();
        } else {
            hal_ledc_gen_stop();
        }
        return ESP_OK;
    }

    return ESP_ERR_NOT_SUPPORTED;
}
