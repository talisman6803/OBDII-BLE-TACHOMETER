#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <BLEDevice.h>

#include "OBDParser.h"
#include "BLEManager.h"
#include "DisplayManager.h"

#ifdef GFX_BL
  #define BL_PIN GFX_BL
#else
  #define BL_PIN 1
#endif

Arduino_DataBus *bus     = new Arduino_ESP32QSPI(45,47,21,48,40,39);
Arduino_GFX     *panel   = new Arduino_NV3041A(bus, GFX_NOT_DEFINED, 0, true);
Arduino_GFX     *display = new Arduino_Canvas(480, 272, panel);

void setup() {
  Serial.begin(115200);
  pinMode(BL_PIN, OUTPUT); digitalWrite(BL_PIN, HIGH);

  initDisplay();
  showStatus("Initializing...");
  initBLE();
}

void loop() {
  if (!isBLEConnected()) {
    showStatus("Scanning BLE...");
    delay(200);
    return;
  }

  pollOBD();
  updateDashboard();
}