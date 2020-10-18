#ifndef DUCKLOGGER_H
#define DUCKLOGGER_H

#define CDP_LOG_ERROR
#define CDP_LOG_INFO
// #define CDP_LOG_DEBUG
#define CDP_LOG_WARN

#define __FILENAME__                                                           \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#if defined(CDP_LOG_ERROR)
#define logerr(...)                                                            \
  do {                                                                         \
    Serial.print("[ERR] ");                                                      \
    Serial.print(__VA_ARGS__);                                                 \
    Serial.println(" in " + String(__FILENAME__) + "(" + String(__LINE__) +    \
                   ")");                                                       \
  } while (0)
#define logerr_f(...)                                                          \
  do {                                                                         \
    Serial.print("[ERR] ");                                                    \
    Serial.printf(__VA_ARGS__);                                                \
    Serial.println(" in " + String(__FILENAME__) + "(" + String(__LINE__) +    \
                   ")");                                                       \
  } while (0)
#else
#define logerr(...)                                                            \
  {}
#define logerr_f(...)                                                          \
  {}
#endif

#if defined(CDP_LOG_WARN)
#define logwarn(...)                                                           \
  do {                                                                         \
    Serial.print("[WRN]  ");                                                     \
    Serial.print(__VA_ARGS__);                                                 \
    Serial.println(" in " + String(__FILENAME__) + "(" + String(__LINE__) +    \
                   ")");                                                       \
  } while (0)
#define logwarn_f(...)                                                         \
  do {                                                                         \
    Serial.print("[WRN]  ");                                                     \
    Serial.printf(__VA_ARGS__);                                                \
    Serial.println(" in " + String(__FILENAME__) + "(" + String(__LINE__) +    \
                   ")");                                                       \
  } while (0)
#else
#define logerr(...)                                                            \
  {}
#define logerr_f(...)                                                          \
  {}
#endif

#if defined(CDP_LOG_INFO)
#define loginfo(...)                                                           \
  do {                                                                         \
    Serial.print("[INF]  ");                                                     \
    Serial.print(__VA_ARGS__);                                                 \
    Serial.println(" in " + String(__FILENAME__));                             \
  } while (0)
#define loginfo_f(...)                                                         \
  do {                                                                         \
    Serial.print("[INF]  ");                                                     \
    Serial.printf(__VA_ARGS__);                                                \
    Serial.println(" in " + String(__FILENAME__));                             \
  } while (0)
#else
#define loginfo(...)                                                           \
  {}
#define loginfo_f(...)                                                         \
  {}
#endif

#if defined(CDP_LOG_DEBUG)
#define logdbg(...)                                                            \
  do {                                                                         \
    Serial.print("[DBG]  ");                                                     \
    Serial.print(__VA_ARGS__);                                                 \
    Serial.println(" in " + String(__FILENAME__));                             \
  } while (0)
#define logdbg_f(...)                                                          \
  do {                                                                         \
    Serial.print("[DBG]  ");                                                   \
    Serial.printf(__VA_ARGS__);                                                \
    Serial.println(" in " + String(__FILENAME__));                             \
  } while (0)
#else
#define logdbg(...)                                                            \
  {}
#define logdbg_f(...)                                                          \
  {}
#endif
#endif
