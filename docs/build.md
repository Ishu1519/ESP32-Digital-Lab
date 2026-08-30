# ESP32 Digital Lab — Build & Flashing Guide

## Prerequisites
* **Python 3.10+**
* **PlatformIO CLI (`pio`)** or PlatformIO IDE extension
* **Physical Target**: Original ESP32 Dev Module (e.g. ESP32-WROOM-32, ESP32-D0WD-V3) connected via USB to Serial (e.g. CP2102/CH340).

---

## 1. Building the Firmware

Navigate to the project root:
```bash
cd esp32-digital-lab
pio run
```

---

## 2. Flashing to ESP32

Identify your serial port (e.g. `COM7` on Windows, or `/dev/ttyUSB0` on Linux/macOS):
```bash
pio run -t upload --upload-port COM7
```

---

## 3. Connecting to the Web Dashboard

1. Power on the ESP32.
2. Connect your laptop, tablet, or phone to the Wi-Fi Access Point:
   * **SSID**: `ESP32-Digital-Lab`
   * **Password**: `digitallab123`
3. Open your browser and navigate to:
   * **`http://esp32.local`** or **`http://192.168.4.1`**
4. Real-time telemetry will stream automatically via WebSocket at 10 Hz!

---

## 4. Running the Loopback Frequency Benchmark

1. Connect a single DuPont jumper wire between **GPIO 19** (Reference Generator output) and **GPIO 18** (Frequency Counter input).
2. On the Web Dashboard, select any frequency preset (1 kHz, 10 kHz, 100 kHz, 1 MHz, 10 MHz) or type a custom frequency.
3. Observe the live frequency readout, period, duty cycle, reconstructed waveform, and statistics update instantly.
