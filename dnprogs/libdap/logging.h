// libdap/logging.h
//
#include <stdarg.h>

void init_logging(char *, char, bool);
void daplog(int level, char *fmt, ...);

#define DAPLOG(x) daplog x
