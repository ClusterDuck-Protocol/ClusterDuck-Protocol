#include <unity.h>
#include <cdpcfg.h>
#include <DuckRadio.h>
#include <vector>
#include <DuckError.h>


LoraConfigParams defaultConfig = {
    .band = CDPCFG_RF_LORA_FREQ,
    .ss = CDPCFG_PIN_LORA_CS,
    .rst = CDPCFG_PIN_LORA_RST,
    .di0 = CDPCFG_PIN_LORA_DIO0,
    .di1 = CDPCFG_PIN_LORA_DIO1,
    .txPower = CDPCFG_RF_LORA_TXPOW,
    .bw = CDPCFG_RF_LORA_BW,
    .sf = CDPCFG_RF_LORA_SF,
    .gain = CDPCFG_RF_LORA_GAIN,
    .func = DuckRadio::onInterrupt
};

void test_DuckRadio_SetupRadio_Normal(void) {
    // definitions from the board configuration file and cdpcfg.h

    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(0, radio.setupRadio(defaultConfig));
    TEST_ASSERT_EQUAL(CDPCFG_RADIO_CHANNEL_1, radio.getChannel());
    
}
void test_DuckRadio_SetupRadio_AlreadySetup(void) {
    // definitions from the board configuration file and cdpcfg.h

    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(0, radio.setupRadio(defaultConfig));
    TEST_ASSERT_EQUAL(CDPCFG_RADIO_CHANNEL_1, radio.getChannel());
    TEST_ASSERT_EQUAL(0, radio.setupRadio(defaultConfig));
    
}

void test_DuckRadio_SetupRadio_Null_Interrupt_Function() {
    // definitions from the board configuration file and cdpcfg.h
    LoraConfigParams config = {
        .band = CDPCFG_RF_LORA_FREQ,
        .ss = CDPCFG_PIN_LORA_CS,
        .rst = CDPCFG_PIN_LORA_RST,
        .di0 = CDPCFG_PIN_LORA_DIO0,
        .di1 = CDPCFG_PIN_LORA_DIO1,
        .txPower = CDPCFG_RF_LORA_TXPOW,
        .bw = CDPCFG_RF_LORA_BW,
        .sf = CDPCFG_RF_LORA_SF,
        .gain = CDPCFG_RF_LORA_GAIN,
        .func = NULL
    };
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(DUCK_ERR_INVALID_ARGUMENT, radio.setupRadio(config));
}

void test_DuckRadio_SetupRadio_Invalid_SF_Lower() {
    // definitions from the board configuration file and cdpcfg.h
    LoraConfigParams config = {
        .band = CDPCFG_RF_LORA_FREQ,
        .ss = CDPCFG_PIN_LORA_CS,
        .rst = CDPCFG_PIN_LORA_RST,
        .di0 = CDPCFG_PIN_LORA_DIO0,
        .di1 = CDPCFG_PIN_LORA_DIO1,
        .txPower = CDPCFG_RF_LORA_TXPOW,
        .bw = CDPCFG_RF_LORA_BW,
        .sf = 0,
        .gain = CDPCFG_RF_LORA_GAIN,
        .func = DuckRadio::onInterrupt
    };
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(DUCK_ERR_INVALID_ARGUMENT, radio.setupRadio(config));
}
void test_DuckRadio_SetupRadio_Invalid_SF_Upper() {
    // definitions from the board configuration file and cdpcfg.h
    LoraConfigParams config = {
        .band = CDPCFG_RF_LORA_FREQ,
        .ss = CDPCFG_PIN_LORA_CS,
        .rst = CDPCFG_PIN_LORA_RST,
        .di0 = CDPCFG_PIN_LORA_DIO0,
        .di1 = CDPCFG_PIN_LORA_DIO1,
        .txPower = CDPCFG_RF_LORA_TXPOW,
        .bw = CDPCFG_RF_LORA_BW,
        .sf = 13,
        .gain = CDPCFG_RF_LORA_GAIN,
        .func = DuckRadio::onInterrupt
    };
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(DUCK_ERR_INVALID_ARGUMENT, radio.setupRadio(config));
}

void test_DuckRadio_SetupRadio_Invalid_BW_Lower() {
    // definitions from the board configuration file and cdpcfg.h
    LoraConfigParams config = {
        .band = CDPCFG_RF_LORA_FREQ,
        .ss = CDPCFG_PIN_LORA_CS,
        .rst = CDPCFG_PIN_LORA_RST,
        .di0 = CDPCFG_PIN_LORA_DIO0,
        .di1 = CDPCFG_PIN_LORA_DIO1,
        .txPower = CDPCFG_RF_LORA_TXPOW,
        .bw = 0,
        .sf = CDPCFG_RF_LORA_SF,
        .gain = CDPCFG_RF_LORA_GAIN,
        .func = DuckRadio::onInterrupt
    };
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(DUCK_ERR_INVALID_ARGUMENT, radio.setupRadio(config));
}
void test_DuckRadio_SetupRadio_Invalid_BW_Upper() {
    // definitions from the board configuration file and cdpcfg.h
    LoraConfigParams config = {
        .band = CDPCFG_RF_LORA_FREQ,
        .ss = CDPCFG_PIN_LORA_CS,
        .rst = CDPCFG_PIN_LORA_RST,
        .di0 = CDPCFG_PIN_LORA_DIO0,
        .di1 = CDPCFG_PIN_LORA_DIO1,
        .txPower = CDPCFG_RF_LORA_TXPOW,
        .bw = 501.0,
        .sf = CDPCFG_RF_LORA_SF,
        .gain = CDPCFG_RF_LORA_GAIN,
        .func = DuckRadio::onInterrupt
    };
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(DUCK_ERR_INVALID_ARGUMENT, radio.setupRadio(config));
}
void test_DuckRadio_SetupRadio_Invalid_TXPower_Lower() {
    // definitions from the board configuration file and cdpcfg.h
    LoraConfigParams config = {
        .band = CDPCFG_RF_LORA_FREQ,
        .ss = CDPCFG_PIN_LORA_CS,
        .rst = CDPCFG_PIN_LORA_RST,
        .di0 = CDPCFG_PIN_LORA_DIO0,
        .di1 = CDPCFG_PIN_LORA_DIO1,
        .txPower = -10,
        .bw = CDPCFG_RF_LORA_BW,
        .sf = CDPCFG_RF_LORA_SF,
        .gain = CDPCFG_RF_LORA_GAIN,
        .func = DuckRadio::onInterrupt
    };
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(DUCK_ERR_INVALID_ARGUMENT, radio.setupRadio(config));
}

void test_DuckRadio_SetupRadio_Invalid_TXPower_Upper() {
    // definitions from the board configuration file and cdpcfg.h
    LoraConfigParams config = {
        .band = CDPCFG_RF_LORA_FREQ,
        .ss = CDPCFG_PIN_LORA_CS,
        .rst = CDPCFG_PIN_LORA_RST,
        .di0 = CDPCFG_PIN_LORA_DIO0,
        .di1 = CDPCFG_PIN_LORA_DIO1,
        .txPower = 23,
        .bw = CDPCFG_RF_LORA_BW,
        .sf = CDPCFG_RF_LORA_SF,
        .gain = CDPCFG_RF_LORA_GAIN,
        .func = DuckRadio::onInterrupt
    };
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(DUCK_ERR_INVALID_ARGUMENT, radio.setupRadio(config));
}
void test_DuckRadio_SetupRadio_Invalid_Band_Lower() {
    // definitions from the board configuration file and cdpcfg.h
    LoraConfigParams config = {
        .band = 149,
        .ss = CDPCFG_PIN_LORA_CS,
        .rst = CDPCFG_PIN_LORA_RST,
        .di0 = CDPCFG_PIN_LORA_DIO0,
        .di1 = CDPCFG_PIN_LORA_DIO1,
        .txPower = CDPCFG_RF_LORA_TXPOW,
        .bw = CDPCFG_RF_LORA_BW,
        .sf = CDPCFG_RF_LORA_SF,
        .gain = CDPCFG_RF_LORA_GAIN,
        .func = DuckRadio::onInterrupt
    };
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(DUCK_ERR_INVALID_ARGUMENT, radio.setupRadio(config));
}

void test_DuckRadio_SetupRadio_Invalid_Band_Upper() {
    // definitions from the board configuration file and cdpcfg.h
    LoraConfigParams config = {
        .band = 961,
        .ss = CDPCFG_PIN_LORA_CS,
        .rst = CDPCFG_PIN_LORA_RST,
        .di0 = CDPCFG_PIN_LORA_DIO0,
        .di1 = CDPCFG_PIN_LORA_DIO1,
        .txPower = CDPCFG_RF_LORA_TXPOW,
        .bw = CDPCFG_RF_LORA_BW,
        .sf = CDPCFG_RF_LORA_SF,
        .gain = CDPCFG_RF_LORA_GAIN,
        .func = DuckRadio::onInterrupt
    };
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(DUCK_ERR_INVALID_ARGUMENT, radio.setupRadio(config));
}


void test_DuckRadio_SetSyncWord_Normal() {
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(0, radio.setupRadio(defaultConfig));
    TEST_ASSERT_EQUAL(0, radio.setSyncWord(0x34));
}

void test_DuckRadio_SetSyncWord_Uninialized() {
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(DUCKLORA_ERR_NOT_INITIALIZED, radio.setSyncWord(0x34));
}


void test_DuckRadio_SendData_Normal() {
    DuckRadio& radio = DuckRadio::getInstance();
    std::vector<byte> data = {0x01, 0x02, 0x03, 0x04};
    TEST_ASSERT_EQUAL(0, radio.setupRadio(defaultConfig));
    TEST_ASSERT_EQUAL(0, radio.sendData(data));
}

void test_DuckRadio_SendData_Uninitialized() {
    DuckRadio& radio = DuckRadio::getInstance();
    std::vector<byte> data = {0x01, 0x02, 0x03, 0x04};
    TEST_ASSERT_EQUAL(DUCKLORA_ERR_NOT_INITIALIZED, radio.sendData(data));
}

void test_DuckRadio_SendData_Invalid_Length() {
    DuckRadio& radio = DuckRadio::getInstance();
    std::vector<byte> data;

    for (int i = 0; i < 300; i++) {
        data.push_back(i);
    }
    TEST_ASSERT_EQUAL(0, radio.setupRadio(defaultConfig));
    TEST_ASSERT_NOT_EQUAL(DUCK_ERR_NONE, radio.sendData(data));
}

void test_DuckRadio_SetChannel_Once() {
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(0, radio.setupRadio(defaultConfig));
    const int TEST_CHANNEL = 4;
    int result = radio.setChannel(TEST_CHANNEL);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(TEST_CHANNEL, radio.getChannel());
}

void test_DuckRadio_SetChannel_Normal() {
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(0, radio.setupRadio(defaultConfig));
    for (int c = 1; c < 7; c++) {
        int result = radio.setChannel(c);
        delay(500);
        TEST_ASSERT_EQUAL(0, result);
        TEST_ASSERT_EQUAL(c, radio.getChannel());
    }
}

void test_DuckRadio_SetChannel_Invalid() {
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(0, radio.setupRadio(defaultConfig));
    int result = radio.setChannel(0);
    TEST_ASSERT_EQUAL(DUCKLORA_ERR_INVALID_CHANNEL, result);
    result = radio.setChannel(7);
    TEST_ASSERT_EQUAL(DUCKLORA_ERR_INVALID_CHANNEL, result);
}

void test_DuckRadio_SetChannel_Unchanged_If_Invalid() {
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(0, radio.setupRadio(defaultConfig));
    const int INITIAL_CHANNEL = 3;
    int result = radio.setChannel(INITIAL_CHANNEL);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(INITIAL_CHANNEL, radio.getChannel());

    result = radio.setChannel(0);
    TEST_ASSERT_EQUAL(DUCKLORA_ERR_INVALID_CHANNEL, result);
    TEST_ASSERT_EQUAL(INITIAL_CHANNEL, radio.getChannel());
    result = radio.setChannel(7);
    TEST_ASSERT_EQUAL(DUCKLORA_ERR_INVALID_CHANNEL, result);
    TEST_ASSERT_EQUAL(INITIAL_CHANNEL, radio.getChannel());
}

void test_DuckRadio_SetChannel_Uninitialized() {
    DuckRadio& radio = DuckRadio::getInstance();
    const int TEST_CHANNEL = 4;
    int result = radio.setChannel(TEST_CHANNEL);
    TEST_ASSERT_EQUAL(DUCKLORA_ERR_NOT_INITIALIZED, result);
}

void test_DuckRadio_StandBy_Normal() {
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(0, radio.setupRadio(defaultConfig));
    TEST_ASSERT_EQUAL(0, radio.standBy());
}

void test_DuckRadio_StandBy_Uninitialized() {
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(DUCKLORA_ERR_NOT_INITIALIZED, radio.standBy());
}

void test_DuckRadio_Sleep_Normal() {
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(0, radio.setupRadio(defaultConfig));
    TEST_ASSERT_EQUAL(0, radio.sleep());
}

void test_DuckRadio_Sleep_Uninitialized() {
    DuckRadio& radio = DuckRadio::getInstance();
    TEST_ASSERT_EQUAL(DUCKLORA_ERR_NOT_INITIALIZED, radio.sleep());
}

void setup() {
    UNITY_BEGIN();

    // RUN THESE TESTS FIRST TO ENSURE THE RADIO SETUP HAS NOT BEEN CALLED YET
    RUN_TEST(test_DuckRadio_SendData_Uninitialized);
    RUN_TEST(test_DuckRadio_SetChannel_Uninitialized);
    RUN_TEST(test_DuckRadio_SetSyncWord_Uninialized);
    RUN_TEST(test_DuckRadio_StandBy_Uninitialized);
    RUN_TEST(test_DuckRadio_Sleep_Uninitialized);

    // TEST ORDER MATTERS
    RUN_TEST(test_DuckRadio_SetupRadio_Normal);
    RUN_TEST(test_DuckRadio_SetupRadio_AlreadySetup);
    RUN_TEST(test_DuckRadio_SetupRadio_Null_Interrupt_Function);
    RUN_TEST(test_DuckRadio_SetupRadio_Invalid_SF_Lower);
    RUN_TEST(test_DuckRadio_SetupRadio_Invalid_SF_Upper);
    RUN_TEST(test_DuckRadio_SetupRadio_Invalid_BW_Lower);
    RUN_TEST(test_DuckRadio_SetupRadio_Invalid_BW_Upper);
    RUN_TEST(test_DuckRadio_SetupRadio_Invalid_TXPower_Lower);
    RUN_TEST(test_DuckRadio_SetupRadio_Invalid_TXPower_Upper);
    RUN_TEST(test_DuckRadio_SetupRadio_Invalid_Band_Lower);
    RUN_TEST(test_DuckRadio_SetupRadio_Invalid_Band_Upper);
    RUN_TEST(test_DuckRadio_SetSyncWord_Normal);
    RUN_TEST(test_DuckRadio_SendData_Normal);

    // RUN THESE AFTER SETUP TESTS
    RUN_TEST(test_DuckRadio_SendData_Invalid_Length);
    RUN_TEST(test_DuckRadio_SetChannel_Once);
    RUN_TEST(test_DuckRadio_SetChannel_Invalid);
    RUN_TEST(test_DuckRadio_SetChannel_Unchanged_If_Invalid);
    RUN_TEST(test_DuckRadio_SetChannel_Normal);
    RUN_TEST(test_DuckRadio_StandBy_Normal);
    RUN_TEST(test_DuckRadio_Sleep_Normal);
    
    UNITY_END();
}

void loop() {
}