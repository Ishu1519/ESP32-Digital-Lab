# ESP32 Digital Lab — Performance & Benchmark Log (Milestone 1)

All entries follow the strict test table format required for physical verification.

---

## 1. Loopback Validation Sweep (GPIO 19 → GPIO 18)

* **Physical Configuration**: Jumper wire connecting GPIO 19 (LEDC/Timer output) to GPIO 18 (PCNT input).
* **Reference Generator Mode**: Hybrid LEDC / Microsecond Timer.

| Target | Generator | Measured | Error | Error % | Stable? | Test Method |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **1.00 Hz** | Hybrid Timer Toggle | *Pending Hardware Run* | -- | -- | -- | Reciprocal Period Timing ($T = 1.000\,\text{s}$) |
| **2.00 Hz** | Hybrid Timer Toggle | *Pending Hardware Run* | -- | -- | -- | Reciprocal Period Timing ($T = 500.0\,\text{ms}$) |
| **5.00 Hz** | Hybrid Timer Toggle | *Pending Hardware Run* | -- | -- | -- | Reciprocal Period Timing ($T = 200.0\,\text{ms}$) |
| **10.00 Hz** | Hybrid Timer Toggle | *Pending Hardware Run* | -- | -- | -- | Reciprocal Period Timing ($T = 100.0\,\text{ms}$) |
| **20.00 Hz** | Hybrid Timer Toggle | *Pending Hardware Run* | -- | -- | -- | Reciprocal Period Timing ($T = 50.0\,\text{ms}$) |
| **50.00 Hz** | LEDC 13-bit Timer | *Pending Hardware Run* | -- | -- | -- | Reciprocal Period Timing ($T = 20.0\,\text{ms}$) |
| **100.0 Hz** | LEDC 13-bit Timer | *Pending Hardware Run* | -- | -- | -- | Reciprocal + Gate Timing |
| **1.000 kHz** | LEDC 13-bit Timer | *Pending Hardware Run* | -- | -- | -- | Reciprocal + Gate Timing |
| **10.000 kHz**| LEDC 11-bit Timer | *Pending Hardware Run* | -- | -- | -- | Direct PCNT Accumulator (500 ms gate) |
| **100.000 kHz**| LEDC 9-bit Timer | *Pending Hardware Run* | -- | -- | -- | Direct PCNT Accumulator (500 ms gate) |
| **1.0000 MHz**| LEDC 6-bit Timer | *Pending Hardware Run* | -- | -- | -- | Direct PCNT Accumulator (500 ms gate) |
| **2.0000 MHz**| LEDC 6-bit Timer | *Pending Hardware Run* | -- | -- | -- | Direct PCNT Accumulator (500 ms gate) |
| **5.0000 MHz**| LEDC 6-bit Timer | *Pending Hardware Run* | -- | -- | -- | Direct PCNT Accumulator (500 ms gate) |
| **10.0000 MHz**| LEDC 3-bit Timer | *Pending Hardware Run* | -- | -- | -- | Direct PCNT Accumulator (500 ms gate) |
| **15.0000 MHz**| LEDC 3-bit Timer | *Pending Hardware Run* | -- | -- | -- | Direct PCNT Accumulator (500 ms gate) |
| **20.0000 MHz**| LEDC 3-bit Timer | *Pending Hardware Run* | -- | -- | -- | Direct PCNT Accumulator (500 ms gate) |

---

## 2. Resource Footprint & FreeRTOS Telemetry

| Metric | THEORETICAL LIMIT | TESTED SPECIFICATION | RECOMMENDED LIMIT |
| :--- | :--- | :--- | :--- |
| **Free SRAM Heap** | 320 KB total | **282,156 bytes free** (13.9% used) | > 150 KB free |
| **Flash Partition** | 1,310,720 bytes (app0) | **897,073 bytes** (68.4% used) | < 1,100,000 bytes |
| **WebSocket Refresh Rate** | 50 Hz | **10 Hz (100 ms)** | 10 Hz |
| **Web Asset Memory** | < 200 KB target | **18 KB embedded payload** | < 50 KB |
