import serial
import json
import time
import base64
import sys

# =============================================================================
# BROWSER-EQUIVALENT PROTOCOL DECODERS
# =============================================================================

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


def decode_i2c(raw_bytes, sample_rate_hz, sda_ch=0, scl_ch=1):
    sda = [(b >> sda_ch) & 1 for b in raw_bytes]
    scl = [(b >> scl_ch) & 1 for b in raw_bytes]
    total_samples = len(sda)
    time_per_sample_us = 1000000.0 / sample_rate_hz

    packets = []
    in_transaction = False
    bit_count = 0
    current_byte = 0
    start_sample = 0
    state = "IDLE"

    for i in range(1, total_samples):
        # 1. Check for START condition: SDA falling while SCL is HIGH
        if scl[i] == 1 and sda[i-1] == 1 and sda[i] == 0:
            evt = "REPEATED_START" if in_transaction else "START"
            packets.append({
                "type": "I2C", "event": evt, "sample": i, "time_us": i * time_per_sample_us
            })
            in_transaction = True
            state = "ADDRESS"
            bit_count = 0
            current_byte = 0
            start_sample = i
            continue

        # 2. Check for STOP condition: SDA rising while SCL is HIGH
        if scl[i] == 1 and sda[i-1] == 0 and sda[i] == 1:
            if in_transaction:
                packets.append({
                    "type": "I2C", "event": "STOP", "sample": i, "time_us": i * time_per_sample_us
                })
                in_transaction = False
                state = "IDLE"
            continue

        # 3. Sample data bits on SCL rising edge (0 -> 1)
        if in_transaction and scl[i-1] == 0 and scl[i] == 1:
            bit_val = sda[i]
            if bit_count < 8:
                current_byte = (current_byte << 1) | bit_val
                bit_count += 1
            elif bit_count == 8:
                ack_val = "ACK" if bit_val == 0 else "NACK"
                end_time_us = i * time_per_sample_us
                start_time_us = start_sample * time_per_sample_us

                if state == "ADDRESS":
                    addr = (current_byte >> 1) & 0x7F
                    rw = "READ" if (current_byte & 1) else "WRITE"
                    packets.append({
                        "type": "I2C", "event": "ADDRESS", "addr_7bit": addr, "addr_hex": f"0x{addr:02X}",
                        "rw": rw, "ack": ack_val, "start_sample": start_sample, "end_sample": i,
                        "time_us": start_time_us, "duration_us": end_time_us - start_time_us
                    })
                    state = "DATA"
                else:
                    packets.append({
                        "type": "I2C", "event": "DATA", "byte": current_byte, "hex": f"0x{current_byte:02X}",
                        "ascii": chr(current_byte) if 32 <= current_byte <= 126 else '.',
                        "ack": ack_val, "start_sample": start_sample, "end_sample": i,
                        "time_us": start_time_us, "duration_us": end_time_us - start_time_us
                    })

                bit_count = 0
                current_byte = 0
                start_sample = i

    return packets


def decode_spi(raw_bytes, sample_rate_hz, cs_ch=3, clk_ch=2, mosi_ch=0, miso_ch=1, cpol=0, cpha=0, word_len=8, msb_first=True, cs_active_low=True):
    total_samples = len(raw_bytes)
    time_per_sample_us = 1000000.0 / sample_rate_hz

    cs = [(b >> cs_ch) & 1 for b in raw_bytes]
    clk = [(b >> clk_ch) & 1 for b in raw_bytes]
    mosi = [(b >> mosi_ch) & 1 for b in raw_bytes]
    miso = [(b >> miso_ch) & 1 for b in raw_bytes]

    packets = []
    bit_count = 0
    mosi_word = 0
    miso_word = 0
    start_sample = 0

    for i in range(1, total_samples):
        is_cs_active = (cs[i] == 0) if cs_active_low else (cs[i] == 1)

        if not is_cs_active:
            bit_count = 0
            mosi_word = 0
            miso_word = 0
            continue

        clk_prev = clk[i-1]
        clk_curr = clk[i]
        sample_edge = False

        if cpha == 0:
            sample_edge = (clk_prev == 0 and clk_curr == 1) if cpol == 0 else (clk_prev == 1 and clk_curr == 0)
        else:
            sample_edge = (clk_prev == 1 and clk_curr == 0) if cpol == 0 else (clk_prev == 0 and clk_curr == 1)

        if sample_edge:
            if bit_count == 0:
                start_sample = i

            bit_mosi = mosi[i]
            bit_miso = miso[i]

            if msb_first:
                mosi_word = (mosi_word << 1) | bit_mosi
                miso_word = (miso_word << 1) | bit_miso
            else:
                mosi_word |= (bit_mosi << bit_count)
                miso_word |= (bit_miso << bit_count)

            bit_count += 1

            if bit_count == word_len:
                start_time_us = start_sample * time_per_sample_us
                end_time_us = i * time_per_sample_us

                packets.append({
                    "type": "SPI", "mosi_byte": mosi_word, "miso_byte": miso_word,
                    "mosi_hex": f"0x{mosi_word:02X}", "miso_hex": f"0x{miso_word:02X}",
                    "mosi_ascii": chr(mosi_word) if 32 <= mosi_word <= 126 else '.',
                    "miso_ascii": chr(miso_word) if 32 <= miso_word <= 126 else '.',
                    "start_sample": start_sample, "end_sample": i,
                    "start_time_us": start_time_us, "end_time_us": end_time_us,
                    "duration_us": end_time_us - start_time_us
                })
                bit_count = 0
                mosi_word = 0
                miso_word = 0

    return packets


# =============================================================================
# SERIAL COMMUNICATION HELPERS
# =============================================================================

def send_command(ser, cmd):
    ser.reset_input_buffer()
    ser.write(json.dumps(cmd).encode() + b"\n")
    start = time.time()
    while time.time() - start < 2.0:
        line = ser.readline().decode('utf-8', errors='replace').strip()
        if line.startswith("{") and '"instrument"' in line:
            try:
                return json.loads(line)
            except:
                pass
    return None


def main():
    print("=" * 110)
    print("ESP32 DIGITAL LAB — PROTOCOL DECODING PHYSICAL & SYNTHETIC BENCHMARK SUITE")
    print("=" * 110)

    ser = serial.Serial('COM7', 115200, timeout=1.5)
    time.sleep(2.5)
    ser.reset_input_buffer()

    # Switch to Mode 3 (Logic Analyzer)
    send_command(ser, {"cmd": "switch_mode", "mode": 3})
    time.sleep(0.3)

    # -------------------------------------------------------------------------
    # TEST 1: Physical Hardware UART Loopback Test (GPIO 19 TX -> GPIO 18 CH0)
    # -------------------------------------------------------------------------
    print("\n[PART 1] Physical Hardware UART Loopback Benchmark (GPIO 19 -> GPIO 18)")
    print("-" * 110)
    print(f"{'Baud':>8} | {'Sample Rate':>12} | {'Sent String':>18} | {'Decoded String':>18} | {'Bytes':>6} | {'Errors':>7} | {'Status':>8}")
    print("-" * 110)

    test_baud_configs = [
        (9600, 100000),    # 100 kS/s -> 40.96 ms buffer (10.4 smp/bit)
        (19200, 250000),   # 250 kS/s -> 16.38 ms buffer (13.0 smp/bit)
        (38400, 500000),   # 500 kS/s -> 8.19 ms buffer (13.0 smp/bit)
        (57600, 500000),   # 500 kS/s -> 8.19 ms buffer (8.7 smp/bit)
        (115200, 1000000), # 1 MS/s -> 4.09 ms buffer (8.7 smp/bit)
        (115200, 2000000), # 2 MS/s -> 2.05 ms buffer (17.4 smp/bit)
    ]
    uart_all_pass = True

    for baud, sample_rate in test_baud_configs:
        test_msg = f"TEST{baud}"
        prefix = " " if baud >= 230400 else "   "
        send_payload = prefix + test_msg + "\r\n"

        # Configure falling edge trigger on CH0
        send_command(ser, {
            "action": "set_config",
            "sample_rate_hz": sample_rate,
            "sample_depth": 4096,
            "num_channels": 4,
            "trigger_channel": 0,
            "trigger_mode": 2, # LA_TRIGGER_FALLING
            "continuous": False
        })
        time.sleep(0.05)

        pkt = send_command(ser, {"action": "send_uart_test", "baud_rate": baud, "text": send_payload})
        if pkt and pkt.get("instrument", {}).get("data", {}).get("data_b64"):
            raw_bytes = base64.b64decode(pkt["instrument"]["data"]["data_b64"])
            decoded = decode_uart(raw_bytes, sample_rate_hz=sample_rate, baud_rate=baud)

            decoded_str = "".join([p["ascii"] for p in decoded if not p["error"]])
            errors = sum(1 for p in decoded if p["error"])
            status = "PASS" if test_msg in decoded_str and errors == 0 else "FAIL"
            if status == "FAIL": uart_all_pass = False

            print(f"{baud:8d} | {sample_rate/1000000:6.2f} MS/s   | {test_msg:>18} | {decoded_str:>18} | {len(decoded):6d} | {errors:7d} | {status:>8}")
        else:
            print(f"{baud:8d} | {sample_rate/1000000:6.2f} MS/s   | {test_msg:>18} | {'NO TELEMETRY':>18} |      0 |       0 |     FAIL")
            uart_all_pass = False

    ser.close()

    # -------------------------------------------------------------------------
    # TEST 2 & 3: I2C and SPI Synthetic Stream Verification
    # -------------------------------------------------------------------------
    print("\n[PART 2] I2C & SPI Protocol Decoder Engine Verification")
    print("-" * 110)

    # I2C Verification
    from validate_decoders import generate_synthetic_i2c_waveform
    raw_i2c = generate_synthetic_i2c_waveform(0x3C, False, [0x00, 0xAF, 0x20, 0x00, 0x8D, 0x14, 0xD5, 0x80], sample_rate_hz=1000000, i2c_clk_hz=100000)
    dec_i2c = decode_i2c(raw_i2c, 1000000, 0, 1)
    i2c_bytes = [p["byte"] for p in dec_i2c if p.get("event") == "DATA"]
    i2c_pass = (i2c_bytes == [0x00, 0xAF, 0x20, 0x00, 0x8D, 0x14, 0xD5, 0x80])
    print(f"  I2C (100 kHz Standard Mode)  | Addr: 0x3C | Payload: 8 Bytes | Decoded: {len(i2c_bytes)} Bytes | Status: {'PASS' if i2c_pass else 'FAIL'}")

    # SPI 4-Mode Verification
    spi_pass = True
    for cpol in [0, 1]:
        for cpha in [0, 1]:
            # Generate synthetic SPI
            test_payload = [0x9F, 0x00, 0xA5, 0x5A, 0xFF]
            samples_per_half = 10
            s_mosi, s_miso, s_clk, s_cs = [], [], [], []
            idle = 0 if cpol == 0 else 1
            s_cs.extend([1]*20); s_clk.extend([idle]*20); s_mosi.extend([0]*20); s_miso.extend([0]*20)
            s_cs.extend([0]*(len(test_payload)*8*2*samples_per_half + 20))
            for b in test_payload:
                for bit_idx in range(7, -1, -1):
                    val = (b >> bit_idx) & 1
                    if cpha == 0:
                        s_mosi.extend([val]*(2*samples_per_half)); s_miso.extend([val]*(2*samples_per_half))
                        s_clk.extend([idle]*samples_per_half); s_clk.extend([1-idle]*samples_per_half)
                    else:
                        s_clk.extend([1-idle]*samples_per_half); s_clk.extend([idle]*samples_per_half)
                        s_mosi.extend([val]*(2*samples_per_half)); s_miso.extend([val]*(2*samples_per_half))
            s_cs.extend([1]*20)
            s_clk.extend([idle]*(len(s_cs)-len(s_clk)))
            s_mosi.extend([0]*(len(s_cs)-len(s_mosi)))
            s_miso.extend([0]*(len(s_cs)-len(s_miso)))

            raw_spi = bytearray(len(s_cs))
            for k in range(len(s_cs)):
                raw_spi[k] = (s_mosi[k]&1) | ((s_miso[k]&1)<<1) | ((s_clk[k]&1)<<2) | ((s_cs[k]&1)<<3)

            dec_spi = decode_spi(raw_spi, 1000000, 3, 2, 0, 1, cpol, cpha)
            mosi_res = [p["mosi_byte"] for p in dec_spi]
            match = (mosi_res == test_payload)
            if not match: spi_pass = False
            print(f"  SPI (CPOL={cpol}, CPHA={cpha})           | Mode {cpol*2+cpha}  | Payload: {len(test_payload)} Bytes | Decoded: {len(mosi_res)} Bytes | Status: {'PASS' if match else 'FAIL'}")

    print("\n" + "=" * 110)
    print(f"OVERALL PROTOCOL SUITE STATUS: {'ALL TESTS PASSED (100% METROLOGY FIDELITY)' if (uart_all_pass and i2c_pass and spi_pass) else 'FAILURES DETECTED'}")
    print("=" * 110)

if __name__ == "__main__":
    main()
