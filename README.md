![Logo](docs/assets/images/CDP_LOGO_small.png)

[![License](https://img.shields.io/badge/License-Apache2-blue.svg)](https://www.apache.org/licenses/LICENSE-2.0) [![Slack](https://img.shields.io/badge/Join-Slack-blue)](https://www.project-owl.com/slack) [![Wiki](https://img.shields.io/badge/Read-Wiki-blue)](https://github.com/Call-for-Code/ClusterDuck-Protocol/wiki) [![Build Status](https://travis-ci.com/Call-for-Code/ClusterDuck-Protocol.svg?branch=master)](https://app.travis-ci.com/github/Call-for-Code/ClusterDuck-Protocol)

## Why are We Building CDP?
In 2017 a category-5 Atlantic hurricane, Maria, hit Puerto Rico and destroyed most of the island's infrastructure. Cellular and power were disabled for an extended period of time leaving medical and communications problems long after the hurricane passed. If access to a basic communications network was available, the community could have connected with emergency services, local government, or family to mitigate some of the widespread problems resulting from this disaster.

## What is the CDP?
The ClusterDuck Protocol was created by [Project OWL] to be an easy to use mobile ad-hoc mesh network that can be accessed by people without the need to have specific hardware or pre-downloaded software (such as a mobile app). Since its creation, the vision for the [ClusterDuck Protocol] has grown beyond only servicing people in need after a hurricane towards additional use cases around earthquakes, wildfires, cellular congestion (large events), sensor networks, and more. [Project OWL] open-sourced this project so that the [ClusterDuck Protocol] could reach the communities that need it as fast as possible and continue to explore new directions.

## How does it work?
The network is made up of multiple nodes called **"Ducks"**. There are 3 core roles in a ClusterDuck network: DuckLink, MamaDuck, and PapaDuck. **DuckLinks** serve as edge nodes that only transmit data. These function as remote sensors or as additional access points to a Captive Portal. **MamaDucks** inherit the same functionality of the DuckLinks, but also receive messages. MamaDucks repeat messages from DuckLinks and other MamaDucks until the message reaches a PapaDuck. **PapaDucks** are the endpoint of the network where all of the data is collected and can be stored or pushed up to the cloud. **(free tier cloud DMS platform in BETA! Reach out to us on Slack for more info)**

![overview](https://www.project-owl.com/assets/wiki/cdp-explain-gif.gif)

## Installation
Check out the [Wiki](https://github.com/Call-for-Code/ClusterDuck-Protocol/wiki) to learn how to build your own ClusterDuck Protocol Network. And go to the [ClusterDuck Protocol](https://clusterduckprotocol.org/) website for more information and to learn about projects built upon this codebase. You can reach out directly on our [Slack Workspace] for any questions and work with the community. 


**To use the ClusterDuck Protocol follow the [Installation Manual](https://github.com/Call-for-Code/ClusterDuck-Protocol/wiki/getting-started).**


## Testing

From the project root, run the following snippet:

`g++ -g -Wall -DCDP_NO_LOG test_bloomfilter.cpp src/bloomfilter.cpp -o test_bloomfilter && ./test_bloomfilter`

This runs an acceptance test for the bloom filter. 

## How to Contribute

We host a weekly CDP Town Hall every Monday at 2pm EST. The town hall is the place to get updates on protocol, get your questions about CDP answered, and discuss on-going projects. All the current projects is documented in a public [roadmap in Trello](https://trello.com/b/bU0cZuUJ/cdp-roadmap). 

[CDP Town Hall](meet.google.com/unq-duaq-ygj)

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our Code of Conduct, the process for submitting ClusterDuck Protocol improvements, and how to join our town halls and livestreams.

## Contributors
<a href="https://github.com/Call-for-Code/clusterduck-protocol/graphs/contributors">
  <img src="https://contributors-img.web.app/image?repo=Call-for-Code/clusterduck-protocol" />
</a>

## License

This project is licensed under the Apache 2 License - see the [LICENSE](LICENSE) file for details.

## Version

v3.1.5



[Project OWL]: <https://www.project-owl.com/>
[ClusterDuck Protocol]: <https://github.com/Call-for-Code/ClusterDuck-Protocol/wiki>
[Slack Workspace]: <https://www.project-owl.com/slack>

