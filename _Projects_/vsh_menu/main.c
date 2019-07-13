#include <sys/syscall.h>
#include <cell/pad.h>
#include <cell/cell_fs.h>

#include "include/vsh_exports.h"

#define MAX_PATH_LEN 512

#include "include/misc.h"
#include "include/mem.h"
#include "include/blitting.h"
#include "include/pad.h"
#include "include/file.h"
#include "include/network.h"
#include "include/sys_info.h"
#include "include/vsh_plugin.h"

SYS_MODULE_INFO(VSH_MENU, 0, 1, 0);
SYS_MODULE_START(vsh_menu_start);
SYS_MODULE_STOP(vsh_menu_stop);

#define THREAD_NAME         "vsh_menu_thread"
#define STOP_THREAD_NAME    "vsh_menu_stop_thread"

#define IS_ON_XMB			(vshmain_EB757101() == 0)
#define IS_INGAME			(vshmain_EB757101() != 0)

#define WHITE               0xFFFFFFFF
#define GREEN               0xFF00FF00
#define BLUE                0xFF008FFF
#define GRAY                0xFF999999
#define YELLOW              0xFFFFFF55

#define MAX_LAST_GAMES	(5)

#define MAX_ITEMS	256

enum menus
{
	MAIN_MENU,
	REBUG_MENU,
	PLUGINS_MANAGER,
	FILE_MANAGER,
	LAST_GAME,
};

////////////////////////////////
typedef struct
{
	uint16_t  version;

	uint8_t padding0[14];

	uint8_t lang; //0=EN, 1=FR, 2=IT, 3=ES, 4=DE, 5=NL, 6=PT, 7=RU, 8=HU, 9=PL, 10=GR, 11=HR, 12=BG, 13=IN, 14=TR, 15=AR, 16=CN, 17=KR, 18=JP, 19=ZH, 20=DK, 21=CZ, 22=SK, 99=XX

	// scan devices settings

	uint8_t usb0;
	uint8_t usb1;
	uint8_t usb2;
	uint8_t usb3;
	uint8_t usb6;
	uint8_t usb7;
	uint8_t dev_sd;
	uint8_t dev_ms;
	uint8_t dev_cf;
	uint8_t ntfs; // 1=enable internal prepNTFS to scan content

	uint8_t padding1[5];

	// scan content settings

	uint8_t refr; // 1=disable content scan on startup
	uint8_t foot; // buffer size during content scanning : 0=896KB,1=320KB,2=1280KB,3=512KB,4 to 7=1280KB
	uint8_t cmask;

	uint8_t nogrp;
	uint8_t nocov;
	uint8_t nosetup;
	uint8_t rxvid;
	uint8_t ps2l;
	uint8_t pspl;
	uint8_t tid;
	uint8_t use_filename;
	uint8_t launchpad_xml;
	uint8_t launchpad_grp;
	uint8_t ps3l;
	uint8_t roms;
	uint8_t noused; // formerly mc_app
	uint8_t info;   // info level: 0=Path, 1=Path + ID, 2=ID, 3=None
	uint8_t npdrm;
	uint8_t mc_app; // allow allocation from app memory container

	uint8_t padding2[13];

	// start up settings

	uint8_t wmstart; // 1=disable start up message (webMAN Loaded!)
	uint8_t lastp;
	uint8_t autob;
	char    autoboot_path[256];
	uint8_t delay;
	uint8_t bootd;
	uint8_t boots;
	uint8_t nospoof;
	uint8_t blind;
	uint8_t spp;    //disable syscalls, offline: lock PSN, offline ingame
	uint8_t noss;   //no singstar
	uint8_t nosnd0; //mute snd0.at3
	uint8_t dsc;    //disable syscalls if physical disc is inserted

	uint8_t padding3[4];

	// fan control settings

	uint8_t fanc;      // 1 = enabled, 0 = disabled (syscon)
	uint8_t man_speed; // manual fan speed (calculated using man_rate)
	uint8_t dyn_temp;  // max temp for dynamic fan control (0 = disabled)
	uint8_t man_rate;  // % manual fan speed (0 = dynamic fan control)
	uint8_t ps2_rate;  // % ps2 fan speed
	uint8_t nowarn;
	uint8_t minfan;

	uint8_t padding4[9];

	// combo settings

	uint8_t  nopad;
	uint8_t  keep_ccapi;
	uint32_t  combo;
	uint32_t  combo2;
	uint8_t  sc8mode; // 0/4=Remove cfw syscall disables syscall8 / PS3MAPI=disabled, 1=Keep syscall8 / PS3MAPI=enabled
	uint8_t  nobeep;

	uint8_t padding5[20];

	// ftp server settings

	uint8_t  bind;
	uint8_t  ftpd;
	uint16_t  ftp_port;
	uint8_t  ftp_timeout;
	char ftp_password[20];
	char allow_ip[16];

	uint8_t padding6[7];

	// net server settings

	uint8_t  netsrvd;
	uint16_t  netsrvp;

	uint8_t padding7[13];

	// net client settings

	uint8_t  netd[5];
	uint16_t  netp[5];
	char neth[5][16];

	uint8_t padding8[33];

	// mount settings

	uint8_t bus;
	uint8_t fixgame;
	uint8_t ps1emu;
	uint8_t autoplay;
	uint8_t ps2emu;
	uint8_t ps2config;

	uint8_t padding9[10];

	// profile settings

	uint8_t profile;
	char uaccount[9];
	uint8_t admin_mode;

	uint8_t padding10[5];

	// misc settings

	uint8_t default_restart;
	uint8_t poll; // poll usb

	uint32_t  rec_video_format;
	uint32_t  rec_audio_format;

	uint8_t auto_power_off; // 0 = prevent auto power off on ftp, 1 = allow auto power off on ftp (also on install.ps3, download.ps3)

	uint8_t padding12[5];

	uint8_t homeb;
	char home_url[255];

	uint8_t sman;
	uint8_t padding11[31];

	// spoof console id

	uint8_t sidps;
	uint8_t spsid;
	char vIDPS1[17];
	char vIDPS2[17];
	char vPSID1[17];
	char vPSID2[17];

	uint8_t padding13[34];
} /*__attribute__((packed))*/ WebmanCfg_v2;

typedef struct
{
	uint16_t bgindex;
	uint8_t  dnotify;
	char filler[509];
} __attribute__((packed)) vsh_menu_Cfg;

typedef struct
{
	uint8_t last;
	char game[MAX_LAST_GAMES][MAX_PATH_LEN];
} __attribute__((packed)) _lastgames;


static uint8_t vsh_menu_config[sizeof(vsh_menu_Cfg)];
static vsh_menu_Cfg *config = (vsh_menu_Cfg*) vsh_menu_config;

#define NONE -1
#define SYS_PPU_THREAD_NONE        (sys_ppu_thread_t)NONE

static sys_ppu_thread_t vsh_menu_tid = SYS_PPU_THREAD_NONE;
static int32_t running = 1;
static uint8_t menu_running = 0;	// vsh menu 0=off / 1=on
static uint8_t clipboard_mode = 0;

int32_t vsh_menu_start(uint64_t arg);
int32_t vsh_menu_stop(void);

static void finalize_module(void);
static void vsh_menu_stop_thread(uint64_t arg);

static char tempstr[1024];
static uint16_t t_icon_X;
static char drivestr[6][64];
static uint8_t drive_type[6];

static uint8_t has_icon0 = 0;
static uint8_t unload_mode = 0;

#define REFRESH_DIR  0

static char curdir[MAX_PATH_LEN];
static char items[MAX_ITEMS][MAX_PATH_LEN];
static uint8_t items_isdir[MAX_ITEMS];
static uint16_t nitems = 0, cur_item = 0, curdir_offset = 0, cdir = 0;

static char item_size[64];

static void vsh_menu_thread(uint64_t arg);

char *strcasestr(const char *s1, const char *s2);

static void start_VSH_Menu(void)
{
	struct CellFsStat s;
	char bg_image[48], sufix[8];

	for(uint8_t i = 0; i < 2; i++)
	{
		if(config->bgindex == 0) sprintf(sufix, ""); else sprintf(sufix, "_%i", config->bgindex);
		sprintf(bg_image, "/dev_hdd0/tmp/wm_res/images/wm_vsh_menu%s.png", sufix);
		if(cellFsStat(bg_image, &s) == CELL_FS_SUCCEEDED) break; else sprintf(bg_image, "/dev_hdd0/wm_vsh_menu%s.png", sufix);
		if(cellFsStat(bg_image, &s) == CELL_FS_SUCCEEDED) break; else sprintf(bg_image, "/dev_hdd0/plugins/wm_vsh_menu%s.png", sufix);
		if(cellFsStat(bg_image, &s) == CELL_FS_SUCCEEDED) break; else sprintf(bg_image, "/dev_hdd0/littlebalup_vsh_menu%s.png", sufix);
		if(cellFsStat(bg_image, &s) == CELL_FS_SUCCEEDED) break; else sprintf(bg_image, "/dev_hdd0/plugins/images/wm_vsh_menu%s.png", sufix);
		if(cellFsStat(bg_image, &s) == CELL_FS_SUCCEEDED) break; else sprintf(bg_image, "/dev_hdd0/plugins/littlebalup_vsh_menu%s.png", sufix);
		if(cellFsStat(bg_image, &s) == CELL_FS_SUCCEEDED) break; else config->bgindex = 0;
	}

	rsx_fifo_pause(1);

	int32_t ret, mem_size;

	// create VSH Menu heap memory from memory container 1("app")
	mem_size = (((CANVAS_W * CANVAS_H * 4 * 2) + (FONT_CACHE_MAX * 32 * 32)) + (320 * 176 * 4) + MB(4)) / MB(1);
	ret = create_heap(mem_size);  // 6 MB

	if(ret) {rsx_fifo_pause(0); return;}

	// initialize VSH Menu graphic
	init_graphic();

	// set_font(17, 17, 1, 1);  // set font(char w/h = 20 pxl, line-weight = 1 pxl, distance between chars = 1 pxl)

	// load png image
	load_png_bitmap(0, bg_image);

	get_payload_type();

	get_network_info();

	// stop vsh pad
	start_stop_vsh_pad(0);

	// set menu_running on
	menu_running = 1;

	// reset clipboard mode
	clipboard_mode = 0;
}

//////////////////////////////////////////////////////////////////////
//                       STOP VSH MENU                              //
//////////////////////////////////////////////////////////////////////

static void stop_VSH_Menu(void)
{
	// menu off
	menu_running = 0;

	// unbind renderer and kill font-instance
	font_finalize();

	// free heap memory
	destroy_heap();

	// prevent pass cross button to XMB
	release_cross();

	// continue rsx rendering
	rsx_fifo_pause(0);

	// restart vsh pad
	start_stop_vsh_pad(1);

	sys_timer_usleep(100000);
}

#include "include/rebug.h"

////////////////////////////////////////////////////////////////////////
//							BLITTING								//
////////////////////////////////////////////////////////////////////////
static uint16_t line = 0;			 // current line into menu, init 0 [Menu Entry 1]
#define MAX_MENU	 12
#define MAX_MENU2	8

static uint8_t view = MAIN_MENU;
static bool last_game_view = false;

static uint8_t entry_mode[MAX_MENU] = {0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static char entry_str[2][MAX_MENU][32] = {
											{
											"0: Unmount Game",
											"1: sLaunch MOD",
											"2: Fan (+)",
											"3: Refresh XML",
											"4: Toggle gameDATA",
											"5: Backup Disc to HDD",
											"6: Screenshot (XMB)",
											"7: File Manager",
											"8: webMAN Setup",
											"9: Disable Syscalls",
											"A: Shutdown PS3",
											"B: Reboot PS3 (soft)",
											 },
											 {
											"0: Unload VSH Menu",
											"1: Toggle Rebug Mode",
											"2: Toggle XMB Mode",
											"3: Toggle Debug Menu",
											"4: Disable Cobra",
											"5: Disable webMAN MOD",
											"6: Recovery Mode",
											"7: Startup Message : ON",
											}
										};

////////////////////////////////////////////////////////////////////////
//				 EXECUTE ACTION DEPENDING LINE SELECTED				 //
////////////////////////////////////////////////////////////////////////
static uint8_t fan_mode = 0;

static void return_to_xmb(void)
{
	sys_timer_sleep(1);
	stop_VSH_Menu();
	if(last_game_view || view == FILE_MANAGER)  return;

	view = MAIN_MENU;
}

static void do_main_menu_action(void)
{
	switch(line)
	{
		case 0:
			beep(1);
			send_wm_request("/mount_ps3/unmount");

			//if(entry_mode[line]) wait_for_request(); else
			sys_timer_sleep(1);

			if(entry_mode[line] == 2) eject_insert(0, 1); else if(entry_mode[line] == 1) eject_insert(1, 0);

			stop_VSH_Menu();

			if(entry_mode[line])
			{
				entry_mode[line]=(entry_mode[line] == 2) ? 1 : 2;
				strcpy(entry_str[view][line], ((entry_mode[line] == 2) ? "0: Insert Disc\0" : (entry_mode[line] == 1) ? "0: Eject Disc\0" : "0: Unmount Game"));
			}
			return;
		case 1:
			if(entry_mode[line] == 0) {send_wm_request("/mount_ps3/net0");}
			if(entry_mode[line] == 1) {send_wm_request("/mount_ps3/net1");}
			if(entry_mode[line] == 2) {send_wm_request("/mount_ps3/net2");}
			if(entry_mode[line] == 3) {send_wm_request("/mount_ps3/net3");}
			if(entry_mode[line] == 4) {send_wm_request("/mount_ps3/net4");}
			if(entry_mode[line] == 5) {send_wm_request("/unmap.ps3/dev_usb000");}
			if(entry_mode[line] == 6) {send_wm_request("/remap.ps3/dev_usb000&to=/dev_hdd0/packages");}
			if(entry_mode[line] == 7) {return_to_xmb(); play_rco_sound("system_plugin", "snd_system_ok"); send_wm_request("/browser.ps3$slaunch"); return;}

			break;
		case 2:
			// get fan_mode (0 = dynamic / 1 = manual)
			if(line<3)
			{
				int fd = 0;
				if(cellFsOpen("/dev_hdd0/tmp/wm_config.bin", CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
				{

					 uint8_t wmconfig[sizeof(WebmanCfg_v2)];
					 WebmanCfg_v2 *webman_config = (WebmanCfg_v2*) wmconfig;

					 cellFsRead(fd, (void *)wmconfig, sizeof(WebmanCfg_v2), 0);
					 cellFsClose(fd);

					 fan_mode = (webman_config->man_speed > 0); // manual
				}
			}

			if(entry_mode[line] == (fan_mode ? 1 : 0)) {send_wm_request("/cpursx.ps3?dn"); beep(1);}
			if(entry_mode[line] == (fan_mode ? 0 : 1)) {send_wm_request("/cpursx.ps3?up"); beep(1);}

			if(entry_mode[line] == 2) {send_wm_request("/cpursx.ps3?mode"); beep(3); entry_mode[line]=3; strcpy(entry_str[view][line], "2: System Info"); fan_mode = fan_mode ? 0 : 1;} else
			if(entry_mode[line] == 3) {send_wm_request("/popup.ps3"); return_to_xmb();}

			play_rco_sound("system_plugin", "snd_system_ok");
			return;
		case 3:
			if(entry_mode[line] == 1) send_wm_request("/refresh.ps3?1"); else
			if(entry_mode[line] == 2) send_wm_request("/refresh.ps3?2"); else
			if(entry_mode[line] == 3) send_wm_request("/refresh.ps3?3"); else
			if(entry_mode[line] == 4) send_wm_request("/refresh.ps3?4"); else
			if(entry_mode[line] == 5) send_wm_request("/refresh.ps3?0"); else
									  send_wm_request("/refresh.ps3");

			entry_mode[line] = 0; sprintf(entry_str[view][line], "3: Refresh XML");
			break;
		case 4:
			send_wm_request("/extgd.ps3");

			break;
		case 5:
			send_wm_request("/copy.ps3/dev_bdvd");

			break;
		case 6:
			beep(1);
			screenshot(entry_mode[line]); // mode = 0 (XMB only), 1 (XMB + menu)
			stop_VSH_Menu();

			play_rco_sound("system_plugin", "snd_system_ok");

			return;
		case 7:
			send_wm_request("/browser.ps3/");

			break;
		case 8:
			send_wm_request("/browser.ps3/setup.ps3");

			break;
		case 9:
			if(entry_mode[line] == 1) send_wm_request("/browser.ps3$block_servers");	else
			if(entry_mode[line] == 2) send_wm_request("/browser.ps3$restore_servers");  else
			if(entry_mode[line] == 3) send_wm_request("/delete.ps3?history");			 else
			if(entry_mode[line] == 4) send_wm_request("/syscall.ps3mapi?sce=1");		else
									  send_wm_request("/browser.ps3$disable_syscalls");

			break;
		case 0xA:
			return_to_xmb();

			beep(2);
			shutdown_system();
			return;
		case 0xB:
			return_to_xmb();

			beep(1);
			if(entry_mode[line]) hard_reboot(); else soft_reboot();
			return;
	}

	// return to XMB
	return_to_xmb();

	if(line == 9)
	{
		if(entry_mode[line] == 2) vshtask_notify("Restoring PSN servers..."); else
		if(entry_mode[line] == 3) vshtask_notify("Deleting history...");		else
		if(entry_mode[line] == 4) vshtask_notify("Restoring syscalls...");
	}

	play_rco_sound("system_plugin", "snd_system_ok");
}

static void do_rebug_menu_action(void)
{
	beep(1);

	switch(line)
	{
		case 0:
			send_wm_request("/unloadprx.ps3?prx=VSH_MENU");
			break;

		case 1:
			toggle_normal_rebug_mode();
			return;

		case 2:
			toggle_xmb_mode();
			return;

		case 3:
			toggle_debug_menu();
			return;

		case 4:
			disable_cobra_stage2();
			return;

		case 5:
			disable_webman();
			return;

		case 6:
			recovery_mode();
			return;

		case 7:
			config->dnotify = config->dnotify ? 0 : 1;
			strcpy(entry_str[view][line], (config->dnotify) ? "7: Startup Message : OFF\0" : "7: Startup Message : ON\0");

			// save config
			int fd = 0;
			if(cellFsOpen("/dev_hdd0/tmp/wm_vsh_menu.cfg", CELL_FS_O_CREAT|CELL_FS_O_WRONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
			{
			 cellFsWrite(fd, (void *)vsh_menu_config, sizeof(vsh_menu_Cfg), NULL);
			 cellFsClose(fd);
			}
			return;
	}

	// return to XMB
	return_to_xmb();

	play_rco_sound("system_plugin", "snd_system_ok");
}

static void set_initial_file(void);
static void sort_files(void);

static void do_file_manager_action(uint32_t curpad)
{
	uint16_t old_nitems = nitems, old_line = line;

	int fd;

	// delete file
	if((curpad & (PAD_L2 | PAD_SQUARE))==(PAD_L2 | PAD_SQUARE))
	{
			sprintf(tempstr, "%s/%s", curdir, items[cur_item]);
			del(tempstr, true);
	}

	// cut / copy /paste
	else if(curpad & PAD_SQUARE)
	{
			if(clipboard_mode == 0 && strcmp(items[cur_item], "..") == 0) return;

			char url[2 * MAX_PATH_LEN];
			if(clipboard_mode)
				sprintf(tempstr, "%s", curdir);
			else
				sprintf(tempstr, "%s/%s", curdir, items[cur_item]);

			urlenc(url, tempstr);

			if(clipboard_mode)
				{sprintf(item_size, "<Paste>"); sprintf(tempstr, "/paste.ps3%s", curdir); clipboard_mode = 0;}
			else if(curpad & PAD_R2)
				{sprintf(item_size, "<Cut>"); sprintf(tempstr, "/cut.ps3%s", url); clipboard_mode = 2;}
			else
				{sprintf(item_size, "<Copy>"); sprintf(tempstr, "/cpy.ps3%s", url); clipboard_mode = 1;}

			send_wm_request(tempstr);
			sys_timer_sleep(1);

			tempstr[0]=0;

			if(clipboard_mode) return;
	}

	// do file action
	else if(curpad & (PAD_CROSS | PAD_START | PAD_TRIANGLE))
	{
			int ext_offset = strlen(items[cur_item]) - 4; if(ext_offset<0) ext_offset = 0; bool is_file = !items_isdir[cur_item];

			// go folder up
			if(curpad & PAD_TRIANGLE)
			{
				if(!strcmp(items[cur_item], "..")) sprintf(curdir, "/");

				cur_item = 0;
				char *p = strrchr(curdir, '/'); p[0]=0;
				if(strlen(curdir)==0) sprintf(curdir, "/");
			}
			else

			// edit txt
			if( is_file && (strstr(items[cur_item], "_plugins.txt")!=NULL) )
			{
				char url[2 * MAX_PATH_LEN];
				sprintf(tempstr, "%s/%s", curdir, items[cur_item]);
				urlenc(url, tempstr);
				sprintf(tempstr, "/browser.ps3/edit.ps3%s", url);

				return_to_xmb();
				send_wm_request(tempstr);
				return;
			}
			else

			// open file in browser
			if( is_file && (strcasestr(".png|.bmp|.jpg|.gif|.sht|.txt|.htm|.log|.cfg|.hip|.his", items[cur_item] + ext_offset)!=NULL) )
			{
				char url[2 * MAX_PATH_LEN];
				sprintf(tempstr, "%s/%s", curdir, items[cur_item]);
				urlenc(url, tempstr);
				if(curpad & PAD_CROSS)
					sprintf(tempstr, "/browser.ps3%s", url);
				else
					sprintf(tempstr, "/copy.ps3%s", url);

				return_to_xmb();
				send_wm_request(tempstr);
				return;
			}
			else

			// rename file
			if( is_file && (strcasestr(".bak", items[cur_item] + ext_offset)!=NULL) )
			{
				char source[MAX_PATH_LEN];
				sprintf(source, "%s/%s", curdir, items[cur_item]); items[cur_item][ext_offset] = NULL;
				sprintf(tempstr, "%s/%s", curdir, items[cur_item]);
				cellFsRename(source, tempstr);
				return;
			}
			else

			// install pkg
			if( is_file && (strcasestr(".pkg", items[cur_item] + ext_offset)!=NULL) )
			{
				char url[1024];
				sprintf(tempstr, "%s/%s", curdir, items[cur_item]);
				urlenc(url, tempstr);
				sprintf(tempstr, "/install.ps3%s", url);

				return_to_xmb();
				send_wm_request(tempstr);
				return;
			}
			else

			// copy file from hdd0->usb000 / usb000->hdd0
			if( (is_file && (strcasestr(".p3t|.mp3|.mp4|.mkv|.avi|sprx|edat|.rco|.qrc", items[cur_item] + ext_offset)!=NULL || strstr(items[cur_item], "coldboot")!=NULL)) || (strstr(curdir, "/dev_hdd0/home")==curdir) )
			{
				char url[2 * MAX_PATH_LEN];
				sprintf(tempstr, "%s/%s", curdir, items[cur_item]);
				urlenc(url, tempstr);
				sprintf(tempstr, "/copy.ps3%s", url);

				return_to_xmb();
				send_wm_request(tempstr);
				return;
			}
			else

			// mount game
			if( (is_file && (strcasestr(".iso|so.0|.img|.mdf|.cue|.bin", items[cur_item] + ext_offset)!=NULL || strstr(curdir, "/PS3_GAME")!=NULL)) || (strcmp(items[cur_item], "PS3_DISC.SFB")==0)  || (last_game_view) || (items_isdir[cur_item] && (curpad & PAD_START)) )
			{
				if(last_game_view)
					sprintf(tempstr, "%s", items[cur_item]);
				else if(strcmp(items[cur_item], "PS3_DISC.SFB")==0)
					sprintf(tempstr, "%s", curdir);
				else
					sprintf(tempstr, "%s/%s", curdir, items[cur_item]);

				char url[2 * MAX_PATH_LEN];
				urlenc(url, tempstr);

				if(strstr(curdir, "/dev_hdd0/game") == curdir)
					sprintf(tempstr, "/fixgame.ps3%s", url);
				else if(strstr(url, "/dev_bdvd") || strstr(url, "/app_home"))
					sprintf(tempstr, "/play.ps3");
				else if(strstr(url, "/dev_hdd0/home"))
					sprintf(tempstr, "/copy.ps3%s", url);
				else
					sprintf(tempstr, "/mount_ps3%s", url);

				return_to_xmb();
				send_wm_request(tempstr);
				return;
			}
			else

			// change folder
			if(items_isdir[cur_item])
			{
				if(!strcmp(items[cur_item], ".."))
				{
					char *p = strrchr(curdir, '/'); p[0]=0;
					if(strlen(curdir)==0) sprintf(curdir, "/");
				}
				else
				{
					if(strlen(curdir)>1) strcat(curdir, "/");
					strcat(curdir, items[cur_item]);
				}
			}

	}

	// set title offset
	if(strlen(curdir) < 38) curdir_offset = 0; else curdir_offset = strlen(curdir) - 38;

	// clear list
	for(int i = 0;i < MAX_ITEMS; i++) {items[i][0] = 0; items_isdir[i] = 0;}

	nitems = line = 0;

	// set initial file after delete or read folder
	if((curpad & (PAD_L2 | PAD_SQUARE))==(PAD_L2 | PAD_SQUARE))
	{
			line = old_line; old_nitems--;
			if(line >= old_nitems) {if(line > 0) line--; else line = 0;}
	}
	else
			line = (nitems > 1) ? 1 : 0;

	// list files
	if(last_game_view)
	{
		_lastgames lastgames;

		if(read_file("/dev_hdd0/tmp/wmtmp/last_games.bin", (char*)&lastgames, sizeof(_lastgames), 0))
		{
			for(uint8_t n = 0; n < MAX_LAST_GAMES; n++)
			{
				sprintf(tempstr, "/%s", lastgames.game[n]);
				if(strstr(tempstr, "/net") || file_exists(tempstr))
				{
					sprintf(items[nitems], "/%s", lastgames.game[n]);
					items_isdir[nitems] = 1;

					nitems++;
				}
			}
		}
	}
	else if(cellFsOpendir(curdir, &fd) == CELL_FS_SUCCEEDED)
	{
			CellFsDirent dir; uint64_t read = sizeof(CellFsDirent);

			while(!cellFsReaddir(fd, &dir, &read))
			{
				if(!read || nitems>=MAX_ITEMS) break;
				if(dir.d_name[0] == '.' && dir.d_name[1] == 0) continue;

				sprintf(items[nitems], "/%s", dir.d_name);

				sprintf(tempstr, "%s/%s", curdir, dir.d_name);
				items_isdir[nitems] = isDir(tempstr);

				if(items_isdir[nitems]) items[nitems][0]=' ';

				nitems++;
			}
			cellFsClosedir(fd);
	}

	// sort files
	sort_files();

	set_initial_file();

	tempstr[0] = 0;
}

static void do_plugins_manager_action(uint32_t curpad)
{
	nitems = line = 0;

	int fd; const char paths[8][32] = {"/dev_hdd0", "/dev_hdd0/plugins", "/dev_hdd0/plugins/ps3xpad", "/dev_hdd0/plugins/ps3_menu", "/dev_usb000", "/dev_usb001", "/dev_hdd0/game/UPDWEBMOD/USRDIR", "/dev_hdd0/tmp/wm_res"};

	// clear list
	for(int i = 0; i < MAX_ITEMS; i++) {items[i][0] = 0, items_isdir[i] = 0;}

	nitems = line = 0;

	// list plugins
	for(uint8_t i = 0; i < 8; i++)
	if(cellFsOpendir(paths[i], &fd) == CELL_FS_SUCCEEDED)
	{
			CellFsDirent dir; uint64_t read = sizeof(CellFsDirent);

			while(!cellFsReaddir(fd, &dir, &read))
			{
				if(!read || nitems>=MAX_ITEMS) break;
				if(strstr(dir.d_name, ".sprx"))
				{
					snprintf(items[nitems], MAX_PATH_LEN-1, "%s/%s", paths[i], dir.d_name);

					items_isdir[nitems] = get_vsh_plugin_slot_by_name(items[nitems], false);

					snprintf(items[nitems], MAX_PATH_LEN-1, "/%s/%s", paths[i], dir.d_name);

					if(items_isdir[nitems]) items[nitems][0]=' ';

					nitems++;
				}
			}
			cellFsClosedir(fd);
	}

	// sort files
	sort_files();

	set_initial_file();

	// save boot_plugins.txt
	if(curpad & PAD_START)
	{
			int fd;
			if(cellFsOpen("/dev_hdd0/boot_plugins.txt", CELL_FS_O_CREAT|CELL_FS_O_WRONLY|CELL_FS_O_TRUNC, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
			{
				for (int i = 0; i < nitems; i++)
				{
					if(items_isdir[i])
					{
						sprintf(tempstr, "%s\n", items[i]);
						cellFsWrite(fd, (void *)tempstr, strlen(tempstr), NULL);
					}
				}
				cellFsClose(fd);
			}

			return_to_xmb();
			vshtask_notify("Saved /dev_hdd0/boot_plugins.txt");
	}

	tempstr[0]=0;
}

static void sort_files(void)
{
	// sort file entries
	uint16_t n, m; char swap[MAX_PATH_LEN]; uint8_t s;
	for(n = 0; n < (nitems - 1); n++)
			for(m = (n + 1); m < nitems; m++)
				if(strcasecmp(items[n], items[m]) > 0)
				{
					strcpy(swap, items[n]);
					strcpy(items[n], items[m]);
					strcpy(items[m], swap);

					s = items_isdir[n];
					items_isdir[n] = items_isdir[m];
					items_isdir[m] = s;
				}

	// remove sort prefix for directories
	for(n = 0; n < nitems; n++) memcpy(items[n], items[n]+1, MAX_PATH_LEN-1);
}

static void set_initial_file(void)
{
	// set first option
	if(strcmp(items[0], "..") == 0 && nitems>1) cur_item = 1; else cur_item = 0;

	if(!curdir[1]) for(int i=1; i<nitems; i++) if(!strcmp(items[i], "dev_hdd0")) {cur_item = line=i; break;}

	// show initial icon0
	has_icon0 = 0;
	sprintf(tempstr, "%s/ICON0.PNG", curdir);
	if(file_exists(tempstr)) {has_icon0 = 2; load_png_bitmap(1, tempstr);} else
	{
			sprintf(tempstr, "%s/ICON2.PNG", curdir);
			if(file_exists(tempstr)) {has_icon0 = 2; load_png_bitmap(1, tempstr);}
	}

	if(!has_icon0)
	{
			sprintf(tempstr, "%s/PS3_GAME/ICON0.PNG", curdir);
			if(file_exists(tempstr)) {has_icon0 = 2; load_png_bitmap(1, tempstr);}
	}

	if(!has_icon0 && items_isdir[cur_item])
	{

			sprintf(tempstr, "%s/%s/ICON0.PNG", curdir, items[cur_item]);
			if(file_exists(tempstr)) {has_icon0 = 1; load_png_bitmap(1, tempstr);} else
			{
				sprintf(tempstr, "%s/%s/PS3_GAME/ICON0.PNG", curdir, items[cur_item]);
				if(file_exists(tempstr)) {has_icon0 = 1; load_png_bitmap(1, tempstr);} else
				{
					sprintf(tempstr, "%s/%s/../ICON0.PNG", curdir, items[cur_item]);
					if(file_exists(tempstr)) {has_icon0 = 2; load_png_bitmap(1, tempstr);} else
					{
						sprintf(tempstr, "%s/%s/../../ICON0.PNG", curdir, items[cur_item]);
						if(file_exists(tempstr)) {has_icon0 = 2; load_png_bitmap(1, tempstr);}
					}
				}
			}
	}
}


////////////////////////////////////////////////////////////////////////
//							 DRAW A FRAME							 //
////////////////////////////////////////////////////////////////////////
static uint32_t frame = 0;

static void draw_background_and_title(void)
{
	// all 32bit colors are ARGB, the framebuffer format
	set_background_color(0xEE333333);  // dark gray, semitransparent
	set_foreground_color(WHITE);		 // white, opac

	if(ctx.png[0].w == 0 || ctx.png[0].h == 0)
	{
			// fill background with background color
			draw_background();
	}
	else
	{
			// draw background from png
			draw_png(0, 0, 0, 0, 0, 720, 400);
	}

	// draw logo from png
	draw_png(0, 648, 336, 576, 400, 64, 64);

	if(has_icon0 && (view == FILE_MANAGER && clipboard_mode == 0))
		 draw_png(1, 18, 208, 0, 0, ctx.png[1].w, ctx.png[1].h);

	// print headline string, center(x = -1)
	set_font(22.f, 23.f, 1.f, 1); print_text(CENTER_TEXT, 8,  ( (view == REBUG_MENU)	  ? "VSH Menu for Rebug"    :
																(view == FILE_MANAGER &&  last_game_view) ? "LAST GAMES" :
																(view == FILE_MANAGER && !last_game_view) ? curdir + curdir_offset :
																(view == PLUGINS_MANAGER) ? "Plugins Manager"		:
																						    "VSH Menu for webMAN") );
	set_font(14.f, 14.f, 1.f, 1); print_text(650, 8, "v1.16");
}

static void draw_menu_options(void)
{
	int32_t i, selected;
	const int32_t count = 4;						// list count  .. tip: if less than entries nbr, will scroll :)
	uint32_t color = 0, selcolor = 0;

	set_font(20.f, 20.f, 1.f, 1);

	selcolor = (frame & 0x2) ? WHITE : GREEN;

	// draw menu entry list
	for(i = 0; i < count; i++)
	{
		if(line < count)
		{
			selected = 0;
			color = (i == line) ? selcolor : WHITE;
		}
		else
		{
			selected = line - (count - 1);
			color = (i == (count - 1)) ? selcolor : WHITE;
		}

		if(view == FILE_MANAGER || view == PLUGINS_MANAGER)
		{
			if( (selected + i) >= nitems ) break;

			if(color == WHITE && items_isdir[selected + i]) color = YELLOW;
			if(color == GREEN) cur_item = selected + i;
		}

		set_foreground_color(color);

		if(view == FILE_MANAGER)
			print_text(20, 40 + (LINE_HEIGHT * (i + 1)), items[selected + i]);
		else if(view == PLUGINS_MANAGER)
			print_text(20, 40 + (LINE_HEIGHT * (i + 1)), strrchr(items[selected + i], '/') + 1 );
		else
			print_text(20, 40 + (LINE_HEIGHT * (i + 1)), entry_str[view][selected + i]);

		selected++;
	}

	if (i < MAX_MENU)
	if (line > count - 1)	draw_png(0, 20, 56, 688, 400, 16, 8);   // UP arrow
	if (line < MAX_MENU - 1) draw_png(0, 20, 177, 688, 408, 16, 8);  // DOWN arrow
}

static void draw_legend(void)
{
	bool no_button = (ctx.png[0].w == 0 || ctx.png[0].h == 0);

	// draw command buttons
	set_foreground_color(GRAY);

	set_font(20.f, 17.f, 1.f, 1);

	// draw 1st button
	if(no_button) ;

	else if(view == FILE_MANAGER)
	{
			// draw start button
			draw_png(0, 522, 230, 320, 400, 32, 32);
			print_text(560, 234, items_isdir[cur_item] ? " Mount" : " Copy");
	}
	else if(view == PLUGINS_MANAGER)
	{
			// draw start button
			draw_png(0, 522, 230, 320, 400, 32, 32);
			print_text(560, 234, " Save");
	}
	else if(view == MAIN_MENU)
	{
			// draw up-down button
			draw_png(0, 522, 230, 128 + ((line<4||line==6||line==9||line==0xB) ? 64 : 0), 432, 32, 32);
			print_text(560, 234, " Choose");
	}
	else if(view == REBUG_MENU)
	{
			// draw up-down button
			draw_png(0, 522, 230, 128 + ((line==7) ? 64 : 0), 432, 32, 32);
			print_text(560, 234, " Choose");
	}

	// draw X button
	if(no_button) print_text(530, 266, "X"); else draw_png(0, 522, 262, 0, 400, 32, 32);

	// print X legend
	if(view == FILE_MANAGER)
	{
			int ext_offset = strlen(items[cur_item]) - 4; if(ext_offset < 0) ext_offset = 0; bool is_file = !items_isdir[cur_item];

			if( is_file && (strcasestr(".png|.bmp|.jpg|.gif|.sht|.txt|.htm|.log|.cfg|.hip|.his", items[cur_item] + ext_offset)!=NULL) )
				print_text(570, 266, "View");
			else
			if( is_file && (strcasestr(".pkg", items[cur_item] + ext_offset)!=NULL) )
				print_text(570, 266, "Install");
			else
			if( is_file && (strcasestr(".bak", items[cur_item] + ext_offset)!=NULL) )
				print_text(570, 266, "Rename");
			else
			if( (is_file && (strcasestr(".p3t|.mp3|.mp4|.mkv|.avi|sprx|edat|.rco|.qrc", items[cur_item] + ext_offset)!=NULL || strstr(items[cur_item], "coldboot")!=NULL)) || (strstr(curdir, "/dev_hdd0/home")==curdir) )
				print_text(570, 266, "Copy");
			else
			if( is_file && ((strcasestr(".iso|so.0|.img|.mdf|.cue|.bin", items[cur_item] + ext_offset)!=NULL || strstr(curdir, "/PS3_GAME")!=NULL) || (last_game_view) || (strcmp(items[cur_item], "PS3_DISC.SFB")==0)) )
				print_text(570, 266, "Mount");
			else
			if( is_file && (strstr(items[cur_item], "_plugins.txt")!=NULL) )
				print_text(570, 266, "Edit");
			else
				print_text(570, 266, "Select");  // draw X button

			// draw file size / clipboard operation
			if(!items_isdir[cur_item] || clipboard_mode)
												 print_text(410, 208, item_size);
	}
	else if(view == PLUGINS_MANAGER)
	{
												 print_text(570, 266, items_isdir[cur_item] ? " Unload" : " Load");  // draw X button
	}
	else
	{
												 print_text(570, 266, "Select");  // draw X button
	}

	if(no_button) return;

	draw_png(0, 522, 294, 416, 400, 32, 32); print_text(570, 298, "Exit");	// draw select button

	draw_png(0, 522, 326, 128, 400, 32, 32); print_text(570, 330, "Mode");	// draw L1 button
}

static void draw_system_info(void)
{
	set_foreground_color(BLUE);

	// draw firmware version info
	print_text(352, 30 + (LINE_HEIGHT * 1), cfw_str);

	// draw network info
	print_text(352, 30 + (LINE_HEIGHT * 2.5), netstr);

	// draw temperatures
	if(frame == 1 || frame == 33 || !tempstr[0])
	{
			uint32_t cpu_temp_c = 0, rsx_temp_c = 0, cpu_temp_f = 0, rsx_temp_f = 0, higher_temp;

			get_temperature(0, &cpu_temp_c);
			get_temperature(1, &rsx_temp_c);
			cpu_temp_c = cpu_temp_c >> 24;
			rsx_temp_c = rsx_temp_c >> 24;
			cpu_temp_f = (1.8f * (float)cpu_temp_c + 32.f);
			rsx_temp_f = (1.8f * (float)rsx_temp_c + 32.f);

			sprintf(tempstr, "CPU :  %i°C  •  %i°F\r\nRSX :  %i°C  •  %i°F", cpu_temp_c, cpu_temp_f, rsx_temp_c, rsx_temp_f);

			if (cpu_temp_c > rsx_temp_c) higher_temp = cpu_temp_c;
			else higher_temp = rsx_temp_c;

				 if (higher_temp < 50)						 t_icon_X = 224;  // blue icon
			else if (higher_temp >= 50 && higher_temp <= 65) t_icon_X = 256;  // green icon
			else if (higher_temp >  65 && higher_temp <  75) t_icon_X = 288;  // yellow icon
			else											 t_icon_X = 320;  // red icon
	}

	set_font(24.f, 17.f, 1.f, 1);

	draw_png(0, 355, 38 + (LINE_HEIGHT * 5), t_icon_X, 464, 32, 32);
	print_text(395, 30 + (LINE_HEIGHT * 5), tempstr);
}

static void draw_drives_info(void)
{
	set_font(20.f, 17.f, 1.f, 1);
	set_foreground_color(YELLOW);
	//print_text(20, 208, "Available free space on device(s):");

	int fd, j;

	//draw drives info
	if( (frame == 1) && (cellFsOpendir("/", &fd) == CELL_FS_SUCCEEDED) )
	{
		char drivepath[32], freeSizeStr[32], devSizeStr[32];
		uint64_t read, freeSize, devSize;
		CellFsDirent dir;

		for(j = 0; j < 6; j++) {memset(drivestr[j], 0, 64); drive_type[j] = 0;}

		j = 0;

		while(cellFsReaddir(fd, &dir, &read) == 0 && read > 0)
		{
			if (strncmp("dev_hdd", dir.d_name, 7) == 0)
			drive_type[j] = 1;
			else if (strncmp("dev_usb", dir.d_name, 7) == 0)
			drive_type[j] = 2;
			else if (strncmp("dev_blind", dir.d_name, 9) == 0 )
			drive_type[j] = 3;
			else if ((strncmp("dev_sd", dir.d_name, 6) == 0 ) || (strncmp("dev_ms", dir.d_name, 6) == 0 ) ||  (strncmp("dev_cf", dir.d_name, 6) == 0 ))
			drive_type[j] = 4;
			else
			continue;

			sprintf(drivepath, "/%s", dir.d_name);

			system_call_3(840, (uint64_t)(uint32_t)drivepath, (uint64_t)(uint32_t)&devSize, (uint64_t)(uint32_t)&freeSize);

			if (freeSize < 1073741824)
				sprintf(freeSizeStr, "%.2f MB", (double) (freeSize / 1048576.00f));
			else
				sprintf(freeSizeStr, "%.2f GB", (double) (freeSize / 1073741824.00f));

			if (devSize < 1073741824)
				sprintf(devSizeStr, "%.2f MB", (double) (devSize / 1048576.00f));
			else
				sprintf(devSizeStr, "%.2f GB", (double) (devSize / 1073741824.00f));

			sprintf(drivestr[j], "%s :  %s / %s", drivepath, freeSizeStr, devSizeStr);

			j++; if(j >= 6) break;
		}

		cellFsClosedir(fd);
	}

	print_text(20, 208, "Available free space on device(s):");

	for(j = 0; j < 6; j++)
	{
			if(drive_type[j]) {draw_png(0, 25, 230 + (26 * j), 32 + (32 * drive_type[j]), 464, 32, 32); print_text(60, 235 + (26 * j), drivestr[j]);}
	}

	//...
}
static void draw_frame(void)
{
	draw_background_and_title();

	draw_menu_options();

	draw_legend();

	if(view != FILE_MANAGER)
	{
			draw_system_info();
	}
	else if(has_icon0 && (clipboard_mode == 0)) return;

	draw_drives_info();
}

static void change_current_folder(uint32_t curpad)
{
	if(curpad & PAD_LEFT)  if(cdir>0) cdir--;
	if(curpad & PAD_RIGHT) cdir++;

	const char *paths[19] = {
								"/",
								"/dev_hdd0",
								"/dev_hdd0/GAMES",
								"/dev_hdd0/GAMEZ",
								"/dev_hdd0/PS3ISO",
								"/dev_hdd0/PS2ISO",
								"/dev_hdd0/PSXISO",
								"/dev_hdd0/PSPISO",
								"/dev_hdd0/packages",
								"/dev_hdd0/plugins",
								"/dev_hdd0/BDISO",
								"/dev_hdd0/DVDISO",
								"/dev_hdd0/game/BLES80608/USRDIR",
								"/dev_usb001",
								"/dev_usb001/GAMES",
								"/dev_usb001/PS3ISO",
								"/dev_usb000/PS3ISO",
								"/dev_usb000/GAMES",
								"/dev_usb000"
							};

	for(;;)
	{
			sprintf(curdir, "%s", paths[cdir]);
			if(isDir(curdir)) {curpad = REFRESH_DIR; break;}
			if(curpad & PAD_LEFT) {if(cdir>0) cdir--;} else cdir++;
			if(cdir > 18) cdir=0;
	}

	do_file_manager_action(REFRESH_DIR);
}

static void change_main_menu_options(uint32_t curpad)
{
	uint8_t last_opt = ((line==0) ? 2 : (line==1) ? 7 : (line==9) ? 4 : (line==2) ? 3 : (line==3) ? 5 : (line==6 || line==0xB) ? 1 : 0);

	if(curpad & PAD_RIGHT) ++entry_mode[line];
	else
	if(entry_mode[line] == 0)  entry_mode[line] = last_opt;
	else
							 --entry_mode[line];

	if(entry_mode[line]>last_opt) entry_mode[line]=0;

	uint8_t opt = entry_mode[line];

	switch (line)
	{
		case 0x0: strcpy(entry_str[view][line], ((opt == 1) ? "0: Eject Disc\0"  :
												 (opt == 2) ? "0: Insert Disc\0" :
															  "0: Unmount Game\0"));
		break;

		case 0x1:	if(opt == 5)
						sprintf(entry_str[view][line], "1: Unmap USB000");
					else
					if(opt == 6)
						sprintf(entry_str[view][line], "1: Remap USB000 to HDD0");
					else
					if(opt == 7)
						sprintf(entry_str[view][line], "1: slaunch MOD");
					else
						sprintf(entry_str[view][line], "1: Mount /net%i", opt);
		break;

		case 0x2: strcpy(entry_str[view][line], ((opt == 1) ? "2: Fan (-)\0"	 :
												 (opt == 2) ? "2: Fan Mode\0"	:
												 (opt == 3) ? "2: System Info\0" :
															  "2: Fan (+)\0"));
		break;

		case 0x3: sprintf(entry_str[view][line], "3: Refresh XML"); if(opt) sprintf(entry_str[view][line] + 14, " (%i)", (opt==5) ? 0 : opt);

		break;

		case 0x6: strcpy(entry_str[view][line], ((opt) ? "6: Screenshot (XMB + Menu)\0"  :
														 "6: Screenshot (XMB)\0"));
		break;

		case 0x9: strcpy(entry_str[view][line], ((opt == 1) ? "9: Block PSN Servers\0"   :
												 (opt == 2) ? "9: Restore PSN Servers\0" :
												 (opt == 3) ? "9: Delete History\0"		:
												 (opt == 4) ? "9: Restore Syscalls\0"	:
															  "9: Disable Syscalls\0"));
		break;

		case 0xB: strcpy(entry_str[view][line], ((opt) ? "B: Reboot PS3 (hard)\0" :
														 "B: Reboot PS3 (soft)\0"));
		break;
	}
}

static void show_icon0(uint32_t curpad)
{
	if(view == FILE_MANAGER)
	{
		if(nitems == 0) return;

		if(curpad & PAD_DOWN) {cur_item++; if(cur_item >= nitems) cur_item = 0;}
		if(curpad & PAD_UP)   {if(cur_item > 0) cur_item--; else cur_item = nitems - 1;}


		if(has_icon0 == 1) has_icon0 = 0;

		if(!items_isdir[cur_item])
		{
			struct CellFsStat s;
			sprintf(tempstr, "%s/%s", curdir, items[cur_item]);
			cellFsStat(tempstr, &s);

			if(s.st_size >= 0x40000000ULL)
				sprintf(item_size, "%1.2f GB", ((double)(s.st_size)) / 1073741824.0f);
			if(s.st_size >= 0x100000ULL)
				sprintf(item_size, "%1.2f MB", ((double)(s.st_size)) / 1048576.0f);
			else
				sprintf(item_size, "%1.2f KB", ((double)(s.st_size)) / 1024.0f);
		}
		else if(has_icon0 == 0)
		{

			sprintf(tempstr, "%s/%s/ICON0.PNG", curdir, items[cur_item]);
			if(file_exists(tempstr)) {has_icon0 = 1; load_png_bitmap(1, tempstr);} else
			{
				sprintf(tempstr, "%s/%s/PS3_GAME/ICON0.PNG", curdir, items[cur_item]);
				if(file_exists(tempstr)) {has_icon0 = 1; load_png_bitmap(1, tempstr);} else
				{
					sprintf(tempstr, "%s/%s/../ICON0.PNG", curdir, items[cur_item]);
					if(file_exists(tempstr)) {has_icon0 = 1; load_png_bitmap(1, tempstr);} else
					{
						sprintf(tempstr, "%s/%s/../../ICON0.PNG", curdir, items[cur_item]);
						if(file_exists(tempstr)) {has_icon0 = 1; load_png_bitmap(1, tempstr);}
					}
				}
			}
		}

		tempstr[0] = 0;
	}
}

////////////////////////////////////////////////////////////////////////
//                      PLUGIN MAIN PPU THREAD                        //
////////////////////////////////////////////////////////////////////////
static bool gui_allowed(bool popup)
{
	if(menu_running) return 0;

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

static void vsh_menu_thread(uint64_t arg)
{
#ifdef DEBUG
	dbg_init();
	dbg_printf("programstart:\n");
#endif

	uint32_t DELAY, show_menu = 0;

	// init config
	config->bgindex = 0;
	config->dnotify = 0;

	// read config file
	int fd = 0;
	if(cellFsOpen("/dev_hdd0/tmp/wm_vsh_menu.cfg", CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		 cellFsRead(fd, (void *)vsh_menu_config, sizeof(vsh_menu_Cfg), 0);
		 cellFsClose(fd);

		 strcpy(entry_str[1][7], (config->dnotify) ? "7: Startup Message : OFF\0" : "7: Startup Message : ON\0");
	}

	if(!arg)
	{
		sys_timer_sleep(13);	// wait 13s and not interfere with boot process

		if(!config->dnotify)
		{
			vshtask_notify("VSH Menu loaded.\nHold [Select] to open it.");
		}

		unload_mode = 0, DELAY = 3;
	}
	else
		unload_mode = 2, DELAY = 1;

	//View_Find = getNIDfunc("paf", 0xF21655F3, 0);

	*payload_type = NULL;
	*kernel_type = NULL;
	*netstr = NULL;
	*cfw_str = NULL;
	sprintf(curdir,  "/");

	get_firmware_version();
	memset(payload_type, 0, 64);

	while(running)
	{
		if(!menu_running)													 // VSH menu is not running, normal XMB execution
		{
			if(unload_mode > 2)
			{
				send_wm_request("/unloadprx.ps3?prx=VSH_MENU");
				sys_ppu_thread_exit(0);
			}

			if(!gui_allowed(true)) {sys_timer_sleep(5); continue;} sys_timer_usleep(300000);

			pdata.len = 0;
			for(uint8_t p = 0; p < 8; p++)
				if(cellPadGetData(p, &pdata) == CELL_PAD_OK && pdata.len > 0) break;

			// remote start
			if(arg)
			{
				show_menu = 0, oldpad = PAD_SELECT;
				start_VSH_Menu(); unload_mode = 5;
				continue;
			}

			if(pdata.len)					// if pad data and we are on XMB
			{
				if((pdata.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == CELL_PAD_CTRL_SELECT) && (pdata.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == 0)) ++show_menu; else show_menu = 0;

				if(show_menu > DELAY)			// Start VSH menu if SELECT button was pressed for few seconds
				{
					show_menu = 0, oldpad = PAD_SELECT;
					if(line & 1) line = 0;	// menu on first entry in list

					if(gui_allowed(true)) start_VSH_Menu();
				}
			}
		}
		else // menu is running, draw frame, flip frame, check for pad events, sleep, ...
		{
			frame++; if(frame & 0x40) frame = 0; if(frame & 1) {draw_frame(); flip_frame();}

			pad_read();

			if(curpad == oldpad && (curpad & (PAD_SELECT | PAD_R1 | PAD_R2))) ;

			else if(curpad)
			{
			oldpad = curpad;

			if(curpad & (PAD_SELECT | PAD_CIRCLE))  // Stop VSH menu if SELECT button was pressed
			{
				return_to_xmb();
			}
			else
			if((curpad & (PAD_LEFT | PAD_RIGHT)))
			{
				if(view == MAIN_MENU)
				{
					change_main_menu_options(curpad);
				}
				if(view == REBUG_MENU)
				{
				switch (line)
				{
				 case 7: do_rebug_menu_action(); break;
				}
				}
				if(view == FILE_MANAGER)
				{
					change_current_folder(curpad);
				}
			}
			else
			if(curpad & PAD_UP)
			{
				frame = 0;
				if(line > 0)
				{
				line--;
				play_rco_sound("system_plugin", "snd_cursor");
				}
				else
				line = (view > REBUG_MENU ? nitems : view == REBUG_MENU ? MAX_MENU2 : MAX_MENU)-1;

				show_icon0(curpad);
				oldpad = 0;
			}
			else
			if(curpad & PAD_DOWN)
			{
				frame = 0;
				if(line < ((view > REBUG_MENU ? nitems : view == REBUG_MENU ? MAX_MENU2 : MAX_MENU)-1))
				{
				line++;
				play_rco_sound("system_plugin", "snd_cursor");
				}
				else
				line = 0;

				show_icon0(curpad);
				oldpad = 0;
			}

			if(view == FILE_MANAGER && ((curpad & (PAD_CROSS | PAD_SQUARE)) || curpad == REFRESH_DIR))
			{
				do_file_manager_action(curpad);
			}
			else
			if(curpad & (PAD_R1 | PAD_TRIANGLE | PAD_START | PAD_L1))
			{
				if(last_game_view) view = LAST_GAME;

				if((curpad & PAD_R1) == PAD_R1 || (view != FILE_MANAGER && (curpad & PAD_TRIANGLE))) {clipboard_mode = 0; if(++view > 4) view = MAIN_MENU;}
				if((curpad & PAD_L1) == PAD_L1) {clipboard_mode = 0; if(view > 0) --view; else view = 4;}

				if(view == LAST_GAME) {view = FILE_MANAGER, last_game_view = true;} else last_game_view = false;

				if(view == PLUGINS_MANAGER) do_plugins_manager_action(curpad);
				if(view == FILE_MANAGER) do_file_manager_action(curpad);
			}
			else
			if(curpad & PAD_CROSS)
			{
				if(view == MAIN_MENU) do_main_menu_action(); else
				if(view == REBUG_MENU) do_rebug_menu_action();
				if(view == PLUGINS_MANAGER)
				{
					unsigned int slot = get_vsh_plugin_slot_by_name(items[cur_item], true);

					return_to_xmb();

					if(!slot) sprintf(tempstr, "%s unloaded", items[cur_item]); else {cobra_load_vsh_plugin(slot, items[cur_item], NULL, 0); sprintf(tempstr, "%s loaded in slot %i", items[cur_item], slot);}

					if(strstr(items[cur_item], "webftp_server")) wm_unload ^= 1;

					vshtask_notify(tempstr);
				}
			}
			else
			if(curpad & PAD_SQUARE)
			{
				// switch them file
				stop_VSH_Menu();
				config->bgindex++;
				start_VSH_Menu();

				// save config
				int fd = 0;
				if(cellFsOpen("/dev_hdd0/tmp/wm_vsh_menu.cfg", CELL_FS_O_CREAT|CELL_FS_O_WRONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
				{
				 cellFsWrite(fd, (void *)vsh_menu_config, sizeof(vsh_menu_Cfg), NULL);
				 cellFsClose(fd);
				}
			}

			// ...
			}
			else
			oldpad = 0;

			sys_timer_usleep(90000); // short menu frame delay
		}
	}

#ifdef DEBUG
	dbg_fini();
#endif

	if(menu_running)
		stop_VSH_Menu();

	sys_ppu_thread_exit(0);
}

/***********************************************************************
* start thread
***********************************************************************/
int32_t vsh_menu_start(uint64_t arg)
{
	sys_ppu_thread_create(&vsh_menu_tid, vsh_menu_thread, arg, -0x1d8, 0x2000, 1, THREAD_NAME);

	_sys_ppu_thread_exit(0);
	return SYS_PRX_RESIDENT;
}

/***********************************************************************
* stop thread
***********************************************************************/
static void vsh_menu_stop_thread(uint64_t arg)
{
	if(menu_running) stop_VSH_Menu();

	if(unload_mode < 2) vshtask_notify("VSH Menu unloaded.");

	running = 0;

	uint64_t exit_code;

	if(vsh_menu_tid != SYS_PPU_THREAD_NONE)
			sys_ppu_thread_join(vsh_menu_tid, &exit_code);

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
int vsh_menu_stop(void)
{
	sys_ppu_thread_t t;
	uint64_t exit_code;

	int ret = sys_ppu_thread_create(&t, vsh_menu_stop_thread, 0, 0, 0x2000, 1, STOP_THREAD_NAME);
	if (ret == 0) sys_ppu_thread_join(t, &exit_code);

	finalize_module();

	_sys_ppu_thread_exit(0);
	return SYS_PRX_STOP_OK;
}
