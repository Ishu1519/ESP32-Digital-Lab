#include <Arduino.h>
#include "esp_log.h"
#include "system_config.h"
#include "instrument_manager.h"
#include "instruments/freq_counter.h"
#include "instruments/logic_analyzer.h"
#include "instruments/dmm.h"
#include "wifi_manager.h"
#include "web_server.h"

static const char *TAG = "MAIN";

static FrequencyCounterInstrument s_freq_counter_inst;
static LogicAnalyzerInstrument s_logic_analyzer_inst;
static DmmInstrument s_dmm_inst;

void telemetry_task(void *pvParameters) {
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t interval = pdMS_TO_TICKS(TELEMETRY_INTERVAL_MS);

    while (1) {
        WebServerManager::getInstance().broadcastTelemetry();
        vTaskDelayUntil(&last_wake_time, interval);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setTimeout(50);
    delay(1000);

    Serial.println("\n=======================================================");
    Serial.println("       ESP32 DIGITAL LAB — ALL-IN-ONE INSTRUMENT       ");
    Serial.println("=======================================================");
    Serial.printf("Target MCU:     ESP32 (Original)\n");
    Serial.printf("Firmware:       v%d.%d.%d (%s)\n", 
                  LAB_FW_VERSION_MAJOR, LAB_FW_VERSION_MINOR, LAB_FW_VERSION_PATCH, LAB_FW_BUILD_STR);
    Serial.printf("Free Heap:      %d bytes\n", ESP.getFreeHeap());
    Serial.println("-------------------------------------------------------");

    // 1. Initialize Instrument Manager
    InstrumentManager &instMgr = InstrumentManager::getInstance();
    instMgr.init();

    // 2. Register Instruments
    instMgr.registerInstrument(&s_freq_counter_inst);
    instMgr.registerInstrument(&s_logic_analyzer_inst);
    instMgr.registerInstrument(&s_dmm_inst);

    // 3. Activate Default Instrument (Frequency Counter)
    ESP_LOGI(TAG, "Activating Frequency Counter...");
    instMgr.switchInstrument(INST_TYPE_FREQ_COUNTER);

    // 4. Initialize Wi-Fi AP & mDNS
    WiFiManager::getInstance().init();

    // 5. Initialize HTTP & WebSocket Server
    WebServerManager::getInstance().init();

    // 6. Start FreeRTOS Telemetry Streaming Task
    xTaskCreatePinnedToCore(
        telemetry_task,
        "telemetry_task",
        TASK_STACK_WEBSOCKET,
        NULL,
        TASK_PRIO_WEBSOCKET,
        NULL,
        1 // Pin to Core 1
    );

    Serial.println("-------------------------------------------------------");
    Serial.println("System Ready!");
    Serial.printf("Connect to Wi-Fi AP:    '%s' (Pass: '%s')\n", LAB_WIFI_AP_SSID, LAB_WIFI_AP_PASS);
    Serial.printf("Web Dashboard URL:      http://%s or http://%s.local\n", WiFiManager::getInstance().getAPIPAddress(), LAB_MDNS_HOST);
    Serial.println("=======================================================\n");
}

void loop() {
    // Process serial JSON commands from benchmark runner or serial terminal
    if (Serial.available() > 0) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            DynamicJsonDocument doc(16384);
            DeserializationError err = deserializeJson(doc, line);
            if (!err) {
                JsonObject root = doc.as<JsonObject>();
                InstrumentManager::getInstance().dispatchCommand(root);
                
                // Immediately respond with latest telemetry
                DynamicJsonDocument resp(16384);
                InstrumentManager::getInstance().buildTelemetryPacket(resp);
                serializeJson(resp, Serial);
                Serial.println();
            } else if (line == "READ" || line == "STATUS") {
                DynamicJsonDocument resp(16384);
                InstrumentManager::getInstance().buildTelemetryPacket(resp);
                serializeJson(resp, Serial);
                Serial.println();
            }
        }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
}
