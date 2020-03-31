#include <ClusterDuck.h>

#define SSID        ""
#define PASSWORD    ""

#define ORG         ""                  
#define DEVICE_ID   ""
#define DEVICE_TYPE "PAPA"                
#define TOKEN       ""      

#define SERVER            ORG ".messaging.internetofthings.ibmcloud.com"
#define TOPIC             "iot-2/evt/status/fmt/json"
#define AUTHMETHOD        "use-token-auth"
#define CLIENTID          "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID

PapaDuck duck(SSID, PASSWORD, ORG, DEVICE_ID, DEVICE_TYPE, TOKEN, SERVER, TOPIC, AUTHMETHOD, CLIENTID);

void setup() {
  // put your setup code here, to run once:

  duck.begin();
  duck.setDeviceId("Papa");
  duck.setup();
}

void loop() {
  // put your main code here, to run repeatedly:
  duck.run();
}