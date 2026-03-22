// =============================================================================
// M5_MAC_Address.ino
// -----------------------------------------------------------------------------
// Run this sketch FIRST before setting up the main IR Remote sketches.
// It reads and displays the M5StickC Plus's real hardware MAC address —
// the unique identifier the CYD needs to send ESP-NOW messages to this device.
//
// Hardware:  M5StickC Plus
// Serial:    Open Serial Monitor at 115200 baud
//
// -----------------------------------------------------------------------------
// HOW TO USE:
//   1. Flash this sketch to the M5StickC Plus
//   2. The MAC address will appear on the screen and in Serial Monitor
//   3. Copy the MAC address into CYD_IR_Remote.ino:
//
//      uint8_t broadcastAddress[] = {0xF0, 0x24, 0xF9, 0xBC, 0x1D, 0x1C};
//                                     ↑ replace with your values
//
// NOTE: Why "REAL MAC"?
//   The ESP32 has a base MAC in eFuse. When WiFi is initialized in WIFI_STA
//   mode, the actual station MAC is derived from that base address and may
//   differ by one byte. Always use this sketch to get the MAC that ESP-NOW
//   will actually respond to — don't rely on the sticker on the board.
// =============================================================================

#include <M5StickCPlus.h>
#include <WiFi.h>

void setup() {
  // M5.begin() starts the LCD, power management, and Serial
  M5.begin();

  M5.Lcd.setRotation(3);      // Landscape, USB port on left
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);

  // -------------------------------------------------------------------------
  // Step 1: Set WiFi to Station mode
  // ESP-NOW requires WiFi to be initialized first. WIFI_STA mode gives us
  // the correct MAC address that ESP-NOW will use for receiving messages.
  // -------------------------------------------------------------------------
  WiFi.mode(WIFI_STA);

  // Step 2: Short delay to let the WiFi radio fully power up
  delay(500);

  // Step 3: Read the MAC address into a 6-byte array
  uint8_t mac[6];
  WiFi.macAddress(mac);

  // -------------------------------------------------------------------------
  // Step 4: Print to Serial Monitor
  // Format: F0:24:F9:BC:1D:1C  (colon-separated hex pairs)
  // -------------------------------------------------------------------------
  Serial.begin(115200);
  Serial.printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println("\nCopy this into CYD_IR_Remote.ino as:");
  Serial.printf("uint8_t broadcastAddress[] = {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X};\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // -------------------------------------------------------------------------
  // Step 5: Display on screen
  // Split across two lines so it fits the small display in large text.
  // First half: XX:XX:XX  Second half: XX:XX:XX
  // -------------------------------------------------------------------------
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(10, 20);
  M5.Lcd.println("MAC Address:");

  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setCursor(10, 50);
  M5.Lcd.printf("%02X:%02X:%02X\n%02X:%02X:%02X",
                mac[0], mac[1], mac[2],
                mac[3], mac[4], mac[5]);
}

// -----------------------------------------------------------------------------
// loop()
// Nothing to do — MAC address only needs to be read once.
// M5.update() is called to keep the power management chip happy.
// -----------------------------------------------------------------------------
void loop() {
  M5.update();
}
