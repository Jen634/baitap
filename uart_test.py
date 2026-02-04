#!/usr/bin/env python3
"""
ESP32-C3 LED Controller UART Testing Script
Gửi lệnh UART đến ESP32-C3 LED Controller
"""

import serial
import time
import sys
from enum import Enum

class LEDCommand(Enum):
    PWM_MODE = 0x03
    ON_OFF = 0x01

class LEDState(Enum):
    OFF = 0x00
    ON = 0x01

UART_HEADER1 = 0x55
UART_HEADER2 = 0xAA

def calculate_checksum(header1, header2, cmd, value):
    """Tính checksum (XOR của tất cả byte)"""
    return header1 ^ header2 ^ cmd ^ value

def create_frame(cmd, value):
    """Tạo frame UART"""
    checksum = calculate_checksum(UART_HEADER1, UART_HEADER2, cmd, value)
    frame = bytes([UART_HEADER1, UART_HEADER2, cmd, value, checksum])
    return frame

def send_command(ser, cmd, value, description=""):
    """Gửi lệnh qua UART"""
    frame = create_frame(cmd, value)
    
    # Display command
    hex_str = ' '.join(f'{b:02X}' for b in frame)
    print(f"\n[→] Sending: {hex_str}")
    if description:
        print(f"    {description}")
    
    # Send frame
    ser.write(frame)
    time.sleep(0.1)
    
    # Read response
    try:
        response = ser.read(ser.in_waiting)
        if response:
            print(f"[←] Response: {response.decode('utf-8', errors='ignore').strip()}")
    except:
        pass

def test_pwm_brightness(ser, brightness):
    """Gửi lệnh PWM brightness"""
    if brightness < 0 or brightness > 100:
        print(f"Error: Brightness must be 0-100, got {brightness}")
        return
    
    frame = create_frame(LEDCommand.PWM_MODE.value, brightness)
    hex_str = ' '.join(f'{b:02X}' for b in frame)
    print(f"\n[→] PWM Brightness {brightness}%: {hex_str}")
    ser.write(frame)
    time.sleep(0.1)
    
    response = ser.read(ser.in_waiting)
    if response:
        print(f"[←] {response.decode('utf-8', errors='ignore').strip()}")

def test_on_off(ser, turn_on):
    """Gửi lệnh bật/tắt LED"""
    state_val = LEDState.ON.value if turn_on else LEDState.OFF.value
    state_str = "ON" if turn_on else "OFF"
    
    frame = create_frame(LEDCommand.ON_OFF.value, state_val)
    hex_str = ' '.join(f'{b:02X}' for b in frame)
    print(f"\n[→] Turn LED {state_str}: {hex_str}")
    ser.write(frame)
    time.sleep(0.1)
    
    response = ser.read(ser.in_waiting)
    if response:
        print(f"[←] {response.decode('utf-8', errors='ignore').strip()}")

def print_menu():
    """In menu"""
    print("\n" + "="*50)
    print("ESP32-C3 LED Controller - UART Test Menu")
    print("="*50)
    print("1. Set brightness 0% (OFF)")
    print("2. Set brightness 25%")
    print("3. Set brightness 50%")
    print("4. Set brightness 75%")
    print("5. Set brightness 100%")
    print("6. Turn LED ON (Fade to 100%)")
    print("7. Turn LED OFF (Fade to 0%)")
    print("8. Test invalid command")
    print("9. Test invalid checksum")
    print("0. Exit")
    print("="*50)

def test_invalid_frame(ser):
    """Kiểm tra error handling"""
    print("\n[Test] Sending invalid frame (wrong checksum)...")
    bad_frame = bytes([0x55, 0xAA, 0x03, 50, 0xFF])  # Wrong checksum
    ser.write(bad_frame)
    time.sleep(0.1)
    response = ser.read(ser.in_waiting)
    if response:
        print(f"[←] {response.decode('utf-8', errors='ignore').strip()}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 uart_test.py <COM_PORT> [speed]")
        print("Example: python3 uart_test.py /dev/ttyUSB0 115200")
        sys.exit(1)
    
    port = sys.argv[1]
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
    
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Connected to {port} at {baudrate} bps")
        time.sleep(2)  # Wait for ESP32 to be ready
        
        while True:
            print_menu()
            choice = input("Select option: ").strip()
            
            if choice == '0':
                print("Exiting...")
                break
            elif choice == '1':
                test_pwm_brightness(ser, 0)
            elif choice == '2':
                test_pwm_brightness(ser, 25)
            elif choice == '3':
                test_pwm_brightness(ser, 50)
            elif choice == '4':
                test_pwm_brightness(ser, 75)
            elif choice == '5':
                test_pwm_brightness(ser, 100)
            elif choice == '6':
                test_on_off(ser, True)
            elif choice == '7':
                test_on_off(ser, False)
            elif choice == '8':
                print("\nTesting invalid command (0xFF)...")
                frame = create_frame(0xFF, 50)
                ser.write(frame)
                time.sleep(0.1)
                response = ser.read(ser.in_waiting)
                if response:
                    print(f"[←] {response.decode('utf-8', errors='ignore').strip()}")
            elif choice == '9':
                test_invalid_frame(ser)
            else:
                print("Invalid choice")
        
        ser.close()
    
    except serial.SerialException as e:
        print(f"Error: Cannot open {port} - {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
