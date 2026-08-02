import serial
import time

PORT = "COM11"
BAUD = 115200
DURATION = 5.0

frames = []
partial = b""

ser = serial.Serial(PORT, BAUD, timeout=0.2)
print(f"Opened {PORT} @ {BAUD}")

# Flush any stale buffered data
time.sleep(0.2)
ser.reset_input_buffer()

# Send command to start task 2
ser.write(b"T2\n")
print("Sent: T2\\n")

deadline = time.time() + DURATION
while time.time() < deadline:
    data = ser.read(4096)
    if not data:
        continue
    partial += data
    while b"\n" in partial:
        line, partial = partial.split(b"\n", 1)
        line = line.strip(b"\r")
        if line.startswith(b"T="):
            frames.append(line.decode("utf-8", "replace"))

ser.close()
print(f"Closed {PORT}")

print(f"\nTotal frames received: {len(frames)}")
print("First 5 frames:")
for f in frames[:5]:
    print(f"  {f}")

expected = int(DURATION / 0.1)
print(f"Expected ~{expected} frames (100ms/frame)")
