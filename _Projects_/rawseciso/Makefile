CELL_MK_DIR = $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

BUILD_TYPE     	= release

ROOT_PATH = ../..

LIBSTUB_DIR		= ../lib
PRX_DIR			= .
INSTALL			= cp
PEXPORTPICKUP		= ppu-lv2-prx-exportpickup
PRX_LDFLAGS_EXTRA	= -L../lib -Wl,--strip-unused-data

CRT_HEAD                += $(shell ppu-lv2-gcc -print-file-name'='ecrti.o)
CRT_HEAD                += $(shell ppu-lv2-gcc -print-file-name'='crtbegin.o)
CRT_TAIL                += $(shell ppu-lv2-gcc -print-file-name'='crtend.o)
CRT_HEAD                += $(shell ppu-lv2-gcc -print-file-name'='ecrtn.o)

PPU_SRCS = libc.c main.c printf.c asmcode.s
PPU_PRX_TARGET = raw_iso.prx
PPU_PRX_LDFLAGS += $(PRX_LDFLAGS_EXTRA)
PPU_PRX_STRIP_FLAGS = -s
PPU_PRX_LDLIBS 	+= -lfs_stub
PPU_CFLAGS += -Os -ffunction-sections -fdata-sections -fno-builtin-printf -nodefaultlibs -std=gnu99 -Wno-shadow -Wno-unused-parameter -Werror -Wno-inline

ifeq ($(BUILD_TYPE), debug)
PPU_CFLAGS += -DDEBUG
endif

CLEANFILES = $(PRX_DIR)/$(PPU_SPRX_TARGET)

all:
	$(MAKE) $(PPU_OBJS_DEPENDS)
	$(PPU_PRX_STRIP) --strip-debug --strip-section-header $(PPU_PRX_TARGET)
	$(MAKE_FSELF) $(PPU_PRX_TARGET) $(PPU_SPRX_TARGET)
#	scetool --sce-type=SELF --compress-data=TRUE --skip-sections=FALSE --key-revision=07 --self-ctrl-flags=4000000000000000000000000000000000000000000000000000000000000002 --self-auth-id=1010000001000003 --self-add-shdrs=TRUE --self-vendor-id=01000002 --self-app-version=0001000000000000 --self-type=APP --self-fw-version=0003005000000000 --encrypt "raw_iso.prx" "raw_iso.prx"
	cp -r $(ROOT_PATH)/data data
	$(ROOT_PATH)/scetool -0 SELF -1 TRUE -s FALSE -2 0A -3 1070000052000001 -4 01000002 -5 APP -6 0003004000000000 -A 0001000000000000 -e $(PPU_PRX_TARGET) $(PPU_SPRX_TARGET)
	rm -r data objs $(PPU_PRX_TARGET) *.sym

include $(CELL_MK_DIR)/sdk.target.mk
