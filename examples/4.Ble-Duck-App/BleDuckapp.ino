/**
 * @brief Uses the built in Mama Duck with some customatizations.
 * 
 * This example is a Mama Duck, but it is also periodically sending a message in the Mesh
 * It is setup to provide a custom Emergency portal, instead of using the one provided by the SDK.
 * Notice the background color of the captive portal is Black instead of the default Red.
 * 
 * @date 2020-09-21
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <MamaDuck.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif


#include "BluetoothSerial.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

// create a built-in mama duck
MamaDuck duck = MamaDuck();

char message[32]; 
int counter = 1;

void setup() {

  duck.setupSerial();
  duck.setupRadio();

  // We are using a hardcoded device id here, but it should be retrieved or
  // given during the device provisioning then converted to a byte vector to
  // setup the duck NOTE: The Device ID must be exactly 8 bytes otherwise it
  // will get rejected
  std::string deviceId("MAMA0001");
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());
  duck.setDeviceId(devId);

  Serial.println("MAMA-DUCK...READY!");
  SerialBT.begin("MamaDuck1"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

}

void loop() {

  if (SerialBT.available()) {
    String text = SerialBT.readString();
    Serial.println(text);
    duck.sendData(topics::status, text);
    delay(10);
  }

  duck.run();
}

