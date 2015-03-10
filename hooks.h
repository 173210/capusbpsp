/*
 * Copyright (C) 2015 173210 <root.3.173210@live.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _HOOKS_H_
#define _HOOKS_H_

#include <stdint.h>

enum {
	CALL_sceUsbbdReqSend,
	CALL_sceUsbbdReqRecv,
	CALL_sceUsbbdClearFIFO,
	CALL_sceUsbbdRegister,
	CALL_sceUsbbdUnregister,
	CALL_sceUsbGetState,
	CALL_sceUsbbdReqCancelAll,
	CALL_sceUsbbdReqCancel,
	CALL_sceUsbbdStall,

	CALL_NUM
};

typedef struct {
	int32_t nid;
	const void *org;
	const void *hook;
} call_t;

extern call_t calls[];

#endif
