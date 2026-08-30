# ESP32 Digital Lab 🔬⚡

An open-source, all-in-one electronics laboratory instrument running on a single **Original ESP32** (`ESP32-D0WD-V3`), providing a browser-based, zero-install interface over Wi-Fi.

![Target MCU: Original ESP32](https://img.shields.io/badge/MCU-Original%20ESP32%20(ESP32--D0WD--V3)-00f2fe?style=for-the-badge&logo=espressif)
![Web Stack: Vanilla HTML/CSS/Canvas](https://img.shields.io/badge/Web%20Stack-Vanilla%20%3C20KB%20SPA-10b981?style=for-the-badge&logo=html5)
![Architecture: FreeRTOS + Hardware HAL](https://img.shields.io/badge/RTOS-FreeRTOS%20%2B%20ESP--IDF-blue?style=for-the-badge)
![License: MIT](https://img.shields.io/badge/License-MIT-orange?style=for-the-badge)

---

## 🌟 Instruments & Capabilities

| Instrument | Underlying Hardware | Status | Key Specifications & Limits |
| :--- | :--- | :--- | :--- |
| **Frequency Counter** | PCNT0 + Hardware Timer | **Validated** | $10\text{ Hz} \text{ to } 25\text{ MHz}$, Period, Duty Cycle, Pulse Width, Real-time Statistics |
| **Signal Generator** | LEDC High-Speed PWM | **Validated** | $1\text{ Hz} \text{ to } 40\text{ MHz}$ square wave reference with configurable duty cycle |
| **Logic Analyzer** | I2S0 DMA Parallel Capture | **Validated** | 8 channels, $100\text{ kS/s} \text{ to } 2\text{ MS/s}$, edge triggering, deep canvas zoom/pan |
| **UART Protocol Decoder** | Browser JavaScript | **Validated** | $9600 \text{ to } 115200\text{ baud}$, 8N1/8E1/8O1/7N1, Hex/ASCII, timing packet annotations |
| **I²C Protocol Decoder** | Browser JavaScript | **Validated** | Standard Mode ($100\text{ kHz}$), 7-bit addressing, ACK/NACK, Repeated START, Data, STOP |
| **SPI Protocol Decoder** | Browser JavaScript | **Validated** | Modes 0, 1, 2, 3 (CPOL 0/1, CPHA 0/1), $4–32\text{ bit}$ word lengths, MOSI/MISO dual decode |
| **Digital Multimeter (DMM)**| ADC1 (GPIO 34) + 128x Oversample | **Implemented** | $0–3.3\text{ V}$ & $0–5.0\text{ V}$ DC, 12-bit SAR, real-time strip chart *(Absolute accuracy pending external standard)* |
| **Oscilloscope** | ADC1 DMA + Timer | *Planned* | Dual-channel analog waveform capture, time/div, volt/div, trigger |
| **Spectrum Analyzer (FFT)**| Dual-Core DSP Engine | *Planned* | Audio/PWM/ripple magnitude spectrum |

---

## 📌 Dedicated Hardware Pin Mapping

| Pin | Function / Instrument | Direction | Electrical Characteristics & Protection |
| :--- | :--- | :--- | :--- |
| **GPIO 18** | **PCNT Pulse Counter Input / LA CH0** | Input | $3.3\text{ V}$ LVTTL logic input with pull-down |
| **GPIO 19** | **LEDC Signal Generator / UART TX** | Output | Push-pull square wave test reference output |
| **GPIO 5** | **Logic Analyzer CH1 (UART RX)** | Input | $3.3\text{ V}$ LVTTL digital logic channel |
| **GPIO 16** | **Logic Analyzer CH2 (I²C SDA)** | Input | $3.3\text{ V}$ LVTTL digital logic channel |
| **GPIO 17** | **Logic Analyzer CH3 (I²C SCL)** | Input | $3.3\text{ V}$ LVTTL digital logic channel |
| **GPIO 21** | **Logic Analyzer CH4 (SPI CS)** | Input | $3.3\text{ V}$ LVTTL digital logic channel |
| **GPIO 22** | **Logic Analyzer CH5 (SPI CLK)** | Input | $3.3\text{ V}$ LVTTL digital logic channel |
| **GPIO 23** | **Logic Analyzer CH6 (SPI MOSI)** | Input | $3.3\text{ V}$ LVTTL digital logic channel |
| **GPIO 27** | **Logic Analyzer CH7 (SPI MISO)** | Input | $3.3\text{ V}$ LVTTL digital logic channel |
| **GPIO 34** | **Analog DMM Input (ADC1_CH6)** | Input | Input-only analog pin, $10\text{ k}\Omega$ series resistor, BAT54S Schottky clamp, $100\text{ nF}$ filter |

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
   * **Password**: `digitallab123`
2. Open your web browser and navigate to:
   * **`http://192.168.4.1`** or **`http://esp32.local`**
3. **Loopback Quick Test**: Connect a jumper wire between **GPIO 19** (Generator Output) and **GPIO 18** (Counter Input / Logic Analyzer CH0) to verify full-stack operation in real-time.

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
|  | (PCNT0 Reciprocal)|  | (1Hz - 40MHz PWM) |  | (8-Channel Parallel)|  |
|  +-------------------+  +-------------------+  +---------------------+  |
|  | ADC1 DMM HAL      |                                                  |
|  | (12-bit Oversamp) |                                                  |
|  +-------------------+                                                  |
+-------------------------------------------------------------------------+
```

* **Zero-Install Web Experience**: Complete web dashboard is compiled directly into ESP32 flash memory (`PROGMEM`) as a compressed asset. No internet connection or cloud service required.
* **Collision-Free Hardware Management**: The `InstrumentManager` enforces exclusive ownership over shared GPIOs, timers, and DMA channels, preventing peripheral conflicts across operating modes.
* **Hardware-Accelerated I2S DMA Engine**: Logic Analyzer samples 8 channels in parallel via I2S parallel slave DMA, bypassing CPU interrupt overhead.
* **Client-Side Protocol Decoding**: High-bandwidth digital captures are decoded on the client browser using Web Workers and optimized Canvas renderers, keeping ESP32 CPU free for real-time sampling.

---

## 🛡️ Safety & Metrology Principles

* **Low-Voltage Development Only**: This instrument is designed solely for **low-voltage electronics prototyping ($0–3.3\text{ V} / 0–5.0\text{ V}$)**. **NEVER connect directly to household AC mains.**
* **Input Electrical Stress**: On the DMM channel, a $10\text{ k}\Omega$ series resistor and BAT54S Schottky clamp network limits fault current during overvoltage events. Maximum continuous DC voltage ratings remain subject to board-level front-end configuration.
* **Metrology Transparency**: We maintain a strict distinction between **self-consistency verification** and **absolute external metrology calibration**. Full validation reports and characterization data are maintained in [`docs/performance.md`](docs/performance.md) and [`docs/limitations.md`](docs/limitations.md).

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
