// =============================================================================
// CYD_Touch_Calibration.ino
// -----------------------------------------------------------------------------
// Run this sketch BEFORE the main CYD_IR_Remote sketch.
// Every XPT2046 touchscreen unit has slightly different raw ADC output ranges.
// This sketch walks you through tapping all 4 corners of the screen and prints
// the raw X/Y values to Serial Monitor so you can plug them into the map()
// calls in CYD_IR_Remote.ino.
//
// Board:   Hosyond 4" ESP32 CYD (model 3248S040) — ESP32 Dev Module in IDE
// Serial:  Open Serial Monitor at 115200 baud
// =============================================================================

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// -----------------------------------------------------------------------------
// Pin definitions for the 4" Hosyond CYD
// NOTE: The touch controller shares the TFT SPI bus on this model.
//       These are different from the common 2.8" CYD (which uses 25/32/39).
// -----------------------------------------------------------------------------
#define XPT2046_IRQ  36   // Touch interrupt — input only pin, do not change
#define XPT2046_MOSI 13   // Shared with TFT
#define XPT2046_MISO 12   // Shared with TFT
#define XPT2046_CLK  14   // Shared with TFT
#define XPT2046_CS   33   // Touch chip select — separate from TFT CS (pin 15)

// -----------------------------------------------------------------------------
// SPI bus and peripheral objects
// VSPI is used for both the TFT and touch controller on this board.
// TFT_eSPI manages its own SPI internally; we pass mySpi to the touch library.
// -----------------------------------------------------------------------------
SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();

// -----------------------------------------------------------------------------
// Calibration state
// -----------------------------------------------------------------------------
int step = 0;                    // Which corner we are currently collecting (0-3)
unsigned long lastTap = 0;       // Debounce timer — prevents double-registering a tap

// Stores the raw ADC values for each corner tap
struct CalPoint { int rawX, rawY; };
CalPoint pts[4];

// On-screen instructions for each step
const char* labels[] = {
  "Tap TOP-LEFT corner",
  "Tap TOP-RIGHT corner",
  "Tap BOTTOM-LEFT corner",
  "Tap BOTTOM-RIGHT corner"
};

// -----------------------------------------------------------------------------
// showStep()
// Draws the instruction text and a small red crosshair in the target corner.
// The crosshair is placed slightly inset from the edge so it's visible.
// -----------------------------------------------------------------------------
void showStep() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString(labels[step], 160, 100, 4);

  // Position crosshair in the correct corner
  int cx, cy;
  if (step == 0) { cx = 20;  cy = 20;  }   // Top-left
  if (step == 1) { cx = 300; cy = 20;  }   // Top-right
  if (step == 2) { cx = 20;  cy = 220; }   // Bottom-left
  if (step == 3) { cx = 300; cy = 220; }   // Bottom-right

  // Draw crosshair lines
  tft.drawLine(cx - 10, cy,      cx + 10, cy,      TFT_RED);
  tft.drawLine(cx,      cy - 10, cx,      cy + 10, TFT_RED);
}

// -----------------------------------------------------------------------------
// setup()
// Touch SPI must be initialized before tft.init() to avoid SPI bus conflicts.
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  // Start touch SPI first
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(1);

  // Then init the display
  tft.init();
  tft.setRotation(1);

  // Show the first calibration step
  showStep();
}

// -----------------------------------------------------------------------------
// loop()
// Waits for a valid tap, records the raw values, then advances to the next step.
// After all 4 corners are collected, prints the calibration summary to Serial.
// -----------------------------------------------------------------------------
void loop() {
  // 1500ms cooldown prevents a single tap registering twice
  if (ts.touched() && millis() - lastTap > 1500) {
    TS_Point p = ts.getPoint();

    // Reject invalid readings:
    // z == 0 means no pressure, z >= 4095 is a saturated/phantom reading
    if (p.z == 0 || p.z >= 4095) return;

    // Store raw values for this corner
    pts[step].rawX = p.x;
    pts[step].rawY = p.y;

    Serial.printf("Step %d (%s): RAW X=%d, Y=%d\n",
                  step, labels[step], p.x, p.y);

    // Flash green to confirm the tap was recorded
    tft.fillScreen(TFT_GREEN);
    tft.drawCentreString("Got it!", 160, 100, 4);
    delay(800);

    step++;

    if (step < 4) {
      // More corners to collect — show the next step
      showStep();
    } else {
      // All 4 corners collected — print the calibration summary
      tft.fillScreen(TFT_BLACK);
      tft.drawCentreString("Done! Check Serial.", 160, 100, 4);

      Serial.println("\n=== CALIBRATION RESULTS ===");
      Serial.printf("TOP-LEFT:     X=%d, Y=%d\n", pts[0].rawX, pts[0].rawY);
      Serial.printf("TOP-RIGHT:    X=%d, Y=%d\n", pts[1].rawX, pts[1].rawY);
      Serial.printf("BOTTOM-LEFT:  X=%d, Y=%d\n", pts[2].rawX, pts[2].rawY);
      Serial.printf("BOTTOM-RIGHT: X=%d, Y=%d\n", pts[3].rawX, pts[3].rawY);
      Serial.println("===========================");

      // ---------------------------------------------------------------------
      // HOW TO USE THESE VALUES in CYD_IR_Remote.ino:
      //
      // Look at your results to determine if the axes are flipped.
      // On most 4" Hosyond units, both X and Y read HIGH on the left/top
      // and LOW on the right/bottom, so the map() calls need to be reversed:
      //
      //   int x = map(p.x, TOP_LEFT_X, TOP_RIGHT_X, 0, 320);
      //   int y = map(p.y, TOP_LEFT_Y, BOTTOM_LEFT_Y, 0, 240);
      //
      // Example using sample calibration values:
      //   int x = map(p.x, 3750, 400, 0, 320);
      //   int y = map(p.y, 3600, 400, 0, 240);
      //
      // Replace 3750, 400, 3600, 400 with your actual results above.
      // ---------------------------------------------------------------------
      Serial.println("\nUse these values in CYD_IR_Remote.ino:");
      Serial.printf("  int x = map(p.x, %d, %d, 0, 320);\n", pts[0].rawX, pts[1].rawX);
      Serial.printf("  int y = map(p.y, %d, %d, 0, 240);\n", pts[0].rawY, pts[2].rawY);
      Serial.println("===========================");
    }

    lastTap = millis();
  }
}
