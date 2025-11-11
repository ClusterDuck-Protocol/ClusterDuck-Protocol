#include <CDP.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// create a built-in duck link
DuckLink duck("LINK0001");

void setup()
{
    rc = duck.setupWithDefaults();

    loginfo_ln("Hello, World!");
}

void loop()
{

}
