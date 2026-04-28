#ifndef DUCKLOGGER_H
#define DUCKLOGGER_H
#include "tinyformat.h"

#ifdef CDP_DEBUG
#define CDP_LOG_ERROR
#define CDP_LOG_INFO
#define CDP_LOG_DEBUG
#define CDP_LOG_WARN
#endif

#ifdef CDP_INFO
#define CDP_LOG_ERROR
#define CDP_LOG_INFO
#define CDP_LOG_WARN
#endif

#ifdef CDP_WARN
#define CDP_LOG_ERROR
#define CDP_LOG_WARN
#endif

#ifdef CDP_ERROR
#define CDP_LOG_ERROR
#endif

#ifndef __FILENAME__
#define __FILENAME__                                                           \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifdef CDP_LOG_ERROR
#define logerr(format, ...)                                     \
  do {                                                          \
    tfm::printf("[E][** %s : %d] ",__FILENAME__, __LINE__);          \
    tfm::printf(format, ##__VA_ARGS__);                         \
  } while (0)
#define logerr_ln(format, ...)                                  \
  do {                                                          \
    tfm::printf("[E][** %s : %d] ",__FILENAME__, __LINE__);          \
    tfm::printfln(format, ##__VA_ARGS__);        \
  } while (0)
#else
#define logerr(format, ...)                                     \
  {}
#define logerr_ln(format, ...)                                  \
  {}
#endif // CDP_LOG_ERROR


#ifdef CDP_LOG_WARN
#define logwarn(format, ...)                                    \
  do {                                                          \
    tfm::printf("[W][%s : %d] ",__FILENAME__, __LINE__);          \
    tfm::printf(format, ##__VA_ARGS__);                         \
  } while (0)

#define logwarn_ln(format, ...)                                 \
  do {                                                          \
    tfm::printf("[W][%s : %d] ",__FILENAME__, __LINE__);             \
    tfm::printfln(format"\n", ##__VA_ARGS__);          \
  } while (0)
#else
#define logwarn(format, ...)                                    \
  {}
#define logwarn_ln(format, ...)                                 \
  {}
#endif // CDP_LOG_WARN

#if defined(CDP_LOG_INFO)
#define loginfo(format, ...)                                    \
  do {                                                     \
    tfm::printf("[I][%s] ",__FILENAME__);                        \
    tfm::printf(format, ##__VA_ARGS__);                           \
  } while (0)

#define loginfo_ln(format, ...)                                 \
  do {                                                          \
    tfm::printf("[I][%s] ",__FILENAME__);                         \
    tfm::printfln(format, ##__VA_ARGS__);           \
  } while (0)
#else
#define loginfo(format, ...)                                    \
  {}
#define loginfo_ln(format, ...)                                 \
  {}
#endif // CDP_LOG_INFO

#ifdef CDP_LOG_DEBUG
#define logdbg(format, ...)                                     \
  do {                                                          \
    tfm::printf("[D][** %s : %d] ",__FILENAME__, __LINE__);                            \
    tfm::printf(format, ##__VA_ARGS__);                           \
  } while (0)

#define logdbg_ln(format, ...)                                  \
  do {                                                          \
    tfm::printf("[D][** %s : %d] ",__FILENAME__, __LINE__);          \
    tfm::printfln(format, ##__VA_ARGS__);          \
  } while (0)
#else
#define logdbg(format, ...)                                     \
  {}
#define logdbg_ln(format, ...)                                  \
  {}
#endif
#endif
