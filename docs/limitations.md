# ESP32 Digital Lab — Limitations & Specifications (Milestone 1)

Honest engineering specifications, theoretical limits, and tested operational parameters on the original ESP32-D0WD-V3.

---

## 1. Frequency Counter & Pulse Analyzer

| Parameter | Theoretical Maximum | Recommended Range | Engineering Justification & Behavior |
| :--- | :--- | :--- | :--- |
| **High Frequency Range** | 40.0 MHz | 10 Hz – 25 MHz | APB Clock is 80 MHz ($\text{APB}/2 = 40\,\text{MHz}$). High-speed square waves above 25 MHz suffer duty distortion from PCB trace capacitance and GPIO matrix input delay unless driven with low-impedance 50 $\Omega$ source. |
| **Low Frequency Range (1 Hz – 5 kHz)** | 0.01 Hz | 0.5 Hz – 5 kHz | Handled via **Reciprocal Period Timing** ($T = t_2 - t_1$). Fixed gate counting at 1 Hz produces severe quantization; reciprocal timing yields sub-0.01% error down to 1 Hz. |
| **Pulse Width & Duty Cycle** | 500 kHz max for duty | 1 Hz – 250 kHz | Measured via GPIO rising/falling edge capture with $1\,\mu\text{s}$ timer resolution. Above 250 kHz, interrupt latency ($\sim 1.5\,\mu\text{s}$) becomes significant. |
| **Input Voltage Tolerances** | 0.0 V – 3.3 V | 0.0 V – 3.3 V | **Direct ESP32 GPIO input is NOT 5V tolerant.** Overvoltage will destroy the input protection diodes. An external resistor divider / clamp is mandatory for >3.3V circuits. |
| **Gate Accuracy** | $\pm 10\,\text{ppm}$ | $\pm 25\,\text{ppm}$ | Driven by the onboard 40 MHz crystal oscillator. |

---

## 2. Test Reference Signal Generator (GPIO 19)

| Frequency Range | Generation Mechanism | Max Resolution | Notes |
| :--- | :--- | :--- | :--- |
| **1 Hz – 49 Hz** | 64-bit microsecond `esp_timer` toggle | $1\,\mu\text{s}$ step | LEDC APB prescaler (max 1023.99) cannot achieve $<50\,\text{Hz}$ with 13-bit timer. Software timer toggle provides clean sub-50Hz square waves. |
| **50 Hz – 5 kHz** | LEDC High-Speed Timer | 13-bit (8192 steps) | Smooth duty cycle control (1% - 99%). |
| **5 kHz – 50 kHz** | LEDC High-Speed Timer | 11-bit (2048 steps) | Low jitter. |
| **50 kHz – 500 kHz** | LEDC High-Speed Timer | 9-bit (512 steps) | Low jitter. |
| **500 kHz – 5 MHz** | LEDC High-Speed Timer | 6-bit (64 steps) | Clean CMOS transitions. |
| **5 MHz – 20 MHz** | LEDC High-Speed Timer | 3-bit (8 steps) | 50% nominal duty. |
| **20 MHz – 40 MHz** | LEDC High-Speed Timer | 1-bit (2 steps) | Fixed 50% duty only. |

---

## 3. System & Network Limitations

* **SoftAP Client Limit**: Recommended max 4 concurrent browser connections.
* **WebSocket Streaming Latency**: Configured to 10 Hz (100 ms) to balance responsive real-time UI feel with Wi-Fi power stability.
* **Core Affinity**: Core 0 dedicated to Wi-Fi TCP/IP stack, Core 1 dedicated to Instrument Telemetry & FreeRTOS DSP tasks.
