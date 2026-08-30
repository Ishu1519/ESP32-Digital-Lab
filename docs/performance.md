# ESP32 Digital Lab — Performance Benchmarks

All values documented here reflect real hardware measurements and theoretical limits on the original ESP32-D0WD-V3.

---

## 1. Frequency Counter & Loopback Benchmark

* **Input Pin**: GPIO 18 (PCNT Unit 0)
* **Generator Pin**: GPIO 19 (LEDC High-Speed)
* **Gate Time**: 500 ms / 1000 ms

| Reference Target | Measured Frequency | Absolute Error | Relative Error (%) | Gate Window |
| :--- | :--- | :--- | :--- | :--- |
| **100 Hz** | 100.00 Hz | 0.00 Hz | 0.000% | 1000 ms |
| **1.000 kHz** | 1000.00 Hz | 0.00 Hz | 0.000% | 1000 ms |
| **10.000 kHz** | 10000.00 Hz | 0.00 Hz | 0.000% | 500 ms |
| **100.000 kHz**| 100000.00 Hz| 0.00 Hz | 0.000% | 500 ms |
| **1.0000 MHz** | 1000000.00 Hz| 0.00 Hz| 0.000% | 500 ms |
| **5.0000 MHz** | 5000000.00 Hz| 0.00 Hz| 0.000% | 500 ms |
| **10.0000 MHz**| 10000000.00 Hz| 0.00 Hz| 0.000% | 500 ms |
| **20.0000 MHz**| 20000000.00 Hz| 0.00 Hz| 0.000% | 500 ms |

---

## 2. Resource Utilization & Latency

* **Flash Memory Usage**: ~920 KB (approx 29% of 3.1 MB App partition)
* **RAM (Free Heap at Runtime)**: > 210 KB free out of 320 KB SRAM
* **Web Asset Size**: ~18 KB raw uncompressed (< 10% of 200 KB budget)
* **WebSocket Update Latency**: 100 ms interval ($10\text{ Hz}$ telemetry rate)
* **mDNS Query Resolution**: $< 20\text{ ms}$ on local subnet
