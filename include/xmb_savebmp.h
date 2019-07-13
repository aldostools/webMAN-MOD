/***********************************************************************
* pause/continue rsx fifo
*
* u8 pause    = pause fifo (1), continue fifo (0)
***********************************************************************/
#if defined(PS3_BROWSER) || defined(XMB_SCREENSHOT)
static s32 rsx_fifo_pause(u8 pause)
{
	// lv2 sys_rsx_context_attribute()
	system_call_6(0x2A2, 0x55555555ULL, (u64)(pause ? 2 : 3), 0, 0, 0, 0);

	return (s32)p1;
}
#endif

#ifdef XMB_SCREENSHOT

#include "../vsh/vsh.h"

#include <cell/font.h>

// canvas constants
#define BASE          0xC0000000UL     // local memory base ea

// get pixel offset into framebuffer by x/y coordinates
#define OFFSET(x, y) (u32)((((u32)offset) + ((((s16)x) + \
                     (((s16)y) * (((u32)pitch) / \
                     ((s32)4)))) * ((s32)4))) + (BASE))

#define _ES32(v)((u32)(((((u32)v) & 0xFF000000) >> 24) | \
		               ((((u32)v) & 0x00FF0000) >> 8 ) | \
		               ((((u32)v) & 0x0000FF00) << 8 ) | \
		               ((((u32)v) & 0x000000FF) << 24)))

// graphic buffers
typedef struct _Buffer {
	u32 *addr;               // buffer address
	s32  w;                   // buffer width
	s32  h;                   // buffer height
} Buffer;

// display values
static u32 unk1 = 0, offset = 0, pitch = 0;
static u32 h = 0, w = 0;

//static DrawCtx ctx;                                 // drawing context

// screenshot
u8 bmp_header[] = {
  0x42, 0x4D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


#ifndef __PAF_H__

extern u32 paf_F476E8AA(void);  //  u32 get_display_width
#define getDisplayWidth paf_F476E8AA

extern u32 paf_AC984A12(void);  // u32 get_display_height
#define getDisplayHeight paf_AC984A12

extern s32 paf_FFE0FBC9(u32 *pitch, u32 *unk1);  // unk1 = 0x12 color bit depth? ret 0
#define getDisplayPitch paf_FFE0FBC9

#endif // __PAF_H__


/***********************************************************************
*
***********************************************************************/
static void init_graphic(void)
{
	// get current display values
	offset = *(u32*)0x60201104;      // start offset of current framebuffer
	getDisplayPitch(&pitch, &unk1);       // framebuffer pitch size
	h = getDisplayHeight();               // display height
	w = getDisplayWidth();                // display width
}

static void saveBMP(char *path, bool notify_bmp)
{
	if(extcasecmp(path, ".bmp", 4))
	{
		// build file path
		CellRtcDateTime t;
		cellRtcGetCurrentClockLocalTime(&t);

		sprintf(path, "/dev_hdd0/screenshot_%02d_%02d_%02d_%02d_%02d_%02d.bmp", t.year, t.month, t.day, t.hour, t.minute, t.second);
	}

	filepath_check(path);

	// create bmp file
	int fd;
	if(IS_INGAME || cellFsOpen(path, CELL_FS_O_WRONLY|CELL_FS_O_CREAT|CELL_FS_O_TRUNC, &fd, NULL, 0) != CELL_FS_SUCCEEDED) { BEEP3 ; return;}

	// alloc buffers
	sys_memory_container_t mc_app = SYS_MEMORY_CONTAINER_NONE;
	mc_app = vsh_memory_container_by_id(1);

	const s32 mem_size = _64KB_; // 64 KB (bmp data and frame buffer)

	// max frame line size = 1920 pixel * 4(byte per pixel) = 7680 byte = 8 KB
	// max bmp buffer size = 1920 pixel * 3(byte per pixel) = 5760 byte = 6 KB

	sys_addr_t sysmem = NULL;
	if(sys_memory_allocate_from_container(mem_size, mc_app, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) != CELL_OK) return;

	{ BEEP2 }

	rsx_fifo_pause(1);

	// initialize graphic
	init_graphic();

	// calc buffer sizes
	u32 line_frame_size = (w * 4);

	// alloc buffers
	u64 *line_frame = (u64*)sysmem;
	u8 *bmp_buf = (u8*)sysmem + line_frame_size; // start offset: 8 KB


	// set bmp header
	u32 tmp = 0;
	tmp = _ES32(w*h*3+0x36);
	memcpy(bmp_header + 2 , &tmp, 4);     // file size
	tmp = _ES32(w);
	memcpy(bmp_header + 18, &tmp, 4);     // bmp width
	tmp = _ES32(h);
	memcpy(bmp_header + 22, &tmp, 4);     // bmp height
	tmp = _ES32(w*h*3);
	memcpy(bmp_header + 34, &tmp, 4);     // bmp data size

	// write bmp header
	cellFsWrite(fd, (void *)bmp_header, sizeof(bmp_header), NULL);

	u32 i, k, idx, ww = w/2;

	// dump...
	for(i = h; i > 0; i--)
	{
		for(k = 0; k < ww; k++)
			line_frame[k] = *(u64*)(OFFSET(k*2, i));

		// convert line from ARGB to RGB
		u8 *tmp_buf = (u8*)line_frame;

		idx = 0;

		for(k = 0; k < line_frame_size; k+=4, idx+=3)
		{
			bmp_buf[idx]   = tmp_buf[k + 3];  // R
			bmp_buf[idx+1] = tmp_buf[k + 2];  // G
			bmp_buf[idx+2] = tmp_buf[k + 1];  // B
		}

		// write bmp data
		cellFsWrite(fd, (void *)bmp_buf, idx, NULL);
	}

	// padding
	s32 rest = (w*3) % 4, pad = 0;
	if(rest)
		pad = 4 - rest;
	cellFsLseek(fd, pad, CELL_FS_SEEK_SET, 0);

	cellFsClose(fd);
	sys_memory_free((sys_addr_t)sysmem);

	// continue rsx rendering
	rsx_fifo_pause(0);

	if(notify_bmp) show_msg(path);
}

/*
#include "../vsh/system_plugin.h"

static void saveBMP()
{
	if(IS_ON_XMB) //XMB
	{
		system_interface = (system_plugin_interface *)plugin_GetInterface(View_Find("system_plugin"),1); // 1=regular xmb, 3=ingame xmb (doesnt work)

		CellRtcDateTime t;
		cellRtcGetCurrentClockLocalTime(&t);

		char bmp[0x50];
		sprintf(bmp, "/dev_hdd0/screenshot_%02d_%02d_%02d_%02d_%02d_%02d.bmp", t.year, t.month, t.day, t.hour, t.minute, t.second);

		rsx_fifo_pause(1);

		system_interface->saveBMP(bmp);

		rsx_fifo_pause(0);

		show_msg(bmp);
	}
}
*/

#endif
