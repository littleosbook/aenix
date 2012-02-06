#ifndef LOG_H
#define LOG_H

#include "stdarg.h"

void log_debug(char *fname, char *fmt, ...);
void log_info(char *fname, char *fmt, ...);
void log_error(char *fname, char *fmt, ...);

#endif /* LOG_H */
