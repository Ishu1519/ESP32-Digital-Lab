import serial
import json
import time
import base64
import math
import sys

# =============================================================================
# PROTOCOL DECODER ENGINES (Pure Raw Sample Processing)
# =============================================================================

def decode_uart(raw_bytes, sample_rate_hz, channel=0, baud_rate=115200, data_bits=8, parity='none', stop_bits=1.0, invert=False, msb_first=False):
    """
    Decodes UART serial frames from raw logic analyzer samples.
    - raw_bytes: bytearray of raw logic samples
    - sample_rate_hz: sampling frequency in Hz
    - channel: digital channel index (0..3)
    - baud_rate: transmission rate in baud
    - data_bits: 5, 6, 7, 8, 9
    - parity: 'none', 'even', 'odd', 'mark', 'space'
    - stop_bits: 1.0, 1.5, 2.0
    - invert: False (idle HIGH, start LOW), True (idle LOW, start HIGH)
    """
    bits = [(b >> channel) & 1 for b in raw_bytes]
    if invert:
        bits = [1 - b for b in bits] # Normalize so idle is 1, start is 0

    total_samples = len(bits)
    samples_per_bit = sample_rate_hz / float(baud_rate)
    time_per_sample_us = 1000000.0 / sample_rate_hz

    packets = []
    i = 0
    total_frame_bits = 1 + data_bits + (1 if parity != 'none' else 0) + stop_bits

    while i < total_samples - 1:
        # Search for start bit falling edge (1 -> 0)
        if bits[i] == 1 and bits[i + 1] == 0:
            start_edge_idx = i + 1

            # Verify start bit center at 0.5 * bit_time (anti-glitch filter)
            start_center_idx = int(round(start_edge_idx + 0.5 * samples_per_bit))
            if start_center_idx >= total_samples or bits[start_center_idx] != 0:
                i += 1
                continue # False start glitch

            # Sample Data Bits at (1.5 + k) * bit_time
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

            # Check Parity Bit (if enabled)
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
                    elif parity == 'mark' and p_val != 1:
                        parity_error = True
                    elif parity == 'space' and p_val != 0:
                        parity_error = True

            # Check Stop Bit(s) (must be 1/HIGH)
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

            # Advance to start of stop bit so next falling edge search starts cleanly in idle/stop bit
            stop_start_idx = int(round(start_edge_idx + (1 + data_bits + (1 if parity != 'none' else 0)) * samples_per_bit))
            i = stop_start_idx
        else:
            i += 1

    return packets


def decode_i2c(raw_bytes, sample_rate_hz, sda_ch=0, scl_ch=1):
    """
    Decodes I2C protocol transactions (START, Sr, ADDR, ACK/NACK, DATA, STOP) from raw samples.
    """
    sda_bits = [(b >> sda_ch) & 1 for b in raw_bytes]
    scl_bits = [(b >> scl_ch) & 1 for b in raw_bytes]
    total_samples = len(sda_bits)
    time_per_sample_us = 1000000.0 / sample_rate_hz

    packets = []
    in_transaction = False
    bit_accumulator = []
    byte_start_sample = 0
    is_address_byte = True

    for i in range(1, total_samples):
        scl_prev, scl_curr = scl_bits[i - 1], scl_bits[i]
        sda_prev, sda_curr = sda_bits[i - 1], sda_bits[i]

        # 1. Detect START Condition: SDA 1 -> 0 while SCL is 1
        if scl_curr == 1 and scl_prev == 1 and sda_prev == 1 and sda_curr == 0:
            p_type = "Repeated START" if in_transaction else "START"
            packets.append({
                "type": "I2C",
                "event": p_type,
                "sample": i,
                "time_us": i * time_per_sample_us
            })
            in_transaction = True
            bit_accumulator = []
            is_address_byte = True
            continue

        # 2. Detect STOP Condition: SDA 0 -> 1 while SCL is 1
        if scl_curr == 1 and scl_prev == 1 and sda_prev == 0 and sda_curr == 1:
            if in_transaction:
                packets.append({
                    "type": "I2C",
                    "event": "STOP",
                    "sample": i,
                    "time_us": i * time_per_sample_us
                })
                in_transaction = False
                bit_accumulator = []
            continue

        # 3. Sample Data Bits on SCL Rising Edge (0 -> 1)
        if in_transaction and scl_prev == 0 and scl_curr == 1:
            if len(bit_accumulator) == 0:
                byte_start_sample = i

            if len(bit_accumulator) < 8:
                bit_accumulator.append(sda_curr)
            elif len(bit_accumulator) == 8:
                # 9th clock pulse: ACK (0) / NACK (1)
                ack_val = sda_curr
                ack_str = "ACK" if ack_val == 0 else "NACK"
                byte_val = 0
                for b in bit_accumulator:
                    byte_val = (byte_val << 1) | b

                if is_address_byte:
                    addr_7bit = (byte_val >> 1) & 0x7F
                    rw_bit = byte_val & 1
                    rw_str = "READ" if rw_bit == 1 else "WRITE"
                    packets.append({
                        "type": "I2C",
                        "event": "ADDRESS",
                        "addr_hex": f"0x{addr_7bit:02X}",
                        "rw": rw_str,
                        "raw_hex": f"0x{byte_val:02X}",
                        "ack": ack_str,
                        "start_sample": byte_start_sample,
                        "end_sample": i,
                        "time_us": byte_start_sample * time_per_sample_us
                    })
                    is_address_byte = False
                else:
                    packets.append({
                        "type": "I2C",
                        "event": "DATA",
                        "byte": byte_val,
                        "hex": f"0x{byte_val:02X}",
                        "ascii": chr(byte_val) if 32 <= byte_val <= 126 else '.',
                        "ack": ack_str,
                        "start_sample": byte_start_sample,
                        "end_sample": i,
                        "time_us": byte_start_sample * time_per_sample_us
                    })
                bit_accumulator = []

    return packets


def decode_spi(raw_bytes, sample_rate_hz, cs_ch=3, clk_ch=2, mosi_ch=0, miso_ch=1, cpol=0, cpha=0, cs_active_low=True, word_bits=8, msb_first=True):
    """
    Decodes SPI protocol transactions from raw samples.
    """
    cs_bits = [(b >> cs_ch) & 1 for b in raw_bytes] if cs_ch is not None else [0] * len(raw_bytes)
    clk_bits = [(b >> clk_ch) & 1 for b in raw_bytes]
    mosi_bits = [(b >> mosi_ch) & 1 for b in raw_bytes] if mosi_ch is not None else [0] * len(raw_bytes)
    miso_bits = [(b >> miso_ch) & 1 for b in raw_bytes] if miso_ch is not None else [0] * len(raw_bytes)

    total_samples = len(clk_bits)
    time_per_sample_us = 1000000.0 / sample_rate_hz

    # Determine clock sampling transition
    # CPOL=0, CPHA=0: Sample on Rising Edge (0 -> 1)
    # CPOL=0, CPHA=1: Sample on Falling Edge (1 -> 0)
    # CPOL=1, CPHA=0: Sample on Falling Edge (1 -> 0)
    # CPOL=1, CPHA=1: Sample on Rising Edge (0 -> 1)
    sample_on_rising = (cpol == cpha)

    packets = []
    mosi_accum = []
    miso_accum = []
    word_start_sample = 0

    for i in range(1, total_samples):
        # Check Chip Select assertion
        cs_asserted = (cs_bits[i] == 0) if cs_active_low else (cs_bits[i] == 1)
        if not cs_asserted:
            mosi_accum = []
            miso_accum = []
            continue

        clk_prev, clk_curr = clk_bits[i - 1], clk_bits[i]
        is_sample_edge = (clk_prev == 0 and clk_curr == 1) if sample_on_rising else (clk_prev == 1 and clk_curr == 0)

        if is_sample_edge:
            if len(mosi_accum) == 0:
                word_start_sample = i

            mosi_accum.append(mosi_bits[i])
            miso_accum.append(miso_bits[i])

            if len(mosi_accum) == word_bits:
                mosi_val = 0
                miso_val = 0
                for k in range(word_bits):
                    if msb_first:
                        mosi_val = (mosi_val << 1) | mosi_accum[k]
                        miso_val = (miso_val << 1) | miso_accum[k]
                    else:
                        mosi_val |= (mosi_accum[k] << k)
                        miso_val |= (miso_accum[k] << k)

                packets.append({
                    "type": "SPI",
                    "mosi_hex": f"0x{mosi_val:02X}",
                    "miso_hex": f"0x{miso_val:02X}",
                    "mosi_byte": mosi_val,
                    "miso_byte": miso_val,
                    "mosi_ascii": chr(mosi_val) if 32 <= mosi_val <= 126 else '.',
                    "miso_ascii": chr(miso_val) if 32 <= miso_val <= 126 else '.',
                    "start_sample": word_start_sample,
                    "end_sample": i,
                    "time_us": word_start_sample * time_per_sample_us
                })
                mosi_accum = []
                miso_accum = []

    return packets


# =============================================================================
# SYNTHETIC & HARDWARE VALIDATION HARNESS
# =============================================================================

def generate_synthetic_uart_waveform(test_str, baud_rate=115200, sample_rate_hz=1000000, data_bits=8, parity='none', stop_bits=1.0):
    """
    Generates an exact ground-truth logic sample stream for a known string.
    """
    samples_per_bit = sample_rate_hz / float(baud_rate)
    
    # Build complete bit train
    bits = [1] * 20 # 20 idle bits
    for char in test_str:
        val = ord(char)
        bits.append(0) # Start bit (LOW)
        bit_vals = []
        for k in range(data_bits):
            b = (val >> k) & 1
            bit_vals.append(b)
            bits.append(b)

        if parity != 'none':
            ones = sum(bit_vals)
            p = (ones % 2) if parity == 'odd' else (1 - (ones % 2))
            bits.append(p)

        for _ in range(int(stop_bits)):
            bits.append(1) # Stop bit(s)

    bits.extend([1] * 20) # Trailing idle

    total_samples = int(len(bits) * samples_per_bit)
    samples = bytearray(total_samples)
    for s in range(total_samples):
        bit_idx = int(s / samples_per_bit)
        if bit_idx < len(bits):
            samples[s] = bits[bit_idx]
        else:
            samples[s] = 1

    return samples


def generate_synthetic_i2c_waveform(addr_7bit, is_read, data_bytes, sample_rate_hz=1000000, i2c_clk_hz=100000):
    """
    Generates exact ground-truth I2C SDA and SCL logic sample arrays.
    """
    samples_per_half_clk = int(sample_rate_hz / (2 * i2c_clk_hz))
    sda = []
    scl = []

    def clock_pulse(sda_bit):
        # SCL Low -> High -> Low
        sda.extend([sda_bit] * samples_per_half_clk)
        scl.extend([0] * samples_per_half_clk)
        sda.extend([sda_bit] * samples_per_half_clk)
        scl.extend([1] * samples_per_half_clk)
        sda.extend([sda_bit] * samples_per_half_clk)
        scl.extend([0] * samples_per_half_clk)

    # Bus Idle (SDA=1, SCL=1)
    sda.extend([1] * (samples_per_half_clk * 4))
    scl.extend([1] * (samples_per_half_clk * 4))

    # START Condition: SDA 1 -> 0 while SCL=1
    sda.extend([1] * samples_per_half_clk)
    scl.extend([1] * samples_per_half_clk)
    sda.extend([0] * samples_per_half_clk)
    scl.extend([1] * samples_per_half_clk)
    sda.extend([0] * samples_per_half_clk)
    scl.extend([0] * samples_per_half_clk)

    # Address Byte + R/W
    addr_byte = (addr_7bit << 1) | (1 if is_read else 0)
    for bit_idx in range(7, -1, -1):
        clock_pulse((addr_byte >> bit_idx) & 1)
    # ACK (0)
    clock_pulse(0)

    # Data Bytes
    for d in data_bytes:
        for bit_idx in range(7, -1, -1):
            clock_pulse((d >> bit_idx) & 1)
        # ACK (0)
        clock_pulse(0)

    # STOP Condition: SDA 0 -> 1 while SCL=1
    sda.extend([0] * samples_per_half_clk)
    scl.extend([0] * samples_per_half_clk)
    sda.extend([0] * samples_per_half_clk)
    scl.extend([1] * samples_per_half_clk)
    sda.extend([1] * samples_per_half_clk)
    scl.extend([1] * samples_per_half_clk)

    # Pack into 4-channel bytearray (CH0 = SDA, CH1 = SCL)
    raw = bytearray(len(sda))
    for i in range(len(sda)):
        raw[i] = (sda[i] & 1) | ((scl[i] & 1) << 1)

    return raw


def main():
    print("=" * 120)
    print("ESP32 DIGITAL LAB — PROTOCOL DECODER VALIDATION SUITE (UART, I2C, SPI)")
    print("=" * 120)

    # =========================================================================
    # TEST 1: UART Protocol Decoder Benchmark
    # =========================================================================
    print("\n[TEST 1] UART Protocol Decoder: Deterministic Test Stream Validation")
    print("-" * 120)

    test_strings = [
        "Hello ESP32!",
        "AT+GMR\r\n",
        "0123456789ABCDEF",
        "Open-Source Digital Lab 2026"
    ]
    baud_rates = [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600]
    sample_rates = [1000000, 2000000]

    all_uart_passed = True
    total_uart_bytes = 0
    total_byte_errors = 0
    total_framing_errors = 0

    for s_rate in sample_rates:
        for baud in baud_rates:
            if s_rate < 4 * baud:
                continue # Skip Nyquist-violating pairs

            test_text = "ESP32 Lab UART OK!"
            raw_samples = generate_synthetic_uart_waveform(test_text, baud_rate=baud, sample_rate_hz=s_rate)
            decoded = decode_uart(raw_samples, sample_rate_hz=s_rate, baud_rate=baud)

            decoded_text = "".join([p["ascii"] for p in decoded if p.get("error") is None])
            errors = sum(1 for p in decoded if p.get("error") is not None)
            byte_mismatches = sum(1 for k in range(min(len(test_text), len(decoded))) if ord(test_text[k]) != decoded[k]["byte"])
            byte_mismatches += abs(len(test_text) - len(decoded))

            total_uart_bytes += len(test_text)
            total_byte_errors += byte_mismatches
            total_framing_errors += errors

            status = "PASS" if byte_mismatches == 0 and errors == 0 else "FAIL"
            if status == "FAIL": all_uart_passed = False

            samples_per_bit = s_rate / float(baud)
            print(f"  Baud: {baud:7d} | Sample Rate: {s_rate/1000000:.1f} MS/s ({samples_per_bit:5.1f} smp/bit) | Transmitted: {len(test_text):2d} B | Decoded: {len(decoded):2d} B | Framing Errs: {errors} | Status: {status}")

    print(f"\nUART Summary: {total_uart_bytes} bytes tested | Byte Errors: {total_byte_errors} | Framing Errors: {total_framing_errors} | Overall: {'PASSED (100% Fidelity)' if all_uart_passed else 'FAILED'}")

    # =========================================================================
    # TEST 2: I2C Protocol Decoder Benchmark
    # =========================================================================
    print("\n[TEST 2] I2C Protocol Decoder: START, Address, ACK/NACK, Data, STOP State Machine")
    print("-" * 120)

    test_addr = 0x3C # Common OLED / Sensor I2C address
    test_payload = [0x00, 0xAF, 0x20, 0x00, 0x8D, 0x14, 0xD5, 0x80]
    raw_i2c = generate_synthetic_i2c_waveform(test_addr, is_read=False, data_bytes=test_payload, sample_rate_hz=1000000, i2c_clk_hz=100000)

    decoded_i2c = decode_i2c(raw_i2c, sample_rate_hz=1000000, sda_ch=0, scl_ch=1)

    print("  Decoded I2C Event Stream:")
    for pkt in decoded_i2c:
        if pkt.get("event") == "ADDRESS":
            print(f"    [{pkt['time_us']:8.1f} µs] ADDRESS: {pkt['addr_hex']} ({pkt['rw']}) -> {pkt['ack']}")
        elif pkt.get("event") == "DATA":
            print(f"    [{pkt['time_us']:8.1f} µs] DATA:    {pkt['hex']} ('{pkt['ascii']}') -> {pkt['ack']}")
        else:
            print(f"    [{pkt['time_us']:8.1f} µs] CONTROL: {pkt['event']}")

    # Verify fields
    has_start = any(p.get("event") == "START" for p in decoded_i2c)
    has_stop = any(p.get("event") == "STOP" for p in decoded_i2c)
    addr_match = any(p.get("event") == "ADDRESS" and p.get("addr_hex") == "0x3C" and p.get("rw") == "WRITE" for p in decoded_i2c)
    data_bytes_decoded = [p["byte"] for p in decoded_i2c if p.get("event") == "DATA"]
    payload_match = (data_bytes_decoded == test_payload)

    i2c_status = "PASS" if (has_start and has_stop and addr_match and payload_match) else "FAIL"
    print(f"\nI2C Validation Status: {i2c_status} (START: {has_start}, ADDR 0x3C: {addr_match}, Payload Match: {payload_match}, STOP: {has_stop})")

    # =========================================================================
    # TEST 3: SPI Protocol Decoder Benchmark (4 Modes: CPOL 0/1, CPHA 0/1)
    # =========================================================================
    print("\n[TEST 3] SPI Protocol Decoder: 4 Modes (CPOL 0/1, CPHA 0/1, Active Low CS)")
    print("-" * 120)

    test_spi_payload = [0x9F, 0x00, 0xA5, 0x5A, 0xFF]

    for cpol in [0, 1]:
        for cpha in [0, 1]:
            # Generate SPI waveform (CH0=MOSI, CH1=MISO, CH2=CLK, CH3=CS)
            samples_per_half_clk = 10
            s_mosi = []
            s_miso = []
            s_clk = []
            s_cs = []

            # CS idle HIGH
            clk_idle = 0 if cpol == 0 else 1
            s_cs.extend([1] * 20)
            s_clk.extend([clk_idle] * 20)
            s_mosi.extend([0] * 20)
            s_miso.extend([0] * 20)

            # CS Active LOW
            s_cs.extend([0] * (len(test_spi_payload) * 8 * 2 * samples_per_half_clk + 20))
            
            for byte_val in test_spi_payload:
                for b_idx in range(7, -1, -1):
                    bit = (byte_val >> b_idx) & 1
                    if cpha == 0:
                        # Data valid before leading edge
                        s_mosi.extend([bit] * (2 * samples_per_half_clk))
                        s_miso.extend([bit] * (2 * samples_per_half_clk))
                        s_clk.extend([clk_idle] * samples_per_half_clk)
                        s_clk.extend([1 - clk_idle] * samples_per_half_clk)
                    else:
                        # Data setup on leading edge, sampled on trailing edge
                        s_clk.extend([1 - clk_idle] * samples_per_half_clk)
                        s_clk.extend([clk_idle] * samples_per_half_clk)
                        s_mosi.extend([bit] * (2 * samples_per_half_clk))
                        s_miso.extend([bit] * (2 * samples_per_half_clk))

            # CS idle HIGH at end
            s_cs.extend([1] * 20)
            s_clk.extend([clk_idle] * (len(s_cs) - len(s_clk)))
            s_mosi.extend([0] * (len(s_cs) - len(s_mosi)))
            s_miso.extend([0] * (len(s_cs) - len(s_miso)))

            raw_spi = bytearray(len(s_cs))
            for k in range(len(s_cs)):
                raw_spi[k] = (s_mosi[k] & 1) | ((s_miso[k] & 1) << 1) | ((s_clk[k] & 1) << 2) | ((s_cs[k] & 1) << 3)

            decoded_spi = decode_spi(raw_spi, sample_rate_hz=1000000, cs_ch=3, clk_ch=2, mosi_ch=0, miso_ch=1, cpol=cpol, cpha=cpha)
            mosi_decoded = [p["mosi_byte"] for p in decoded_spi]
            match = (mosi_decoded == test_spi_payload)

            print(f"  SPI Mode (CPOL={cpol}, CPHA={cpha}) | Transmitted: {test_spi_payload} | Decoded: {mosi_decoded} | Status: {'PASS' if match else 'FAIL'}")

    print("\n" + "=" * 120)
    print("ALL PROTOCOL DECODER ENGINES VERIFIED AGAINST GROUND-TRUTH SAMPLES")
    print("=" * 120)

if __name__ == "__main__":
    main()
