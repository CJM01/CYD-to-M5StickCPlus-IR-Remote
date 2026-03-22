// =============================================================================
// CYD_IR_Remote.ino
// -----------------------------------------------------------------------------
// Touchscreen remote control for the Hosyond 4" ESP32 CYD (model 3248S040).
// Displays POWER, VOL+, and VOL- buttons on the screen. When tapped, sends
// the corresponding command string to the M5StickC Plus via ESP-NOW.
// The M5 then fires the IR signal at your TV or device.
//
// Board:    Hosyond 4" ESP32 CYD — select "ESP32 Dev Module" in Arduino IDE
// Serial:   115200 baud (for touch debugging)
//
// -----------------------------------------------------------------------------
// SETUP CHECKLIST (do these before flashing):
//   1. Replace TFT_eSPI/User_Setup.h with the one in /docs (see README)
//   2. Run M5_MAC_Address.ino on the M5 and copy the MAC into broadcastAddress[]
//   3. Run CYD_Touch_Calibration.ino and update the map() values below
//   4. Make sure both devices are on the same WiFi channel (default: 1)
// =============================================================================

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>       // Required for esp_wifi_set_channel()

// -----------------------------------------------------------------------------
// Pin definitions for the 4" Hosyond CYD (model 3248S040)
// NOTE: These differ from the common 2.8" CYD.
//       The touch controller shares the TFT SPI bus on this model.
// -----------------------------------------------------------------------------
#define XPT2046_IRQ  36   // Touch interrupt — input only pin, do not change
#define XPT2046_MOSI 13   // Shared SPI bus with TFT
#define XPT2046_MISO 12   // Shared SPI bus with TFT
#define XPT2046_CLK  14   // Shared SPI bus with TFT
#define XPT2046_CS   33   // Touch chip select (TFT CS is pin 15, set in User_Setup.h)

// -----------------------------------------------------------------------------
// SPI bus and peripheral objects
// VSPI is used for both TFT and touch on this board.
// TFT_eSPI manages its own SPI internally; mySpi is passed to the touch library.
// -----------------------------------------------------------------------------
SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();

// -----------------------------------------------------------------------------
// ESP-NOW target address
// Replace with your M5StickC Plus MAC address from M5_MAC_Address.ino
// -----------------------------------------------------------------------------
uint8_t broadcastAddress[] = {0xF0, 0x24, 0xF9, 0xBC, 0x1D, 0x1C};

// -----------------------------------------------------------------------------
// Touch debounce
// Prevents a single tap from sending multiple commands.
// Increase cooldown (ms) if buttons feel too sensitive.
// -----------------------------------------------------------------------------
unsigned long lastTouchTime = 0;
const unsigned long cooldown = 1000;   // Minimum ms between accepted taps

// -----------------------------------------------------------------------------
// drawButtons()
// Draws the three control buttons on the screen.
// Button positions are tuned to match this unit's touch calibration.
// If buttons feel misaligned on your unit, re-run CYD_Touch_Calibration.ino
// and adjust the fillRoundRect() coordinates and hit zones in loop() to match.
// -----------------------------------------------------------------------------
void drawButtons() {
  tft.fillScreen(TFT_BLACK);

  // POWER button — top center
  tft.fillRoundRect(60, 10, 200, 70, 15, TFT_RED);
  tft.drawCentreString("POWER", 160, 33, 4);

  // VOL+ button — bottom left
  tft.fillRoundRect(10, 130, 140, 90, 15, TFT_BLUE);
  tft.drawCentreString("V +", 80, 165, 4);

  // VOL- button — bottom right
  tft.fillRoundRect(170, 130, 140, 90, 15, TFT_BLUE);
  tft.drawCentreString("V -", 240, 165, 4);
}

// -----------------------------------------------------------------------------
// setup()
// Initialization order matters:
//   1. Touch SPI must start before tft.init() to avoid SPI bus conflicts
//   2. WiFi channel must be forced before esp_now_init()
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  // --- Touch controller first ---
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(1);
  delay(50);

  // --- Then the display ---
  tft.init();
  tft.setRotation(1);
  delay(50);

  // -------------------------------------------------------------------------
  // Force WiFi to channel 1.
  // Both the CYD and M5 must be on the same channel or ESP-NOW packets won't
  // be received. Promiscuous mode is used temporarily to allow the channel
  // change without connecting to an access point.
  // -------------------------------------------------------------------------
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  delay(100);

  // --- Initialize ESP-NOW and register the M5 as a peer ---
  esp_now_init();
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel  = 1;
  peerInfo.encrypt  = false;
  esp_now_add_peer(&peerInfo);

  drawButtons();
  Serial.println("Ready. Tap the screen!");
}

// -----------------------------------------------------------------------------
// loop()
// Polls the touchscreen, maps raw ADC values to screen coordinates, checks
// which button was tapped, and sends the command string via ESP-NOW.
//
// CALIBRATION NOTE:
//   The map() values below were derived from CYD_Touch_Calibration.ino.
//   On the 4" Hosyond CYD both axes are inverted (high raw = low screen coord).
//   If your unit reads differently, update the four map() arguments to match
//   your calibration output:
//
//     int x = map(p.x, TOP_LEFT_RAW_X, TOP_RIGHT_RAW_X, 0, 320);
//     int y = map(p.y, TOP_LEFT_RAW_Y, BOTTOM_LEFT_RAW_Y, 0, 240);
// -----------------------------------------------------------------------------
void loop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();

    // Reject invalid readings:
    // z == 0 means no pressure detected
    // z >= 4095 is a saturated ADC value — phantom/floating touch
    if (p.z == 0 || p.z >= 4095) return;

    // Map raw ADC values to screen pixel coordinates
    // Values are inverted on this unit (high raw = left/top of screen)
    int x = map(p.x, 3750, 400, 0, 320);
    int y = map(p.y, 3600, 400, 0, 240);

    // Clamp to valid screen bounds in case raw values exceed calibration range
    x = constrain(x, 0, 320);
    y = constrain(y, 0, 240);

    Serial.printf("MAPPED - X: %d, Y: %d, Z: %d\n", x, y, p.z);

    // Only process a tap if the cooldown period has passed
    if (millis() - lastTouchTime > cooldown) {
      char command[10] = "";

      // Check which button was tapped based on mapped coordinates
      // These hit zones match the fillRoundRect() positions in drawButtons()
      if      (x > 60  && x < 260 && y > 10  && y < 80)  strcpy(command, "POWER");
      else if (x > 10  && x < 150 && y > 130 && y < 220) strcpy(command, "VOL_UP");
      else if (x > 170 && x < 310 && y > 130 && y < 220) strcpy(command, "VOL_DOWN");

      if (strlen(command) > 0) {
        // Send the command string to the M5 via ESP-NOW
        esp_now_send(broadcastAddress, (uint8_t *)command, sizeof(command));
        Serial.printf(">>> Sent: %s\n", command);

        lastTouchTime = millis();

        // Brief white flash around the screen edge to confirm the send
        tft.drawRoundRect(0, 0, 320, 240, 10, TFT_WHITE);
        delay(150);
        drawButtons();
      } else {
        // Touch registered but didn't land on any button
        // Check Serial output to see where the tap landed and adjust hit zones
        Serial.println("--- Miss: outside button zones");
      }
    }
  }
}
