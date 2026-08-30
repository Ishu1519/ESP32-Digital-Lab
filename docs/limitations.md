# ESP32 Digital Lab — Hardware Limitations & Specifications

This document outlines the performance boundaries of the ESP32 Digital Lab on the original **ESP32-D0WD-V3**. All specifications adhere strictly to the 3-tier convention: **THEORETICAL LIMIT**, **TESTED LIMIT**, and **RECOMMENDED LIMIT**.

---

## 1. Frequency Counter Limits

| Limit Tier | Frequency Range | Resolution / Accuracy | Notes |
|:---|:---|:---|:---|
| **THEORETICAL LIMIT** | $0.1\text{ Hz} \text{ to } 40.0\text{ MHz}$ | $f_{\text{APB}} / 2 = 80\text{ MHz} / 2$ | Maximum physical counting rate of the ESP32 PCNT module with ideal external square wave. |
| **TESTED LIMIT** | $1.0\text{ Hz} \text{ to } 5.0\text{ MHz}$ | **$1\text{ Hz} – 5\text{ MHz}$**: $\pm 0.002\%$ | Measured via internal self-consistency loopback (GPIO 19 to GPIO 18). |
| **RECOMMENDED LIMIT** | $1.0\text{ Hz} \text{ to } 1.0\text{ MHz}$ | $\pm 0.1\%$ | Recommended operating range for standard digital electronics debugging on original ESP32. |

---

## 2. Signal Generator Limits (LEDC & Hardware Timer)

| Limit Tier | Frequency Range | Duty Resolution | Notes |
|:---|:---|:---|:---|
| **THEORETICAL LIMIT** | $0.001\text{ Hz} \text{ to } 5.0\text{ MHz}$ | 4-bit to 20-bit | Upper limit constrained by ESP32 LEDC 4-bit resolution floor ($80\text{ MHz} / 16 = 5.0\text{ MHz}$). |
| **TESTED LIMIT** | $1.0\text{ Hz} \text{ to } 5.0\text{ MHz}$ | 14-bit ($50\text{ Hz} – 1\text{ kHz}$), 4-bit ($5\text{ MHz}$) | Verified on GPIO 19. Signals $>5\text{ MHz}$ clamp to 5.0 MHz. |
| **RECOMMENDED LIMIT** | $1.0\text{ Hz} \text{ to } 1.0\text{ MHz}$ | 6-bit to 14-bit | Provides stable, jitter-free square wave generation with precise duty cycle control. |

---

## 3. Logic Analyzer Limits (4-Channel Digital Capture)

| Limit Tier | Tested Sample Rate Range | Measured Edge Resolution / Quantization | Channel Count | Notes |
|:---|:---|:---|:---|:---|
| **THEORETICAL LIMIT** | $10\text{ kS/s} \text{ to } 20.0\text{ MS/s}$ | $\Delta t_q = 50\text{ ns} \text{ to } 100\text{ \mu s}$ | 4 to 8 Channels | Direct I2S / DMA peripheral FIFO theoretical ceiling. |
| **TESTED LIMIT** | $100\text{ kS/s} \text{ to } 2.0\text{ MS/s}$ | $\Delta t_q = 0.5\text{ \mu s} \text{ (at 2 MS/s)}$<br>$\Delta t_q = 1.0\text{ \mu s} \text{ (at 1 MS/s)}$ | 4 Channels | **$100\text{ kS/s} – 2.0\text{ MS/s}$**: Sample-index reconstruction error $\le \pm 1\text{ sample quantization}$. Rates $\ge 5\text{ MS/s}$ clamp at CPU execution floor ($\approx 4.4\text{ MS/s}$). |
| **RECOMMENDED LIMIT** | $100\text{ kS/s} \text{ to } 2.0\text{ MS/s}$ | $\Delta t_q \le 0.5\text{ \mu s}$ | 4 Channels | Recommended for UART (up to 115.2 kbps / 921.6 kbps), standard/fast I²C (100/400 kHz), and low-speed SPI (up to 500 kHz) protocol decoding. |

---

## 4. Protocol Analyzer Limits (UART, I²C, SPI)

| Protocol | Limit Tier | Maximum Speed / Clock | Configuration Range | Notes |
|:---|:---|:---|:---|:---|
| **UART** | **THEORETICAL LIMIT** | $921,600\text{ baud}$ | 5–9 data bits, Parity (None/Even/Odd/Mark/Space), 1–2 stop bits | At $921.6\text{ kbaud}$, oversampling ratio is $< 2.2\times$ at $2.0\text{ MS/s}$. |
| **UART** | **TESTED LIMIT** | **$115,200\text{ baud}$** | 8 data bits, None/Even/Odd parity, 1.0 stop bit | **100% bit-accurate physical loopback verification** (0 framing errors across 9600 to 115200 baud). |
| **UART** | **RECOMMENDED LIMIT** | **$115,200\text{ baud}$** | Standard 8-N-1 / 8-E-1 | Requires $\ge 8.7\times$ oversampling ratio (achieved at $\ge 1.0\text{ MS/s}$). |
| **I²C** | **THEORETICAL LIMIT** | $1.0\text{ MHz}$ (Fast-mode Plus) | 7-bit & 10-bit addressing, ACK/NACK | At $1.0\text{ MHz}$, requires $>10\text{ MS/s}$ for transition resolution. |
| **I²C** | **TESTED LIMIT** | **$100\text{ kHz}$ (Standard Mode)** | 7-bit addressing, ACK/NACK, Data, Repeated START, STOP | Validated with 100% byte fidelity at $1.0\text{ MS/s}$ ($10\times$ oversampling). |
| **I²C** | **RECOMMENDED LIMIT** | **$100\text{ kHz}$ to $400\text{ kHz}$** | Standard (100k) & Fast (400k) modes | Sample at $1\text{ MS/s}$ for 100 kHz, $2\text{ MS/s}$ for 400 kHz. |
| **SPI** | **THEORETICAL LIMIT** | $5.0\text{ MHz}$ | Modes 0, 1, 2, 3 (CPOL 0/1, CPHA 0/1), 4–32 bit words | Theoretical limit of synchronous edge sampling. |
| **SPI** | **TESTED LIMIT** | **$500\text{ kHz}$** | Modes 0, 1, 2, 3, 8-bit / 16-bit word lengths | All 4 modes verified with 100% payload matching against synthetic ground truth. |
| **SPI** | **RECOMMENDED LIMIT** | **$100\text{ kHz}$ to $500\text{ kHz}$** | Modes 0, 1, 2, 3 with active-low CS | Recommended sample rate $2.0\text{ MS/s}$ for clean clock phase edge detection. |

---

## 5. Digital Multimeter Limits (0–3.3 V & 0–5.0 V DC Voltage)

| Measurement Range | Limit Tier | Voltage Span | Absolute Accuracy Status | Electrical Ratings & Notes |
|:---|:---|:---|:---|:---|
| **0–3.3 V (Direct 1:1)** | **THEORETICAL LIMIT** | $0.0\text{ V} \text{ to } 3.30\text{ V}$ | Full Scale 12-bit | SAR ADC input span constrained by $3.3\text{ V}$ supply rail. |
| **0–3.3 V (Direct 1:1)** | **TESTED LIMIT** | **$0.0000\text{ V}$ (GND baseline)** | **NOT VERIFIED** (eFuse Model) | **30 V input: NOT RATED FOR CONTINUOUS OPERATION**.<br>At 30 V input, $10\text{ k}\Omega$ limits calculated clamp current to $\approx 2.64\text{ mA}$ at $3.65\text{ V}$ clamp ($P_R = 69.4\text{ mW}$). This is an **electrical-stress calculation, NOT a guaranteed safe-input rating**.<br>**Maximum allowable input voltage and transient capability remain UNSPECIFIED** until complete protection network, 3.3 V rail behavior, diode surge characteristics, and failure modes are validated. |
| **0–3.3 V (Direct 1:1)** | **RECOMMENDED LIMIT** | **$0.100\text{ V} \text{ to } 3.100\text{ V}$** | Target $\pm 0.5\% \text{ F.S.}$ (Unverified) | Recommended linear operating zone of ESP32 ADC1 with 11 dB attenuation. |
| **0–5.0 V (Scaled 2:1)** | **THEORETICAL LIMIT** | $0.0\text{ V} \text{ to } 6.60\text{ V}$ | $1.61\text{ mV/count}$ | Attenuated via $2:1$ resistive divider ($100\text{ k}\Omega / 100\text{ k}\Omega$). |
| **0–5.0 V (Scaled 2:1)** | **TESTED LIMIT** | **$0.0000\text{ V}$ (GND baseline)** | **NOT VERIFIED** (Divider Model) | Input ratings remain **UNSPECIFIED** pending complete board-level validation. |
| **0–5.0 V (Scaled 2:1)** | **RECOMMENDED LIMIT** | **$0.200\text{ V} \text{ to } 5.000\text{ V}$** | Target $\pm 1.0\% \text{ F.S.}$ (Unverified) | Safe for standard $5\text{ V}$ microcontrollers (Arduino, TTL, USB bus). |

---

## 6. Self-Consistency vs Absolute Accuracy Distinction

> [!IMPORTANT]
> **Metrology Rule — Self-Consistency vs Independent External Reference**:
> Testing the ESP32 against its own signal generator, internal eFuse tables, or grounded baseline verifies **driver synchronization, register configuration, and peripheral counter fidelity** across the APB clock domain.
> For the DMM instrument, the status is explicitly:
> - **FIRMWARE & DRIVERS**: `IMPLEMENTED + FUNCTIONALLY TESTED`
> - **ABSOLUTE ACCURACY**: `NOT VERIFIED` until physically calibrated against an independent external precision reference meter.
