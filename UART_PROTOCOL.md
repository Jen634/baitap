# UART Protocol Reference

## Protocol Format

```
Byte 0     Byte 1     Byte 2   Byte 3   Byte 4
[Header1]  [Header2]  [CMD]    [Value]  [Checksum]
  0x55       0xAA     ...      ...      (XOR)
```

## Checksum Calculation

**Formula**: `Checksum = Header1 XOR Header2 XOR CMD XOR Value`

### Ví dụ 1: PWM Mode - Set brightness 50%

```
Header1 = 0x55
Header2 = 0xAA
CMD = 0x03 (PWM mode)
Value = 50 (decimal) = 0x32 (hex)

Checksum = 0x55 XOR 0xAA XOR 0x03 XOR 0x32
```

**Tính toán từng bước:**
```
  0x55 = 0101 0101
  0xAA = 1010 1010
       ___________
XOR  = 1111 1111 = 0xFF

  0xFF = 1111 1111
  0x03 = 0000 0011
       ___________
XOR  = 1111 1100 = 0xFC

  0xFC = 1111 1100
  0x32 = 0011 0010
       ___________
XOR  = 1100 1110 = 0xCE
```

**Kết quả**: Frame = `55 AA 03 32 CE`

**Wait, recalculate**:
```python
>>> hex(0x55 ^ 0xAA ^ 0x03 ^ 0x32)
'0x8a'
```

Correct: **Frame = `55 AA 03 32 8A`**

---

## Commands Reference

### Command 1: PWM Mode (0x03)

**Mục đích**: Đặt độ sáng LED từ 0-100%

**Format**:
```
[0x55] [0xAA] [0x03] [Brightness 0-100] [Checksum]
```

**Valid Values**:
- Brightness: 0-100 (decimal)
- 0% = LED OFF
- 100% = LED fully ON
- Values > 100 sẽ báo error: `UART_ERR_INVALID_VALUE`

**Ví dụ 1**: Set brightness 25%
```
Brightness = 25 (0x19)
Checksum = 0x55 ^ 0xAA ^ 0x03 ^ 0x19 = 0x7A

Frame: 55 AA 03 19 7A
```

**Ví dụ 2**: Set brightness 75%
```
Brightness = 75 (0x4B)
Checksum = 0x55 ^ 0xAA ^ 0x03 ^ 0x4B = 0x08

Frame: 55 AA 03 4B 08
```

**Ví dụ 3**: Set brightness 100%
```
Brightness = 100 (0x64)
Checksum = 0x55 ^ 0xAA ^ 0x03 ^ 0x64 = 0x31

Frame: 55 AA 03 64 31
```

**LED behavior**:
- Fade từ hiện tại → target brightness
- Mượt mà 30ms per step (1% per step)
- Có thể interrupt bằng button

---

### Command 2: ON/OFF Mode (0x01)

**Mục đích**: Bật hoặc tắt LED

**Format**:
```
[0x55] [0xAA] [0x01] [State: 0x00 or 0x01] [Checksum]
```

**Valid Values**:
- `0x00` = LED OFF (fade to 0%)
- `0x01` = LED ON (fade to 100%)
- Giá trị khác → `UART_ERR_INVALID_VALUE`

**Ví dụ 1**: Turn LED ON
```
State = 0x01
Checksum = 0x55 ^ 0xAA ^ 0x01 ^ 0x01 = 0x55

Frame: 55 AA 01 01 55
```

**Ví dụ 2**: Turn LED OFF
```
State = 0x00
Checksum = 0x55 ^ 0xAA ^ 0x01 ^ 0x00 = 0x54

Frame: 55 AA 01 00 54
```

**LED behavior**:
- ON: Fade từ hiện tại → 100%
- OFF: Fade từ hiện tại → 0%
- Nếu LED OFF trước đó, bật ON sẽ tới 100%
- Nếu LED ON ở 50%, tắt OFF sẽ fade từ 50% → 0%

---

## Error Codes

### UART_ERR_HEADER1 (Code 1)
**Nguyên nhân**: Byte đầu tiên không phải 0x55
```
Example: AA 55 03 32 8A
         ↑
         Expected 0x55, got 0xAA
```

**Response**: 
```
[ERROR] Code=1, Message: Invalid Header Byte 1 (expected 0x55)
```

---

### UART_ERR_HEADER2 (Code 2)
**Nguyên nhân**: Byte thứ hai không phải 0xAA
```
Example: 55 55 03 32 8A
            ↑
            Expected 0xAA, got 0x55
```

**Response**:
```
[ERROR] Code=2, Message: Invalid Header Byte 2 (expected 0xAA)
```

---

### UART_ERR_INVALID_CMD (Code 3)
**Nguyên nhân**: CMD không phải 0x01 hoặc 0x03
```
Example: 55 AA FF 32 8A
                ↑
                Expected 0x01 or 0x03, got 0xFF
```

**Response**:
```
[ERROR] Code=3, Message: Invalid Command (0x03=PWM, 0x01=ON/OFF)
```

---

### UART_ERR_INVALID_VALUE (Code 4)
**Nguyên nhân**: Value không hợp lệ cho command

**Cases**:
- **CMD 0x03 (PWM)**: Value > 100
  ```
  Example: 55 AA 03 FF 8A   (brightness 255 > 100)
  ```

- **CMD 0x01 (ON/OFF)**: Value ≠ 0x00 và ≠ 0x01
  ```
  Example: 55 AA 01 02 8A   (state 2, should be 0 or 1)
  ```

**Response**:
```
[ERROR] Code=4, Message: Invalid Value (PWM: 0-100, ON/OFF: 0x00/0x01)
```

---

### UART_ERR_CHECKSUM (Code 5)
**Nguyên nhân**: Checksum không khớp
```
Example: 55 AA 03 32 FF
                     ↑
Correct checksum: 8A
Given checksum: FF
```

**Response**:
```
[ERROR] Code=5, Message: Checksum Mismatch
```

**Debug**: Tính lại checksum
```python
correct = 0x55 ^ 0xAA ^ 0x03 ^ 0x32  # = 0x8A
given = 0xFF
# Mismatch!
```

---

## Complete Frame Examples

### Example 1: Brightness 0% (OFF)
```
Description: Turn LED off gradually
Brightness:  0%

Calculation:
  Header1   = 0x55
  Header2   = 0xAA
  CMD       = 0x03 (PWM)
  Value     = 0x00 (0%)
  Checksum  = 0x55 ^ 0xAA ^ 0x03 ^ 0x00 = 0xFC

Frame: 55 AA 03 00 FC
```

### Example 2: Brightness 33%
```
Description: Set LED to 1/3 brightness
Brightness:  33%

Calculation:
  Header1   = 0x55
  Header2   = 0xAA
  CMD       = 0x03 (PWM)
  Value     = 33 = 0x21
  Checksum  = 0x55 ^ 0xAA ^ 0x03 ^ 0x21 = 0x49

Frame: 55 AA 03 21 49
```

### Example 3: Brightness 90%
```
Description: Set LED to 90% brightness
Brightness:  90%

Calculation:
  Header1   = 0x55
  Header2   = 0xAA
  CMD       = 0x03 (PWM)
  Value     = 90 = 0x5A
  Checksum  = 0x55 ^ 0xAA ^ 0x03 ^ 0x5A = 0xF7

Frame: 55 AA 03 5A F7
```

### Example 4: Turn ON with fade
```
Description: Turn LED ON (fade from current to 100%)
State:       ON

Calculation:
  Header1   = 0x55
  Header2   = 0xAA
  CMD       = 0x01 (ON/OFF)
  Value     = 0x01 (ON)
  Checksum  = 0x55 ^ 0xAA ^ 0x01 ^ 0x01 = 0x55

Frame: 55 AA 01 01 55
```

### Example 5: Turn OFF with fade
```
Description: Turn LED OFF (fade from current to 0%)
State:       OFF

Calculation:
  Header1   = 0x55
  Header2   = 0xAA
  CMD       = 0x01 (ON/OFF)
  Value     = 0x00 (OFF)
  Checksum  = 0x55 ^ 0xAA ^ 0x01 ^ 0x00 = 0x54

Frame: 55 AA 01 00 54
```

---

## Quick Reference Table

### Brightness Values (CMD 0x03)

| Brightness | Hex | Frame |
|------------|-----|-------|
| 0% | 0x00 | 55 AA 03 00 FC |
| 10% | 0x0A | 55 AA 03 0A F6 |
| 20% | 0x14 | 55 AA 03 14 EC |
| 30% | 0x1E | 55 AA 03 1E E2 |
| 40% | 0x28 | 55 AA 03 28 D8 |
| 50% | 0x32 | 55 AA 03 32 CE |
| 60% | 0x3C | 55 AA 03 3C C4 |
| 70% | 0x46 | 55 AA 03 46 BA |
| 80% | 0x50 | 55 AA 03 50 A8 |
| 90% | 0x5A | 55 AA 03 5A F7 |
| 100% | 0x64 | 55 AA 03 64 31 |

### ON/OFF Values (CMD 0x01)

| State | Hex | Frame | Effect |
|-------|-----|-------|--------|
| OFF | 0x00 | 55 AA 01 00 54 | Fade to 0% |
| ON | 0x01 | 55 AA 01 01 55 | Fade to 100% |

---

## Testing Checklist

- [ ] Frame header correct (0x55 0xAA)
- [ ] CMD value valid (0x01 or 0x03)
- [ ] Value in valid range
- [ ] Checksum calculated correctly
- [ ] Send at 115200 bps
- [ ] LED fades (doesn't jump)
- [ ] Error messages clear

---

## Checksum Calculator (Python)

```python
def calc_checksum(cmd, value):
    return 0x55 ^ 0xAA ^ cmd ^ value

# Example
brightness = 50
checksum = calc_checksum(0x03, brightness)
print(f"Checksum for brightness {brightness}: 0x{checksum:02X}")
# Output: Checksum for brightness 50: 0x8a
```

---

## Common Mistakes

### ❌ Mistake 1: Forgetting Header
```
Wrong:  03 32 8A        (missing 55 AA)
Correct: 55 AA 03 32 8A
```

### ❌ Mistake 2: Wrong Decimal/Hex Conversion
```
Wrong:  55 AA 03 100 8A  (100 decimal, not 0x64)
Correct: 55 AA 03 64 31
```

### ❌ Mistake 3: Wrong Checksum
```
Wrong:  55 AA 03 32 FF   (0xFF is wrong)
Correct: 55 AA 03 32 8A  (0x8A is correct)
```

### ❌ Mistake 4: Wrong Frame Format
```
Wrong:  55 AA 03 50 A8 00 00  (extra bytes)
Correct: 55 AA 03 50 A8       (exactly 5 bytes)
```

### ❌ Mistake 5: Invalid Value for Command
```
Wrong:  55 AA 03 150 XX   (brightness 150 > 100)
Correct: 55 AA 03 64 31   (brightness 100)
```

---

## LED Behavior Notes

1. **Fade Duration**: 
   - 0% → 100%: ~3 seconds (100 steps × 30ms)
   - Can be changed via `PWM_FADE_DELAY_MS` and `PWM_STEP`

2. **Button Priority**:
   - Button always overrides UART
   - Button press interrupts fade

3. **State Persistence**:
   - LED remembers last state after reboot
   - Brightness = 0 initially

4. **Error Handling**:
   - Bad frames logged to serial
   - LED doesn't change on error
   - Can retry with correct frame

---

**Last Updated**: 2026-02-03  
**Protocol Version**: 1.0  
