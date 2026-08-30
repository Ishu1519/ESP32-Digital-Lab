#include "web_server.h"
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include "esp_log.h"
#include "web_assets.h"
#include "instrument_manager.h"
#include "system_config.h"

static const char *TAG = "WEB_SRV";

static AsyncWebServer s_server(LAB_HTTP_PORT);
static AsyncWebSocket s_ws("/ws");

static void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                           AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        ESP_LOGI(TAG, "WS Client #%u connected from %s", client->id(), client->remoteIP().toString().c_str());
    } else if (type == WS_EVT_DISCONNECT) {
        ESP_LOGI(TAG, "WS Client #%u disconnected", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            data[len] = 0;
            StaticJsonDocument<512> doc;
            DeserializationError err = deserializeJson(doc, (char*)data);
            if (!err) {
                JsonObject root = doc.as<JsonObject>();
                InstrumentManager::getInstance().dispatchCommand(root);
            } else {
                ESP_LOGW(TAG, "JSON parse error: %s", err.c_str());
            }
        }
    }
}

WebServerManager& WebServerManager::getInstance() {
    static WebServerManager instance;
    return instance;
}

WebServerManager::WebServerManager() {}
WebServerManager::~WebServerManager() {}

esp_err_t WebServerManager::init() {
    ESP_LOGI(TAG, "Initializing Web and WebSocket Server...");

    s_ws.onEvent(onWebSocketEvent);
    s_server.addHandler(&s_ws);

    // Root page
    s_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", INDEX_HTML);
    });

    // REST API - Status
    s_server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
        StaticJsonDocument<512> doc;
        InstrumentManager::getInstance().buildTelemetryPacket(doc);
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    s_server.begin();
    ESP_LOGI(TAG, "HTTP Server started on port %d", LAB_HTTP_PORT);
    return ESP_OK;
}

void WebServerManager::broadcastTelemetry() {
    if (s_ws.count() == 0) return;

    StaticJsonDocument<1024> doc;
    InstrumentManager::getInstance().buildTelemetryPacket(doc);

    char buffer[1024];
    size_t len = serializeJson(doc, buffer, sizeof(buffer));
    if (len > 0) {
        s_ws.textAll(buffer, len);
    }
}

size_t WebServerManager::getConnectedClientsCount() {
    return s_ws.count();
}
