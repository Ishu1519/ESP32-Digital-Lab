import serial
import json
import time

def main():
    ser = serial.Serial('COM7', 115200, timeout=1.5)
    time.sleep(2.0)
    ser.reset_input_buffer()

    ser.write(b'{"cmd":"get_status"}\n')
    time.sleep(0.5)

    while ser.in_waiting:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if line.startswith("{"):
            try:
                data = json.loads(line)
                print("System Health Report:")
                print(f"  Chip: {data.get('system', {}).get('chip', 'ESP32')}")
                print(f"  Free Heap: {data.get('system', {}).get('free_heap', 0) / 1024:.1f} KB")
                print(f"  Active Instrument Mode: {data.get('instrument', {}).get('type', 0)}")
                print(f"  Uptime: {data.get('system', {}).get('uptime_ms', 0) / 1000:.1f} s")
                print(f"  WiFi AP Clients: {data.get('system', {}).get('wifi_clients', 0)}")
                break
            except:
                pass
    ser.close()

if __name__ == "__main__":
    main()
