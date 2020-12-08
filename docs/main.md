

## ClusterDuck Protocol Info

## About
The ClusterDuck Protocol is an open source firmware for mesh network internet-of-things devices based on LoRa radio and can include WiFi and Bluetooth compatibility. This protocol enables quick and easy set up of wireless communications, sensor, and other network devices.


## What is it?
In 2017 a category-5 hurricane, Maria, hit Puerto Rico and wreaked havoc on the island's infrastructure. Communication and power were disabled for an extended period of time causing lasting effects long after the hurricane passed. Many of these issues could have been prevented if civilians had access to a basic network where they could send short messages to emergency services and local governments. 

The ClusterDuck Protocol was created by [Project OWL] to be an easy to use mobile mesh network that can be accessed by people without the need to have specific hardware or pre-downloaded software (such as a mobile app). Since its creation, the vision for the [ClusterDuck Protocol] has grown beyond only servicing people in need after a hurricane towards additional use cases around earthquakes, wildfires, cellular congestion (large events), sensor networks, and more. [Project OWL] open-sourced this project so that the [ClusterDuck Protocol] could reach the communities that need it as fast as possible and continue to explore new directions.

## How does it work?
The network is made up of multiple nodes called **Ducks**. There are 3 core roles in a ClusterDuck network: DuckLink, MamaDuck, and PapaDuck. **DuckLinks** serve as edge nodes that only transmit data. These function as remote sensors or as additional access points to a Captive Portal. **MamaDucks** inherit the same functionality of the DuckLinks, but also receive messages. MamaDucks repeat messages from DuckLinks and other MamaDucks until the message reaches a PapaDuck. **PapaDucks** are the endpoint of the network where all of the data is collected and can be stored or pushed up to the cloud. **(free tier cloud DMS platform in BETA! Reach out to us on Slack for more info)**


---------------------------

[Github](https://github.com/Call-for-Code/ClusterDuck-Protocol)

[Github Wiki ](https://github.com/Call-for-Code/ClusterDuck-Protocol/wiki)

[Project Page](https://clusterduckprotocol.org/)


[Project OWL]: <https://www.project-owl.com/>
[ClusterDuck Protocol]: <https://github.com/Call-for-Code/ClusterDuck-Protocol/wiki>
[Slack Workspace]: <https://www.project-owl.com/slack>