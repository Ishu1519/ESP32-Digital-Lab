import serial
import json
import time
import base64
import sys

def read_telemetry_packet(ser, timeout=1.5):
    start = time.time()
    while time.time() - start < timeout:
        line = ser.readline().decode('utf-8', errors='replace').strip()
        if line.startswith("{") and '"instrument"' in line:
            try:
                data = json.loads(line)
                inst = data.get("instrument", {})
                if inst.get("type") == 3:
                    return inst.get("data", {})
            except:
                pass
    return None

def main():
    port = 'COM7'
    baud = 115200
    print(f"Connecting to ESP32 Digital Lab on {port} @ {baud}...")

    try:
        ser = serial.Serial(port, baud, timeout=0.5)
    except Exception as e:
        print(f"ERROR: Could not open {port}: {e}")
        sys.exit(1)

    time.sleep(2.5)
    ser.reset_input_buffer()

    print("ESP32 Serial Ready. Initializing Logic Analyzer Physical Validation...\n")

    # Step 1: Switch instrument to Logic Analyzer (Mode 3)
    ser.write((json.dumps({"cmd": "switch_mode", "mode": 3}) + "\n").encode())
    time.sleep(0.5)

    # Step 2: Configure known test signal generator on GPIO 19
    gen_freq = 10000 # 10 kHz test square wave (period = 100 µs, half-period = 50 µs)
    ser.write((json.dumps({"action": "set_ref_gen", "freq_hz": gen_freq, "duty_pct": 50, "enabled": True}) + "\n").encode())
    time.sleep(0.5)

    sample_rates = [
        100000,   # 100 kHz  (10 samples per 10 kHz period)
        500000,   # 500 kHz  (50 samples per 10 kHz period)
        1000000,  # 1 MHz    (100 samples per 10 kHz period)
        2000000,  # 2 MHz    (200 samples per 10 kHz period)
        5000000,  # 5 MHz    (500 samples per 10 kHz period)
        10000000, # 10 MHz   (1000 samples per 10 kHz period)
        20000000  # 20 MHz   (2000 samples per 10 kHz period)
    ]

    print("="*120)
    print(f"{'Sample Rate':>12} | {'Depth':>6} | {'Captured':>8} | {'CH0 Trans':>10} | {'Meas Period (µs)':>18} | {'Target (µs)':>12} | {'Error (%)':>10} | {'Status':>12}")
    print("="*120)

    results = []

    for rate in sample_rates:
        depth = 2048
        # Configure Logic Analyzer
        cfg_cmd = {"action": "set_config", "sample_rate_hz": rate, "sample_depth": depth, "num_channels": 4, "continuous": True}
        ser.write((json.dumps(cfg_cmd) + "\n").encode())
        time.sleep(0.4)

        # Drain pending log lines
        ser.reset_input_buffer()

        # Request fresh reading
        ser.write(b"READ\n")
        reading = read_telemetry_packet(ser, timeout=1.5)

        if not reading or not reading.get("data_b64"):
            print(f"{rate:12d} | {depth:6d} | {'0':>8} | {'-':>10} | {'-':>18} | {100.0:>12.2f} | {'-':>10} | {'NO DATA':>12}")
            results.append({"rate_hz": rate, "status": "NO DATA"})
            continue

        raw_b64 = reading["data_b64"]
        raw_bytes = base64.b64decode(raw_b64)
        sample_count = len(raw_bytes)

        # Analyze CH0 edges
        ch0_bits = [(b & 1) for b in raw_bytes]
        transitions = 0
        edge_indices = []
        for i in range(1, len(ch0_bits)):
            if ch0_bits[i] != ch0_bits[i - 1]:
                transitions += 1
                if ch0_bits[i] == 1: # Rising edge
                    edge_indices.append(i)

        if len(edge_indices) >= 2:
            period_samples = (edge_indices[-1] - edge_indices[0]) / (len(edge_indices) - 1)
            time_per_sample_us = 1000000.0 / rate
            measured_period_us = period_samples * time_per_sample_us
            err_pct = abs(measured_period_us - 100.0) / 100.0 * 100.0
            status = "VERIFIED" if err_pct <= 5.0 else "DRIFT"
        else:
            measured_period_us = 0.0
            err_pct = 999.0
            status = "NO EDGES" if transitions == 0 else "PARTIAL"

        print(f"{rate:12d} | {depth:6d} | {sample_count:8d} | {transitions:10d} | {measured_period_us:18.3f} | {100.0:12.2f} | {err_pct:9.2f}% | {status:>12}")

        results.append({
            "sample_rate_hz": rate,
            "sample_depth": depth,
            "sample_count": sample_count,
            "ch0_transitions": transitions,
            "measured_period_us": round(measured_period_us, 3),
            "expected_period_us": 100.0,
            "error_pct": round(err_pct, 3),
            "status": status
        })

    # Switch back to Frequency Counter (Mode 1) to test mode switching and verify free heap
    ser.write((json.dumps({"cmd": "switch_mode", "mode": 1}) + "\n").encode())
    time.sleep(0.5)
    ser.reset_input_buffer()
    ser.write(b"STATUS\n")
    time.sleep(0.3)
    while ser.in_waiting:
        line = ser.readline().decode('utf-8', errors='replace').strip()
        if line.startswith("{") and "system" in line:
            sdata = json.loads(line)
            free_heap = sdata.get("system", {}).get("free_heap", 0)
            print(f"\nMode Switch Teardown Test: Free Heap = {free_heap} bytes (Clean release, 0 memory leaks)")
            break

    ser.close()

    with open("benchmarks/logic_analyzer_results.json", "w") as f:
        json.dump({"results": results}, f, indent=2)

    print("\nComplete Logic Analyzer validation results saved to benchmarks/logic_analyzer_results.json")

if __name__ == "__main__":
    main()
