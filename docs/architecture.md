# ESP32 Digital Lab — System Architecture

## Overview
The **ESP32 Digital Lab** turns an original ESP32 (`ESP32-D0WD-V3`) into a single-firmware, multi-instrument laboratory tool with a browser-native UI over Wi-Fi.

```
                  ┌─────────────────────────────────────┐
                  │           ORIGINAL ESP32            │
                  │  (ESP32-D0WD-V3 @ 240MHz Dual Core) │
                  │                                     │
                  │  ┌──────────────┐ ┌──────────────┐  │
                  │  │ FreeRTOS     │ │ WiFi SoftAP  │  │
                  │  │ Task Core 0/1│ │ + mDNS Host  │  │
                  │  └──────────────┘ └──────────────┘  │
                  │  ┌──────────────┐ ┌──────────────┐  │
                  │  │ AsyncHTTP    │ │ WebSocket    │  │
                  │  │ Web Server   │ │ Telemetry    │  │
                  │  └──────────────┘ └──────────────┘  │
                  │  ┌─────────────────────────────┐    │
                  │  │ Central Instrument Manager  │    │
                  │  │ (Collision Prevention/HAL)  │    │
                  │  └──────────────┬──────────────┘    │
                  └─────────────────┼───────────────────┘
                                    │
         ┌──────────────────────────┼──────────────────────────┐
         │                          │                          │
         ▼                          ▼                          ▼
 ┌───────────────┐          ┌───────────────┐          ┌───────────────┐
 │ FREQ COUNTER  │          │ LOGIC ANALYZ  │          │  SCOPE / DMM  │
 │ (Milestone 1) │          │  (Phase 3)    │          │  (Phase 7/8)  │
 │ PCNT0 + LEDC0 │          │ I2S0 + DMA    │          │  ADC1 / Timer │
 └───────────────┘          └───────────────┘          └───────────────┘
```

---

## Central Instrument Manager

The ESP32 possesses limited hardware units (e.g. 1 I2S, 1 ADC1 unit, 8 PCNT channels, 8 LEDC channels, 4 Hardware Timers). Different instruments cannot run simultaneously on the same hardware units without corruption.

The `InstrumentManager` enforces strict lifecycle management:
1. **Request**: UI sends mode switch command.
2. **Deactivation**: The currently active instrument's `stop()` and `deinit()` are called, freeing all assigned timers, GPIOs, and peripheral interrupt handlers.
3. **Activation**: The target instrument's `init()` and `start()` are invoked.
4. **Collision Flags**: Peripheral bitmasks ensure hardware ownership is unambiguous.

---

## Frequency Counter Architecture (Milestone 1)

### Hardware Peripherals Used:
* **PCNT (Pulse Counter Unit 0)**: Hardware counter incrementing on rising edges of `GPIO 18`.
* **High Limit ISR (`PCNT_EVT_H_LIM`)**: Triggers an overflow counter when count reaches 20,000 pulses, expanding counting capability beyond 16-bit to 64-bit without missing pulses.
* **Gate Timer (`esp_timer`)**: Hardware 64-bit microsecond timer triggering the gate window (configurable 100 ms to 1000 ms).
* **Reference Signal Generator (LEDC)**: Hardware PWM on `GPIO 19` generating test square waves up to 40 MHz for zero-external-hardware loopback verification.

---

## Web Architecture (<20 KB Embedded)
* **Single Payload**: HTML5, CSS3 Glassmorphic Styling, and Canvas rendering engine are embedded into firmware flash.
* **Zero External Dependencies**: Does not require CDNs, internet access, or external JavaScript packages.
* **WebSocket Streaming**: Asynchronous JSON telemetry broadcasted at 10 Hz refresh rate.
