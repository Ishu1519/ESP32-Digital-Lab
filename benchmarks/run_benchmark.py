import serial
import json
import time
import sys

def main():
    port = 'COM7'
    baud = 115200
    print(f"Connecting to ESP32 Digital Lab on {port} @ {baud}...")

    try:
        ser = serial.Serial(port, baud, timeout=1.0)
    except Exception as e:
        print(f"ERROR: Could not open {port}: {e}")
        sys.exit(1)

    time.sleep(1.0)
    while ser.in_waiting:
        ser.read(ser.in_waiting)

    print("ESP32 Serial Ready. Running Milestone 1 Self-Consistency Benchmark...\n")

    test_frequencies = [
        1, 2, 5, 10, 20,
        50, 100, 500, 1000, 5000,
        10000, 50000, 100000, 500000,
        1000000, 2000000, 5000000, 10000000, 15000000, 20000000
    ]

    print("="*120)
    print(f"{'Target (Hz)':>12} | {'Gen Mode':>8} | {'Res':>4} | {'div_param':>9} | {'Calculated Gen':>15} | {'PCNT Measured':>15} | {'Error (%)':>10} | {'Status':>12}")
    print("="*120)

    results = []

    for freq in test_frequencies:
        gate_time_ms = 2000 if freq == 1 else (1000 if freq <= 5 else 500)
        
        # Configure Gate Time
        gate_cmd = {"action": "set_gate_time", "gate_time_ms": gate_time_ms}
        ser.write((json.dumps(gate_cmd) + "\n").encode())
        time.sleep(0.05)

        # Set Reference Generator Frequency
        gen_cmd = {"action": "set_ref_gen", "freq_hz": freq, "duty_pct": 50, "enabled": True}
        ser.write((json.dumps(gen_cmd) + "\n").encode())

        # Wait at least 3 gate cycles for steady state
        settle_time = (gate_time_ms * 3.2) / 1000.0
        time.sleep(settle_time)

        # Drain serial buffer
        while ser.in_waiting:
            ser.read(ser.in_waiting)

        # Take 3 successive samples
        samples = []
        gen_info = {}
        for _ in range(3):
            ser.write(b"READ\n")
            time.sleep((gate_time_ms * 1.1) / 1000.0)
            start = time.time()
            while time.time() - start < 0.6:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if line.startswith("{") and "instrument" in line:
                    try:
                        data = json.loads(line)
                        inst = data.get("instrument", {}).get("data", {})
                        if "freq_hz" in inst:
                            samples.append(inst.get("freq_hz", 0.0))
                            gen_info = inst.get("ref_gen", {})
                            break
                    except:
                        pass

        if not samples:
            print(f"{freq:12d} | {'NO RESPONSE':>8} | {'-':>4} | {'-':>9} | {'-':>15} | {'-':>15} | {'-':>10} | {'FAIL':>12}")
            results.append({
                "requested_hz": freq,
                "gen_mode": "UNKNOWN",
                "res_bits": 0,
                "div_param": 0,
                "calc_gen_hz": 0.0,
                "pcnt_meas_hz": 0.0,
                "error_pct": 100.0,
                "status": "FAIL"
            })
            continue

        avg_meas = sum(samples) / len(samples)
        mode = "Timer" if freq < 50 else "LEDC"
        res = gen_info.get("res_bits", 0)
        div_p = gen_info.get("div_param", 0)
        calc_f = gen_info.get("calc_freq_hz", 0.0)

        # For self-consistency, compare measured against the actual calculated generator output
        ref_target = calc_f if calc_f > 0 else freq
        err_pct = abs(avg_meas - ref_target) / ref_target * 100.0 if ref_target > 0 else 0.0

        if freq > 5000000:
            status = "LEDC_LIMIT (5MHz)"
        elif err_pct <= 0.5:
            status = "VERIFIED"
        else:
            status = "UNSTABLE"

        print(f"{freq:12d} | {mode:>8} | {res:4d} | {div_p:9d} | {calc_f:15.2f} | {avg_meas:15.2f} | {err_pct:9.3f}% | {status:>12}")

        results.append({
            "requested_hz": freq,
            "gen_mode": mode,
            "res_bits": res,
            "div_param": div_p,
            "calc_gen_hz": calc_f,
            "pcnt_meas_hz": avg_meas,
            "error_pct": err_pct,
            "status": status
        })

    ser.close()

    with open("benchmarks/results.json", "w") as f:
        json.dump({"sweep": results, "test_type": "ESP32 Internal Loopback Self-Consistency"}, f, indent=2)
    print("\nBenchmark results successfully recorded in benchmarks/results.json")

if __name__ == "__main__":
    main()
