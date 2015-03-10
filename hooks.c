#include <stdio.h>
#include <string.h>
#include <pspkernel.h>
#include <psprtc.h>
#include <pspusbbus.h>
#include "io.h"
#include "hooks.h"

typedef struct {
	uint8_t index;
	const char *name;
} name_t;

int (* _sceUsbbdReqRecvCB)(struct UsbdDeviceReq *, void *, int);

static const char *getNameOfbDecriptorType(unsigned bDecriptorType)
{
	static const char *names[] = {
		[0] = "UNKNOWN",
		[1] = "DEVICE",
		[2] = "CONFIGURATION",
		[3] = "STRING",
		[4] = "INTERFACE",
		[5] = "ENDPOINT"
	};

	return names[bDecriptorType < sizeof(names) / sizeof(char *) ? bDecriptorType : 0];
}

static const char *getNameOfbDeviceClass(unsigned bDeviceClass)
{
	static const name_t names[] = {
		{ 0, "Depends on Interface Descriptor" },
		{ 2, "Communication" },
		{ 9, "HUB" },
		{ 220, "Diagnostic Device" },
		{ 224, "Wireless" },
		{ 239, "Misc. Device" },
		{ 255, "Vendor-Specific" }
	};
	const name_t *p;

	for (p = names; p != names + sizeof(names) / sizeof(name_t); p++)
		if (p->index == bDeviceClass)
			return p->name;

	return "Unknown";
}

static const char *getNameOfbInterfaceClass(unsigned bInterfaceClass)
{
	static const name_t names[] = {
		{ 1, "Audio" },
		{ 2, "CDC-Control" },
		{ 3, "HID" },
		{ 5, "Physical" },
		{ 6, "Image" },
		{ 7, "Printer" },
		{ 8, "Mass-Storage" },
		{ 9, "HUB" },
		{ 10, "CDC-Data" },
		{ 11, "Chip/Smart Card" },
		{ 13, "Content-Security" },
		{ 14, "Video" },
		{ 220, "Diagnostic Device" },
		{ 224, "Wireless" },
		{ 254, "Application-Specific" },
		{ 255, "Vendor-Specific" }
	};
	const name_t *p;

	for (p = names; p != names + sizeof(names) / sizeof(name_t); p++)
		if (p->index == bInterfaceClass)
			return p->name;

	return "Unknown";
}

static int dumpDev(const char *func, const char *desc, const void *dev)
{
	const struct DeviceDescriptor *p;

	if (func == NULL || desc == NULL || dev == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;

	p = dev;
	cupPrintf("%s: GET_DESCRIPTOR(%s)\n", func, desc);
	cupPrintf("%s:  bLength = %d, .bDescriptorType = %d (%s)\n",
		func, p->bLength,
		p->bDescriptorType, getNameOfbDecriptorType(p->bDescriptorType));
	cupPrintf("%s:  bcdUSB = 0x%03X\n", func, p->bcdUSB);
	cupPrintf("%s:  bDeviceClass = 0x%02X (%s), .bDeviceSubClass = 0x%02X\n",
		func, p->bDeviceClass, getNameOfbDeviceClass(p->bDeviceClass),
		p->bDeviceSubClass);
	cupPrintf("%s:  bDeviceProtocol = 0x%02X\n",
		func, p->bDeviceProtocol);
	cupPrintf("%s:  bMaxPacketSize = %d\n",
		func, p->bMaxPacketSize);
	cupPrintf("%s:  idVendor = 0x%04X .idProduct = 0x%04X\n",
		func, p->idVendor, p->idProduct);
	cupPrintf("%s:  bcdDevice = 0x%03X\n", func, p->bcdDevice);
	cupPrintf("%s:  iManufacturer = %d\n", func, p->iManufacturer);
	cupPrintf("%s:  iProduct = %d\n", func, p->iProduct);
	cupPrintf("%s:  iSerialNumber = %d\n", func, p->iSerialNumber);

	cupPrintf("%s:  bNumConfigurations = %d\n",
		func, p->bNumConfigurations);

	return 0;
}

static int dumpConf(const char *func, const char *desc,
	const void *conf, unsigned bNumConfigurations)
{
	union {
		intptr_t i;
		const struct ConfigDescriptor *conf;
		const struct InterfaceDescriptor *intf;
		const struct EndpointDescriptor *endp;
	} p;
	unsigned i, j, k, bNumInterfaces, bNumEndpoints;

	if (func == NULL || desc == NULL || conf == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;

	p.conf = conf;
	for (i = 0; i < bNumConfigurations; i++) {
		cupPrintf("%s: GET_DESCRIPTOR(%s INDEX:%d)\n",
			func, desc, i);
		cupPrintf("%s:  bLength = %d, bDescriptorType = %d (%s)\n",
			func, p.conf->bLength, p.conf->bDescriptorType,
			getNameOfbDecriptorType(p.conf->bDescriptorType));
		cupPrintf("%s:  wTotalLength = %d\n",
			func, p.conf->wTotalLength);
		cupPrintf("%s:  bNumInterfaces = %d, bConfigurationValue = %d\n",
			func, p.conf->bNumInterfaces, p.conf->bConfigurationValue);
		cupPrintf("%s:  iConfiguration = %d\n",
			func, p.conf->iConfiguration);

		bNumInterfaces = p.conf->bNumInterfaces;
		p.i += p.conf->bLength;
		for (j = 0; j < bNumInterfaces; j++) {
			cupPrintf("%s:  INTERFACE INDEX:%d\n", func, j);
			cupPrintf("%s:   bLength = %d, bDescriptorType = %d (%s)\n",
				func, p.intf->bLength, p.intf->bDescriptorType,
				getNameOfbDecriptorType(p.intf->bDescriptorType));
			cupPrintf("%s:   bInterfaceNumber = %d, bAlternateSetting = %d\n",
				func, p.intf->bInterfaceNumber, p.intf->bAlternateSetting);
			cupPrintf("%s:   bNumEndpoints = %d\n",
				func, p.intf->bNumEndpoints);
			cupPrintf("%s:   bInterfaceClass = %d (%s), bInterfaceSubClass = %d\n",
				func, p.intf->bInterfaceClass,
				getNameOfbInterfaceClass(p.intf->bInterfaceClass),
				p.intf->bInterfaceSubClass);
			cupPrintf("%s:   bInterfaceProtocol = %d\n",
				func, p.intf->bInterfaceProtocol);
			cupPrintf("%s:   iInterface = %d\n",
				func, p.intf->iInterface);

			bNumEndpoints = p.intf->bNumEndpoints;
			p.i += p.intf->bLength;
			for (k = 0; k < bNumEndpoints; k++) {
				cupPrintf("%s:   ENDPOINT INDEX:%d", func, k);
				cupPrintf("%s:   bLength = %d, bDescriptorType = %d (%s)\n",
					func, p.endp->bLength, p.endp->bDescriptorType,
					getNameOfbDecriptorType(p.endp->bDescriptorType));
				cupPrintf("%s:   bEndpointAddress = 0x%02X, bmAtributes = 0x%02X\n",
					func, p.endp->bEndpointAddress,
					p.endp->bmAttributes);
				cupPrintf("%s:   wMaxPacketSize = %d\n",
					func, p.endp->wMaxPacketSize);
				cupPrintf("%s:   bInterval = %d\n",
					func, p.endp->bInterval);

				p.i += p.endp->bLength;
			}
		}
	}

	return 0;
}

static int hookUsbbdReqSend(struct UsbdDeviceReq *req)
{
	const char *f = "sceUsbbdReqSend";
	int (* _sceUsbbdReqSend)(struct UsbdDeviceReq *req);
	const char *p;
	int ret;

	if (req == NULL || req->endp == NULL || req->data == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;

	cupPrintf("%s: req = 0x%08X\n", f, (int)req);
	cupPrintf("%s: endpnum = %d, size = %d\n", f,
		req->endp->endpnum, req->size);

	cupIoWrite(f, req->data, req->size);

	_sceUsbbdReqSend = calls[CALL_sceUsbbdReqSend].org;
	ret = _sceUsbbdReqSend(req);

	switch (req->retcode) {
		case 0:
			p = "success";
			break;
		case -3:
			p = "cancelled";
			break;
		default:
			p = "unknown";
	}
	cupPrintf("%s: recvsize = %d, retcode = %d (%s)\n",
		f, req->recvsize, req->retcode, p);

	cupPrintf("%s: return = 0x%08X\n", f, ret);
	return ret;
}

static int hookUsbbdReqRecvCB(struct UsbdDeviceReq *req, void *a1, int a2)
{
	if (req == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;

	cupIoWrite("recv", req->data, req->recvsize);

	return _sceUsbbdReqRecvCB(req, a1, a2);
}

static int hookUsbbdReqRecv(struct UsbdDeviceReq *req)
{
	const char *f = "sceUsbbdReqRecv";
	int (* _sceUsbbdReqRecv)(struct UsbdDeviceReq *req);
	const char *p;
	int ret;

	if (req == NULL || req->endp == NULL || req->data == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;

	_sceUsbbdReqRecv = calls[CALL_sceUsbbdReqRecv].org;

	cupPrintf("%s: req = 0x%08X\n", f, (int)req);
	cupPrintf("%s: endpnum = %d, size = %d\n", f,
		req->endp->endpnum, req->size);

	_sceUsbbdReqRecvCB = req->func;
	req->func = hookUsbbdReqRecvCB;

	ret = _sceUsbbdReqRecv(req);

	req->func = _sceUsbbdReqRecvCB;

	switch (req->retcode) {
		case 0:
			p = "success";
			break;
		case -3:
			p = "cancelled";
			break;
		default:
			p = "unknown";
	}
	cupPrintf("%s: recvsize = %d, retcode = %d (%s)\n",
		f, req->recvsize, req->retcode, p);

	cupPrintf("%s: return = 0x%08X\n", f, ret);
	return ret;
}

static int hookUsbbdClearFIFO(struct UsbEndpoint *endp)
{
	const char *f = "sceUsbbdClearFIFO";
	int (* _sceUsbbdClearFIFO)(struct UsbEndpoint *endp);
	int ret;

	if (endp == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;

	cupPrintf("%s: endpnum = %d\n", f, endp->endpnum);

	_sceUsbbdClearFIFO = calls[CALL_sceUsbbdClearFIFO].org;
	ret = _sceUsbbdClearFIFO(endp);

	cupPrintf("%s: return = 0x%08X\n", f, ret);
	return ret;
}

static int hookUsbbdRegister(struct UsbDriver *drv)
{
	const char *f = "sceUsbbdRegister";
	int (* _sceUsbbdRegister)(struct UsbDriver *drv);
	int ret;

	if (drv == NULL || drv->devp_hi == NULL || drv->confp_hi == NULL
		|| drv->devp == NULL || drv->confp == NULL || drv->str == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;

	cupPrintf("%s: name = %s, endpoints = %d,\n",
		f, drv->name == NULL ? "NULL" : drv->name, drv->endpoints);

	dumpDev(f, "DEVICE (HI)", drv->devp_hi);
	dumpConf(f, "CONFIGURATION (HI)", drv->confp_hi,
		((struct DeviceDescriptor *)drv->devp_hi)->bNumConfigurations);

	dumpDev(f, "DEVICE", drv->devp);
	dumpConf(f, "CONFIGURATION", drv->confp,
		((struct DeviceDescriptor *)drv->devp)->bNumConfigurations);

	_sceUsbbdRegister = calls[CALL_sceUsbbdRegister].org;
	ret = _sceUsbbdRegister(drv);

	cupPrintf("%s: return = 0x%08X\n", f, ret);
	return ret;
}

static int hookUsbbdUnregister(struct UsbDriver *drv)
{
	const char *f = "sceUsbbdUnregister";
	int (* _sceUsbbdUnregister)(struct UsbDriver *drv);
	int ret;

	cupPrintf("%s: name = %s", f, drv->name == NULL ? "NULL" : drv->name);

	_sceUsbbdUnregister = calls[CALL_sceUsbbdUnregister].org;
	ret = _sceUsbbdUnregister(drv);

	cupPrintf("%s: return = 0x%08X\n", f, ret);
	return ret;
}

static int hookUsbGetState(const char *driverName)
{
	const char *f = "sceUsbGetState";
	int (* _sceUsbGetState)(const char *driverName);
	int ret;

	cupPrintf("%s: driverName = %s\n",
		f, driverName == NULL ? "NULL" : driverName);

	_sceUsbGetState = calls[CALL_sceUsbGetState].org;
	ret = _sceUsbGetState(driverName);

	cupPrintf("%s: return = 0x%08X\n", f, ret);
	return ret;
}

static int hookUsbbdReqCancelAll(struct UsbEndpoint *endp)
{
	const char *f = "sceUsbbdReqCancelAll";
	int (* _sceUsbbdReqCancelAll)(struct UsbEndpoint *endp);
	int ret;

	if (endp == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;

	cupPrintf("%s: endpnum = %d\n", f, endp->endpnum);

	_sceUsbbdReqCancelAll = calls[CALL_sceUsbbdReqCancelAll].org;
	ret = _sceUsbbdReqCancelAll(endp);

	cupPrintf("%s: return = 0x%08X\n", f, ret);
	return ret;
}

static int hookUsbbdReqCancel(struct UsbdDeviceReq *req)
{
	const char *f = "sceUsbbdReqCancel";
	int (* _sceUsbbdReqCancel)(struct UsbdDeviceReq *req);
	int ret;

	cupPrintf("%s: req = 0x%08X\n", f, (intptr_t)req);

	_sceUsbbdReqCancel = calls[CALL_sceUsbbdReqCancel].org;
	ret = _sceUsbbdReqCancel(req);

	cupPrintf("%s: return = 0x%08X\n", f, ret);
	return ret;
}

static int hookUsbbdStall(struct UsbEndpoint *endp)
{
	const char *f = "sceUsbbdReqCancelAll";
	int (* _sceUsbbdStall)(struct UsbEndpoint *endp);
	int ret;

	if (endp == NULL)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;

	cupPrintf("%s: endpnum = %d\n", f, endp->endpnum);

	_sceUsbbdStall = calls[CALL_sceUsbbdStall].org;
	ret = _sceUsbbdStall(endp);

	cupPrintf("%s: return = 0x%08X\n", f, ret);
	return ret;
}

call_t calls[] = {
	[CALL_sceUsbbdReqSend] = {
		.nid = 0x23E51D8F,
		.org = NULL,
		.hook = hookUsbbdReqSend
	},
	[CALL_sceUsbbdReqRecv] = {
		.nid = 0x913EC15D,
		.org = NULL,
		.hook = hookUsbbdReqRecv
	},
	[CALL_sceUsbbdClearFIFO] = {
		.nid = 0x951A24CC,
		.org = NULL,
		.hook = hookUsbbdClearFIFO
	},
	[CALL_sceUsbbdRegister] = {
		.nid = 0xB1644BE7,
		.org = NULL,
		.hook = hookUsbbdRegister
	},
	[CALL_sceUsbbdUnregister] = {
		.nid = 0xC1E2A540,
		.org = NULL,
		.hook = hookUsbbdUnregister
	},
	[CALL_sceUsbGetState] = {
		.nid = 0xC21645A4,
		.org = NULL,
		.hook = hookUsbGetState
	},
	[CALL_sceUsbbdReqCancelAll] = {
		.nid = 0xC5E53685,
		.org = NULL,
		.hook = hookUsbbdReqCancelAll
	},
	[CALL_sceUsbbdReqCancel] = {
		.nid = 0xCC57EC9D,
		.org = NULL,
		.hook = hookUsbbdReqCancel
	},
	[CALL_sceUsbbdStall] = {
		.nid = 0xE65441C1,
		.org = NULL,
		.hook = hookUsbbdStall
	}
};
