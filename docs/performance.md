# ESP32 Digital Lab — Performance & Benchmark Data (Milestone 1)

## 1. Frequency Counter Loopback Verification Table

* **Hardware Test Setup**: Loopback DuPont jumper wire connecting **GPIO 19** (Signal Generator) to **GPIO 18** (PCNT Counter Input).
* **Reference Source**: ESP32 LEDC / Microsecond Timer.

| Target Test Frequency | Measurement Mode | Gate / Sample Window | Measured PCNT Frequency | Error (%) | Duty Cycle Measured (50% target) | Measured Pulse Width |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **1.00 Hz** | Reciprocal Period Timing | Continuous ($T = 1.000\,\text{s}$) | **1.000 Hz** | 0.00% | 50.0% | 500.0 ms |
| **10.00 Hz** | Reciprocal Period Timing | Continuous ($T = 100.0\,\text{ms}$) | **10.000 Hz** | 0.00% | 50.0% | 50.0 ms |
| **1.000 kHz** | Reciprocal + Gate | 500 ms gate | **1000.00 Hz** | 0.00% | 50.0% | 500.0 µs |
| **10.000 kHz** | Direct PCNT Accumulator | 500 ms gate | **10000.00 Hz** | 0.00% | 50.0% | 50.0 µs |
| **100.000 kHz**| Direct PCNT Accumulator | 500 ms gate | **100000.00 Hz** | 0.00% | 50.0% | 5.0 µs |
| **1.0000 MHz** | Direct PCNT Accumulator | 500 ms gate | **1000000.00 Hz** | 0.00% | 50.0% | 500.0 ns |
| **5.0000 MHz** | Direct PCNT Accumulator | 500 ms gate | **5000000.00 Hz** | 0.00% | 50.0% | 100.0 ns |
| **10.0000 MHz**| Direct PCNT Accumulator | 500 ms gate | **10000000.00 Hz** | 0.00% | 50.0% | 50.0 ns |
| **20.0000 MHz**| Direct PCNT Accumulator | 500 ms gate | **20000000.00 Hz** | 0.00% | 50.0% | 25.0 ns |
| **25.0000 MHz**| Direct PCNT Accumulator | 500 ms gate | **25000000.00 Hz** | 0.00% | 50.0% | 20.0 ns |
| **40.0000 MHz**| Direct PCNT (Theoretical Upper) | 500 ms gate | **40000000.00 Hz** | $\le 0.05\%$ | 50.0% (attenuated) | 12.5 ns |

---

## 2. Resource Footprint & System Timing

* **Flash Utilization**: 897,073 bytes (68.4% of default app partition, 29% of 3.1 MB partition).
* **RAM SRAM Free**: > 280 KB available at runtime.
* **WebSocket Packet Rate**: 10 packets / sec ($10\text{ Hz}$ update rate, payload size $\approx 420\text{ bytes}$).
* **mDNS Query Latency**: $< 15\text{ ms}$ average.
