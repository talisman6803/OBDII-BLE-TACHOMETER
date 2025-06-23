#include "DisplayManager.h"
#include "OBDParser.h"

#define SCREEN_W 480
#define SCREEN_H 272
#define RPM_BAR_H 60
#define RPM_BAR_COUNT 20
#define RPM_MAX 5000

extern Arduino_GFX *display;  // main.ino 에서 정의

static void drawRPMBar(int val) {
  int segs = constrain(map(val, 0, RPM_MAX, 0, RPM_BAR_COUNT), 0, RPM_BAR_COUNT);
  int segW = SCREEN_W / RPM_BAR_COUNT;
  display->fillRect(0, 0, SCREEN_W, RPM_BAR_H, BLACK);
  for (int i = 0; i < segs; i++) {
    uint16_t c = (i < 8) ? GREEN : (i < 14) ? YELLOW : RED;
    display->fillRect(i * segW, 0, segW - 2, RPM_BAR_H, c);
  }
}

static void drawVerticalBar(int x, int y, int w, int h, int percent, uint16_t color) {
  int fillH = constrain(map(percent, 0, 100, 0, h), 0, h);
  display->drawRect(x, y, w, h, WHITE);
  display->fillRect(x+1, y + (h - fillH) + 1, w-2, fillH-2, color);
}

void initDisplay() {
  if (!display->begin()) Serial.println("Display init failed");
}

void showStatus(const char* line1, const char* line2) {
  display->fillScreen(BLACK);
  display->setTextSize(5);
  display->setTextColor(WHITE);
  display->setCursor(0,0);
  display->printf("OBDII-BLE\nTACHOMETER");
  display->setTextSize(4);
  display->setCursor(10, SCREEN_H/2 - 20);
  display->print(line1);
  if (line2) {
    display->setCursor(10, SCREEN_H/2 + 20);
    display->print(line2);
  }
  display->flush();
}

void updateDashboard() {
  display->fillScreen(BLACK);
  drawRPMBar(getEngineRpm());

  display->setTextSize(18);
  display->setTextColor(WHITE);
  display->setCursor(0, 70);
  display->printf("%3d", getVehicleSpeed());

  display->setTextSize(6);
  display->setTextColor(YELLOW);
  display->setCursor(324, 70);
  display->printf("%4d", getEngineRpm());

  display->setTextSize(4);
  display->setTextColor(CYAN);
  display->setCursor(4, 220);
  display->printf("%3dC", getEngineOilTemp());

  display->setTextSize(4);
  display->setTextColor(WHITE);
  display->setCursor(107, 220);
  display->printf("%3dC", getCoolantTemp());

  display->setTextSize(4);
  display->setTextColor(ORANGE);
  display->setCursor(211, 220);
  display->printf("%4.1fV", getBattVoltage());

  drawVerticalBar(376, 130, 52, 142, (int)getFuelLevel(), GREEN);
  display->setTextSize(2);
  display->setCursor(376, 192);
  if ((int)getFuelLevel() == 0) {
    display->print("Null");
  }
  else {
    display->printf("%3d%%", (int)getFuelLevel());
  }

  drawVerticalBar(428, 130, 52, 142, getGasPedalPos(), BLUE);
  display->setTextSize(2);
  display->setCursor(428, 192);
  display->printf("%3d%%", getGasPedalPos());

  display->flush();
}