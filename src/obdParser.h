#pragma once
#include <Arduino.h>

void parseOBDLine(const String &line);
int  getCoolantTemp();
int  getEngineRpm();
int  getVehicleSpeed();
float getFuelLevel();
float getBattVoltage();
int  getGasPedalPos();
int  getEngineOilTemp();