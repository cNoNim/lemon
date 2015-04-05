#ifndef _LEMON_ERROR_H_
#define _LEMON_ERROR_H_

#include "log.h"

#define ErrorMsg(state, lineno, format, ...) do { \
    lprintf(LERROR, "%s:%d:"#format, (state)->filename, lineno, ## __VA_ARGS__); \
    (state)->errorcnt++; \
  } while(0)

#define MemoryCheck(X)                              \
  if (!(X)) {                                       \
    lprintf(LFATAL, "Out of memory.  Aborting..."); \
  }

#endif //_LEMON_ERROR_H_
