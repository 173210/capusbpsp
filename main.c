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

#include <systemctrl.h>
#include <pspkernel.h>
#include <psprtc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("CAPUSBPSP", PSP_MODULE_KERNEL, 0, 0);

typedef struct {
	int32_t nid;
	const void *org;
	const void *hook;
} call_t;

typedef struct {
	const char *name;
	unsigned callsNum;
	call_t *calls;
} lib_t;

lib_t libs[] = {
};

static const char *modname = "sceUSB_Driver";
static const SceModule2 *module = NULL;


static SceUID thid = 0;

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

static int resolveCallsInLib(const struct SceLibraryEntryTable *ent, call_t *calls, unsigned callsNum)
{
	int32_t *p;
	call_t *call;

	if (ent == NULL || call == NULL)
		return SCE_KERNEL_ERROR_ERROR;

	for (p = ent->entrytable; p != ent->entrytable + ent->stubcount; p++)
		for (call = calls; call != calls + callsNum; call++)
			if (*p == call->nid)
				call->org = (void *)(*(p + ent->stubcount + ent->vstubcount));

	return 0;
}

static int resolveCallsInModule(const SceModule2 *module, const lib_t *libs, unsigned libsNum)
{
	const struct SceLibraryEntryTable *p;
	const lib_t *lib;

	if (module == NULL || libs == NULL)
		return SCE_KERNEL_ERROR_ERROR;

	for (p = module->ent_top; 
		(intptr_t)p < (intptr_t)module->ent_top + module->ent_size * 4;
		p = (void *)((intptr_t)p + p->len * 4))
		for (lib = libs; lib != libs + libsNum; lib++)
			if (!strcmp(p->libname, libs->name))
				resolveCallsInLib(p, libs->calls, libs->callsNum);

	return 0;
}

static int hookCallsInModule(const lib_t *libs, unsigned libsNum)
{
	const call_t *call;
	const lib_t *lib;
	void *p;
	const void **tbl;
	int size;

	if (libs == NULL)
		return -1;

	__asm__("cfc0 %0, $12" : "=r"(p));

	tbl = (const void **)p + 4;
	for (size = ((int *)p)[3] - 16; size > 0; size -= sizeof(void *)) {
		for (lib = libs; lib != libs + libsNum; lib++)
			for (call = lib->calls; call != lib->calls + lib->callsNum; call++)
				if (*tbl == call->org)
					*tbl = call->hook;
		tbl++;
	}

	return 0;
}

static int unhookCallInModule(const lib_t *libs, unsigned libsNum)
{
	const call_t *call;
	const lib_t *lib;
	void *p;
	const void **tbl;
	int size;

	if (libs == NULL)
		return -1;

	__asm__("cfc0 %0, $12" : "=r"(p));

	tbl = (const void **)p + 4;
	for (size = ((int *)p)[3] - 16; size > 0; size -= sizeof(void *)) {
		for (lib = libs; lib != libs + libsNum; lib++)
			for (call = lib->calls; call != lib->calls + lib->callsNum; call++)
				if (*tbl == call->hook)
					*tbl = call->org;
		tbl++;
	}

	return 0;
}

static int main()
{
	const unsigned libsNum = sizeof(libs) / sizeof(lib_t);
	call_t *call;
	void *p;

	do {
		sceKernelDelayThread(65536);
		module = (SceModule2 *)sceKernelFindModuleByName(modname);
	} while (module == NULL);

	resolveCallsInModule(module, libs, libsNum);
	hookCallsInModule(libs, libsNum);

	dbgPuts("capusbpsp Registered\n");

	return 0;
}

int module_start(SceSize arglen, void *argp)
{
	int ret;

	ret = sceKernelCreateThread(
		module_info.modname, main, 32, 2048, 0, NULL);
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
	if (thid)
		sceKernelTerminateDeleteThread(thid);

	if (module != NULL && module == (SceModule2 *)sceKernelFindModuleByName(modname))
		unhookCallInModule(libs, sizeof(libs) / sizeof(lib_t));

	dbgPuts("capusbpsp Unregistered\n");
	return 0;
}
