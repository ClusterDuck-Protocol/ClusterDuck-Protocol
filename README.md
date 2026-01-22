<p align="center">
  <img src="docs/assets/images/cdp_logo_white_bkg.png" alt="Logo">
</p>

<p align="center">
  <a href="https://www.apache.org/licenses/LICENSE-2.0"><img src="https://img.shields.io/badge/License-Apache2-FFA500.svg" alt="License"></a>
  <a href="https://discord.com/invite/Cbgbzq353z"><img src="https://img.shields.io/badge/Join-Discord-aa80ff" alt="Discord"></a>
  <a href="https://github.com/Call-for-Code/ClusterDuck-Protocol/wiki"><img src="https://img.shields.io/badge/Read-Wiki-50dda0" alt="Wiki"></a>
  <img src="https://img.shields.io/github/v/release/ClusterDuck-Protocol/ClusterDuck-Protocol?label=Release&color=10ccff" alt="GitHub Release">
  <img src="https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol/actions/workflows/arduino_ci.yml/badge.svg" alt="Build Status">
  <a href="https://insights.linuxfoundation.org/project/ClusterDuck"><img src="https://insights.linuxfoundation.org/api/badge/health-score?project=ClusterDuck" alt="LFX Health Score"></a>
  <a href="https://insights.linuxfoundation.org/project/ClusterDuck"><img src="https://insights.linuxfoundation.org/api/badge/contributors?project=ClusterDuck" alt="LFX Contributors"></a>
</p>


# A Flexible Wireless Mesh Protocol for Connecting IoT Devices
The ClusterDuck Protocol (CDP) is a low-bandwidth wireless mesh networking protocol harnessing the low-power capabilities or Long Range Radio (LoRa). It is distinct from LoRaWAN, which uses a star-of-stars topology and has certain restrictions on data flow.

CDP is a project under the Linux Foundation created by [OWL Integrations] (formerly Project OWL) to provide developers with an advanced library for crafting robust distributed communication networks, tailored for a variety of IoT applications and functional in diverse environments.

# Ducks, Explained
Ducks (or DuckLinks) are node firmwares specifically built to work with the ClusterDuck Protocol. They offer a user-friendly way to interact with sensors and hardware to send data across the mesh and store it locally or in the cloud. 

There are 3 main Duck types, PapaDuck, MamaDuck, and DuckLink. PapaDuck is the sink node/backhaul, it offers both relay functionality as well as MQTT integration to help store incoming packet data locally through something like InfluxDB, or in the cloud via OWL DMS (Device Management Software) or AWS solutions. **(free tier cloud DMS platform in BETA! Reach out to us on Discord for more info)**

MamaDuck is the core mesh node. It offers relay and routing capabilities like the Papa, but conserves resources by not needing MQTT or Wifi capabilities by default. 

DuckLinks are a leaf node and offer no relay ability, having minimal routing. Their intention is as pure sensor nodes you can put at the edges of your network where data only needs to flow toward the backhaul. You can create a true mesh by only using Mama and Papa ducks, but have the option to add DuckLinks to create a hybrid mesh.

![overview](https://i.imgur.com/hotfgHr.png)

# Setting Up

Check out the [Wiki](https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol/wiki/getting-started) to learn how to create your Duck Devices and set up a ClusterDuck Network. Feel free to visit the  [ClusterDuck Protocol](https://clusterduckprotocol.org/) website for more information on projects that the community is building. You can reach out directly on our [Discord Community](https://discord.gg/Cbgbzq353z) for any questions and/or to work with the community.


# Basic Usage
## Constructing a Duck
```c++
DuckLink duck("LINK0001"); //Declare the duck type and pass in your Device ID name

MamaDuck duck("MAMA0002"); //By default, DuckLinks and MamaDucks use LoRa radio but do not have wifi
MamaDuck<DuckWifiNone, DuckLora> duck("MAMA0002"); //so these two MamaDuck constructors are equivalent

PapaDuck<DuckWifiNone> duck("PAPA0003"); //by default PapaDucks do have wifi enabled; when using local storage you may want to turn it off

 void setup() {
    duck.setupWithDefaults(); //start the duck with default serial output and parameters for it's radio type
    . . .
 }
 void loop() {
  . . .
   duck.run(); //run duck loop iteration
 }
```

## Sending Messages

Once your duck has booted, it will begin searching for a CDP network to join. After joining or creating a network, your duck will be able to send messages into the mesh.
```c++
std::string msg("I am alive!");
duck.sendData(topics::health, msg); //send a health message relayed to all PapaDucks

uint8_t* data;
duck.sendData(topics::sensor, data, BROADCAST_DUID); //send raw byte sensor data, relayed to all ducks in the network

Duid destination("MAMADEST");
duck.sendData(topics::alert, "power level: LOW", destination); //send an alert message only relayed to and read by MAMADEST
```

For a full list of topics available for use, see [the topics page in the wiki](https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol/wiki/Message-Topics).

## Expanding Duck Capabilities with QuackPacks

QuackPacks are sensor integrations and libraries maintained by [OWL Integrations], allowing you to quickly build out common use cases in the mesh. You can find offically supported QuackPacks in the [Project Owl GitHub](https://github.com/Project-Owl) complete with examples sketches and platformIO settings. 

# How to Contribute

Check the Issues tab and leave a comment to let us know you would like to work on it. Fork the CDP repo and commit your changes to a branch on your fork, then open a PR for us to review! If you find any bugs or want to submit a feature request, open a new Issue for discussion and to provide details or bug reproduction steps. Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our Code of Conduct and the process for submitting ClusterDuck Protocol improvements.

# Contributors

<a href="https://github.com/Call-for-Code/clusterduck-protocol/graphs/contributors">
  <img src="https://contributors-img.web.app/image?repo=Call-for-Code/clusterduck-protocol" />
</a>

# License

This project is licensed under the Apache 2 License - see the [LICENSE](LICENSE) file for details.

[OWL Integrations]: <https://www.owlintegrations.com/>
[ClusterDuck Protocol]: <https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol/wiki>
[Discord]: <https://discord.com/invite/Cbgbzq353z>

This project is governed by its [Technical Charter](ClusterDuck-Protocol-Technical-Charter.pdf)
