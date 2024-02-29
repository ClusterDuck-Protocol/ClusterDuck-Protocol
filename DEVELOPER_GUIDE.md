# CDP Project Developer Guide

## Table of Contents
- [Introduction](#introduction)
- [Prerequisites](#prerequisites)
- [Project Setup And Build](#setup)
- [Testing](#testing)
- [How to run the tests](#how-to-run-the-tests)
- [How to run the examples](#how-to-run-the-examples)

## Introduction 
This guide will help you install the ClusterDuck Protocol (CDP) on your development machine. The CDP is a set of libraries and tools that enable the development of mesh networks for IoT devices. The CDP is designed to be used with the PlatformIO development environment and is compatible with the Arduino framework and IDE as well.

This guide will help you set up the CDP on your development machine and build the firmware for your development board. It is  primarily intended for developers who want to contribute to the CDP or use it in their projects.

## Prerequisites
- [PlatformIO](https://platformio.org/install/ide?install=vscode) installed on your development machine.
- [VSCode](https://code.visualstudio.com/download) or any other IDE compatible with PlatformIO. 
- A development board compatible with the CDP. For example, the Heltec LoRa v3 Arduino board.
- C/C++ IntelliSense, debugging, and code browsing capabilities for your IDE. For example, the C/C++ extension for VSCode. (C/C++, C/C++ Themes and C/C++ Extension Pack from Microsoft)

## Project Setup And Build
1. Clone the CDP repository to your development machine.
    ```bash
    $ git clone https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol.git
    ```

2. Open the project in VSCode or your preferred IDE. In VSCode by clicking on the "Open Folder" icon in the left sidebar and selecting the `ClusterDuck-Protocol` folder.

3. Open the `platformio.ini` file and select the environment for your development board. For example, the Heltec LoRa v3 Arduino board environment is defined as follows:
    ```ini
    [platformio]

    default_envs = heltec_wifi_lora_32_V3
    ;  default_envs = ttgo_t_beam
    ;  default_envs = zeroUSB
    ;  default_envs = cubecell_gps
    ;  default_envs = cubecell_board_v2
    ;  default_envs = test_heltec_wifi_lora_32_V3
    ;  default_envs = test_cubecell_board_v2
    ```
This will set the environment for the Heltec LoRa v3 Arduino board. You can select the environment for your development board by uncommenting the corresponding line.

4. Build the project by clicking on the PlatformIO icon in the VSCode sidebar and selecting the `Build` option. This will compile the project and generate the firmware for your development board.

The CDP project build a library which by itself does not do much. It is intended to be used as a dependency in other projects. However, the project also includes a few examples that demonstrate how to use the CDP library. You can find these examples in the `examples` folder. Additionally the project includes unit tests that validate the CDP publicly accessible APIs. These tests are located in the `test` folder.

## Testing
Starting with release 3.7.0 we have unit tests available with the PlatformIO test framework `unity`

Tests are located in the `ClusterDuckProtocol/test` folder. These tests are unit tests as they validate the CDP publicly accessible APIs. However they must be run on a device. This means you have to connect a device to your development machine and build the tests to run on the device. Platform IO `test` command will build, deploy and run the tests and report back the results on your terminal console.

Before building and uploading tests and examples to the device you need to install the platformio CLI (command line interface) on your system. You can find the installation instructions [here](https://platformio.org/install/cli).

### How to run the tests
Here are the steps to run the tests (on Linux or Mac OS). This assumes you have platformIO installed on your system.

1. Open a terminal

2. Go to the project root folder (where the platformio.ini is located)
    ```bash
    $ cd ClusterDuckProtocol
    ```

3. Run the tests
    ```bash
    $ platformio test -e test_heltec_wifi_lora_32_V3
    ```

    To run a specific test suite, you can use the `--filter` of `-f` option. For example, to run the `test_DuckUtils` test suite, you can use the following command:
    ```bash
    $ platformio test -e test_heltec_wifi_lora_32_V3 --filter test_DuckUtils
    ```

This will build, deploy and run the tests and report back the results on your terminal console.

### How to run the examples
Here are the steps to run the examples (on Linux or Mac OS). This assumes you have platformIO installed on your system.

1. Open a terminal

2. Go to the project root folder (where the platformio.ini is located)
    ```bash
    cd ClusterDuckProtocol
    ```

3. Run the examples for the Heltec LoRa v3 Arduino board using your local CDP library
    ```bash
    cd examples/1.Ducks/DuckLink
    platformio run -e local_heltec_wifi_lora_32_V3 -t upload
    ```

4. Run the examples for the Heltec LoRa v3 Arduino board using the CDP library from the PlatformIO library registry
    ```bash
    cd examples/1.Ducks/DuckLink
    platformio run -e prod_heltec_wifi_lora_32_V3 -t upload
    ```



