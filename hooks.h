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
