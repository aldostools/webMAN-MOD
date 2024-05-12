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

#include "types.h"
#include "include/init/flags.h"

#define IS_ON_XMB		(GetCurrentRunningMode() == 0)
#define IS_INGAME		(GetCurrentRunningMode() != 0)

static vu8 wm_unload_combo = 0;
static vu8 working = 1;

#include "common.h"

#include "cobra/cobra.h"
#include "cobra/storage.h"
#include "vsh/download_plugin.h"
#include "vsh/explore_plugin.h"
#include "vsh/game_ext_plugin.h"
#include "vsh/game_plugin.h"
#include "vsh/netctl_main.h"
#include "vsh/paf.h"
#include "vsh/vsh.h"
#include "vsh/vshcommon.h"
#include "vsh/vshmain.h"
#include "vsh/vshnet.h"
#include "vsh/vshtask.h"
#include "vsh/xmb_plugin.h"

static char _game_TitleID[16]; //#define _game_TitleID  _game_info+0x04
static char _game_Title  [64]; //#define _game_Title    _game_info+0x14

//static char _game_info[0x120];
static char search_url[50];

#ifdef COBRA_ONLY
 #include "cobra/netiso.h"
/////////////////////////////////////
 #ifdef LITE_EDITION
	#define EDITION_ " [Lite]"
	#define _EDITION EDITION_
 #elif defined(PS3NET_SERVER) && defined(NET3NET4) && defined(XMB_SCREENSHOT)
	#define EDITION_ " [Full]"
	#define _EDITION EDITION_
 #else
  #define _EDITION ""
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
 #define _EDITION EDITION_
 #undef PS3MAPI
 #undef WM_PROXY_SPRX
#endif

#ifdef USE_NTFS
#define EDITION			" (NTFS)" EDITION_			// webMAN version (NTFS)
#else
#define EDITION			EDITION_					// webMAN version
#endif
/////////////////////////////////////

SYS_MODULE_INFO(WWWD, 0, 1, 0);
SYS_MODULE_START(wwwd_start);
SYS_MODULE_STOP(wwwd_stop);
SYS_MODULE_EXIT(wwwd_stop);

#define WM_APPNAME			"webMAN"
#define WM_VERSION			"1.47.46b MOD"
#define WM_APP_VERSION		WM_APPNAME " " WM_VERSION
#define WEBMAN_MOD			WM_APPNAME " MOD"

#include "include/init/timer.h"
#include "include/init/thread.h"
#include "include/init/paths.h"
#include "include/init/string.h"
#include "include/init/wm_config.h"

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

////// PS3MAPI VSH PLUGIN MANAGEMENT //////
static int ps3mapi_get_vsh_plugin_slot_by_name(const char *name, int mode);
#define get_free_slot(a)	 ps3mapi_get_vsh_plugin_slot_by_name(PS3MAPI_FIND_FREE_SLOT, 0)
#define unload_vsh_plugin(a) ps3mapi_get_vsh_plugin_slot_by_name(a, 1)
#define load_vsh_plugin(a)   ps3mapi_get_vsh_plugin_slot_by_name(a, 2)
#define toggle_vsh_plugin(a) ps3mapi_get_vsh_plugin_slot_by_name(a, 3)
#define load_vsh_gui(a)      ps3mapi_get_vsh_plugin_slot_by_name(a, 4)
#define load_vsh_module(a)   ps3mapi_get_vsh_plugin_slot_by_name(a, 5)
///////////////////////////////////////////

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
#define FTP_BACKLOG		(7)
#define NET_BACKLOG		(4)
#define PS3MAPI_BACKLOG	(4)

static int active_socket[4] = {NONE, NONE, NONE, NONE}; // 0=FTP, 1=WWW, 2=PS3MAPI, 3=PS3NETSRV

////////////

static const u32 MODE  = 0777; // S_IRWXO | S_IRWXU | S_IRWXG
static const u32 DMODE = (CELL_FS_S_IFDIR | 0777);
static const u32 NOSND = (S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);

#define TITLE_ID_LEN	9

#define FAILED		-1

#define ip_size			0x10

////////////

#define START_DAEMON		(0xC0FEBABE)
#define REFRESH_CONTENT		(0xC0FEBAB0)
#define WM_FILE_REQUEST		(0xC0FEBEB0)

static u8 profile = 0; // 0 = default, 1-4 = user profiles, 5 = all profiles

static vu8 loading_html = 0;
static vu8 refreshing_xml = 0;

#ifdef SYS_BGM
static u8 system_bgm = 0;
#endif

#define APP_GAME  0xFF

static bool from_xmb_ps3 = false;
static bool show_info_popup = false;
static bool show_scan_progress = false;
static bool is_custom_popup = false;
static bool do_restart = false;
static bool payload_ps3hen = false;

#ifdef USE_DEBUG
 static int debug_s = -1;
 static char debug[256];
#endif

static u8 max_mapped = 0;
static int init_delay = 0;

static u8 CELL_PAD_CIRCLE_BTN = CELL_PAD_CTRL_CIRCLE;

static bool syscalls_removed = false;

static float c_firmware = 0.0f;
static u8 dex_mode = 0;
static u8 pex_mode = 0;

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
	WM_COVERS = 2,
	NO_PATH   = 3,
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

static u64 copied_size  = 0;
static bool is_mounting = false;
static bool copy_aborted = false;
static u8 automount = 0;
static u8 ftp_state = 0; //  1 = sending, 2 = receiving
static u64  current_size = 0;

#ifdef COPY_PS3
static char current_file[STD_PATH_LEN + 1];
static char cp_path[STD_PATH_LEN + 1];  // cut/copy/paste buffer
static u8 cp_mode = CP_MODE_NONE;       // 0 = none / 1 = copy / 2 = cut/move
static u64 file_size(const char* path);
static bool script_running = false;
#endif

#define ONLINE_TAG		"[online]"
#define OFFLINE_TAG		"[offline]"
#define AUTOPLAY_TAG	" [auto]"

static char fw_version[8] = "4.xx";
static char local_ip[16] = "127.0.0.1";

#ifdef MOUNT_ROMS
static char *ROMS_EXTENSIONS = NULL;
#endif

static void show_msg(const char *text);
static void show_status(const char *label, u8 status);
static void sys_get_cobra_version(void);
static void get_sys_info(char *msg, u8 op, bool nolabel);

static bool isDir(const char* path);
static bool file_exists(const char* path);
static const char *get_ext(const char *path);
static bool is_app_dir(const char *path, const char *app_name);
static void force_copy(const char *file1, char *file2);
static int add_breadcrumb_trail(char *pbuffer, const char *param);
static int add_breadcrumb_trail2(char *pbuffer, const char *label, const char *param);
static char *remove_filename(const char *path);
static int save_settings(void);

#ifdef PATCH_GAMEBOOT
static void patch_gameboot(u8 boot_type);
#endif
#ifdef UNLOCK_SAVEDATA
static u8 unlock_param_sfo(const char *param_sfo, unsigned char *mem, u16 sfo_size);
#endif

size_t read_file(const char *file, char *data, size_t size, s32 offset);
int save_file(const char *file, const char *mem, s64 size);
int wait_for(const char *path, u8 timeout);

//static int (*vshtask_notify)(int, const char *) = NULL;
//static int (*View_Find)(const char *) = NULL;
//static int (*plugin_GetInterface)(int,int) = NULL;

#include "include/init/ntfs.h"
#include "include/init/compare.h"
#include "include/init/eval.h"
#include "include/ps3mapi/peek_poke.h"
#include "include/init/socket.h"

#include "include/feat/syscall.h"
#include "include/feat/xregistry.h"
#include "include/feat/idps.h"
#include "include/feat/vpad.h"

#include "include/init/html.h"
#include "include/init/language.h"
#include "include/poll/fancontrol.h"
#include "include/init/firmware.h"

#include "include/notify/led.h"
#include "include/notify/vsh_notify.h"
#include "include/notify/show_msg2.h"

#ifdef USE_NTFS

static ntfs_md *mounts = NULL;
static int mountCount = NTFS_UNMOUNTED;
static bool skip_prepntfs = false;
static bool root_check = true; // check ntfs volumes accessing file manager's root only once; check is re-enabled if save settings, refresh_xml or unmount game

static int prepNTFS(u8 clear);
#endif

int wwwd_start(u64 arg);
int wwwd_stop(void);

#ifdef REMOVE_SYSCALLS
static void remove_cfw_syscalls(bool keep_ccapi);
#ifdef PS3MAPI
static void restore_cfw_syscalls(void);
#endif
#endif

#ifdef PKG_HANDLER
static int installPKG(const char *pkgpath, char *msg);
static void installPKG_all(const char *path, bool delete_after_install);
#endif

static void start_www(u64 conn_s_p);
static void handleclient_www(u64 conn_s_p);
static void do_web_command(u64 conn_s_p, const char *wm_url);

static void do_umount(bool clean);
static void mount_autoboot(void);
static bool mount_game(const char *_path, u8 do_eject);
#ifdef COBRA_ONLY
static void do_umount_iso(bool clean);
static void unload_vsh_gui(void);
static void set_app_home(const char *game_path);
static bool is_iso_0(const char *filename);
#endif

static void get_cpursx(char *cpursx);
static void get_last_game(char *last_path);
static void add_game_info(char *buffer, char *templn, u8 is_cpursx);

static bool use_open_path = false;
static bool from_reboot = false;
static bool wm_reload = false;
static bool is_busy = false;
static u8 mount_unk = EMU_OFF;

#include "include/init/process.h"
#include "include/init/buffer_size.h"
#include "include/init/vsh.h"

#ifdef COBRA_ONLY
#include "include/mount/cue_file.h"
#include "include/mount/psxemu.h"
#include "include/mount/netclient.h"
#include "include/mount/netserver.h"
#include "include/mount/netiso.h"
#include "include/mount/rawseciso.h"
#endif

#include "include/file/file.h"
#include "include/mount/ps2_disc.h"
#include "include/mount/ps2_classic.h"
#include "include/mount/fix_game.h" // + param_sfo.h

#include "include/unused/webchat.h"
#include "include/unused/toggle_dlna.h"
#include "include/unused/ingame_music.h"
#include "include/unused/secure_file_id.h"

#include "include/feat/script.h"
#include "include/feat/rospatch.h"
#include "include/feat/video_rec.h"
#include "include/feat/xmb_savebmp.h"
#include "include/feat/ingame_screenshot.h"
#include "include/feat/vsh_random_res.h"

#include "include/init/singstar.h"
#include "include/init/autopoweroff.h"

#include "include/mount/eject_insert.h"
#include "include/mount/gamedata.h"

#include "include/ftp.h"

#include "include/ps3mapi/debug_mem.h"
#include "include/ps3mapi/ps3mapi.h"
#include "include/ps3mapi/ps3mapi_payload.h"
#include "include/ps3mapi/ps3mapi_server.h"
#include "include/ps3mapi/stealth.h"
#include "include/ps3mapi/artemis.h"

#include "include/scan/snd0.h"
#include "include/scan/games_html.h"
#include "include/scan/games_xml.h"
#include "include/scan/prepntfs.h"

#include "include/setup.h"
#include "include/cpursx.h"

#include "include/mount/patch_gameboot.h"
#include "include/mount/patch_ps2demo.h"
#include "include/mount/mount.h"

#include "include/file_manager.h"

#include "include/feat/pkg_handler.h"
#include "include/poll/poll.h"

#include "include/www/www_client.h"
#include "include/www/www_start.h"

static void wwwd_thread(u64 arg)
{
	////////////////////////////////////////

	led(YELLOW, BLINK_FAST);

	//WebmanCfg *webman_config = (WebmanCfg*) wmconfig;
	read_settings();

#ifdef COBRA_ONLY
	#ifdef VISUALIZERS
	map_vsh_resource(4, MAP_SELECTED, html_base_path, false); // coldboot_*.ac3
	map_vsh_resource(3, MAP_SELECTED, html_base_path, false); // lines.qrc
	map_vsh_resource(7, MAP_SELECTED, html_base_path, false); // impose_plugin.rco
	map_vsh_resource(8, MAP_SELECTED, html_base_path, false); // xmb_plugin_normal.rco + xmb_ingame.rco
	*html_base_path = NULL;
	#endif

	map_patched_modules();
#endif

	if(webman_config->blind) enable_dev_blind(NO_MSG);

	set_buffer_sizes(webman_config->foot);

	#ifdef MOUNT_ROMS
	size_t fsize = file_ssize(WM_ROMS_EXTENSIONS);
	if(fsize && (fsize <= 1024))
	{
		ROMS_EXTENSIONS = malloc(fsize);
		read_file(WM_ROMS_EXTENSIONS, ROMS_EXTENSIONS, fsize, 0);
	}
	if(!ROMS_EXTENSIONS) ROMS_EXTENSIONS = (char *)".BIN.ISO.CUE.CCD.IMG.ZIP";
	#endif

	#ifdef COPY_PS3
	_memset(cp_path, sizeof(cp_path));
	start_event(EVENT_BOOT_INIT);
	#endif

	if(webman_config->fanc)
	{
		if(webman_config->man_speed == FAN_AUTO) max_temp = webman_config->dyn_temp;
		set_fan_speed(webman_config->man_speed);
	}

	#ifdef NOBD_PATCH
	apply_noBD_patches(webman_config->noBD, true);
	#endif

	#ifdef WM_REQUEST
	cellFsUnlink(WM_REQUEST_FILE);
	#endif

	sys_ppu_thread_create(&thread_id_poll, poll_thread, (u64)webman_config->poll, THREAD_PRIO_POLL, THREAD_STACK_SIZE_POLL_THREAD, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_POLL);

	#ifdef COBRA_ONLY
	set_app_home(NULL); // initialize /app_home
	#endif

	from_reboot = file_exists(WM_NOSCAN_FILE) || file_exists(WM_RELOAD_FILE);

	#ifdef AUTO_POWER_OFF
	restoreAutoPowerOff();
	#endif

	if(cobra_version >= 0x0800) sys_ppu_thread_sleep(3); // wait 3 seconds on cobra 8.x for network

	if(!webman_config->ftpd)
		sys_ppu_thread_create(&thread_id_ftpd, ftpd_thread, NULL, THREAD_PRIO, THREAD_STACK_SIZE_FTP_SERVER, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_FTP); // start ftp daemon immediately

	sys_ppu_thread_t t_id;
	sys_ppu_thread_create(&t_id, start_www, (u64)START_DAEMON, THREAD_PRIO, THREAD_STACK_SIZE_WEB_CLIENT, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_CMD);

	#ifdef PS3NET_SERVER
	if(!webman_config->netsrvd && (webman_config->ftp_port != NETPORT))
		sys_ppu_thread_create(&thread_id_netsvr, netsvrd_thread, NULL, THREAD_PRIO, THREAD_STACK_SIZE_NET_SERVER, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_NETSVR);
	#endif

	#ifdef PS3MAPI
	///////////// PS3MAPI BEGIN //////////// [requires PS3MAPI enabled in /setup.ps3, the option is found in "XMB/In-Game PAD SHORTCUTS", next to DEL CFW SYSCALLS]
	if(!webman_config->ftpd && (webman_config->ftp_port != PS3MAPIPORT) && (webman_config->sc8mode != PS3MAPI_DISABLED))
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

	// sys_ppu_thread_sleep(2);

	led(YELLOW, OFF);

	start_event(EVENT_AUTOEXEC);

	////////////////////////////////////////

	int list_s = NONE;

relisten:
	#ifdef USE_DEBUG
	ssend(debug_s, "Listening on port 80...");
	#endif

	if(!working) goto end;

	list_s = slisten(WWWPORT, WWW_BACKLOG);

	if(list_s < 0)
	{
		sys_ppu_thread_sleep(1);
		goto relisten;
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
			// wait for free thread
			for(timeout = 0; loading_html > MAX_WWW_THREADS; timeout++)
			{
				if(!working) goto end;
				if(timeout > 200)
				{
					// respawn http after wait for 10 seconds
					sys_net_abort_socket(list_s, SYS_NET_ABORT_STRICT_CHECK);
					sys_ppu_thread_sleep(1);
					sclose(&list_s);

					loading_html = 0;
					goto relisten;
				}
				sys_ppu_thread_usleep(50000);
			}

			if(!working) goto end;

			if((conn_s = accept(list_s, NULL, NULL)) >= 0)
			{
				if(!working) {sclose(&conn_s); break;}

				#ifdef USE_DEBUG
				ssend(debug_s, "*** Incoming connection... ");
				#endif

				loading_html++;

				sys_ppu_thread_t t_id;
				sys_ppu_thread_create(&t_id, handleclient_www, (u64)conn_s, THREAD_PRIO, THREAD_STACK_SIZE_WEB_CLIENT, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_WEB);
			}
			else if((sys_net_errno == SYS_NET_EBADF) || (sys_net_errno == SYS_NET_ENETDOWN))
			{
				sclose(&list_s);
				goto relisten;
			}
		}
	}

end:
	sclose(&list_s);

	//thread_id_wwwd = SYS_PPU_THREAD_NONE;
	sys_ppu_thread_exit(0);
}

int wwwd_start(u64 arg)
{
	cellRtcGetCurrentTick(&rTick); gTick = rTick;

	detect_firmware();

	if(!payload_ps3hen && !(webman_config->fanc)) // SYSCON
	{
		if(not_exists(WM_RELOAD_FILE)) sys_ppu_thread_sleep(webman_config->boots);
	}

#ifdef PS3MAPI
 #ifdef REMOVE_SYSCALLS
	backup_cfw_syscalls();
 #endif
#endif

	//View_Find = getNIDfunc("paf", 0xF21655F3, 0);
	//plugin_GetInterface = getNIDfunc("paf", 0x23AFB290, 0);

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
	show_msg(STR_WMUNL);

	#ifdef FPS_OVERLAY
	if(overlay_enabled)
		disable_progress(); // clear message address
	#endif

	restore_settings();

	thread_join(thread_id_wwwd);

	thread_join(thread_id_ftpd);

	#ifdef PS3NET_SERVER
	thread_join(thread_id_netsvr);
	#endif

	#ifdef PS3MAPI
	thread_join(thread_id_ps3mapi);
	#endif

	#ifdef ARTEMIS_PRX
	thread_join(thread_id_art);
	#endif

	if(!wm_unload_combo)
	{
		thread_join(thread_id_poll);
	}

	#ifdef REMOVE_SYSCALLS
	remove_cfw_syscall8(); // remove cobra if syscalls were disabled
	#endif

	sys_ppu_thread_exit(0);
}

int wwwd_stop(void)
{
	while(is_mounting | refreshing_xml) sys_ppu_thread_usleep(50000);
	#ifdef MOUNT_ROMS
	if(ROMS_EXTENSIONS) free(ROMS_EXTENSIONS);
	#endif

	sys_ppu_thread_t t_id;
	sys_ppu_thread_create(&t_id, wwwd_stop_thread, NULL, THREAD_PRIO_STOP, THREAD_STACK_SIZE_STOP_THREAD, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
	thread_join(t_id);

	finalize_module();

	_sys_ppu_thread_exit(0);
	return SYS_PRX_STOP_OK;
}
