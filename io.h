#ifndef _LOG_H_
#define _LOG_H_

#include <psptypes.h>

int logPuts(const char *s);
int logPrintf(const char *fmt, ...);

int cupIoInit(SceSize len, char *path);
int cupIoDeinit();
int cupIoWrite(const char *pre, void *data, SceSize size);
#endif
