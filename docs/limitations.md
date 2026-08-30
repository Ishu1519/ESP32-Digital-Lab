# ESP32 Digital Lab — Limitations & Specifications

Honest engineering specifications, theoretical limits, and tested operational parameters.

---

## 1. Frequency Counter (Milestone 1)

| Parameter | Theoretical Maximum | Recommended Range | Notes / Measured Characteristics |
| :--- | :--- | :--- | :--- |
| **Max Input Frequency** | 40.0 MHz | 10 Hz – 25 MHz | APB Clock / 2 is ~40 MHz theoretical. Higher frequencies require clean 3.3V CMOS square transitions. |
| **Min Input Frequency** | 0.01 Hz | 1 Hz | Sub-1 Hz requires long gate times (> 2000 ms). |
| **Input Voltage Range** | 0.0 V – 3.3 V | 0.0 V – 3.3 V | **Direct ESP32 GPIO input. NOT 5V tolerant! Use protection diode/divider.** |
| **Gate Accuracy** | $\pm 10\,\text{ppm}$ | $\pm 20\,\text{ppm}$ | Governed by onboard 40 MHz crystal oscillator. |
| **Pulse Width Resolution** | $\sim 1\,\mu\text{s}$ | $5\,\mu\text{s} – 1000\,\text{ms}$ | Measured via microsecond timestamping on low-to-medium frequencies. |

---

## 2. General System Limitations

1. **Low-Voltage Only**: Designed exclusively for embedded low-voltage DC signals ($0\text{V} - 3.3\text{V}$ direct, up to $24\text{V}$ with external attenuator). **NEVER connect to mains AC.**
2. **Internal ADC Linearity**: The original ESP32 ADC exhibits non-linear response below $0.15\text{V}$ and above $3.15\text{V}$. Milestone 3/4 incorporates polynomial calibration.
3. **Wi-Fi + Peripheral Bandwidth**: Heavy continuous ADC DMA streaming can experience brief Wi-Fi frame drops if task core affinity is not isolated. Core 0 handles Wi-Fi, Core 1 handles Instrument DSP.
