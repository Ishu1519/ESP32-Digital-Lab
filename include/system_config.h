#pragma once

#include <stdint.h>

// Firmware version
#define LAB_FW_VERSION_MAJOR 1
#define LAB_FW_VERSION_MINOR 0
#define LAB_FW_VERSION_PATCH 0
#define LAB_FW_BUILD_STR     "Milestone 1 - Frequency Counter & Lab UI"

// Network configuration
#define LAB_WIFI_AP_SSID     "ESP32-Digital-Lab"
#define LAB_WIFI_AP_PASS     "digitallab123"
#define LAB_MDNS_HOST        "esp32"
#define LAB_HTTP_PORT        80
#define LAB_WS_PORT          80

// Pin Mapping for Original ESP32
// Frequency Counter
#define PIN_FREQ_INPUT       18  // PCNT Pulse input (GPIO 18)
#define PIN_REF_GEN_OUTPUT   19  // Reference test signal generator output (GPIO 19)

// Logic Analyzer channels (Milestone 2 ready)
#define PIN_LOGIC_CH0        4
#define PIN_LOGIC_CH1        5
#define PIN_LOGIC_CH2        12
#define PIN_LOGIC_CH3        13
#define PIN_LOGIC_CH4        14
#define PIN_LOGIC_CH5        15
#define PIN_LOGIC_CH6        16
#define PIN_LOGIC_CH7        17

// Analog / Scope / DMM channels (Milestone 3/4 ready)
#define PIN_ADC_SCOPE_CH1    34  // ADC1_CH6 (Input only)
#define PIN_ADC_SCOPE_CH2    35  // ADC1_CH7 (Input only)
#define PIN_ADC_DMM          36  // ADC1_CH0 (SENSOR_VP, Input only)

// DAC Analog Output
#define PIN_DAC_OUT1         25
#define PIN_DAC_OUT2         26

// FreeRTOS Task Priorities & Stack Sizes
#define TASK_PRIO_INSTRUMENT 5
#define TASK_PRIO_WEBSOCKET  2
#define TASK_STACK_INSTRUMENT 4096
#define TASK_STACK_WEBSOCKET  4096

// WebSocket telemetry streaming rate
#define TELEMETRY_INTERVAL_MS 100 // 10 Hz refresh
