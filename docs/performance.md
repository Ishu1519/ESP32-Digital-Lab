# Milestone 1 Hardware Performance Benchmarks (Internal Loopback Self-Consistency)

> [!IMPORTANT]
> **Measurement Classification Note**:
> The ESP32 generating and measuring its own signal via internal loopback (GPIO 19 to GPIO 18) is an **Internal Self-Consistency Test**, not an absolute external standard calibration. It measures the synchronization and counting fidelity between the ESP32 APB-derived generator and PCNT hardware counter.

---

## 1. 20-Point Self-Consistency Frequency Sweep (1 Hz to 20 MHz)

**Hardware Setup**: Physical **ESP32-D0WD-V3 (rev 3.1)** on `COM7` using a direct hardware jumper wire between **GPIO 19** (Signal Generator) and **GPIO 18** (PCNT Input).

| Requested Freq | Generator Mode | Res (bits) | div_param | Actual Configured Output | PCNT Measured Freq | Error vs Configured (%) | Validation Status |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **1 Hz** | Timer (64-bit) | N/A | N/A | 1.00 Hz | **1.00 Hz** | 0.000% | **VERIFIED** |
| **2 Hz** | Timer (64-bit) | N/A | N/A | 2.00 Hz | **2.00 Hz** | 0.000% | **VERIFIED** |
| **5 Hz** | Timer (64-bit) | N/A | N/A | 5.00 Hz | **5.00 Hz** | 0.000% | **VERIFIED** |
| **10 Hz** | Timer (64-bit) | N/A | N/A | 10.00 Hz | **10.00 Hz** | 0.000% | **VERIFIED** |
| **20 Hz** | Timer (64-bit) | N/A | N/A | 20.00 Hz | **20.00 Hz** | 0.001% | **VERIFIED** |
| **50 Hz** | LEDC (80MHz APB) | 14-bit | 25,000 | 50.00 Hz | **50.00 Hz** | 0.001% | **VERIFIED** |
| **100 Hz** | LEDC (80MHz APB) | 14-bit | 12,500 | 100.00 Hz | **100.00 Hz** | 0.000% | **VERIFIED** |
| **500 Hz** | LEDC (80MHz APB) | 14-bit | 2,500 | 500.00 Hz | **500.00 Hz** | 0.001% | **VERIFIED** |
| **1 kHz** | LEDC (80MHz APB) | 14-bit | 1,250 | 1,000.00 Hz | **1,000.00 Hz** | 0.000% | **VERIFIED** |
| **5 kHz** | LEDC (80MHz APB) | 13-bit | 500 | 5,000.00 Hz | **4,999.97 Hz** | 0.001% | **VERIFIED** |
| **10 kHz** | LEDC (80MHz APB) | 12-bit | 500 | 10,000.00 Hz | **10,000.01 Hz** | 0.000% | **VERIFIED** |
| **50 kHz** | LEDC (80MHz APB) | 10-bit | 400 | 50,000.00 Hz | **49,998.87 Hz** | 0.002% | **VERIFIED** |
| **100 kHz** | LEDC (80MHz APB) | 9-bit | 400 | 100,000.00 Hz | **99,998.73 Hz** | 0.001% | **VERIFIED** |
| **500 kHz** | LEDC (80MHz APB) | 7-bit | 320 | 500,000.00 Hz | **499,987.67 Hz** | 0.002% | **VERIFIED** |
| **1 MHz** | LEDC (80MHz APB) | 6-bit | 320 | 1,000,000.00 Hz | **999,984.67 Hz** | 0.002% | **VERIFIED** |
| **2 MHz** | LEDC (80MHz APB) | 5-bit | 320 | 2,000,000.00 Hz | **1,999,958.00 Hz** | 0.002% | **VERIFIED** |
| **5 MHz** | LEDC (80MHz APB) | 4-bit | 256 | 5,000,000.00 Hz | **4,999,926.00 Hz** | 0.001% | **VERIFIED** |
| **10 MHz** | LEDC (80MHz APB) | 4-bit | 256 | 5,000,000.00 Hz *(Clamped)* | **4,999,928.67 Hz** | 0.001% | **LEDC LIMIT** |
| **15 MHz** | LEDC (80MHz APB) | 4-bit | 256 | 5,000,000.00 Hz *(Clamped)* | **4,999,894.67 Hz** | 0.002% | **LEDC LIMIT** |
| **20 MHz** | LEDC (80MHz APB) | 4-bit | 256 | 5,000,000.00 Hz *(Clamped)* | **4,999,929.33 Hz** | 0.001% | **LEDC LIMIT** |

---

## 2. Root Cause Analysis of Previous Intermediate Discrepancies

1. **Gate Window Asynchrony**: In earlier runs, telemetry was requested after short delays (<0.6s) before a complete steady-state gate window (500ms – 2000ms) had elapsed at the newly configured frequency. Adding multi-sample steady-state window synchronization resolved this completely.
2. **Interrupt Saturation**: Attaching software edge interrupts (`gpio_edge_isr`) simultaneously with hardware PCNT caused interrupt storms at frequencies >50 kHz. Reverting to pure hardware PCNT counting with 16-bit overflow tracking eliminates all CPU overhead.
3. **LEDC 4-Bit Resolution Floor**: On original ESP32, the minimum supported LEDC timer resolution is 4 bits ($2^4 = 16$). With an 80 MHz APB clock and minimum divider $\text{div} = 1.0$ (`div_param = 256`), the maximum frequency LEDC can generate is:
   $$\frac{80,000,000\text{ Hz}}{16 \times 1.0} = \mathbf{5.000\text{ MHz}}$$
   Requesting $>5.0\text{ MHz}$ from LEDC clamps at 5.0 MHz. Measuring signals $>5\text{ MHz}$ requires an external signal generator.

---

## 3. Milestone 1 Validation Status

* **Status**: **PARTIALLY VALIDATED** (Self-Consistency Loopback Verified up to 5.0 MHz; external reference generator required for absolute accuracy validation up to 40 MHz).

---
## 4. Phase 3: Logic Analyzer 4-Channel Comprehensive Physical Validation

**Hardware Setup**: Physical **ESP32-D0WD-V3 (rev 3.1)** on `COM7` @ 240 MHz. Reference test patterns generated dynamically on GPIO 19 and captured via CH0 on GPIO 18, with CH1 (GPIO 12), CH2 (GPIO 13), CH3 (GPIO 14).

### Test Suite 1: Multi-Frequency Timing & Sample-Index Precision Sweep

> [!NOTE]
> **Metrology Definition**:
> In digital logic sampling, the time between discrete samples defines the **sample-time quantization** ($\Delta t_q = 1/f_s$). Edge transitions are detected at discrete sample boundaries ($\text{Index}_{\text{edge}}$).
> The **sample-index reconstruction error** represents the deviation between the sample-discrete measured period ($\Delta \text{Index}_{\text{period}} \times \Delta t_q$) and the nominal reference period. A reconstruction error of $0.00\%$ indicates that the mean discrete edge index interval matches the target cycle count within $\pm 1$ quantization step.

| Target Freq | Tested Sample Rate | Quantization ($\Delta t_q$) | Depth | Detected Trans | Target Period | Meas Period | Meas Freq | Duty Cycle | Sample-Index Recon Error | Status |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **1 kHz** | 100 kS/s | 10.0 µs | 2,048 | 41 | 1000.000 µs | **1000.000 µs** | **1,000.0 Hz** | 50.0% | **0.00% ($\pm 0\text{ samples}$)** | **PASS** |
| **1 kHz** | 500 kS/s | 2.0 µs | 2,048 | 8 | 1000.000 µs | **1000.000 µs** | **1,000.0 Hz** | 50.0% | **0.00% ($\pm 0\text{ samples}$)** | **PASS** |
| **1 kHz** | 1 MS/s | 1.0 µs | 2,048 | 4 | 1000.000 µs | **1000.000 µs** | **1,000.0 Hz** | 50.0% | **0.00% ($\pm 0\text{ samples}$)** | **PASS** |
| **10 kHz** | 100 kS/s | 10.0 µs | 2,048 | 410 | 100.000 µs | **100.000 µs** | **10,000.0 Hz** | 50.0% | **0.00% ($\pm 0\text{ samples}$)** | **PASS** |
| **10 kHz** | 500 kS/s | 2.0 µs | 2,048 | 82 | 100.000 µs | **100.000 µs** | **10,000.0 Hz** | 50.0% | **0.00% ($\pm 0\text{ samples}$)** | **PASS** |
| **10 kHz** | 1 MS/s | 1.0 µs | 2,048 | 41 | 100.000 µs | **100.000 µs** | **10,000.0 Hz** | 50.0% | **0.00% ($\pm 0\text{ samples}$)** | **PASS** |
| **10 kHz** | 2 MS/s | 0.5 µs | 2,048 | 21 | 100.000 µs | **99.800 µs** | **10,020.0 Hz** | 49.9% | **0.20% ($< 1\text{ sample}$)** | **PASS** |
| **100 kHz** | 500 kS/s | 2.0 µs | 2,048 | 819 | 10.000 µs | **10.000 µs** | **100,000.0 Hz** | 50.0% | **0.00% ($\pm 0\text{ samples}$)** | **PASS** |
| **100 kHz** | 1 MS/s | 1.0 µs | 2,048 | 410 | 10.000 µs | **10.000 µs** | **100,000.0 Hz** | 50.0% | **0.00% ($\pm 0\text{ samples}$)** | **PASS** |
| **100 kHz** | 2 MS/s | 0.5 µs | 2,048 | 206 | 10.000 µs | **9.966 µs** | **100,344.3 Hz** | 49.8% | **0.34% ($< 1\text{ sample}$)** | **PASS** |
| **250 kHz** | 1 MS/s | 1.0 µs | 2,048 | 1,024 | 4.000 µs | **4.000 µs** | **250,000.0 Hz** | 50.0% | **0.00% ($\pm 0\text{ samples}$)** | **PASS** |
| **250 kHz** | 2 MS/s | 0.5 µs | 2,048 | 513 | 4.000 µs | **4.000 µs** | **250,000.0 Hz** | 50.0% | **0.00% ($\pm 0\text{ samples}$)** | **PASS** |
| **500 kHz** | 2 MS/s | 0.5 µs | 2,048 | 1,024 | 2.000 µs | **2.000 µs** | **500,000.0 Hz** | 50.0% | **0.00% ($\pm 0\text{ samples}$)** | **PASS** |
| **10 kHz** | 5 MS/s | 0.2 µs | 2,048 | 11 | 100.000 µs | 77.100 µs | 12,970.2 Hz | 50.1% | 22.90% | **CLAMPED ($\approx 4.4\text{ MS/s}$)** |
| **10 kHz** | 10 MS/s | 0.1 µs | 2,048 | 10 | 100.000 µs | 40.000 µs | 25,000.0 Hz | 50.0% | 60.00% | **CLAMPED ($\approx 4.4\text{ MS/s}$)** |.00% | **CLAMPED (~4.4 MS/s)** |

---

### Test Suite 2: DC Flatline & Glitch Immunity Baseline (Generator OFF @ 1 MS/s, 4096 samples)

| Channel | Input Configuration | Measured DC Level | False Transitions | Glitch Status |
|:---:|:---:|:---:|:---:|:---:|
| **CH0** | Physical Pin 18 (Pull-up / Low) | 0.0% HIGH | **0** | **CLEAN (0 glitches)** |
| **CH1** | Physical Pin 12 (Internal Pull-up) | 100.0% HIGH | **0** | **CLEAN (0 glitches)** |
| **CH2** | Physical Pin 13 (Internal Pull-up) | 100.0% HIGH | **0** | **CLEAN (0 glitches)** |
| **CH3** | Physical Pin 14 (Internal Pull-up) | 100.0% HIGH | **0** | **CLEAN (0 glitches)** |

---

### Test Suite 3: Buffer Depth Scaling & Memory Footprint

| Buffer Depth | Captured Payload | Free Heap | Transfer Latency | Measured Frequency | Validation Status |
|:---:|:---:|:---:|:---:|:---:|:---:|
| **1,024 samples** | 1,024 Bytes (1.36 KB Base64) | 186,248 Bytes | 232.5 ms | **10,000.0 Hz** | **VERIFIED** |
| **2,048 samples** | 2,048 Bytes (2.73 KB Base64) | 184,200 Bytes | 346.0 ms | **10,000.0 Hz** | **VERIFIED** |
| **4,096 samples** | 4,096 Bytes (5.46 KB Base64) | 180,104 Bytes | 583.3 ms | **9,997.5 Hz** | **VERIFIED** |
| **8,192 samples** | 8,192 Bytes (10.92 KB Base64) | 171,912 Bytes | 1,063.5 ms | **10,000.0 Hz** | **VERIFIED** |

---

### Test Suite 4: 100-Cycle Endurance & Concurrency Stress Test

* **Configuration**: 10.000 kHz test pattern @ 1 MS/s, 2,048 samples per cycle, continuous HTTP & Wi-Fi broadcast.
* **Completed Cycles**: **100 / 100 cycles (100.0% success rate)**
* **Dropped Frames**: **0**
* **Minimum Free Heap**: **184,272 Bytes** (Steady flatline across all 100 cycles, 0 memory leaks)
* **Average Transfer Latency**: **326.1 ms** (including Base64 serialization, JSON construction, and Serial transport)

---

### Test Suite 5: Mode Switching Resource Exclusivity

* **Switches Executed**: 20 alternating transitions between Frequency Counter (Mode 1) and Logic Analyzer (Mode 3)
* **Success Rate**: **20 / 20 (100%)**
* **Memory Integrity**: Initial Heap = 184,296 B, Final Heap = 184,180 B (Difference < 120 B due to transient buffers, 0 permanent leaks).

---

## 5. Protocol Analysis Phase: Physical Hardware & Synthetic Stream Validation

**Hardware Setup**: Physical **ESP32-D0WD-V3 (rev 3.1)** on `COM7` @ 240 MHz.
* **UART Hardware Validation**: Transmitted deterministic test strings via hardware UART1 TX on GPIO 19 and captured via Logic Analyzer DMA sampling on CH0 (GPIO 18).
* **Decoders**: Executed browser-equivalent decoding engines on raw unpacked sample bitstreams.

### Test Suite 6: Physical Hardware UART Loopback Benchmark (GPIO 19 -> GPIO 18)

| Baud Rate | Logic Sample Rate | Oversampling Ratio | Sent Payload | Decoded String | Captured Bytes | Framing Errors | Byte Error Rate | Status |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **9,600 baud** | 100 kS/s | **10.42 smp/bit** | `TEST9600` | `TEST9600` | 12 Bytes | **0** | **0.00%** | **VERIFIED** |
| **19,200 baud** | 250 kS/s | **13.02 smp/bit** | `TEST19200` | `TEST19200` | 13 Bytes | **0** | **0.00%** | **VERIFIED** |
| **38,400 baud** | 500 kS/s | **13.02 smp/bit** | `TEST38400` | `TEST38400` | 13 Bytes | **0** | **0.00%** | **VERIFIED** |
| **57,600 baud** | 500 kS/s | **8.68 smp/bit** | `TEST57600` | `TEST57600` | 13 Bytes | **0** | **0.00%** | **VERIFIED** |
| **115,200 baud** | 1.0 MS/s | **8.68 smp/bit** | `TEST115200` | `TEST115200` | 14 Bytes | **0** | **0.00%** | **VERIFIED** |
| **115,200 baud** | 2.0 MS/s | **17.36 smp/bit** | `TEST115200` | `TEST115200` | 14 Bytes | **0** | **0.00%** | **VERIFIED** |

* **UART Summary**: **100% bit-accurate decoding** across all standard baud rates from 9,600 to 115,200 baud. Zero framing errors, zero dropped characters, and exact sample-time phase alignment.

---

### Test Suite 7: I2C Protocol Engine Validation

* **Bus Speed**: 100 kHz Standard Mode ($\Delta t_q = 1.0 \ \mu\text{s}$ @ 1 MS/s).
* **Conditions Verified**:
  - **START Condition**: Detected (SDA falling while SCL is HIGH).
  - **7-bit Slave Address**: `0x3C` decoded accurately with WRITE bit ($R/\bar{W} = 0$).
  - **ACK/NACK Detection**: 9th clock ACK (SDA = 0) verified on address and all data payload bytes.
  - **Data Payload**: 8 bytes `[0x00, 0xAF, 0x20, 0x00, 0x8D, 0x14, 0xD5, 0x80]` decoded with 100% byte fidelity.
  - **STOP Condition**: Detected (SDA rising while SCL is HIGH).
* **Validation Status**: **PASSED (100% Protocol Compliance)**

---

### Test Suite 8: SPI Protocol Engine Validation (All 4 Modes)

* **Word Length**: 8-bit, MSB first, active-low Chip Select ($\overline{\text{CS}}$).
* **Modes Tested**:
  - **Mode 0 (CPOL=0, CPHA=0)**: Sample on leading rising edge $\to$ **PASS** (100% match)
  - **Mode 1 (CPOL=0, CPHA=1)**: Sample on trailing falling edge $\to$ **PASS** (100% match)
  - **Mode 2 (CPOL=1, CPHA=0)**: Sample on leading falling edge $\to$ **PASS** (100% match)
  - **Mode 3 (CPOL=1, CPHA=1)**: Sample on trailing rising edge $\to$ **PASS** (100% match)
* **Payload Verified**: 5-byte sequence `[0x9F, 0x00, 0xA5, 0x5A, 0xFF]`.
---

### Test Suite 9: DMM Phase 1 (Protected 0–3.3 V & 0–5.0 V DC Metrology Audit)

* **Physical Channel**: GPIO 34 (ADC1 Channel 6) — input-only high-impedance analog input, completely independent from active Wi-Fi AP.
* **Front-End Protection Circuit & Electrical Stress Analysis**:
  - Series Resistor: $R_{\text{series}} = 10\text{ k}\Omega$.
  - Dual Schottky Clamp: BAT54S to $3.3\text{ V}$ rail ($V_f \approx 0.35\text{ V}$) and $\text{GND}$. Pin clamp voltage $V_{\text{clamp}} = 3.3\text{ V} + 0.35\text{ V} = 3.65\text{ V}$.
  - **Electrical Stress Calculation ($30\text{ V}$ Input)**:
    - **$30\text{ V}$ input: NOT RATED FOR CONTINUOUS OPERATION.**
    - The $10\text{ k}\Omega$ series resistor limits calculated clamp current to approximately **$2.64\text{ mA}$** at a $30\text{ V}$ input assuming a $3.65\text{ V}$ clamp:
      $$I_{\text{clamp}} = \frac{30.0\text{ V} - 3.65\text{ V}}{10\text{ k}\Omega} = \mathbf{2.635\text{ mA}}$$
    - Resistor power dissipation: $P_R = (2.635\text{ mA})^2 \cdot 10\text{ k}\Omega = \mathbf{69.4\text{ mW}}$.
    - **This is an electrical-stress calculation, NOT a guaranteed safe-input rating.**
  - **Unspecified Input Rating**:
    - Maximum allowable input voltage and transient capability remain **UNSPECIFIED** until the complete protection network, $3.3\text{ V}$ rail behavior, diode surge characteristics, and board-level failure modes are validated.
    - When the MCU is powered off or in deep sleep ($I < 10\ \mu\text{A}$), injected clamp current back-feeds the $3.3\text{ V}$ rail toward $3.65\text{ V}$.
* **Filter Settling & Step Response ($10\text{ k}\Omega + 100\text{ nF}$)**:
  - Time constant $\tau = 1.000\text{ ms}$, cutoff frequency $f_c = 159.15\text{ Hz}$.
  - $99.0\%$ settling: $4.61\text{ ms}$; $99.9\%$ settling: $6.91\text{ ms}$; $12$-bit resolution ($1/4096$ error): $\mathbf{8.32\text{ ms}}$.
  - Recommended measurement delay: $\ge 10.0\text{ ms}$ after any voltage step.
* **Un-Averaged Raw Sample Analysis (1,000 Hardware Conversions @ 50 µs)**:
  - Raw ADC mean: $0.000\text{ LSB}$ at GND, $\sigma_{\text{raw}} = 0.000\text{ LSB}$.
  - Root cause: Physical comparator deadband clamping below silicon threshold, confirmed not to be an integer truncation artifact.
* **Data Origin & Metrology Audit Classification**:
  - **Grounded Baseline ($0.000\text{ V}$)**: Physical silicon measurement on `COM7` $\to$ **TESTED ON HARDWARE**.
  - **Multi-Point Voltage Steps ($0.5\text{ V} – 5.0\text{ V}$)**: Model transfer calculations $\to$ **SIMULATED MODEL (ABSOLUTE ACCURACY NOT VERIFIED)**.

#### Metrology Audit Dataset

| Test Point | Reference Voltage ($V_{\text{Ref}}$) | Data Origin / Source | Raw ADC Code | Measured DMM ($V_{\text{Meas}}$) | Classification & Metrology Status |
|:---|:---:|:---|:---:|:---:|:---|
| **0.000 V (GND)** | $0.0000\text{ V}$ | **PHYSICAL SILICON (COM7)** | $0\text{ LSB}$ | **$0.0000\text{ V}$** | **TESTED ON HARDWARE** |
| **0.500 V Step** | $0.5000\text{ V}$ | SIMULATED MODEL (IDEAL) | $620\text{ LSB}$ | $--$ | **ABSOLUTE ACCURACY = NOT VERIFIED** |
| **1.000 V Step** | $1.0000\text{ V}$ | SIMULATED MODEL (IDEAL) | $1240\text{ LSB}$ | $--$ | **ABSOLUTE ACCURACY = NOT VERIFIED** |
| **1.500 V Step** | $1.5000\text{ V}$ | SIMULATED MODEL (IDEAL) | $1861\text{ LSB}$ | $--$ | **ABSOLUTE ACCURACY = NOT VERIFIED** |
| **2.000 V Step** | $2.0000\text{ V}$ | SIMULATED MODEL (IDEAL) | $2481\text{ LSB}$ | $--$ | **ABSOLUTE ACCURACY = NOT VERIFIED** |
| **2.500 V Step** | $2.5000\text{ V}$ | SIMULATED MODEL (IDEAL) | $3102\text{ LSB}$ | $--$ | **ABSOLUTE ACCURACY = NOT VERIFIED** |
| **3.000 V Step** | $3.0000\text{ V}$ | SIMULATED MODEL (IDEAL) | $3722\text{ LSB}$ | $--$ | **ABSOLUTE ACCURACY = NOT VERIFIED** |
| **3.300 V Rail** | $3.3000\text{ V}$ | SIMULATED MODEL (IDEAL) | $4095\text{ LSB}$ | $--$ | **ABSOLUTE ACCURACY = NOT VERIFIED** |
| **1.000 V (5V Rng)** | $1.0000\text{ V}$ | SIMULATED MODEL (2:1) | $620\text{ LSB}$ | $--$ | **ABSOLUTE ACCURACY = NOT VERIFIED** |
| **2.000 V (5V Rng)** | $2.0000\text{ V}$ | SIMULATED MODEL (2:1) | $1240\text{ LSB}$ | $--$ | **ABSOLUTE ACCURACY = NOT VERIFIED** |
| **3.000 V (5V Rng)** | $3.0000\text{ V}$ | SIMULATED MODEL (2:1) | $1861\text{ LSB}$ | $--$ | **ABSOLUTE ACCURACY = NOT VERIFIED** |
| **4.000 V (5V Rng)** | $4.0000\text{ V}$ | SIMULATED MODEL (2:1) | $2481\text{ LSB}$ | $--$ | **ABSOLUTE ACCURACY = NOT VERIFIED** |
| **5.000 V (5V Rng)** | $5.0000\text{ V}$ | SIMULATED MODEL (2:1) | $3102\text{ LSB}$ | $--$ | **ABSOLUTE ACCURACY = NOT VERIFIED** |

*Note: 12 V and 24 V measurement ranges remain strictly OUT OF SCOPE until dedicated high-voltage front-ends and active resistor dividers are designed and validated.*





