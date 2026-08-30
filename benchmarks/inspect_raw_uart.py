import serial
import json
import time
import base64

ser = serial.Serial('COM7', 115200, timeout=1.2)
time.sleep(2.5)
ser.reset_input_buffer()

def send_cmd(cmd):
    ser.reset_input_buffer()
    ser.write(json.dumps(cmd).encode() + b"\n")
    start = time.time()
    while time.time() - start < 1.5:
        line = ser.readline().decode('utf-8', errors='replace').strip()
        if line.startswith("{") and '"instrument"' in line:
            return json.loads(line)
    return None

send_cmd({"cmd": "switch_mode", "mode": 3})
time.sleep(0.2)
send_cmd({"action": "set_config", "sample_rate_hz": 1000000, "sample_depth": 2048, "num_channels": 4, "continuous": True})
time.sleep(0.1)

pkt = send_cmd({"action": "send_uart_test", "baud_rate": 115200, "text": "U"})
if pkt and pkt.get("instrument", {}).get("data", {}).get("data_b64"):
    raw = base64.b64decode(pkt["instrument"]["data"]["data_b64"])
    bits = [b & 1 for b in raw]
    print(f"Total samples: {len(bits)}")
    print(f"High samples: {sum(bits)}, Low samples: {len(bits) - sum(bits)}")
    
    # Print bit transitions
    trans = [i for i in range(1, len(bits)) if bits[i] != bits[i-1]]
    print(f"Transitions ({len(trans)}): {trans[:20]}")
    print(f"First 100 bits: {''.join(str(b) for b in bits[:100])}")
else:
    print("No packet")

ser.close()
