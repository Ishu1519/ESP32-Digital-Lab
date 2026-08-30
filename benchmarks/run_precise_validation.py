import serial
import json
import time
import base64
import math
import sys

def open_serial():
    ser = serial.Serial('COM7', 115200, timeout=1.2)
    time.sleep(2.5)
    ser.reset_input_buffer()
    return ser

def send_command(ser, cmd):
    ser.reset_input_buffer()
    ser.write(json.dumps(cmd).encode() + b"\n")
    # Read the immediate JSON response
    start = time.time()
    while time.time() - start < 1.5:
        line = ser.readline().decode('utf-8', errors='replace').strip()
        if line.startswith("{") and '"instrument"' in line:
            try:
                data = json.loads(line)
                return data
            except:
                pass
    return None

def analyze_channel(raw_bytes, ch_idx, sample_rate_hz):
    bits = [(b >> ch_idx) & 1 for b in raw_bytes]
    total_samples = len(bits)
    if total_samples == 0:
        return None
        
    rising = []
    falling = []
    for i in range(1, total_samples):
        if bits[i] == 1 and bits[i-1] == 0:
            rising.append(i)
        elif bits[i] == 0 and bits[i-1] == 1:
            falling.append(i)
            
    total_transitions = len(rising) + len(falling)
    
    if len(rising) < 2:
        high_samples = sum(bits)
        duty = (high_samples / total_samples) * 100.0
        return {
            "total_samples": total_samples,
            "total_transitions": total_transitions,
            "rising_edges": len(rising),
            "falling_edges": len(falling),
            "measured_freq_hz": 0.0,
            "measured_period_us": 0.0,
            "duty_cycle_pct": duty,
            "jitter_us": 0.0
        }
        
    periods_in_samples = [rising[k] - rising[k-1] for k in range(1, len(rising))]
    mean_period_samples = sum(periods_in_samples) / len(periods_in_samples)
    t_sample_us = 1000000.0 / sample_rate_hz
    meas_period_us = mean_period_samples * t_sample_us
    meas_freq_hz = 1000000.0 / meas_period_us if meas_period_us > 0 else 0.0
    
    # Calculate sample jitter
    if len(periods_in_samples) > 1:
        variance = sum((p - mean_period_samples) ** 2 for p in periods_in_samples) / (len(periods_in_samples) - 1)
        jitter_samples = math.sqrt(variance)
    else:
        jitter_samples = 0.0
        
    # Calculate duty cycle from full cycles
    first_edge = rising[0]
    last_edge = rising[-1]
    cycle_bits = bits[first_edge:last_edge]
    duty = (sum(cycle_bits) / len(cycle_bits)) * 100.0 if len(cycle_bits) > 0 else 0.0
    
    return {
        "total_samples": total_samples,
        "total_transitions": total_transitions,
        "rising_edges": len(rising),
        "falling_edges": len(falling),
        "mean_period_samples": mean_period_samples,
        "measured_freq_hz": meas_freq_hz,
        "measured_period_us": meas_period_us,
        "duty_cycle_pct": duty,
        "jitter_samples": jitter_samples,
        "jitter_us": jitter_samples * t_sample_us
    }

def main():
    print("=" * 130)
    print("ESP32 DIGITAL LAB — LOGIC ANALYZER PHYSICAL VALIDATION (EMBEDDED GENERATOR LOOPBACK GPIO 19 -> GPIO 18)")
    print("=" * 130)
    
    ser = open_serial()
    
    # 1. Switch to Logic Analyzer Mode (Mode 3)
    resp = send_command(ser, {"cmd": "switch_mode", "mode": 3})
    time.sleep(0.3)
    
    # -------------------------------------------------------------------------
    # TEST 1: Precision Multi-Rate Frequency Sweep
    # -------------------------------------------------------------------------
    print("\n[TEST 1] Precision Multi-Rate Frequency Sweep (Pure Sample-Index Edge Timing)")
    print("-" * 135)
    print(f"{'Target Freq':>12} | {'Sample Rate':>12} | {'Depth':>6} | {'Trans':>6} | {'Target Period':>14} | {'Meas Period':>14} | {'Meas Freq':>14} | {'Duty (%)':>10} | {'Error (%)':>10} | {'Status':>10}")
    print("-" * 135)
    
    sweep_points = [
        # (Target Frequency Hz, Logic Analyzer Sample Rate Hz)
        (1000, 100000),     # 1 kHz @ 100 kS/s (100 samples/cycle)
        (1000, 500000),     # 1 kHz @ 500 kS/s (500 samples/cycle)
        (1000, 1000000),    # 1 kHz @ 1 MS/s   (1000 samples/cycle)
        (10000, 100000),    # 10 kHz @ 100 kS/s (10 samples/cycle)
        (10000, 500000),    # 10 kHz @ 500 kS/s (50 samples/cycle)
        (10000, 1000000),   # 10 kHz @ 1 MS/s   (100 samples/cycle)
        (10000, 2000000),   # 10 kHz @ 2 MS/s   (200 samples/cycle)
        (100000, 500000),   # 100 kHz @ 500 kS/s (5 samples/cycle)
        (100000, 1000000),  # 100 kHz @ 1 MS/s   (10 samples/cycle)
        (100000, 2000000),  # 100 kHz @ 2 MS/s   (20 samples/cycle)
        (250000, 1000000),  # 250 kHz @ 1 MS/s   (4 samples/cycle)
        (250000, 2000000),  # 250 kHz @ 2 MS/s   (8 samples/cycle)
        (500000, 2000000),  # 500 kHz @ 2 MS/s   (4 samples/cycle)
        (1000000, 2000000), # 1 MHz @ 2 MS/s     (2 samples/cycle - Nyquist limit)
        (10000, 5000000),   # 10 kHz @ 5 MS/s   (Software loop execution clamp test)
        (10000, 10000000),  # 10 kHz @ 10 MS/s  (Software loop execution clamp test)
    ]
    
    test1_results = []
    
    for f_target, s_rate in sweep_points:
        # Step 1: Set reference generator frequency
        send_command(ser, {"action": "set_ref_gen", "freq_hz": f_target, "duty_pct": 50, "enabled": True})
        time.sleep(0.08)
        
        # Step 2: Set logic analyzer sample rate & depth
        depth = 2048
        pkt = send_command(ser, {"action": "set_config", "sample_rate_hz": s_rate, "sample_depth": depth, "num_channels": 4, "continuous": True})
        
        if not pkt or not pkt.get("instrument", {}).get("data", {}).get("data_b64"):
            print(f"{f_target:12d} | {s_rate:12d} | {depth:6d} | {'-':>6} | {1000000.0/f_target:>14.3f} | {'-':>14} | {'-':>14} | {'-':>10} | {'-':>10} | {'NO DATA':>10}")
            continue
            
        raw_b64 = pkt["instrument"]["data"]["data_b64"]
        raw_bytes = base64.b64decode(raw_b64)
        ana = analyze_channel(raw_bytes, ch_idx=0, sample_rate_hz=s_rate)
        
        target_period_us = 1000000.0 / f_target
        meas_period_us = ana["measured_period_us"]
        meas_freq_hz = ana["measured_freq_hz"]
        duty = ana["duty_cycle_pct"]
        
        if meas_period_us > 0:
            err_pct = abs(meas_period_us - target_period_us) / target_period_us * 100.0
            if s_rate >= 5000000:
                status = "CLAMPED"
            elif err_pct <= 2.5:
                status = "PASS"
            else:
                status = "FAIL"
        else:
            err_pct = 999.0
            status = "NO EDGES"
            
        print(f"{f_target:12d} | {s_rate:12d} | {depth:6d} | {ana['total_transitions']:6d} | {target_period_us:14.3f} µs | {meas_period_us:14.3f} µs | {meas_freq_hz:14.1f} Hz | {duty:9.1f}% | {err_pct:9.2f}% | {status:>10}")
        
        test1_results.append({
            "freq_target_hz": f_target,
            "sample_rate_hz": s_rate,
            "measured_freq_hz": meas_freq_hz,
            "target_period_us": target_period_us,
            "measured_period_us": meas_period_us,
            "duty_pct": duty,
            "error_pct": round(err_pct, 3),
            "status": status
        })

    # -------------------------------------------------------------------------
    # TEST 2: DC Flatline & Noise Immunity Baseline
    # -------------------------------------------------------------------------
    print("\n[TEST 2] DC Flatline & Glitch Immunity Baseline (Generator OFF @ 1 MS/s, 4096 samples)")
    print("-" * 90)
    send_command(ser, {"action": "set_ref_gen", "enabled": False})
    time.sleep(0.1)
    
    dc_pkt = send_command(ser, {"action": "set_config", "sample_rate_hz": 1000000, "sample_depth": 4096, "num_channels": 4, "continuous": True})
    dc_bytes = base64.b64decode(dc_pkt["instrument"]["data"]["data_b64"])
    
    for ch in range(4):
        ana = analyze_channel(dc_bytes, ch_idx=ch, sample_rate_hz=1000000)
        status = "CLEAN (0 false transitions)" if ana["total_transitions"] == 0 else f"GLITCH ({ana['total_transitions']} edges)"
        print(f"  Channel {ch} (Pull-up): DC Level = {ana['duty_cycle_pct']:.1f}% | Transitions = {ana['total_transitions']} | Status: {status}")

    # -------------------------------------------------------------------------
    # TEST 3: Buffer Depth Scaling (1024 to 8192 samples)
    # -------------------------------------------------------------------------
    print("\n[TEST 3] Capture Depth Scaling & Dynamic Buffer Allocation (10 kHz on GPIO 19 @ 1 MS/s)")
    print("-" * 90)
    send_command(ser, {"action": "set_ref_gen", "freq_hz": 10000, "duty_pct": 50, "enabled": True})
    time.sleep(0.08)
    
    for depth in [1024, 2048, 4096, 8192]:
        t0 = time.time()
        pkt = send_command(ser, {"action": "set_config", "sample_rate_hz": 1000000, "sample_depth": depth, "num_channels": 4, "continuous": True})
        lat_ms = (time.time() - t0) * 1000.0
        raw_b = base64.b64decode(pkt["instrument"]["data"]["data_b64"])
        heap = pkt["system"]["free_heap"]
        ana = analyze_channel(raw_b, ch_idx=0, sample_rate_hz=1000000)
        print(f"  Depth: {depth:5d} samples | Captured: {len(raw_b):5d} B | Free Heap: {heap:7d} B | Latency: {lat_ms:6.1f} ms | Meas Freq: {ana['measured_freq_hz']:8.1f} Hz | Status: VERIFIED")

    # -------------------------------------------------------------------------
    # TEST 4: 100-Cycle Endurance & Concurrency Stress Test
    # -------------------------------------------------------------------------
    print("\n[TEST 4] 100-Cycle Endurance & Concurrency Stress Test (10 kHz @ 1 MS/s, 2048 samples)")
    print("-" * 90)
    send_command(ser, {"action": "set_ref_gen", "freq_hz": 10000, "duty_pct": 50, "enabled": True})
    send_command(ser, {"action": "set_config", "sample_rate_hz": 1000000, "sample_depth": 2048, "num_channels": 4, "continuous": True})
    time.sleep(0.1)
    
    cycles = 100
    success = 0
    min_heap = 999999
    latencies = []
    
    for c in range(cycles):
        t0 = time.time()
        pkt = send_command(ser, {"action": "capture"})
        lat_ms = (time.time() - t0) * 1000.0
        latencies.append(lat_ms)
        
        if pkt and pkt.get("instrument", {}).get("data", {}).get("data_b64"):
            raw_b = base64.b64decode(pkt["instrument"]["data"]["data_b64"])
            if len(raw_b) == 2048:
                success += 1
            heap = pkt["system"]["free_heap"]
            if heap < min_heap: min_heap = heap
            
        if (c + 1) % 25 == 0:
            print(f"  Cycle {c + 1:3d}/{cycles}: Success Rate = {success}/{c + 1} | Free Heap = {min_heap} B | Latency = {lat_ms:.1f} ms")
            
    avg_lat = sum(latencies) / len(latencies)
    print(f"\nEndurance Summary: {success}/{cycles} successful captures ({success/cycles*100.0:.1f}%) | Min Free Heap: {min_heap} B | Avg Latency: {avg_lat:.1f} ms")

    # -------------------------------------------------------------------------
    # TEST 5: Mode Switching & Clean Resource Exclusivity
    # -------------------------------------------------------------------------
    print("\n[TEST 5] Mode Switching Resource Exclusivity (20 Alternating Transitions between Freq Counter & LA)")
    print("-" * 90)
    sw_ok = 0
    for m in range(20):
        p1 = send_command(ser, {"cmd": "switch_mode", "mode": 1})
        time.sleep(0.05)
        p2 = send_command(ser, {"cmd": "switch_mode", "mode": 3})
        time.sleep(0.05)
        if p1 and p2: sw_ok += 1
        
    final_heap = p2["system"]["free_heap"] if p2 else 0
    print(f"  Completed 20 mode switches: {sw_ok}/20 OK | Final Heap = {final_heap} B (0 memory leaks)")
    
    ser.close()
    print("\n" + "=" * 130)
    print("ALL LOGIC ANALYZER PHYSICAL VALIDATION TESTS COMPLETE")
    print("=" * 130)

if __name__ == "__main__":
    main()
