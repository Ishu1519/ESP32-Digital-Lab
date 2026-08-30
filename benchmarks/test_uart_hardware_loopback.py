import serial
import json
import time
import base64
import sys

def decode_uart(raw_bytes, sample_rate_hz, channel=0, baud_rate=115200, data_bits=8, parity='none', stop_bits=1.0, invert=False, msb_first=False):
    bits = [(b >> channel) & 1 for b in raw_bytes]
    if invert:
        bits = [1 - b for b in bits]

    total_samples = len(bits)
    samples_per_bit = sample_rate_hz / float(baud_rate)
    time_per_sample_us = 1000000.0 / sample_rate_hz

    packets = []
    i = 0
    total_frame_bits = 1 + data_bits + (1 if parity != 'none' else 0) + stop_bits

    while i < total_samples - 1:
        if bits[i] == 1 and bits[i + 1] == 0:
            start_edge_idx = i + 1

            start_center_idx = int(round(start_edge_idx + 0.5 * samples_per_bit))
            if start_center_idx >= total_samples or bits[start_center_idx] != 0:
                i += 1
                continue

            data_val = 0
            bit_samples = []
            for k in range(data_bits):
                sample_pos = int(round(start_edge_idx + (1.5 + k) * samples_per_bit))
                if sample_pos >= total_samples:
                    break
                bit_val = bits[sample_pos]
                bit_samples.append(bit_val)
                if msb_first:
                    data_val = (data_val << 1) | bit_val
                else:
                    data_val |= (bit_val << k)

            if len(bit_samples) < data_bits:
                break

            parity_error = False
            if parity != 'none':
                p_sample_pos = int(round(start_edge_idx + (1.5 + data_bits) * samples_per_bit))
                if p_sample_pos < total_samples:
                    p_val = bits[p_sample_pos]
                    ones_count = sum(bit_samples)
                    if parity == 'even' and (ones_count + p_val) % 2 != 0:
                        parity_error = True
                    elif parity == 'odd' and (ones_count + p_val) % 2 != 1:
                        parity_error = True

            framing_error = False
            stop_pos_offset = 1.5 + data_bits + (1 if parity != 'none' else 0)
            stop_sample_pos = int(round(start_edge_idx + stop_pos_offset * samples_per_bit))
            if stop_sample_pos < total_samples:
                if bits[stop_sample_pos] != 1:
                    framing_error = True

            frame_end_idx = int(round(start_edge_idx + total_frame_bits * samples_per_bit))
            start_time_us = start_edge_idx * time_per_sample_us
            end_time_us = frame_end_idx * time_per_sample_us

            ascii_char = chr(data_val) if 32 <= data_val <= 126 else '.'
            err_type = "FRAMING_ERROR" if framing_error else ("PARITY_ERROR" if parity_error else None)

            packets.append({
                "type": "UART",
                "byte": data_val,
                "hex": f"0x{data_val:02X}",
                "ascii": ascii_char,
                "start_sample": start_edge_idx,
                "end_sample": frame_end_idx,
                "start_time_us": start_time_us,
                "end_time_us": end_time_us,
                "duration_us": end_time_us - start_time_us,
                "error": err_type
            })

            stop_start_idx = int(round(start_edge_idx + (1 + data_bits + (1 if parity != 'none' else 0)) * samples_per_bit))
            i = stop_start_idx
        else:
            i += 1

    return packets

def send_command(ser, cmd):
    ser.reset_input_buffer()
    ser.write(json.dumps(cmd).encode() + b"\n")
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

def main():
    print("=" * 100)
    print("ESP32 DIGITAL LAB — HARDWARE UART TRANSMITTER LOOPBACK VALIDATION")
    print("=" * 100)

    ser = serial.Serial('COM7', 115200, timeout=1.2)
    time.sleep(2.5)
    ser.reset_input_buffer()

    # Switch to Logic Analyzer (Mode 3)
    send_command(ser, {"cmd": "switch_mode", "mode": 3})
    time.sleep(0.3)

    # Set sample rate to 2 MS/s, depth 4096, with falling edge trigger on CH0 (trigger_mode=2)
    sample_rate = 2000000
    send_command(ser, {
        "action": "set_config",
        "sample_rate_hz": sample_rate,
        "sample_depth": 4096,
        "num_channels": 4,
        "trigger_channel": 0,
        "trigger_mode": 2, # LA_TRIGGER_FALLING
        "continuous": False
    })
    time.sleep(0.1)

    # Trigger UART test transmission on GPIO 19 -> GPIO 18
    test_message = "HELLO-ESP32"
    send_payload = "\r\n" + test_message + "\r\n"
    baud = 115200
    print(f"Transmitting Hardware UART on GPIO 19: '{test_message}' @ {baud} baud (Trigger: Falling Edge on CH0)...")
    
    pkt = send_command(ser, {"action": "send_uart_test", "baud_rate": baud, "text": send_payload})

    if pkt and pkt.get("instrument", {}).get("data", {}).get("data_b64"):
        raw_bytes = base64.b64decode(pkt["instrument"]["data"]["data_b64"])
        print(f"Captured {len(raw_bytes)} raw logic samples from CH0 (GPIO 18). Running UART decoder...")
        decoded = decode_uart(raw_bytes, sample_rate_hz=sample_rate, baud_rate=baud)

        print("\nDecoded Packets:")
        print(f"{'#':>3} | {'Timestamp (µs)':>15} | {'Hex':>6} | {'ASCII':>6} | {'Bit Duration (µs)':>18} | {'Status':>10}")
        print("-" * 75)
        for idx, p in enumerate(decoded):
            status = p["error"] if p["error"] else "OK"
            print(f"{idx+1:3d} | {p['start_time_us']:15.2f} | {p['hex']:>6} | {p['ascii']:>6} | {p['duration_us']:18.2f} | {status:>10}")

        decoded_str = "".join([p["ascii"] for p in decoded if not p["error"]])
        print(f"\nExpected String: '{test_message}'")
        print(f"Decoded String:  '{decoded_str}'")

        if test_message in decoded_str:
            print("\n>>> PHYSICAL HARDWARE UART LOOPBACK VALIDATION PASSED! <<<")
        else:
            print(f"\n>>> Partial / Raw decode: '{decoded_str}' <<<")
    else:
        print("ERROR: No logic telemetry received from ESP32")
    
    ser.close()

if __name__ == "__main__":
    main()
