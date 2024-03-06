
This directory is intended for PlatformIO Test Runner and project tests.

Unit Testing is a software testing method by which individual units of
source code, sets of one or more MCU program modules together with associated
control data, usage procedures, and operating procedures, are tested to
determine whether they are fit for use. Unit testing finds problems early
in the development cycle.

More information about PlatformIO Unit Testing:
- https://docs.platformio.org/en/latest/advanced/unit-testing/index.html


**Notes for CDP unit testing**
---

Building and running all tests for the Heltec WiFi LoRa 32 V3 board:

```
platformio test -e <test env defined in platform.ini>

platformio test -e test_heltec_wifi_lora_32_V3
```

Building and running a specific test suite for the Heltec WiFi LoRa 32 V3 board:

```
platformio test -e <test env defined in platform.ini> -f <test name >
platformio test -e test_heltec_wifi_lora_32_V3 -f test_DuckUtils
```


Example output after running some tests:

```
Testing...
If you don't see any output for the first 10 secs, please reset board (press reset button)

test/test_CdpPacket/test_CdpPacket.cpp:69: test_CdpPacket_constructor   [PASSED]
test/test_CdpPacket/test_CdpPacket.cpp:70: test_CdpPacket_reset [PASSED]
test/test_CdpPacket/test_CdpPacket.cpp:71: test_CdpPacket_constructor_from_buffer       [PASSED]
```

For more options:
```
platformio test --help
```

