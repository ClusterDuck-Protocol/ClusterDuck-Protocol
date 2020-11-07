//
// Copyright (c) 2013 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#include <CRC32.h>


void setup()
{
  // Begin serial output for testing / debugging.
  Serial.begin(9600);
}

void loop()
{
  // The known CRC32 Checksum for the "Hello World" string below.
  const uint32_t KNOWN_CHECKSUM = 0x4A17B156;

  // Create some test data. This is an array of arbitrary bytes.
  // For this test, we'll create an array of bytes representing text.
  uint8_t byteBuffer[] = "Hello World";
  size_t numBytes = sizeof(byteBuffer) - 1;

  // Calculate a checksum one byte at a time.
  CRC32 crc;

  // Here we add each byte to the checksum, caclulating the checksum as we go.
  for (size_t i = 0; i < numBytes; i++)
  {
    crc.update(byteBuffer[i]);
  }

  // Once we have added all of the data, generate the final CRC32 checksum.
  uint32_t checksum = crc.finalize();

  if (checksum == KNOWN_CHECKSUM)
  {
    Serial.println(F("TEST PASSED"));
  }
  else
  {
    Serial.println(F("TEST FAILED"));
  }

  // Alternatively, we can calculate the checksum for the whole buffer. 

  checksum = CRC32::calculate(byteBuffer, numBytes);

  if (checksum == KNOWN_CHECKSUM)
  {
    Serial.println(F("TEST PASSED"));
  }
  else
  {
    Serial.println(F("TEST FAILED"));
  }

  Serial.println("Done.");

  // Wait a little bit because this is just a test.
  delay(3000);

}
