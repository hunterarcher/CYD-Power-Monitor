import serial
import time
import sys

try:
    # Open serial port
    ser = serial.Serial('COM10', 115200, timeout=1)
    print(f"Connected to {ser.name} at 115200 baud")
    print("Press Ctrl+C to exit\n")
    
    while True:
        if ser.in_waiting > 0:
            data = ser.readline().decode('utf-8', errors='ignore').strip()
            if data:
                print(data)
        time.sleep(0.01)
        
except serial.SerialException as e:
    print(f"Serial error: {e}")
except KeyboardInterrupt:
    print("\nExiting...")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial port closed")