CELL_MK_DIR = $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

PEXPORTPICKUP = ppu-lv2-prx-exportpickup

CRT_HEAD += $(shell ppu-lv2-gcc -print-file-name'='ecrti.o)
CRT_HEAD += $(shell ppu-lv2-gcc -print-file-name'='crtbegin.o)
CRT_TAIL += $(shell ppu-lv2-gcc -print-file-name'='crtend.o)
CRT_HEAD += $(shell ppu-lv2-gcc -print-file-name'='ecrtn.o)

ROOT_PATH = ../..

PPU_SRCS = $(ROOT_PATH)/libc.c mem.c misc.c png_dec.c blitting.c main.c
PPU_INCDIRS	= -I$(ROOT_PATH)/vsh
PPU_PRX_TARGET = wm_vsh_menu.prx
PPU_PRX_LDFLAGS = -L$(ROOT_PATH)/lib -Wl,--strip-unused-data
PPU_PRX_STRIP_FLAGS = -s
PPU_PRX_LDLIBS 	+= -lfs_stub -lnet_stub -lrtc_stub -lio_stub -lnetctl_stub \
                   -lstdc_export_stub \
                   -lallocator_export_stub \
                   -lxsetting_export_stub \
                   -lpngdec_ppuonly_export_stub \
                   -lvsh_export_stub \
                   -lpaf_export_stub \
                   -lvshmain_export_stub \
                   -lsdk_export_stub \
                   -lsys_io_export_stub \
                   -lvshtask_export_stub \
                   -lnetctl_main_export_stub

PPU_CFLAGS += -Os -ffunction-sections -fdata-sections \
              -fno-builtin-printf -nodefaultlibs -std=gnu99 \
              -Wno-shadow -Wno-unused-parameter

CLEANFILES = ./$(PPU_SPRX_TARGET)

all:
	$(MAKE) $(PPU_OBJS_DEPENDS)
	$(PPU_PRX_STRIP) --strip-debug --strip-section-header $(PPU_PRX_TARGET)
#	$(MAKE_FSELF) $(PPU_PRX_TARGET) $(PPU_SPRX_TARGET)
	cp -r $(ROOT_PATH)/data data
	$(ROOT_PATH)/scetool -0 SELF -1 TRUE -s FALSE -2 0A -3 1070000052000001 -4 01000002 -5 APP -6 0003004000000000 -A 0001000000000000 -e $(PPU_PRX_TARGET) $(PPU_SPRX_TARGET)
	rm -r data objs $(PPU_PRX_TARGET) *.sym

include $(CELL_MK_DIR)/sdk.target.mk
