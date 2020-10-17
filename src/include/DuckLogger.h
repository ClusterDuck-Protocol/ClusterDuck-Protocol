#ifndef DUCKLOGGER_H
#define DUCKLOGGER_H

#define CDP_LOG_ERROR
#define CDP_LOG_INFO
#define CDP_LOG_DEBUG

#define __FILENAME__                                                           \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#if defined(CDP_LOG_ERROR)
#define logerr(...)                                                            \
  do {                                                                         \
    Serial.print("[ERR][" + String(__FILENAME__) + "] -- ");                  \
    Serial.println(__VA_ARGS__);                                               \
  } while (0)
#define logerr_ln(...)                                                         \
  do {                                                                         \
    Serial.print("[ERR][" + String(__FILENAME__) + "] -- ");                   \
    Serial.println(__VA_ARGS__);                                               \
  } while (0)
#define logerr_f(...)                                                          \
  do {                                                                         \
    Serial.print("[ERR][" + String(__FILENAME__) + "] -- ");                   \
    Serial.printf(__VA_ARGS__);                                                \
  } while (0)
#else
#define logerr(...)                                                            \
  {}
#define logerr_ln(...)                                                         \
  {}
#define logerr_f(...)                                                          \
  {}
#endif

#if defined(CDP_LOG_INFO)
#define loginfo(...)                                                           \
  do {                                                                         \
    Serial.print("[INF][" + String(__FILENAME__) + "] -- ");                  \
    Serial.println(__VA_ARGS__);                                               \
  } while (0)
#define loginfo_ln(...)                                                        \
  do {                                                                         \
    Serial.print("[INF][" + String(__FILENAME__) + "] -- ");                  \
    Serial.println(__VA_ARGS__);                                               \
  } while (0)
#define loginfo_f(...)                                                         \
  do {                                                                         \
    Serial.print("[INF][" + String(__FILENAME__) + "] -- ");                  \
    Serial.printf(__VA_ARGS__);                                                \
  } while (0)
#else
#define loginfo(...)                                                           \
  {}
#define loginfo_ln(...)                                                        \
  {}
#define loginfo_f(...)                                                         \
  {}
#endif

#if defined(CDP_LOG_DEBUG)
#define logdbg(...)                                                            \
  do {                                                                         \
    Serial.print("[DBG][" + String(__FILENAME__) + "] -- ");                   \
    Serial.println(__VA_ARGS__);                                               \
  } while (0)
#define logdbg_ln(...)                                                         \
  do {                                                                         \
    Serial.print("[DBG][" + String(__FILENAME__) + "] -- ");                   \
    Serial.println(__VA_ARGS__);                                               \
  } while (0)
#define logdbg_f(...)                                                          \
  do {                                                                         \
    Serial.print("[DBG][" + String(__FILENAME__) + "] -- ");                   \
    Serial.printf(__VA_ARGS__);                                                \
  } while (0)
#else
#define logdbg(...)                                                            \
  {}
#define logdbg_ln(...)                                                         \
  {}
#define logdbg_f(...)                                                          \
  {}
#endif
#endif
