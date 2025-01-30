<p align="center">
  <img src="docs/assets/images/cdp_logo_white_bkg.png" alt="Logo">
</p>

<p align="center">
  <a href="https://www.apache.org/licenses/LICENSE-2.0"><img src="https://img.shields.io/badge/License-Apache2-FFA500.svg" alt="License"></a>
  <a href="https://discord.com/invite/Cbgbzq353z"><img src="https://img.shields.io/badge/Join-Discord-aa80ff" alt="Discord"></a>
  <a href="https://github.com/Call-for-Code/ClusterDuck-Protocol/wiki"><img src="https://img.shields.io/badge/Read-Wiki-50dda0" alt="Wiki"></a>
  <img src="https://img.shields.io/github/v/release/ClusterDuck-Protocol/ClusterDuck-Protocol?label=Release&color=10ccff" alt="GitHub Release">
  <img src="https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol/actions/workflows/arduino_ci.yml/badge.svg" alt="Build Status">
</p>



## A Flexible Wireless Mesh Protocol for Connecting IoT Devices
The ClusterDuck Protocol (CDP) is a low-bandwidth wireless mesh networking protocol harnessing the low-power capabilities or Long Range Radio (LoRa). It is distinct from LoRaWAN, which uses a star-of-stars topology and has certain restrictions on data flow.

CDP is a project under the Linux Foundation created by [OWL Integrations] (formerly Project OWL) to provide developers with an advanced library for crafting robust distributed communication networks, tailored for a variety of IoT applications and functional in diverse environments.

## How does it work?

The network is made up of multiple nodes called **Ducks**. There are 3 basic types of Ducks in a ClusterDuck Network: DuckLink, MamaDuck, and PapaDuck. **DuckLinks** serve as leaf nodes that only transmit data. **MamaDucks** are relay and transmission nodes, creating the core mesh functionality. **PapaDucks** are sink nodes that can also serve as gateways, so you can either gather data locally or publish over wifi to cloud solutions. **(free tier cloud DMS platform in BETA! Reach out to us on Discord for more info)**

![overview](https://i.imgur.com/hotfgHr.png)

## Getting Started

Check out the [Wiki](https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol/wiki) to learn how to build your own ClusterDuck Network. Feel free to visit the  [ClusterDuck Protocol](https://clusterduckprotocol.org/) website for more information on projects that the community is building. You can reach out directly on our [Discord Community](https://discord.gg/Cbgbzq353z) for any questions and/or to work with the community.

**To use the ClusterDuck Protocol follow the [Installation Manual](https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol/wiki/getting-started).**


## How to Contribute

Development tasks are tracked publicly on our [Trello](https://trello.com/b/bU0cZuUJ/cdp-roadmap). Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our Code of Conduct and the process for submitting ClusterDuck Protocol improvements.

This project is governed by its [Technical Charter](ClusterDuck-Protocol-Technical-Charter.pdf) and led by its [Technical Steering Committee](https://github.com/Call-for-Code/ClusterDuck-Protocol/wiki/technical-steering-committee).

## Contributors

<a href="https://github.com/Call-for-Code/clusterduck-protocol/graphs/contributors">
  <img src="https://contributors-img.web.app/image?repo=Call-for-Code/clusterduck-protocol" />
</a>

## License

This project is licensed under the Apache 2 License - see the [LICENSE](LICENSE) file for details.

[OWL Integrations]: <https://www.owlintegrations.com/>
[ClusterDuck Protocol]: <https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol/wiki>
[Discord]: <https://discord.com/invite/Cbgbzq353z>
