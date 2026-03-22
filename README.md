# CYD to M5StickC Plus — ESP-NOW IR Remote

Control any IR device using a Hosyond 4" ESP32 CYD (Cheap Yellow Display) as a touchscreen remote, sending commands wirelessly via ESP-NOW to an M5StickC Plus which blasts the IR signal.

```
[ CYD Touchscreen ] --ESP-NOW--> [ M5StickC Plus ] --IR--> [ TV / Device ]
```

---

## Hardware Required

| Device | Notes |
|---|---|
| Hosyond 4" ESP32 CYD (model 3248S040) | The 4" model has different SPI pins than the common 2.8" CYD — see setup notes |
| M5StickC Plus 1.1 | Has built-in IR transmitter on pin 32 |
| IR Remote | Infrared Emitter & Receiver Unit |

---

## Required Libraries

Install all of these via **Arduino IDE → Tools → Manage Libraries**:

| Library | Author | Install Name |
|---|---|---|
| TFT_eSPI | Bodmer | `TFT_eSPI` |
| XPT2046_Touchscreen | Paul Stoffregen | `XPT2046_Touchscreen` |
| M5StickCPlus | M5Stack | `M5StickCPlus` |
| IRremote | shirriff / z3t0 | `IRremote` |

These are **built into the ESP32 Arduino core** — no separate install needed:
- `esp_now.h`
- `esp_wifi.h`
- `WiFi.h`

---

## Board Support

If you haven't already, add ESP32 board support to Arduino IDE:

1. Go to **File → Preferences**
2. Add this URL to "Additional Boards Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Go to **Tools → Board → Boards Manager**, search `esp32` and install

For the CYD, select: **ESP32 Dev Module**  
For the M5StickC Plus, select: **M5StickCPlus** (under M5Stack)

---

## Critical: TFT_eSPI User_Setup.h

The TFT_eSPI library requires a manual pin configuration file for the 4" Hosyond CYD.  
**You must replace the default file** or the display will not work.

1. Find the file at:
   ```
   ~/Arduino/libraries/TFT_eSPI/User_Setup.h
   ```
2. Replace its entire contents with the file in `/docs/User_Setup.h` from this repo.

Key pins for the 4" Hosyond model:
```cpp
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_BL   27
#define TOUCH_CS 33
```

> ⚠️ The common 2.8" CYD uses different pins. This repo is tested on the 4" model only.

---

## Setup Order

Follow these steps in order:

### Step 1 — Get the M5StickC Plus MAC Address
Flash `M5StickCPlus/M5_MAC_Address/M5_MAC_Address.ino` to the M5.  
The MAC address will display on screen. Write it down — you'll need it for the CYD sketch.

### Step 2 — Sniff Your IR Remote (optional)
If you want to learn the IR codes from your existing remote:  
Flash `M5StickCPlus/M5_IR_Sniffer/M5_IR_Sniffer.ino` to the M5.  
Point your remote at the M5 and press buttons. Codes will display on screen.  
Note the **protocol**, **address**, and **command** values for each button.

### Step 3 — Calibrate the CYD Touchscreen
Every CYD unit varies slightly. Run the calibration sketch first:  
Flash `CYD/CYD_Touch_Calibration/CYD_Touch_Calibration.ino` to the CYD.  
Follow the on-screen prompts to tap each corner.  
Note the 4 RAW X/Y values printed to Serial Monitor (115200 baud).

Use those values to update the `map()` lines in the main CYD sketch:
```cpp
int x = map(p.x, TOP_LEFT_X, TOP_RIGHT_X, 0, 320);
int y = map(p.y, TOP_LEFT_Y, BOTTOM_LEFT_Y, 0, 240);
```

### Step 4 — Update and Flash the Main Sketches

**In `CYD/CYD_IR_Remote/CYD_IR_Remote.ino`:**
- Replace the MAC address with your M5's MAC:
  ```cpp
  uint8_t broadcastAddress[] = {0xF0, 0x24, 0xF9, 0xBC, 0x1D, 0x1C};
  ```
- Update the `map()` values with your calibration results

**In `M5StickCPlus/M5_IR_Remote/M5_IR_Remote.ino`:**
- Update the IR address and command codes to match your device:
  ```cpp
  const uint16_t NEC_ADDRESS = 0x586;
  // POWER = 0xF, VOL_UP = 0xC, VOL_DOWN = 0xD
  ```

Flash the M5 sketch first, then the CYD sketch.

---

## How It Works

1. The CYD displays touchscreen buttons
2. When a button is tapped, the CYD sends a command string (e.g. `"POWER"`) via ESP-NOW to the M5's MAC address
3. The M5 receives the command and fires the corresponding IR signal
4. Your TV/device responds as if you used the original remote

ESP-NOW is used instead of WiFi because it's faster, needs no router, and works even when no network is available.

---

## Gotchas & Troubleshooting

**Both devices must be on the same WiFi channel.**  
Both sketches force channel 1 using `esp_wifi_set_channel()`. If you change one, change both.

**ESP-IDF 5.x changed the ESP-NOW callback signature.**  
If you get a compilation error about `esp_now_recv_cb_t`, your ESP32 core is 5.x and needs:
```cpp
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len)
```
Instead of the older:
```cpp
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len)
```

**Never send IR or update the LCD inside the ESP-NOW callback.**  
The callback runs in a WiFi interrupt context. Doing hardware operations there causes crashes. Use flags and handle everything in `loop()`.

**Serial Monitor must be set to 115200 baud.**  
The sketches use `Serial.begin(115200)`. Garbage characters in the monitor means the baud rate is mismatched.

**The 4" Hosyond CYD touchscreen shares the TFT SPI bus.**  
The XPT2046 touch controller uses pins 13/12/14 (MOSI/MISO/CLK), not the 32/39/25 pins common in many CYD tutorials written for the 2.8" model.

**Phantom touches (X:0, Y:0, Z:4095) mean the touch IC isn't initializing.**  
Make sure `mySpi.begin()` and `ts.begin()` are called before `tft.init()` in `setup()`.

---

## Project Structure

```
CYD-to-M5StickCPlus-IR-Remote/
│
├── README.md
├── CYD/
│   └── CYD_IR_Remote.ino
│   └── CYD_Touch_Calibration.ino
│
├── M5Stick/
│   └── M5_IR_Remote.ino
│   └── M5_IR_Sniffer.ino
│   └── M5_MAC_Address.ino
│
└── docs/
    └── User_Setup.h
```

---
## Links to Hardware used

- [Hosyond 4" ESP32 CYD](https://www.lcdwiki.com/4.0inch_ESP32-32E_Display)
- [M5StickC Plus 1.1](https://shop.m5stack.com/products/m5stickc-plus-esp32-pico-mini-iot-development-kit)
- [IR REmote](https://shop.m5stack.com/products/ir-unit)

---

## License

MIT — do whatever you want with it. If it helps you, great!
