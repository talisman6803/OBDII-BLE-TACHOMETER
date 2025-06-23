#include "OBDParser.h"

static int coolantTemp   = 0;
static int engineRpm     = 0;
static int vehicleSpeed  = 0;
static float fuelLevel   = 0.0;
static float battVoltage = 0.0;
static int gasPedalPos   = 0;
static int engineOilTemp = 0;

void parseOBDLine(const String &line) {
  int p0, p1, p2, p3;
  int cnt = sscanf(line.c_str(), "%x %x %x %x", &p0, &p1, &p2, &p3);
  if (cnt < 3 || p0 != 0x41) return;
  switch (p1) {
    case 0x05: coolantTemp   = p2 - 40; break;
    case 0x0C: engineRpm     = ((p2 << 8) + p3) / 4; break;
    case 0x0D: vehicleSpeed  = p2; break;
    case 0x2F: fuelLevel     = p2 * 100.0 / 255.0; break;
    case 0x42: battVoltage   = ((p2 << 8) + p3) / 1000.0; break;
    case 0x49: gasPedalPos   = p2 * 100 / 255; break;
    case 0x5C: engineOilTemp = p2 - 40; break;
  }
}

int getCoolantTemp()   { return coolantTemp; }
int getEngineRpm()     { return engineRpm; }
int getVehicleSpeed()  { return vehicleSpeed; }
float getFuelLevel()   { return fuelLevel; }
float getBattVoltage() { return battVoltage; }
int getGasPedalPos()   { return gasPedalPos; }
int getEngineOilTemp() { return engineOilTemp; }