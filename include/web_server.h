#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

class WebServerManager {
public:
    static WebServerManager& getInstance();

    esp_err_t init();
    void broadcastTelemetry();
    size_t getConnectedClientsCount();

private:
    WebServerManager();
    ~WebServerManager();
};
