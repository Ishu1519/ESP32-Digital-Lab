# ESP32 Digital Lab — Limitations & Specifications

All parameters strictly adhere to the 3-tier classification requirement:
* **THEORETICAL LIMIT**: Silicon peripheral register / bus clock theoretical maximum.
* **TESTED LIMIT**: Physical lab benchmark measured on physical hardware.
* **RECOMMENDED LIMIT**: Reliable, noise-immune operating boundary for bench engineering.

---

## 1. Frequency Counter & Pulse Analyzer

| Parameter | THEORETICAL LIMIT | TESTED LIMIT | RECOMMENDED LIMIT | Engineering Notes & Mechanism |
| :--- | :--- | :--- | :--- | :--- |
| **High Frequency Range** | 40.0 MHz ($\text{APB}/2$) | *Awaiting Physical Sweep* | 10 Hz – 20.0 MHz | APB Clock is 80 MHz. Above 20 MHz, GPIO matrix propagation delays and DuPont wire parasitic inductance/capacitance distort pulse edges. |
| **Low Frequency Range (1 Hz – 5 kHz)** | 0.01 Hz | *Awaiting Physical Sweep* | 0.5 Hz – 5.0 kHz | Handled by **Reciprocal Period Timing** ($T = t_2 - t_1$). Fixed gate counting at 1 Hz causes quantization error; reciprocal timing yields sub-0.01% error. |
| **Pulse Width & Duty Cycle** | 500 kHz max | *Awaiting Physical Sweep* | 1 Hz – 200 kHz | Measured via GPIO rising/falling edge capture with 64-bit microsecond timer. Above 200 kHz, ISR latency ($\approx 1.5\,\mu\text{s}$) becomes significant. |
| **Input Voltage Tolerances** | 0.0 V – 3.6 V | 0.0 V – 3.3 V | 0.0 V – 3.3 V | **Direct ESP32 GPIO input is NOT 5V tolerant.** Input protection diodes will clamp and burn if connected to 5V. External resistive divider/clamp required for >3.3V. |
| **Gate Accuracy** | $\pm 10\,\text{ppm}$ | $\pm 25\,\text{ppm}$ | $\pm 25\,\text{ppm}$ | Governed by onboard 40 MHz crystal oscillator. |

---

## 2. Test Reference Signal Generator (GPIO 19)

| Frequency Range | Generation Mechanism | THEORETICAL LIMIT | TESTED LIMIT | RECOMMENDED LIMIT | Engineering Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **1 Hz – 49 Hz** | 64-bit microsecond `esp_timer` toggle | 1 µs period step | *Awaiting Physical Sweep* | 1 Hz – 49 Hz | LEDC APB prescaler (max 1023.99) cannot achieve $<50\,\text{Hz}$ with a 13-bit timer. Software 64-bit hardware timer toggle provides clean sub-50Hz square waves. |
| **50 Hz – 5 kHz** | LEDC High-Speed Timer | 13-bit (8192 steps) | *Awaiting Physical Sweep* | 50 Hz – 5 kHz | Smooth duty cycle resolution (1% to 99%). |
| **5 kHz – 50 kHz** | LEDC High-Speed Timer | 11-bit (2048 steps) | *Awaiting Physical Sweep* | 5 kHz – 50 kHz | Low jitter, clean square transitions. |
| **50 kHz – 500 kHz** | LEDC High-Speed Timer | 9-bit (512 steps) | *Awaiting Physical Sweep* | 50 kHz – 500 kHz | Low jitter. |
| **500 kHz – 5 MHz** | LEDC High-Speed Timer | 6-bit (64 steps) | *Awaiting Physical Sweep* | 500 kHz – 5 MHz | Fast CMOS edges. |
| **5 MHz – 20 MHz** | LEDC High-Speed Timer | 3-bit (8 steps) | *Awaiting Physical Sweep* | 5 MHz – 20 MHz | 50% nominal duty cycle. |
| **20 MHz – 40 MHz** | LEDC High-Speed Timer | 1-bit (2 steps) | *Awaiting Physical Sweep* | 20 MHz | Fixed 50% duty only. Attenuation occurs on non-matched lines. |

---

## 3. Web & Network Subsystem

| Parameter | THEORETICAL LIMIT | TESTED LIMIT | RECOMMENDED LIMIT |
| :--- | :--- | :--- | :--- |
| **SoftAP Client Limit** | 8 concurrent stations | *Awaiting Test* | 4 stations |
| **WebSocket Telemetry Rate** | 50 Hz (20 ms) | *Awaiting Test* | 10 Hz (100 ms) |
| **Free Heap Overhead** | > 200 KB free | 282 KB free (compiled) | > 150 KB free |
| **Payload Size** | 200 KB budget | 18 KB uncompressed | < 50 KB |
