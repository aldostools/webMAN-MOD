#define USE_INTERNAL_PLUGIN 1

#include <sdk_version.h>
#include <cellstatus.h>
#include <cell/cell_fs.h>
#include <cell/rtc.h>
#include <cell/gcm.h>
#include <cell/pad.h>
#include <sys/vm.h>
#include <sysutil/sysutil_common.h>

#include <sys/prx.h>
#include <sys/ppu_thread.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/memory.h>
#include <sys/timer.h>
#include <sys/process.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netex/net.h>
#include <netex/errno.h>
#include <netex/libnetctl.h>
#include <netex/sockinfo.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "flags.h"
#include "types.h"
#include "include/timer.h"

#ifdef REX_ONLY
 #ifndef DEX_SUPPORT
 #define DEX_SUPPORT	1
 #endif
#endif

#undef NET_SUPPORT
#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
 #define NET_SUPPORT 1
 #endif
#endif

#ifdef LAST_FIRMWARE_ONLY
 #undef DECR_SUPPORT
 #undef FIX_GAME
#endif

#ifdef PKG_LAUNCHER
 #define MOUNT_ROMS 1
#endif

#ifndef WM_REQUEST
 #undef WM_CUSTOM_COMBO
#endif

#define IS_ON_XMB		(GetCurrentRunningMode() == 0)
#define IS_INGAME		(GetCurrentRunningMode() != 0)

#include "common.h"

#include "cobra/cobra.h"
#include "cobra/storage.h"
#include "vsh/game_plugin.h"
#include "vsh/netctl_main.h"
#include "vsh/xregistry.h"
#include "vsh/vsh.h"
#include "vsh/vshnet.h"
#include "vsh/vshmain.h"
#include "vsh/explore_plugin.h"

#include "include/thread.h"
#include "include/paths.h"

static char _game_TitleID[16]; //#define _game_TitleID  _game_info+0x04
static char _game_Title  [64]; //#define _game_Title    _game_info+0x14

//static char _game_info[0x120];
static char search_url[50];

#ifdef COBRA_ONLY
 #include "cobra/netiso.h"
/////////////////////////////////////
 #ifdef LITE_EDITION
	#define EDITION_ " [Lite]"
 #elif defined(PS3NET_SERVER) && defined(NET3NET4) && defined(XMB_SCREENSHOT)
	#define EDITION_ " [Full]"
 #else
  #ifdef PS3MAPI
	#ifdef REX_ONLY
		#define EDITION_ " [Rebug-PS3MAPI]"
	#else
		#define EDITION_ " [PS3MAPI]"
	#endif
  #else
   #ifdef REX_ONLY
	#define EDITION_ " [Rebug]"
   #else
	#define EDITION_ ""
   #endif
  #endif
 #endif
#else
 #define EDITION_ " [nonCobra]"
 #undef PS3MAPI
 #undef WM_PROXY_SPRX
#endif

#ifdef USE_NTFS
#define EDITION			" (NTFS)" EDITION_			// webMAN version (NTFS)
#else
#define EDITION			EDITION_					// webMAN version
#endif
/////////////////////////////////////

SYS_MODULE_INFO(WWWD, 0, 1, 1);
SYS_MODULE_START(wwwd_start);
SYS_MODULE_STOP(wwwd_stop);
SYS_MODULE_EXIT(wwwd_stop);

#define WM_APPNAME			"webMAN"
#define WM_VERSION			"1.47.28 MOD"
#define WM_APP_VERSION		WM_APPNAME " " WM_VERSION
#define WEBMAN_MOD			WM_APPNAME " MOD"

///////////// PS3MAPI BEGIN //////////////
#ifdef COBRA_ONLY
 #define SYSCALL8_OPCODE_PS3MAPI					0x7777

 #define PS3MAPI_OPCODE_SET_ACCESS_KEY				0x2000
 #define PS3MAPI_OPCODE_REQUEST_ACCESS				0x2001
 #define PS3MAPI_OPCODE_PCHECK_SYSCALL8 			0x0094
 #define PS3MAPI_OPCODE_PDISABLE_SYSCALL8 			0x0093

// static u64 ps3mapi_key = 0;
 static int pdisable_sc8 = NONE;
 #define PS3MAPI_ENABLE_ACCESS_SYSCALL8		//if(syscalls_removed) { system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_REQUEST_ACCESS, ps3mapi_key); }
 #define PS3MAPI_DISABLE_ACCESS_SYSCALL8	//if(syscalls_removed && !is_mounting) { system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_ACCESS_KEY, ps3mapi_key); }

 #define PS3MAPI_REENABLE_SYSCALL8			{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PCHECK_SYSCALL8); pdisable_sc8 = (int)p1;} \
											if(pdisable_sc8 > 0) { system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, 0); }
 #define PS3MAPI_RESTORE_SC8_DISABLE_STATUS	if(pdisable_sc8 > 0) { system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, pdisable_sc8); }
#else
 #define PS3MAPI_ENABLE_ACCESS_SYSCALL8
 #define PS3MAPI_DISABLE_ACCESS_SYSCALL8
 #define PS3MAPI_REENABLE_SYSCALL8
 #define PS3MAPI_RESTORE_SC8_DISABLE_STATUS
#endif
///////////// PS3MAPI END ////////////////

#define SC_SYS_POWER 					(379)
#define SYS_SOFT_REBOOT 				0x0200
#define SYS_HARD_REBOOT					0x1200
#define SYS_REBOOT						0x8201 /*load LPAR id 1*/
#define SYS_SHUTDOWN					0x1100

#define SC_RING_BUZZER  				(392)

#define BEEP1 { system_call_3(SC_RING_BUZZER, 0x1004, 0x4,   0x6); }
#define BEEP2 { system_call_3(SC_RING_BUZZER, 0x1004, 0x7,  0x36); }
#define BEEP3 { system_call_3(SC_RING_BUZZER, 0x1004, 0xa, 0x1b6); }

////////////

#define WWWPORT			(80)
#define FTPPORT			(21)
#define NETPORT			(38008)
#define PS3MAPIPORT		(7887)

#define WWW_BACKLOG		(2001)
#define	FTP_BACKLOG		(7)
#define	NET_BACKLOG		(4)
#define	PS3MAPI_BACKLOG	(4)

int active_socket[4] = {NONE, NONE, NONE, NONE}; // 0=FTP, 1=WWW, 2=PS3MAPI, 3=PS3NETSRV

////////////

static const u32 MODE  = 0777;
static const u32 DMODE = (CELL_FS_S_IFDIR | 0777);
static const u32 NOSND = (S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);

#define TITLE_ID_LEN	9

#define FAILED		-1

#define ip_size			0x10

////////////

#define START_DAEMON		(0xC0FEBABE)
#define REFRESH_CONTENT		(0xC0FEBAB0)
#define WM_FILE_REQUEST		(0xC0FEBEB0)

static u8 profile = 0;

static u8 loading_html = 0;
static u8 refreshing_xml = 0;

#ifdef SYS_BGM
static u8 system_bgm = 0;
#endif

#define NTFS 			(12)

#define APP_GAME  0xFF

static bool show_info_popup = false;
static bool do_restart = false;
static bool payload_ps3hen = false;

#ifdef USE_DEBUG
 static int debug_s = -1;
 static char debug[256];
#endif

static volatile u8 wm_unload_combo = 0;
static volatile u8 working = 1;
static u8 max_mapped = 0;
static int init_delay = 0;

static u8 CELL_PAD_CIRCLE_BTN = CELL_PAD_CTRL_CIRCLE;

static bool syscalls_removed = false;

static float c_firmware = 0.0f;
static u8 dex_mode = 0;

#ifndef LITE_EDITION
static u8 chart_init = 0;
static u8 chart_count = 0;
#endif

#ifdef SYS_ADMIN_MODE
static u8 sys_admin = 0;
static u8 pwd_tries = 0;
#else
static u8 sys_admin = 1;
#endif

#ifdef OFFLINE_INGAME
static s32 net_status = NONE;
#endif

static u64 SYSCALL_TABLE = 0;
static u64 LV2_OFFSET_ON_LV1; // value is set on detect_firmware -> 0x1000000 on 4.46, 0x8000000 on 4.76/4.78

enum get_name_options
{
	NO_EXT    = 0,
	GET_WMTMP = 1,
	NO_PATH   = 2,
};

enum is_binary_options
{
	WEB_COMMAND = 0,
	BINARY_FILE = 1,
	FOLDER_LISTING = 2
};

enum cp_mode_options
{
	CP_MODE_NONE = 0,
	CP_MODE_COPY = 1,
	CP_MODE_MOVE = 2,
};

static CellRtcTick rTick, gTick;

#ifdef GET_KLICENSEE
int npklic_struct_offset = 0; u8 klic_polling = 0;
#endif

#ifdef COBRA_ONLY
static bool is_mamba = false;
#endif
static u16 cobra_version = 0;

static bool is_mounting = false;
static bool copy_aborted = false;
static u8 automount = 0;
static u8 ftp_state = 0;

#ifdef COPY_PS3
static char current_file[STD_PATH_LEN + 1];
static char cp_path[STD_PATH_LEN + 1];  // cut/copy/paste buffer
static u8 cp_mode = CP_MODE_NONE;       // 0 = none / 1 = copy / 2 = cut/move
static void parse_script(const char *script_file);
static bool script_running = false;
#endif

#define ONLINE_TAG		"[online]"
#define OFFLINE_TAG		"[offline]"
#define AUTOPLAY_TAG	" [auto]"

static char fw_version[8] = "4.xx";
static char local_ip[16] = "127.0.0.1";

static void show_msg(char* msg);
static void sys_get_cobra_version(void);

static bool not_exists(const char* path);
static bool file_exists(const char* path);
static bool isDir(const char* path);
static void _file_copy(char *file1, char *file2);
static int add_breadcrumb_trail(char *pbuffer, const char *param);

size_t read_file(const char *file, char *data, size_t size, s32 offset);
int save_file(const char *file, const char *mem, s64 size);
int wait_for(const char *path, u8 timeout);

int (*vshtask_notify)(int, const char *) = NULL;
int (*View_Find)(const char *) = NULL;
int (*plugin_GetInterface)(int,int) = NULL;

#include "include/string.h"
#include "include/wm_config.h"
#include "include/html.h"
#include "include/peek_poke.h"
#include "include/idps.h"
#include "include/led.h"
#include "include/vpad.h"
#include "include/socket.h"
#include "include/language.h"
#include "include/fancontrol.h"
#include "include/firmware.h"
#include "include/ntfs.h"

#ifdef USE_NTFS

static ntfs_md *mounts = NULL;
static int mountCount = NTFS_UNMOUNTED;
static bool skip_prepntfs = false;
static bool root_check = true; // check ntfs volumes accessing file manager's root only once; check is re-enabled if save settings, refresh_xml or unmount game

static int prepNTFS(u8 clear);
#endif

int wwwd_start(size_t args, void *argp);
int wwwd_stop(void);
static void stop_prx_module(void);
static void unload_prx_module(void);

#ifdef REMOVE_SYSCALLS
static void remove_cfw_syscalls(bool keep_ccapi);
#ifdef PS3MAPI
static void restore_cfw_syscalls(void);
#endif
#endif

#ifdef PKG_HANDLER
static int installPKG(const char *pkgpath, char *msg);
static void installPKG_all(char *path, bool delete_after_install);
#endif

static void handleclient_www(u64 conn_s_p);

static void do_umount(bool clean);
static void mount_autoboot(void);
static bool mount_game(const char *_path, u8 do_eject);
#ifdef COBRA_ONLY
static void do_umount_iso(void);
static void unload_vsh_gui(void);
static void set_app_home(const char *game_path);
#endif

static size_t get_name(char *name, const char *filename, u8 cache);
static void get_cpursx(char *cpursx);
static void get_last_game(char *last_path);
static void add_game_info(char *buffer, char *templn, bool is_cpursx);
static void mute_snd0(bool scan_gamedir);

static bool from_reboot = false;
static bool is_busy = false;
static u8 mount_unk = EMU_OFF;

#include "include/buffer_size.h"
#include "include/eject_insert.h"

#ifdef COBRA_ONLY

#include "include/cue_file.h"
#include "include/psxemu.h"
#include "include/rawseciso.h"
#include "include/netclient.h"
#include "include/netserver.h"

#endif //#ifdef COBRA_ONLY

#include "include/webchat.h"
#include "include/vsh.h"
#include "include/file.h"
#include "include/ps2_disc.h"
#include "include/ps2_classic.h"
#include "include/xmb_savebmp.h"
#include "include/singstar.h"
#include "include/autopoweroff.h"

#include "include/gamedata.h"

#include "include/debug_mem.h"
#include "include/fix_game.h"
#include "include/ftp.h"
#include "include/ps3mapi.h"
#include "include/stealth.h"
#include "include/process.h"
#include "include/video_rec.h"
#include "include/secure_file_id.h"

#include "include/games_html.h"
#include "include/games_xml.h"
#include "include/prepntfs.h"

#include "include/cpursx.h"
#include "include/snd0.h"
#include "include/setup.h"
#include "include/togglers.h"

#include "include/_mount.h"
#include "include/file_manager.h"

#include "include/pkg_handler.h"
#include "include/poll.h"
#include "include/md5.h"
#include "include/script.h"

#include "include/www_client.h"

static void wwwd_thread(u64 arg)
{
	////////////////////////////////////////

	led(YELLOW, BLINK_FAST);

#ifdef COPY_PS3
	memset(cp_path, 0, STD_PATH_LEN);
#endif

	//WebmanCfg *webman_config = (WebmanCfg*) wmconfig;
	read_settings();

	if(webman_config->blind) enable_dev_blind(NO_MSG);

	set_buffer_sizes(webman_config->foot);

#ifdef COPY_PS3
	parse_script("/dev_hdd0/boot_init.txt");
#endif

	if(webman_config->fanc)
	{
		if(webman_config->man_speed == FAN_AUTO) max_temp = webman_config->dyn_temp;
		set_fan_speed(webman_config->man_speed);
	}

	sys_ppu_thread_create(&thread_id_poll, poll_thread, (u64)webman_config->poll, THREAD_PRIO_POLL, THREAD_STACK_SIZE_POLL_THREAD, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_POLL);

#ifdef WM_REQUEST
	cellFsUnlink(WMREQUEST_FILE);
#endif

#ifdef COBRA_ONLY
	if(isDir(webman_config->home_url))
		set_app_home(webman_config->home_url);

	{sys_map_path((char*)"/app_home", NULL);}
#endif

	from_reboot = file_exists(WMNOSCAN);

	#ifdef AUTO_POWER_OFF
	restoreAutoPowerOff();
	#endif

	if(cobra_version >= 0x0800) sys_ppu_thread_sleep(2); // wait 2 seconds on cobra 8.x for network

	if(!webman_config->ftpd)
		sys_ppu_thread_create(&thread_id_ftpd, ftpd_thread, NULL, THREAD_PRIO, THREAD_STACK_SIZE_FTP_SERVER, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_FTP); // start ftp daemon immediately

	sys_ppu_thread_t t_id;
	sys_ppu_thread_create(&t_id, handleclient_www, (u64)START_DAEMON, THREAD_PRIO, THREAD_STACK_SIZE_WEB_CLIENT, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_CMD);

#ifdef PS3NET_SERVER
	if(!webman_config->netsrvd && (webman_config->ftp_port != NETPORT))
		sys_ppu_thread_create(&thread_id_netsvr, netsvrd_thread, NULL, THREAD_PRIO, THREAD_STACK_SIZE_NET_SERVER, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_NETSVR);
#endif

#ifdef PS3MAPI
	///////////// PS3MAPI BEGIN //////////// [requires PS3MAPI enabled in /setup.ps3, the option is found in "XMB/In-Game PAD SHORTCUTS", next to DEL CFW SYSCALLS]
	if(!webman_config->ftpd && (webman_config->ftp_port != PS3MAPIPORT) && (webman_config->sc8mode != 4))
		sys_ppu_thread_create(&thread_id_ps3mapi, ps3mapi_thread, NULL, THREAD_PRIO, THREAD_STACK_SIZE_PS3MAPI_SVR, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_PS3MAPI);
	///////////// PS3MAPI END //////////////
#endif

#ifdef USE_DEBUG
	u8 d_retries = 0;
again_debug:
	debug_s = connect_to_server("192.168.100.209", 38009);
	if(debug_s <  0) {d_retries++; sys_ppu_thread_sleep(2); if(d_retries < 10) goto again_debug;}
	if(debug_s >= 0) ssend(debug_s, "Connected...\r\n");
	sprintf(debug, "FC=%i T0=%i T1=%i\r\n", webman_config->fanc, webman_config->man_speed, webman_config->dyn_temp);
	ssend(debug_s, debug);
#endif

	//sys_ppu_thread_sleep(2);

	led(YELLOW, OFF);

#ifdef COPY_PS3
	parse_script("/dev_hdd0/autoexec.bat");
#endif

	////////////////////////////////////////

	int list_s = NONE;

relisten:
#ifdef USE_DEBUG
	ssend(debug_s, "Listening on port 80...");
#endif

	if(working) list_s = slisten(WWWPORT, WWW_BACKLOG);
	else goto end;

	if(list_s < 0)
	{
		sys_ppu_thread_sleep(1);
		if(working) goto relisten;
		else goto end;
	}

	active_socket[1] = list_s;

	//if(list_s >= 0)
	{
		#ifdef USE_DEBUG
		ssend(debug_s, " OK!\r\n");
		#endif

		int conn_s; u8 timeout = 0;

		while(working)
		{
			timeout = 0;
			while((loading_html > MAX_WWW_THREADS) && working)
			{
				sys_ppu_thread_usleep(40000);
				if(++timeout > 250) {loading_html = 0; sclose(&list_s); goto relisten;} // continue after 10 seconds
			}

			if(!working) goto end;

			if((conn_s = accept(list_s, NULL, NULL)) >= 0)
			{
				loading_html++;

				#ifdef USE_DEBUG
				ssend(debug_s, "*** Incoming connection... ");
				#endif

				sys_ppu_thread_t t_id;

				if(working) sys_ppu_thread_create(&t_id, handleclient_www, (u64)conn_s, THREAD_PRIO, THREAD_STACK_SIZE_WEB_CLIENT, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_WEB);
				else {sclose(&conn_s); break;}
			}
			else
			if((sys_net_errno == SYS_NET_EBADF) || (sys_net_errno == SYS_NET_ENETDOWN))
			{
				sclose(&list_s);
				if(working) goto relisten;
				else break;
			}
		}

	}
end:
	sclose(&list_s);
	sys_ppu_thread_exit(0);
}

int wwwd_start(size_t args, void *argp)
{
	cellRtcGetCurrentTick(&rTick); gTick = rTick;

	detect_firmware();

	if(!payload_ps3hen && !(webman_config->fanc)) // SYSCON
		sys_ppu_thread_sleep(webman_config->boots);

	if(set_fan_policy_offset) restore_set_fan_policy = peekq(set_fan_policy_offset); // sys 389 get_fan_policy

#ifdef PS3MAPI
 #ifdef REMOVE_SYSCALLS
	backup_cfw_syscalls();
 #endif
#endif

	View_Find = getNIDfunc("paf", 0xF21655F3, 0);
	plugin_GetInterface = getNIDfunc("paf", 0x23AFB290, 0);

#ifdef SYS_BGM
	BgmPlaybackEnable  = getNIDfunc("vshmain", 0xEDAB5E5E, 16*2);
	BgmPlaybackDisable = getNIDfunc("vshmain", 0xEDAB5E5E, 17*2);
#endif

	if(!payload_ps3hen) { ENABLE_INGAME_SCREENSHOT }

	//pokeq(0x8000000000003560ULL, 0x386000014E800020ULL); // li r3, 0 / blr
	//pokeq(0x8000000000003D90ULL, 0x386000014E800020ULL); // li r3, 0 / blr

	sys_ppu_thread_create(&thread_id_wwwd, wwwd_thread, NULL, THREAD_PRIO, THREAD_STACK_SIZE_WEB_SERVER, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_SVR);

#ifndef CCAPI
	_sys_ppu_thread_exit(0); // remove for ccapi compatibility
#endif
	return SYS_PRX_RESIDENT;
}

static void wwwd_stop_thread(u64 arg)
{
	working = 0;

	restore_settings();

	sys_ppu_thread_sleep(2);  // Prevent unload too fast (give time to other threads to finish)

	while(refreshing_xml) sys_ppu_thread_usleep(500000);

	u64 exit_code;

/*
	sys_ppu_thread_t t_id;

	#ifndef LITE_EDITION
	sys_ppu_thread_create(&t_id, netiso_stop_thread, NULL, THREAD_PRIO_STOP, THREAD_STACK_SIZE_STOP_THREAD, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
	sys_ppu_thread_join(t_id, &exit_code);
	#endif

	sys_ppu_thread_create(&t_id, rawseciso_stop_thread, NULL, THREAD_PRIO_STOP, THREAD_STACK_SIZE_STOP_THREAD, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
	sys_ppu_thread_join(t_id, &exit_code);

	while(netiso_loaded || rawseciso_loaded) {sys_ppu_thread_usleep(100000);}
*/

	sys_ppu_thread_join(thread_id_wwwd, &exit_code);

	if(thread_id_ftpd != SYS_PPU_THREAD_NONE)
	{
		sys_ppu_thread_join(thread_id_ftpd, &exit_code);
	}

#ifdef PS3NET_SERVER
	if(thread_id_netsvr != SYS_PPU_THREAD_NONE)
	{
		sys_ppu_thread_join(thread_id_netsvr, &exit_code);
	}
#endif

	if(thread_id_ftpd != SYS_PPU_THREAD_NONE)
	{
		sys_ppu_thread_join(thread_id_ftpd, &exit_code);
	}

#ifdef PS3MAPI
	if(thread_id_ps3mapi != SYS_PPU_THREAD_NONE)
	{
		sys_ppu_thread_join(thread_id_ps3mapi, &exit_code);
	}
#endif

	if(wm_unload_combo != 1)
	{
		if(thread_id_poll != SYS_PPU_THREAD_NONE)
		{
			sys_ppu_thread_join(thread_id_poll, &exit_code);
		}
	}

	sys_ppu_thread_exit(0);
}

int wwwd_stop(void)
{
	sys_ppu_thread_t t_id;
	int ret = sys_ppu_thread_create(&t_id, wwwd_stop_thread, NULL, THREAD_PRIO_STOP, THREAD_STACK_SIZE_STOP_THREAD, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);

	u64 exit_code;
	if (ret == 0) sys_ppu_thread_join(t_id, &exit_code);

	sys_ppu_thread_usleep(500000);

	unload_prx_module();

	_sys_ppu_thread_exit(0);

	return SYS_PRX_STOP_OK;
}
