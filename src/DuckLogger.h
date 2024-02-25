#ifndef DUCKLOGGER_H
#define DUCKLOGGER_H

#ifndef CDP_NO_LOG
#include "Arduino.h"
#define CDP_DEBUG
#endif


#define CDP_DEBUG


#ifdef CDP_DEBUG
#define CDP_LOG_ERROR
#define CDP_LOG_INFO
#define CDP_LOG_DEBUG
#define CDP_LOG_WARN
#endif

#ifndef __FILENAME__
#define __FILENAME__                                                           \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif


#if defined(ARDUINO)
#define OUTPUT_PORT Serial
#else
#define PORT std::cout
#endif


// https://github.com/esp8266/Arduino/blob/65579d29081cb8501e4d7f786747bf12e7b37da2/cores/esp8266/Print.cpp#L50
static size_t cdpPrintf(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    char temp[64];
    char* buffer = temp;
    size_t len = vsnprintf(temp, sizeof(temp), format, arg);
    va_end(arg);
    if (len > sizeof(temp) - 1) {
        buffer = new (std::nothrow) char[len + 1];
        if (!buffer) {
            return 0;
        }
        va_start(arg, format);
        vsnprintf(buffer, len + 1, format, arg);
        va_end(arg);
    }
    len = OUTPUT_PORT.write((const uint8_t*) buffer, len);
    if (buffer != temp) {
        delete[] buffer;
    }
    return len;
}

#if defined(CDP_LOG_ERROR)
#define logerr(format, ...)                                     \
  do {                                                          \
    cdpPrintf("[E]");                                           \
    cdpPrintf("[** %s : %d] ",__FILENAME__, __LINE__);          \
    cdpPrintf(format, ##__VA_ARGS__);                           \
  } while (0)

#define logerr_ln(format, ...)                                  \
  do {                                                          \
    cdpPrintf("[E]");                                           \
    cdpPrintf("[** %s : %d] ",__FILENAME__, __LINE__);          \
    cdpPrintf(format, ##__VA_ARGS__);cdpPrintf("\n");           \
  } while (0)
#else
#define logerr(format, ...)                                     \
  {}                                                                            
#define logerr_ln(format, ...)                                  \
  {}
#endif // CDP_LOG_ERROR


#if defined(CDP_LOG_WARN)
#define logwarn(format, ...)                                    \
  do {                                                          \
    cdpPrintf("[W]");                                           \
    cdpPrintf("%s : %d] ",__FILENAME__, __LINE__);              \
    cdpPrintf(format, ##__VA_ARGS__);                           \
  } while (0)

#define logwarn_ln(format, ...)                                 \
  do {                                                          \
    cdpPrintf("[W]");                                           \
    cdpPrintf("%s : %d] ",__FILENAME__, __LINE__);              \
    cdpPrintf(format, ##__VA_ARGS__);cdpPrintf("\n");           \
  } while (0)
#else
#define logwarn(format, ...)                                    \
  {}                                                                            
#define logwarn_ln(format, ...)                                 \
  {}
#endif // CDP_LOG_WARN

#if defined(CDP_LOG_INFO)
#define loginfo(format, ...)                                    \
  do {                                                          \
    cdpPrintf("[I]");                                           \
    cdpPrintf("[%s] ",__FILENAME__);                            \
    cdpPrintf(format, ##__VA_ARGS__);                           \
  } while (0)

#define loginfo_ln(format, ...)                                 \
  do {                                                          \
    cdpPrintf("[I]");                                           \
    cdpPrintf("[%s] ",__FILENAME__);                            \
    cdpPrintf(format, ##__VA_ARGS__);cdpPrintf("\n");           \
  } while (0)
#else
#define loginfo(format, ...)                                    \
  {}                                                                            
#define loginfo_ln(format, ...)                                 \
  {}
#endif // CDP_LOG_INFO

#if defined(CDP_LOG_DEBUG)
#define logdbg(format, ...)                                     \
  do {                                                          \
    cdpPrintf("[D]");                                           \
    cdpPrintf("[%s] ",__FILENAME__);                            \
    cdpPrintf(format, ##__VA_ARGS__);                           \
  } while (0)

#define logdbg_ln(format, ...)                                  \
  do {                                                          \
    cdpPrintf("[D]");                                           \
    cdpPrintf("[%s] ",__FILENAME__);                            \
    cdpPrintf(format, ##__VA_ARGS__);cdpPrintf("\n");           \
  } while (0)
#else
#define logdbg(format, ...)                                     \
  {}                                                                            
#define logdbg_ln(format, ...)                                  \
  {}
#endif
#endif
