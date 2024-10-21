# Quick Start Guide for Building CDP Library and Examples

Instructions provided bellow are for Linux and MacOS X

## Prerequisites 
**Python 3:** Ensure Python 3 is installed on your system.

**PlatformIO CLI:** This tool requires the PlatformIO Command Line Interface. Please install it from PlatformIO's [CLI installation guide](https://docs.platformio.org/en/latest/core/installation/methods/installer-script.html).

## Clone the CDP Repository
Clone the CDP repository from GitHub using the following command and navigate to the root directory of the repository.
```bash
git clone https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol.git

cd ClusterDuck-Protocol
```
## Usage
This tools wraps the PlatformIO CLI to provide a simplified interface for building and uploading CDP libraries and examples. The following commands are available:

### Help
```bash
./tools/cdp.py -h
```
```
usage: cdp.py [-h] {build,test,clean,list-targets} ...

CDP - A PlatformIO CLI Wrapper to build CDP

positional arguments:
  {build,test,clean,list-targets}
                        Available commands
    build               build the project for a specific target
    test                test the project for a specific target
    clean               clean the project for a specific target
    list-targets        list all available target environments

options:
  -h, --help            show this help message and exit
 ```

## CDP Library
### List available targets
```bash
./tools/cdp.py list-targets
```
```
Available target environments:
  - heltec_wifi_lora_32_V3
  - lilygo_t_beam_sx1262
  - heltec_wifi_lora_32_V2
  - ttgo_lora32_v1
  - test_heltec_wifi_lora_32_V2
  - test_heltec_wifi_lora_32_V3
  - test_lilygo_t_beam_sx1262
```

### Build the CDP library for a specific target
```bash
./tools/cdp.py build --target <target_env>
```

```
./tools/cdp.py build --target heltec_wifi_lora_32_V3

Executing: platformio run -e heltec_wifi_lora_32_V3
Processing heltec_wifi_lora_32_V3 (board: heltec_wifi_lora_32_V3; platform: espressif32; framework: arduino)
.....
.....
Environment             Status    Duration
----------------------  --------  ------------
heltec_wifi_lora_32_V3  SUCCESS   00:00:32.583
```

### Deploy the CDP library for a specific target
```bash
./tools/cdp.py build --target <target_env> --deploy
```
```
./tools/cdp.py build -t heltec_wifi_lora_32_V3 --deploy

Retrieving maximum program size .pio/build/heltec_wifi_lora_32_V3/firmware.elf
Checking size .pio/build/heltec_wifi_lora_32_V3/firmware.elf
Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
RAM:   [=         ]   8.3% (used 27048 bytes from 327680 bytes)
Flash: [=         ]  13.8% (used 461569 bytes from 3342336 bytes)
Configuring upload protocol...
[.....]
Writing at 0x0005a4a8... (68 %)
Writing at 0x0005fb3d... (75 %)
Writing at 0x000666f7... (81 %)
Writing at 0x00070989... (87 %)
Writing at 0x00076280... (93 %)
Writing at 0x0007b96e... (100 %)
Wrote 461936 bytes (259949 compressed) at 0x00010000 in 6.9 seconds (effective 536.4 kbit/s)...
Hash of data verified.

Leaving...
Hard resetting via RTS pin...
```

### Test the CDP library for a specific target
```bash
./tools/cdp.py test --target <test_target_env> [--suite <test_suite_name>] # skip --suite to run all tests
```
```
./tools/cdp.py test -t test_heltec_wifi_lora_32_V3 -s test_CdpPacket

Processing test_CdpPacket in test_heltec_wifi_lora_32_V3 environment
--------------------------------------------------------------------
If you don't see any output for the first 10 secs, please reset board (press reset button)

test/test_CdpPacket/test_CdpPacket.cpp:109: test_CdpPacket_constructor                      [PASSED]
test/test_CdpPacket/test_CdpPacket.cpp:110: test_CdpPacket_reset                            [PASSED]
test/test_CdpPacket/test_CdpPacket.cpp:111: test_CdpPacket_constructor_from_buffer          [PASSED]
test/test_CdpPacket/test_CdpPacket.cpp:112: test_CdpPacket_constructor_from_buffer_variant  [PASSED]
```

### Clean the CDP library for a specific target
```bash
./tools/cdp.py clean --target <target_env>
```
