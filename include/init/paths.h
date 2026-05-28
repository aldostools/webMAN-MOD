#define VSH_MODULE_DIR		"/dev_flash/vsh/module/"
#define VSH_MODULE_PATH		"/dev_blind/vsh/module/"
#define VSH_ETC_PATH		"/dev_blind/vsh/etc/"
#define PS2_EMU_PATH		"/dev_blind/ps2emu/"
#define PSP_EMU_PATH		"/dev_blind/pspemu/"
#define REBUG_COBRA_PATH	"/dev_blind/rebug/cobra/"
#define HABIB_COBRA_PATH	"/dev_blind/habib/cobra/"
#define SYS_COBRA_PATH		"/dev_blind/sys/"
#define REBUG_TOOLBOX		"/dev_hdd0//game/RBGTLBOX2/USRDIR/"
#define COLDBOOT_PATH		"/dev_blind/vsh/resource/coldboot.raf"
#define IMAGEFONT_PATH		"/dev_flash/vsh/resource/imagefont.bin"

#define ORG_LIBFS_PATH		"/dev_flash/sys/external/libfs.sprx"
#define NEW_LIBFS_PATH		"/./dev_hdd0/tmp/wm_res/libfs.sprx"

#define ORG_GAME_EXT_PATH	"/dev_flash/vsh/module/game_ext_plugin.sprx"
#define NEW_GAME_EXT_PATH	"/./dev_hdd0/tmp/gameboot/game_ext_plugin.sprx"

#define ORG_EXPLORE_PLUGIN_PATH	"/dev_flash/vsh/module/explore_plugin.sprx"
#define NEW_EXPLORE_PLUGIN_PATH	"/./dev_hdd0/tmp/explore_plugin.sprx"

#define ORG_LIBAUDIO_PATH		"/dev_flash/sys/external/libaudio.sprx"
#define NEW_LIBAUDIO_PATH		"/./dev_hdd0/tmp/libaudio.sprx"

#define CUSTOM_RENDER_PLUGIN_RCO	"/dev_flash/vsh/resource/custom_render_plugin.rco"
#define GAMEBOOT_MULTI_AC3			"/dev_flash/vsh/resource/gameboot_multi.ac3"
#define GAMEBOOT_STEREO_AC3			"/dev_flash/vsh/resource/gameboot_stereo.ac3"

#define NPSIGNIN_PLUGIN_RCO		"/dev_flash/vsh/resource/npsignin_plugin.rco"
#define NPSIGNIN_PLUGIN_OFF		"/./dev_hdd0/tmp/wm_res/npsignin_plugin.rco"
#define NPSIGNIN_PLUGIN_ON		"/./dev_flash/vsh/resource/npsignin_plugin.rco"

#define ORG_PS1_ROM_PATH	"/dev_flash/ps1emu/ps1_rom.bin"
#define NEW_PS1_ROM_PATH	"/dev_hdd0/tmp/wm_res/ps1_bios.bin"

#define XMB_DISC_ICON		"/dev_hdd0/tmp/game/ICON0.PNG"
#define CATEGORY_GAME_XML	"/dev_flash/vsh/resource/explore/xmb/category_game.xml"

#define APP_HOME_DIR		"/app_home/PS3_GAME"

#define HDD0_HOME_DIR		"/dev_hdd0/home"
#define HDD0_GAME_DIR		"/dev_hdd0/game/"
#define _HDD0_GAME_DIR		"/dev_hdd0//game/"

#define INT_HDD_ROOT_PATH	"/dev_hdd0/"
#define HDD_PACKAGES_PATH	"/dev_hdd0/packages"
#define DEFAULT_PKG_PATH	"/dev_hdd0/packages/"
#define TEMP_DOWNLOAD_PATH	"/dev_hdd0/tmp/downloader/"

#define MANAGUNZ			_HDD0_GAME_DIR "MANAGUNZ0/USRDIR"	// ManaGunZ folder
#define MM_ROOT_STD			_HDD0_GAME_DIR "BLES80608/USRDIR"	// multiMAN root folder
#define MM_ROOT_SSTL		_HDD0_GAME_DIR "NPEA00374/USRDIR"	// multiman SingStar® Stealth root folder
#define MM_ROOT_STL			"/dev_hdd0/tmp/game_repo/main"		// stealthMAN root folder

#define XMBMANPLS_PATH		_HDD0_GAME_DIR "XMBMANPLS/USRDIR"
#define PS2CONFIG_PATH		_HDD0_GAME_DIR "PS2CONFIG/USRDIR"

#define TMP_DIR				"/dev_hdd0/tmp"

#define WMTMP				TMP_DIR "/wmtmp"				// webMAN work/temp folder
#define WMTMP_COVERS		TMP_DIR "/wmtmp/covers"			// webMAN covers folder
#define WM_RES_PATH			TMP_DIR "/wm_res"				// webMAN resources
#define WM_LANG_PATH		TMP_DIR "/wm_lang"				// webMAN language folder
#define WM_ICONS_PATH		TMP_DIR "/wm_icons"				// webMAN icons folder
#define WM_COMBO_PATH		TMP_DIR "/wm_combo"				// webMAN custom combos folder
#define WM_INGAME_PATH		TMP_DIR "/wm_ingame"			// webMAN ingame scripts
#define WM_GAMEBOOT_PATH	TMP_DIR "/gameboot"				// gameboot animations per console emulator
#define WM_EXTRACT_PATH		TMP_DIR "/extract"				// PKGLAUNCH extract path for zip, rar, 7z, gzip, tar

#define WM_CONFIG_FILE		TMP_DIR "/wm_config.bin"		// webMAN config file
#define WM_NOSCAN_FILE		TMP_DIR "/wm_noscan"			// webMAN config file to skip on boot
#define WM_UNLOAD_FILE		TMP_DIR "/wm_unload"			// webMAN config file to force unload the plugin from memory; causes an interrupt(exception) in the user PPU Thread that freezes some consoles
#define WM_RELOAD_FILE		TMP_DIR "/wm_reload"			// webMAN config file to skip on reload
#define WM_REQUEST_FILE		TMP_DIR "/wm_request"			// webMAN request file
#define WM_NETDISABLED		TMP_DIR "/wm_netdisabled"		// webMAN config file to re-enable network

#define SC_LOG_FILE			TMP_DIR "/log.txt"
#define ARTEMIS_CODES		TMP_DIR "/art"
#define ARTEMIS_CODES_FILE	ARTEMIS_CODES ".txt"
#define ARTEMIS_CODES_L2	ARTEMIS_CODES "_l2.txt"
#define ARTEMIS_CODES_R2	ARTEMIS_CODES "_r2.txt"
#define ARTEMIS_CODES_LOG	ARTEMIS_CODES ".log"

#define WM_PROXY			"/." WM_RES_PATH "/wm_proxy.sprx"

#define WM_ONLINE_IDS_FILE	WM_RES_PATH "/wm_online_ids.txt"	// webMAN config file to skip disable network setting on these title ids
#define WM_OFFLINE_IDS_FILE	WM_RES_PATH "/wm_offline_ids.txt"	// webMAN config file to disable network setting on specific title ids (overrides wm_online_ids.txt)

#define WM_IGNORE_FILES		WM_RES_PATH "/wm_ignore.txt"	// webMAN config file to ignore files during content scanning
#define WM_ROMS_EXTENSIONS	WM_RES_PATH "/roms_extensions.txt"
#define WM_ROMS_PATHS		WM_RES_PATH "/roms_paths.txt"

#define FILE_LIST_TXT		WMTMP "/filelist.txt"
#define FILE_LIST_HTM		WMTMP "/filelist.htm"

#define SLAUNCH_FILE		WMTMP "/slist.bin"
#define DEL_CACHED_ISO		WMTMP "/deliso.txt"

#define LAST_GAME_TXT		WMTMP "/last_game.txt"
#define LAST_GAMES_BIN		WMTMP "/last_games.bin"

#define VSH_MENU_IMAGES		"/dev_hdd0/plugins/images"

#define PKGLAUNCH_ID		"PKGLAUNCH"
#define PKGLAUNCH_DIR		_HDD0_GAME_DIR PKGLAUNCH_ID
#define PKGLAUNCH_ICON		PKGLAUNCH_DIR "/ICON0.PNG"
#define PKGLAUNCH_PS3_GAME	PKGLAUNCH_DIR "/PS3_GAME"

#define RETROARCH_DIR0		_HDD0_GAME_DIR "RETROARCH"
#define RETROARCH_DIR1		_HDD0_GAME_DIR "SSNE10000"
#define RETROARCH_DIR2		_HDD0_GAME_DIR "SSNE10001"

#define RELOADXMB_DIR		_HDD0_GAME_DIR "RELOADXMB"
#define RELOADXMB_ISO		WM_RES_PATH "/RELOAD_XMB.ISO"

#define VSH_RESOURCE_DIR	"/dev_flash/vsh/resource/"
#define SYSMAP_EMPTY_DIR	"/." VSH_RESOURCE_DIR "AAA"		//redirect firmware update to empty folder (formerly redirected to "/dev_bdvd")

#define PS2_CLASSIC_TOGGLER		"/dev_hdd0/classic_ps2"

#define PS2_CLASSIC_LAUCHER_DIR		_HDD0_GAME_DIR "PS2U10000"
#define PS2_CLASSIC_ISO_ICON		PS2_CLASSIC_LAUCHER_DIR "/ICON0.PNG"
#define PS2_CLASSIC_PLACEHOLDER		PS2_CLASSIC_LAUCHER_DIR "/USRDIR"
#define PS2_CLASSIC_ISO_CONFIG		PS2_CLASSIC_LAUCHER_DIR "/USRDIR/CONFIG"
#define PS2_CLASSIC_ISO_PATH		PS2_CLASSIC_LAUCHER_DIR "/USRDIR/ISO.BIN.ENC"

#define PSP_LAUNCHER_MINIS_ID		"PSPM66820"
#define PSP_LAUNCHER_REMASTERS_ID	"PSPC66820"
#define PSP_LAUNCHER_MINIS			_HDD0_GAME_DIR PSP_LAUNCHER_MINIS_ID
#define PSP_LAUNCHER_REMASTERS		_HDD0_GAME_DIR PSP_LAUNCHER_REMASTERS_ID

//////////////////////////////

#define XML_HOST_PATH			"/dev_hdd0/xmlhost"
#define HTML_BASE_PATH			"/dev_hdd0/xmlhost/game_plugin"

#define HEN_HFW_SETTINGS		"/dev_hdd0/hen/hfw_settings.xml"

#define _FB_XML					HTML_BASE_PATH "//fb.xml"
#define FB_XML					HTML_BASE_PATH "/fb.xml"
#define FB_HEN_XML				"/." HTML_BASE_PATH "/fb-hen.xml"

#ifdef COBRA_ONLY
#define MY_GAMES_XML			HTML_BASE_PATH "/mygames.xml"
#else
#define MY_GAMES_XML			HTML_BASE_PATH "/jbgames.xml"
#endif
#define GAMES_HTML				HTML_BASE_PATH "/games.html"
#define MOBILE_HTML				is_ps3_http ? (GAMES_HTML) : (HTML_BASE_PATH "/mobile.html")
#define GAMELIST_JS				HTML_BASE_PATH "/gamelist.js"
#define CPU_RSX_CHART			HTML_BASE_PATH "/cpursx.html"

#define COMMON_CSS				HTML_BASE_PATH "/common.css"
#define COMMON_SCRIPT_JS		HTML_BASE_PATH "/common.js"
#define FM_SCRIPT_JS			HTML_BASE_PATH "/fm.js"
#define FS_SCRIPT_JS			HTML_BASE_PATH "/fs.js"
#define GAMES_SCRIPT_JS			HTML_BASE_PATH "/games.js"

#define JQUERY_LIB_JS			HTML_BASE_PATH "/jquery.min.js"
#define JQUERY_UI_LIB_JS		HTML_BASE_PATH "/jquery-ui.min.js"

#define DELETE_CACHED_GAMES		{cellFsUnlink(WMTMP "/games.html"); cellFsUnlink(GAMELIST_JS);}

///////////////////////////

#define AUTOBOOT_PATH				"/dev_hdd0/PS3ISO/AUTOBOOT.ISO"

#ifdef COBRA_ONLY
 #define DEFAULT_AUTOBOOT_PATH		"/dev_hdd0/PS3ISO/AUTOBOOT.ISO"
#else
 #define DEFAULT_AUTOBOOT_PATH		"/dev_hdd0/GAMES/AUTOBOOT"
#endif

#define MAX_ISO_PARTS				(16)
#define ISO_EXTENSIONS				".cue.ccd.chd.iso.0.bin.img.mdf.enc"
#define ARCHIVE_EXTENSIONS			".zip.rar.bz2.tgz.tar.7z.gz"
#define SKIP_CUE					8

static const char *smonth[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static char drives[17][13] = {"/dev_hdd0", "/dev_usb000", "/dev_usb001", "/dev_usb002", "/dev_usb003", "/dev_usb006", "/dev_usb007", "/net0", "/net1", "/net2", "/net3", "/net4", "/dev_ntfs0:/", "/dev_sd", "/dev_ms", "/dev_cf", "/dev_blind"};
static char paths [13][10] = {"GAMES", "GAMEZ", "PS3ISO", "BDISO", "DVDISO", "PS2ISO", "PSXISO", "PSXGAMES", "PSPISO", "ISO", "video", "GAMEI", "ROMS"};

#ifdef COBRA_ONLY
#define NUM_ISO_EXTS   13
#define NUM_ISO_EXTS_  12
#define NUM_CUE_EXTS    4
static const char *iso_ext[NUM_ISO_EXTS] = {".bin", ".iso", ".iso.0", ".BIN", ".ISO", ".ISO.0", ".img", ".mdf", ".IMG", ".MDF", ".chd", ".CHD", ".bin"};
static const char *cue_ext[NUM_CUE_EXTS]  = {".cue", ".ccd", ".CUE", ".CCD"};
#endif

#ifdef VISUALIZERS
static const char *vsh_res_path[10] = {
										TMP_DIR "/wallpaper", // 0
										TMP_DIR "/earth"    , // 1
										TMP_DIR "/canyon"   , // 2
										TMP_DIR "/lines"    , // 3
										TMP_DIR "/coldboot" , // 4
										TMP_DIR "/theme",     // 5
										TMP_DIR "/theme",     // 6 (last selected theme)
										TMP_DIR "/impose"   , // 7
										TMP_DIR "/psn_icons", // 8
										TMP_DIR "/system_plugin", // 9
									  };

static const char *vsh_res_action[10] = {
											"/wallpaper.ps3", // 0
											"/earth.ps3"    , // 1
											"/canyon.ps3"   , // 2
											"/lines.ps3"    , // 3
											"/coldboot.ps3" , // 4
											"/theme.ps3",     // 5
											"/theme.ps3",     // 6 (last selected theme)
											"/impose.ps3"   , // 7
											"/psn_icons.ps3", // 8
											"/clock.ps3"	  // 9
										};
#endif

#ifdef COPY_PS3
static const char *script_events[4] = {
										"/dev_hdd0/boot_init.txt",
										"/dev_hdd0/autoexec.bat",
										"/dev_hdd0/onxmb.bat",
										"/dev_hdd0/ingame.bat"
									  };
#endif

#define NET				(7)
#define NTFS 			(12)
#define MAX_DRIVES		16

#define LINELEN			512 // file listing
#define MAX_LINE_LEN	640 // html games
#define STD_PATH_LEN	263 // standard path len (260 characters in NTFS - Windows 10 removed this limit in 2016)
#define MAX_PATH_LEN	512 // do not change!
#define MAX_TEXT_LEN	1300 // should not exceed HTML_RECV_SIZE (RECV buffer is unstable above 1400 bytes)
#define D_NAME_LEN		256

#define not_exists(path)	(!file_exists(path))
