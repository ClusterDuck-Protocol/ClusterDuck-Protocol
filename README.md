![Logo](https://www.project-owl.com/assets/CDP_LOGO.png)

[![License](https://img.shields.io/badge/License-Apache2-blue.svg)](https://www.apache.org/licenses/LICENSE-2.0) [![Slack](https://img.shields.io/badge/Join-Slack-blue)](https://www.project-owl.com/slack)

## What is it?
In 2017 a category-5 hurricane, Maria, hit Puerto Rico and wreaked havoc on the island's infrastructure. Communication and power were disabled for an extended period of time causing lasting effects long after the hurricane passed. Many of these issues could have been prevented if civilians had access to a system where they could send short messages to emergency services and local governments. 

The ClusterDuck Protocol was created by [Project OWL] to be an easy to use mobile mesh network that can be accessed by people without the need to have specific hardware or pre-downloaded software (such as a mobile app). Since its creation, the vision for the [ClusterDuck Protocol] has grown beyond only servicing people in need after a hurricane to earthquakes, wildfires, cellular congestion (large events), sensor networks, and more. [Project OWL] open-sourced this project so that the [ClusterDuck Protocol] could reach the communities that need it as fast as possible and continue to explore new directions.

## How does it work?
The network is made up of multiple nodes called **"Ducks"**. There are 3 core roles in a ClusterDuck network: DuckLink, MamaDuck, and PapaDuck. **DuckLinks** serve as edge nodes that only transmit data. These function as remote sensors or as additional access points to a Captive Portal. **MamaDucks** inherit the same functionality of the DuckLinks, but also receive messages. MamaDucks repeat messages from DuckLinks and other MamaDucks until the message reaches a PapaDuck. **PapaDucks** are the endpoint of the network where all of the data is collected and can be stored or pushed up to the cloud. **(free tier cloud platform coming soon)**

![overview](https://www.project-owl.com/assets/DuckExplain.jpg)

## Captive Portal
The Captive Portal is an important feature in the ClusterDuck Protocol network. The Captive Portal allows devices such as smartphones and laptops to access the network without the need to download additional software as it takes advantage a system that is native to smartphones such as Android and iPhone devices. 

This is beneficial after events such as earthquakes or hurricanes where communication infrastructure is crippled. Survivors are able to connect and send messages just by connecting to the WiFi access point of a DuckLink or MamaDuck.

![portal](https://github.com/knouse1344/owl/blob/master/app/assets/images/cluster_demo_vector.gif)

# API and Getting Started
Check out the [ClusterDuck Protocol] website for more information and to learn about projects built upon this codebase. You can reach out directly on our [Slack] too! Check out our [How To Build A Duck User Manual] for in depth instructions for setting up your environment and materials. 

## PlatformIO
[PlatformIO](https://platformio.org/) is a ecosystem for embedded development. Grab your favorite IDE from [here](https://platformio.org/install/integration) and make sure to install the platformIO ide extension. If you are not sure which IDE to use, use [VSCode](https://docs.platformio.org/en/latest/integration/ide/vscode.html#installation).

### Installing ClusterDuck-Protocol globally

1. From the [PIO Home](https://docs.platformio.org/en/latest/integration/ide/vscode.html#setting-up-the-project) tab select `Libraries`
1. Search for `ClusterDuck Protocol` and install it

### Installing ClusterDuck-Protocol for project only

1. Create a `New Project` from the [PIO Home](https://docs.platformio.org/en/latest/integration/ide/vscode.html#setting-up-the-project) tab
1. Choose a name, select the `Heltec Wifi LoRa 32 (V2) (Heltec Automation)` as board and `Arduino` as framework
1. Open the `platformio.ini` within your newly created project and add `lib_deps = ClusterDuck Protocol` at the end.

## Arduino 
### Downloading Library from GitHub

#### Manual Install 
To start we will need to copy the library into your Arduino library folder.
1. Copy ClusterDuck folder
1. Navigate to your ``Arduino`` folder. This can be found in your default Documents folder.
1. Navigate to the ``library`` folder
1. Paste into ``library`` folder
1. Restart Arduino
1. You should now be able to see examples by going to File -> Examples -> ClusterDuck

You should be able pull new commits directly to this folder in your Arduino library.

#### Importing as a .zip Library
Download The ClusterDuck Protocol as a .Zip file.

In the Arduino IDE, navigate to ``Sketch > Include Library > Add .ZIP Library.`` At the top of
the drop down list, select the option to "Add .ZIP Library''.

Navigate to the downloaded ClusterDuck Protocol Folder and select.
Return to the Sketch > Include Library menu. menu. You should now see the library at the
bottom of the drop-down menu. It is ready to be used in your sketch. 

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

## Setting up the IBM Watson IoT Platform

1. Create an IBM Cloud account through [this link](https://ibm.biz/BdqiVW). Fill out all the required information and confirm your email address.
1. Follow this link to provision an instance of the [IBM Watson IoT Platform](https://cloud.ibm.com/catalog/services/internet-of-things-platform). Note: you can also find this by [browsing through the catalog](https://cloud.ibm.com/catalog).
1. Make sure the Lite plan is selected and click `Create`.  You can change the `Service Name` if you want to, but it's not required.
![iot-platform-3](https://github.com/Code-and-Response/ClusterDuck-Protocol/blob/master/assets/images/iot-platform-3.png)
1. After the service provisions, click `Launch`.
![iot-platform-4](https://github.com/Code-and-Response/ClusterDuck-Protocol/blob/master/assets/images/iot-platform-4.png)
1. Click `Add Device`.
![iot-platform-5](https://github.com/Code-and-Response/ClusterDuck-Protocol/blob/master/assets/images/iot-platform-5.png)
1. Enter your `Device Type` and `Device ID`, then click `Next`.
![iot-platform-6](https://github.com/Code-and-Response/ClusterDuck-Protocol/blob/master/assets/images/iot-platform-6.png)
1. Filling out anything in the `Device Information` tab is optional, click `Next`.
![iot-platform-7](https://github.com/Code-and-Response/ClusterDuck-Protocol/blob/master/assets/images/iot-platform-7.png)
1. Leave the field for `Authentication Token` blank, as one will be generated automatically. You can specify your own if you prefer. Click `Next`.
![iot-platform-8](https://github.com/Code-and-Response/ClusterDuck-Protocol/blob/master/assets/images/iot-platform-8.png)
1. Ensure that the `Summary` page looks good, then click `Finish`.
![iot-platform-9](https://github.com/Code-and-Response/ClusterDuck-Protocol/blob/master/assets/images/iot-platform-9.png)
1. You'll see the authentication token listed, ensure that you do not misplace it, otherwise, you will have to regenerate a new token.
![iot-platform-10](https://github.com/Code-and-Response/ClusterDuck-Protocol/blob/master/assets/images/iot-platform-10.png)
1. Open `PapaDuckExample.ino` and replace `ORG`, `DEVICE_ID`, `DEVICE_TYPE`, and `TOKEN` with the information from the summary screen. Make sure you also have the correct WiFi SSID and password filled out in that file as well. After you flash the duck with this information, you'll see data flowing through the IBM Watson IoT Platform.
![iot-platform-11](https://github.com/Code-and-Response/ClusterDuck-Protocol/blob/master/assets/images/iot-platform-11.png)

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
v1.0.4


[Project OWL]: <https://project-owl.com>
[ClusterDuck Protocol]: <http://clusterduckprotocol.org>
[Slack]: <https://www.project-owl.com/slack>
[How To Build A Duck User Manual]: <https://docs.google.com/document/d/1HNsU7lN5gbZgcciP0BSyMYGDVNDM6RYfsXgdpwo66YE/edit?usp=sharing>
