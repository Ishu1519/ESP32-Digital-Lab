# ESP32 Digital Lab 🔬⚡

An open-source, all-in-one electronics laboratory instrument running on a single **Original ESP32** (`ESP32-D0WD-V3`), providing a browser-based, zero-install interface over Wi-Fi.

![Target MCU: Original ESP32](https://img.shields.io/badge/MCU-Original%20ESP32%20(ESP32--D0WD--V3)-00f2fe?style=for-the-badge&logo=espressif)
![Web Stack: Vanilla HTML/CSS/Canvas](https://img.shields.io/badge/Web%20Stack-Vanilla%20%3C20KB%20SPA-10b981?style=for-the-badge&logo=html5)
![Architecture: FreeRTOS + Hardware HAL](https://img.shields.io/badge/RTOS-FreeRTOS%20%2B%20ESP--IDF-blue?style=for-the-badge)
![License: MIT](https://img.shields.io/badge/License-MIT-orange?style=for-the-badge)

---

## 🌟 Instruments & Status

| Instrument | Underlying Hardware | Status | Tested & Recommended Limits | Theoretical Limits |
| :--- | :--- | :--- | :--- | :--- |
| **Frequency Counter** | PCNT0 + Hardware Timer | **Validated** | **Tested**: $1\text{ Hz} \text{ to } 5.0\text{ MHz}$ (Internal Loopback)<br>**Recommended**: $10\text{ Hz} \text{ to } 5.0\text{ MHz}$ | Up to $40\text{ MHz}$ (PCNT clocking ceiling) |
| **Signal Generator** | LEDC High-Speed PWM | **Validated** | **Tested**: $1\text{ Hz} \text{ to } 5.0\text{ MHz}$ (Clamped at 5 MHz LEDC ceiling)<br>**Recommended**: $10\text{ Hz} \text{ to } 5.0\text{ MHz}$ | Up to $40\text{ MHz}$ (1-bit resolution) |
| **Logic Analyzer (4-CH)** | I2S0 DMA Parallel Capture | **Validated** | **Tested**: $100\text{ kS/s} \text{ to } 2.0\text{ MS/s}$ (4 channels: CH0–CH3)<br>*2 MS/s maximum validated capture rate under documented test setup* | Up to $20\text{ MS/s}$ DMA burst |
| **Logic Analyzer (8-CH)** | I2S0 DMA Parallel Capture | **Implemented / Experimental** | Implemented in driver; 8-channel physical capture not fully validated | 8 parallel channels |
| **UART Protocol Decoder** | Browser JavaScript | **Validated** | **Tested**: $9600 \text{ to } 115200\text{ baud}$, 8N1/8E1/8O1/7N1, Hex/ASCII, timing packet annotations | Up to $115200\text{ baud}$ (Higher bauds unvalidated) |
| **I²C Protocol Decoder** | Browser JavaScript | **Validated** | **Tested**: Standard Mode ($100\text{ kHz}$), 7-bit addressing, ACK/NACK, Repeated START, Data, STOP | Fast Mode ($400\text{ kHz}$) |
| **SPI Protocol Decoder** | Browser JavaScript | **Validated** | **Tested**: Modes 0, 1, 2, 3 (CPOL 0/1, CPHA 0/1), $4–32\text{ bit}$ word lengths, MOSI/MISO dual decode | Synchronous edge decode |
| **Digital Multimeter (0–3.3V / 0–5V)** | ADC1 (GPIO 34) + 128x Oversample | **Implemented / Experimental** | **Tested**: $0.000\text{ V}$ GND baseline on silicon.<br>**Absolute External Accuracy**: **NOT VERIFIED** (Pending independent external voltage standard) | $0–3.3\text{ V}$ (1:1) / $0–5.0\text{ V}$ (2:1) |
| **DMM High-Voltage (12V / 24V)** | Dedicated Divider Network | **Planned / Deferred** | Requires external high-voltage front-end | $0–12\text{ V}$ / $0–24\text{ V}$ |
| **Current Measurement** | Shunt Resistor + Op-Amp | **Planned** | Out of scope for current hardware baseline | Low-side shunt |
| **Oscilloscope** | ADC1 DMA + Timer | **Planned** | Dual-channel analog waveform capture, time/div, volt/div, trigger | Single/Dual channel |
| **Spectrum Analyzer (FFT)** | Dual-Core DSP Engine | **Planned** | Audio/PWM/ripple magnitude spectrum | Real-time FFT |

---

## 📌 Validated Hardware Pin Mapping

| Pin | Function / Instrument | Direction | Electrical Characteristics & Validation Status |
| :--- | :--- | :--- | :--- |
| **GPIO 18** | **Frequency Counter Input (PCNT) / LA CH0** | Input | $3.3\text{ V}$ LVTTL logic input with pull-down (**Validated**) |
| **GPIO 19** | **Signal Generator Output (LEDC) / UART TX** | Output | Push-pull square wave test reference output (**Validated**) |
| **GPIO 12** | **Logic Analyzer CH1** | Input | $3.3\text{ V}$ LVTTL digital logic channel (**Validated**) |
| **GPIO 13** | **Logic Analyzer CH2** | Input | $3.3\text{ V}$ LVTTL digital logic channel (**Validated**) |
| **GPIO 14** | **Logic Analyzer CH3** | Input | $3.3\text{ V}$ LVTTL digital logic channel (**Validated**) |
| **GPIO 25** | **Logic Analyzer CH4** | Input | $3.3\text{ V}$ LVTTL digital logic channel (*Implemented / Unvalidated*) |
| **GPIO 26** | **Logic Analyzer CH5** | Input | $3.3\text{ V}$ LVTTL digital logic channel (*Implemented / Unvalidated*) |
| **GPIO 27** | **Logic Analyzer CH6** | Input | $3.3\text{ V}$ LVTTL digital logic channel (*Implemented / Unvalidated*) |
| **GPIO 32** | **Logic Analyzer CH7** | Input | $3.3\text{ V}$ LVTTL digital logic channel (*Implemented / Unvalidated*) |
| **GPIO 34** | **Analog DMM Input (ADC1_CH6)** | Input | Input-only analog pin, $10\text{ k}\Omega$ series resistor, BAT54S Schottky clamp, $100\text{ nF}$ filter (**Implemented**) |

---

## ⚡ Quick Start

### 1. Requirements & Building
* [PlatformIO Core](https://platformio.org/) or VSCode with PlatformIO IDE
* Original ESP32 development board (e.g. ESP32-WROOM-32 / ESP32-D0WD-V3)

```bash
# Clone the repository
git clone https://github.com/Ishu1519/ESP32-Digital-Lab.git
cd ESP32-Digital-Lab

# Build and flash via PlatformIO
pio run -t upload --upload-port COM7
```

### 2. Connect & Open Dashboard
1. Connect your PC or mobile device to the ESP32 Wi-Fi Access Point:
   * **SSID**: `ESP32-Digital-Lab`
   * **Password**: Configured in `include/system_config.h` (default: `digitallab123`; **change this default before deployment**)
2. Open your web browser and navigate to:
   * **`http://192.168.4.1`** or **`http://esp32.local`**
3. **Loopback Quick Test**: Connect a jumper wire between **GPIO 19** (Generator Output) and **GPIO 18** (Counter Input / Logic Analyzer CH0) to verify loopback frequency counting and digital waveform capture.

---

## 🏗️ Architecture Highlights

```
+-------------------------------------------------------------------------+
|                              Browser UI                                 |
|   Single-Page Vanilla HTML5 / CSS3 / Canvas (Zero External Dependencies)|
+-------------------------------------------------------------------------+
                                    ▲
                                    │ WebSockets (JSON Control & Telemetry)
                                    ▼
+-------------------------------------------------------------------------+
|                         ESP32 Firmware Core                             |
|                                                                         |
|  +--------------------+   +---------------------+   +----------------+  |
|  | Instrument Manager |<->| Web / Async Server  |<->| Wi-Fi SoftAP   |  |
|  +--------------------+   +---------------------+   +----------------+  |
|           │                                                             |
|           ▼ Hardware Abstraction Layer (HAL)                            |
|  +-------------------+  +-------------------+  +---------------------+  |
|  | PCNT Counter HAL  |  | LEDC Generator HAL|  | I2S DMA LA Engine   |  |
|  | (PCNT0 Reciprocal)|  | (1Hz - 5MHz PWM)  |  | (4/8-CH Parallel)   |  |
|  +-------------------+  +-------------------+  +---------------------+  |
|  | ADC1 DMM HAL      |                                                  |
|  | (12-bit Oversamp) |                                                  |
|  +-------------------+                                                  |
+-------------------------------------------------------------------------+
```

* **Zero-Install Web Experience**: Complete web dashboard is compiled directly into ESP32 flash memory (`PROGMEM`) as a compressed asset (<20KB gzipped). No internet connection or cloud service required.
* **Collision-Free Hardware Management**: The `InstrumentManager` enforces exclusive ownership over shared GPIOs, timers, and DMA channels, preventing peripheral conflicts across operating modes.
* **Hardware-Accelerated I2S DMA Engine**: Logic Analyzer samples digital channels in parallel via I2S parallel slave DMA, bypassing CPU interrupt overhead (2 MS/s maximum validated capture rate under the documented test setup).
* **Client-Side Protocol Decoding**: High-bandwidth digital captures are decoded on the client browser using Web Workers and Canvas renderers, keeping ESP32 CPU free for real-time sampling.

---

## 🛡️ Safety & Metrology Principles

* **Low-Voltage Electronics Only**: This instrument is intended strictly for **low-voltage electronics prototyping ($0–3.3\text{ V} / 0–5.0\text{ V}$)**. **NEVER connect directly to household AC mains.**
* **DMM Input Protection & Electrical Stress**:
  - The DMM input uses a $10\text{ k}\Omega$ series resistor and BAT54S dual Schottky clamp to $3.3\text{ V}$ and GND.
  - At a $30\text{ V}$ input fault, the $10\text{ k}\Omega$ resistor limits calculated clamp current to $\approx 2.64\text{ mA}$ ($P_R = 69.4\text{ mW}$) assuming a $3.65\text{ V}$ clamp.
  - **This is an electrical-stress calculation, NOT a guaranteed safe-input rating.**
  - **The 30 V input is NOT rated for continuous operation.** Maximum allowable input voltage and transient capability remain **UNSPECIFIED** until the complete protection network, 3.3 V rail behavior, and board-level failure modes are validated.
* **Metrology Transparency**:
  - **Frequency Counter**: Validated from $1\text{ Hz}$ to $5.0\text{ MHz}$ via internal loopback.
  - **Logic Analyzer & UART**: 4 channels validated at $100\text{ kS/s} \text{ to } 2.0\text{ MS/s}$; UART validated up to $115200\text{ baud}$.
  - **Digital Multimeter**: Implemented + functionally tested on hardware; **absolute external accuracy is NOT VERIFIED** pending physical multi-point calibration against an independent precision voltage standard.
  - Detailed metrology benchmarks and limits are documented in [`docs/performance.md`](docs/performance.md) and [`docs/limitations.md`](docs/limitations.md).

---

## 📁 Repository Structure

```
├── platformio.ini              # PlatformIO environment configuration
├── data/
│   └── index.html              # Source Web Dashboard (HTML5, CSS3, Canvas)
├── include/
│   ├── system_config.h         # Pin mappings, buffer sizes, network constants
│   ├── instrument_manager.h    # Central peripheral collision prevention
│   ├── wifi_manager.h          # SoftAP & mDNS configuration
│   ├── web_server.h            # Async HTTP & WebSocket server
│   ├── web_assets.h            # Packaged PROGMEM web dashboard
│   ├── hal/                    # Hardware Abstraction Layer headers
│   │   ├── hal_pcnt.h          # Pulse counter HAL
│   │   ├── hal_ledc_gen.h      # Signal generator HAL
│   │   ├── hal_i2s_la.h        # I2S parallel DMA capture HAL
│   │   └── hal_adc_dmm.h       # ADC1 DMM HAL
│   └── instruments/            # High-level instrument logic
│       ├── freq_counter.h      # Frequency counter instrument
│       ├── logic_analyzer.h    # Logic analyzer instrument
│       └── dmm.h               # Digital multimeter instrument
├── src/                        # Implementation source files
├── benchmarks/                 # Python automated hardware test & validation suite
├── docs/                       # Detailed engineering documentation
│   ├── architecture.md         # System design & FreeRTOS task layout
│   ├── limitations.md          # 3-tier limits, silicon physics & safety boundaries
│   ├── performance.md          # Complete validation test suites & benchmarks
│   └── build.md                # Build, flashing & troubleshooting instructions
└── README.md
```

---

## 📄 License

This project is open-source under the **[MIT License](LICENSE)**.
