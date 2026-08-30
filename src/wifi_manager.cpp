#include "wifi_manager.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include "esp_log.h"
#include "system_config.h"

static const char *TAG = "WIFI_MGR";

WiFiManager& WiFiManager::getInstance() {
    static WiFiManager instance;
    return instance;
}

WiFiManager::WiFiManager() {}
WiFiManager::~WiFiManager() {}

esp_err_t WiFiManager::init() {
    ESP_LOGI(TAG, "Starting Wi-Fi Access Point: %s", LAB_WIFI_AP_SSID);

    WiFi.mode(WIFI_AP_STA);
    bool ap_ok = WiFi.softAP(LAB_WIFI_AP_SSID, LAB_WIFI_AP_PASS);
    if (!ap_ok) {
        ESP_LOGE(TAG, "Failed to start SoftAP");
        return ESP_FAIL;
    }

    IPAddress apIP = WiFi.softAPIP();
    ESP_LOGI(TAG, "AP Started. IP: %s", apIP.toString().c_str());

    // Start mDNS
    if (MDNS.begin(LAB_MDNS_HOST)) {
        MDNS.addService("http", "tcp", LAB_HTTP_PORT);
        ESP_LOGI(TAG, "mDNS responder started at http://%s.local", LAB_MDNS_HOST);
    } else {
        ESP_LOGW(TAG, "mDNS responder failed to start");
    }

    return ESP_OK;
}

bool WiFiManager::isConnected() {
    return (WiFi.getMode() == WIFI_AP || WiFi.status() == WL_CONNECTED);
}

const char* WiFiManager::getIPAddress() {
    static char buf[32];
    snprintf(buf, sizeof(buf), "%s", WiFi.localIP().toString().c_str());
    return buf;
}

const char* WiFiManager::getAPIPAddress() {
    static char buf[32];
    snprintf(buf, sizeof(buf), "%s", WiFi.softAPIP().toString().c_str());
    return buf;
}
