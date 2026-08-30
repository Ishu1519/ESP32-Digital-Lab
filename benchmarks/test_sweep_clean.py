import serial
import json
import time
import base64
import math

def analyze(raw_bytes, sample_rate_hz):
    bits = [b & 1 for b in raw_bytes]
    rising = [i for i in range(1, len(bits)) if bits[i] == 1 and bits[i-1] == 0]
    if len(rising) < 2:
        return 0, 0, len(rising)
    periods = [rising[k] - rising[k-1] for k in range(1, len(rising))]
    mean_period_samples = sum(periods) / len(periods)
    t_us = mean_period_samples * (1000000.0 / sample_rate_hz)
    f_hz = 1000000.0 / t_us if t_us > 0 else 0
    return f_hz, t_us, len(rising)

ser = serial.Serial('COM7', 115200, timeout=1.0)
time.sleep(2.5)
ser.reset_input_buffer()

# Switch to logic analyzer
ser.write(b'{"cmd":"switch_mode","mode":3}\n')
time.sleep(0.3)
ser.reset_input_buffer()

test_cases = [
    (1000, 100000),
    (1000, 1000000),
    (10000, 500000),
    (10000, 1000000),
    (10000, 2000000),
    (100000, 1000000),
    (100000, 2000000),
    (250000, 2000000),
    (500000, 2000000),
]

print(f"{'Target Freq':>12} | {'Sample Rate':>12} | {'Meas Freq':>14} | {'Target Period':>14} | {'Meas Period':>14} | {'Error (%)':>10}")
print("-" * 90)

for f_target, rate in test_cases:
    # 1. Set generator frequency
    ser.write(json.dumps({"action": "set_ref_gen", "freq_hz": f_target, "duty_pct": 50, "enabled": True}).encode() + b"\n")
    time.sleep(0.1)
    
    # 2. Set logic analyzer sample rate & depth
    ser.write(json.dumps({"action": "set_config", "sample_rate_hz": rate, "sample_depth": 2048, "num_channels": 4, "continuous": True}).encode() + b"\n")
    time.sleep(0.1)
    
    # 3. Read fresh telemetry
    ser.reset_input_buffer()
    ser.write(b"READ\n")
    time.sleep(0.05)
    
    line = ser.readline().decode('utf-8', errors='replace').strip()
    if line.startswith("{"):
        data = json.loads(line)
        b64 = data.get("instrument", {}).get("data", {}).get("data_b64", "")
        if b64:
            raw = base64.b64decode(b64)
            meas_f, meas_t, n_edges = analyze(raw, rate)
            target_t = 1000000.0 / f_target
            err = abs(meas_f - f_target) / f_target * 100.0 if meas_f > 0 else 999.0
            print(f"{f_target:12d} | {rate:12d} | {meas_f:14.1f} Hz | {target_t:14.3f} µs | {meas_t:14.3f} µs | {err:9.2f}%")
        else:
            print(f"{f_target:12d} | {rate:12d} | NO B64 DATA")
    else:
        print(f"{f_target:12d} | {rate:12d} | NO JSON")

ser.close()
