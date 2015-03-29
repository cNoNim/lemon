#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void
ErrorMsg(const char *filename, int lineno, const char *format, ...) {
  va_list ap;
  fprintf(stderr, "%s:%d: ", filename, lineno);
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  fprintf(stderr, "\n");
}

/* Report an out-of-memory condition and abort.  This function
 * is used mostly by the "MemoryCheck" macro in struct.h
 */
void
memory_error() {
  fprintf(stderr, "Out of memory.  Aborting...\n");
  exit(1);
}
