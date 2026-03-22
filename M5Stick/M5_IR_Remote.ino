// =============================================================================
// M5_IR_Remote.ino
// -----------------------------------------------------------------------------
// Receives ESP-NOW commands from the CYD touchscreen and fires the
// corresponding IR signal to control a TV or other IR device.
//
// Hardware:  M5StickC Plus (built-in IR transmitter on pin 32)
// Library:   IRremote by shirriff/z3t0, M5StickCPlus by M5Stack
// Serial:    115200 baud (optional, for debugging)
//
// -----------------------------------------------------------------------------
// SETUP:
//   1. Run M5_MAC_Address.ino first and copy your MAC into CYD_IR_Remote.ino
//   2. Run M5_IR_Sniffer.ino to find your device's IR protocol/address/commands
//   3. Update NEC_ADDRESS and the command values in sendNEC() calls below
//   4. Both this device and the CYD must be on the same WiFi channel (default: 1)
//
// IR CODE CUSTOMIZATION:
//   - Change NEC_ADDRESS to match your device's address from the sniffer
//   - Change the 0xF / 0xC / 0xD command bytes to match your remote's codes
//   - If your device uses a protocol other than NEC (e.g. SAMSUNG, SONY),
//     replace IrSender.sendNEC() with the appropriate IRremote send function,
//     e.g. IrSender.sendSamsung(), IrSender.sendSony(), etc.
// =============================================================================

#include <M5StickCPlus.h>   // LCD, buttons, AXP192 power management
#include <IRremote.hpp>     // IR transmit library
#include <esp_now.h>        // ESP-NOW wireless protocol (no router needed)
#include <WiFi.h>
#include <esp_wifi.h>       // Required for esp_wifi_set_channel()

// -----------------------------------------------------------------------------
// IR configuration
// Update these values with the codes from M5_IR_Sniffer.ino
// -----------------------------------------------------------------------------
const int IR_SEND_PIN       = 32;     // Built-in IR LED on M5StickC Plus
const uint16_t NEC_ADDRESS  = 0x586;  // Device address (same for all buttons)
// Button command codes — replace with your device's values from the sniffer:
//   POWER   = 0xF
//   VOL_UP  = 0xC
//   VOL_DOWN= 0xD

// -----------------------------------------------------------------------------
// ESP-NOW callback flags
// The OnDataRecv callback runs in a WiFi interrupt context — it is NOT safe
// to call IrSender or M5.Lcd from inside it. Instead we set boolean flags
// here and handle everything in loop() on the main thread.
// -----------------------------------------------------------------------------
volatile bool sendPower   = false;
volatile bool sendVolUp   = false;
volatile bool sendVolDown = false;

// Buffer to pass the received command string to loop() for display
char displayMsg[20] = "";

// -----------------------------------------------------------------------------
// OnDataRecv()
// Called automatically by ESP-NOW when a packet arrives from the CYD.
// IMPORTANT: Keep this function minimal — only set flags and copy data.
//            Never call IrSender, M5.Lcd, or delay() from here.
// -----------------------------------------------------------------------------
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  char command[10];
  memcpy(&command, incomingData, sizeof(command));

  // Copy command to display buffer for loop() to show on screen
  strncpy(displayMsg, command, sizeof(displayMsg) - 1);
  displayMsg[sizeof(displayMsg) - 1] = '\0';   // Ensure null termination

  // Set the appropriate flag — loop() will handle the actual IR send
  if      (strcmp(command, "POWER")    == 0) sendPower   = true;
  else if (strcmp(command, "VOL_UP")   == 0) sendVolUp   = true;
  else if (strcmp(command, "VOL_DOWN") == 0) sendVolDown = true;
}

// -----------------------------------------------------------------------------
// setup()
// -----------------------------------------------------------------------------
void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);    // Landscape, USB port on left
  M5.Lcd.setTextSize(2);

  // Start the IR transmitter on the built-in IR LED pin
  IrSender.begin(IR_SEND_PIN);

  // -------------------------------------------------------------------------
  // Force WiFi to channel 1 before initializing ESP-NOW.
  // Both the CYD and M5 must be on the same channel or packets won't arrive.
  // Promiscuous mode is used temporarily to allow the channel change without
  // needing to be connected to an access point.
  // -------------------------------------------------------------------------
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  // Initialize ESP-NOW and register the receive callback
  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(OnDataRecv);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.println("Listening on Ch1...");
  } else {
    // If this appears, try power cycling the device
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.println("ESP-NOW init FAILED");
  }
}

// -----------------------------------------------------------------------------
// loop()
// Handles screen updates and IR sends that were flagged by OnDataRecv().
// Doing hardware operations here (instead of in the callback) prevents crashes.
// -----------------------------------------------------------------------------
void loop() {
  M5.update();    // Required — keeps the AXP192 power management chip happy

  // Update screen if a new command was received
  if (strlen(displayMsg) > 0) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.printf("Got: %s\n", displayMsg);
    displayMsg[0] = '\0';   // Clear after displaying
  }

  // Fire IR signals based on flags set by OnDataRecv()
  // The third argument to sendNEC() is the repeat count (0 = send once)
  if (sendPower) {
    IrSender.sendNEC(NEC_ADDRESS, 0xF, 0);
    sendPower = false;
  }
  if (sendVolUp) {
    IrSender.sendNEC(NEC_ADDRESS, 0xC, 0);
    sendVolUp = false;
  }
  if (sendVolDown) {
    IrSender.sendNEC(NEC_ADDRESS, 0xD, 0);
    sendVolDown = false;
  }
}
