#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <pspkernel.h>
#include <psprtc.h>
#include "io.h"

typedef struct _queue_t queue_t;

struct _queue_t {
	queue_t *next;
	SceUID blockid;
	size_t size;
};

static SceUID cupIoThid = 0;

static queue_t *queue = NULL;

static size_t left = 0;
static char buf[4096];

static int cupIoThread(SceSize args, void *argp)
{
	SceUID log = -1;
	SceUID fd;
	const char *p;
	int ret;

	ret = sceIoChdir(argp);
	if (ret)
		return ret;

	while (1) {
		sceKernelSleepThread();

		if (left) {
			if (log < 0)
				log = sceIoOpen("LOG.TXT",
					PSP_O_WRONLY | PSP_O_APPEND | PSP_O_CREAT,
					0777);
			if (log >= 0) {
				sceIoWrite(log, buf, left);
				left = 0;

				if (!sceIoClose(log))
					log = -1;
			}
		}

		while (queue != NULL) {
			p = (char *)(queue + 1);
			fd = sceIoOpen(p,
				PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC,
				0777);
			if (fd >= 0) {
				p += strlen(p) + 1;
				sceIoWrite(fd, p, queue->size);
				sceIoClose(fd);
			}

			sceKernelFreePartitionMemory(queue->blockid);
			queue = queue->next;
		}
	}
}

int logTime()
{
	int ret;
	pspTime time;

	if (left >= sizeof(buf))
		return 0;

	ret = sceRtcGetCurrentClockLocalTime(&time);
	if (!ret) {
		ret = snprintf(buf + left, sizeof(buf) - left,
			"[%02d:%02d.%06d] ",
			time.minutes, time.seconds, time.microseconds);
		if (ret > 0)
			left += ret;
	}

	return ret;
}

int cupPuts(const char *s)
{
	char *p;

	if (s == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;

	if (left >= sizeof(buf))
		return SCE_KERNEL_ERROR_NO_MEMORY;

	logTime();

	p = buf + left;
	while (*s && left < sizeof(buf)) {
		*p = *s;
		p++;
		s++;
		left++;
	}

	sceKernelWakeupThread(cupIoThid);

	return 0;
}

int cupPrintf(const char *fmt, ...)
{
	va_list va;
	int ret;

	if (fmt == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;

	if (left >= sizeof(buf))
		return 0;

	logTime();

	va_start(va, fmt);

	ret = vsnprintf(buf + left, sizeof(buf) - left, fmt, va);
	if (ret > 0)
		left += ret;

	va_end(va);

	sceKernelWakeupThread(cupIoThid);

	return ret;
}

int cupIoInit(SceSize len, char *path)
{
	int ret;

	ret = sceKernelCreateThread(
		"cupIoThread", cupIoThread, 16, 2048, 0, NULL);
	if (ret < 0)
		return cupIoThid;
	cupIoThid = ret;

	ret = sceKernelStartThread(cupIoThid, len, path);
	if (ret)
		sceKernelDeleteThread(cupIoThid);

	cupPrintf("capusbpsp started\n");

	return ret;
}

int cupIoDeinit()
{
	return cupIoThid ? sceKernelTerminateDeleteThread(cupIoThid) : 0;
}

int cupIoWrite(const char *pre, void *data, SceSize size)
{
	pspTime time;
	SceUID blockid;
	size_t len;
	char *p;

	if (pre == NULL || data == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;

	len = strlen(pre);

	blockid = sceKernelAllocPartitionMemory(PSP_MEMORY_PARTITION_KERNEL,
		"cupIo", PSP_SMEM_Low, sizeof(queue_t) + len + 16 + size, NULL);
	if (blockid < 0)
		return blockid;

	p = sceKernelGetBlockHeadAddr(blockid);
	if (p == NULL)
		return sceKernelFreePartitionMemory(blockid);

	((queue_t *)p)->next = queue;
	queue = (queue_t *)p;

	queue->blockid = blockid;
	queue->size = size;
	p += sizeof(queue_t);

	sceRtcGetCurrentClockLocalTime(&time);

	len = sprintf(p, "%s_%02d%02d%06d.BIN", pre,
		time.minutes, time.seconds, time.microseconds);

	memcpy(p + len + 1, data, size);

	sceKernelWakeupThread(cupIoThid);

	return 0;
}
