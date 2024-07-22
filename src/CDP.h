#ifndef CDP_H_
#define CDP_H_

/**
 * @file CDP.h
 * @brief CDP public header.
 * 
 * @version
*/

// if we have an external board definition, include it before anything else
#ifdef CDP_EXTERNAL_BOARD
#include "cdp_external_board.h"
#endif

#include "include/cdpcfg.h"
#include "DuckDetect.h"
#include "DuckLink.h"
#include "MamaDuck.h"
#include "PapaDuck.h"
#include "MemoryFree.h"
#include "CdpPacket.h"
#include "DuckError.h"
#include "DuckLogger.h"
#include "include/DuckNet.h"
#define CDP_STRINGIFY(x) #x
#define CDP_VALUE(x) CDP_STRINGIFY(x)

#ifndef CDP_BOARD_NAME
#warning "No board definition found! Please provide a board definition in your cdpcfg.h"
#else
#pragma message("\n" \
"-- CDP Library Info --\n" \
"Version:  \"" CDP_VALUE(CDP_VERSION_MAJOR) "." CDP_VALUE(CDP_VERSION_MINOR) "." CDP_VALUE(CDP_VERSION_PATCH) "\"\n" \
"Board: " CDP_VALUE(CDP_BOARD_NAME) "\n" \
"Compile Date: " __DATE__ " - " __TIME__ \
)
#endif

#endif // CDP_H_