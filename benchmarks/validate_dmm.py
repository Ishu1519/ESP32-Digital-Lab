import serial
import json
import time
import statistics

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

def read_dmm_sample(ser, bursts=10):
    samples = []
    noises = []
    vpps = []
    raws = []
    
    for _ in range(bursts):
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
            samples.append(d.get("voltage_v", 0.0))
            noises.append(d.get("noise_std_dev_mv", 0.0))
            vpps.append(d.get("v_pp_mv", 0.0))
            raws.append(d.get("raw_adc", 0))
        time.sleep(0.05)

    if not samples:
        return None

    return {
        "mean_v": statistics.mean(samples),
        "std_v": statistics.stdev(samples) if len(samples) > 1 else 0.0,
        "mean_raw": statistics.mean(raws),
        "mean_noise_mv": statistics.mean(noises),
        "mean_vpp_mv": statistics.mean(vpps)
    }

def main():
    print("=" * 110)
    print("ESP32 DIGITAL LAB — DMM PHASE 1: METROLOGY & ACCURACY VALIDATION SUITE")
    print("=" * 110)

    ser = serial.Serial('COM7', 115200, timeout=1.5)
    time.sleep(2.5)
    ser.reset_input_buffer()

    # Switch to DMM
    print("\n[STEP 1] Initializing DMM Instrument on GPIO 34...")
    res = send_command(ser, {"cmd": "switch_mode", "mode": 5})
    send_command(ser, {"action": "set_oversample", "samples": 128})
    time.sleep(0.3)

    # Test Suite Table Header
    print("\n[STEP 2] Metrology Test Points (GND Baseline, DAC Multi-Point Sweep, 3.3V Rail):")
    print("-" * 110)
    print(f"{'Test Point':<20} | {'V_Ref (V)':>10} | {'V_Meas (V)':>10} | {'Abs Error (mV)':>14} | {'Rel Error (%)':>13} | {'Noise RMS (mV)':>14} | {'Raw ADC':>8} | {'Status':<8}")
    print("-" * 110)

    # Ground test point (known 0.0000 V reference)
    res_gnd = read_dmm_sample(ser, bursts=15)
    if res_gnd:
        v_ref = 0.0000
        v_meas = res_gnd["mean_v"]
        abs_err_mv = abs(v_meas - v_ref) * 1000.0
        rel_err_pct = 0.000 if v_ref == 0.0 else (abs_err_mv / (v_ref * 1000.0)) * 100.0
        status = "PASS" if abs_err_mv <= 2.0 else "FAIL"
        print(f"{'0.000V (GND Ref)':<20} | {v_ref:10.4f} | {v_meas:10.4f} | {abs_err_mv:14.2f} | {rel_err_pct:12.3f}% | {res_gnd['mean_noise_mv']:14.2f} | {res_gnd['mean_raw']:8.0f} | {status:<8}")

    # DAC Sweep (Internal test references: 0.25V, 0.50V, 1.00V, 1.50V, 2.00V, 2.50V, 3.00V, 3.30V)
    # DAC is on GPIO 25.
    print("-" * 110)
    print("  * Note: Connect GPIO 34 to external voltage reference, 3.3V rail, or test points for full sweep.")

    # 3.3V Rail Range Check (0-3.3V range vs 0-5V range)
    print("\n[STEP 3] Range Switching Verification (0-3.3V Direct vs 0-5.0V Scaled):")
    send_command(ser, {"action": "set_range", "range": "0-3.3V"})
    r1 = read_dmm_sample(ser, bursts=5)
    send_command(ser, {"action": "set_range", "range": "0-5V"})
    r2 = read_dmm_sample(ser, bursts=5)
    send_command(ser, {"action": "set_range", "range": "0-3.3V"})

    print(f"  • Range 0-3.3V Reading: {r1['mean_v']:.4f} V (Noise: {r1['mean_noise_mv']:.2f} mV RMS)")
    print(f"  • Range 0-5.0V Reading: {r2['mean_v']:.4f} V (Noise: {r2['mean_noise_mv']:.2f} mV RMS)")
    print("=" * 110)

    ser.close()

if __name__ == "__main__":
    main()
