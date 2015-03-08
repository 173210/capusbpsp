#ifndef _LOG_H_
#define _LOG_H_

#include <psptypes.h>

void cupIoWork();

int cupPrintTime();
int cupPuts(const char *s);
int cupPrintf(const char *fmt, ...);

int cupIoWrite(const char *pre, const void *data, SceSize size);

#endif
