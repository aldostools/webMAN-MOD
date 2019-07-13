#include "include/pad.h"
#include "include/network.h"

#include "include/misc.h"
#include "include/mem.h"
#include "include/blitting.h"

SYS_MODULE_INFO(sLaunch, 0, 1, 1);
SYS_MODULE_START(slaunch_start);
SYS_MODULE_STOP(slaunch_stop);

#define THREAD_NAME         "slaunch_thread"
#define STOP_THREAD_NAME    "slaunch_stop_thread"

#define STR_UNMOUNT		"Unmount"
#define STR_REFRESH		"Refresh"
#define STR_GAMEDATA	"gameDATA"
#define STR_SYSCALLS	"CFW Syscalls"
#define STR_RESTART		"Restart"
#define STR_SHUTDOWN	"Shutdown"
#define STR_SETUP		"Setup"
#define STR_FILEMNGR	"File Manager"
#define STR_UNLOAD		"Unload webMAN"
#define STR_QUIT		"Quit"

#define APP_VERSION		"1.11"

typedef struct {
	uint8_t  gmode;
	uint8_t  dmode;
	uint8_t  gpp;
	uint8_t  fav_mode;
	uint16_t cur_game;
	uint16_t cur_game_;
	uint16_t fav_game;
	uint16_t padd0;
	uint64_t padd1;
} _sconfig;

#define IS_ON_XMB			(vshmain_EB757101() == 0)
#define IS_INGAME			(vshmain_EB757101())

#define MAX_GAMES 2000
#define WMTMP				"/dev_hdd0/tmp/wmtmp"				// webMAN work/temp folder
#define WM_ICONS_PATH		"/dev_hdd0/tmp/wm_icons"			// webMAN icons folder
#define RB_ICONS_PATH		"/dev_flash/vsh/resource/explore/icon"

#define XMLMANPLS_DIR		"/dev_hdd0/game/XMBMANPLS"
#define XMLMANPLS_IMAGES_DIR XMLMANPLS_DIR "/USRDIR/IMAGES"

#define TYPE_ALL 0
#define TYPE_PSX 1
#define TYPE_PS2 2
#define TYPE_PS3 3
#define TYPE_PSP 4
#define TYPE_VID 5
#define TYPE_ROM 6
#define TYPE_MAX 7

static char game_type[TYPE_MAX][8]=
{
	"\0",
	"PSX",
	"PS2",
	"PS3",
	"PSP",
	"video",
	"ROMS"
};

static char wm_icons[7][56] =	{
									RB_ICONS_PATH "/icon_wm_ps3.png",       //020.png  [0]
									RB_ICONS_PATH "/icon_wm_psx.png",       //021.png  [1]
									RB_ICONS_PATH "/icon_wm_ps2.png",       //022.png  [2]
									RB_ICONS_PATH "/icon_wm_ps3.png",       //023.png  [3]
									RB_ICONS_PATH "/icon_wm_psp.png",       //024.png  [4]
									RB_ICONS_PATH "/icon_wm_dvd.png",       //025.png  [5] (vid)
									RB_ICONS_PATH "/icon_wm_dvd.png",       //026.png  [6] (roms)
								};

#define SLIST	"slist"
/*
typedef struct // 1MB for 2000+1 titles
{
	uint8_t type;
	char id[10];
	char name[141]; // 128+12+1 for added ' [BXXX12345]'
	char icon[160];
	char path[160];
	char padd[52];
} __attribute__((packed)) _slaunch;
*/

typedef struct // 1MB for 2000+1 titles
{
	uint8_t  type;
	char     id[10];
	uint8_t  path_pos; // start position of path
	uint16_t icon_pos; // start position of icon
	uint16_t padd;
	char     name[508]; // name + path + icon
} __attribute__((packed)) _slaunch;

static _slaunch *slaunch = NULL;

#define GRAY_TEXT   0xff808080
#define LIGHT_TEXT  0xffa0a0a0
#define WHITE_TEXT  0xffc0c0c0
#define BRIGHT_TEXT 0xffe0e0e0

#define RED         0xffc00000ffc00000
#define BLUE        0xff0000ffff0000ff
#define DARK_RED    0xff500000ff500000
#define DARK_BLUE   0xff000050ff000050
#define DARK_GRAY   0xff606060ff606060
#define GRAY        0xff808080ff808080
#define LIGHT_GRAY  0xffa0a0a0ffa0a0a0

// globals
static uint16_t init_delay=0;
static uint16_t games = 0;
static uint16_t cur_game=0, cur_game_=0, fav_game=0;
static uint8_t do_once = 1;

uint32_t disp_w=0;
uint32_t disp_h=0;
uint32_t gpp=10;

uint8_t web_page=0;

static uint8_t key_repeat=0, can_skip=0;

static uint8_t opt_mode=0;
static uint8_t unload_mode=1;

static uint64_t tick=0x80;
static int8_t   delta=5;

#define SYS_PPU_THREAD_NONE        (sys_ppu_thread_t)NONE

static sys_ppu_thread_t slaunch_tid = SYS_PPU_THREAD_NONE;
static int32_t running = 1;
static uint8_t slaunch_running = 0;	// vsh menu off[0] or on[1]
static uint8_t fav_mode = 0;

static void return_to_xmb(void);
int32_t slaunch_start(uint64_t arg);
int32_t slaunch_stop(void);

static void finalize_module(void);
static void slaunch_stop_thread(uint64_t arg);
static void slaunch_thread(uint64_t arg);

static void draw_selection(uint16_t game_idx);

#define HDD0  1
#define NTFS  2

#define DEVS_MAX 4

static char drives[DEVS_MAX][12] = {"dev_hdd0", "ntfs", "dev_usb", "net"};

static uint8_t gmode = TYPE_ALL;
static uint8_t dmode = TYPE_ALL;
static uint8_t cpu_rsx = 0;

static uint32_t frame = 0;

static void load_background(void)
{
	char path[64];

	if(fav_mode)
		sprintf(path, "%s_fav.jpg", "/dev_hdd0/tmp/wm_res/images/slaunch");
	else if(gmode)
	{
		sprintf(path, "%s_%s.jpg", "/dev_hdd0/tmp/wm_res/images/slaunch", game_type[gmode]);

		if(!gmode || !file_exists(path)) sprintf(path, "%s.jpg", "/dev_hdd0/plugins/images/slaunch");
	}

	if(file_exists(path) == false)
	{
		if(fav_mode)
			sprintf(path, "/dev_flash/vsh/resource/explore/icon/cinfo-bg-%s","whatsnew.jpg");
		else if(!gmode)
			sprintf(path, "/dev_flash/vsh/resource/explore/icon/cinfo-bg-%s","storemain.jpg");
		else
			sprintf(path, "/dev_flash/vsh/resource/explore/icon/cinfo-bg-%s","storegame.jpg");
	}

	load_img_bitmap(0, path, "\0");
}

static void draw_page(uint16_t game_idx, uint8_t key_repeat)
{
	if(game_idx>=games) game_idx=0;

	if(!games) return;

	const int left_margin=56;

	uint8_t slot = 0;
	uint16_t i, j;
	int px=left_margin, py=90;	// top-left

	// draw background and menu strip
	flip_frame((uint64_t*)ctx.canvas);
	memcpy((uint8_t *)ctx.menu, (uint8_t *)(ctx.canvas)+INFOBAR_Y*CANVAS_W*4, CANVAS_W*INFOBAR_H*4);

	set_textbox(LIGHT_GRAY, 0, INFOBAR_Y - 10, CANVAS_W, 1);
	set_textbox(GRAY,       0, INFOBAR_Y - 9,  CANVAS_W, 1);
	set_textbox(DARK_GRAY,  0, INFOBAR_Y - 8,  CANVAS_W, 1);

	set_textbox(DARK_GRAY,  0, INFOBAR_Y + INFOBAR_H + 2, CANVAS_W, 1);
	set_textbox(GRAY,       0, INFOBAR_Y + INFOBAR_H + 3, CANVAS_W, 1);
	set_textbox(LIGHT_GRAY, 0, INFOBAR_Y + INFOBAR_H + 4, CANVAS_W, 1);

	if(disp_h<720) gpp=10;

	// draw game icons (5x2) or (10x4)
	j=(game_idx/gpp)*gpp;
	for(i=j;((slot<gpp)&&(i<games));i++)
	{
		slot++;

		if(load_img_bitmap(slot, slaunch[i].name + slaunch[i].icon_pos, wm_icons[slaunch[i].type])<0) break;

		if(gpp==10)
		{
			py=((i-j)/5)*400+90+(300-ctx.img[slot].h)/2;
			ctx.img[slot].x=((px+(320-ctx.img[slot].w)/2)/2)*2;
			ctx.img[slot].y=py;
			set_backdrop(slot, 0);
			set_texture(slot, ctx.img[slot].x, ctx.img[slot].y);
			px+=(320+left_margin); if(px>1600) px=left_margin;
		}
		else
		{
			py=((i-j)/10)*200+90+(150-ctx.img[slot].h)/2;
			ctx.img[slot].x=((px+(160-ctx.img[slot].w)/2)/2)*2;
			ctx.img[slot].y=py;
			set_backdrop(slot, 0);
			set_texture(slot, ctx.img[slot].x, ctx.img[slot].y);
			px+=(160+24); if(px>1800) px=left_margin;
		}

		if(key_repeat) break;
	}
	draw_selection(game_idx);
}

static void draw_selection(uint16_t game_idx)
{
	char one_of[32], mode[8];

	if(games)
	{
		char *path = slaunch[game_idx].name + slaunch[game_idx].path_pos;

		// game name
		if(ISHD(disp_w))	set_font(32.f, 32.f, 1.0f, 1);
		else				set_font(32.f, 32.f, 3.0f, 1);
		ctx.fg_color=WHITE_TEXT;
		print_text(ctx.menu, CANVAS_W, CENTER_TEXT, 0, slaunch[game_idx].name );


		float s = (float)(int)(strlen(path)/8); if(s>30) s=30;

		// game path
		if(ISHD(disp_w))	set_font(36.f-s, 16.f, 1.0f, 1);
		else				set_font(42.f-s, 16.f, 2.0f, 1);
		ctx.fg_color=GRAY_TEXT;

		if(*path == '/' && path[10] == '/') path += 10;
		if(*path == '/') print_text(ctx.menu, CANVAS_W, CENTER_TEXT, 40, path);
	}

	// game index
	if(ISHD(disp_w))	set_font(20.f, 20.f, 1.0f, 1);
	else				set_font(32.f, 20.f, 2.0f, 1);
	ctx.fg_color=LIGHT_TEXT;
	sprintf(one_of, "%i / %i", game_idx+1, games);
	print_text(ctx.menu, CANVAS_W, CENTER_TEXT, 64, one_of );

	// game list mode
	if(gmode) sprintf(mode, "%s", game_type[gmode]);

	if(fav_mode)
		print_text(ctx.menu, CANVAS_W, 80, 64, "Favorites");
	else if(dmode && gmode)
	{
		sprintf(one_of, "/%s/%s", drives[dmode-1], mode);
		print_text(ctx.menu, CANVAS_W, 80, 64, one_of);
	}
	else if(dmode)
		print_text(ctx.menu, CANVAS_W, 80, 64, drives[dmode-1]);
	else if(gmode)
		print_text(ctx.menu, CANVAS_W, 80, 64, mode);

	// temperature
	if(ISHD(disp_w) || (disp_h==720))
	{
		char s_temp[64];
		uint32_t temp_c = 0, temp_f = 0;
		get_temperature(cpu_rsx, &temp_c);
		temp_f = (uint32_t)(1.8f * (float)temp_c + 32.f);
		sprintf(s_temp, "%s :  %i C  /  %i F", cpu_rsx ? "RSX" : "CPU", temp_c, temp_f);
		print_text(ctx.menu, CANVAS_W, CANVAS_W - ((disp_h==720) ? 450 : 300), 64, s_temp);
	}

	// set frame buffer for menu strip
	set_texture_direct(ctx.menu, 0, INFOBAR_Y, CANVAS_W, INFOBAR_H);
	memcpy((uint8_t *)ctx.menu, (uint8_t *)(ctx.canvas)+INFOBAR_Y*CANVAS_W*4, CANVAS_W*INFOBAR_H*4);
}

static void draw_side_menu_option(uint8_t option)
{
	memset((uint8_t *)ctx.side, 0x40, SM_M);
	ctx.fg_color=BRIGHT_TEXT;
	set_font(28.f, 24.f, 1.f, 0); print_text(ctx.side, (CANVAS_W-SM_X), SM_TO, SM_Y, "sLaunch MOD " APP_VERSION);

	ctx.fg_color=(option==1 ? WHITE_TEXT : GRAY_TEXT);
	print_text(ctx.side, (CANVAS_W-SM_X), SM_TO+(option!=1)*32, SM_Y+4*24, STR_UNMOUNT);
	ctx.fg_color=(option==2 ? WHITE_TEXT : GRAY_TEXT);
	print_text(ctx.side, (CANVAS_W-SM_X), SM_TO+(option!=2)*32, SM_Y+6*24, STR_REFRESH);
	ctx.fg_color=(option==3 ? WHITE_TEXT : GRAY_TEXT);
	print_text(ctx.side, (CANVAS_W-SM_X), SM_TO+(option!=3)*32, SM_Y+8*24, opt_mode ? STR_SYSCALLS : STR_GAMEDATA);
	ctx.fg_color=(option==4 ? WHITE_TEXT : GRAY_TEXT);
	print_text(ctx.side, (CANVAS_W-SM_X), SM_TO+(option!=4)*32, SM_Y+10*24, STR_RESTART);
	ctx.fg_color=(option==5 ? WHITE_TEXT : GRAY_TEXT);
	print_text(ctx.side, (CANVAS_W-SM_X), SM_TO+(option!=5)*32, SM_Y+12*24, STR_SHUTDOWN);
	ctx.fg_color=(option==6 ? WHITE_TEXT : GRAY_TEXT);
	print_text(ctx.side, (CANVAS_W-SM_X), SM_TO+(option!=6)*32, SM_Y+14*24, !unload_mode ? STR_QUIT : STR_UNLOAD);
	ctx.fg_color=(option==7 ? WHITE_TEXT : GRAY_TEXT);
	print_text(ctx.side, (CANVAS_W-SM_X), SM_TO+(option!=7)*32, SM_Y+20*24, web_page ? STR_FILEMNGR : STR_SETUP);

	set_texture_direct(ctx.side, SM_X, 0, (CANVAS_W-SM_X), CANVAS_H);
	set_textbox(GRAY, SM_X+SM_TO, SM_Y+30,    CANVAS_W-SM_X-SM_TO*2+24, 1);
	set_textbox(GRAY, SM_X+SM_TO, SM_Y+18*24, CANVAS_W-SM_X-SM_TO*2, 1);
}

static uint8_t draw_side_menu(void)
{
	uint8_t option=1;
	play_rco_sound("snd_cursor");

	dump_bg(SM_X-6, 0, (CANVAS_W-SM_X)+6, CANVAS_H);

	set_textbox(0xffe0e0e0ffd0d0d0, SM_X-6, 0, 2, CANVAS_H);
	set_textbox(0xffc0c0c0ffb0b0b0, SM_X-4, 0, 2, CANVAS_H);
	set_textbox(0xffa0a0a0ff909090, SM_X-2, 0, 2, CANVAS_H);

	draw_side_menu_option(option);

	while(slaunch_running)
	{
		pad_read();

		if(curpad)
		{
			if(curpad==oldpad)	// key-repeat
			{
				init_delay++;
				if(init_delay<=40) continue;
				else { sys_timer_usleep(40000); key_repeat=1; }
			}
			else
			{
				init_delay=key_repeat=0;
			}

			oldpad = curpad;

			if(curpad & (PAD_LEFT | PAD_RIGHT))
			{
				if(option == 3) opt_mode^=1;
				if(option == 6) unload_mode^=1;
				if(option == 7) web_page^=1;
			}

			if(curpad & PAD_UP)		option--;
			if(curpad & PAD_DOWN)	option++;

			if(option<1) option=7;
			if(option>7) option=1;

			if(curpad & PAD_TRIANGLE || curpad & PAD_CIRCLE) {option=0; play_rco_sound("snd_cancel"); break;}

			if(curpad & PAD_CROSS) {play_rco_sound("snd_system_ok"); break;}

			play_rco_sound("snd_cursor");
			draw_side_menu_option(option);
		}
		else
		{
			init_delay=oldpad=0;
		}
	}
	init_delay=key_repeat=0;
	set_texture_direct(ctx.canvas, SM_X-6, 0, (CANVAS_W-SM_X)+6, CANVAS_H);

	load_background();
	return option;
}

static void show_no_content(void)
{
	char no_content[32];
	if(gmode >= TYPE_MAX)
		{sprintf(no_content, "webMAN is not loaded."); gmode = TYPE_ALL;}
	else
		sprintf(no_content, "There are no %s%stitles.", game_type[gmode], gmode ? " " : "\0");

	ctx.fg_color=WHITE_TEXT;
	set_font(32.f, 32.f, 1.5f, 1); print_text(ctx.canvas, CANVAS_W, 0, 520, no_content);
	flip_frame((uint64_t*)ctx.canvas);

	sys_timer_usleep(1500000);

	if(gmode) load_background();
}

static void show_content(void)
{
	slaunch_running = 1;

	if(games)
	{
		draw_page(cur_game, 0);
	}
	else	// no content
	{
		show_no_content();
	}
}

static void load_config(void)
{
	int fd;
	_sconfig sconfig;

	if(cellFsOpen(WMTMP "/slaunch.cfg", CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		cellFsRead(fd, (void *)&sconfig, sizeof(_sconfig), 0);
		cellFsClose(fd);

		gmode=sconfig.gmode;
		dmode=sconfig.dmode;
		fav_mode=sconfig.fav_mode;
		fav_game=sconfig.fav_game;
		cur_game=sconfig.cur_game;
		cur_game_=sconfig.cur_game_;
		gpp=sconfig.gpp;
	}
}

static void save_config(void)
{
	_sconfig sconfig;

	sconfig.gmode=gmode;
	sconfig.dmode=dmode;
	sconfig.gpp=gpp;
	sconfig.fav_mode=fav_mode;
	sconfig.fav_game=fav_game;
	sconfig.cur_game=cur_game;
	sconfig.cur_game_=cur_game_;
	sconfig.padd1=sconfig.padd0=0;
	int fd;

	if(cellFsOpen(WMTMP "/slaunch.cfg", CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		cellFsWrite(fd, (void *)&sconfig, sizeof(_sconfig), NULL);
		cellFsClose(fd);
	}
}

static void load_data(void)
{
	int fd;

	if(get_vsh_plugin_slot_by_name("WWWD") >= 7) {games=fav_mode=0, gmode = TYPE_MAX; return;}

	char filename[64];
	if(fav_mode) sprintf(filename, WMTMP "/" SLIST "1.bin"); else sprintf(filename, WMTMP "/" SLIST ".bin");

	uint64_t size = file_len(filename);
	if(fav_mode && (size == 0)) {sprintf(filename, WMTMP "/" SLIST ".bin"); size = file_len(filename); fav_mode = TYPE_ALL;} else
	if(fav_mode) cur_game = fav_game; else cur_game = cur_game_;

	games=size/sizeof(_slaunch);

	if(games>=MAX_GAMES) games=MAX_GAMES-1;
	if(cur_game>=games) cur_game=0;

reload:
	reset_heap(); load_background(); slaunch = NULL;

	if(games && (cellFsOpen(filename, CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED))
	{
		// load game list in MC memory
		slaunch = (_slaunch*)mem_alloc((games+1)*sizeof(_slaunch));

		cellFsRead(fd, (void *)slaunch, sizeof(_slaunch)*games, NULL);
		cellFsClose(fd);

		//if(!fav_mode)
		{
			uint16_t ngames = games; char *path;

			if(gmode)		// filter games by type
			{
				ngames = 0;

				for(uint16_t n=0; n < games; n++)
				{
					if(slaunch[n].type == gmode) slaunch[ngames++] = slaunch[n];
				}

				if(ngames == 0) {gmode++; if(gmode >= TYPE_MAX) gmode = TYPE_ALL; goto reload;}
			}

			if(dmode)		// filter games by device
			{
				int8_t dlen = strlen(drives[dmode-1]);

				games = ngames; ngames = 0;

				for(uint16_t n=0; n < games; n++)
				{
					path = slaunch[n].name + slaunch[n].path_pos;

					if( ((dmode == NTFS) && (strncmp(path+10, "/dev_hdd0/tmp", 13)  ==0)) ||
						((dmode == HDD0) && (strncmp(path+11, drives[dmode-1], dlen)==0) && (path[10+10]!='t')) ||
						((dmode >  NTFS) && (strncmp(path+11, drives[dmode-1], dlen)==0))
					  )
						slaunch[ngames++] = slaunch[n];
				}

				if(ngames == 0) {dmode++; if(dmode > DEVS_MAX) dmode = TYPE_ALL; goto reload;}
			}

			games = ngames; ngames = 0;

			// filter empty entries
			for(uint16_t n=0; n < games; n++)
			{
				if(*slaunch[n].name == 0 || *(slaunch[n].name + slaunch[n].path_pos) == 0) slaunch[ngames] = slaunch[n]; else ngames++;
			}

			if(games > ngames) mem_free((games - ngames) * sizeof(_slaunch));

			games = ngames;

			// sort game list
			if(games>1)
			{
				_slaunch swap; uint8_t sorted = 1; ngames=(games-1);
				for(uint16_t n=0; n<ngames; n++)
				{
					if(strcasecmp(slaunch[n].name, slaunch[n+1].name)>0)
					{
						sorted = 0; break;
					}
				}

				if(!sorted)
				{
					for(uint16_t n=0; n<ngames; n++)
					{
						for(uint16_t m=(n+1); m<games; m++)
						{
							if(strcasecmp(slaunch[n].name, slaunch[m].name)>0)
							{
								swap=slaunch[n];
								slaunch[n]=slaunch[m];
								slaunch[m]=swap;
							}
						}
					}

					if(!gmode && !dmode)
					{
						cellFsOpen(filename, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fd, NULL, 0);
						cellFsWrite(fd, (void *)slaunch, sizeof(_slaunch)*games, NULL);
						cellFsClose(fd);
					}
				}
			}
		}
	}
	else
		games = 0;
}

static void reload_data(uint32_t curpad)
{
	play_rco_sound("snd_cursor");

	if(curpad & PAD_R2) gmode=dmode=TYPE_ALL;

	cur_game=0;

	load_data();
	show_content();
}

//////////////////////////////////////////////////////////////////////
//                       START VSH MENU                             //
//////////////////////////////////////////////////////////////////////

static void start_VSH_Menu(void)
{
	int32_t ret, mem_size;

	uint64_t slist_size = file_len(WMTMP "/" SLIST ".bin"); if(slist_size>(MAX_GAMES*sizeof(_slaunch))) slist_size=MAX_GAMES*sizeof(_slaunch);

	mem_size = ((CANVAS_W * CANVAS_H * 4)   + (CANVAS_W * INFOBAR_H * 4) + (FONT_CACHE_MAX * 32 * 32) + (MAX_WH4) + (SM_M) + (slist_size + sizeof(_slaunch)) + MB(1)) / MB(1);

	// create VSH Menu heap memory from memory container 1("app")
	ret = create_heap(mem_size);

	if(ret) return;

	disp_w = getDisplayWidth();
	disp_h = getDisplayHeight();

	rsx_fifo_pause(1);

	// initialize VSH Menu graphic
	init_graphic();

	// stop vsh pad
	start_stop_vsh_pad(0);

	// load game list
	load_config();
	load_data();
	show_content();

	if(!games && !dmode && !gmode)
	{
		return_to_xmb();

		if(do_once)
		{
			do_once = 0;
			send_wm_request("/refresh_ps3");
			send_wm_request("/browser.ps3/setup.ps3");
		}
	}
}

//////////////////////////////////////////////////////////////////////
//                       STOP VSH MENU                              //
//////////////////////////////////////////////////////////////////////

static void stop_VSH_Menu(void)
{
	// menu off
	slaunch_running = 0;

	// unbind renderer and kill font-instance
	font_finalize();

	// free heap memory
	destroy_heap();

	// save config
	save_config();

	// prevent pass cross button to XMB
	release_cross();

	// continue rsx rendering
	rsx_fifo_pause(0);

	// restart vsh pad
	start_stop_vsh_pad(1);
}

static void return_to_xmb(void)
{
	dim_bg(0.5f, 0.0f);
	stop_VSH_Menu();
}

static void blink_option(uint64_t color, uint64_t color2, uint32_t msecs)
{
	for(uint8_t u = 0; u < 10; u++)
	{
		set_frame(1 + cur_game % gpp, (u & 1) ? color : color2);
		sys_timer_usleep(msecs);
	}
}

static void add_game(void)
{
	int fd;

	blink_option(BLUE, DARK_BLUE, 25000);

	play_rco_sound("snd_cursor"); if(!games) return;

	if(cellFsOpen(WMTMP "/" SLIST "1.bin", CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		_slaunch temp; uint64_t bytes_read;

		char *path = slaunch[cur_game].name + slaunch[cur_game].path_pos;

		while(cellFsRead(fd, (void *)&temp, sizeof(_slaunch), &bytes_read) == CELL_FS_SUCCEEDED)
		{
			if(bytes_read < sizeof(_slaunch)) break;

			if(!strcmp(temp.name + temp.path_pos, path)) {cellFsClose(fd); return;}
		}
		cellFsClose(fd);
	}

	play_rco_sound("snd_system_ok");

	if(cellFsOpen(WMTMP "/" SLIST "1.bin", CELL_FS_O_APPEND | CELL_FS_O_CREAT | CELL_FS_O_WRONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		cellFsWrite(fd, (void *)&slaunch[cur_game], sizeof(_slaunch), NULL);
		cellFsClose(fd);
	}
}

static void remove_game(void)
{
	int fd;

	if(games && (cellFsOpen(WMTMP "/" SLIST "1.bin", CELL_FS_O_CREAT | CELL_FS_O_WRONLY | CELL_FS_O_TRUNC, &fd, NULL, 0) == CELL_FS_SUCCEEDED))
	{
		play_rco_sound("snd_cancel");

		uint16_t n;

		for(n = 0; n < games; n++)
		{
			if(n!=cur_game) cellFsWrite(fd, (void *)&slaunch[n], sizeof(_slaunch), NULL);
		}
		cellFsClose(fd);

		games--;

		blink_option(RED, DARK_RED, 25000);

		for(n = cur_game; n < games; n++)
		{
			slaunch[n] = slaunch[n+1];
		}

		memset(&slaunch[games], 0, sizeof(_slaunch)); mem_free(sizeof(_slaunch)); if(cur_game>=games) {cur_game--; draw_page(cur_game, 0);}

		show_content();
	}
	else
		play_rco_sound("snd_cursor");
}

////////////////////////////////////////////////////////////////////////
//                      PLUGIN MAIN PPU THREAD                        //
////////////////////////////////////////////////////////////////////////

static bool gui_allowed(bool popup)
{
	if(slaunch_running) return 0;

	if(xsetting_CC56EB2D()->GetCurrentUserNumber()<0) // user not logged in
	{
		if(popup) {send_wm_request("/popup.ps3?Not%20logged%20in!"); sys_timer_sleep(2);}
		return 0;
	}

	if(
		IS_INGAME ||
		FindLoadedPlugin("videoplayer_plugin") ||
		FindLoadedPlugin("sysconf_plugin") ||
		FindLoadedPlugin("netconf_plugin") ||
		FindLoadedPlugin("software_update_plugin") ||
		FindLoadedPlugin("photoviewer_plugin") ||
		FindLoadedPlugin("audioplayer_plugin") ||
		FindLoadedPlugin("bdp_plugin") ||
		FindLoadedPlugin("download_plugin")
	)
	{
		if(popup) {send_wm_request("/popup.ps3?Not%20in%20XMB!"); sys_timer_sleep(2);}
		return 0;
	}

	return 1;
}

static void slaunch_thread(uint64_t arg)
{
	if(!arg)
	{
		sys_timer_sleep(12);										// wait 12s and not interfere with boot process
		send_wm_request("/popup.ps3?sLaunch%20MOD%20" APP_VERSION);
		unload_mode = 0;
		//play_rco_sound("snd_system_ng");
	}
	else
		unload_mode = 2;

	for(uint8_t n = 0; n < 7; n++)
	{
		if(file_exists(wm_icons[n]) == false) {sprintf(wm_icons[n], WM_ICONS_PATH "%s", wm_icons[n] + 36); // /dev_hdd0/tmp/wm_icons/
		if(file_exists(wm_icons[n]) == false)  sprintf(wm_icons[n], "/dev_flash/vsh/resource/explore/user/0%i.png", n + 20);} // 020.png - 026.png
	}

	uint8_t gpl; uint16_t pg_idx; uint8_t p;

	while(running)
	{
		if(!slaunch_running)
		{
			if(unload_mode > 2)
			{
				send_wm_request("/unloadprx.ps3?prx=sLaunch");
				sys_ppu_thread_exit(0);
			}

			if(!IS_ON_XMB) {sys_timer_sleep(5); continue;} sys_timer_usleep(300000);

			pdata.len = 0;
			for(p = 0; p < 2; p++)
				if(cellPadGetData(p, &pdata) == CELL_PAD_OK && pdata.len > 0) break;

			// remote start
			if(arg)
			{
				unload_mode = 5;
				if(!gui_allowed(1)) continue;

				start_VSH_Menu();
				init_delay = 0;

				// prevent set favorite with start button
				while(slaunch_running) {pad_read(); if(curpad == PAD_START) sys_timer_usleep(20000); else break;}

				continue;
			}

			if(pdata.len)					// if pad data and we are on XMB
			{
				if( ((pdata.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == 0) &&
					 (pdata.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2)))
					||
					((pdata.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == 0) &&
					 (pdata.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == (CELL_PAD_CTRL_START))) )
				{
					if(!gui_allowed(1)) continue;

					start_VSH_Menu();
					init_delay=0;

					// prevent set favorite with start button
					while(slaunch_running) {pad_read(); if(curpad == PAD_START) sys_timer_usleep(20000); else break;}
				}
			}
		}
		else // menu is running
		{
			while(slaunch_running && running)
			{
				pad_read();

				if(curpad)
				{
					if(curpad==oldpad)				// key-repeat
					{
						init_delay++;
						if(init_delay<=40) continue;
						else sys_timer_usleep(40000);
					}
					else
						init_delay=0;

					can_skip=0;
					oldpad = curpad;

					uint16_t _cur_game = cur_game;
					if(curpad & (PAD_SELECT | PAD_SQUARE | PAD_CIRCLE | PAD_CROSS))
					{
						if(fav_mode) fav_game = cur_game; else cur_game_ = cur_game;
					}

					if(curpad & PAD_TRIANGLE)		// open side-menu
					{
						uint8_t option=draw_side_menu();
						if(option)
						{
							if(option==1) send_wm_request("/mount_ps3/unmount");
							if(option==2) send_wm_request("/refresh_ps3");
							if(option==3)
							{
								if(opt_mode)
								{
									send_wm_request("/popup.ps3?CFW%20Syscalls%20disabled!");
									send_wm_request("/syscall.ps3mapi?scd=1");
								}
								else
									send_wm_request("/extgd.ps3");
							}
							return_to_xmb();
							if(option==4) send_wm_request("/restart.ps3");
							if(option==5) send_wm_request("/shutdown.ps3");
							if(option==6)
							{
								send_wm_request("/popup.ps3?sLaunch%20unloaded!");

								if(!unload_mode)
									send_wm_request("/unloadprx.ps3?prx=sLaunch");
								else
									send_wm_request("/quit.ps3");

								running=0;
							}
							if(option==7) send_wm_request(web_page ? "/browser.ps3/" : "/browser.ps3/setup.ps3");
							break;
						}
						else
							{load_data(); cur_game=_cur_game;}
					}
					else if(curpad & PAD_CIRCLE)	// back to XMB
					{
						//if(fav_mode) {fav_mode=0; reload_data(curpad);} else
						{
							play_rco_sound("snd_cancel");
							stop_VSH_Menu(); /*return_to_xmb();*/
						}
						break;
					}
					else if(curpad & PAD_SELECT)	// favorites menu
					{
						if(init_delay)
						{
							play_rco_sound("snd_cursor");
							fav_mode^=1; init_delay = 0;
							reload_data(curpad);
						}
						else if(curpad & PAD_R3)	{return_to_xmb(); send_wm_request("/popup.ps3"); break;}
						continue;
					}
					else if(curpad & PAD_R3 && games)	{gpp^=34; draw_page(cur_game, 0); init_delay=0;}

					else if(curpad & PAD_L3)	{return_to_xmb(); send_wm_request("/refresh_ps3"); break;}
					else if((curpad & PAD_SQUARE) && !fav_mode && !(curpad & PAD_L2)) {gmode++; if(gmode>=TYPE_MAX) gmode=TYPE_ALL; dmode=TYPE_ALL; reload_data(curpad); continue;}
					else if((curpad & PAD_SQUARE) && !fav_mode &&  (curpad & PAD_L2)) {dmode++; if(dmode> DEVS_MAX) dmode=TYPE_ALL; reload_data(curpad); continue;}
					else if((curpad == PAD_START) && games)	// favorite game XMB
					{
						if(fav_mode) remove_game(); else add_game();
						continue;
					}

					if(!games) continue;

					gpl = (gpp == 10) ? 5 : 10; // games per line

						if(curpad & PAD_RIGHT)	cur_game+=1;
					else if(curpad & PAD_LEFT)	{if(!cur_game) cur_game=games-1; else cur_game-=1;}
					else if(curpad & PAD_UP)	{if(!cur_game) cur_game=games-1; else if(cur_game<gpl) cur_game=0; else cur_game-=gpl;}
					else if(curpad & PAD_DOWN)	{if(cur_game==(games-1)) cur_game=0; else if((cur_game+gpl)>=games) cur_game=games-1; else cur_game+=gpl;}
					else if(curpad & PAD_R1)	{can_skip=1; if(cur_game==(games-1)) cur_game=0; else if((cur_game+gpp)>=games) cur_game=games-1; else cur_game+=gpp;}
					else if(curpad & PAD_L1)	{can_skip=1; if(!cur_game) cur_game=games-1; else cur_game-=gpp;}

					else if(curpad & PAD_CROSS && games)	// execute action & return to XMB
					{
						play_rco_sound("snd_system_ok");

						blink_option(RED, DARK_RED, 75000);

						char path[512];
						snprintf(path, 512, "%s", slaunch[cur_game].name + slaunch[cur_game].path_pos);

						return_to_xmb();
						send_wm_request(path);
						break;
					}

					if(cur_game!=_cur_game && games)		// draw backgrop
					{
						tick=0xc0;
						play_rco_sound("snd_cursor");
						pg_idx=(1 + _cur_game % gpp);
						if(pg_idx<=games) set_backdrop(pg_idx, 1);
						if(cur_game>=games) cur_game=0;
						if((cur_game/gpp)*gpp != (_cur_game/gpp)*gpp)
							draw_page(cur_game, key_repeat & can_skip);
						else
							draw_selection(cur_game);
					}
				}
				else
				{
					init_delay=0, oldpad=0, tick+=delta;	// pulsing selection frame
					pg_idx=(1 + cur_game % gpp);
					if(pg_idx<=games) set_frame(1 + cur_game % gpp, 0xff000000ff000000|tick<<48|tick<<16); else cur_game=0;
					if(tick<0x80 || tick>0xF0)delta=-delta;

					// update temperature
					if(++frame > 300) {frame = 0, cpu_rsx ^= 1; draw_selection(cur_game);}
				}
			}
		}
	}

	if(slaunch_running)
		stop_VSH_Menu();

	sys_ppu_thread_exit(0);
}

/***********************************************************************
* start thread
***********************************************************************/
int32_t slaunch_start(uint64_t arg)
{
	sys_ppu_thread_create(&slaunch_tid, slaunch_thread, arg, -0x1d8, 0x2000, 1, THREAD_NAME);

	_sys_ppu_thread_exit(0);
	return SYS_PRX_RESIDENT;
}

/***********************************************************************
* stop thread
***********************************************************************/
static void slaunch_stop_thread(uint64_t arg)
{
	if(slaunch_running) stop_VSH_Menu();

	running = 0;

	uint64_t exit_code;

	if(slaunch_tid != SYS_PPU_THREAD_NONE)
			sys_ppu_thread_join(slaunch_tid, &exit_code);

	sys_ppu_thread_exit(0);
}

/***********************************************************************
*
***********************************************************************/
static void finalize_module(void)
{
	uint64_t meminfo[5];

	sys_prx_id_t prx = prx_get_module_id_by_address(finalize_module);

	meminfo[0] = 0x28;
	meminfo[1] = 2;
	meminfo[3] = 0;

	system_call_3(482, prx, 0, (uint64_t)(uint32_t)meminfo);
}

/***********************************************************************
*
***********************************************************************/
int slaunch_stop(void)
{
	sys_ppu_thread_t t;
	uint64_t exit_code;

	int ret = sys_ppu_thread_create(&t, slaunch_stop_thread, 0, 0, 0x2000, 1, STOP_THREAD_NAME);
	if (ret == 0) sys_ppu_thread_join(t, &exit_code);

	finalize_module();

	_sys_ppu_thread_exit(0);
	return SYS_PRX_STOP_OK;
}
