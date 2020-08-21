![Logo](doc/assets/images/CDP_LOGO_small.png)

[![License](https://img.shields.io/badge/License-Apache2-blue.svg)](https://www.apache.org/licenses/LICENSE-2.0) [![Slack](https://img.shields.io/badge/Join-Slack-blue)](https://www.project-owl.com/slack)

## What is it?
In 2017 a category-5 hurricane, Maria, hit Puerto Rico and wreaked havoc on the island's infrastructure. Communication and power were disabled for an extended period of time causing lasting effects long after the hurricane passed. Many of these issues could have been prevented if civilians had access to a basic network where they could send short messages to emergency services and local governments. 

The ClusterDuck Protocol was created by [Project OWL] to be an easy to use mobile mesh network that can be accessed by people without the need to have specific hardware or pre-downloaded software (such as a mobile app). Since its creation, the vision for the [ClusterDuck Protocol] has grown beyond only servicing people in need after a hurricane towards additional use cases around earthquakes, wildfires, cellular congestion (large events), sensor networks, and more. [Project OWL] open-sourced this project so that the [ClusterDuck Protocol] could reach the communities that need it as fast as possible and continue to explore new directions.

## How does it work?
The network is made up of multiple nodes called **"Ducks"**. There are 3 core roles in a ClusterDuck network: DuckLink, MamaDuck, and PapaDuck. **DuckLinks** serve as edge nodes that only transmit data. These function as remote sensors or as additional access points to a Captive Portal. **MamaDucks** inherit the same functionality of the DuckLinks, but also receive messages. MamaDucks repeat messages from DuckLinks and other MamaDucks until the message reaches a PapaDuck. **PapaDucks** are the endpoint of the network where all of the data is collected and can be stored or pushed up to the cloud. **(free tier cloud platform coming soon)**

![overview](doc/assets/images/DuckExplain.jpg)

## Captive Portal
The Captive Portal is an important feature in the ClusterDuck Protocol network. The Captive Portal allows devices such as smartphones and laptops to access the network without the need to download additional software as it takes advantage a system that is native to smartphones such as Android and iPhone devices and laptops. 

This is beneficial after events such as earthquakes or hurricanes where traditional communication infrastructure is crippled. Users are able to connect to the WiFi access point of a DuckLink or MamaDuck which will in turn relay their message onward.

![portal](doc/assets/images/cluster_demo_vector.gif)


## DetectorDuck
When setting up a network it can be difficult to figure out where to place Duck devices in the field. A DetectorDuck can be used to easily make sure that Ducks are able to maintain connectivity when deploying *ad hoc*. It works by broadcasing a ping every 3 seconds and takes the **RSSI** value of the *pong* of the nearest Duck device. Based on the **RSSI** value, the attached RGB LED will change color between **Blue** (no devices in range), **Green** (great connection), **Purple** (good connection), and **Red** (fair connection).

# Installation
Check out the [ClusterDuck Protocol] website for more information and to learn about projects built upon this codebase. You can reach out directly on our [Slack Workspace] for any questions! 

To use the ClusterDuck Protocol follow the [How To Build a Duck](doc/howToBuildADuck.md) installation guide.

- [Use with PlatformIO](doc/howToBuildADuck.md#PlatformIO)

- [Use with the Arduino IDE](doc/howToBuildADuck.md#Arduino-IDE)

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our Code of Conduct, and the process for submitting ClusterDuck Protocol improvements.

## License

This project is licensed under the Apache 2 License - see the [LICENSE](LICENSE) file for details.

## Version
v1.1.6


[Project OWL]: <https://www.project-owl.com/>
[ClusterDuck Protocol]: <https://clusterduckprotocol.org/>
[Slack Workspace]: <https://www.project-owl.com/slack>

