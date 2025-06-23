#pragma once
#include <Arduino_GFX_Library.h>

void initDisplay();
void showStatus(const char* line1, const char* line2 = nullptr);
void updateDashboard();