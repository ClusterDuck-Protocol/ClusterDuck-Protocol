# The ClusterDuck Protocol
#### What is it?
The ClusterDuck Protocol is a mesh communication protocol based on LoRa. It is meant to be easy to implement and expand the mesh network. Currently it supports hardware architectures that use **ESP32** and **SX1276** chips.

[![License](https://img.shields.io/badge/License-Apache2-blue.svg)](https://www.apache.org/licenses/LICENSE-2.0) [![Slack](https://img.shields.io/badge/Join-Slack-blue)](https://www.project-owl.com/slack)


#### How does it work?
The network is made up of multiple nodes called **"Ducks"**. There are 3 core roles in the ClusterDuck network: DuckLink, MamaDuck, and PapaDuck. **DuckLinks** serve as edge nodes that only transmit data. These function as remote sensors or as additional access points to a Captive Portal. **MamaDucks** inherit the same functionality of the DuckLinks, but also receive messages. MamaDucks repeat messages from DuckLinks and other MamaDucks until the message reaches a PapaDuck. **PapaDucks** are the endpoint of the network where all of the data is collected and can be stored or pushed up to the cloud. **(free tier cloud platform coming soon)**

![overview](https://media.wired.com/photos/5bd3991147d18c4c60798e17/master/w_1616,c_limit/ProjectOwl-Inline_01.jpg)

#### Captive Portal
The Captive Portal is an important feature in the ClusterDuck Protocol. The Captive Portal allows for devices such as smartphones and laptops to access the network without the need of downloading additional software. This is beneficial after events such as earthquakes or hurricanes where communication infrastructure becomes crippled. Survivors are able to connect and send messages just by connecting to the WiFi access point of a DuckLink or MamaDuck.

![portal](https://github.com/knouse1344/owl/blob/master/app/assets/images/cluster_demo_vector.gif)

# API and Getting Started
Check out the [ClusterDuck Protocol] website for more information and projects.
### Downloading Library from GitHub
For ease of use, we will need to copy the library into your Arduino library folder.
1. Copy ClusterDuck folder
2. Navigate to your ``Arduino`` folder. This can be found in your default Documents folder.
3. Navigate to the ``library`` folder
4. Paste into ``library`` folder
5. Restart Arduino
6. You should now be able to see examples by going to File -> Examples -> ClusterDuck

You should be able pull new commits directly to this folder in your Arduino library.

### Quick Start
Open new sketch in Adruino IDE and include the ClusterDuck library

    #include "ClusterDuck.h"
Create ClusterDuck object

    ClusterDuck duck;
Initializes the ClusterDuck class object

##### In setup()

    duck.begin(baudRate);
Initializes the baude rate for serial printing and messaging. You can adust to your desired baude rate.
- int baudRate
-- Default is **115000**

Set device ID and captive portal form length.
    
    duck.setDeviceId(String deviceId, const int formLength);
- String deviceId
-- input the device ID used to identify your registered device on the web
-- do not leave null or empty string
- const int formLength 
-- (optional) define the number of captive portal form fields
-- Default is **10** to match our default captive portal template

Setup **DuckLink**

    duck.setupDuckLink();
    
``duck.setupMamaDuck`` can also be used here to setup a MamaDuck, however you **cannot** use both in the same sketch.

#### In loop()

Add cooresponding Duck run code. Must be of the same device type as used in ``setup()``. (e.g. if ``duck.setupMamaDuck()`` is used in ``setup()`` use ``duck.runMamaDuck()``)

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

Now compile and upload to your device. If using a Heltec LoRa ESP32 board you should see a Duck Online message on the LED screen. You can now open your phone or laptop's wifi preferences and connect to the ``SOS DuckLink Network``!

## API
``setDeviceId(String deviceId, const int formLength)``
- Set device ID and captive portal form length. Do not leave **deviceId** *null* or as an empty string. **formLength** defaults to 10. Use in ``setup()``.

``void begin(int baudRate)``
- Initialize baude rate for serial. Use in ``setup()``.

``void setupDisplay(String deviceType)``
- Initializes LED screen on Heltec LoRa ESP32 and configures it to show status, device ID, and the device type. Use in ``setup()``.

``void setupLoRa(long BAND, int SS, int RST, int DI0, int TxPower)``
- Initializes LoRa radio. If using **Heltec LoRa ESP32** set **SS** to , **RST** to and **DIO** to . **TxPower** corresponds to the the transmit power of the radio (max value: 20). Use in ``setup()``.

``void setupPortal(const char *AP)``
- Initializes the captive portal code. ***AP** is the value that will be displayed when accessing the wifi settings of devices such as smartphones and laptops. Use in ``setup()``.

``bool runCaptivePortal()``
- Processes requests coming through the captive portal. Returns ``true`` if there is a new submission. Use this in your ``loop()`` function to run continuously. 

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
- Note: if using standerd byte codes it will store **senderId**, **messageId**, **payload**, and **path** in a Packet object. This can be accessed using ``getLastPacket()``

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




[Project OWL]: <https://project-owl.com>
[ClusterDuck Protocol]: <http://clusterduckprotocol.org>
