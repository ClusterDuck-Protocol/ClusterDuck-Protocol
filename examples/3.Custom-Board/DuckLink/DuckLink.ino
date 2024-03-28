#include <CDP.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// create a built-in duck link
DuckLink duck;

void setup()
{
    std::string deviceId("LINK0001");
    std::vector<byte> devId;
    int rc;
    devId.insert(devId.end(), deviceId.begin(), deviceId.end());
    rc = duck.setupWithDefaults(devId);

    loginfo_ln("Hello, World!");
}

void loop()
{

}
