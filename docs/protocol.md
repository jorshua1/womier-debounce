# Womier V-K66 USB Lighting Protocol Specification

## 1. Hardware Identification

* **Manufacturer**: China Resource Semico Co., Ltd.
* **USB Vendor ID (VID)**: `0x1A2C`
* **USB Product ID (PID)**: `0x9206`
* **Device**: Womier V-K66 Mechanical Keyboard

---

## 2. USB Transport

Lighting control does not require proprietary kernel drivers. It operates through standard **USB HID Feature Reports** on Endpoint 0 (Control Transfer).

* **bmRequestType**: `0x21` (`Host-to-Device | Class | Interface`)
* **bRequest**: `0x09` (`SET_REPORT`)
* **wValue**: `0x0307`
  * Report Type: `0x03` (Feature Report)
  * Report ID: `0x07`
* **wIndex**: `0x0001` (Interface 1 / UsagePage `0xFF01`)
* **wLength**: `8 bytes`

---

## 3. Payload Structure (8 Bytes)

```
+---------+---------+---------+---------+---------+------------+---------+---------+
| Byte 0  | Byte 1  | Byte 2  | Byte 3  | Byte 4  |   Byte 5   | Byte 6  | Byte 7  |
+---------+---------+---------+---------+---------+------------+---------+---------+
|  0x07   |  0xFF   |  0xFF   |  MODE   |  SPEED  | BRIGHTNESS |  0x00   |  0x00   |
+---------+---------+---------+---------+---------+------------+---------+---------+
```

### Field Definitions:

* **Byte 0 (`0x07`)**: Mandatory HID Report ID.
* **Bytes 1-2 (`0xFF 0xFF`)**: Protocol magic signature.
* **Byte 3 (`MODE`)**: Zero-based lighting animation mode (`0x00` - `0x13`).
  * Note: The OEM Windows driver calculates this as `ModeIndex - 1`.
* **Byte 4 (`SPEED`)**: Animation speed level (`0x01` - `0x05`, default `0x04`).
* **Byte 5 (`BRIGHTNESS`)**: Intensity level (`0x00` - `0x04`, where `0x00` is minimum level and `0x04` is maximum).
* **Bytes 6-7 (`0x00 0x00`)**: Reserved padding.

---

## 4. Animation Modes

| Index (Wire Byte) | Internal Name | Description |
| :---: | :--- | :--- |
| `0x00` | `steady` | Static constant color |
| `0x01` | `custom` | Custom per-key RGB profile |
| `0x02` | `breathing` | Breathing effect |
| `0x03` | `press_destroy` | Reactive press & destroy |
| `0x04` | `neon_stream` | Neon stream wave |
| `0x05` | `streamer` | Streamer wave |
| `0x06` | `flowing` | Flowing light and color |
| `0x07` | `dripping` | Dripping ripples |
| `0x08` | `reactive` | **Brilliant point**: Completely dark/off in idle; lights up only active keys |
| `0x09` | `flash_away` | Keypress flash expansion |
| `0x0A` | `shadow` | Shadow disappear on press |
| `0x0B` | `ripples` | Ripple wave on press |
| `0x0C` | `rich` | Rich and honored pulse |
| `0x0D` | `marquee` | Marquee chase |
| `0x0E` | `storm` | Rotating spiral storm |
| `0x0F` | `serpentine` | Serpentine snake chase |
| `0x10` | `stars` | Twinkling stars |
| `0x11` | `snake` | Retro snake |
| `0x12` | `diagonal` | Diagonal transformation |
| `0x13` | `sine` | Sine wave |
