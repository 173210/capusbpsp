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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <systemctrl.h>
#include <pspkernel.h>
#include "io.h"
#include "hooks.h"

PSP_MODULE_INFO("CAPUSBPSP", PSP_MODULE_KERNEL, 0, 0);

typedef struct {
	char *name;
	uint32_t flags;
	uint8_t funcNum;
	uint8_t varNum;
	int32_t *nids;
	int32_t *imports;
} stub_t;

static const int32_t J_OPCODE = 0x08000000;

static char modname[32];
static const SceModule2 *module = NULL;
static const stub_t *stub = NULL;

static SceUID mainThid = 0;

static int32_t jAsm(const void *p)
{
	return (intptr_t)p & 3 ? SCE_KERNEL_ERROR_ILLEGAL_ADDRESS :
		J_OPCODE | (((intptr_t)p >> 2) & 0x03FFFFFF);
}

static void *getPtrWithJ(int32_t instr)
{
	return (instr & 0xFC000000) == J_OPCODE ?
		(void *)(((instr & 0x03FFFFFF) << 2) | 0x80000000) :
		NULL;
}

static stub_t *findStub(const SceModule2 *mod, const char *name)
{
	const stub_t *stub;

	if (mod != NULL && mod->stub_top != NULL && name != NULL)
		for (stub = mod->stub_top;
			stub != mod->stub_top + mod->stub_size / sizeof(stub_t);
			stub++)
			if (!strcmp(stub->name, name))
				return (stub_t *)stub;

	return NULL;
}

static int hook(const stub_t *stub, call_t *calls, unsigned callsNum)
{
	int32_t *nid, *import;
	call_t *call;

	if (stub == NULL || stub->nids == NULL || stub->imports == NULL
		|| calls == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;

	import = stub->imports;
	for (nid = stub->nids; nid != stub->nids + stub->funcNum; nid++) {
		for (call = calls; call != calls + callsNum; call++)
			if (call->nid == *nid) {
				call->org = getPtrWithJ(*import);
				*import = jAsm(call->hook);
				break;
			}
		import += 2;
	}

	return 0;
}

static int unhook(const stub_t *stub, call_t *calls, unsigned callsNum)
{
	int32_t *nid, *import;
	call_t *call;

	if (stub == NULL || calls == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;

	import = stub->imports;
	for (nid = stub->nids; nid != stub->nids + stub->funcNum; nid++) {
		for (call = calls; call != calls + callsNum; call++)
			if (call->nid == *nid && call->org != NULL) {
				*import = jAsm(call->org);
				break;
			}
		import++;
	}

	return 0;
}

int mainThread(SceSize args, void *argp)
{
	char *p;
	SceUID fd;
	int ret;

	p = strrchr(argp, '/');
	if (p != NULL)
		*p = 0;

	ret = sceIoChdir(argp);
	if (ret)
		goto exit;

	fd = sceIoOpen("TARGET.TXT", PSP_O_RDONLY, 0777);
	if (fd < 0) {
		ret = fd;
		goto exit;
	}

	ret = sceIoRead(fd, modname, sizeof(modname));
	if (ret < 0)
		goto exit;

	sceIoClose(fd);

	for (p = modname; p != modname + sizeof(modname); p++)
		if (*p == 0 || *p == '\r' || *p == '\n') {
			*p = 0;
			break;
		}

	do {
		module = (SceModule2 *)sceKernelFindModuleByName(modname);
	} while (module == NULL);

	stub = findStub(module, "sceUsbBus_driver");
	
	if (stub == NULL)
		ret = SCE_KERNEL_ERROR_ERROR;
	else {
		ret = hook(stub, calls, CALL_NUM);
		cupIoInit(args, argp);
	}

exit:
	sceKernelExitDeleteThread(ret);
	return ret;
}

int module_start(SceSize args, void *argp)
{
	int ret;

	ret = sceKernelCreateThread(
		module_info.modname, mainThread, 111, 2048, 0, NULL);
	if (ret < 0)
		return ret;
	mainThid = ret;

	ret = sceKernelStartThread(mainThid, args, argp);
	if (ret)
		sceKernelDeleteThread(mainThid);

	return ret;
}

int module_stop()
{
	if (mainThid)
		sceKernelTerminateDeleteThread(mainThid);

	cupIoDeinit();

	if (module == (SceModule2 *)sceKernelFindModuleByName(modname)
		&& stub == findStub(module, "sceUsbBus_driver"))
		unhook(stub, calls, CALL_NUM);

	return 0;
}
