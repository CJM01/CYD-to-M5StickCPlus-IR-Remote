// =============================================================================
// M5_IR_Sniffer.ino
// -----------------------------------------------------------------------------
// Use this sketch to learn the IR codes from your existing remote control.
// Point your remote at the M5StickC Plus IR receiver and press a button.
// The protocol, address, and command codes will appear on the screen and
// in the Serial Monitor. Write these down — you'll need them in M5_IR_Remote.ino
//
// Hardware:  M5StickC Plus
// Receiver:  Connected to Grove port — White wire (data) to G33
//            Yellow wire is G32, not used here.
// Library:   IRremote by shirriff/z3t0 (install via Arduino Library Manager)
// Serial:    Open Serial Monitor at 115200 baud
//
// -----------------------------------------------------------------------------
// HOW TO USE:
//   1. Flash this sketch to the M5StickC Plus
//   2. Open Serial Monitor at 115200 baud
//   3. Point your TV/device remote at the M5 and press a button
//   4. Note the Protocol, Address, and Command values for each button
//   5. Enter those values into M5_IR_Remote.ino
//
// Example output:
//   Protocol: NEC | Address: 0x586 | Command: 0xF   <- Power button
//   Protocol: NEC | Address: 0x586 | Command: 0xC   <- Volume Up
//   Protocol: NEC | Address: 0x586 | Command: 0xD   <- Volume Down
// =============================================================================

#include <M5StickCPlus.h>   // Handles LCD, power management (AXP192), and buttons
#include <IRremote.hpp>

// -----------------------------------------------------------------------------
// Pin definition
// Grove port on M5StickC Plus: Yellow = G32, White = G33
// Connect your IR receiver data pin to G33 (White)
// -----------------------------------------------------------------------------
const int IR_RECEIVE_PIN = 33;

// -----------------------------------------------------------------------------
// setup()
// M5.begin() starts Serial, LCD, and the AXP192 power management chip.
// Arguments: (LCD enable, Power enable, Serial enable)
// -----------------------------------------------------------------------------
void setup() {
  M5.begin(true, true, true);

  M5.Lcd.setRotation(3);        // Landscape, USB port on left
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("IR Sniffer");
  M5.Lcd.println("Ready...");

  // Start the IR receiver on G33
  // ENABLE_LED_FEEDBACK flashes the built-in LED when a signal is received
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  Serial.println("IR Sniffer ready. Point your remote at the M5 and press a button.");
  Serial.println("Protocol | Address | Command");
  Serial.println("-------------------------------");
}

// -----------------------------------------------------------------------------
// loop()
// Waits for an IR signal. When one arrives, prints the decoded result to both
// the Serial Monitor and the M5 screen, then resumes listening.
//
// The three values you need for M5_IR_Remote.ino are:
//   Protocol — e.g. NEC, SONY, SAMSUNG. Determines which send function to use.
//   Address  — identifies the device (same for all buttons on one remote)
//   Command  — identifies the specific button pressed
// -----------------------------------------------------------------------------
void loop() {
  M5.update();   // Required — keeps the AXP192 power management happy

  if (IrReceiver.decode()) {

    // --- Print to Serial Monitor ---
    Serial.print("Protocol: ");
    Serial.print(getProtocolString(IrReceiver.decodedIRData.protocol));
    Serial.print(" | Address: 0x");
    Serial.print(IrReceiver.decodedIRData.address, HEX);
    Serial.print(" | Command: 0x");
    Serial.println(IrReceiver.decodedIRData.command, HEX);

    // --- Print to M5 screen ---
    // Useful when you're away from the computer — you can still read the codes
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.printf("Prot:\n %s\n\n",  getProtocolString(IrReceiver.decodedIRData.protocol));
    M5.Lcd.printf("Addr: 0x%X\n\n",  IrReceiver.decodedIRData.address);
    M5.Lcd.printf("Cmd:  0x%X\n",    IrReceiver.decodedIRData.command);

    // IMPORTANT: Must call resume() after every decode or the receiver locks up
    IrReceiver.resume();
  }
}
