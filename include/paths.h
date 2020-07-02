#define VSH_MODULE_DIR		"/dev_flash/vsh/module/"
#define VSH_MODULE_PATH		"/dev_blind/vsh/module/"
#define VSH_ETC_PATH		"/dev_blind/vsh/etc/"
#define PS2_EMU_PATH		"/dev_blind/ps2emu/"
#define REBUG_COBRA_PATH	"/dev_blind/rebug/cobra/"
#define HABIB_COBRA_PATH	"/dev_blind/habib/cobra/"
#define SYS_COBRA_PATH		"/dev_blind/sys/"
#define REBUG_TOOLBOX		"/dev_hdd0//game/RBGTLBOX2/USRDIR/"
#define COLDBOOT_PATH		"/dev_blind/vsh/resource/coldboot.raf"
#define ORG_LIBFS_PATH		"/dev_flash/sys/external/libfs.sprx"
#define NEW_LIBFS_PATH		"/dev_hdd0/tmp/wm_res/libfs.sprx"

#define XMB_DISC_ICON		"/dev_hdd0/tmp/game/ICON0.PNG"
#define CATEGORY_GAME_XML	"/dev_flash/vsh/resource/explore/xmb/category_game.xml"

#define HDD0_GAME_DIR		"/dev_hdd0/game/"
#define _HDD0_GAME_DIR		"/dev_hdd0//game/"

#define MANAGUNZ			_HDD0_GAME_DIR "MANAGUNZ0/USRDIR"	// ManaGunZ folder
#define MM_ROOT_STD			_HDD0_GAME_DIR "BLES80608/USRDIR"	// multiMAN root folder
#define MM_ROOT_SSTL		_HDD0_GAME_DIR "NPEA00374/USRDIR"	// multiman SingStar® Stealth root folder
#define MM_ROOT_STL			"/dev_hdd0/tmp/game_repo/main"		// stealthMAN root folder

#define WM_INSTALLER_PATH	_HDD0_GAME_DIR "UPDWEBMOD/USRDIR"
#define XMBMANPLS_PATH		_HDD0_GAME_DIR "XMBMANPLS/USRDIR"
#define PS2CONFIG_PATH		_HDD0_GAME_DIR "PS2CONFIG/USRDIR"

#define TMP_DIR				"/dev_hdd0/tmp"

#define WMCONFIG			TMP_DIR "/wm_config.bin"		// webMAN config file
#define WMTMP				TMP_DIR "/wmtmp"				// webMAN work/temp folder
#define WM_RES_PATH			TMP_DIR "/wm_res"				// webMAN resources
#define WM_LANG_PATH		TMP_DIR "/wm_lang"				// webMAN language folder
#define WM_ICONS_PATH		TMP_DIR "/wm_icons"				// webMAN icons folder
#define WM_COMBO_PATH		TMP_DIR "/wm_combo"				// webMAN custom combos folder
#define WMNOSCAN			TMP_DIR "/wm_noscan"			// webMAN config file to skip on boot
#define WMREQUEST_FILE		TMP_DIR "/wm_request"			// webMAN request file
#define WMNET_DISABLED		TMP_DIR "/wm_netdisabled"		// webMAN config file to re-enable network

#define WMONLINE_GAMES		WM_RES_PATH "/wm_online_ids.txt"	// webMAN config file to skip disable network setting on these title ids
#define WMOFFLINE_GAMES		WM_RES_PATH "/wm_offline_ids.txt"	// webMAN config file to disable network setting on specific title ids (overrides wm_online_ids.txt)

#define WMIGNORE_FILES		WM_RES_PATH "/wm_ignore.txt"	// webMAN config file to ignore files during content scanning

#define SLAUNCH_FILE		WMTMP "/slist.bin"
#define DEL_CACHED_ISO		WMTMP "/deliso.txt"

#define LAST_GAME_TXT		WMTMP "/last_game.txt"
#define LAST_GAMES_BIN		WMTMP "/last_games.bin"

#define VSH_MENU_IMAGES		"/dev_hdd0/plugins/images"

#define PKGLAUNCH_ID		"PKGLAUNCH"
#define PKGLAUNCH_DIR		_HDD0_GAME_DIR PKGLAUNCH_ID
#define PKGLAUNCH_ICON		PKGLAUNCH_DIR "/ICON0.PNG"

#define RETROARCH_DIR1		_HDD0_GAME_DIR "SSNE10000"
#define RETROARCH_DIR2		_HDD0_GAME_DIR "SSNE10001"

#define RELOADXMB_DIR		_HDD0_GAME_DIR "RELOADXMB"
#define RELOADXMB_EBOOT		RELOADXMB_DIR "/USRDIR/EBOOT.BIN"
#define RELOADXMB_ISO		WM_RES_PATH "/RELOAD_XMB.ISO"

#define VSH_RESOURCE_DIR	"/dev_flash/vsh/resource/"
#define SYSMAP_EMPTY_DIR	VSH_RESOURCE_DIR "AAA"		//redirect firmware update to empty folder (formerly redirected to "/dev_bdvd")

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

#define HTML_BASE_PATH			"/dev_hdd0/xmlhost/game_plugin"

#define HEN_HFW_SETTINGS		"/dev_hdd0/hen/hfw_settings.xml"

#define FB_XML					"/dev_hdd0/xmlhost/game_plugin/fb.xml"
#ifdef COBRA_ONLY
#define MY_GAMES_XML			HTML_BASE_PATH "/mygames.xml"
#else
#define MY_GAMES_XML			HTML_BASE_PATH "/jbgames.xml"
#endif
#define MOBILE_HTML				HTML_BASE_PATH "/mobile.html"
#define GAMELIST_JS				HTML_BASE_PATH "/gamelist.js"
#define CPU_RSX_CHART			HTML_BASE_PATH "/cpursx.html"

#ifndef EMBED_JS
#define COMMON_CSS				HTML_BASE_PATH "/common.css"
#define COMMON_SCRIPT_JS		HTML_BASE_PATH "/common.js"
#define FM_SCRIPT_JS			HTML_BASE_PATH "/fm.js"
#define FS_SCRIPT_JS			HTML_BASE_PATH "/fs.js"
#define GAMES_SCRIPT_JS			HTML_BASE_PATH "/games.js"
#endif

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
#define ISO_EXTENSIONS				".cue|.ccd|.iso.0|.bin|.img|.mdf"

static const char *smonth[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static char drives[17][12] = {"/dev_hdd0", "/dev_usb000", "/dev_usb001", "/dev_usb002", "/dev_usb003", "/dev_usb006", "/dev_usb007", "/net0", "/net1", "/net2", "/net3", "/net4", "/dev_ntfs", "/dev_sd", "/dev_ms", "/dev_cf", "/dev_blind"};
static char paths [13][10] = {"GAMES", "GAMEZ", "PS3ISO", "BDISO", "DVDISO", "PS2ISO", "PSXISO", "PSXGAMES", "PSPISO", "ISO", "video", "GAMEI", "ROMS"};

#define LINELEN			512 // file listing
#define MAX_LINE_LEN	640 // html games
#define STD_PATH_LEN	263 // standard path len (260 characters in NTFS - Windows 10 removed this limit in 2016)
#define MAX_PATH_LEN	512 // do not change!
#define MAX_TEXT_LEN	1300 // should not exceed HTML_RECV_SIZE

static char html_base_path[MAX_PATH_LEN];
