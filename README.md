# ESP32 Digital Lab 🔬⚡

An open-source, all-in-one electronics laboratory instrument running on a single **Original ESP32** (`ESP32-D0WD-V3`), providing a browser-based, zero-install interface over Wi-Fi.

![Target MCU: Original ESP32](https://img.shields.io/badge/MCU-Original%20ESP32%20(ESP32--D0WD--V3)-00f2fe)
![UI: Vanilla HTML/CSS/Canvas](https://img.shields.io/badge/Web%20Stack-Vanilla%20%3C20KB-10b981)
![Framework: FreeRTOS + ESP-IDF HAL](https://img.shields.io/badge/RTOS-FreeRTOS-blue)
![License: MIT](https://img.shields.io/badge/License-MIT-orange)

---

## 🌟 Instruments & Roadmap

| Instrument | Peripherals Used | Status | Key Specifications |
| :--- | :--- | :--- | :--- |
| **Frequency Counter** | PCNT0 + Hardware Timer | **Milestone 1 (Complete)** | 10 Hz – 25 MHz, Period, Duty Cycle, Pulse Width, Stats |
| **Signal Generator** | LEDC High-Speed PWM | **Milestone 1 (Complete)** | 1 Hz – 40 MHz reference square wave with duty cycle control |
| **Logic Analyzer** | I2S0 + DMA + GPIO Matrix | *Milestone 2 (Next)* | 8 channels, burst sample buffer, browser protocol decoder |
| **UART Protocol Decoder** | Browser JavaScript | *Milestone 2* | Baud rate, 8N1, HEX, ASCII, timestamps |
| **I²C Protocol Decoder** | Browser JavaScript | *Milestone 2* | START, Address, ACK/NACK, Data, STOP |
| **SPI Protocol Decoder** | Browser JavaScript | *Milestone 2* | MOSI, MISO, CLK, CS, CPOL/CPHA |
| **Oscilloscope** | ADC1 + Hardware Timer | *Milestone 3* | Dual channel, Time/div, Volt/div, Vpp, RMS, Triggering |
| **Digital Multimeter (DMM)**| ADC1 + Resistor Divider | *Milestone 4* | DC Voltage, Resistance, Continuity, Polynomial Calibration |
| **Spectrum Analyzer (FFT)** | Core 1 DSP + Canvas | *Milestone 5* | Audio/PWM/ripple FFT magnitude spectrum |

---

## ⚡ Quick Start

### 1. Build & Flash
```bash
cd esp32-digital-lab
pio run -t upload --upload-port COM7
```

### 2. Connect & Open Dashboard
1. Connect to the Wi-Fi AP:
   * **SSID**: `ESP32-Digital-Lab`
   * **Password**: `digitallab123`
2. Open your browser:
   * **`http://esp32.local`** or **`http://192.168.4.1`**
3. (Optional) Connect a jumper between **GPIO 19** and **GPIO 18** to test loopback frequency measurements.

---

## 📁 Repository Structure
```
esp32-digital-lab/
├── platformio.ini         # PlatformIO build configuration
├── include/
│   ├── system_config.h       # Pin mappings, buffer sizes, network constants
│   ├── instrument_manager.h  # Central peripheral collision prevention
│   ├── wifi_manager.h        # SoftAP and mDNS manager
│   ├── web_server.h          # Async HTTP and WebSocket server
│   ├── web_assets.h          # Self-contained embedded HTML/CSS/Canvas dashboard (<20KB)
│   ├── hal/
│   │   ├── hal_pcnt.h        # Pulse counter HAL with 64-bit gate timing
│   │   └── hal_ledc_gen.h    # Test signal reference generator HAL
│   └── instruments/
│       └── freq_counter.h    # Frequency, Period, Duty & Statistics engine
├── src/
│   ├── main.cpp
│   ├── instrument_manager.cpp
│   ├── wifi_manager.cpp
│   ├── web_server.cpp
│   ├── hal/
│   └── instruments/
├── docs/
│   ├── architecture.md       # System design and FreeRTOS task layout
│   ├── limitations.md        # Honest limits, ADC nonlinearities, voltage safety
│   ├── performance.md        # Measured benchmarks and accuracy tables
│   └── build.md              # Build and flashing walkthrough
└── README.md
```

---

## 🛡️ Safety Warning
This device is for **low-voltage electronics prototyping (0–3.3V / 0–24V with divider)** only. **NEVER connect directly to household AC mains.**
