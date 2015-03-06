TARGET = capusbpsp

OBJS = main.o io.o hooks.o
LIBS = -lpsprtc

CFLAGS = -O2 -G0 -Wall

BUILD_PRX = 1
PRX_EXPORTS = exports.exp

USE_PSPSDK_LIBC = 1

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
