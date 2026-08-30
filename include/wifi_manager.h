#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

class WiFiManager {
public:
    static WiFiManager& getInstance();

    esp_err_t init();
    bool isConnected();
    const char* getIPAddress();
    const char* getAPIPAddress();

private:
    WiFiManager();
    ~WiFiManager();
};
