////////////////////////////////
typedef struct
{
	u16 version; // 0x1337

	u8 padding0[12];

	u8 artemis;  // 0=disabled, 1=enabled on game
	u8 wm_proxy; // 0=use wm_proxy, 1=use webbrowser_plugin
	u8 lang;     // 0=EN, 1=FR, 2=IT, 3=ES, 4=DE, 5=NL, 6=PT, 7=RU, 8=HU, 9=PL, 10=GR, 11=HR, 12=BG, 13=IN, 14=TR, 15=AR, 16=CN, 17=KR, 18=JP, 19=ZH, 20=DK, 21=CZ, 22=SK, 99=XX

	// scan devices settings

	u8 usb0;    // 0=none, 1=scan /dev_usb000
	u8 usb1;    // 0=none, 1=scan /dev_usb001
	u8 usb2;    // 0=none, 1=scan /dev_usb002
	u8 usb3;    // 0=none, 1=scan /dev_usb003
	u8 usb6;    // 0=none, 1=scan device (find first port available from /dev_usb006 to /dev_usb128)
	u8 usb7;    // 0=none, 1=scan device (find first port available from /dev_usb007 to /dev_usb128)
	u8 dev_sd;  // 0=none, 1=scan /dev_sd
	u8 dev_ms;  // 0=none, 1=scan /dev_ms
	u8 dev_cf;  // 0=none, 1=scan /dev_cf
	u8 ntfs;    // 0=use prepISO for NTFS, 1=enable internal prepNTFS to scan content

	u8 padding1[5];

	// scan content settings

	u8 refr;  // 1=disable content scan on startup
	u8 foot;  // buffer size during content scanning : 0=896KB,1=320KB,2=1280KB,3=512KB,4 to 7=1280KB
	u8 cmask; // content mask

	u8 nogrp;   // 0=group content on XMB, 1=no group
	u8 nocov;   // 0=show covers, 1=no covers
	u8 nosetup; // 0=show setup, 1=no setup
	u8 rxvid;   // 0=none, 1=show Video sub-folder
	u8 ps2l;    // 0=none, 1=show PS2 Classic Launcher
	u8 pspl;    // 0=none, 1=show PSP Launcher
	u8 tid;     // 0=none, 1=show title ID in the name of the game
	u8 use_filename;  // 0=use title in PARAM.SFO, 1=show filename as name
	u8 launchpad_xml; // 0=none, 1=use launchpad / PhotoGUI
	u8 launchpad_grp; // 0=sort launchpad by type/name, 1=sort launchpad by name (disable grouping)
	u8 gamei;   // 0=none, 1=scan GAMEI folder
	u8 roms;   // 0=none, 1=ROMS group
	u8 noused; // formerly mc_app
	u8 info;   // info level: 0=Path, 1=Path + ID, 2=ID, 3=None
	u8 npdrm;  // 0=none, 1=show NP games in /dev_hdd0/game
	u8 vsh_mc; // allow allocation from vsh memory container (0=none, 1=app, 2=debug, 3=fg, 4=bg)
	u8 ignore; // 0=none, 1=ignore files/title id listed in /dev_hdd0/tmp/wm_res/wm_ignore.txt
	u8 root;   // 0=webMAN folder, 1=root

	u8 padding2[11];

	// start up settings

	u8 wmstart; // 0=show start up message, 1=disable start up message (webMAN Loaded!)
	u8 lastp;   // 0=none, 1=load last-played game on startup
	u8 autob;   // 0=check for AUTOBOOT.ISO, 1=disable check for AUTOBOOT.ISO
	char autoboot_path[256]; // autoboot path (default: /dev_hdd0/PS3ISO/AUTOBOOT.ISO)
	u8 delay;   // 0=none, 1=delay loading of AUTOBOOT.ISO/last-game (Disc Auto-start)
	u8 bootd;   // Wait for any USB device to be ready: 0, 5, 9 or 15 seconds
	u8 boots;   // Wait additionally for each selected USB device to be ready: 0, 3, 5, 10, 15 seconds
	u8 nospoof; // 0=spoof fw version < 4.53, 1=don't spoof fw version (default)
	u8 blind;   // 0=none, 1=enable /dev_blind on startup
	u8 spp;     // 0=none, 1=disable syscalls, offline: 2=lock PSN, 4=offline ingame
	u8 noss;    // 0=allow singstar icon, 1=remove singstar icon
	u8 nosnd0;  // 0=allow SND0.AT3/ICON1.PAM, 1=mute SND0.AT3/ICON1.PAM
	u8 dsc;     // 0=none, 1=disable syscalls if physical disc is inserted
	u8 noBD;    // 0=normal, 1=apply noBD patch
	u8 music;   // 0=none, 1=play music on startup

	u8 padding3[2];

	// fan control settings

	u8 fanc;      // 1 = enabled, 0 = disabled (syscon)
	u8 man_speed; // manual fan speed (calculated using man_rate)
	u8 dyn_temp;  // max temp for dynamic fan control (0 = disabled)
	u8 man_rate;  // % manual fan speed (0 = dynamic fan control)
	u8 ps2_rate;  // % ps2 fan speed
	u8 nowarn;    // 0=show warning, 1=no warning
	u8 minfan;    // minimum fan speed (25%)
	u8 chart;     // 0=none, 1=log to CPU/RSX/FAN Chart
	u8 maxfan;    // maximum fan speed (80%)

	u8 padding4[7];

	// combo settings

	u8  nopad;      // unused
	u8  keep_ccapi; // 0=disable syscalls keep CCAPI, 1=disable syscalls removes CCAPI
	u32 combo;      // combo  flags
	u32 combo2;     // combo2 flags
	u8  sc8mode;    // 0/4=Remove cfw syscall disables syscall8 / PS3MAPI=disabled, 1=Keep syscall8 / PS3MAPI=enabled
	u8  nobeep;     // 0=beep, 1=no beeps

	u8 padding5[20];

	// ftp server settings

	u8  bind;         // 0=allow remote FTP/WWW services, 1=disable remote access to FTP/WWW services
	u8  ftpd;         // 0=allow ftp service, 1=disable ftp service
	u16 ftp_port;     // default: 21
	u8  ftp_timeout;  // 0=20 seconds, 1-255= number of minutes
	char ftp_password[20];
	u8 ftp_password_null;
	char allow_ip[16]; // block all remote IP addresses except this

	u8 padding6[7];

	// net server settings

	u8  netsrvd;  // 0=none, 1=enable local ps3netsrv
	u16 netsrvp;  // local ps3netsrv port

	u8 padding7[13];

	// net client settings

	u8   netd[5];     //0..4
	u16  netp[5];     //port 0..65535
	char neth[5][16]; //ip 255.255.255.255

	u8 nsd;
	u8 padding8[32];

	// mount settings

	u8 bus;       // 0=enable reset USB bus, 1=disable reset USB bus
	u8 fixgame;   // 0=Auto, 1=Quick, 2=Forced, 3=Disable auto-fix game
	u8 ps1emu;    // 0=ps1emu, 1=ps1_netemu
	u8 autoplay;  // 0=none, 1=Auto-Play after mount
	u8 ps2emu;    // 0=ps2emu, 1=ps2_netemu
	u8 ps2config; // 0=enable auto lookup for PS2 CONFIG, 1=disable auto lookup for PS2 CONFIG
	u8 minfo;     // Mount info level: 0=Both messages, 1=After mount, 2=Previous to mount, 3=none
	u8 deliso;    // 0=none, 1=delete cached PS2ISO copied from net/ntfs
	u8 auto_install_pkg; // 0=auto install PKG when a .ntfs[BDFILE] is mounted, 1=no auto install PKG
	u8 app_home;  // 0=mount folders in app_home, 1=do not mount app_home
	u8 ps1rom;    // 0=ps1_rom.bin, 1=ps1_bios.bin

	u8 padding9[5];

	// profile settings

	u8 profile;          // User profile
	char uaccount[9];    // default  user account (not used)
	u8 admin_mode;       // 0=USER MODE, 1=ADMIN MODE / requires !(webman_config->combo & SYS_ADMIN)
	u8 unlock_savedata;  // 0=none, 1=auto unlock savedata on file operations (copy/ftp/download) // deprecated //

	u8 padding10[4];

	// misc settings

	u8 default_restart;  // default restart mode set by /restart.ps3?<mode>$
	u8 poll;             // poll all usb drives every 2 minutes to keep them awake

	u32 rec_video_format;
	u32 rec_audio_format;

	u8 auto_power_off; // 0 = prevent auto power off on ftp, 1 = allow auto power off on ftp (also on install.ps3, download.ps3)

	u8 ps3mon; // 0 = none; 1 = load ps3mon.sprx on startup

	u8 padding12[4];

	u8 homeb; // 0=none, 1=show home button in original GUI
	char home_url[255]; // url for home button, search path for files not found or path for default application in app_home

	u8 sman;     // 0=original GUI, 1=sman GUI
	u8 msg_icon; // 0=VSH notify with icon, 1=vshtask_notify (info)

	u8 padding11[30];

	// spoof console id

	u8 sidps; // 0=none, 1=spoof IDPS
	u8 spsid; // 0=none, 1=spoof PSID
	char vIDPS1[17];
	char vIDPS2[17];
	char vPSID1[17];
	char vPSID2[17];

	u8 padding13[24];

	u8 resource_id[12];
	//u8 wallpaper_id;	// [0]: 0=random, 1-255: select specific #.png in /dev_hdd0/tmp/wallpaper on each boot
	//u8 earth_id;		// [1]: 0=random, 1-255: select specific #.qrc in /dev_hdd0/tmp/earth on each boot
	//u8 canyon_id;		// [2]: 0=random, 1-255: select specific #.qrc in /dev_hdd0/tmp/canyon on each boot
	//u8 lines_id;		// [3]: 0=random, 1-255: select specific #.qrc in /dev_hdd0/tmp/lines on each boot
	//u8 coldboot_id;	// [4]: 0=random, 1-255: select specific #.ac3 in /dev_hdd0/tmp/coldboot on each boot
	//u8 theme_id;		// [5]: 0=random, 1-255: select specific #.p3t in /dev_hdd0/tmp/theme on each boot
	//u8 last_theme_id;	// [6]: 0=none, 1-255: last selected theme (used to prevent install the current theme again)
	//u8 impose_id;		// [7]: 0=random, 1-255: select specific #.rco in /dev_hdd0/tmp/impose on each boot
	//u8 psn_icons_id	// [8]: 0=random, 1-255: select specific #.rco in /dev_hdd0/tmp/psn_icons on each boot
	//u8 clock_id		// [9]: 0=random, 1-255: select specific #.rco in /dev_hdd0/tmp/system_plugin on each boot
						//[10]: reserved
						//[11]: reserved
} /*__attribute__((packed))*/ WebmanCfg;

static u8 wmconfig[sizeof(WebmanCfg)];
static WebmanCfg *webman_config = (WebmanCfg*) wmconfig;

#ifdef COBRA_ONLY
static u8 cconfig[sizeof(CobraConfig)];
static CobraConfig *cobra_config = (CobraConfig*) cconfig;
#endif

////////////////////////////////
