![Logo](doc/assets/images/CDP_LOGO_small.png)

[![License](https://img.shields.io/badge/License-Apache2-blue.svg)](https://www.apache.org/licenses/LICENSE-2.0) [![Slack](https://img.shields.io/badge/Join-Slack-blue)](https://www.project-owl.com/slack)

## What is it?
In 2017 a category-5 hurricane, Maria, hit Puerto Rico and wreaked havoc on the island's infrastructure. Communication and power were disabled for an extended period of time causing lasting effects long after the hurricane passed. Many of these issues could have been prevented if civilians had access to a system where they could send short messages to emergency services and local governments. 

The ClusterDuck Protocol was created by [Project OWL] to be an easy to use mobile mesh network that can be accessed by people without the need to have specific hardware or pre-downloaded software (such as a mobile app). Since its creation, the vision for the [ClusterDuck Protocol] has grown beyond only servicing people in need after a hurricane to earthquakes, wildfires, cellular congestion (large events), sensor networks, and more. [Project OWL] open-sourced this project so that the [ClusterDuck Protocol] could reach the communities that need it as fast as possible and continue to explore new directions.

## How does it work?
The network is made  up of multiple nodes called **"Ducks"**. There are 3 core roles in a ClusterDuck network: DuckLink, MamaDuck, and PapaDuck. **DuckLinks** serve as edge nodes that only transmit data. These function as remote sensors or as additional access points to a Captive Portal. **MamaDucks** inherit the same functionality of the DuckLinks, but also receive messages. MamaDucks repeat messages from DuckLinks and other MamaDucks until the message reaches a PapaDuck. **PapaDucks** are the endpoint of the network where all of the data is collected and can be stored or pushed up to the cloud. **(free tier cloud platform coming soon)**

![overview](doc/assets/images/DuckExplain.jpg)

## Captive Portal
The Captive Portal is an important feature in the ClusterDuck Protocol network. The Captive Portal allows devices such as smartphones and laptops to access the network without the need to download additional software as it takes advantage a system that is native to smartphones such as Android and iPhone devices. 

This is beneficial after events such as earthquakes or hurricanes where communication infrastructure is crippled. Survivors are able to connect and send messages just by connecting to the WiFi access point of a DuckLink or MamaDuck.

![portal](doc/assets/images/cluster_demo_vector.gif)


# Installation
Check out the [ClusterDuck Protocol] website for more information and to learn about projects built upon this codebase. You can reach out directly on our [Slack] too for any questions! 

To use the ClusterDuck Protocol follow the [How To Build a Duck](doc/howToBuildADuck.md) installation guide.

- [Use with platformIO](doc/howToBuildADuck.md#PlatformIO)

- [Use with the Arduino IDE](doc/howToBuildADuck.md#Arduino-IDE)


## Quick Start

Open new sketch in Arduino IDE or create a new project with platformIO and include the ClusterDuck library

    #include "ClusterDuck.h"
Create ClusterDuck object

    ClusterDuck duck;
Initializes the ClusterDuck class object

#### In setup()

    duck.begin(baudRate);
Initializes the baud rate for serial printing and messaging. You can adust to your desired baud rate.
- int baudRate
-- Default is **115200**

Set device ID and captive portal form length.
    
    duck.setDeviceId(String deviceId, const int formLength);
- String deviceId
-- input the device ID used to identify your registered device on the web
-- do not leave null or an empty string
- const int formLength 
-- (optional) define the number of captive portal form fields
-- Default is **10** to match our default captive portal template

Setup **DuckLink**

    duck.setupDuckLink();
    
``duck.setupMamaDuck`` can also be used here to setup a MamaDuck, however you **cannot** use both in the same sketch.

#### In loop()

Add corresponding Duck run code. Must be of the same device type as used in ``setup()``. (e.g. if ``duck.setupMamaDuck()`` is used in ``setup()`` use ``duck.runMamaDuck()``)

    duck.runDuckLink();
    
Your sketch should look something like this:
    
    #include "ClusterDuck.h"
    ClusterDuck duck;
    
    void setup() {
    // put your setup code here, to run once:
        duck.begin();
        duck.setDeviceId("Z", 10);
        duck.setupDuckLink();
    }

    void loop() {
    // put your main code here, to run repeatedly:
        duck.runDuckLink();
    }

Now compile and upload to your device. If using a Heltec LoRa ESP32 board you should see a Duck Online message on the LED screen. You can now open your phone or laptop's Wi-Fi preferences and connect to the ``SOS DuckLink Network``! 

If you don't see the captive portal screen, you can force it by accessing [neverssl.com](http://neverssl.com) which will force the captive portal to intercept the HTTP request.



## API
``setDeviceId(String deviceId)``
- Set device ID that will be used to prevent DDoS (Duck Denial of Service or Duck DoS). Do not leave **deviceId** *null* or as an empty string. Use in ``setup()``.

``void begin(int baudRate)``
- Initialize baud rate for serial. Use in ``setup()``.

``void setupDisplay(String deviceType)``
- Initializes LED screen on Heltec LoRa ESP32 and configures it to show status, device ID, and the device type. Use in ``setup()``.

``void setupLoRa(long BAND, int SS, int RST, int DI0, int TxPower)``
- Initializes LoRa radio. If using **Heltec LoRa ESP32** set **SS** to , **RST** to and **DIO** to . **TxPower** corresponds to the the transmit power of the radio (max value: 20). Use in ``setup()``.

``void setupPortal(const char *AP)``
- Initializes the captive portal code. ***AP** is the value that will be displayed when accessing the wifi settings of devices such as smartphones and laptops. Use in ``setup()``.

``void processPortalRequest()``
- Handles incoming and active connections to the captive portal. **Required** for runing captive portal. Use in ``loop()``.

``void setupDuckLink()``
- Template for setting up a **DuckLink** device. Use in ``setup()``

``void runDuckLink()``
- Template for running core functionality of a **DuckLink**. Use in ``loop()``.

``void setupMamaDuck()``
- Template for setting up a **MamaDuck** device. Use in ``setup()``.

``void runMamaDuck()``
- Template for running core functionality of a **MamaDuck**. Use in ``loop()``.

``String * getPortalDataArray()``
- Returns webserver arguments based on **formLength** as an array of ``Strings``.

``String getPortalDataString()``
- Returns webserver arguments based on **formLength** as a single String with arguments separated by *

``void sendPayloadMessage(String msg)``
- Packages **msg** into a LoRa packet and sends over LoRa. Will automatically set the current device's ID as the sender ID and create a UUID for the message.

``void sendPayloadStandard(String msg, String senderId = "", String messageId = "", String path = "")`` 
- Similar to and might replace ``sendPayloadMessage()``. **senderId** is the ID of the originator of the message. **messageId** is the UUID of the message. **ms** is the message payload to be sent. **path** is the recorded pathway of the message and is used as a check to prevent the device from sending multiple of the same message.

``void couple(byte byteCode, String outgoing)``
- Writes data to LoRa packet. **outgoing** is the payload data to be sent. **byteCode** is paired with the **outgoing** so it can be used to identify data on an individual level. Reference ``setDeviceId()`` for byte codes. In addition it writes the **outgoing** length to the LoRa packet. 
- Use between a ``LoRa.beginPacket()`` and ``LoRa.endPacket()`` (**note:** ``LoRa.endPacket()`` will send the LoRa packet)

``String readMessages(byte mLength)``
- Returns a String. Used after ``LoRa.read()`` to convert LoRa packet into a String.

``bool idInPath(String path)``
- Checks if the **path** contains **deviceId**. Returns bool.

``String * getPacketData(int pSize)``
- Called to iterate through received LoRa packet and return data as an array of Strings.
- Note: if using standard byte codes it will store **senderId**, **messageId**, **payload**, and **path** in a Packet object. This can be accessed using ``getLastPacket()``

``void restartDuck()``
- If using the ESP32 architecture, calling this function will reboot the device.

``void reboot(void *)``
- Used to call ``restartDuck()`` when using a timer

``void imAlive(void *)``
- Used to send a '1' over LoRa on a timer to signify the device is still on and functional.

``String duckMac(boolean format)``
- Returns the MAC address of the device. Using ``true`` as an argument will return the MAC address formatted using ':'

``String uuidCreator()``
- Returns as String of 8 random characters and numbers

``String getDeviceId()``
- Returns the device ID

``Packet getLastPacket()``
- Returns a Packet object containing **senderId**, **messageId**, **payload**, and **path** of last packet received.
- Note: values are updated after running ``getPacketData()``

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our Code of Conduct, and the process for submitting ClusterDuck Protocol improvements.

## License

This project is licensed under the Apache 2 License - see the [LICENSE](LICENSE) file for details.

## Version
v1.0.6


[Project OWL]: <https://www.project-owl.com/>
[ClusterDuck Protocol]: <http://clusterduckprotocol.org>
[Slack Workspace]: <https://www.project-owl.com/slack>

