#ifdef DUCKLOGGER_H
#define DUCKLOGGER_H

#define CDP_LOG_ERROR
#define CDP_LOG_INFO
#define CDP_LOG_DEBUG

#if defined(CDP_LOG_ERROR)
  #define logerr(...) { Serial.print(__VA_ARGS__); }
  #define logerr_ln(...) { Serial.println(__VA_ARGS__); }
#else
  #define logerr(...) {}
  #define logerr_ln(...) {}
#endif

#if defined(CDP_LOG_INFO)
  #define loginfo(...) { Serial.print(__VA_ARGS__); }
  #define loginfo_ln(...) { Serial.println(__VA_ARGS__); }
#else
  #define log_info(...) {}
  #define log_infoln(...) {}
#endif

#if defined(CDP_LOG_DEBUG)
  #define logdbg(...) { Serial.print(__VA_ARGS__); }
  #define logdbg_ln(...) { Serial.println(__VA_ARGS__); }
#else
  #define logdbg(...) {}
  #define logdbg_ln(...) {}
#endif
#endif
