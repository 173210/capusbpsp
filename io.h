#ifndef _LOG_H_
#define _LOG_H_

#include <psptypes.h>

int cupPuts(const char *s);
int cupPrintf(const char *fmt, ...);

int cupIoInit(SceSize len, char *path);
int cupIoDeinit();
int cupIoWrite(const char *pre, const void *data, SceSize size);
#endif
