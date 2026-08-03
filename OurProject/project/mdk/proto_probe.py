import serial
import time

def probe(port):
    try:
        ser = serial.Serial(port, 115200, timeout=0.3)
    except Exception as e:
        print(f"{port}: open fail - {e}")
        return
    print(f"{port}: opened, sending PROTO")
    ser.reset_input_buffer()
    ser.write(b"PROTO\n")
    deadline = time.time() + 5.0
    out = b""
    while time.time() < deadline:
        data = ser.read(1024)
        if data:
            out += data
    try:
        ser.write(b"STOP\n")
    except Exception:
        pass
    ser.close()
    print(f"{port}: received {len(out)} bytes")
    print(out.decode("utf-8", "replace")[:2500])
    print("=" * 50)

for p in ("COM4", "COM9"):
    probe(p)
