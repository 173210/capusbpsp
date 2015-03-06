#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <pspkernel.h>
#include <psprtc.h>
#include "log.h"

static SceUID logOpen()
{
	pspTime time;
	SceUID fd;
	char data[16];

	fd = sceIoOpen("LOG.TXT",
		PSP_O_WRONLY | PSP_O_APPEND | PSP_O_CREAT,
		0777);

	if (fd >= 0 && !sceRtcGetCurrentClockLocalTime(&time)) {
		sprintf(data, "[%02d:%02d.%06d] ",
			time.minutes, time.seconds, time.microseconds);
		sceIoWrite(fd, data, sizeof(data) - 1);
	}

	return fd;
}

static int logItoa(unsigned val, char *buf, unsigned base, unsigned w)
{
	char *p1, *p2;
	char c;
	int i = 0;
	int rem;

	if (buf == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;

	do {
		rem = val % base;
		buf[i++] = (rem < 10 ? 0x30 : 0x37) + rem;
		val /= base;
		w--;
	} while (val || w > 0);

	p1 = buf;
	p2 = buf + i - 1;
	while (p1 < p2) {
		c = *p2;
		*p2-- = *p1;
		*p1++ = c;
	}

	return i;
}

int logPuts(const char *s)
{
	SceUID fd;
	size_t size;
	int ret;

	if (s == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;

	size = strlen(s);

	fd = logOpen();
	if (fd < 0)
		return fd;

	ret = sceIoWrite(fd, s, size);
	if (ret < 0)
		return ret;

	return sceIoClose(fd);
}

void logPrintf(const char *fmt, ...)
{
	SceUID fd;
	va_list va;
	const char *p;
	char buf[11];
	char c, w;
	int i, val;

	if (fmt == NULL)
		return;

	va_start(va, fmt);
	fd = logOpen();

	while ((c = *fmt) != '\0') {
		if (c != '%') {
			p = fmt;

			for (i = 1; fmt[i] != '%' && fmt[i]; i++);
			fmt += i;
		} else {
			fmt++;
			c = *fmt++;

			w = 0;
			if (c == '0') {
				c = *fmt++;
				if (c >= '0' && c <= '8') {
					w = c - '0';
					c = *fmt++;
				}
			}

			switch (c) {
				case 'd':
					p = buf;

					val = va_arg(va, int);
					if (val < 0) {
						buf[0] = '-';
						val= -val;
						i = logItoa((unsigned)val, buf + 1, 10, w);
					} else
						i = logItoa((unsigned)val, buf, 10, w);
					break;
				case 'X' : 
					p = buf;
					i = logItoa((unsigned)va_arg(va, int), buf, 16, w);
					break;
				case 's' :
					p = va_arg(va, const char *);
					i = strlen(p);
					break;
				default:
					p = NULL;
					i = 0;
			}
		}

		sceIoWrite(fd, p, i);
	}

	va_end(va);
	sceIoClose(fd);
}
