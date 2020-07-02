////////////////////////////////
typedef struct
{
	u16 version;

	u8 padding0[13];

	u8 wm_proxy;
	u8 lang; //0=EN, 1=FR, 2=IT, 3=ES, 4=DE, 5=NL, 6=PT, 7=RU, 8=HU, 9=PL, 10=GR, 11=HR, 12=BG, 13=IN, 14=TR, 15=AR, 16=CN, 17=KR, 18=JP, 19=ZH, 20=DK, 21=CZ, 22=SK, 99=XX

	// scan devices settings

	u8 usb0;
	u8 usb1;
	u8 usb2;
	u8 usb3;
	u8 usb6;
	u8 usb7;
	u8 dev_sd;
	u8 dev_ms;
	u8 dev_cf;
	u8 ntfs; // 1=enable internal prepNTFS to scan content

	u8 padding1[5];

	// scan content settings

	u8 refr; // 1=disable content scan on startup
	u8 foot; // buffer size during content scanning : 0=896KB,1=320KB,2=1280KB,3=512KB,4 to 7=1280KB
	u8 cmask;

	u8 nogrp;
	u8 nocov;
	u8 nosetup;
	u8 rxvid;
	u8 ps2l;
	u8 pspl;
	u8 tid;
	u8 use_filename;
	u8 launchpad_xml;
	u8 launchpad_grp;
	u8 ps3l;
	u8 roms;
	u8 noused; // formerly mc_app
	u8 info;   // info level: 0=Path, 1=Path + ID, 2=ID, 3=None
	u8 npdrm;
	u8 vsh_mc; // allow allocation from vsh memory container

	u8 padding2[13];

	// start up settings

	u8 wmstart; // 1=disable start up message (webMAN Loaded!)
	u8 lastp;
	u8 autob;
	char autoboot_path[256];
	u8 delay;
	u8 bootd;
	u8 boots;
	u8 nospoof;
	u8 blind;
	u8 spp;    //disable syscalls, offline: lock PSN, offline ingame
	u8 noss;   //no singstar
	u8 nosnd0; //mute snd0.at3
	u8 dsc;    //disable syscalls if physical disc is inserted

	u8 padding3[4];

	// fan control settings

	u8 fanc;      // 1 = enabled, 0 = disabled (syscon)
	u8 man_speed; // manual fan speed (calculated using man_rate)
	u8 dyn_temp;  // max temp for dynamic fan control (0 = disabled)
	u8 man_rate;  // % manual fan speed (0 = dynamic fan control)
	u8 ps2_rate;  // % ps2 fan speed
	u8 nowarn;
	u8 minfan;
	u8 chart;
	u8 maxfan;

	u8 padding4[7];

	// combo settings

	u8  nopad;
	u8  keep_ccapi;
	u32 combo;
	u32 combo2;
	u8  sc8mode; // 0/4=Remove cfw syscall disables syscall8 / PS3MAPI=disabled, 1=Keep syscall8 / PS3MAPI=enabled
	u8  nobeep;

	u8 padding5[20];

	// ftp server settings

	u8  bind;
	u8  ftpd;
	u16 ftp_port;
	u8  ftp_timeout;
	char ftp_password[20];
	char allow_ip[16];

	u8 padding6[7];

	// net server settings

	u8  netsrvd;
	u16 netsrvp;

	u8 padding7[13];

	// net client settings

	u8   netd[5];     //0..4
	u16  netp[5];     //port 0..65535
	char neth[5][16]; //ip 255.255.255.255

	u8 padding8[33];

	// mount settings

	u8 bus;
	u8 fixgame;
	u8 ps1emu;
	u8 autoplay;
	u8 ps2emu;
	u8 ps2config;
	u8 minfo;
	u8 deliso;

	u8 padding9[8];

	// profile settings

	u8 profile;
	char uaccount[9];
	u8 admin_mode;

	u8 padding10[5];

	// misc settings

	u8 default_restart;
	u8 poll; // poll usb

	u32 rec_video_format;
	u32 rec_audio_format;

	u8 auto_power_off; // 0 = prevent auto power off on ftp, 1 = allow auto power off on ftp (also on install.ps3, download.ps3)

	u8 padding12[5];

	u8 homeb;
	char home_url[255];

	u8 sman;
	u8 earth_id;
	u8 padding11[30];

	// spoof console id

	u8 sidps;
	u8 spsid;
	char vIDPS1[17];
	char vIDPS2[17];
	char vPSID1[17];
	char vPSID2[17];

	u8 padding13[34];
} /*__attribute__((packed))*/ WebmanCfg;

static u8 wmconfig[sizeof(WebmanCfg)];
static WebmanCfg *webman_config = (WebmanCfg*) wmconfig;

#ifdef COBRA_ONLY
static u8 cconfig[sizeof(CobraConfig)];
static CobraConfig *cobra_config = (CobraConfig*) cconfig;
#endif

static int save_settings(void);
static void restore_settings(void);
////////////////////////////////
