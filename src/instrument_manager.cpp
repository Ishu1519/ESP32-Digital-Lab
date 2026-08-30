#include "instrument_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include "esp_log.h"
#include "system_config.h"

static const char *TAG = "INST_MGR";

InstrumentManager& InstrumentManager::getInstance() {
    static InstrumentManager instance;
    return instance;
}

InstrumentManager::InstrumentManager()
    : m_num_instruments(0),
      m_active_instrument(nullptr),
      m_active_peripherals(PERIPH_NONE),
      m_active_pins_mask(0) {
    for (int i = 0; i < 8; i++) {
        m_instruments[i] = nullptr;
    }
}

InstrumentManager::~InstrumentManager() {
}

esp_err_t InstrumentManager::init() {
    ESP_LOGI(TAG, "Initializing Instrument Manager...");
    return ESP_OK;
}

esp_err_t InstrumentManager::registerInstrument(Instrument *inst) {
    if (!inst || m_num_instruments >= 8) {
        return ESP_ERR_INVALID_ARG;
    }
    m_instruments[m_num_instruments++] = inst;
    ESP_LOGI(TAG, "Registered instrument: %s (Type %d)", inst->getName(), (int)inst->getType());
    return ESP_OK;
}

Instrument* InstrumentManager::getInstrument(InstrumentType type) {
    for (int i = 0; i < m_num_instruments; i++) {
        if (m_instruments[i]->getType() == type) {
            return m_instruments[i];
        }
    }
    return nullptr;
}

Instrument* InstrumentManager::getActiveInstrument() {
    return m_active_instrument;
}

esp_err_t InstrumentManager::switchInstrument(InstrumentType type) {
    if (type == INST_TYPE_NONE) {
        if (m_active_instrument) {
            ESP_LOGI(TAG, "Stopping active instrument: %s", m_active_instrument->getName());
            m_active_instrument->stop();
            m_active_instrument->deinit();
            m_active_instrument = nullptr;
            m_active_peripherals = PERIPH_NONE;
            m_active_pins_mask = 0;
        }
        return ESP_OK;
    }

    Instrument *target = getInstrument(type);
    if (!target) {
        ESP_LOGE(TAG, "Target instrument %d not registered", (int)type);
        return ESP_ERR_NOT_FOUND;
    }

    if (m_active_instrument == target) {
        return ESP_OK; // already active
    }

    // Step 1: Cleanly stop and deinit current active instrument
    if (m_active_instrument) {
        ESP_LOGI(TAG, "Deactivating current instrument: %s", m_active_instrument->getName());
        m_active_instrument->stop();
        m_active_instrument->deinit();
        m_active_instrument = nullptr;
        m_active_peripherals = PERIPH_NONE;
        m_active_pins_mask = 0;
    }

    // Step 2: Check peripheral resources for target
    uint32_t req_periph = target->getRequiredPeripherals();
    uint64_t req_pins = target->getRequiredPinsMask();

    ESP_LOGI(TAG, "Activating instrument: %s (Periph: 0x%08lX, Pins: 0x%llX)", 
             target->getName(), (unsigned long)req_periph, req_pins);

    // Step 3: Initialize and start target
    esp_err_t err = target->init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init instrument %s: %s", target->getName(), esp_err_to_name(err));
        return err;
    }

    err = target->start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start instrument %s: %s", target->getName(), esp_err_to_name(err));
        target->deinit();
        return err;
    }

    m_active_instrument = target;
    m_active_peripherals = req_periph;
    m_active_pins_mask = req_pins;

    ESP_LOGI(TAG, "Instrument %s activated successfully", target->getName());
    return ESP_OK;
}

void InstrumentManager::buildTelemetryPacket(JsonDocument &doc) {
    doc.clear();
    doc["type"] = "telemetry";
    doc["timestamp_ms"] = millis();

    // System stats
    JsonObject sys = doc.createNestedObject("system");
    sys["free_heap"] = ESP.getFreeHeap();
    sys["min_heap"] = ESP.getMinFreeHeap();
    sys["uptime_s"] = millis() / 1000;
    sys["wifi_mode"] = WiFi.getMode() == WIFI_AP ? "AP" : (WiFi.getMode() == WIFI_STA ? "STA" : "AP_STA");
    sys["wifi_ip"] = WiFi.getMode() == WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    sys["active_periph_mask"] = m_active_peripherals;

    // Active Instrument data
    JsonObject instObj = doc.createNestedObject("instrument");
    if (m_active_instrument) {
        instObj["type"] = (int)m_active_instrument->getType();
        instObj["name"] = m_active_instrument->getName();
        instObj["running"] = m_active_instrument->isRunning();
        JsonObject data = instObj.createNestedObject("data");
        m_active_instrument->getTelemetryJson(data);
    } else {
        instObj["type"] = (int)INST_TYPE_NONE;
        instObj["name"] = "IDLE";
        instObj["running"] = false;
    }
}

esp_err_t InstrumentManager::dispatchCommand(const JsonObject &cmd) {
    if (cmd.containsKey("cmd")) {
        const char *cmd_str = cmd["cmd"];
        if (strcmp(cmd_str, "switch_mode") == 0) {
            int target_type = cmd["mode"] | 0;
            return switchInstrument((InstrumentType)target_type);
        }
    }

    if (m_active_instrument) {
        return m_active_instrument->handleCommand(cmd);
    }

    return ESP_ERR_INVALID_STATE;
}
