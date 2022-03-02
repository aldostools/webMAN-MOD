CELL_MK_DIR = $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

BUILD_TYPE     	= release

ROOT_PATH = ../..

LIBSTUB_DIR		= ./lib
PRX_DIR			= .
INSTALL			= cp
PEXPORTPICKUP		= ppu-lv2-prx-exportpickup
PRX_LDFLAGS_EXTRA	= -L./lib -Wl,--strip-unused-data

CRT_HEAD                += $(shell ppu-lv2-gcc -print-file-name'='ecrti.o)
CRT_HEAD                += $(shell ppu-lv2-gcc -print-file-name'='crtbegin.o)
CRT_TAIL                += $(shell ppu-lv2-gcc -print-file-name'='crtend.o)
CRT_HEAD                += $(shell ppu-lv2-gcc -print-file-name'='ecrtn.o)

PPU_SRCS = libc.c main.c 
PPU_PRX_TARGET = wm_proxy.prx
PPU_PRX_LDFLAGS += $(PRX_LDFLAGS_EXTRA)
PPU_PRX_STRIP_FLAGS = -s
PPU_PRX_LDLIBS 	= -lstdc_export_stub -lnet_stub -lio_stub

PPU_CFLAGS += -Os -fno-builtin-printf -nodefaultlibs -fmerge-all-constants -std=gnu99 -Wno-shadow -Wno-unused-parameter
#				-ffunction-sections -fdata-sections

CLEANFILES = $(PRX_DIR)/$(PPU_SPRX_TARGET)

all:
	$(MAKE) $(PPU_OBJS_DEPENDS)
	$(PPU_PRX_STRIP) --strip-debug --strip-section-header $(PPU_PRX_TARGET)
#	./make_fself -c $(PPU_PRX_TARGET) $(PPU_SPRX_TARGET)
	cp -r $(ROOT_PATH)/data data
#	$(ROOT_PATH)/scetool -0 SELF -1 TRUE -s FALSE -2 04 -3 1070000052000001 -4 01000002 -5 APP -6 0003004000000000 -A 0001000000000000 -e $(PPU_PRX_TARGET) $(PPU_SPRX_TARGET)
#	$(ROOT_PATH)/scetool -0 SELF --template=idle_plugin.sprx -e $(PPU_PRX_TARGET) $(PPU_SPRX_TARGET)

#	scetool --verbose --sce-type=SELF --compress-data=TRUE --key-revision=0004 --self-auth-id=1070000052000001 --self-vendor-id=01000002 --self-type=APP --self-app-version=0003004000000000 --self-ctrl-flags=4000000000000000000000000000000000000000000000000000000000000000 --self-cap-flags=00000000000000000000000000000000000000000000007B0000000100020000 --encrypt wm_proxy.prx wm_proxy.sprx
	$(ROOT_PATH)/scetool -0 SELF -1 TRUE -s FALSE -2 0A -3 1070000052000001 -4 01000002 -5 APP -6 0003004000000000 -A 0001000000000000 -8 4000000000000000000000000000000000000000000000000000000000000000 -9 00000000000000000000000000000000000000000000007B0000000100020000 -e $(PPU_PRX_TARGET) $(PPU_SPRX_TARGET)

	rm -r data objs $(PPU_PRX_TARGET) *.sym

include $(CELL_MK_DIR)/sdk.target.mk
