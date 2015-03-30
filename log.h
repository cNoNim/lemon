#ifndef LEMON_LOG_H
#define LEMON_LOG_H

#define _log_LOG_LEVELS(_log_APPLY) \
  _log_APPLY(FATAL)                 \
  _log_APPLY(ERROR)                 \
  _log_APPLY(WARN)                  \
  _log_APPLY(INFO)                  \
  _log_APPLY(DEBUG)                 \
  _log_APPLY(TRACE)

typedef enum LogLevel {
#define _log_APPLY(level) L##level,
  _log_LOG_LEVELS(_log_APPLY)
#undef _log_APPLY
  _log_LOG_LEVEL_MAX
} LogLevel;

typedef enum LogMask {
  OFF_LOG,
#define _log_APPLY(level) level##_LOG = 1 << L##level,
  _log_LOG_LEVELS(_log_APPLY)
#undef _log_APPLY
  ALL_LOG = -1
} LogMask;

LogMask log_mask(LogMask mask);
void log_level(LogLevel level);
void lprintf(LogLevel level, char const *restrict format, ...);

#endif // LEMON_LOG_H
