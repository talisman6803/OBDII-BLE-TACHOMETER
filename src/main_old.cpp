#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>

#define SVC_UUID        "0000fff0-0000-1000-8000-00805f9b34fb"
#define CHAR_UUID_TX    "0000fff2-0000-1000-8000-00805f9b34fb" // Write
#define CHAR_UUID_RX    "0000fff1-0000-1000-8000-00805f9b34fb" // Notify

#define SCREEN_W        480
#define SCREEN_H        272
#define RPM_BAR_H       60
#define RPM_BAR_COUNT   20
#define RPM_MAX         5000

Arduino_DataBus *bus     = new Arduino_ESP32QSPI(45,47,21,48,40,39);
Arduino_GFX     *panel   = new Arduino_NV3041A(bus, GFX_NOT_DEFINED, 0, true);
Arduino_GFX     *display = new Arduino_Canvas(SCREEN_W, SCREEN_H, panel);
#ifdef GFX_BL
  #define BL_PIN GFX_BL
#else
  #define BL_PIN 1
#endif

static BLEAddress      *pServerAddress;
static bool             doConnect = false;
static bool             connected = false;
static BLERemoteCharacteristic* pWriteChar;
static BLERemoteCharacteristic* pReadChar;

static String bleBuf = "";

int coolantTemp   = 0;    //pid 0x05
int engineRpm     = 0;    //pid 0x0C
int vehicleSpeed  = 0;    //pid 0x0D
float fuelLevel   = 0.0;    //pid 0x2F
float battVoltage = 0.0;  //pid 0x42
int gasPedalPos   = 0;    //pid 0x49
int engineOilTemp = 0;    //pid 0x5C

void parseOBD(const String &line);

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    if (advertisedDevice.haveServiceUUID() &&
        advertisedDevice.isAdvertisingService(BLEUUID(SVC_UUID))) {
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
      BLEDevice::getScan()->stop();
    }
  }
};

static void notifyCallback(
  BLERemoteCharacteristic* chr,
  uint8_t* data,
  size_t length,
  bool isNotify)
{
  bleBuf += String((char*)data).substring(0, length);
  int idx;
  while ((idx = bleBuf.indexOf('\r')) >= 0) {
    String line = bleBuf.substring(0, idx);
    bleBuf = bleBuf.substring(idx + 1);
    line.trim();
    if (line.length()) {
      parseOBD(line);
    }
  }
}

void parseOBD(const String &line) {
  int p0, p1, p2, p3;
  int cnt = sscanf(line.c_str(), "%x %x %x %x", &p0, &p1, &p2, &p3);
  if (cnt < 3 || p0 != 0x41) return;
  switch (p1) {
    case 0x05: 
      coolantTemp = p2 - 40;
      break;
    case 0x0C: 
      engineRpm = ((p2 << 8) + p3) / 4; 
      break;
    case 0x0D: 
      vehicleSpeed = p2;
      break;
    case 0x2F:
      fuelLevel = p2 * 100.0 / 255.0;
      break;
    case 0x42:
      battVoltage = ((p2 << 8) + p3) / 1000.0;
      break;
    case 0x49:
      gasPedalPos = p2 * 100 / 255;
      break;
    case 0x5C:
      engineOilTemp = p2 - 40;
      break;
  }
}

bool connectToServer() {
  BLEClient* pClient = BLEDevice::createClient();
  if (!pClient->connect(*pServerAddress)) return false;
  BLERemoteService* svc = pClient->getService(SVC_UUID);
  if (!svc) {
    pClient->disconnect();
    return false;
  }
  pWriteChar = svc->getCharacteristic(CHAR_UUID_TX);
  pReadChar  = svc->getCharacteristic(CHAR_UUID_RX);
  if (!pWriteChar || !pReadChar) {
    pClient->disconnect();
    return false;
  }
  if (pReadChar->canNotify()) {
    pReadChar->registerForNotify(notifyCallback);
  } else {
    pClient->disconnect();
    return false;
  }
  return true;
}

void initELM() {
  static const char* cmds[] = {
    "ATZ", "ATE0", "ATL1", "ATH1", "AT CAF1", "AT SP 6"
  };
  for (auto c: cmds) {
    String cmd = String(c) + "\r";
    pWriteChar->writeValue((uint8_t*)cmd.c_str(), cmd.length(), true);
    delay(100);
  }
}

void requestPID(const char* pid) {
  String cmd = String(pid) + "\r";
  pWriteChar->writeValue((uint8_t*)cmd.c_str(), cmd.length(), true);
}

void drawRPMBar(int val) {
  int segs = constrain(map(val, 0, RPM_MAX, 0, RPM_BAR_COUNT), 0, RPM_BAR_COUNT);
  int segW = SCREEN_W / RPM_BAR_COUNT;
  display->fillRect(0, 0, SCREEN_W, RPM_BAR_H, BLACK);
  for (int i = 0; i < segs; i++) {
    uint16_t c = (i < 8) ? GREEN : (i < 14) ? YELLOW : RED;
    display->fillRect(i * segW, 0, segW - 2, RPM_BAR_H, c);
  }
}

void drawVerticalBar(int x, int y, int w, int h, int percent, uint16_t color) {
  int fillH = constrain(map(percent, 0, 100, 0, h), 0, h);
  display->drawRect(x, y, w, h, WHITE);
  display->fillRect(x+1,
                    y + (h - fillH) + 1,
                    w-2,
                    fillH-2,
                    color);
}

void drawDashboard() {
    display->fillScreen(BLACK);
  drawRPMBar(engineRpm);

  //Vehicle Speed
  display->setTextSize(18);
  display->setTextColor(WHITE);
  display->setCursor(0, 70);
  display->printf("%3d", vehicleSpeed);

  //Engine RPM
  display->setTextSize(6);
  display->setTextColor(YELLOW);
  display->setCursor(324, 70);
  display->printf("%4d", engineRpm);

  //Engine Oil Temperature
  display->setTextSize(4);
  display->setTextColor(CYAN);
  display->setCursor(4, 220);
  display->printf("%3dC", engineOilTemp);

  //Coolant temperature
  display->setTextSize(4);
  display->setTextColor(WHITE);
  display->setCursor(107, 220);
  display->printf("%3dC", coolantTemp);

  //Battery Voltage
  display->setTextSize(4);
  display->setTextColor(ORANGE);
  display->setCursor(211, 220);
  display->printf("%4.1fV", battVoltage);

  //Fuel Level Gauge
  drawVerticalBar(376, 130, 52, 142, (int)fuelLevel, GREEN);
  display->setTextSize(2);
  display->setCursor(376, 192);
  if ((int)fuelLevel == 0) {
    display->printf("Null");
  }
  else {
    display->printf("%3d%%", (int)fuelLevel);
  }

  //Accelerator Pedal
  drawVerticalBar(428, 130, 52, 142, gasPedalPos, BLUE);
  display->setTextSize(2);
  display->setCursor(428, 192);
  display->printf("%3d%%", gasPedalPos);

  display->flush();
}

void drawStatus(const char* line1, const char* line2 = nullptr) {
  display->fillScreen(BLACK);
  display->setTextSize(5);
  display->setTextColor(WHITE);
  display->setCursor(0,0);
  display->printf("OBDII-BLE\nTACHOMETER");
  display->setTextSize(4);
  display->setTextColor(WHITE);
  display->setCursor(10, SCREEN_H/2 - 20);
  display->print(line1);
  if (line2) {
    display->setCursor(10, SCREEN_H/2 + 20);
    display->print(line2);
  }
  display->flush();
}

void setup() {
  Serial.begin(115200);
  delay(100);
  pinMode(BL_PIN, OUTPUT); digitalWrite(BL_PIN, HIGH);
  BLEDevice::init("");
  if (!display->begin()) {
    Serial.println("Display init failed");
  }

  drawStatus("Initializing...");

  BLEScan* scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  scan->setActiveScan(true);
  scan->start(5, false);

  drawStatus("Scanning BLE...", nullptr);
}

void loop() {
  if (!connected) {
    if (!doConnect) {
      drawStatus("Scanning BLE...");
    }
    else {
      drawStatus("Connecting...");
      connected = connectToServer();
      if (connected) {
        initELM();
      }
      doConnect = false;
      delay(500);
    }
    delay(100);
    return;
  }

  static unsigned long lastPoll = 0;
  unsigned long now = millis();
  if (now - lastPoll > 200) {
    lastPoll = now;
    requestPID("010C");  // RPM
    requestPID("010D");  // Speed
    requestPID("0105");  // Coolant
    //requestPID("012F");  // Fuel Level
    requestPID("0142");  // Battery Voltage
    requestPID("015C");  // Engine Oil Temp
    requestPID("0149");  // PedalPosition D - Accel Pedal
  }
  drawDashboard();
}
