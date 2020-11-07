CRC32
=====

[![Build Status](https://travis-ci.org/bakercp/CRC32.svg?branch=master)](https://travis-ci.org/bakercp/CRC32)

## Description

An Arduino library for calculating a CRC32 checksum.

## Features

- Calculate the CRC32 checksum with minimal memory overhead.
- Calculate the CRC32 checksum as a stream or using buffered data.
- Calculate the CRC32 checksum of any arbitrary data type.

## Background

A cyclic redundancy check (CRC) is often used to verify data integrity. The CRC32 algorithm is a common way to quickly produce a small, repeatable hash representing an arbitrary block of data. It is often used to verify data packet integrity during serial or network communication.

## Use

The CRC32 checksum can be calculated on-the-fly with no buffering or the user can pass a complete data buffer.

### Streaming

```c++
CRC32 crc;

// Read a stream of bytes.
while (bytesRemain)
{
    // Read a byte from arbitrary source.
    uint8_t byte = readAByteFromAStream();

    // Add the byte to the checksum.
    crc.update(byte);
}

// Calculate the checksum.
uint32_t checksum = crc.finalize();

// To reuse the same instance, call.
crc.reset();
```

### Buffer

```c++
uint8_t byteBuffer[32] = { 0x00, 0x01, 0x02, 0x03,
                           0x04, 0x05, 0x06, 0x07,
                           0x08, 0x09, 0x0A, 0x0B,
                           0x0C, 0x0D, 0x0E, 0x0F,
                           0x10, 0x11, 0x12, 0x13,
                           0x14, 0x15, 0x16, 0x17,
                           0x18, 0x19, 0x1A, 0x1B,
                           0x1C, 0x1D, 0x1E, 0x1F };

// Calculate the checksum of an entire buffer at once.
uint32_t checksum = CRC32::calculate(byteBuffer, 32);
```

## Examples

See the included examples and tests for further usage examples.

## Compatible Implementations

- Java / Processing
    - [java.util.zip.CRC32](https://docs.oracle.com/javase/7/docs/api/java/util/zip/CRC32.html-
- C++ / openFrameworks
    - [Poco::Checksum](https://pocoproject.org/docs/Poco.Checksum.html)

## Changelog
See [CHANGELOG.md](CHANGELOG.md)


## License
See [LICENSE.md](LICENSE.md)