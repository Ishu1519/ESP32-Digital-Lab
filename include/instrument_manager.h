#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <ArduinoJson.h>
#include "esp_err.h"

enum InstrumentType {
    INST_TYPE_NONE = 0,
    INST_TYPE_FREQ_COUNTER,
    INST_TYPE_SIGNAL_GEN,
    INST_TYPE_LOGIC_ANALYZER,
    INST_TYPE_OSCILLOSCOPE,
    INST_TYPE_DMM
};

// Peripheral allocation bitmask for collision detection
enum PeripheralFlags {
    PERIPH_NONE     = 0,
    PERIPH_PCNT0    = 1 << 0,
    PERIPH_LEDC0    = 1 << 1,
    PERIPH_I2S0     = 1 << 2,
    PERIPH_ADC1     = 1 << 3,
    PERIPH_DAC      = 1 << 4,
    PERIPH_RMT      = 1 << 5,
    PERIPH_TIMER0   = 1 << 6
};

class Instrument {
public:
    virtual ~Instrument() {}
    virtual const char* getName() const = 0;
    virtual InstrumentType getType() const = 0;
    virtual esp_err_t init() = 0;
    virtual esp_err_t start() = 0;
    virtual esp_err_t stop() = 0;
    virtual esp_err_t deinit() = 0;
    virtual void getTelemetryJson(JsonObject &root) = 0;
    virtual esp_err_t handleCommand(const JsonObject &cmd) = 0;
    virtual uint32_t getRequiredPeripherals() const = 0;
    virtual uint64_t getRequiredPinsMask() const = 0;
    virtual bool isRunning() const = 0;
};

class InstrumentManager {
public:
    static InstrumentManager& getInstance();

    esp_err_t init();
    esp_err_t registerInstrument(Instrument *inst);
    esp_err_t switchInstrument(InstrumentType type);
    Instrument* getActiveInstrument();
    Instrument* getInstrument(InstrumentType type);

    // Global telemetry packet (system health + active instrument data)
    void buildTelemetryPacket(JsonDocument &doc);
    esp_err_t dispatchCommand(const JsonObject &cmd);

    uint32_t getActivePeripheralsMask() const { return m_active_peripherals; }

private:
    InstrumentManager();
    ~InstrumentManager();

    Instrument* m_instruments[8];
    int m_num_instruments;
    Instrument* m_active_instrument;
    uint32_t m_active_peripherals;
    uint64_t m_active_pins_mask;
};
