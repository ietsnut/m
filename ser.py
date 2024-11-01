import serial
import struct

ser = serial.Serial('COM7', 9600)
while True:
    # Read 8 bytes (two floats)
    data = ser.read(8)
    val1, val2 = struct.unpack('ff', data)
    print(f"Values: {val1}, {val2}")