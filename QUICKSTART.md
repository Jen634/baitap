# 🚀 QUICK START GUIDE

## 1️⃣ Chuẩn Bị

### Yêu cầu phần mềm:
- ESP-IDF v4.4 hoặc mới hơn
- Python 3.6+
- pyserial: `pip install pyserial`

### Yêu cầu phần cứng:
- ESP32-C3 board
- LED + 330Ω resistor
- 2 push buttons (normally open)
- USB cable để lập trình và serial monitoring

## 2️⃣ Kết Nối Phần Cứng

```
ESP32-C3 Pin Layout:
     GPIO20(RX)  GPIO21(TX)
           ↓         ↓
┌─────────────────────────────┐
│    ESP32-C3 DEVKIT          │
│                             │
│  GPIO3(PWM) → LED → GND     │
│  GPIO4 → Button1 → GND      │
│  GPIO5 → Button2 → GND      │
└─────────────────────────────┘
```

**Chi tiết:**
- **LED**: GPIO3 → 330Ω → LED(+) → GND
- **Button UP**: GPIO4 → Button → GND (pullup enable)
- **Button DOWN**: GPIO5 → Button → GND (pullup enable)
- **UART**: GPIO21(TX), GPIO20(RX) cho USB Serial

## 3️⃣ Build & Flash

```bash
# 1. Clone project
git clone <repo>
cd esp32c3_led_controller

# 2. Setup ESP-IDF
. $IDF_PATH/export.sh

# 3. Build project
idf.py build

# 4. Detect port
ls /dev/ttyUSB*          # Linux
# hoặc
Get-PSDrive | grep COM   # Windows

# 5. Flash (đổi COM port tương ứng)
idf.py -p /dev/ttyUSB0 flash

# 6. Monitor logs (Ctrl+C để thoát)
idf.py -p /dev/ttyUSB0 monitor

# Hoặc combine step 5+6:
idf.py -p /dev/ttyUSB0 flash monitor
```

## 4️⃣ Kiểm Tra Hoạt Động

### Qua Physical Buttons:
1. **Nhấn Button UP**: LED sáng thêm 1%
2. **Nhấn liên tục Button UP**: LED tăng sáng liên tục
3. **Nhấn Button DOWN**: LED tối thêm 1%
4. **Nhấn liên tục Button DOWN**: LED giảm sáng liên tục

### Qua Serial Monitor (115200 bps):
```
I (1000) MAIN: LED Status - On: YES, Brightness: 50%, Fading: YES
I (2000) BTN_HDL: Button GPIO 4 pressed
```

### Qua UART Commands (Python):
```bash
python3 uart_test.py /dev/ttyUSB0 115200
```

Chọn menu:
- 1-5: Set brightness 0-100%
- 6: Bật LED
- 7: Tắt LED
- 8-9: Test error handling

## 5️⃣ Gửi Lệnh UART Trực Tiếp

### Ví dụ lệnh HEX (Brightness 50%):
```
55 AA 03 32 8A
```
- `55 AA` = Header
- `03` = PWM mode
- `32` = 50 (decimal)
- `8A` = Checksum (XOR)

### Dùng Arduino IDE Serial Monitor:
1. Mở Serial Monitor (115200 bps)
2. Chọn "Hex" input mode
3. Gõ: `55AA0332 8A` (không space)
4. Gửi → LED sáng 50%

### Dùng Python:
```python
import serial
ser = serial.Serial('/dev/ttyUSB0', 115200)
ser.write(bytes([0x55, 0xAA, 0x03, 50, 0x8A]))  # 50% brightness
```

## 6️⃣ Kiểm Tra Lỗi

| Lỗi | Nguyên Nhân | Fix |
|-----|-----------|-----|
| LED không sáng | GPIO sai hoặc hardware lỗi | Kiểm tra GPIO3, test LED |
| Button không hoạt động | Pin chưa kết nối đúng | Kiểm tra GPIO4/5 |
| Serial không nhận | COM port sai hoặc baudrate sai | Kiểm tra baudrate 115200 |
| LED nhảy cứng (không fade) | Fade setting sai | Kiểm tra `PWM_FADE_DELAY_MS` |

## 7️⃣ Kiểm Tra UART Error Handling

Gửi lệnh **sai checksum**:
```
55 AA 03 32 FF   (FF là checksum sai)
```

Kết quả: `[ERROR] Code=5, Message: Checksum Mismatch`

Gửi lệnh **sai CMD**:
```
55 AA FF 32 8A   (FF là CMD không hợp lệ)
```

Kết quả: `[ERROR] Code=3, Message: Invalid Command`

## 8️⃣ Cấu Hình Tuỳ Chỉnh

Chỉnh trong `main/common.h`:

```c
// Tăng tốc độ fade (nhỏ hơn = nhanh hơn)
#define PWM_FADE_DELAY_MS   15   // từ 30 → 15ms

// Tăng bước fade (bước lớn hơn = nhảy hơn)
#define PWM_STEP            20   // từ 10 → 20 (2% mỗi step)

// Thay đổi độ nhạy nút (nhỏ hơn = nhạy hơn)
#define BUTTON_DEBOUNCE_MS  10   // từ 20 → 10ms

// Đổi GPIO
#define LED_PIN             GPIO_NUM_2   // từ GPIO3
#define BUTTON_UP_PIN       GPIO_NUM_6   // từ GPIO4
#define BUTTON_DOWN_PIN     GPIO_NUM_7   // từ GPIO5
```

Sau đó rebuild: `idf.py build`

## 9️⃣ Mở Rộng (Webserver)

Cấu trúc project đã sẵn sàng cho webserver:
1. Tạo `main/http_server.c`
2. Sử dụng esp_http_server library
3. Gọi `led_set_brightness_uart()` từ HTTP handler
4. Broadcast state changes qua WebSocket

Ví dụ:
```c
// GET /api/led
{
    "brightness": 50,
    "is_on": true,
    "mode": "PWM"
}

// POST /api/led/brightness
{"brightness": 75}
```

## 🔟 Serial Output Format

```
Boot logs:
========================================
ESP32-C3 LED Controller with Buttons
========================================

I (245) LED_CTRL: Initializing LED Controller...
I (255) PWM_MGR: PWM initialized on GPIO 3, Freq: 1000 Hz
I (265) BTN_HDL: Button handler initialized
I (285) UART: UART initialized - Baudrate: 115200

System ready:
========== SYSTEM READY ==========
Commands:
  PWM Mode:  [0x55][0xAA][0x03][brightness 0-100][checksum]
  ON/OFF:    [0x55][0xAA][0x01][0x00/0x01][checksum]
==================================

Status output (every 1s):
I (10000) MAIN: LED Status - On: YES, Brightness: 50%, Fading: NO
```

## ✅ Checklist Trước Khi Dùng

- [ ] Kết nối phần cứng đúng
- [ ] Flash code thành công
- [ ] Serial monitor hiển thị logs
- [ ] Nhấn button → LED thay đổi
- [ ] Gửi UART command → LED fade mượt mà
- [ ] Kiểm tra error handling
- [ ] Đọc README.md để hiểu cấu trúc

## 📚 Tài Liệu Thêm

- **README.md**: Tài liệu đầy đủ
- **main/common.h**: Hằng số và định nghĩa
- **main/*.h**: API documentation

---

**Bước cuối cùng**: Chuẩn bị cho webserver!

Hiện tại code đã:
✅ Modular và cleancode  
✅ Global state (`g_led_state`)  
✅ Callback-based architecture  
✅ Dễ mở rộng HTTP/WebSocket  

Tiếp theo: Thêm HTTP Server + Web UI 🌐
