//combo
#define FAIL_SAFE (1<<0)
#define SHOW_TEMP (1<<1)
#define PREV_GAME (1<<2)
#define NEXT_GAME (1<<3)
#define SHUT_DOWN (1<<4)
#define RESTARTPS (1<<5)
#define UNLOAD_WM (1<<6)
#define MANUALFAN (1<<7)
#define SHOW_IDPS (1<<8)
#define DISABLESH (1<<9)
#define DISABLEFC (1<<10)
#define MINDYNFAN (1<<11)
#define DISACOBRA (1<<12)
#define SYS_ADMIN (1<<13)
#define GOTO_HOME (1<<14)
#define C_FPSINFO (1<<15)

//combo2
#define EXTGAMDAT (1<<0)
#define MOUNTNET0 (1<<1)
#define MOUNTNET1 (1<<2)
#define PS2TOGGLE (1<<3)
#define PS2SWITCH (1<<4)
#define CUSTOMCMB (1<<5)
#define XMLREFRSH (1<<6)
#define UMNT_GAME (1<<7)
#define VIDRECORD (1<<8)
#define PLAY_DISC (1<<9)
#define INSTALPKG (1<<10)

#define C_VSHMENU (1<<11)
#define C_SLAUNCH (1<<12)

#define REBUGMODE (1<<13)
#define NORMAMODE (1<<14)
#define DEBUGMENU (1<<15)

static void setup_parse_settings(char *param)
{
	if(!strstr(param, "&ic=")) return;

	_memset(webman_config, sizeof(WebmanCfg) - 12); // reset wm_config except resource_id[10]

	get_param("&autop=", webman_config->autoboot_path, param, 255);

	if((webman_config->autoboot_path[0] != '/') && !islike(webman_config->autoboot_path, "http")) sprintf(webman_config->autoboot_path, "%s", DEFAULT_AUTOBOOT_PATH);

	webman_config->usb0 = IS_MARKED("u0=1");
	webman_config->usb1 = IS_MARKED("u1=1");
	webman_config->usb2 = IS_MARKED("u2=1");
	webman_config->usb3 = IS_MARKED("u3=1");
	webman_config->usb6 = IS_MARKED("u6=1");
	webman_config->usb7 = IS_MARKED("u7=1");

	webman_config->dev_sd = IS_MARKED("x0=1");
	webman_config->dev_ms = IS_MARKED("x1=1");
	webman_config->dev_cf = IS_MARKED("x2=1");
	webman_config->npdrm  = IS_MARKED("np=1");

#ifdef USE_NTFS
	webman_config->ntfs = IS_MARKED("xn=1");
#endif

	webman_config->lastp = IS_MARKED("lp=1");
	webman_config->autob = IS_MARKED("ab=1");
	webman_config->delay = IS_MARKED("dy=1");

#ifdef COBRA_ONLY
	webman_config->nosnd0 = IS_MARKED("sn=1");
#endif

	webman_config->nobeep = IS_UNMARKED("nb=1");
	webman_config->wm_proxy = IS_UNMARKED("wp=1");
	webman_config->msg_icon = IS_UNMARKED("mn=1");
#ifdef PLAY_MUSIC
	webman_config->music = get_valuen(param, "&ms=", 0, 2);
#endif
#ifdef PKG_HANDLER
	webman_config->auto_install_pkg = IS_UNMARKED("ai=1");
#endif
#ifdef UNLOCK_SAVEDATA
	webman_config->unlock_savedata = IS_MARKED("up=1");
#endif
#ifdef ARTEMIS_PRX
	webman_config->artemis = IS_MARKED("ar=1");
#endif
#ifdef PS3MON_SPRX
	webman_config->ps3mon = IS_MARKED("pm=1");
	if(webman_config->ps3mon)
		load_vsh_plugin(PS3MON_SPRX);
	else
		unload_vsh_plugin(PS3MON_SPRX);
#endif

	//Wait for any USB device to be ready
	webman_config->bootd = get_valuen(param, "&b=", 0, 30);

	//Wait additionally for each selected USB device to be ready
	webman_config->boots = get_valuen(param, "&s=", 0, 30);

	webman_config->blind = IS_MARKED("bl=1");
	if(webman_config->blind)
		enable_dev_blind(NO_MSG);
	else
		disable_dev_blind();

#ifdef NOBD_PATCH
	webman_config->noBD = IS_MARKED("bd=1"); apply_noBD_patches(webman_config->noBD, false);
#endif

	webman_config->root    = IS_UNMARKED("rt=1");
	webman_config->nosetup = IS_MARKED("ns=1");
	webman_config->nogrp   = IS_MARKED("ng=1");
	webman_config->sman    = IS_MARKED("sm=1");

#ifdef NOSINGSTAR
	webman_config->noss = IS_MARKED("nss=1");
	no_singstar_icon();
#endif
#ifndef LITE_EDITION
	webman_config->chart = IS_MARKED("ct=1");
#endif
#ifdef COBRA_ONLY
	webman_config->cmask = 0;
	if(IS_UNMARKED("ps1=1")) webman_config->cmask|=PS1;
	if(IS_UNMARKED("psp=1")) webman_config->cmask|=PSP;
	if(IS_UNMARKED("blu=1")) webman_config->cmask|=BLU;
	if(IS_UNMARKED("dvd=1")) webman_config->cmask|=DVD;
#else
	webman_config->cmask=(PSP | PS1 | BLU | DVD);
#endif
	if(IS_UNMARKED("ps3=1")) webman_config->cmask|=PS3;
	if(IS_UNMARKED("ps2=1")) webman_config->cmask|=PS2;

	webman_config->pspl   = IS_MARKED("psl=1");
	webman_config->ps2l   = IS_MARKED("p2l=1");
	webman_config->rxvid  = IS_MARKED("rxv=1");
	webman_config->ps1emu = IS_MARKED("pse=1");
	webman_config->ps2emu = IS_MARKED("b2n=1");

	webman_config->ps1rom = IS_MARKED("psr=1"); // 0=ps1_rom.bin, 1=ps1_bios.bin

	webman_config->app_home = IS_UNMARKED("ap=1"); // Mount JB GAMES as /app_home
#ifdef MOUNT_GAMEI
	webman_config->gamei = IS_MARKED("gmi=1");
#endif
#ifdef MOUNT_ROMS
	webman_config->roms  = IS_MARKED("rom=1");
#endif

	webman_config->ignore = IS_MARKED("igf=1"); // ignore game in content scanning

	webman_config->combo = webman_config->combo2 = 0;

#ifdef SYS_ADMIN_MODE
	if(IS_MARKED("adm=1")) {webman_config->combo|=SYS_ADMIN, sys_admin = 0;} else sys_admin = 1;
#endif

	if(IS_UNMARKED("pfs=1")) webman_config->combo|=FAIL_SAFE;
	if(IS_UNMARKED("pss=1")) webman_config->combo|=SHOW_TEMP;
	if(IS_UNMARKED("ppv=1")) webman_config->combo|=PREV_GAME;
	if(IS_UNMARKED("pnx=1")) webman_config->combo|=NEXT_GAME;
	if(IS_UNMARKED("psd=1")) webman_config->combo|=SHUT_DOWN;
	if(IS_UNMARKED("pid=1")) webman_config->combo|=SHOW_IDPS;
	if(IS_UNMARKED("prs=1")) webman_config->combo|=RESTARTPS;
	if(IS_UNMARKED("puw=1")) webman_config->combo|=UNLOAD_WM;
	if(IS_UNMARKED("pf1=1")) webman_config->combo|=MANUALFAN;
	if(IS_UNMARKED("pf2=1")) webman_config->combo|=MINDYNFAN;
	if(IS_UNMARKED("pdf=1")) webman_config->combo|=DISABLEFC;
	if(IS_UNMARKED("psc=1")) webman_config->combo|=DISABLESH;
	if(IS_UNMARKED("hom=1")) webman_config->combo|=GOTO_HOME;
#ifdef WM_REQUEST
	if(IS_UNMARKED("fps=1")) webman_config->combo|=C_FPSINFO;
#endif
	if(IS_UNMARKED("kcc=1")) webman_config->keep_ccapi = true;

#ifdef COBRA_NON_LITE
	if(IS_UNMARKED("pdc=1")) webman_config->combo|=DISACOBRA;
	if(IS_UNMARKED("cf2=1")) webman_config->ps2config = true;
#endif

#ifdef COBRA_ONLY
	webman_config->sc8mode = IS_MARKED("sc8=1") ? PS3MAPI_ENABLED : PS3MAPI_DISABLED;

	webman_config->bus = IS_MARKED("bus=1");
	webman_config->deliso = IS_MARKED("dx=1"); if(!webman_config->deliso) cellFsUnlink(DEL_CACHED_ISO);
#endif
	webman_config->autoplay = IS_MARKED("apd=1");
#ifdef REX_ONLY
	if(IS_UNMARKED("pr0=1")) webman_config->combo2|=REBUGMODE;
	if(IS_UNMARKED("pr1=1")) webman_config->combo2|=NORMAMODE;
	if(IS_UNMARKED("pr2=1")) webman_config->combo2|=DEBUGMENU;

	if(IS_UNMARKED("p2c=1")) webman_config->combo2|=PS2TOGGLE;
#endif

#ifdef PKG_HANDLER
	if(IS_UNMARKED("pkg=1")) webman_config->combo2|=INSTALPKG;
#endif
	if(IS_UNMARKED("pgd=1")) webman_config->combo2|=EXTGAMDAT;
#ifndef LITE_EDITION
	if(IS_UNMARKED("p2s=1")) webman_config->combo2|=PS2SWITCH;
#endif
#ifdef NET_SUPPORT
	if(IS_UNMARKED("pn0=1")) webman_config->combo2|=MOUNTNET0;
	if(IS_UNMARKED("pn1=1")) webman_config->combo2|=MOUNTNET1;
#endif
	if(IS_UNMARKED("psv=1")) webman_config->combo2|=CUSTOMCMB;
	if(IS_UNMARKED("pxr=1")) webman_config->combo2|=XMLREFRSH;
	if(IS_UNMARKED("umt=1")) webman_config->combo2|=UMNT_GAME;
	if(IS_UNMARKED("pld=1")) webman_config->combo2|=PLAY_DISC;

#ifdef COBRA_ONLY
	if(IS_UNMARKED("vs=1")) webman_config->combo2|=C_VSHMENU;
	if(IS_UNMARKED("gm=1")) webman_config->combo2|=C_SLAUNCH;
#endif

#ifdef VIDEO_REC
	if(IS_UNMARKED("vrc=1")) webman_config->combo2|=VIDRECORD;
#endif

	webman_config->info  = get_valuen(param, "&xi=", 0, 0x33); // XMB info level
	webman_config->minfo = get_valuen(param, "&mi=", 0, 3);    // Mount info level

	webman_config->wmstart = IS_MARKED("wn=1");
	webman_config->tid     = IS_MARKED("tid=1");
	webman_config->poll    = IS_MARKED("pl=1" );
	webman_config->use_filename = IS_UNMARKED("sfo=1"); // show filename instead of title in PARAM.SFO
#ifdef AUTO_POWER_OFF
	if(IS_UNMARKED("pw=1" )){setAutoPowerOff(false); AutoPowerOffGame = AutoPowerOffVideo = -1; webman_config->auto_power_off = 1;}
#endif
	webman_config->ftpd = IS_MARKED("ft=1" );
//	webman_config->nopad = IS_MARKED("xp=1");

//	if( IS_MARKED("ic=0" )) webman_config->nocov = SHOW_MMCOVERS;	 // default
	if( IS_MARKED("nc=1" )) webman_config->nocov = SHOW_ICON0;	else // (0 = Use MM covers, 1 = Use ICON0.PNG, 2 = No game icons, 3 = Online Covers)
	if( IS_MARKED("ic=1" )) webman_config->nocov = SHOW_ICON0;	else
	if( IS_MARKED("ic=2" )) webman_config->nocov = SHOW_DISC;
#ifndef ENGLISH_ONLY
	else
	if( IS_MARKED("ic=3" )) webman_config->nocov = ONLINE_COVERS;
#endif

	webman_config->ftp_port = get_port(param, "ff=", 21);
	webman_config->ftp_timeout = get_valuen(param, "tm=", 0, 255); //mins

#ifdef PS3NET_SERVER
	webman_config->netsrvd = IS_MARKED("nd=1");
	webman_config->netsrvp = get_port(param, "netp=", NETPORT);
#endif

#ifdef FIX_GAME
	webman_config->fixgame = get_valuen(param, "fm=", 0, 2);
	if(IS_MARKED("nf=1")) webman_config->fixgame = FIX_GAME_DISABLED;
#endif

#ifdef COBRA_ONLY
	webman_config->nospoof = IS_MARKED("nsp=1"); //don't spoof fw version
	if(c_firmware >= 4.53f) webman_config->nospoof = 1;
#endif

	if(IS_MARKED("fc=1") && IS_UNMARKED("temp=2")) webman_config->fanc = ENABLED;

	webman_config->dyn_temp = MY_TEMP;

	webman_config->minfan = get_valuen(param, "mfan=", MIN_FANSPEED, 95); //%
	webman_config->maxfan = get_valuen(param, "mfs=",  40, 95); //%
	if(webman_config->minfan > webman_config->maxfan) webman_config->maxfan = webman_config->minfan;

	webman_config->bind = IS_MARKED("bn=1");

	get_param("pwd=", webman_config->ftp_password, param, 20);

	webman_config->refr = IS_MARKED("rf=1");

#ifdef LAUNCHPAD
	webman_config->launchpad_xml = IS_UNMARKED("lx=1");
	webman_config->launchpad_grp = IS_MARKED("lg=1");
#endif

	webman_config->man_speed = 0;

	webman_config->dyn_temp = get_valuen(param, "step=", 40, MAX_TEMPERATURE); //°C
	webman_config->ps2_rate = get_valuen(param, "fsp0=", MIN_FANSPEED, webman_config->maxfan); // %
	webman_config->man_rate = get_valuen(param, "manu=", MIN_FANSPEED, webman_config->maxfan); // %

	if(IS_MARKED("&temp=1"))
		webman_config->man_speed = (u8)(((float)(webman_config->man_rate + 1) * 255.f)/100.f); // manual fan speed
	else
		webman_config->man_speed = FAN_AUTO; // dynamic fan control mode

	if(IS_MARKED("&temp=3"))
		webman_config->fanc = FAN_AUTO2;

	original_fanc = (webman_config->fanc == FAN_AUTO2) ? FAN_AUTO2 : ENABLED;

	max_temp = 0;
	if(webman_config->fanc)
	{
		if(webman_config->man_speed == FAN_AUTO)
			max_temp = webman_config->dyn_temp; // dynamic fan max temperature in °C
		else
			set_fan_speed(webman_config->man_speed);
	}
	else
		restore_fan(SYSCON_MODE); //restore syscon fan control mode

	webman_config->nowarn = IS_MARKED("warn=1");

	webman_config->foot=get_valuen(param, "fp=", 0, 8); set_buffer_sizes(webman_config->foot);
	webman_config->vsh_mc = get_valuen(param, "mc=", 0, 4);

#ifdef REMOVE_SYSCALLS
	webman_config->dsc = IS_MARKED("dsc=1");
#endif

	webman_config->spp = 0;
#ifdef COBRA_ONLY
	#ifdef REMOVE_SYSCALLS
	if(IS_MARKED("spp=1"))  webman_config->spp|=1;  //remove syscalls & history
	#endif
	if(IS_MARKED("shh=1"))  webman_config->spp|=2;  //remove history & block psn servers (offline mode)
	#ifdef OFFLINE_INGAME
	if(IS_MARKED("shh=2"))  webman_config->spp|=4;  //offline mode in game
	#endif
#endif
#ifdef SPOOF_CONSOLEID
	if(IS_MARKED("id1=1"))  webman_config->sidps = 1; //spoof IDPS
	if(IS_MARKED("id2=1"))  webman_config->spsid = 1; //spoof PSID

	get_param("vID1=", webman_config->vIDPS1, param, 16);
	get_param("vID2=", webman_config->vIDPS2, param, 16);

	get_param("vPS1=", webman_config->vPSID1, param, 16);
	get_param("vPS2=", webman_config->vPSID2, param, 16);

	spoof_idps_psid();
#endif

#ifdef VIDEO_REC
	char value[8];

	// set video format
	if(get_param("vif=", value, param, 4))
	{
		rec_video_format = webman_config->rec_video_format = convertH(value);
	}
	// set audio format
	if(get_param("auf=", value, param, 4))
	{
		rec_audio_format = webman_config->rec_audio_format = convertH(value);
	}
#endif

	profile = webman_config->profile = get_valuen(param, "&usr=", 0, 5);
	webman_config->lang = 0; //English

#ifndef ENGLISH_ONLY
	webman_config->lang = get_valuen(param, "&l=", 0, LANG_CUSTOM);
	if(webman_config->lang > 22) webman_config->lang = LANG_CUSTOM; // Custom LANG_XX.TXT

	update_language();
#endif

#ifdef COBRA_ONLY
 #ifdef NET_SUPPORT
	char field[8];
	for(u8 id = 0; id < netsrvs; id++)
	{
		webman_config->neth[id][0] = NULL, webman_config->netp[id] = NETPORT;

		sprintf(field, "nd%i=", id);
		if(IS_MARKED(field))  webman_config->netd[id] = 1;

		sprintf(field, "neth%i=", id);
		if(get_param(field, webman_config->neth[id], param, 16))
		{
			sprintf(field, "netp%i=", id);
			webman_config->netp[id] = get_port(param, field, NETPORT);
		}
	}

	get_param("aip=", webman_config->allow_ip, param, 16);
	webman_config->nsd = IS_UNMARKED("nsd=1");
 #endif
#endif

#ifndef LITE_EDITION
	#ifdef USE_UACCOUNT
	get_param("uacc=", webman_config->uaccount, param, 8);
	#endif

	if(IS_MARKED("hm=")) webman_config->homeb = 1;

	get_param("hurl=", webman_config->home_url, param, 255);
#endif

#ifdef BDVD_REGION
	cobra_read_config(cobra_config);

	cobra_config->bd_video_region  = get_valuen(param, "bdr=", 0, 4);  //BD Region
	cobra_config->dvd_video_region = get_valuen(param, "dvr=", 0, 32); //DVD Region

	if(webman_config->fanc)
		cobra_config->fan_speed = (webman_config->man_speed < MIN_FANSPEED_8BIT) ? 1 : webman_config->man_speed;
	else
		cobra_config->fan_speed = 0; // SYSCON

	cobra_write_config(cobra_config);
#endif

#if defined(WM_CUSTOM_COMBO) || defined(WM_REQUEST)
	char command[256];

	size_t cmdlen = get_param("ccbo=", command, param, 255);

 #ifdef WM_CUSTOM_COMBO
	if(save_file(WM_CUSTOM_COMBO "r2_square", command, cmdlen) != CELL_FS_SUCCEEDED)
 #endif
	save_file(WM_COMBO_PATH, command, cmdlen);
#endif
}

static void setup_form(char *buffer, char *templn)
{
 #ifndef ENGLISH_ONLY
//	char STR_SCAN1[48];//		= "Scan these devices";
	char STR_PSPL[40];//		= "Show PSP Launcher";
	char STR_PS2L[48];//		= "Show PS2 Classic Launcher";
	char STR_RXVID[64];//		= "Show Video sub-folder";
	char STR_LPG[128];//		= "Load last-played game on startup";
	char STR_AUTOB[96];//		= "Check for /dev_hdd0/PS3ISO/AUTOBOOT.ISO on startup";
	char STR_DELAYAB[168];//	= "Delay loading of AUTOBOOT.ISO/last-game (Disc Auto-start)";
	char STR_DEVBL[112];//		= "Enable /dev_blind (writable /dev_flash) on startup";
	char STR_CONTSCAN[120];//	= "Disable content scan on startup";
	char STR_USBPOLL[88];//		= "Disable USB polling";
	char STR_FTPSVC[64];//		= "Disable FTP service";
	char STR_FIXGAME[56];//		= "Disable auto-fix game";
	char STR_COMBOS[88];//		= "Disable all PAD shortcuts";
	char STR_MMCOVERS[72];//	= "Disable multiMAN covers";
	char STR_ACCESS[88];//		= "Disable remote access to FTP/WWW services";
	char STR_NOSETUP[120];//	= "Disable webMAN Setup entry in \"webMAN Games\"";
	char STR_NOSPOOF[96];//		= "Disable firmware version spoofing";
	char STR_NOGRP[104];//		= "Disable grouping of content in \"webMAN Games\"";
	char STR_NOWMDN[112];//		= "Disable startup notification of webMAN on the XMB";
	#ifdef NOSINGSTAR
	static char STR_NOSINGSTAR[48];//	= "Remove SingStar icon";
	#endif
	char STR_AUTO_PLAY[24];//	= "Auto-Play";
	char STR_RESET_USB[48];//	= "Disable Reset USB Bus";
	char STR_TITLEID[128];//	= "Include the ID as part of the title of the game";
	char STR_FANCTRL[96];//		= "Enable dynamic fan control";
	char STR_NOWARN[96];//		= "Disable temperature warnings";
	char STR_AUTOAT[32];//		= "Auto at";
	char STR_LOWEST[24];//		= "Lowest";
	char STR_FANSPEED[48];//	= "fan speed";

	char STR_PS2EMU[32];//		= "PS2 Emulator";
	char STR_LANGAMES[96];//	= "Scan for LAN games/videos";
	char STR_ANYUSB[88];//		= "Wait for any USB device to be ready";
	char STR_ADDUSB[136];//		= "Wait additionally for each selected USB device to be ready";
	char STR_DELCFWSYS[144];//	= "Disable CFW syscalls and delete history files at system startup";
	char STR_MEMUSAGE[80];//	= "Plugin memory usage";
	char STR_PLANG[40];//		= "Plugin language";
	char STR_PROFILE[16];//		= "Profile";
	char STR_DEFAULT[32];//		= "Default";
	char STR_COMBOS2[80];//		= "XMB/In-Game PAD SHORTCUTS";
	char STR_FAILSAFE[40];//	= "FAIL SAFE";
	char STR_SHOWTEMP[56];//	= "SHOW TEMP";
	char STR_SHOWIDPS[24];//	= "SHOW IDPS";
	char STR_PREVGAME[64];//	= "PREV GAME";
	char STR_NEXTGAME[56];//	= "NEXT GAME";
	char STR_SHUTDOWN2[32];//	= "SHUTDOWN ";
	char STR_RESTART2[32];//	= "RESTART&nbsp; ";
	#ifdef REMOVE_SYSCALLS
	char STR_DELCFWSYS2[48];//	= "DEL CFW SYSCALLS";
	#endif
	char STR_UNLOADWM[64];//	= "UNLOAD WM";
	char STR_FANCTRL2[48];//	= "CTRL FAN";
	char STR_FANCTRL4[72];//	= "CTRL DYN FAN";
	char STR_FANCTRL5[88];//	= "CTRL MIN FAN";

//	language("STR_SCAN1",     STR_SCAN1,     "Scan these devices");
	language("STR_PSPL",      STR_PSPL,      "Show PSP Launcher");
	language("STR_PS2L",      STR_PS2L,      "Show PS2 Classic Launcher");
	language("STR_RXVID",     STR_RXVID,     "Show Video sub-folder");
	language("STR_LPG",       STR_LPG,       "Load last-played game on startup");
	language("STR_AUTOB",     STR_AUTOB,     "Check for /dev_hdd0/PS3ISO/AUTOBOOT.ISO on startup");
	language("STR_DELAYAB",   STR_DELAYAB,   "Delay loading of AUTOBOOT.ISO/last-game (Disc Auto-start)");
	language("STR_DEVBL",     STR_DEVBL,     "Enable /dev_blind (writable /dev_flash) on startup");
	language("STR_CONTSCAN",  STR_CONTSCAN,  "Disable content scan on startup");
	language("STR_USBPOLL",   STR_USBPOLL,   "Disable USB polling");
	language("STR_FTPSVC",    STR_FTPSVC,    "Disable FTP service");
	language("STR_FIXGAME",   STR_FIXGAME,   "Disable auto-fix game");
	language("STR_COMBOS",    STR_COMBOS,    "Disable all PAD shortcuts");
	language("STR_MMCOVERS",  STR_MMCOVERS,  "Disable multiMAN covers");
	language("STR_ACCESS",    STR_ACCESS,    "Disable remote access to FTP/WWW services");
	language("STR_NOSETUP",   STR_NOSETUP,   "Disable " WM_APPNAME " Setup entry in \"" WM_APPNAME " Games\"");
	language("STR_NOSPOOF",   STR_NOSPOOF,   "Disable firmware version spoofing");
	language("STR_NOGRP",     STR_NOGRP,     "Disable grouping of content in \"" WM_APPNAME " Games\"");
	language("STR_NOWMDN",    STR_NOWMDN,    "Disable startup notification of " WM_APPNAME " on the XMB");
#ifdef NOSINGSTAR
	language("STR_NOSINGSTAR",STR_NOSINGSTAR, "Remove SingStar icon");
#endif
	language("STR_AUTO_PLAY", STR_AUTO_PLAY, "Auto-Play");
	language("STR_RESET_USB", STR_RESET_USB, "Disable Reset USB Bus");
	language("STR_TITLEID",   STR_TITLEID,   "Include the ID as part of the title of the game");
	language("STR_FANCTRL",   STR_FANCTRL,   "Enable dynamic fan control");
	language("STR_NOWARN",    STR_NOWARN,    "Disable temperature warnings");
	language("STR_AUTOAT",    STR_AUTOAT,    "Auto at");
	language("STR_LOWEST",    STR_LOWEST,    "Lowest");
	language("STR_FANSPEED",  STR_FANSPEED,  "fan speed");

	language("STR_PS2EMU",    STR_PS2EMU,    "PS2 Emulator");
	language("STR_LANGAMES",  STR_LANGAMES,  "Scan for LAN games/videos");
	language("STR_ANYUSB",    STR_ANYUSB,    "Wait for any USB device to be ready");
	language("STR_ADDUSB",    STR_ADDUSB,    "Wait additionally for each selected USB device to be ready");
	language("STR_DELCFWSYS", STR_DELCFWSYS, "Disable CFW syscalls and delete history files at system startup");
	language("STR_MEMUSAGE",  STR_MEMUSAGE,  "Plugin memory usage");
	language("STR_PLANG",     STR_PLANG,     "Plugin language");
	language("STR_PROFILE",   STR_PROFILE,   "Profile");
	language("STR_DEFAULT",   STR_DEFAULT,   "Default");
	language("STR_COMBOS2",   STR_COMBOS2,   "XMB/In-Game PAD SHORTCUTS");
	language("STR_FAILSAFE",  STR_FAILSAFE,  "FAIL SAFE");
	language("STR_SHOWTEMP",  STR_SHOWTEMP,  "SHOW TEMP");
	language("STR_SHOWIDPS",  STR_SHOWIDPS,  "SHOW IDPS");
	language("STR_PREVGAME",  STR_PREVGAME,  "PREV GAME");
	language("STR_NEXTGAME",  STR_NEXTGAME,  "NEXT GAME");
	language("STR_SHUTDOWN2", STR_SHUTDOWN2, "SHUTDOWN ");
	language("STR_RESTART2",  STR_RESTART2,  "RESTART&nbsp; ");
	#ifdef REMOVE_SYSCALLS
	language("STR_DELCFWSYS2",STR_DELCFWSYS2, "DEL CFW SYSCALLS");
	#endif

	language("STR_UNLOADWM", STR_UNLOADWM, "UNLOAD WM");
	language("STR_FANCTRL2", STR_FANCTRL2, "CTRL FAN");
	language("STR_FANCTRL4", STR_FANCTRL4, "CTRL DYN FAN");
	language("STR_FANCTRL5", STR_FANCTRL5, "CTRL MIN FAN");

	close_language();
 #endif
	fast_concat.str = NULL;

	syscalls_removed = CFW_SYSCALLS_REMOVED(TOC);
	if(syscalls_removed)
	{
		sprintf(templn, "<h1>%s</h1>%s", STR_CFWSYSALRD, HTML_RED_SEPARATOR); concat(buffer, templn);
	}
	#ifdef COBRA_ONLY
	else if(!cobra_version)
	{
		sprintf(templn, "<h1>%s %s</h1>%s", "Cobra", STR_DISABLED, HTML_RED_SEPARATOR); concat(buffer, templn);
	}
	#endif

	u8 value, b, h = is_app_home_onxmb();
	add_html('0', 0, buffer, templn);
	sprintf(templn, " %s </a></b><br><div id=\"cnt\">"
					"<table width=\"820\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\">"
					"<tr><td width=\"250\">", STR_SCAN2); concat(buffer, templn);

	//Scan these devices
	if(!isDir("/dev_hdd0/GAMEZ") && h)
		add_checkbox_line("np", "/dev_hdd0/game", (webman_config->npdrm), buffer);
	add_checkbox_line("u0", drives[1], (webman_config->usb0), buffer);
	add_checkbox_line("u1", drives[2], (webman_config->usb1), buffer);
	add_checkbox_line("u2", drives[3], (webman_config->usb2), buffer);
	add_checkbox_line("u3", drives[4], (webman_config->usb3), buffer);
	add_checkbox_line("u6", drives[5], (webman_config->usb6), buffer);
	add_checkbox_line("u7", drives[6], (webman_config->usb7), buffer);

	if(isDir(drives[13])) add_checkbox_line("x0", drives[13], (webman_config->dev_sd), buffer);
	if(isDir(drives[14])) add_checkbox_line("x1", drives[14], (webman_config->dev_ms), buffer);
	if(isDir(drives[15])) add_checkbox_line("x2", drives[15], (webman_config->dev_cf), buffer);

#ifdef USE_NTFS
	concat(buffer, "<label title=\"internal prepNTFS\">");
	add_checkbox_line("xn", "/dev_ntfs", (webman_config->ntfs), buffer);
#endif

	//Scan for content
	concat(buffer, "<td nowrap valign=top>");

	add_checkbox("ps3", "PLAYSTATION\xC2\xAE\x33"," (", !(webman_config->cmask & PS3), buffer);
	add_checkbox("ap", "/app_home",  h ? "," : ")<br>" , !(webman_config->app_home), buffer);
	if(h)
	{
		add_checkbox("gmi", "GAMEI", ")<br>",  (webman_config->gamei), buffer);
	}

	b = isDir(PS2_CLASSIC_PLACEHOLDER);
	add_checkbox("ps2", "PLAYSTATION\xC2\xAE\x32", " (" ,   !(webman_config->cmask & PS2), buffer);
	if(b) add_checkbox("p2l", STR_PS2L           , ", " ,    (webman_config->ps2l)       , buffer);
#ifdef SPOOF_CONSOLEID
	b = ((eid0_idps[0] & 0x00000000000000FF) <= 0x04); // 0x01 = CECH-A*, 0x02 = CECH-B, 0x03 = CECH-C, 0x04 = CECH-E
	if(b) add_checkbox("b2n", "ps2_netemu"       , ", " ,    (webman_config->ps2emu)     , buffer);
#endif
	add_checkbox("cf2", "Auto CONFIG"            , ")<br>", !(webman_config->ps2config)  , buffer);

#ifdef COBRA_ONLY
	add_checkbox("ps1", "PLAYSTATION\xC2\xAE&nbsp;"  ,     " ("       , !(webman_config->cmask & PS1), buffer);
	add_checkbox("pse", "ps1_netemu"                 ,     ", "       ,  (webman_config->ps1emu)     , buffer);
	add_checkbox("psr", "ps1_bios.bin"               ,     ")<br>"    ,  (webman_config->ps1rom)     , buffer);

	b = (isDir(PSP_LAUNCHER_MINIS) || isDir(PSP_LAUNCHER_REMASTERS));
	add_checkbox("psp", "PLAYSTATION\xC2\xAEPORTABLE", b ? " (" : _BR_, !(webman_config->cmask & PSP), buffer);
	if(b) add_checkbox("psl", STR_PSPL               ,     ")<br>"    ,  (webman_config->pspl)       , buffer);

	add_checkbox("blu", "Blu-ray\xE2\x84\xA2"        ,     " ("       , !(webman_config->cmask & BLU), buffer);
	add_checkbox("rxv", STR_RXVID                    ,     ")<br>"    ,  (webman_config->rxvid)      , buffer);

	add_checkbox("dvd", "DVD "                       ,       STR_VIDLG, !(webman_config->cmask & DVD), buffer);
	concat(buffer, "<br>");
#endif

	#if defined(MOUNT_ROMS)
	b = isDir(PKGLAUNCH_DIR);
	if(b)
		add_checkbox_line("rom", "ROMS",  (webman_config->roms),  buffer);
	else
		concat(buffer, "<br>");
	#endif

	add_checkbox("igf", "wm_ignore.txt", " <button onclick=\"window.location='/edit.ps3" WM_IGNORE_FILES "';return false;\">&#x270D;</button><br>", webman_config->ignore, buffer);

	concat(buffer, "</tr></table>" HTML_BLU_SEPARATOR);

#ifdef COBRA_ONLY
 #ifdef NET_SUPPORT
	//ps3netsvr settings
	char _nd[4], _neth[6], _netp[6], PS3NETSRV[88];
	sprintf(PS3NETSRV, " &nbsp; <a href=\"/net0\" style=\"%s\">PS3NETSRV#1 IP:</a>", HTML_URL_STYLE);

	for(u8 id = 0; id < netsrvs; id++)
	{
		sprintf(_nd, "nd%i", id); sprintf(_neth, "neth%i", id); sprintf(_netp, "netp%i", id);

		add_checkbox(_nd, STR_LANGAMES,  PS3NETSRV, (webman_config->netd[id]), buffer);
		sprintf(templn, HTML_INPUT("%s", "%s", "15", "16") ":" HTML_PORT("%s", "%i") "<br>",
				_neth, webman_config->neth[id],
				_netp, webman_config->netp[id]); concat(buffer, templn);
		++PS3NETSRV[21], ++PS3NETSRV[75];
	}
	add_checkbox_line("nsd", "Scan remote sub-directories", !(webman_config->nsd), buffer);
 #endif
#endif

	//fan control settings
	concat(buffer, "</div>" HTML_BLU_SEPARATOR "<table width=\"900\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\"><tr><td>");

	add_checkbox("fc\" onchange=\"temp[fc.checked?0:3].checked=1;" , STR_FANCTRL, "<td>", (webman_config->fanc), buffer);
	add_checkbox("warn", STR_NOWARN, " </tr>", (webman_config->nowarn), buffer);

	concat(buffer, "<tr><td>");
	add_radio_button("temp\" onchange=\"fc.checked=1;", 0, "t_0", STR_AUTOAT , " : ", (webman_config->man_speed == 0), buffer);
	sprintf(templn, HTML_NUMBER("step\"  accesskey=\"T", "%i", "40", "80") " °C"
					"<td><label><input type=\"checkbox\"%s/> %s</label> : " HTML_NUMBER("mfan", "%i", "20", "95") " %% - "
					HTML_NUMBER("mfs", "%i", "40", "95") " %% %s </tr>",
					webman_config->dyn_temp, (webman_config->fanc && (webman_config->man_speed == 0)) ? ITEM_CHECKED : "",
					STR_LOWEST, webman_config->minfan, webman_config->maxfan, STR_FANSPEED); concat(buffer, templn);

	concat(buffer, "<tr><td>");
	add_radio_button("temp\" onchange=\"fc.checked=1;", 1, "t_1", STR_MANUAL , " : ", (webman_config->man_speed != 0), buffer);
	sprintf(templn, HTML_NUMBER("manu", "%i", "20", "95") " %% %s "
					"<td> %s : " HTML_NUMBER("fsp0", "%i", "20", "95") " %% %s </tr>",
					(webman_config->man_rate), STR_FANSPEED, STR_PS2EMU, webman_config->ps2_rate, STR_FANSPEED); concat(buffer, templn);

	concat(buffer, "<tr><td>");
	add_radio_button("temp\" onchange=\"fc.checked=1;", 3, "t_3", "Auto #2", _BR_, (webman_config->fanc == FAN_AUTO2), buffer);
	add_radio_button("temp\" onchange=\"fc.checked=0;", 2, "t_2", "SYSCON", "<td>", !(webman_config->fanc), buffer);

#ifndef LITE_EDITION
	concat(buffer, "<br>");
	add_checkbox("ct", "CPU/RSX/FAN Chart",  " ", (webman_config->chart), buffer);
	if(file_exists(CPU_RSX_CHART)) {sprintf(templn, HTML_URL, CPU_RSX_CHART, "&#x1F453;"); concat(buffer, templn);}
#endif
	concat(buffer, "</table>");

	//general settings
	sprintf(templn, "%scfg);\"> " WM_APPNAME " MOD %s </a></b><br><div id=\"cfg\">", HTML_TOGGLER, STR_SETUP); concat(buffer, templn);

	add_checkbox("lp", STR_LPG   , " • ",   (webman_config->lastp),  buffer);
	add_checkbox("nb", "BEEP", " • ",      !(webman_config->nobeep), buffer);
#ifdef COBRA_ONLY
	add_checkbox("sn", "No SND0.AT3", " • ", (webman_config->nosnd0), buffer);
#endif
	add_checkbox_line("wp", "wm_proxy", !(webman_config->wm_proxy), buffer);

	add_checkbox_line("ab", STR_AUTOB  , (webman_config->autob), buffer);

	add_checkbox_line("dy", STR_DELAYAB, (webman_config->delay), buffer);

#ifdef NOBD_PATCH
	u8 noBD = ALLOW_NOBD;
	const char *SEP = noBD ? " • " : "<br>";
	add_checkbox( "bl", STR_DEVBL, SEP, (webman_config->blind),  buffer);
	if(noBD) add_checkbox_line("bd", "noBD patch", (webman_config->noBD),   buffer);
#else
	add_checkbox_line( "bl", STR_DEVBL,  (webman_config->blind),  buffer);
#endif

	add_checkbox("wn", STR_NOWMDN, " • ",  (webman_config->wmstart),  buffer);
	add_checkbox_line("mn", "Icon", !(webman_config->msg_icon), buffer);

	concat(buffer, HTML_BLU_SEPARATOR);

	add_checkbox("ft", STR_FTPSVC,   " : ", (webman_config->ftpd) , buffer);
	sprintf(templn, HTML_PORT("ff", "%i") " • Timeout ", webman_config->ftp_port); concat(buffer, templn);

#ifdef AUTO_POWER_OFF
	sprintf(templn, HTML_NUMBER("tm", "%i", "0", "255") " mins • ", webman_config->ftp_timeout); concat(buffer, templn);
	add_checkbox_line("pw", "No Auto Power Off",  !(webman_config->auto_power_off), buffer);
#else
	sprintf(templn, HTML_NUMBER("tm", "%i", "0", "255") " mins<br>", webman_config->ftp_timeout); concat(buffer, templn);
#endif

#ifdef PS3NET_SERVER
	sprintf(templn, "%s", STR_FTPSVC); char *pos = strcasestr(templn, "FTP"); if(pos) {pos[0] = 'N', pos[1] = 'E', pos[2] = 'T';}
	add_checkbox("nd", templn,   " : ", (webman_config->netsrvd) , buffer);
	sprintf(templn, HTML_PORT("ndp", "%i") "<br>", webman_config->netsrvp); concat(buffer, templn);
#endif

	add_checkbox("bn", STR_ACCESS,  " : ", (webman_config->bind) , buffer);

	char pwd[24]; _memset(pwd, sizeof(pwd)); memcpy(pwd, webman_config->ftp_password, 20);

	sprintf(templn, HTML_INPUT("aip", "%s", "15", "16") " Pwd: "
					HTML_PASSW("pwd", "%s", "20", "20") "<br>", webman_config->allow_ip, pwd); concat(buffer, templn);

	concat(buffer, HTML_BLU_SEPARATOR);

	add_checkbox_line("pl", STR_USBPOLL, (webman_config->poll) , buffer);
#ifdef COBRA_ONLY
	add_checkbox_line("bus", STR_RESET_USB, (webman_config->bus), buffer);
#endif

#ifdef COBRA_ONLY
	if(c_firmware < 4.53f)
		add_checkbox_line("nsp", STR_NOSPOOF, (webman_config->nospoof), buffer);
#endif

#ifdef PHOTO_GUI
	if(file_exists(LAUNCHPAD_FILE_XML))
		add_checkbox_line("lx", "LaunchPad.xml | PhotoGUI (USB0/PICTURE) | Poll wm_request", !(webman_config->launchpad_xml), buffer);
	else if(payload_ps3hen || cobra_version >= 0x0820)
		add_checkbox_line("lx", "PhotoGUI (USB0/PICTURE) | Poll wm_request", !(webman_config->launchpad_xml), buffer);
#endif

	//game mounting
	sprintf(templn, "%s + %s net/ntfs cached ISO", STR_UNMOUNT, STR_DELETE);
	add_checkbox_line("dx", templn, (webman_config->deliso), buffer);

#ifdef NOSINGSTAR
	add_checkbox_line("nss", STR_NOSINGSTAR,  (webman_config->noss), buffer);
#endif
//	add_checkbox_line("xp", STR_COMBOS,       (webman_config->nopad), buffer);

#ifdef PKG_HANDLER
	add_checkbox_line("ai", "Auto Install PKG", !(webman_config->auto_install_pkg), buffer); // when NTFS/NET ISO is mounted as /dev_bdvd
#endif
#ifdef UNLOCK_SAVEDATA
	add_checkbox_line("up", "Unlock savedata", (webman_config->unlock_savedata), buffer);
#endif
#ifdef PS3MON_SPRX
	if(file_exists(PS3MON_SPRX))
		add_checkbox_line("pm", "PS3Mon", (webman_config->ps3mon), buffer);
#endif

	//game listing
	concat(buffer, "</div>" HTML_BLU_SEPARATOR);

	add_checkbox_line("rt", STR_MYGAMES, !(webman_config->root), buffer);
	add_checkbox_line("rf", STR_CONTSCAN, (webman_config->refr), buffer);

#ifdef LAUNCHPAD
	b = file_exists(LAUNCHPAD_FILE_XML);
	add_checkbox("ng" , STR_NOGRP,  b ? " & " : _BR_, (webman_config->nogrp  ),       buffer);
	if(b)
		add_checkbox_line("lg" , "LaunchPad.xml",    (webman_config->launchpad_grp), buffer);
#else
	add_checkbox_line("ng" , STR_NOGRP,     (webman_config->nogrp  ), buffer);
#endif

	add_checkbox_line("ns" , STR_NOSETUP,   (webman_config->nosetup), buffer);

	value = webman_config->nocov;
	add_checkbox("nc\" onclick=\"ic.value=(nc.checked)?1:0;", STR_MMCOVERS, " : ", (value == SHOW_ICON0), buffer);

	// icon type
	add_html('1', value, buffer, templn);

#ifndef ENGLISH_ONLY
	//language
	sprintf(templn, " • %s: ", STR_PLANG); concat(buffer, templn);
	add_html('2', webman_config->lang, buffer, templn);
#endif
	concat(buffer, "<br>");

	add_checkbox("tid", STR_TITLEID, " • ", (webman_config->tid),  buffer);
	add_checkbox("sfo", "PARAM.SFO", " • ",!(webman_config->use_filename), buffer);

	// info
	#ifndef LITE_EDITION
	add_html('3', webman_config->info, buffer, templn);
	#else
	add_html('3', webman_config->info & 0xF, buffer, templn);
	#endif
	add_html('4', webman_config->minfo, buffer, templn);

#ifdef FIX_GAME
	if(c_firmware >= 4.20f && c_firmware < LATEST_CFW)
	{
		value = webman_config->fixgame;
		add_checkbox("nf", STR_FIXGAME,  " : <select name=\"fm\">", (value == FIX_GAME_DISABLED), buffer);
		add_option_item(0, "Auto"  , (value == FIX_GAME_AUTO), buffer);
		add_option_item(1, "Quick" , (value == FIX_GAME_QUICK), buffer);
		add_option_item(2, "Forced", (value == FIX_GAME_FORCED), buffer);
		concat(buffer, "</select><br>");
	}
#endif

#ifdef PLAY_MUSIC
	add_checkbox("apd", STR_AUTO_PLAY, " • ", (webman_config->autoplay), buffer);
	concat(buffer, "XMB Media: <select name=\"ms\">");
	add_option_item(0, STR_DISABLED, (webman_config->music == 0), buffer);
	add_option_item(1, "Music",      (webman_config->music == 1), buffer);
	add_option_item(2, "Video",      (webman_config->music == 2), buffer);
	concat(buffer, "</select><br>");
#else
	add_checkbox_line("apd", STR_AUTO_PLAY, (webman_config->autoplay), buffer);
#endif
	add_checkbox_line("sm\"  accesskey=\"G", "sMAN GUI", (webman_config->sman), buffer);
	#ifdef ARTEMIS_PRX
	add_checkbox_line("ar", "Artemis", (webman_config->artemis), buffer);
	#endif

	//general settings
#ifdef SPOOF_CONSOLEID
	sprintf(templn,	"%sadv);\"> IDPS & MEM %s </a></b><br><div id=\"adv\">", HTML_TOGGLER, STR_SETUP); concat(buffer, templn);
#else
	sprintf(templn,	"%sadv);\"> MEM %s </a></b><br><div id=\"adv\">", HTML_TOGGLER, STR_SETUP); concat(buffer, templn);
#endif

#ifdef SPOOF_CONSOLEID
	concat(buffer, "<span id='ht'>");

	char tmp[80];
	sprintf(tmp, "%016llX';vID2.value='%016llX", eid0_idps[0], eid0_idps[1]);

	add_checkbox("id1", "IDPS", " : ", (webman_config->sidps), buffer);
	sprintf(templn, HTML_INPUT("%s", "%s", "16", "22"), "vID1", webman_config->vIDPS1); concat(buffer, templn);
	sprintf(templn, HTML_INPUT("%s", "%s", "16", "22"), "vID2", webman_config->vIDPS2); concat(buffer, templn);
	//sprintf(templn, HTML_BUTTON_FMT "<br>", HTML_BUTTON, " ", "onclick=\"if(t=='ht')h();vID2.value=", "1000000000000000"); concat(buffer, templn);
	sprintf(templn, HTML_BUTTON_FMT "<br>", HTML_BUTTON, " ", "onclick=\"if(t=='ht')h();vID1.value=", tmp); concat(buffer, templn);

	sprintf(tmp, "%016llX';vPS2.value='%016llX", PSID[0], PSID[1]);

	add_checkbox("id2", "PSID", " : ", (webman_config->spsid), buffer);
	sprintf(templn, HTML_INPUT("%s", "%s", "16", "22"), "vPS1", webman_config->vPSID1); concat(buffer, templn);
	sprintf(templn, HTML_INPUT("%s", "%s", "16", "22"), "vPS2", webman_config->vPSID2); concat(buffer, templn);
	//sprintf(templn, HTML_BUTTON_FMT "<br><br>", HTML_BUTTON, " ", "onclick=\"vPS1.value=vPS2.value=", "0000000000000000"); concat(buffer, templn);
	//sprintf(templn, HTML_BUTTON_FMT, HTML_BUTTON, " ", "onclick=\"if(t=='ht')h();vPS1.value=vPS2.value=", "0000000000000000"); concat(buffer, templn);
	sprintf(templn, HTML_BUTTON_FMT, HTML_BUTTON, " ", "onclick=\"if(t=='ht')h();vPS1.value=", tmp); concat(buffer, templn);

	add_html('p', 0, buffer, templn);
#endif

	//Disable lv1&lv2 peek&poke syscalls (6,7,9,10,36) and delete history files at system startup
#ifdef COBRA_ONLY
	#ifdef REMOVE_SYSCALLS
	add_checkbox("spp", STR_DELCFWSYS, " ", (webman_config->spp & 1), buffer);
	#endif

	//add_checkbox("shh", "Offline [Lock PSN]", _BR_, (webman_config->spp & 2), buffer);
	concat(buffer, " • Offline  : <select name=\"shh\">");
	add_option_item(0, STR_DISABLED, !(webman_config->spp & 6), buffer);

	if(!payload_ps3hen)
		add_option_item(1, "Lock PSN",    (webman_config->spp & 2), buffer);

	#ifdef OFFLINE_INGAME
	add_option_item(2, STR_GAMES,     (webman_config->spp & 4), buffer);
	#endif
	concat(buffer, "</select>");
#endif
	concat(buffer, HTML_BLU_SEPARATOR);
	buffer += strlen(buffer);

#ifndef LITE_EDITION
	//default content profile
	sprintf(templn, "%s : <select name=\"usr\">", STR_PROFILE); concat(buffer, templn);
	add_option_item(0 , STR_DEFAULT, (profile == 0) , buffer);
	char profile_id[] = {'1', 0};
	for(u8 usr = 1; usr <= 4; ++usr, ++profile_id[0])
		add_option_item(usr, profile_id, (profile == usr) , buffer);
	add_option_item(5, "*", (profile >= 5) , buffer);

	#ifdef USE_UACCOUNT
	//default user account
	if(!webman_config->uaccount[0]) sprintf(webman_config->uaccount, "%08i", xusers()->GetCurrentUserNumber());

	sprintf(templn, "</select> : <a href=\"%s\">%s</a><select name=\"uacc\">", HDD0_HOME_DIR, HDD0_HOME_DIR + 5); concat(buffer, templn);
	{
		int fd;
		if(cellFsOpendir(HDD0_HOME_DIR, &fd) == CELL_FS_SUCCEEDED)
		{
			CellFsDirent dir; u64 read_e;

			while(working && (cellFsReaddir(fd, &dir, &read_e) == CELL_FS_SUCCEEDED) && (read_e > 0))
			{
				if(dir.d_namlen == 8)
					add_string_item(dir.d_name, dir.d_name, IS(dir.d_name, webman_config->uaccount), buffer);
			}
			cellFsClosedir(fd);
		}
	}
	#endif

	#ifdef NOBD_PATCH
	sprintf(templn, "</select> &nbsp; %s : [<a href=\"/delete.ps3?wmconfig\">wmconfig</a>] [<a href=\"/delete.ps3?wmtmp\">wmtmp</a>] [<a href=\"/delete.ps3?history\">history</a>] • [<a href=\"/rebuild.ps3\">rebuild</a>] [<a href=\"/recovery.ps3\">recovery</a>]%s<p>", STR_DELETE, ALLOW_NOBD ? " [<a href=\"/nobd.ps3\">noBD</a>]" : ""); concat(buffer, templn);
	#else
	sprintf(templn, "</select> &nbsp; %s : [<a href=\"/delete.ps3?wmconfig\">wmconfig</a>] [<a href=\"/delete.ps3?wmtmp\">wmtmp</a>] [<a href=\"/delete.ps3?history\">history</a>] • [<a href=\"/rebuild.ps3\">rebuild</a>] [<a href=\"/recovery.ps3\">recovery</a>]<p>", STR_DELETE); concat(buffer, templn);
	#endif
#endif

	//memory usage
	sprintf(templn, " %s [%iKB]: ", STR_MEMUSAGE, (webman_config->vsh_mc) ? 3072 : (int)(BUFFER_SIZE_ALL / KB)); concat(buffer, templn);
	add_html('5', webman_config->foot, buffer, templn);

	//memory container
	add_option_item(0,  STR_DEFAULT, (webman_config->vsh_mc == 0)  , buffer);
	add_html('6', webman_config->vsh_mc, buffer, templn);

	#ifndef LITE_EDITION
	//Home
	sprintf(templn, " : " HTML_INPUT("hurl", "%s", "255", "50") "<p>", webman_config->home_url);
	add_checkbox("hm", STR_HOME, templn, webman_config->homeb, buffer);
	#endif

	#ifdef BDVD_REGION
	cobra_read_config(cobra_config);

	cobra_config->bd_video_region = cconfig[4]; // One of BDRegion, or 0 for default
	cobra_config->dvd_video_region = cconfig[5]; // One of DVDRegion or 0 for default
	cobra_config->fan_speed = cconfig[15]; // 0 = SYSCON, 1 = Dynamic Fan Controller, 0x33 to 0xFF = Set manual fan speed

	//BD Region
	concat(buffer, "BD Region: <select id=\"bdr\" name=\"bdr\">");
	add_option_item(0,  STR_DEFAULT, (cobra_config->bd_video_region == 0)  , buffer);
	add_html('7', cobra_config->bd_video_region, buffer, templn);

	//DVD Region
	add_option_item(0,  STR_DEFAULT, (cobra_config->dvd_video_region == 0)  , buffer);
	add_html('8', cobra_config->dvd_video_region, buffer, templn);
	#endif // #ifdef BDVD_REGION

	#ifdef VIDEO_REC
	add_html('v', webman_config->rec_video_format, buffer, templn);
	add_html('a', webman_config->rec_audio_format, buffer, templn);
	#endif // #ifdef VIDEO_REC

	buffer += strlen(buffer);

	//combos
	sprintf(templn, "</div>%scmb);\"> %s </a></b><br><div id=\"cmb\">"
					"<button onclick=\"var cb=$$('cmb').querySelectorAll('input[type=checkbox]');for(i=0;i<cb.length;i++)cb[i].checked=false;return false;\">%s</button>"
					"<table width=\"800\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\"><tr><td nowrap valign=top>", HTML_TOGGLER, STR_COMBOS2, STR_COMBOS); concat(buffer, templn);

	#ifdef COBRA_ONLY
	add_checkbox("vs", "VSH MENU",      " : <b>SELECT</b><br>"       , !(webman_config->combo2 & C_VSHMENU), buffer);
	add_checkbox("gm", "GAME MENU",     " : <b>START / L2+R2</b><br>", !(webman_config->combo2 & C_SLAUNCH), buffer);
	#ifdef WM_REQUEST
	add_checkbox("fps","FPS COUNT",     " : <b>L3+R3</b><br>"       ,  !(webman_config->combo  & C_FPSINFO), buffer);
	#endif
	#endif // #ifdef COBRA_ONLY

	#ifdef SYS_ADMIN_MODE
	add_checkbox("adm", "ADMIN/USER MODE", " : <b>L2+R2+&#8710;</b><br>" ,  (webman_config->combo & SYS_ADMIN),  buffer);
	#endif

	add_checkbox("pfs", STR_FAILSAFE,   " : <b>SELECT+L3+L2+R2</b><br>"  , !(webman_config->combo & FAIL_SAFE),  buffer);
	add_checkbox("pss", STR_SHOWTEMP,   " : <b>SELECT+START</b><br>"     , !(webman_config->combo & SHOW_TEMP),  buffer);
	add_checkbox("ppv", STR_PREVGAME,   " : <b>SELECT+L1</b><br>"        , !(webman_config->combo & PREV_GAME),  buffer);
	add_checkbox("pnx", STR_NEXTGAME,   " : <b>SELECT+R1</b><br>"        , !(webman_config->combo & NEXT_GAME),  buffer);

	sprintf(templn, " : <b>SELECT+%c</b><br>", (CELL_PAD_CIRCLE_BTN == CELL_PAD_CTRL_CIRCLE) ? 'O' : 'X');
	add_checkbox("umt", STR_UNMOUNT,    templn                           , !(webman_config->combo2 & UMNT_GAME), buffer);
	add_checkbox("pgd", "gameDATA",     " : <b>SELECT+&#9633;</b><br>"   , !(webman_config->combo2 & EXTGAMDAT), buffer);

	sprintf(templn, "%s XML", STR_REFRESH);
	add_checkbox("pxr", templn,         " : <b>SELECT+L3</b><br>"        , !(webman_config->combo2 & XMLREFRSH), buffer);

	#ifdef VIDEO_REC
	add_checkbox("vrc", "VIDEO REC (in-game)", " : <b>SELECT+R3</b><br>" , !(webman_config->combo2 & VIDRECORD), buffer);
	#endif

#ifdef REX_ONLY
	sprintf(templn, " : <b>R2+%c</b><br>", (CELL_PAD_CIRCLE_BTN == CELL_PAD_CTRL_CIRCLE) ? 'O' : 'X');
	add_checkbox("pid", STR_SHOWIDPS,   templn                    , !(webman_config->combo & SHOW_IDPS),  buffer);
	add_checkbox("puw", STR_UNLOADWM,   " : <b>L3+R2+R3</b><br>"  , !(webman_config->combo & UNLOAD_WM),  buffer);
	add_checkbox("psd", STR_SHUTDOWN2,  " : <b>L3+R2+X</b><br>"   , !(webman_config->combo & SHUT_DOWN),  buffer);
	add_checkbox("prs", STR_RESTART2,   " : <b>L3+R2+O</b><br>"   , !(webman_config->combo & RESTARTPS),  buffer);
	#ifdef WM_REQUEST
	add_checkbox("psv", "CUSTOM COMBO", " : <b>R2+&#9633;</b><td>", !(webman_config->combo2 & CUSTOMCMB), buffer);
	#else
	add_checkbox("psv", "BLOCK SERVERS"," : <b>R2+&#9633;</b><td>", !(webman_config->combo2 & CUSTOMCMB), buffer);
	#endif
#else
	#ifdef SPOOF_CONSOLEID
	sprintf(templn, " : <b>R2+%c</b><br>", (CELL_PAD_CIRCLE_BTN == CELL_PAD_CTRL_CIRCLE) ? 'O' : 'X');
	add_checkbox("pid", STR_SHOWIDPS,   templn                         ,   !(webman_config->combo & SHOW_IDPS),  buffer);
	#endif

	#ifdef WM_REQUEST
	add_checkbox("psv", "CUSTOM COMBO", " : <b>R2+&#9633;</b><td>", !(webman_config->combo2 & CUSTOMCMB), buffer);
	#else
	add_checkbox("psv", "BLOCK SERVERS"," : <b>R2+&#9633;</b><td>", !(webman_config->combo2 & CUSTOMCMB), buffer);
	#endif

	add_checkbox("puw", STR_UNLOADWM,   " : <b>L3+R2+R3</b><br>"  , !(webman_config->combo & UNLOAD_WM),  buffer);
	add_checkbox("psd", STR_SHUTDOWN2,  " : <b>L3+R2+X</b><br>"   , !(webman_config->combo & SHUT_DOWN),  buffer);
	add_checkbox("prs", STR_RESTART2,   " : <b>L3+R2+O</b><br>"   , !(webman_config->combo & RESTARTPS),  buffer);
#endif
	add_checkbox("pdf", STR_FANCTRL4,   " : <b>L3+R2+START</b><br>", !(webman_config->combo & DISABLEFC),  buffer);
	add_checkbox("pf1", STR_FANCTRL2,   " : <b>SELECT+&#8593;/&#8595;</b><br>", !(webman_config->combo & MANUALFAN),  buffer);
	add_checkbox("pf2", STR_FANCTRL5,   " : <b>SELECT+&#8592;/&#8594;</b><br>", !(webman_config->combo & MINDYNFAN),  buffer);

#ifdef REMOVE_SYSCALLS
	add_checkbox("psc", STR_DELCFWSYS2, " : <b>R2+&#8710;</b><BR>&nbsp; (", !(webman_config->combo & DISABLESH),  buffer);
	add_checkbox("dsc", "PS3 DISC", " ", (webman_config->dsc), buffer);
	add_checkbox("kcc", "CCAPI", " ",  !(webman_config->keep_ccapi), buffer);

	#ifdef COBRA_ONLY
	concat(buffer, "• PS3MAPI <select name=\"sc8\">");
	add_option_item(1, STR_ENABLED,  (webman_config->sc8mode != PS3MAPI_DISABLED), buffer);
	add_option_item(0, STR_DISABLED, (webman_config->sc8mode == PS3MAPI_DISABLED), buffer);
	concat(buffer, "</select>)<br>");
	#else
	concat(buffer, ")<br>");
	#endif
#endif

#ifdef COBRA_NON_LITE
	#ifdef NET_SUPPORT
	add_checkbox("pn0", "NET0",        " : <b>SELECT+R2+&#9633;</b><br>", !(webman_config->combo2 & MOUNTNET0), buffer);
	add_checkbox("pn1", "NET1",        " : <b>SELECT+L2+&#9633;</b><br>", !(webman_config->combo2 & MOUNTNET1), buffer);
	#endif
	add_checkbox("pdc", STR_DISCOBRA,  " : <b>L3+L2+&#8710;</b><br>", !(webman_config->combo & DISACOBRA),  buffer);
#endif

#ifdef REX_ONLY
	add_checkbox("pr0", STR_RBGMODE,   " : <b>L3+L2+&#9633;</b><br>"    , !(webman_config->combo2 & REBUGMODE), buffer);
	add_checkbox("pr1", STR_RBGNORM,   " : <b>L3+L2+O</b><br>"          , !(webman_config->combo2 & NORMAMODE), buffer);
	add_checkbox("pr2", STR_RBGMENU,   " : <b>L3+L2+X</b><br>"          , !(webman_config->combo2 & DEBUGMENU), buffer);

	if(c_firmware >= 4.65f)
	add_checkbox("p2c", "PS2 CLASSIC", " : <b>SELECT+L2+&#8710;</b><br>", !(webman_config->combo2 & PS2TOGGLE), buffer);
#endif

#ifndef LITE_EDITION
	add_checkbox("p2s", "PS2 SWITCH",  " : <b>SELECT+L2+R2</b><br>"     , !(webman_config->combo2 & PS2SWITCH), buffer);
#endif

#ifdef PKG_HANDLER
	sprintf(templn, " : <b>SELECT+R2+%c</b><br>", (CELL_PAD_CIRCLE_BTN == CELL_PAD_CTRL_CIRCLE) ? 'O' : 'X');
	add_checkbox("pkg", "INSTALL PKG", templn                           , !(webman_config->combo2 & INSTALPKG), buffer);
#endif
	add_checkbox("hom", "GOTO_HOME", " : <b>L2+L3+R3</b><br>"           , !(webman_config->combo & GOTO_HOME), buffer);

	add_checkbox("pld", "PLAY DISC", " : <b>L2+START</b><br>"
						"</tr></table>"                            , !(webman_config->combo2 & PLAY_DISC), buffer);

	// custom combo R2+SQUARE
#if defined(WM_CUSTOM_COMBO) || defined(WM_REQUEST)
	char command[256];

 #ifdef WM_CUSTOM_COMBO
	if( read_file(WM_CUSTOM_COMBO "r2_square", command, 255, 0) == 0)
 #endif
		read_file(WM_COMBO_PATH, command, 255, 0);

	sprintf(templn, "&nbsp; &nbsp;" HTML_INPUT("ccbo\" list=\"cmds", "%s", "255", "50")
					#ifdef WM_CUSTOM_COMBO
					"<button onclick=\"window.location='/edit.ps3%s';return false;\">&#x270D;</button>"
					"<br>", command, WM_CUSTOM_COMBO "r2_square"); concat(buffer, templn);
					#else
					"<br>", command); concat(buffer, templn);
					#endif

	add_html('x', 0, buffer, templn);

#endif // #if defined(WM_CUSTOM_COMBO) || defined(WM_REQUEST)

	concat(buffer, "</div>");

	//Wait for any USB device to be ready
	sprintf(templn, "%swt);\"> %s </a></b><br><div id=\"wt\">", HTML_TOGGLER, STR_ANYUSB); concat(buffer, templn);
	add_html('d', webman_config->bootd, buffer, templn);

	//Wait additionally for each selected USB device to be ready
	sprintf(templn, HTML_BLU_SEPARATOR "<u> %s:</u><br>", STR_ADDUSB); concat(buffer, templn);
	add_html('s', webman_config->boots, buffer, templn);

	sprintf(templn, HTML_RED_SEPARATOR "<input class=\"bs\" type=\"submit\" accesskey=\"S\" value=\" %s \"/>"
					"<script>function tgl(o){o.style.maxHeight=(o.style.maxHeight=='500px')?'0px':'500px';}</script>"
					"</form>", STR_SAVE); concat(buffer, templn);

#ifndef LITE_EDITION
	#ifdef PKG_HANDLER
	add_html('z', 0, buffer, templn);

//	concat(buffer,  HTML_RED_SEPARATOR
//					"<a href=\"http://github.com/aldostools/webMAN-MOD/releases\">" WEBMAN_MOD " - Latest release</a> • "
//					"<a href=\"/install.ps3/dev_hdd0/packages\">Install PKG</a> • "
//					"<a href=\"/install.ps3/dev_hdd0/theme\">Install P3T</a> • "
//					"<a href=\"/install.ps3$\">Add-ons</a><br>"
//					"<a href=\"http://psx-place.com/forums/wMM.126/\">" WEBMAN_MOD " - Info @ PSX-Place</a><br>");
	#else
	concat(buffer,  HTML_RED_SEPARATOR
					"<a href=\"http://ps3.aldostools.org/links.html\">" WEBMAN_MOD " - Latest release</a><br>"
					"<a href=\"http://psx-place.com/forums/wMM.126/\">" WEBMAN_MOD " - Info @ PSX-Place</a><br>");
	#endif
#else
	concat(buffer,  HTML_BLU_SEPARATOR
					WM_APPNAME " - Simple Web Server" EDITION "<p>");
#endif

/*
	#define VSH_GCM_OBJ			0x70A8A8 // 4.53cex
	//#define VSH_GCM_OBJ		0x71A5F8 // 4.46dex

	u32 *gcm_obj0 = VSH_GCM_OBJ + ((u32) 0 << 4);
	u32 *gcm_obj1 = VSH_GCM_OBJ + ((u32) 1 << 4); // offset, pitch, width, height


	_cellGcmIoOffsetToAddress = getNIDfunc("sdk", 0x2a6fba9c, 0);

	void *buf_adr[2];

	if(_cellGcmIoOffsetToAddress)
	{
		_cellGcmIoOffsetToAddress(gcm_obj0[0], &buf_adr[0]);
		_cellGcmIoOffsetToAddress(gcm_obj1[0], &buf_adr[1]); //0x37ee5ac
	}

	sprintf(templn, "OFFSET#0: %x, P: %i, W: %i, H: %i, E: %x <br>",
		gcm_obj0[0], gcm_obj0[1], gcm_obj0[2], gcm_obj0[3], buf_adr[0]); concat(buffer, templn);

	sprintf(templn, "OFFSET#1: %x, P: %i, W: %i, H: %i, E: %x <br>",
		gcm_obj1[0], gcm_obj1[1], gcm_obj1[2], gcm_obj1[3], buf_adr[1]); concat(buffer, templn);
*/
}

static int save_settings(void)
{
	#ifdef COBRA_ONLY
	apply_remaps(); // update remaps on startup / save settngs
	#endif

	#ifdef MUTE_SND0
	mute_snd0(webman_config->nosnd0 != prev_nosnd0);
	#endif

	#ifdef PHOTO_GUI
	photo_gui = !webman_config->launchpad_xml;
	if(photo_gui) { system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PHOTO_GUI, (u64)photo_gui); }
	#endif

	#ifdef USE_NTFS
	root_check = true;
	#endif

	return save_file(WM_CONFIG_FILE, (char*)wmconfig, sizeof(WebmanCfg));
}

static void read_settings(void)
{
	_memset(webman_config, sizeof(WebmanCfg));

	webman_config->version = 0x1337;

	webman_config->usb0 = 1;
	webman_config->usb1 = 1;
	//webman_config->usb2 = 0;
	//webman_config->usb3 = 0;
	webman_config->usb6 = 1;
	//webman_config->usb7 = 0;

	//webman_config->dev_sd = 0;
	//webman_config->dev_ms = 0;
	//webman_config->dev_cf = 0;

	#ifdef USE_NTFS
	webman_config->ntfs = 1; // use internal prepNTFS to scan content
	#endif

	#ifdef MOUNT_ROMS
	webman_config->roms = isDir(PKGLAUNCH_DIR); f1_len = webman_config->roms ? 13 : 11;
	#endif

	//webman_config->lastp = 0;       //disable last play
	//webman_config->autob = 0;       //disable check for AUTOBOOT.ISO
	//webman_config->delay = 0;       //don't delay loading of AUTOBOOT.ISO/last-game (Disc Auto-start)

	//webman_config->bootd = 0;       //don't wait for any USB device to be ready
	webman_config->boots = 3;         //wait 3 additional seconds for each selected USB device to be ready

	//webman_config->nogrp = 0;       //group content on XMB
	//webman_config->wmstart = 0;     //enable start up message (webMAN Loaded!)
	//webman_config->tid = 0;         //don't include the ID as part of the title of the game
	//webman_config->nosetup = 0;     //enable webMAN Setup entry in "webMAN Games"

#ifdef COBRA_ONLY
	webman_config->cmask = 0;
#else
	webman_config->cmask = (PSP | PS1 | BLU | DVD);
#endif

	webman_config->poll = 1;             //disable USB polling
	//webman_config->auto_power_off = 0; //enable prevent auto power off

	//webman_config->nopad = 0;       //enable all PAD shortcuts
	//webman_config->nocov = 0;       //enable multiMAN covers    (0 = Use MM covers, 1 = Use ICON0.PNG, 2 = No game icons, 3 = Online Covers)
	//webman_config->nobeep = 0;      //enable beep on reboot / shutdown / disable syscall

	webman_config->fanc     = ENABLED; //fan control enabled
	//webman_config->man_speed = 0;    //0=dynamic fan control mode, >0 set manual fan speed in %
	webman_config->dyn_temp = MY_TEMP; //°C target temperature for dynamic fan control
	webman_config->man_rate = 35;      //% manual fan speed
	webman_config->ps2_rate = 40;      //% ps2 fan speed

	if(payload_ps3hen) webman_config->man_speed = 0x5A; // ps3hen default is 35% manual
/*
	if(payload_ps3hen) 
	{
		webman_config->man_rate = 30;      //% manual fan speed
		webman_config->man_speed = 0x4C; // ps3hen default is 30% manual
	}
*/
	webman_config->minfan = DEFAULT_MIN_FANSPEED; // 25% defined in fancontrol.h
	webman_config->maxfan = DEFAULT_MAX_FANSPEED; // 80% defined in fancontrol.h

	//webman_config->bind = 0;        //enable remote access to FTP/WWW services
	//webman_config->ftpd = 0;        //enable ftp server
	//webman_config->ftp_password  =  "";

	//webman_config->netsrvp  = NETPORT;
	//webman_config->ftp_port = FTPPORT;

	//for(u8 id = 0; id < 5; id++) webman_config->netp[id] = NETPORT; // webman_config->netd[id] = 0; webman_config->neth[id][0] = '\0';

	webman_config->refr = 1;          //Disable content scan on startup (as default)
	//webman_config->foot  = 0;       //Standard (896KB)
	webman_config->vsh_mc = 4;        //Use background VSH Memory Container (allocates 3MB for content scanning)
	webman_config->nospoof = 1;       //don't spoof fw version

	#ifdef MOUNT_GAMEI
	webman_config->gamei = is_app_home_onxmb(); // scan GAMEI
	#endif

	webman_config->pspl = 1;          //Show PSP Launcher
	webman_config->ps2l = 1;          //Show PS2 Classic Launcher

	if(ALLOW_NOBD)
		webman_config->noBD = isNOBD;     //Get initial status for noBD

	//webman_config->ps2emu = 0;      //default PS2 emulator on B/C consoles: 0 = ps2_emu, 1 = ps2_netemu
	//webman_config->ps2config = 0;   //enable auto lookup for PS2 CONFIG

	//webman_config->spp   = 0;       //disable removal of syscalls
	//webman_config->fixgame = FIX_GAME_AUTO;

	//webman_config->sidps = 0;       //spoof IDPS
	//webman_config->spsid = 0;       //spoof PSID

	//webman_config->vIDPS1[0] = webman_config->vIDPS2[0] = 0;
	//webman_config->vPSID1[0] = webman_config->vPSID2[0] = 0;

	//webman_config->bus = 0;         //enable reset USB bus

	//webman_config->autoplay = 0;    //enable global autoplay

	webman_config->combo  =  DISACOBRA; //disable combo for cobra toggle
	webman_config->combo2 = (REBUGMODE|NORMAMODE|DEBUGMENU|PS2SWITCH|VIDRECORD); //disable combos for rebug/ps2 switch/video record

	//webman_config->rec_video_format = CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_512K_30FPS;
	//webman_config->rec_audio_format = CELL_REC_PARAM_AUDIO_FMT_AAC_96K;

	// default user account (used by /copy.ps3 to import .edat, /exdata, /savedata, /trophy)
	//_memset(webman_config->uaccount, 8);

	// set default language
	#ifndef ENGLISH_ONLY
	get_system_language(&webman_config->lang);
	#else
	webman_config->lang = 0; // english
	#endif

	bool save_defaults = false;

	// read current settings
	if(file_exists(WM_CONFIG_FILE))
		read_file(WM_CONFIG_FILE, (char*)&wmconfig, sizeof(WebmanCfg), DONT_CLEAR_DATA);
	else
		save_defaults = true;

	#ifndef COBRA_ONLY
	webman_config->spp = 0; //disable removal of syscalls on nonCobra
	#else
	if(webman_config->sc8mode < 1 || webman_config->sc8mode >= 4) webman_config->sc8mode = PS3MAPI_DISABLED; // default: disable all syscalls (including sc8)
	#endif

	#ifdef MUTE_SND0
	prev_nosnd0 = webman_config->nosnd0;
	#endif

	// set default autoboot path
	if((webman_config->autoboot_path[0] != '/') && !islike(webman_config->autoboot_path, "http")) sprintf(webman_config->autoboot_path, "%s", DEFAULT_AUTOBOOT_PATH);

	// check stored data
	if(webman_config->maxfan < 40) webman_config->maxfan = 80; // % (0xCC)
	if(webman_config->nowarn >  1) webman_config->nowarn = 0;

	webman_config->minfan   = RANGE(webman_config->minfan, MIN_FANSPEED, 95);   // %
	webman_config->maxfan   = RANGE(webman_config->maxfan, 40, 95);   // %
	if(webman_config->minfan > webman_config->maxfan) webman_config->maxfan = webman_config->minfan;

	webman_config->man_rate = RANGE(webman_config->man_rate, MIN_FANSPEED, webman_config->maxfan);       // %
	webman_config->ps2_rate = RANGE(webman_config->ps2_rate, MIN_FANSPEED, webman_config->maxfan); // %
	webman_config->dyn_temp = RANGE(webman_config->dyn_temp, 40, MAX_TEMPERATURE);  //°C

	original_fanc = (webman_config->fanc == FAN_AUTO2) ? FAN_AUTO2 : ENABLED;

#if defined(SPOOF_CONSOLEID)
	get_eid0_idps();
	if(!webman_config->vIDPS1[0] && !webman_config->vIDPS1[1]) {sprintf(webman_config->vIDPS1, "%016llX", IDPS[0]); sprintf(webman_config->vIDPS2, "%016llX", IDPS[1]);}
	if(!webman_config->vPSID1[0] && !webman_config->vPSID1[1]) {sprintf(webman_config->vPSID1, "%016llX", PSID[0]); sprintf(webman_config->vPSID2, "%016llX", PSID[1]);}
#endif

	for(u8 id = 0; id < 5; id++) if(!webman_config->netp[id]) webman_config->netp[id] = NETPORT;

	if(webman_config->netsrvp  < 1)   webman_config->netsrvp = NETPORT;
	if(webman_config->ftp_port < 1 || webman_config->ftp_port == WWWPORT) webman_config->ftp_port = FTPPORT;

#ifdef SYS_ADMIN_MODE
	if(!(webman_config->combo & SYS_ADMIN)) sys_admin = 1; // set admin mode if ADMIN combo L2+R2+TRIANGLE is disabled
#endif

	// settings
	if(save_defaults)
	{
		if(payload_ps3hen) webman_config->refr = 1; //Disable content scan on startup
		webman_config->sman = 1; //default sMAN GUI
		save_settings();
	}
#ifdef PHOTO_GUI
	else
	{
		photo_gui = !webman_config->launchpad_xml;
		if(photo_gui) { system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PHOTO_GUI, (u64)photo_gui); }
	}
#endif

#ifndef LITE_EDITION
	profile = webman_config->profile;
#endif

#ifdef VIDEO_REC
	rec_video_format = webman_config->rec_video_format;
	rec_audio_format = webman_config->rec_audio_format;
#endif
}

static void reset_settings(void)
{
	cellFsUnlink(WM_CONFIG_FILE);
	read_settings();
}

static void restore_settings(void)
{
	#ifdef COBRA_ONLY
	unload_vsh_gui();
	#endif


	for(u8 n = 0; n < 4; n++)
		if(active_socket[n]>NONE) {sys_net_abort_socket(active_socket[n], SYS_NET_ABORT_STRICT_CHECK); sclose(&active_socket[n]);}

	if(webman_config->fanc == DISABLED || webman_config->man_speed == FAN_AUTO)
	{
		bool set_ps2mode = (webman_config->fanc == ENABLED) && (webman_config->ps2_rate >= MIN_FANSPEED);

		if(set_ps2mode)
			restore_fan(SET_PS2_MODE); //set ps2 fan control mode
		else
			restore_fan(SYSCON_MODE);  //restore syscon fan control mode
	}

	#ifdef COBRA_ONLY
	sys_map_path(FB_XML, NULL); // hide webMAN Games after reload XMB
	#endif

	#ifdef WM_PROXY_SPRX
	//{sys_map_path(VSH_MODULE_DIR WM_PROXY_SPRX ".sprx", NULL);}
	#endif

	#ifdef AUTO_POWER_OFF
	setAutoPowerOff(false);
	#endif

	#ifdef COBRA_ONLY
	if(cobra_config->fan_speed) cobra_read_config(cobra_config);
	#endif

	#ifdef USE_NTFS
	unmount_all_ntfs_volumes();
	#endif

	working = plugin_active = 0;
	sys_ppu_thread_usleep(500000);
}
