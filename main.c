/*
 * Copyright (C) 2015 173210 <root.3.173210@live.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <pspkernel.h>
#include <psprtc.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("CAPUSBPSP", PSP_MODULE_KERNEL, 0, 0);

SceUID thid = 0;

static int dbgPuts(const char *s)
{
	pspTime time;
	SceUID fd;
	char data[16];
	size_t size;
	int ret;

	size = strlen(s);

	fd = sceIoOpen("ms0:/PSP/LOG.TXT",
		PSP_O_WRONLY | PSP_O_APPEND | PSP_O_CREAT,
		0777);
	if (fd < 0)
		return fd;

	if (!sceRtcGetCurrentClockLocalTime(&time)) {
		sprintf(data, "[%02d:%02d.%06d] ",
			time.minutes, time.seconds, time.microseconds);
		sceIoWrite(fd, data, sizeof(data) - 1);
	}

	ret = sceIoWrite(fd, s, size);
	if (ret < 0)
		return ret;

	return sceIoClose(fd);
}

static int main()
{
	dbgPuts("Capture started\n");
	dbgPuts("Capture ended\n");

	return 0;
}

int module_start(SceSize arglen, void *argp)
{
	int ret;

	ret = sceKernelCreateThread(
		module_info.modname, main, 32, 1024, 0, NULL);
	if (ret < 0)
		return thid;
	thid = ret;

	ret = sceKernelStartThread(thid, arglen, argp);
	if (ret)
		sceKernelDeleteThread(thid);

	return ret;
}

int module_stop()
{
	return thid ? sceKernelTerminateDeleteThread(thid) : 0;
}
