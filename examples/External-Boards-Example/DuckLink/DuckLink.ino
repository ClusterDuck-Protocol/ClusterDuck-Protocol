#include <CDP.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// create a built-in duck link
DuckLink duck;

void setup()
{
    std::string deviceId("LINK0001");
    std::array<byte, 8> devId;
    int rc;
    std::copy(deviceId.begin(), deviceId.end(), devId.begin());
    rc = duck.setupWithDefaults(devId);

    loginfo_ln("Hello, World!");
}

void loop()
{

}
