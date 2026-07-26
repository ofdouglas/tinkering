import sys

HDLC_FRAME_BYTE  = b'\x7E'
HDLC_ESCAPE_BYTE = b'\x7D'
HDLC_ESCAPE_XOR  = b'\x20'


# Perform HDLC byte stuffing on a single byte
def hdlc_byte_transform(input: int) -> bytes:
    if input in (HDLC_ESCAPE_BYTE[0], HDLC_FRAME_BYTE[0]):
        return HDLC_ESCAPE_BYTE + (input ^ HDLC_ESCAPE_XOR[0]).to_bytes(1, 'big')
    return bytes([input])


# Create an HDLC frame from a message, with byte stuffing
# Does not include a trailing HDLC_FRAME_BYTE.
def hdlc_encode(input: bytes) -> bytes:
    return HDLC_FRAME_BYTE + b''.join(hdlc_byte_transform(b) for b in input)


# Remove the first complete HDLC frame from the input, if any are found
# Returns the encoded frame contents (Framing bytes are removed)
def hdlc_extract_frame(input: bytearray) -> bytes | None:
    first = input.find(HDLC_FRAME_BYTE)
    next  = input.find(HDLC_FRAME_BYTE, first + 1)
    if first == -1:
        return None

    frame_slice = slice(first + 1, next if (next >= 0) else len(input), 1)
    frame = input[frame_slice]
    del input[frame_slice]
    return frame


# Recover normal data from HDLC byte stuffed data (already extracted from the frame).
# This method will stop at the first HDLC_FRAME_BYTE it encounters.
def hdlc_decode(payload: bytes) -> bytes:
    output = bytearray()
    stuff_flag = False

    for b in payload:
        if b == HDLC_FRAME_BYTE[0]: # End of frame
            break
        elif b == HDLC_ESCAPE_BYTE[0]:
            stuff_flag = True
        else:
            val = (b ^ HDLC_ESCAPE_XOR[0]) if stuff_flag else b
            output.append(val)
            stuff_flag = False
    return bytes(output)




if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: hdlc.py <hex>")
        sys.exit(1)

    try:
        raw = bytes.fromhex(sys.argv[1])
    except ValueError as e:
        print(f"Invalid hex: {e}")
        sys.exit(1)

    encoded = hdlc_encode(raw)
    print(f"encoded: {encoded.hex()}")

    frame = hdlc_extract_frame(bytearray(encoded))
    if frame is None:
        print("decode: (no frame found)")
        sys.exit(1)

    decoded = hdlc_decode(frame)
    print(f"decoded: {decoded.hex()}")

    assert decoded == raw, f"decoded: {decoded.hex()} != raw: {raw.hex()}"
