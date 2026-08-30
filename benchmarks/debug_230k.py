import serial
import json
import time
import base64
from validate_decoders import decode_uart

ser = serial.Serial('COM7', 115200, timeout=1.5)
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
send_cmd({"action": "set_config", "sample_rate_hz": 2000000, "sample_depth": 4096, "num_channels": 4, "trigger_channel": 0, "trigger_mode": 2, "continuous": False})
time.sleep(0.1)

pkt = send_cmd({"action": "send_uart_test", "baud_rate": 230400, "text": "   TEST230400\r\n"})
if pkt and pkt.get("instrument", {}).get("data", {}).get("data_b64"):
    raw = base64.b64decode(pkt["instrument"]["data"]["data_b64"])
    print(f"Captured {len(raw)} samples")
    bits = [b & 1 for b in raw]
    trans = [i for i in range(1, len(bits)) if bits[i] != bits[i-1]]
    print("Transitions count:", len(trans))
    print("Transitions:", trans[:40])
    
    # Check widths
    widths = [trans[i] - trans[i-1] for i in range(1, min(len(trans), 30))]
    print("Widths in samples:", widths)

    dec = decode_uart(raw, 2000000, baud_rate=230400)
    for p in dec:
        print(p)

ser.close()
