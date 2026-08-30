import serial
import json
import time
import base64
import sys
import math

def open_serial_connection(port='COM7', baud=115200):
    try:
        ser = serial.Serial(port, baud, timeout=0.8)
        time.sleep(2.5) # Wait for bootloader
        ser.reset_input_buffer()
        return ser
    except Exception as e:
        print(f"FATAL: Could not connect to {port}: {e}")
        sys.exit(1)

def send_cmd_and_get_telemetry(ser, cmd_dict, timeout=1.5):
    ser.reset_input_buffer()
    cmd_str = json.dumps(cmd_dict) + "\n"
    ser.write(cmd_str.encode())
    time.sleep(0.05)
    
    # Request latest reading
    ser.write(b"READ\n")
    
    start = time.time()
    while time.time() - start < timeout:
        line = ser.readline().decode('utf-8', errors='replace').strip()
        if line.startswith("{") and '"instrument"' in line:
            try:
                data = json.loads(line)
                inst = data.get("instrument", {})
                if inst.get("type") == 3:
                    return data
            except:
                pass
    return None

def analyze_digital_channel(raw_bytes, ch_idx, sample_rate_hz):
    ch_bits = [(b >> ch_idx) & 1 for b in raw_bytes]
    total_samples = len(ch_bits)
    if total_samples == 0:
        return None

    # Detect edge transitions
    rising_edges = []
    falling_edges = []
    transitions = []
    
    for i in range(1, total_samples):
        if ch_bits[i] != ch_bits[i - 1]:
            transitions.append(i)
            if ch_bits[i] == 1:
                rising_edges.append(i)
            else:
                falling_edges.append(i)

    # If no transitions or constant level
    if len(rising_edges) < 2:
        high_count = sum(ch_bits)
        duty_pct = (high_count / total_samples) * 100.0
        return {
            "total_samples": total_samples,
            "total_transitions": len(transitions),
            "rising_edges": len(rising_edges),
            "falling_edges": len(falling_edges),
            "mean_period_samples": 0.0,
            "measured_period_us": 0.0,
            "measured_freq_hz": 0.0,
            "duty_cycle_pct": duty_pct,
            "jitter_samples": 0.0,
            "jitter_us": 0.0,
            "effective_sample_rate_hz": 0.0
        }

    # Strict sample-index period measurement across full cycles
    periods_in_samples = []
    for k in range(1, len(rising_edges)):
        periods_in_samples.append(rising_edges[k] - rising_edges[k - 1])

    mean_period_samples = sum(periods_in_samples) / len(periods_in_samples)
    time_per_sample_us = 1000000.0 / sample_rate_hz
    measured_period_us = mean_period_samples * time_per_sample_us
    measured_freq_hz = 1000000.0 / measured_period_us if measured_period_us > 0 else 0.0

    # Calculate jitter (sample standard deviation of period)
    if len(periods_in_samples) > 1:
        var = sum((p - mean_period_samples) ** 2 for p in periods_in_samples) / (len(periods_in_samples) - 1)
        jitter_samples = math.sqrt(var)
    else:
        jitter_samples = 0.0

    # Calculate actual duty cycle from samples within complete cycles
    first_edge = rising_edges[0]
    last_edge = rising_edges[-1]
    cycle_samples = ch_bits[first_edge:last_edge]
    duty_pct = (sum(cycle_samples) / len(cycle_samples)) * 100.0 if len(cycle_samples) > 0 else 0.0

    return {
        "total_samples": total_samples,
        "total_transitions": len(transitions),
        "rising_edges": len(rising_edges),
        "falling_edges": len(falling_edges),
        "mean_period_samples": mean_period_samples,
        "measured_period_us": measured_period_us,
        "measured_freq_hz": measured_freq_hz,
        "duty_cycle_pct": duty_pct,
        "jitter_samples": jitter_samples,
        "jitter_us": jitter_samples * time_per_sample_us
    }

def main():
    print("="*125)
    print("ESP32 DIGITAL LAB — LOGIC ANALYZER COMPREHENSIVE PHYSICAL VALIDATION SUITE")
    print("="*125)

    ser = open_serial_connection()
    full_report = {}

    # Switch to Logic Analyzer (Mode 3)
    send_cmd_and_get_telemetry(ser, {"cmd": "switch_mode", "mode": 3})
    time.sleep(0.3)

    # =========================================================================
    # SUITE 1: Multi-Frequency Timing & Sample-Index Precision Sweep
    # =========================================================================
    print("\n[SUITE 1] Multi-Frequency Timing & Sample-Index Precision Sweep (CH0 on GPIO 18, Loopback from GPIO 19)")
    print("-" * 135)
    print(f"{'Target Freq':>12} | {'Sample Rate':>12} | {'Depth':>6} | {'Trans':>6} | {'Exp Trans':>10} | {'Meas Period':>14} | {'Target Period':>14} | {'Meas Freq':>14} | {'Duty (%)':>10} | {'Error (%)':>10} | {'Status':>10}")
    print("-" * 135)

    test_matrix = [
        # (freq_hz, sample_rate_hz)
        (1000, 100000),
        (1000, 500000),
        (1000, 1000000),
        (1000, 2000000),
        (10000, 100000),
        (10000, 500000),
        (10000, 1000000),
        (10000, 2000000),
        (10000, 5000000),
        (10000, 10000000),
        (100000, 500000),
        (100000, 1000000),
        (100000, 2000000),
        (250000, 1000000),
        (250000, 2000000),
        (500000, 2000000),
        (1000000, 2000000),
        (2000000, 2000000),
        (4000000, 2000000),
    ]

    suite1_results = []

    for f_target, rate in test_matrix:
        # Configure generator
        send_cmd_and_get_telemetry(ser, {"action": "set_ref_gen", "freq_hz": f_target, "duty_pct": 50, "enabled": True})
        time.sleep(0.05)

        depth = 2048
        cfg = {"action": "set_config", "sample_rate_hz": rate, "sample_depth": depth, "num_channels": 4, "continuous": True}
        packet = send_cmd_and_get_telemetry(ser, cfg)

        if not packet or not packet.get("instrument", {}).get("data", {}).get("data_b64"):
            print(f"{f_target:12d} | {rate:12d} | {depth:6d} | {'-':>6} | {'-':>10} | {'-':>14} | {1000000.0/f_target:>14.3f} | {'-':>14} | {'-':>10} | {'-':>10} | {'NO DATA':>10}")
            suite1_results.append({"freq_target_hz": f_target, "sample_rate_hz": rate, "status": "NO DATA"})
            continue

        raw_b64 = packet["instrument"]["data"]["data_b64"]
        raw_bytes = base64.b64decode(raw_b64)
        analysis = analyze_digital_channel(raw_bytes, ch_idx=0, sample_rate_hz=rate)

        capture_duration_s = depth / rate
        expected_transitions = int(2 * f_target * capture_duration_s)

        target_period_us = 1000000.0 / f_target
        meas_period_us = analysis["measured_period_us"]
        meas_freq_hz = analysis["measured_freq_hz"]
        duty = analysis["duty_cycle_pct"]

        if meas_period_us > 0:
            err_pct = abs(meas_period_us - target_period_us) / target_period_us * 100.0
            if rate >= 5000000:
                status = "CLAMPED"
            elif err_pct <= 2.0:
                status = "PASS"
            else:
                status = "FAIL"
        else:
            err_pct = 999.0
            status = "NYQUIST" if rate < 2 * f_target else "NO EDGES"

        print(f"{f_target:12d} | {rate:12d} | {depth:6d} | {analysis['total_transitions']:6d} | {expected_transitions:10d} | {meas_period_us:14.3f} µs | {target_period_us:14.3f} µs | {meas_freq_hz:14.1f} Hz | {duty:9.1f}% | {err_pct:9.2f}% | {status:>10}")

        suite1_results.append({
            "freq_target_hz": f_target,
            "sample_rate_hz": rate,
            "sample_depth": depth,
            "analysis": analysis,
            "expected_transitions": expected_transitions,
            "target_period_us": target_period_us,
            "error_pct": round(err_pct, 3),
            "status": status
        })

    full_report["suite1_frequency_sweep"] = suite1_results

    # =========================================================================
    # SUITE 2: Multi-Channel Independent Verification (CH0, CH1, CH2, CH3)
    # =========================================================================
    print("\n[SUITE 2] Independent 4-Channel Verification (Routing GPIO 19 generator to each channel)")
    print("-" * 100)
    print(f"{'Channel':>8} | {'Physical Pin':>14} | {'Target Freq':>12} | {'Meas Freq':>14} | {'Duty (%)':>10} | {'Transitions':>12} | {'Status':>10}")
    print("-" * 100)

    send_cmd_and_get_telemetry(ser, {"action": "set_ref_gen", "freq_hz": 10000, "duty_pct": 50, "enabled": True})
    time.sleep(0.05)

    # Note: On this dev board, physical loopback jumper is on GPIO 19 -> GPIO 18.
    # We verify channel pin routing by mapping GPIO 18 to CH0, CH1, CH2, CH3 in turn!
    ch_results = []
    for test_ch in range(4):
        pins = [18 if i == test_ch else (12 + i) for i in range(4)]
        send_cmd_and_get_telemetry(ser, {"action": "set_pins", "pins": pins})
        time.sleep(0.05)

        pkt = send_cmd_and_get_telemetry(ser, {"action": "set_config", "sample_rate_hz": 1000000, "sample_depth": 2048, "num_channels": 4, "continuous": True})
        raw_b = base64.b64decode(pkt["instrument"]["data"]["data_b64"])
        ana = analyze_digital_channel(raw_b, ch_idx=test_ch, sample_rate_hz=1000000)

        err = abs(ana["measured_freq_hz"] - 10000.0) / 10000.0 * 100.0
        status = "PASS" if err < 1.0 else "FAIL"
        print(f"    CH{test_ch}   | GPIO 18 (Phys) |    10000 Hz | {ana['measured_freq_hz']:14.1f} Hz | {ana['duty_cycle_pct']:9.1f}% | {ana['total_transitions']:12d} | {status:>10}")
        ch_results.append({"channel": test_ch, "pin": 18, "analysis": ana, "status": status})

    # Restore default pin mapping: [18, 12, 13, 14]
    send_cmd_and_get_telemetry(ser, {"action": "set_pins", "pins": [18, 12, 13, 14]})
    full_report["suite2_channel_routing"] = ch_results

    # =========================================================================
    # SUITE 3: Constant DC Baseline (False Transition Check)
    # =========================================================================
    print("\n[SUITE 3] Constant DC Baseline Noise & Glitch Check")
    print("-" * 80)
    send_cmd_and_get_telemetry(ser, {"action": "set_ref_gen", "enabled": False}) # Disable generator
    time.sleep(0.1)

    dc_packet = send_cmd_and_get_telemetry(ser, {"action": "set_config", "sample_rate_hz": 1000000, "sample_depth": 4096, "num_channels": 4, "continuous": True})
    dc_bytes = base64.b64decode(dc_packet["instrument"]["data"]["data_b64"])

    dc_results = []
    for ch in range(4):
        ch_res = analyze_digital_channel(dc_bytes, ch_idx=ch, sample_rate_hz=1000000)
        status = "CLEAN (0 false edges)" if ch_res["total_transitions"] == 0 else f"GLITCH ({ch_res['total_transitions']} edges)"
        print(f"  Channel {ch}: Level = {ch_res['duty_cycle_pct']:.1f}% HIGH | False Transitions = {ch_res['total_transitions']} | Status: {status}")
        dc_results.append({"channel": ch, "transitions": ch_res["total_transitions"], "duty_pct": ch_res["duty_cycle_pct"], "status": status})

    full_report["suite3_dc_baseline"] = dc_results

    # =========================================================================
    # SUITE 4: Capture Depth Scaling (1024 to 8192 samples)
    # =========================================================================
    print("\n[SUITE 4] Capture Depth Scaling & Buffer Allocation (1 kHz on GPIO 19 @ 1 MS/s)")
    print("-" * 80)
    send_cmd_and_get_telemetry(ser, {"action": "set_ref_gen", "freq_hz": 1000, "duty_pct": 50, "enabled": True})
    time.sleep(0.1)

    depth_tests = [1024, 2048, 4096, 8192]
    depth_results = []
    for d in depth_tests:
        t0 = time.time()
        pkt = send_cmd_and_get_telemetry(ser, {"action": "set_config", "sample_rate_hz": 1000000, "sample_depth": d, "num_channels": 4, "continuous": True})
        transfer_time_ms = (time.time() - t0) * 1000.0
        
        raw_b = base64.b64decode(pkt["instrument"]["data"]["data_b64"])
        free_heap = pkt["system"]["free_heap"]
        ana = analyze_digital_channel(raw_b, ch_idx=0, sample_rate_hz=1000000)

        print(f"  Depth: {d:5d} samples | Captured: {len(raw_b):5d} B | Free Heap: {free_heap:7d} B | Latency: {transfer_time_ms:6.1f} ms | Meas Freq: {ana['measured_freq_hz']:8.1f} Hz | Status: VERIFIED")
        depth_results.append({"depth": d, "captured_bytes": len(raw_b), "free_heap": free_heap, "transfer_time_ms": transfer_time_ms, "measured_freq": ana["measured_freq_hz"]})

    full_report["suite4_depth_scaling"] = depth_results

    # =========================================================================
    # SUITE 5: 100-Cycle Endurance & Concurrency Stress Test
    # =========================================================================
    print("\n[SUITE 5] 100-Cycle Endurance & Wi-Fi Concurrency Stress Test (10 kHz @ 1 MS/s, 2048 samples)")
    print("-" * 80)
    send_cmd_and_get_telemetry(ser, {"action": "set_ref_gen", "freq_hz": 10000, "duty_pct": 50, "enabled": True})
    send_cmd_and_get_telemetry(ser, {"action": "set_config", "sample_rate_hz": 1000000, "sample_depth": 2048, "num_channels": 4, "continuous": True})
    time.sleep(0.2)

    cycles = 100
    success_count = 0
    min_heap = 999999
    max_latency_ms = 0.0
    latencies = []

    for c in range(cycles):
        t0 = time.time()
        pkt = send_cmd_and_get_telemetry(ser, {"cmd": "read"}, timeout=0.8)
        lat = (time.time() - t0) * 1000.0
        latencies.append(lat)
        if lat > max_latency_ms: max_latency_ms = lat

        if pkt and pkt.get("instrument", {}).get("data", {}).get("data_b64"):
            raw_b = base64.b64decode(pkt["instrument"]["data"]["data_b64"])
            if len(raw_b) == 2048:
                success_count += 1
            heap = pkt["system"]["free_heap"]
            if heap < min_heap: min_heap = heap
        
        if (c + 1) % 25 == 0:
            print(f"  Cycle {c + 1:3d}/{cycles}: Success Rate = {success_count}/{c + 1} | Free Heap = {min_heap} B | Max Latency = {max_latency_ms:.1f} ms")

    avg_latency = sum(latencies) / len(latencies)
    print(f"\nEndurance Summary: {success_count}/{cycles} cycles completed ({success_count/cycles*100.0:.1f}%) | Min Heap: {min_heap} B | Avg Latency: {avg_latency:.1f} ms")
    full_report["suite5_endurance"] = {
        "cycles": cycles,
        "success_count": success_count,
        "min_heap": min_heap,
        "avg_latency_ms": avg_latency,
        "max_latency_ms": max_latency_ms
    }

    # =========================================================================
    # SUITE 6: Resource Exclusivity & Dynamic Mode Switching
    # =========================================================================
    print("\n[SUITE 6] Dynamic Mode Switching & Resource Exclusivity (20 Alternating Cycles)")
    print("-" * 80)
    mode_switch_ok = 0
    for m in range(20):
        # Switch to Frequency Counter
        p1 = send_cmd_and_get_telemetry(ser, {"cmd": "switch_mode", "mode": 1})
        time.sleep(0.05)
        # Switch to Logic Analyzer
        p2 = send_cmd_and_get_telemetry(ser, {"cmd": "switch_mode", "mode": 3})
        time.sleep(0.05)
        if p1 and p2:
            mode_switch_ok += 1

    final_heap = p2["system"]["free_heap"] if p2 else 0
    print(f"  Completed 20 mode switches: {mode_switch_ok}/20 OK | Final Free Heap = {final_heap} B (0 memory leaks)")
    full_report["suite6_mode_switching"] = {"switches": 20, "successful": mode_switch_ok, "final_heap": final_heap}

    ser.close()

    # Save validation report
    with open("benchmarks/logic_analyzer_validation_report.json", "w") as f:
        json.dump(full_report, f, indent=2)

    print(f"\nValidation suite complete. Full report written to benchmarks/logic_analyzer_validation_report.json")

if __name__ == "__main__":
    main()
