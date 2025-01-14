<p align="center">
  <img src="docs/assets/images/cdp_logo_white_bkg.png" alt="Logo">
</p>

<p align="center">
  <a href="https://www.apache.org/licenses/LICENSE-2.0"><img src="https://img.shields.io/badge/License-Apache2-FFA500.svg" alt="License"></a>
  <a href="https://discord.com/invite/Cbgbzq353z"><img src="https://img.shields.io/badge/Join-Discord-aa80ff" alt="Discord"></a>
  <a href="https://github.com/Call-for-Code/ClusterDuck-Protocol/wiki"><img src="https://img.shields.io/badge/Read-Wiki-50dda0" alt="Wiki"></a>
  <a href="https://clusterduck-protocol.github.io/ClusterDuck-Protocol/html/index.html"><img src="https://img.shields.io/badge/API-Doc-50dda0" alt="Wiki"></a>
  <img src="https://img.shields.io/github/v/release/ClusterDuck-Protocol/ClusterDuck-Protocol?label=Release&color=10ccff" alt="GitHub Release">
  <img src="https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol/actions/workflows/arduino_ci.yml/badge.svg" alt="Build Status">
</p>



## What is the CDP?

The ClusterDuck Protocol (CDP), an innovative open-source project under the Linux Foundation, represents a significant leap in IoT communications, harnessing the power of low bandwidth and low power such as LoRa technology. Distinct from LoRaWAN, this protocol utilizes LoRa's point-to-multipoint capabilities to facilitate robust communication in diverse environments.

The ClusterDuck Protocol was created by [OWL Integrations] (formerly Project OWL) to provide developers with an advanced library for crafting distributed communication networks, tailored for a variety of IoT applications. Since its creation, the vision for the [ClusterDuck Protocol] has grown beyond servicing natural disasters to large events (cellular congestion), sensor networks, and more. 

## How does it work?

The network is made up of multiple nodes called **Ducks**. There are 3 basic types of Ducks in a ClusterDuck Network: DuckLink, MamaDuck, and PapaDuck. **DuckLinks** serve as edge nodes that only transmit data. **MamaDucks** inherit the same functionality of the DuckLinks but can also receive messages. This allows the MamaDucks to relay messages from DuckLinks and/or other MamaDucks along the network (towards the gateway). **PapaDucks** are the endpoint of the network where all of the data is collected and can be stored or pushed up into the cloud. **(free tier cloud DMS platform in BETA! Reach out to us on Discord for more info)**

![overview](https://www.project-owl.com/assets/wiki/cdp-explain-gif.gif)

## Installation

Check out the [Wiki](https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol/wiki) to learn how to build your own ClusterDuck Network. Feel free to visit the  [ClusterDuck Protocol](https://clusterduckprotocol.org/) website for more information on projects that the community is building. You can reach out directly on our [Discord Community](https://discord.gg/Cbgbzq353z) for any questions and/or to work with the community.

**To use the ClusterDuck Protocol follow the [Installation Manual](https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol/wiki/getting-started).**

## Testing

Starting with release 4.0.0 we have unit tests available with the PlatformIO test framework `unity`

Tests are located in the `ClusterDuckProtocol/test` folder. These tests are unit tests as they validate the CDP publicly accessible APIs. However, they must be run on a device. This means you have to connect a device to your development machine and build the tests to run on the device. Platform IO `test` command will build, deploy and run the tests and report back the results on your terminal console.

Unit tests are great in detecting problems before they make it to the release, so it is important not only to run them to validate your changes, but to continuously update them.

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

  For more details on setting up your development environment, please refer to the [developer's guide](./DEVELOPER_GUIDE.md).



#### PlatformIO CLI test command
  ```
  # Open a terminal 
  # goto the project root folder (where the platformio.ini is located)
  $ cd ClusterDuckProtocol
  $ platformio test -e test_heltec_wifi_lora_32_V3
  ```
#### Example output

  ```
  Processing test_DuckUtils in test_heltec_wifi_lora_32_V3 environment
-------------------------------------------------------------------------------------------------------------------------------------
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
-- test_heltec_wifi_lora_32_V3:test_DuckUtils [PASSED] Took 23.34 seconds -------------------------------------------------------------
```

## How to Contribute

We host a bi-weekly [CDP Town Hall](meet.google.com/unq-duaq-ygj) on Mondays at 2pm EST. The town hall is the place to get updates on protocol, get your questions about CDP answered, and discuss on-going projects. All the current projects are documented in a public [roadmap in Github Projects](https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol/projects).

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our Code of Conduct, the process for submitting ClusterDuck Protocol improvements, and how to join our town halls and livestreams.

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
