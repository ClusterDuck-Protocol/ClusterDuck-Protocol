/**
 * @file mamaduck-send-message.ino
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


// Set device ID between ""
MamaDuck duck = MamaDuck("DuckOne");

char message[32]; 
int counter = 1;

void setup() {

  duck.setupSerial();
  duck.setupRadio();

 
  Serial.println("MAMA-DUCK...READY!");


  SerialBT.begin("DuckLink1"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

}

void loop() {

  if (SerialBT.available()) {
    String text = SerialBT.readString();
    Serial.println(text);
    duck.sendPayloadStandard(text, "app");
    delay(10);
  }

  duck.run();
}

