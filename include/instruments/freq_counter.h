#pragma once

#include "instrument_manager.h"
#include "hal/hal_pcnt.h"
#include "hal/hal_ledc_gen.h"

class FrequencyCounterInstrument : public Instrument {
public:
    FrequencyCounterInstrument();
    virtual ~FrequencyCounterInstrument();

    virtual const char* getName() const override { return "Frequency Counter"; }
    virtual InstrumentType getType() const override { return INST_TYPE_FREQ_COUNTER; }
    virtual esp_err_t init() override;
    virtual esp_err_t start() override;
    virtual esp_err_t stop() override;
    virtual esp_err_t deinit() override;
    virtual void getTelemetryJson(JsonObject &root) override;
    virtual esp_err_t handleCommand(const JsonObject &cmd) override;
    virtual uint32_t getRequiredPeripherals() const override;
    virtual uint64_t getRequiredPinsMask() const override;
    virtual bool isRunning() const override { return m_running; }

private:
    bool m_running;
    uint32_t m_gate_time_ms;
    int m_input_gpio;
    int m_ref_gpio;

    // Running statistics
    double m_min_freq;
    double m_max_freq;
    double m_sum_freq;
    double m_sum_sq_freq;
    uint32_t m_stat_samples;

    void resetStats();
    void updateStats(double freq);
};
