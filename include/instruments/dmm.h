#pragma once

#include "instrument_manager.h"
#include "hal/hal_adc_dmm.h"

class DmmInstrument : public Instrument {
public:
    DmmInstrument();
    virtual ~DmmInstrument();

    InstrumentType getType() const override { return INST_TYPE_DMM; }
    const char* getName() const override { return "Digital Multimeter"; }

    esp_err_t init() override;
    esp_err_t start() override;
    esp_err_t stop() override;
    esp_err_t deinit() override;

    uint32_t getRequiredPeripherals() const override;
    uint64_t getRequiredPinsMask() const override;
    bool isRunning() const override { return m_running; }

    void getTelemetryJson(JsonObject &root) override;
    esp_err_t handleCommand(const JsonObject &cmd) override;

private:
    bool m_running;
    dmm_range_t m_range;
    uint32_t m_oversample_count;
    dmm_measurement_t m_last_meas;
    uint32_t m_reading_count;
    bool m_stream_ready;
    dmm_raw_stream_stats_t m_last_stream_stats;
};
