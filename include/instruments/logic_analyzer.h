#pragma once

#include "instrument_manager.h"
#include "hal/hal_i2s_la.h"
#include "hal/hal_ledc_gen.h"

class LogicAnalyzerInstrument : public Instrument {
public:
    LogicAnalyzerInstrument();
    virtual ~LogicAnalyzerInstrument();

    InstrumentType getType() const override { return INST_TYPE_LOGIC_ANALYZER; }
    const char* getName() const override { return "Logic Analyzer"; }

    esp_err_t init() override;
    esp_err_t start() override;
    esp_err_t stop() override;
    esp_err_t deinit() override;

    uint32_t getRequiredPeripherals() const override;
    uint64_t getRequiredPinsMask() const override;
    bool isRunning() const override { return m_running; }

    void getTelemetryJson(JsonObject &root) override;
    esp_err_t handleCommand(const JsonObject &cmd) override;

    esp_err_t captureTriggered();

private:
    bool m_running;
    uint32_t m_sample_rate_hz;
    uint32_t m_sample_depth;
    uint8_t m_num_channels;
    uint8_t m_trigger_channel;
    la_trigger_mode_t m_trigger_mode;
    
    int m_channel_pins[8];
    uint8_t *m_last_capture_buf;
    uint32_t m_last_capture_len;
    uint32_t m_capture_seq;
    bool m_continuous_mode;

    uint32_t m_gen_freq_hz;
    uint8_t m_gen_duty_pct;
    bool m_gen_enabled;
};
