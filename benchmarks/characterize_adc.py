import serial
import json
import time
import statistics
import math

def send_command(ser, cmd):
    ser.reset_input_buffer()
    ser.write(json.dumps(cmd).encode() + b"\n")
    start = time.time()
    while time.time() - start < 1.5:
        line = ser.readline().decode('utf-8', errors='replace').strip()
        if line.startswith("{") and '"instrument"' in line:
            try:
                return json.loads(line)
            except:
                pass
    return None

def main():
    print("=" * 100)
    print("ESP32 DIGITAL LAB — DMM PHASE 1: ADC1 (GPIO 34) CHARACTERIZATION BENCHMARK")
    print("=" * 100)

    ser = serial.Serial('COM7', 115200, timeout=1.5)
    time.sleep(2.5)
    ser.reset_input_buffer()

    # Switch to DMM (Mode 5)
    print("\n[STEP 1] Switching to Digital Multimeter (Mode 5)...")
    res = send_command(ser, {"cmd": "switch_mode", "mode": 5})
    if res:
        print(f"  Active Instrument: {res.get('instrument', {}).get('name', 'Unknown')}")
        print(f"  GPIO Pin:          GPIO {res.get('instrument', {}).get('data', {}).get('gpio', 34)}")
        print(f"  Range:             {res.get('instrument', {}).get('data', {}).get('range', '0-3.3V')}")
        print(f"  eFuse Calibrated:  {res.get('instrument', {}).get('data', {}).get('efuse_cal', False)}")

    # Set oversampling to 128 samples per reading
    send_command(ser, {"action": "set_oversample", "samples": 128})
    time.sleep(0.2)

    # Collect 50 continuous telemetry bursts (6,400 raw ADC samples total)
    print("\n[STEP 2] Collecting 50 Burst Measurements (6,400 raw samples @ 128x oversample)...")
    voltages = []
    raw_adcs = []
    noise_devs = []
    v_pps = []

    print(f"\n{'Burst #':>8} | {'Voltage (V)':>12} | {'Raw ADC (0-4095)':>18} | {'Noise RMS (mV)':>16} | {'V_pp (mV)':>12}")
    print("-" * 75)

    for i in range(50):
        ser.write(b"READ\n")
        start = time.time()
        pkt = None
        while time.time() - start < 1.0:
            line = ser.readline().decode('utf-8', errors='replace').strip()
            if line.startswith("{") and '"instrument"' in line:
                try:
                    pkt = json.loads(line)
                    break
                except:
                    pass

        if pkt and pkt.get("instrument", {}).get("data"):
            d = pkt["instrument"]["data"]
            v = d.get("voltage_v", 0.0)
            raw = d.get("raw_adc", 0)
            noise = d.get("noise_std_dev_mv", 0.0)
            vpp = d.get("v_pp_mv", 0.0)

            voltages.append(v)
            raw_adcs.append(raw)
            noise_devs.append(noise)
            v_pps.append(vpp)

            if i < 10 or i % 10 == 0:
                print(f"{i+1:8d} | {v:12.4f} | {raw:18d} | {noise:16.2f} | {vpp:12.2f}")
        time.sleep(0.08)

    ser.close()

    # Metrology Analysis
    print("\n" + "=" * 100)
    print("ESP32 ADC1 METROLOGY CHARACTERIZATION REPORT (GPIO 34)")
    print("=" * 100)

    if voltages:
        mean_v = statistics.mean(voltages)
        std_v = statistics.stdev(voltages) if len(voltages) > 1 else 0.0
        min_v = min(voltages)
        max_v = max(voltages)
        mean_raw = statistics.mean(raw_adcs)
        mean_noise_mv = statistics.mean(noise_devs)
        mean_vpp_mv = statistics.mean(v_pps)

        print(f"  * Total Bursts Sampled:       {len(voltages)} (6,400 raw conversions)")
        print(f"  * Mean Voltage:              {mean_v:.5f} V ({mean_v*1000:.2f} mV)")
        print(f"  * Mean Raw ADC Code:          {mean_raw:.1f} / 4095 (12-bit)")
        print(f"  * Repeatability (Burst Stdev): +/-{std_v*1000:.3f} mV")
        print(f"  * Voltage Span (Min -> Max):  {min_v:.4f} V -> {max_v:.4f} V (Span = {(max_v - min_v)*1000:.2f} mV)")
        print(f"  * Average Intrinsic Noise (RMS): {mean_noise_mv:.3f} mV")
        print(f"  * Average Peak-to-Peak Noise: {mean_vpp_mv:.3f} mV")
        print("=" * 100)

if __name__ == "__main__":
    main()
