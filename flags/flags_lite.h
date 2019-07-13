#define ENGLISH_ONLY	1	// uncomment for english only version
//#define USE_INTERNAL_PLUGIN	1	// comment out to use internal rawseciso & netiso clients when raw_iso.sprx & netiso.sprx are unavailable
#define WM_PROXY_SPRX	"idle_plugin"		// plugin used as xmb proxy to mount games (idle_plugin, xai_plugin, wboard_plugin, bdp_disccheck_plugin)

//// EDITIONS ////

#define COBRA_ONLY		1	// comment out for ccapi/non-cobra release
//#define REX_ONLY		1	// shortcuts for REBUG REX CFWs / comment out for usual CFW

//#define PS3MAPI		1	// ps3 manager API & webGUI by _NzV_
#define LITE_EDITION	1	// no ps3netsrv support, smaller memory footprint

#define DEX_SUPPORT		1	// add support for DEX
//#define DECR_SUPPORT	1	// add support for DECR
#define LAST_FIRMWARE_ONLY 1	//  support only 4.82-4.84 CEX / DEX (no DECR support)

//// FEATURES ////
//#define FIX_GAME		1	// Auto-Fix game
#define EXT_GDATA		1	// /extgd.ps3?status /extgd.ps3?enable /extgd.ps3?disable (external gameDATA)
//#define COPY_PS3		1	// batch script commands and /copy.ps3/<path>
//#define DEBUG_MEM		1	// /dump.ps3 / peek.lv1 / poke.lv1 / find.lv1 / peek.lv2 / poke.lv2 / find.lv2
//#define VIDEO_REC		1	// /videorec.ps3  start/stop video recording (in-game only) (thanks to mysis & 3141card)
//#define LOAD_PRX		1	// /loadprx.ps3?slot=n&prx=path/file.sprx  (load prx)
//#define FAKEISO 		1	// support .ntfs[BDFILE] (fake ISO)
//#define BDVD_REGION	1	// change BD/DVD region
#define REMOVE_SYSCALLS	1	// remove syscalls on startup and using R2+TRIANGLE
//#define SPOOF_CONSOLEID	1	// spoof idps/psid (thanks to M@tumot0 & zar)
//#define VIRTUAL_PAD		1	// virtual pad (thanks to OsiriX)
//#define WM_REQUEST	1	// accept commands via local file /dev_hdd0/tmp/wm_request
//#define GET_KLICENSEE	1	// /klic.ps3
//#define PS3_BROWSER	1	// /browser.ps3 / open browser combos
//#define EMBED_JS		1	// use embedded javascript instead of external js files
#define AUTO_POWER_OFF	1	// Disable temporarily the auto power off setting in xRegistry during ftp session, /install.ps3 & /download.ps3 (idea by 444nonymous)
#define OFFLINE_INGAME	1	// allow set auto-disable network in-game (idea by 444nonymous)
#define SYS_ADMIN_MODE	1	// Enable restricted / admin modes using L2+R2+TRIANGLE
#define PKG_LAUNCHER	1	// scan & mount np games extracted to GAMEI / scan ROMS
#define PKG_HANDLER		1	// /download.ps3?url=<url>, /download.ps3?to=<path>&url=<url>, /install.ps3<pkg-path> (thanks to bguerville & mysis)
//#define USE_NTFS		1	// add NTFS support

//// EXTRA FEATURES ////
//#define XMB_SCREENSHOT	1	// screenshot XMB using L2 + R2 + SELECT + START (thanks to 3141card)
//#define SWAP_KERNEL		1	// load custom lv2_kernel.self patching LV1 and soft rebooting (use /copy.ps3) [requires QA flag enabled]
//#define WM_CUSTOM_COMBO	"/dev_hdd0/tmp/wm_combo/wm_custom_"		// advanced custom combos
//#define NET3NET4			1	// add support for /net3 and /net4
//#define PS3NET_SERVER		1	// ps3 net server
//#define PS2_DISC			1	// uncomment to support /mount.ps2 (mount ps2 game folder as /dev_ps2disc) requires a physical PS2 disc to swap discs (thanks to WuEpE)
//#define NOSINGSTAR		1	// remove SingStar icon from XMB  (thanks to bguerville)
//#define CALC_MD5			1	// /md5.ps3/<file>
//#define LAUNCHPAD			1	// create /dev_hdd0/tmp/wm_lauchpad.xml

//// TEST FEATURES ////
//#define SYS_BGM			1	// system background music (may freeze the system when enabled)
//#define USE_DEBUG			1	// debug using telnet
//#define WEB_CHAT			1	// /chat.ps3
//#define AUTO_EJECT_DISC	1	// eject disc holding SELECT on mount
//#define RAWISO_PSX_MULTI		// support for multi PSX from Iris Manager (it's incomplete - only rawseciso.h code is complete) (thanks to Estwald)
//#define DO_WM_REQUEST_POLLING	// poll file /dev_hdd0/tmp/wm_request (can be used by external apps)
//#define SECURE_FILE_ID	// hook savedata plugin to capture secure file id (this feature is currently broken)
