# ESP32-C3 LED Controller with Buttons and UART

Dự án điều khiển LED bằng PWM trên ESP32-C3 với 2 nút nhấn (ưu tiên cao) và giao tiếp UART.

## 📋 Yêu Cầu Chức Năng

### 1. **Điều Khiển Nút Nhấn (Ưu tiên cao nhất)**
- **Nút Tăng (+)**: Tăng độ sáng 0-100%
- **Nút Giảm (-)**: Giảm độ sáng 100-0%
- **Hành động**: 
  - Nhấn 1 lần: +/- 1%
  - Nhấn liên tục: Tăng/giảm liên tục

### 2. **Hiệu Ứng Fade Smooth**
- LED luôn fade chậm từ từ (không nhảy cứng)
- Fade từ 0→100 hoặc 100→0 mượt mà
- Có thể dừng fade bất cứ lúc nào khi nhấn nút

### 3. **Giao Tiếp UART**
**Format ghi nhận lệnh:**
```
[Header1] [Header2] [CMD] [Value] [Checksum]
  0x55      0xAA     ...    ...       XOR
```

**Các lệnh hỗ trợ:**
- **CMD 0x03** - Chế độ PWM: Value = 0-100 (độ sáng %)
- **CMD 0x01** - Bật/Tắt: Value = 0x00 (OFF) hoặc 0x01 (ON)

**Checksum**: XOR của tất cả byte trước nó (Header1 ^ Header2 ^ CMD ^ Value)

**Ví dụ lệnh:**
```
Tăng sáng 50%:    55 AA 03 32 8A
Bật LED:          55 AA 01 01 55
Tắt LED:          55 AA 01 00 54
```

### 4. **Kiểm Tra Lỗi Tự Động**
- Phát hiện sai Header Byte 1 hoặc 2
- Kiểm tra CMD hợp lệ
- Xác thực Value nằm trong range
- Kiểm tra Checksum
- Hiển thị lỗi chi tiết trên Serial

## 🏗️ Cấu Trúc Project

```
esp32c3_led_controller/
├── CMakeLists.txt              # Build config chính
├── sdkconfig                   # ESP-IDF config
├── main/
│   ├── CMakeLists.txt          # Build config cho main
│   ├── common.h                # Hằng số, struct, định nghĩa chung
│   ├── main.c                  # Main application, timer callback
│   ├── led_controller.c/.h     # Logic điều khiển LED
│   ├── pwm_manager.c/.h        # Quản lý PWM, fade effect
│   ├── button_handler.c/.h     # Xử lý nút nhấn với debounce
│   └── uart_protocol.c/.h      # Giao tiếp UART, parse frame
```

## 🔌 Kết Nối Phần Cứng

### GPIO Mapping
| Thiết Bị | GPIO | Chú Thích |
|----------|------|----------|
| LED (PWM) | GPIO 3 | PWM output |
| Button UP (+) | GPIO 4 | Pull-up (LOW = pressed) |
| Button DOWN (-) | GPIO 5 | Pull-up (LOW = pressed) |
| UART TX | GPIO 21 | Serial output |
| UART RX | GPIO 20 | Serial input |

### Sơ Đồ Kết Nối
```
ESP32-C3
├── GPIO3  ----[330Ω]---→ LED(+) → GND
├── GPIO4  ----[Button]→ GND (Pull-up internal)
├── GPIO5  ----[Button]→ GND (Pull-up internal)
├── GPIO21 → TX (Serial)
└── GPIO20 ← RX (Serial)
```

## ⚙️ Cấu Hình (common.h)

```c
// GPIO Pins
#define LED_PIN             GPIO_NUM_3
#define BUTTON_UP_PIN       GPIO_NUM_4
#define BUTTON_DOWN_PIN     GPIO_NUM_5

// PWM Settings
#define PWM_FREQUENCY       1000            // 1kHz
#define PWM_RESOLUTION      10              // 10-bit (0-1023)
#define PWM_STEP            10              // 1% per step
#define PWM_FADE_DELAY_MS   30              // 30ms per step

// Button Settings
#define BUTTON_DEBOUNCE_MS  20              // 20ms debounce
#define BUTTON_LONG_PRESS_MS 500            // 500ms = long press
#define BUTTON_REPEAT_INTERVAL_MS 100       // Repeat every 100ms

// UART
#define UART_BAUDRATE       115200
```

## 🚀 Cách Sử Dụng

### 1. Setup Môi Trường
```bash
# Clone/Download project
cd esp32c3_led_controller

# Set ESP-IDF environment
. $IDF_PATH/export.sh

# Hoặc nếu chưa cài:
# Tải ESP-IDF từ: https://github.com/espressif/esp-idf
```

### 2. Build & Flash
```bash
# Clean build
idf.py clean

# Build
idf.py build

# Flash lên ESP32-C3 (chỉnh COM port tương ứng)
idf.py -p /dev/ttyUSB0 flash

# Monitor logs
idf.py -p /dev/ttyUSB0 monitor
```

### 3. Kiểm Tra Hoạt Động
```bash
# Terminal hiển thị:
# - Boot logs
# - Button press events
# - LED brightness updates
# - UART command processing
# - Status every 1 second

# Nhấn Button UP → Brightness tăng
# Nhấn Button DOWN → Brightness giảm
# Gửi UART command → LED thay đổi với fade mượt mà
```

## 📡 Gửi Lệnh UART

### Python Script Ví Dụ
```python
import serial
import time

def calculate_checksum(header1, header2, cmd, value):
    return header1 ^ header2 ^ cmd ^ value

def send_command(ser, cmd, value):
    header1 = 0x55
    header2 = 0xAA
    checksum = calculate_checksum(header1, header2, cmd, value)
    
    frame = bytes([header1, header2, cmd, value, checksum])
    ser.write(frame)
    print(f"Sent: {' '.join(f'{b:02X}' for b in frame)}")

# Mở serial
ser = serial.Serial('/dev/ttyUSB0', 115200)
time.sleep(2)

# Bật LED (fade từ 0 → 100%)
print("Turning ON LED...")
send_command(ser, 0x01, 0x01)
time.sleep(2)

# Đặt sáng 50%
print("Setting brightness to 50%...")
send_command(ser, 0x03, 50)
time.sleep(2)

# Tắt LED (fade từ 100 → 0%)
print("Turning OFF LED...")
send_command(ser, 0x01, 0x00)

ser.close()
```

### Arduino IDE Serial Monitor
```
Lệnh bật LED:              55 AA 01 01 55
Lệnh tắt LED:              55 AA 01 00 54
Lệnh set 75% sáng:         55 AA 03 4B 08
Lệnh set 25% sáng:         55 AA 03 19 7A
```

## 🎯 Luồng Hoạt Động

### Main Loop (10ms Timer)
```
Timer Interrupt (10ms)
├─ 10ms: button_handler_task()
│        └─ Debounce, detect press/long press
│           └─ Call button callback
│              └─ led_increase/decrease_brightness()
│
├─ 30ms (every 3 cycles): pwm_fade_task()
│                         └─ Fade LED smooth
│
├─ 50ms (every 5 cycles): uart_process_data()
│                         └─ Parse frame, validate, execute
│
└─ 1000ms (every 100 cycles): Print LED status
```

### Button Handler
```
Button Pressed
├─ Debounce 20ms
├─ Trigger BUTTON_PRESS_SHORT (callback)
├─ If held > 500ms → Trigger BUTTON_PRESS_LONG
│  └─ Repeat every 100ms while held
└─ On Release → Trigger BUTTON_RELEASE
```

### UART Protocol
```
Receive bytes
└─ Buffer & find headers (0x55 0xAA)
   ├─ Extract frame [Header1][Header2][CMD][Value][Checksum]
   ├─ Validate:
   │  ├─ Header1 = 0x55?
   │  ├─ Header2 = 0xAA?
   │  ├─ CMD = 0x01 or 0x03?
   │  ├─ Value in range?
   │  └─ Checksum OK?
   ├─ If Error: Send error message with details
   └─ If OK: Execute command
      ├─ PWM mode (0x03): led_set_brightness_uart(value)
      └─ ON/OFF mode (0x01): led_set_on_off_uart(value)
```

### PWM Fade
```
led_set_brightness_uart(50)
├─ Set target_brightness = 50
├─ Set is_fading = true
└─ pwm_fade_task() (called every 30ms)
   ├─ current_brightness += PWM_STEP (or -)
   ├─ Update duty cycle
   ├─ Repeat until current == target
   └─ Stop fade
```

## 💡 Tính Năng Chính

✅ **Ưu tiên nút nhấn**: Buttons có callback ngay lập tức, không bị UART block  
✅ **Fade mượt mà**: LED tăng/giảm từ từ với step 1% mỗi 30ms  
✅ **Debounce**: Tránh bounce, debounce 20ms  
✅ **Long press detection**: Phát hiện nhấn liên tục sau 500ms  
✅ **UART protocol**: Parse frame, validate, error checking chi tiết  
✅ **Modular code**: Dễ mở rộng cho webserver sau này  
✅ **Clean architecture**: Tách biệt logic, dễ maintain  

## 🔧 Mở Rộng (Chuẩn Bị Webserver)

Cấu trúc hiện tại dễ mở rộng:

1. **Thêm HTTP Server**: Tạo `http_server.c/.h` gọi các function từ `led_controller`
2. **State Sync**: `g_led_state` là global, có thể truy cập từ bất kỳ handler nào
3. **Event System**: Button/UART events có thể broadcast qua WebSocket
4. **REST API**: Endpoints để GET/SET LED brightness

### Ví dụ Integration Webserver
```c
// http_server.c
void http_handler_get_led_state(httpd_req_t *req) {
    led_state_t state = led_get_state();
    // JSON response với brightness, is_on, mode
}

void http_handler_set_brightness(httpd_req_t *req) {
    // Parse JSON, gọi led_set_brightness_uart()
}
```

## 📊 Serial Output Ví Dụ

```
========================================
ESP32-C3 LED Controller with Buttons
========================================

I (245) MAIN: Initializing components...
I (255) LED_CTRL: Initializing LED Controller...
I (255) PWM_MGR: Initializing PWM Manager...
I (265) PWM_MGR: PWM initialized on GPIO 3, Freq: 1000 Hz
I (275) BTN_HDL: Initializing button handler...
I (285) BTN_HDL: Button handler initialized
I (285) UART: Initializing UART Protocol...
I (295) UART: UART initialized - Baudrate: 115200, TX: GPIO21, RX: GPIO20

========== LED STATUS ==========
State: OFF
Brightness: 0%
Mode: PWM
Fading: NO
================================

========== SYSTEM READY ==========
Commands:
  PWM Mode:  [0x55][0xAA][0x03][brightness 0-100][checksum]
  ON/OFF:    [0x55][0xAA][0x01][0x00/0x01][checksum]
==================================

I (305) BTN_HDL: Button GPIO 4 pressed
I (305) MAIN: Button UP - Single press detected
I (305) LED_CTRL: Increase brightness: 1%

[UART CMD] PWM Mode - Brightness: 50%
[ACK] CMD=0x03, Value=0x32, Checksum=0x8A
```

## 🐛 Troubleshooting

| Vấn Đề | Nguyên Nhân | Giải Pháp |
|--------|-----------|----------|
| LED không sáng | GPIO sai, hardware lỗi | Kiểm tra cắm đúng GPIO3 |
| Button không phản ứng | Pin GPIO sai | Kiểm tra GPIO4/5 |
| UART không nhận | Baudrate sai | Đảm bảo 115200 bps |
| LED nhảy cứng | PWM không fade | Kiểm tra `PWM_FADE_DELAY_MS` |
| Button lag | Debounce quá lâu | Giảm `BUTTON_DEBOUNCE_MS` |

## 📝 License

Open source - Tự do sử dụng và phát triển

---

**Phiên bản**: 1.0  
**Target**: ESP32-C3  
**IDF Version**: v4.4 trở lên  
