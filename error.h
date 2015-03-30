#ifndef _LEMON_ERROR_H_
#define _LEMON_ERROR_H_

#include "log.h"

#define ErrorMsg(filename, lineno, format, ...) lprintf(LERROR, "%s:%d:"#format, filename, lineno, ## __VA_ARGS__)

#define MemoryCheck(X)                            \
  if ((X) == 0) {                                 \
    lprintf(LERROR, "Out of memory.  Aborting..."); \
  }

#endif //_LEMON_ERROR_H_
