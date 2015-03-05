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

PSP_MODULE_INFO("CAPUSBPSP", PSP_MODULE_KERNEL, 0, 0);

SceUID thid = 0;

static int main()
{
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
