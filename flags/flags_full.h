//#define ENGLISH_ONLY	1	// uncomment for english only version

//// EDITIONS ////

#define COBRA_ONLY		1	// comment out for ccapi/non-cobra release
#define REX_ONLY		1	// shortcuts for REBUG REX CFWs / comment out for usual CFW

#define PS3MAPI			1	// ps3 manager API & webGUI by _NzV_
//#define CCAPI			1	// uncomment for ccapi release
//#define LITE_EDITION	1	// no ps3netsrv support, smaller memory footprint

#define DEX_SUPPORT		1	// add support for DEX
#define DECR_SUPPORT	1	// add support for DECR

//// FEATURES ////
#define FIX_GAME		1	// Auto-Fix game
#define EXT_GDATA		1	// /extgd.ps3?status /extgd.ps3?enable /extgd.ps3?disable (external gameDATA)
#define COPY_PS3		1	// /copy.ps3/<path>
#define DEBUG_MEM		1	// /dump.ps3 / peek.lv1 / poke.lv1 / find.lv1 / peek.lv2 / poke.lv2 / find.lv2
#define VIDEO_REC		1	// /videorec.ps3  start/stop video recording (in-game only)
#define LOAD_PRX		1	// /loadprx.ps3?slot=n&prx=path/file.sprx  (load prx)
#define FAKEISO 		1	// support .ntfs[BDFILE] (fake ISO)
#define BDVD_REGION		1	// change BD/DVD region
#define REMOVE_SYSCALLS	1	// remove syscalls on startup and using R2+TRIANGLE
#define SPOOF_CONSOLEID	1	// spoof idps/psid
#define VIRTUAL_PAD		1	// virtual pad
#define WM_REQUEST		1	// accept commands via local file /dev_hdd0/tmp/wm_request  (required by VSH Menu for webMAN)
#define GET_KLICENSEE	1	// /klic.ps3
#define PS3_BROWSER		1	// /browser.ps3 / open browser combos

//// EXTRA FEATURES ////
#define XMB_SCREENSHOT		1	// screenshot XMB using L2 + R2 + SELECT + START
//#define AUTO_EJECT_DISC	1	// eject disc holding SELECT on mount
#define SWAP_KERNEL			1	// load custom lv2_kernel.self patching LV1 and soft rebooting (use /copy.ps3)
//#define WEB_CHAT			1	// /chat.ps3
#define WM_CUSTOM_COMBO	"/dev_hdd0/tmp/wm_combo/wm_custom_"		// advanced custom combos
#define NET3NET4			1	// add support for /net3 and /net4
#define PS3NET_SERVER		1	// ps3 net server

//// TEST FEATURES ////
//#define PS2_DISC		1	// uncomment to support /mount.ps2 (mount ps2 game folder as /dev_ps2disc)
//#define NOSINGSTAR	1	// remove SingStar icon from XMB
#define SYS_BGM			1	// system background music (may freeze the system when enabled)
//#define USE_DEBUG		1	// debug using telnet
//#define RAWISO_PSX_MULTI	// support for multi PSX from Iris Manager (it's incomplete - only rawseciso.h code is complete)
#define LAUNCHPAD		1	// create /dev_hdd0/tmp/wm_lauchpad.xml

