#line 2 "CRC32.ino"

#include <ArduinoUnit.h>
#include <CRC32.h>



const uint8_t ALL_ZEROS[32] = { 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00 };

const uint32_t ALL_ZEROS_CHECKSUM = 0x190A55AD;

const uint8_t ALL_ONES[32] =  { 0xFF, 0xFF, 0xFF, 0xFF,
                                0xFF, 0xFF, 0xFF, 0xFF,
                                0xFF, 0xFF, 0xFF, 0xFF,
                                0xFF, 0xFF, 0xFF, 0xFF,
                                0xFF, 0xFF, 0xFF, 0xFF,
                                0xFF, 0xFF, 0xFF, 0xFF,
                                0xFF, 0xFF, 0xFF, 0xFF,
                                0xFF, 0xFF, 0xFF, 0xFF };

const uint32_t ALL_ONES_CHECKSUM = 0xFF6CAB0B;

const uint8_t INCREASING[32] = { 0x00, 0x01, 0x02, 0x03,
                                 0x04, 0x05, 0x06, 0x07,
                                 0x08, 0x09, 0x0A, 0x0B,
                                 0x0C, 0x0D, 0x0E, 0x0F,
                                 0x10, 0x11, 0x12, 0x13,
                                 0x14, 0x15, 0x16, 0x17,
                                 0x18, 0x19, 0x1A, 0x1B,
                                 0x1C, 0x1D, 0x1E, 0x1F };

const uint32_t INCREASING_CHECKSUM = 0x91267E8A;

const uint8_t DECREASING[32] = { 0x1F, 0x1E, 0x1D, 0x1C,
                                 0x1B, 0x1A, 0x19, 0x18,
                                 0x17, 0x16, 0x15, 0x14,
                                 0x13, 0x12, 0x11, 0x10,
                                 0x0F, 0x0E, 0x0D, 0x0C,
                                 0x0B, 0x0A, 0x09, 0x08,
                                 0x07, 0x06, 0x05, 0x04,
                                 0x03, 0x02, 0x01, 0x00 };

const uint32_t DECREASING_CHECKSUM = 0x9AB0EF72;

const uint8_t ISCSI_PDU[48] = { 0x01, 0xC0, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00,
                                0x14, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x04, 0x00,
                                0x00, 0x00, 0x00, 0x14,
                                0x00, 0x00, 0x00, 0x18,
                                0x28, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00,
                                0x02, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00 };

const uint32_t ISCSI_PDU_CHECKSUM = 0x51E17412;


struct TestDatum
{
    size_t size;
    const uint8_t* data;
    uint32_t checksum;
};

const size_t TOTAL_DATUM = 5;
TestDatum test_data[TOTAL_DATUM] {
    { 32, ALL_ZEROS, ALL_ZEROS_CHECKSUM },
    { 32, ALL_ONES, ALL_ONES_CHECKSUM },
    { 32, INCREASING, INCREASING_CHECKSUM },
    { 32, DECREASING, DECREASING_CHECKSUM },
    { 48, ISCSI_PDU, ISCSI_PDU_CHECKSUM }
};


test(static_tests)
{
    for (size_t i = 0; i < TOTAL_DATUM; ++i)
    {
        assertEqual(CRC32::calculate(test_data[i].data, test_data[i].size), test_data[i].checksum);
    }
}


test(one_by_one)
{
    for (size_t i = 0; i < TOTAL_DATUM; ++i)
    {
        CRC32 crc;
        for (size_t j = 0; j < test_data[i].size; ++j)
        {
            crc.update(test_data[i].data[j]);
        }
        assertEqual(test_data[i].checksum, crc.finalize());
    }
}


test(as_array)
{
    for (size_t i = 0; i < TOTAL_DATUM; ++i)
    {
        CRC32 crc;
        crc.update(test_data[i].data, test_data[i].size);
        assertEqual(test_data[i].checksum, crc.finalize());
    }
}


test(static_by_32bits)
{
    for (size_t i = 0; i < TOTAL_DATUM; ++i)
    {
        assertEqual(CRC32::calculate((const uint32_t*)test_data[i].data, test_data[i].size / sizeof(uint32_t)), test_data[i].checksum);
    }
}


test(one_by_one_32)
{
    for (size_t i = 0; i < TOTAL_DATUM; ++i)
    {
        const uint32_t* pData = (const uint32_t*)test_data[i].data;
        size_t size = test_data[i].size / sizeof(uint32_t);
        CRC32 crc;
        for (size_t j = 0; j < size; ++j)
        {
            crc.update(pData[j]);
        }
        assertEqual(test_data[i].checksum, crc.finalize());
    }
}


test(as_array_32)
{
    for (size_t i = 0; i < TOTAL_DATUM; ++i)
    {
        CRC32 crc;
        crc.update((const uint32_t*)test_data[i].data, test_data[i].size / sizeof(uint32_t));
        assertEqual(test_data[i].checksum, crc.finalize());
    }
}


void setup()
{
  Serial.begin(9600);
  while(!Serial) {;}
}

void loop()
{
  Test::run();
}
