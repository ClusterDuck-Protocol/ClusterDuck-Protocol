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

Starting with release 3.7.0 we have unit tests available with the PlatformIO test framework `unity`

Tests are located in the `ClusterDuckProtocol/test` folder. These tests are unit tests as
they validate the CDP publicly accessible APIs. However they must be run on a device.
This means you have to connect a device to your development machine and build the tests to
run on the device. Platform IO `test` command will build, deploy and run the tests and
report back the results on your terminal console.

Unit tests are great in detecting problems before they make it to the release, so it is important
no only to run them to validate your changes, but to continuously update them.

### PlatformIO test environments
The project `platformio.ini` defines environment configurations for supported boards.
For example below is the configuration for tests on the Heltec LoRa v3 Arduino board
```
[env:test_heltec_wifi_lora_32_V3]
  test_build_src = yes
  test_filter = test_* 
  test_framework = unity
  platform = ${env:heltec_wifi_lora_32_V3.platform}
  board = ${env:heltec_wifi_lora_32_V3.board}
  framework = ${env:heltec_wifi_lora_32_V3.framework}
  monitor_speed = ${env.monitor_speed}
  build_src_filter  = +<./> +<./include> +<./include/boards>
  lib_deps =
      ${env.lib_deps} 
      ${env:heltec_wifi_lora_32_V3.lib_deps}
      ; why do I need to add the following libraries is a mystery to me
      SPI
      WIRE
      FS
      WIFI
      
  build_flags = 
      ${env.build_flags}
      -std=gnu++11
      -DUNIT_TEST
  ```

### How to run the tests
  Here are the steps to run the tests (on Linux or Mac OS). This assumes you have platformIO installed on your system.

  For Platform IO VSCode and CLI installation follow these links:

  https://platformio.org/install/ide?install=vscode (VSCode plugin)
 
  https://platformio.org/install/cli (CLI)
  

  ```
  # Open a terminal 
  # goto the project root folder (where the platformio.ini is located)
  $ cd ClusterDuckProtocol
  $ platformio test -e test_heltec_wifi_lora_32_V3
  ```
  ### Example output

  ```
  Processing test_DuckUtils in test_heltec_wifi_lora_32_V3 environment
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Building & Uploading...
Testing...
If you don't see any output for the first 10 secs, please reset board (press reset button)

test/test_DuckUtils/test_DuckUtils.cpp:181: test_DuckUtils_getCdpVersion                        [PASSED]
test/test_DuckUtils/test_DuckUtils.cpp:182: test_DuckUtils_toUpperCase                          [PASSED]
test/test_DuckUtils/test_DuckUtils.cpp:183: test_DuckUtils_stringToByteVector                   [PASSED]
test/test_DuckUtils/test_DuckUtils.cpp:184: test_DuckUtils_getRandomBytes                       [PASSED]
test/test_DuckUtils/test_DuckUtils.cpp:185: test_DuckUtils_createUuid_with_given_length         [PASSED]
test/test_DuckUtils/test_DuckUtils.cpp:186: test_DuckUtils_createUuid_with_default_length       [PASSED]
test/test_DuckUtils/test_DuckUtils.cpp:187: test_DuckUtils_convertToHex                         [PASSED]
test/test_DuckUtils/test_DuckUtils.cpp:188: test_DuckUtils_toString_printable_characters        [PASSED]
test/test_DuckUtils/test_DuckUtils.cpp:189: test_DuckUtils_toString_non_printable_characters    [PASSED]
test/test_DuckUtils/test_DuckUtils.cpp:190: test_DuckUtils_isEqual_true                         [PASSED]
test/test_DuckUtils/test_DuckUtils.cpp:191: test_DuckUtils_isEqual_false                        [PASSED]
test/test_DuckUtils/test_DuckUtils.cpp:192: test_DuckUtils_isEqual_false_with_different_sizes   [PASSED]
test/test_DuckUtils/test_DuckUtils.cpp:193: test_DuckUtils_toUint32                             [PASSED]
test/test_DuckUtils/test_DuckUtils.cpp:194: test_DuckUtils_saveWifiCredentials                  [PASSED]
test/test_DuckUtils/test_DuckUtils.cpp:195: test_DuckUtils_saveWifiCredentials_zero_length      [PASSED]
test/test_DuckUtils/test_DuckUtils.cpp:196: test_DuckUtils_loadWifiSsid                         [PASSED]
------------------------------------------------------------- test_heltec_wifi_lora_32_V3:test_DuckUtils [PASSED] Took 23.34 seconds -------------------------------------------------------------
```


## How to Contribute

We host a bi-weekly [CDP Town Hall](meet.google.com/unq-duaq-ygj) on Mondays at 2pm ET. The town hall is the place to get updates on protocol, get your questions about CDP answered, and discuss on-going projects. All the current projects is documented in a public [roadmap in Trello](https://trello.com/b/bU0cZuUJ/cdp-roadmap).

On the other Mondays (when there is no Town Hall), we host a live stream. Visit OWL's [YouTube Channel](https://www.youtube.com/c/OWLIntegrations) for updates.

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our Code of Conduct, the process for submitting ClusterDuck Protocol improvements, and how to join our town halls and livestreams.

This project is governed by its [Technical Charter](ClusterDuck-Protocol-Technical-Charter.pdf) and led by its [Technical Steering Committee](https://github.com/Call-for-Code/ClusterDuck-Protocol/wiki/technical-steering-committee).

## Contributors

<a href="https://github.com/Call-for-Code/clusterduck-protocol/graphs/contributors">
  <img src="https://contributors-img.web.app/image?repo=Call-for-Code/clusterduck-protocol" />
</a>

## License

This project is licensed under the Apache 2 License - see the [LICENSE](LICENSE) file for details.

## Version

v3.6.1

See `library.json` (PlatformIO) or `library.properties` (Arduino).

[Project OWL]: <https://www.project-owl.com/>
[ClusterDuck Protocol]: <https://github.com/Call-for-Code/ClusterDuck-Protocol/wiki>
[Slack Workspace]: <https://www.project-owl.com/slack>
