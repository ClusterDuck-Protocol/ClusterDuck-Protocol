#include "ClusterDuck.h"
#include "BluetoothSerial.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

ClusterDuck duck;

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

void setup() {
  duck.begin();
  duck.setDeviceId("ble");
  duck.setupLoRa();
  duck.setupDisplay("BLE");

  SerialBT.begin("DuckLink1"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

}

void loop() {
  // put your main code here, to run repeatedly:

  if (SerialBT.available()) {
    String text = SerialBT.readString();
    Serial.println(text);
    duck.sendPayloadStandard(text, "app");
    delay(10);
  }
}