#pragma once
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
static BLEServer* pServer = NULL;
static BLECharacteristic* pChar = NULL;
static BLECharacteristic* pCharOTA = NULL;
static BLECharacteristic* pCharNotify = NULL;
static bool deviceConnected = false;
static std::string value = "NA";


struct ParsedCommand
{
  String command;
  int arg1;
  int arg2;
  int arg3;
  int arg4;
  bool hasArg2;
  bool hasArg3;
  bool hasArg4;
  bool valid;
} ParsedBTCommand;

ParsedCommand parse_command(String input)
{
  ParsedCommand result = {"", 0, 0, 0, 0, false, false, false, false};

  // Basic format check
  if (input.length() < 5 || input[0] != '#' || input[input.length() - 1] != '*')
    return result;

  // Remove '#' and '*'
  input = input.substring(1, input.length() - 1);

  int firstColon = input.indexOf(':');
  if (firstColon == -1)
    return result;

  result.command = input.substring(0, firstColon);

  // Split arguments by colon
  String args = input.substring(firstColon + 1);
  int argIdx = 0;
  int lastPos = 0;
  int nextPos = 0;

  while ((nextPos = args.indexOf(':', lastPos)) != -1 && argIdx < 4)
  {
    int value = args.substring(lastPos, nextPos).toInt();
    if (argIdx == 0) { result.arg1 = value; result.hasArg2 = true; }
    else if (argIdx == 1) { result.arg2 = value; result.hasArg3 = true; }
    else if (argIdx == 2) { result.arg3 = value; result.hasArg4 = true; }
    lastPos = nextPos + 1;
    argIdx++;
  }
  // Last argument (or only argument)
  if (lastPos < args.length() && argIdx < 4)
  {
    int value = args.substring(lastPos).toInt();
    if (argIdx == 0) { result.arg1 = value; }
    else if (argIdx == 1) { result.arg2 = value; }
    else if (argIdx == 2) { result.arg3 = value; }
    else if (argIdx == 3) { result.arg4 = value; }
  }

  result.valid = true;
  return result;
}


void stopBLE(){ // Stop BLE 
  if(!BLE_ENABLED) return;

  if (pServer) pServer->disconnect(0); // Disconnect client (if any)
  BLEDevice::stopAdvertising();
  BLEDevice::deinit(true);
  // deviceConnected = false;
  // BLEDevice::getAdvertising()->stop();
  // deviceConnected = false;
  delay(1000); // Give some time to disconnect
} 

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer*) override { deviceConnected = true; DEBUG_SERIAL.println("Device Connected"); }
    void onDisconnect(BLEServer* pServer) override {
      deviceConnected = false;
      pServer->startAdvertising();
      DEBUG_SERIAL.println("Device Disconnected");
    }
};

class CharacteristicCallBack: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) override { 
    value = pChar->getValue();
    // DEBUG_SERIAL.println(value.c_str());
    ParsedBTCommand = parse_command(value.c_str());
    if (ParsedBTCommand.valid) {
      cmdInterrupt = true;
    }
  }
  void onRead(BLECharacteristic *pChar) override {
    pChar->setValue(value);
  }
};

class OTACharacteristicCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) override {
    String creds = String(pChar->getValue().c_str());
    int sep = creds.indexOf(':');
    if (sep > 0) {
      DEBUG_SERIAL.printf("Received WiFi credentials via BLE - SSID:PASS: %s \n", creds.c_str());
      // update the credentials
      WIFI_SSID = creds.substring(0, sep).c_str();
      WIFI_PASS = creds.substring(sep + 1).c_str();
      CurrentOTAStatus = OTAGit; // OTA Start
      // if(connectToWiFi(creds.substring(0, sep).c_str(), creds.substring(sep + 1).c_str())) {
      //   DEBUG_SERIAL.println("Connected to WiFi via BLE credentials");
      // } else {
      //   DEBUG_SERIAL.println("Failed to connect to WiFi via BLE credentials");
      // }
    } else {
      DEBUG_SERIAL.println("Invalid WiFi credentials format");
    }
  }
  void onRead(BLECharacteristic *pChar) override {
    // pChar->setValue(CurrentOTAStatus); // Return OTA progress status
    std::string statusStr;
    switch (CurrentOTAStatus) {
      case NoOTA: statusStr = "NoOTA"; break;
      case OTAGit: statusStr = "OTAGit"; break;
      case OTALocal: statusStr = "OTALocal"; break;
      case OTACompleted: statusStr = "OTACompleted"; break;
      case OTAInProgress: statusStr = "OTAInProgress"; break;
      case OTAFailed: statusStr = "OTAFailed"; break;
    }
    pChar->setValue(statusStr);
  }
};


void BLEsetup() {
  #ifndef BLE
  return;
  #endif

  // Create the BLE Device
  BLEDevice::init(device_name);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pChar = pService->createCharacteristic(
                      CHAR1_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  
                    );   
  // Create a BLE Characteristic for OTA
  pCharOTA = pService->createCharacteristic(
                      CHAR_OTA_UUID,
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_READ
                    );                

  // Create a BLE Characteristic for Notifications
  pCharNotify = pService->createCharacteristic(
                      CHAR_NOTIFY_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharNotify->addDescriptor(new BLE2902()); 
  
  // // After defining the desriptors, set the callback functions
  pChar->setCallbacks(new CharacteristicCallBack());
  pCharOTA->setCallbacks(new OTACharacteristicCallback());

  pService->start();

  auto* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLEUUID(SERVICE_UUID));
  pAdvertising->setScanResponse(false);
  BLEDevice::startAdvertising();
}

// void BLENotify(std::string notifyValue) {
//   if(!BLE_ENABLED) return;
//   if (deviceConnected) {
//     pCharNotify->setValue(notifyValue);
//     pCharNotify->notify();
//     DEBUG_SERIAL.printf("Notified value: %s\n", notifyValue.c_str());
//   }
// }