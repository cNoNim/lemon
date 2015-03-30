#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static LogMask _log_mask = ALL_LOG;

static char const *levels[] = {
#define _log_APPLY(level) #level,
  _log_LOG_LEVELS(_log_APPLY)
#undef _log_APPLY
};

LogMask
log_mask(LogMask mask) {
  LogMask old = _log_mask;
  _log_mask = mask;
  return old;
}

void
log_level(LogLevel level) {
  _log_mask = level < 0 ? OFF_LOG : (LogMask)((1 << level + 1) - 1);
}

void
lprintf(LogLevel level, char const *restrict format, ...) {
  unsigned lvl = level;
  va_list ap;

  if (lvl >= _log_LOG_LEVEL_MAX)
    return;
  if ((_log_mask & (1 << lvl)) == 0)
    return;
  fprintf(stderr, "%s:", levels[lvl]);
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  fputs("\n", stderr);
  if (lvl == 0)
    exit(EXIT_FAILURE);
}