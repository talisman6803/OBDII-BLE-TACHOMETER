#include "BLEManager.h"
#include "OBDParser.h"

#define SVC_UUID     "0000fff0-0000-1000-8000-00805f9b34fb"
#define CHAR_UUID_TX "0000fff2-0000-1000-8000-00805f9b34fb"
#define CHAR_UUID_RX "0000fff1-0000-1000-8000-00805f9b34fb"

static BLEAddress      *pServerAddress;
static bool             doConnect = false;
static bool             connected = false;
static BLERemoteCharacteristic* pWriteChar;
static BLERemoteCharacteristic* pReadChar;
static String           bleBuf;

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
    if (line.length()) parseOBDLine(line);
  }
}

static bool connectToServer() {
  BLEClient* pClient = BLEDevice::createClient();
  if (!pClient->connect(*pServerAddress)) return false;
  auto svc = pClient->getService(SVC_UUID);
  if (!svc) { pClient->disconnect(); return false; }
  pWriteChar = svc->getCharacteristic(CHAR_UUID_TX);
  pReadChar  = svc->getCharacteristic(CHAR_UUID_RX);
  if (!pWriteChar || !pReadChar || !pReadChar->canNotify()) {
    pClient->disconnect(); return false;
  }
  pReadChar->registerForNotify(notifyCallback);
  return true;
}

void initELM() {
  static const char* cmds[] = {
    "ATZ","ATE0","ATL1","ATH1","AT CAF1","AT SP 6"
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

void initBLE() {
  BLEDevice::init("");
  auto scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  scan->setActiveScan(true);
  scan->start(5, false);
}

bool isBLEConnected() {
  if (connected) return true;
  if (doConnect) {
    connected = connectToServer();
    if (connected) initELM();
    doConnect = false;
  }
  return connected;
}

void pollOBD() {
  static unsigned long lastPoll = 0;
  unsigned long now = millis();
  if (now - lastPoll < 200) return;
  lastPoll = now;
  requestPID("010C");
  requestPID("010D");
  requestPID("0105");
  requestPID("0142");
  requestPID("015C");
  requestPID("0149");
}
