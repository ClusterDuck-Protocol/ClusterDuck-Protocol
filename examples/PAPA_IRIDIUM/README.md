# Getting Started with RockBlock Modules: 
**Rock Seven provides a web based interface for managing your account which you can access at[ http://www.rock7.com/register](http://www.rock7.com/register)**

Once you've created an account and logged in, you can register your specific Rock BLOCK 9603 modem and enable (purchase) service. [More Info here](https://docs.rockblock.rock7.com/docs/rockblock-management-system)

## How to Enable Service:
**In addition to the modem hardware, you will also need to purchase line rental AND credits to use the service.**

**Line Rental** - Purchased in one month increments, currently £12.00/$13.00 per month. Needed to communicate with the Iridium satellites. Expires after the purchased time has elapsed. This is a fixed base cost.

**Credits** - Needed to send/receive data through the Iridium satellites. These get consumed at a rate of about 1 credit / 50 bytes sent/received. They never expire. Cost varies depending on how many you buy at once, from £0.13/$0.14 to £0.45/$0.49 per credit. This is a variable cost.

You can purchase the above through the web based management system once logged in to your account.

## Setting up Arduino IDE for RockBlock 9603 & RockBlock Mk2:
![](/doc/assets/images/Rockblock_Library.jpg)

1. Open the Arduino IDE to Install the required library in order to communicate with the RockBlock Satellite Modules. 

2. To Install the Iridium Library goto Sketch → Include Library → Manage Libraries. Once you click this you should get a dialog box to search for libraries.  
![](/doc/assets/images/Rockblock_SerialMON.png)
3.  Search for Iridium and Install the Iridium SBD as seen in below screenshot. Once that is installed you will now have access to all the RockBlock example sketches.

* I highly recommend if you are going to be prototyping with this board that you add on female header pins. This will make easy wiring using jumper cables. 

* If you look at the schematic for the Heltec Esp32 + Lora V.2 Board, You will see that you can't use U0TXD & U0RXD because they are used for the programming of the board.

    **The pins that are free and not tied up with anything important on the board are as follows:**
    * GPIO-17  GPIO-2

    * GPIO-22  GPIO-23

    * GPIO-0    GPIO-12

    * GPIO-13  GPIO-25    

Note: **All other spare pins are inputs only, best avoided**.

# Wiring the Two Boards Together to RockBlock 9603:

**When you Purchase a Rockblock you will want to buy this accessory cable to interface with the RockBlock easier**

![](/doc/assets/images/Rockblock_cables.png)


[https://www.sparkfun.com/products/14720](https://www.sparkfun.com/products/14720)

1. Wiring the Heltec Esp32 + Lora V.2 Board → RockBlock

    * Heltec Pin 12 (Txpin) →  Iridium txdata pin
    
    * Heltec Pin 13 (Rxpin) → Iridium rxdata pin

    * Heltec 5V  → Iridium 5v IN

    * Heltec Ground → Iridium Ground

2. Wiring the TTgo T-Beam V.2 Board → RockBlock

    * TTgo T-Beam Pin 25 (Txpin) →  Iridium txdata pin

    * TTgo T-Beam Pin 13 (Rxpin) → Iridium rxdata pin

    * TTgo T-Beam 5V  → Iridium 5v IN

    * TTgo T-Beam Ground → Iridium Ground
    
    
![](/doc/assets/images/ttgo-rockblock.png)

**This is called the Basic TTL Connection**  You can also add in pin sleep and more to optimize the performance and power saving of the RockBlock.

3. Next you will want to open the Arduino IDE backup and look for the Papa Iridium in  Cluster Duck Example Sketches.  

4. The Papa Iridium Example has a proper serial setup compared to standard Iridium Library examples which will not properly work with your Heltec Board or TTgo. So pull the code example down from our examples in our Repo. It has the properly set up serial connection method. 

**The Important Note: Make sure your rxpin and txpin are defined as how you plugged in the module to the Heltec Board** 

5. Once you have properly changed the rxpin and txpin. Plug in your Heltec board or Ttgo and upload this sketch to your board. Once you see that it successfully has uploaded to the Heltec or TTgo open Serial Monitor. **Which can be found here:** Tools → Serial Monitor

6. If the RockBlock is wired properly to your Heltec or TTgo board you should get output saying setting up Modem and see the version print out. 

7. To check if it successfully  sent your message to the Iridium cloud check your account messages at the following link: [https://rockblock.rock7.com/Operations](https://rockblock.rock7.com/Operations)

# Helpful Resources:

* Rock Block 9603 Developer's Guide Which is extremely helpful: [https://cdn.sparkfun.com/assets/6/d/4/c/a/RockBLOCK-9603-Developers-Guide_1.pdf](https://cdn.sparkfun.com/assets/6/d/4/c/a/RockBLOCK-9603-Developers-Guide_1.pdf)

* Dave G6EJD was very helpful and has lots of awesome YouTube tutorial videos along with Github code that helped greatly understanding Serial Connections on Esp32 Boards:[https://github.com/G6EJD/ESP32-Using-Hardware-Serial-Ports](https://github.com/G6EJD/ESP32-Using-Hardware-Serial-Ports)

* Github of the actual Iridium Library create by mikalhart: [https://github.com/mikalhart/IridiumSBD ](https://github.com/mikalhart/IridiumSBD)
