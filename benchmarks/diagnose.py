import serial
import json
import time

def main():
    port = 'COM7'
    baud = 115200
    print(f"Connecting to ESP32 on {port}...")
    ser = serial.Serial(port, baud, timeout=1.0)
    time.sleep(1.0)
    while ser.in_waiting:
        ser.read(ser.in_waiting)

    test_freqs = [
        1, 2, 5, 10, 20,
        50, 100, 500, 1000, 5000,
        10000, 50000, 100000, 500000,
        1000000, 2000000, 5000000
    ]

    print("\n" + "="*105)
    print(f"{'Requested':>10} | {'Mode':>6} | {'Res':>4} | {'div_param':>9} | {'Calculated Gen':>15} | {'PCNT Sample 1':>14} | {'PCNT Sample 2':>14} | {'PCNT Sample 3':>14}")
    print("="*105)

    for freq in test_freqs:
        cmd = {"action": "set_ref_gen", "freq_hz": freq, "duty_pct": 50, "enabled": True}
        ser.write((json.dumps(cmd) + "\n").encode())
        
        # Wait 1.8 seconds for gate timer (500ms * 3.5 cycles) to settle
        time.sleep(1.8)
        while ser.in_waiting:
            ser.read(ser.in_waiting)

        samples = []
        gen_info = {}
        for _ in range(3):
            ser.write(b"READ\n")
            time.sleep(0.55) # Wait for a new gate period
            start = time.time()
            while time.time() - start < 0.5:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if line.startswith("{") and "instrument" in line:
                    try:
                        data = json.loads(line)
                        inst = data.get("instrument", {}).get("data", {})
                        samples.append(inst.get("freq_hz", 0.0))
                        gen_info = inst.get("ref_gen", {})
                        break
                    except:
                        pass

        mode = "Timer" if freq < 50 else "LEDC"
        res = gen_info.get("res_bits", 0)
        div_p = gen_info.get("div_param", 0)
        calc_f = gen_info.get("calc_freq_hz", 0.0)
        
        s1 = samples[0] if len(samples) > 0 else 0.0
        s2 = samples[1] if len(samples) > 1 else 0.0
        s3 = samples[2] if len(samples) > 2 else 0.0

        print(f"{freq:10d} | {mode:>6} | {res:4d} | {div_p:9d} | {calc_f:15.2f} | {s1:14.2f} | {s2:14.2f} | {s3:14.2f}")

    ser.close()

if __name__ == "__main__":
    main()
