#ifdef ENGLISH_ONLY

static char STR_HOME[8] = "Home";

#define STR_TRADBY	"<br>"

#define STR_FILES		"Files"
#define STR_GAMES		"Games"
#define STR_SETUP		"Setup"

#define STR_EJECT		"Eject"
#define STR_INSERT		"Insert"
#define STR_UNMOUNT		"Unmount"
#define STR_COPY		"Copy Folder"
#define STR_REFRESH		"Refresh"
#define STR_SHUTDOWN	"Shutdown"
#define STR_RESTART		"Restart"

#define STR_BYTE		"b"
#define STR_KILOBYTE	"KB"
#define STR_MEGABYTE	"MB"
#define STR_GIGABYTE	"GB"

#define STR_COPYING		"Copying"
#define STR_CPYDEST		"Destination"
#define STR_CPYFINISH	"Copy Finished!"
#define STR_CPYABORT	"Copy aborted!"
#define STR_DELETE		"Delete"

#define STR_SCAN1		"Scan these devices"
#define STR_SCAN2		"Scan for content"
#define STR_PSPL		"Show PSP Launcher"
#define STR_PS2L		"Show PS2 Classic Launcher"
#define STR_RXVID		"Show Video sub-folder"
#define STR_VIDLG		"Video"
#define STR_LPG			"Load last-played game on startup"
#define STR_AUTOB		"Check for /dev_hdd0/PS3ISO/AUTOBOOT.ISO on startup"
#define STR_DELAYAB		"Delay loading of AUTOBOOT.ISO/last-game (Disc Auto-start)"
#define STR_DEVBL		"Enable /dev_blind (writable /dev_flash) on startup"
#define STR_CONTSCAN	"Disable content scan on startup"
#define STR_USBPOLL		"Disable USB polling"
#define STR_FTPSVC		"Disable FTP service"
#define STR_FIXGAME		"Disable auto-fix game"
#define STR_COMBOS		"Disable all PAD shortcuts"
#define STR_MMCOVERS	"Disable multiMAN covers"
#define STR_ACCESS		"Disable remote access to FTP/WWW services"
#define STR_NOSETUP		"Disable " WM_APPNAME " Setup entry in \"" WM_APPNAME " Games\""
#define STR_NOSPOOF		"Disable firmware version spoofing"
#define STR_NOGRP		"Disable grouping of content in \"" WM_APPNAME " Games\""
#define STR_NOWMDN		"Disable startup notification of " WM_APPNAME " on the XMB"
#ifdef NOSINGSTAR
 #define STR_NOSINGSTAR	"Remove SingStar icon"
#endif
#define STR_RESET_USB	"Disable Reset USB Bus"
#define STR_AUTO_PLAY	"Auto-Play"
#define STR_TITLEID		"Include the ID as part of the title of the game"
#define STR_FANCTRL		"Enable dynamic fan control"
#define STR_NOWARN		"Disable temperature warnings"
#define STR_AUTOAT		"Auto at"
#define STR_LOWEST		"Lowest"
#define STR_FANSPEED	"fan speed"
#define STR_MANUAL		"Manual"
#define STR_PS2EMU		"PS2 Emulator"
#define STR_LANGAMES	"Scan for LAN games/videos"
#define STR_ANYUSB		"Wait for any USB device to be ready"
#define STR_ADDUSB		"Wait additionally for each selected USB device to be ready"
#define STR_SPOOFID		"Change idps and psid in lv2 memory at system startup"
#define STR_DELCFWSYS	"Disable CFW syscalls and delete history files at system startup"
#define STR_MEMUSAGE	"Plugin memory usage"
#define STR_PLANG		"Plugin language"
#define STR_PROFILE		"Profile"
#define STR_DEFAULT		"Default"
#define STR_COMBOS2		"XMB/In-Game PAD SHORTCUTS"
#define STR_FAILSAFE	"FAIL SAFE"
#define STR_SHOWTEMP	"SHOW TEMP"
#define STR_SHOWIDPS	"SHOW IDPS"
#define STR_PREVGAME	"PREV GAME"
#define STR_NEXTGAME	"NEXT GAME"
#define STR_SHUTDOWN2	"SHUTDOWN "
#define STR_RESTART2	"RESTART&nbsp; "
#define STR_DELCFWSYS2	"DEL CFW SYSCALLS"
#define STR_UNLOADWM	"UNLOAD WM"
#define STR_FANCTRL2	"CTRL FAN"
#define STR_FANCTRL4	"CTRL DYN FAN"
#define STR_FANCTRL5	"CTRL MIN FAN"
#define STR_UPDN		"&#8593;/&#8595;" //↑/↓
#define STR_LFRG		"&#8592;/&#8594;" //←/→

#define STR_DISCOBRA	"COBRA TOGGLE"

#define STR_RBGMODE		"RBG MODE TOGGLE"
#define STR_RBGNORM		"NORM MODE TOGGLE"
#define STR_RBGMENU		"MENU TOGGLE"

#define STR_SAVE		"Save"
#define STR_SETTINGSUPD	"Settings updated.<br><br>Click <a href=\"/restart.ps3\">here</a> to restart your PLAYSTATION®3 system."
#define STR_ERROR		"Error!"

#define STR_MYGAMES		WM_APPNAME " Games"
#define STR_LOADGAMES	"Load games with " WM_APPNAME
#define STR_FIXING		"Fixing"

#define STR_WMSETUP		WM_APPNAME " Setup"
#define STR_WMSETUP2	"Setup " WM_APPNAME " options"

#define STR_EJECTDISC	"Eject Disc"
#define STR_UNMOUNTGAME	"Unmount current game"

#define STR_WMSTART		WM_APPNAME " loaded!"
#define STR_WMUNL		WM_APPNAME " unloaded!"
#define STR_CFWSYSALRD	"CFW Syscalls already disabled"
#define STR_CFWSYSRIP	"Removal History files & CFW Syscalls in progress..."
#define STR_RMVCFWSYS	"History files & CFW Syscalls deleted OK!"
#define STR_RMVCFWSYSF	"Failed to remove CFW Syscalls"

#define STR_RMVWMCFG	WM_APPNAME " config reset in progress..."
#define STR_RMVWMCFGOK	"Done! Restart within 3 seconds"

#define STR_PS3FORMAT	"PS3 format games"
#define STR_PS2FORMAT	"PS2 format games"
#define STR_PS1FORMAT	"PSOne format games"
#define STR_PSPFORMAT	"PSP\xE2\x84\xA2 format games"

#define STR_VIDFORMAT	"Blu-ray\xE2\x84\xA2 and DVD"
#define STR_VIDEO		"Video content"

#define STR_LAUNCHPSP	"Launch PSP ISO mounted through " WM_APPNAME " or mmCM"
#define STR_LAUNCHPS2	"Launch PS2 Classic"

#define STR_GAMEUM		"Game unmounted."

#define STR_EJECTED		"Disc ejected."
#define STR_LOADED		"Disc inserted."

#define STR_GAMETOM		"Game to mount"
#define STR_GAMELOADED	"Game loaded successfully. Start the game from the disc icon<br>or from <b>/app_home</b>&nbsp;XMB entry.</a><hr>Click <a href=\"/mount.ps3/unmount\">here</a> to unmount the game."
#define STR_PSPLOADED	"Game loaded successfully. Start the game using <b>PSP Launcher</b>.<hr>"
#define STR_PS2LOADED	"Game loaded successfully. Start the game using <b>PS2 Classic Launcher</b>.<hr>"
#define STR_LOADED2		"loaded   "

#define STR_MOVIETOM	"Movie to mount"
#define STR_MOVIELOADED	"Movie loaded successfully. Start the movie from the disc icon<br>under the Video column.</a><hr>Click <a href=\"/mount.ps3/unmount\">here</a> to unmount the movie."

#define STR_XMLRF		"Game list refreshed (<a href=\"" MY_GAMES_XML "\">mygames.xml</a>).<br>Click <a href=\"/restart.ps3\">here</a> to restart your PLAYSTATION®3 system now."

#define STR_STORAGE		"System storage"
#define STR_MEMORY		"Memory"
#define STR_MBFREE		"MB free"
#define STR_KBFREE		"KB free"

#define STR_FANCTRL3	"Fan control:"

#define STR_ENABLED 	"Enabled"
#define STR_DISABLED	"Disabled"

#define STR_FANCH0		"Fan setting changed:"
#define STR_FANCH1		"MAX TEMP: "
#define STR_FANCH2		"FAN SPEED: "
#define STR_FANCH3		"MIN FAN SPEED: "

#define STR_OVERHEAT	"System overheat warning!"
#define STR_OVERHEAT2	"  OVERHEAT DANGER!\nFAN SPEED INCREASED!"

#define STR_NOTFOUND	"Not found!"

#define COVERS_PATH		"http://xmbmods.co/wmlp/covers/%s.JPG"

#else
static int fh;

static char lang_code[3]			= "";

static char STR_TRADBY[120];//		= "<br>";

static char STR_FILES[24];//		= "Files";
static char STR_GAMES[24];//		= "Games";
static char STR_SETUP[24];//		= "Setup";
static char STR_HOME[16];//			= "Home";
static char STR_EJECT[40];//		= "Eject";
static char STR_INSERT[40];//		= "Insert";
static char STR_UNMOUNT[40];//		= "Unmount";
static char STR_COPY[40];//			= "Copy Folder";
static char STR_REFRESH[24];//		= "Refresh";
static char STR_SHUTDOWN[32];//		= "Shutdown";
static char STR_RESTART[32];//		= "Restart";

static char STR_BYTE[8];//			= "b";
static char STR_KILOBYTE[8];//		= "KB";
static char STR_MEGABYTE[8];//		= "MB";
static char STR_GIGABYTE[8];//		= "GB";

static char STR_COPYING[24];//		= "Copying";
static char STR_CPYDEST[24];//		= "Destination";
static char STR_CPYFINISH[48];//	= "Copy Finished!";
static char STR_CPYABORT[48];//		= "Copy aborted!";
static char STR_DELETE[24];//		= "Delete";

static char STR_SCAN2[48];//		= "Scan for content";
static char STR_VIDLG[24];//		= "Video";

static char STR_MANUAL[32];//		= "Manual";

static char STR_SAVE[24];//			= "Save";
static char STR_SETTINGSUPD[192];//	= "Settings updated.<br><br>Click <a href=\"/restart.ps3\">here</a> to restart your PLAYSTATION®3 system.";
static char STR_ERROR[16];//		= "Error!";

static char STR_MYGAMES[32];//		= "webMAN Games";
static char STR_LOADGAMES[72];//	= "Load games with webMAN";

static char STR_FIXING[32];//		= "Fixing";

static char STR_WMSETUP[40];//		= "webMAN Setup";

static char STR_UNMOUNTGAME[56];//	= "Unmount current game";

static char STR_WMSTART[32];//		= "webMAN loaded!";
static char STR_WMUNL[72];//		= "webMAN unloaded!";

static char STR_CFWSYSALRD[72];//	= "CFW Syscalls already disabled";

static char STR_GAMEUM[64];//		= "Game unmounted.";

static char STR_EJECTED[40];//		= "Disc ejected.";
static char STR_LOADED[40];//		= "Disc inserted.";

static char STR_LOADED2[48];//		= "loaded   ";

static char STR_STORAGE[40];//		= "System storage";
static char STR_MEMORY[48];//		= "Memory available";
static char STR_MBFREE[24];//		= "MB free";
static char STR_KBFREE[24];//		= "KB free";

static char STR_FANCTRL3[48];//		= "Fan control:";
static char STR_ENABLED[24];//		= "Enabled";
static char STR_DISABLED[24];//		= "Disabled";

static char STR_FANCH0[64];//		= "Fan setting changed:";
static char STR_FANCH1[48];//		= "MAX TEMP: ";
static char STR_FANCH2[48];//		= "FAN SPEED: ";
static char STR_FANCH3[72];//		= "MIN FAN SPEED: ";

static char STR_NOTFOUND[40];//		= "Not found!";

static char COVERS_PATH[100];//		= "";

#define STR_DISCOBRA	"COBRA TOGGLE"
#define STR_RBGMODE		"RBG MODE TOGGLE"
#define STR_RBGNORM		"NORM MODE TOGGLE"
#define STR_RBGMENU		"MENU TOGGLE"

/*
static u32 get_xreg_value(const char *key, u32 default_value)
{
	int reg = NONE;
	u32 reg_value = default_value;
	u16 off_string, len_data, len_string;
	u64 read, pos, off_val_data;
	CellFsStat stat;
	char string[256];

	if(cellFsOpen("/dev_flash2/etc/xRegistry.sys", CELL_FS_O_RDONLY, &reg, NULL, 0) != CELL_FS_SUCCEEDED || reg == NONE)
	{
		return reg_value;
	}

	cellFsStat("/dev_flash2/etc/xRegistry.sys", &stat);

	u64 entry_name = 0x10000;

	while(true)
	{
	//// Data entries ////
		//unk
		entry_name += 2;

		//relative entry offset
		cellFsLseek(reg, entry_name, 0, &pos);
		cellFsRead(reg, &off_string, 2, &read);
		entry_name += 4;

		//data lenght
		cellFsLseek(reg, entry_name, 0, &pos);
		cellFsRead(reg, &len_data, 2, &read);
		entry_name += 3;

		//data
		off_val_data = entry_name;
		entry_name += len_data + 1;

	//// String Entries ////
		off_string += 0x12;

		//string length
		cellFsLseek(reg, off_string, 0, &pos);
		cellFsRead(reg, &len_string, 2, &read);
		off_string += 3;

		//string
		memset(string, 0, sizeof(string));
		cellFsLseek(reg, off_string, 0, &pos);
		cellFsRead(reg, string, len_string, &read);

		//Find language
		if(IS(string, key))
		{
			cellFsLseek(reg, off_val_data, 0, &pos);
			cellFsRead(reg, &reg_value, 4, &read);
			break;
		}

		if(off_string == 0xCCDD || entry_name >= stat.st_size) break;
	}

	cellFsClose(reg);

	return reg_value;
}
*/

static u32 get_system_language(u8 *lang)
{
	//u32 val_lang = get_xreg_value("/setting/system/language", 1);

	int val_lang = 1;
	xsetting_0AF1F161()->GetSystemLanguage(&val_lang);

	switch(val_lang)
	{
		case 0x0:
			*lang = 4;		//ger;
			break;
		//case 0x1:
		//	lang = 0;		//eng-us
		//	break;
		case 0x2:
			*lang = 3;		//spa
			break;
		case 0x3:
			*lang = 1;		//fre
			break;
		case 0x4:
			*lang = 2;		//ita
			break;
		case 0x5:
			*lang = 5;		//dut //Olandese
			break;
		case 0x6:			//por-por
		case 0x11:			//por-bra
			*lang = 6;
			break;
		case 0x7:
			*lang = 7;		//rus
			break;
		case 0x8:
			*lang = 18;		//jap
			break;
		case 0x9:
			*lang = 17;		//kor
			break;
		case 0xA:
		case 0xB:
			*lang = 19;		//chi-tra / chi-sim
			break;
		//case 0xC:
		//	*lang = 0;		//fin /** missing **/
		//	break;
		//case 0xD:
		//	*lang = 0;		//swe /** missing**/
		//	break;
		case 0xE:
			*lang = 20;		//dan
			break;
		//case 0xF:
		//	*lang = 0;		//nor /** missing**/
		//	break;
		case 0x10:
			*lang = 9;		//pol
			break;
		//case 0x12:
		//	*lang = 0;		//eng-uk
		//	break;
		default:
			*lang = 0;
			break;
	}

	return val_lang;
}

#define CHUNK_SIZE 512
#define GET_NEXT_BYTE  {if(p >= CHUNK_SIZE)  {cellFsRead(fd, buffer, CHUNK_SIZE, &bytes_read); p = 0;} c = buffer[p++], lang_pos++;}

static u8 lang_roms = 0;

static bool language(const char *key_name, char *label, const char *default_str)
{
	u64 bytes_read = 0;
	static size_t p = 0, lang_pos = 0, size = 0;
	u8 c, i, key_len = strlen(key_name);

	sprintf(label, "%s", default_str);

	u8 do_retry = 1;
	char buffer[MAX_LINE_LEN];

 retry:

	if(fh == 0)
	{
		char lang_path[34];

		if(lang_roms)
		{
			sprintf(lang_path, "%s/LANG_EMUS.TXT", WM_LANG_PATH);
			if(file_exists(lang_path) == false)
				sprintf(lang_path, "%s/LANG_ROMS.TXT", WM_LANG_PATH);
		}
		else
		{
			if(webman_config->lang > 22 && (webman_config->lang != 99)) webman_config->lang = 0;

			const char lang_codes[24][2] = {"EN", "FR", "IT", "ES", "DE", "NL", "PT", "RU", "HU", "PL", "GR", "HR", "BG", "IN", "TR", "AR", "CN", "KR", "JP", "ZH", "DK", "CZ", "SK", "XX"};

			i = webman_config->lang; if(i > 23) i = 23;

			sprintf(lang_code, "_%.2s", lang_codes[i]);
			sprintf(lang_path, "%s/LANG%s.TXT", WM_LANG_PATH, lang_code);
		}
		struct CellFsStat buf;

		if(cellFsStat(lang_path, &buf) != CELL_FS_SUCCEEDED) return false; size = (size_t)buf.st_size;

		if(cellFsOpen(lang_path, CELL_FS_O_RDONLY, &fh, NULL, 0) != CELL_FS_SUCCEEDED) return false;

		lang_pos = 0;

		cellFsLseek(fh, lang_pos, CELL_FS_SEEK_SET, NULL); p = CHUNK_SIZE;
	}

	int fd = fh;

	do {
		for(i = 0; i < key_len;)
		{
			{ GET_NEXT_BYTE }

			if(c != key_name[i]) break; i++;

			if(i == key_len)
			{
				if(buffer[p] > ' ') break;

				size_t str_len = 0; u8 copy = 0;

				// set value
				while(lang_pos < size)
				{
					{ GET_NEXT_BYTE }

					if(c == ']' || lang_pos >= size) break;

					if(copy) label[str_len++] = c;

					if(c == '[') copy = 1;
				}

				label[str_len] = NULL;

				return true;
			}
		}

	} while(lang_pos < size);

	if(do_retry) {cellFsClose(fh); do_retry--, fh = lang_pos = 0; goto retry;}

	return true;
}

static void close_language(void)
{
	if(fh) cellFsClose(fh); lang_roms = fh = 0;
}

#undef CHUNK_SIZE
#undef GET_NEXT_BYTE

static char TITLE_XX[12];

static void update_language(void)
{
	fh = 0;

	// initialize variables with default values
	sprintf(STR_SETTINGSUPD, "%s%s", "Settings updated.<br>", "<br>Click <a href=\"/restart.ps3\">here</a> to restart your PLAYSTATION®3 system.");

	*COVERS_PATH = NULL;

	if(language("STR_TRADBY", STR_TRADBY, "<br>"))
	{
		language("STR_FILES", STR_FILES, "Files");
		language("STR_GAMES", STR_GAMES, "Games");
		language("STR_SETUP", STR_SETUP, "Setup");
		language("STR_HOME", STR_HOME, "Home");
		language("STR_EJECT", STR_EJECT, "Eject");
		language("STR_INSERT", STR_INSERT, "Insert");
		language("STR_UNMOUNT", STR_UNMOUNT, "Unmount");
		language("STR_COPY", STR_COPY, "Copy Folder");
		language("STR_REFRESH", STR_REFRESH, "Refresh");
		language("STR_SHUTDOWN", STR_SHUTDOWN, "Shutdown");
		language("STR_RESTART", STR_RESTART, "Restart");

		language("STR_BYTE", STR_BYTE, "b");
		language("STR_KILOBYTE", STR_KILOBYTE, "KB");
		language("STR_MEGABYTE", STR_MEGABYTE, "MB");
		language("STR_GIGABYTE", STR_GIGABYTE, "GB");

		language("STR_COPYING", STR_COPYING, "Copying");
		language("STR_CPYDEST", STR_CPYDEST, "Destination");
		language("STR_CPYFINISH", STR_CPYFINISH, "Copy Finished!");
		language("STR_CPYABORT", STR_CPYABORT, "Copy aborted!");
		language("STR_DELETE", STR_DELETE, "Delete");

		language("STR_SCAN2", STR_SCAN2, "Scan for content");
		language("STR_VIDLG", STR_VIDLG, "Video");

		language("STR_MANUAL", STR_MANUAL, "Manual");

		language("STR_SAVE", STR_SAVE, "Save");
		language("STR_SETTINGSUPD", STR_SETTINGSUPD, STR_SETTINGSUPD);
		language("STR_ERROR", STR_ERROR, "Error!");

		language("STR_MYGAMES", STR_MYGAMES, WM_APPNAME " Games");
		language("STR_LOADGAMES", STR_LOADGAMES, "Load games with " WM_APPNAME);

		language("STR_FIXING", STR_FIXING, "Fixing");

		language("STR_WMSETUP", STR_WMSETUP, WM_APPNAME " Setup");

		language("STR_UNMOUNTGAME", STR_UNMOUNTGAME, "Unmount current game");

		language("STR_WMSTART", STR_WMSTART, WM_APPNAME " loaded!");
		language("STR_WMUNL", STR_WMUNL, WM_APPNAME " unloaded!");
		language("STR_CFWSYSALRD", STR_CFWSYSALRD, "CFW Syscalls already disabled");

		language("STR_GAMEUM", STR_GAMEUM, "Game unmounted.");

		language("STR_EJECTED", STR_EJECTED, "Disc ejected.");
		language("STR_LOADED", STR_LOADED, "Disc inserted.");
		language("STR_LOADED2", STR_LOADED2, "loaded   ");

		language("STR_STORAGE", STR_STORAGE, "System storage");
		language("STR_MEMORY", STR_MEMORY, "Memory available");
		language("STR_MBFREE", STR_MBFREE, "MB free");
		language("STR_KBFREE", STR_KBFREE, "KB free");

		language("STR_FANCTRL3", STR_FANCTRL3, "Fan control:");
		language("STR_ENABLED", STR_ENABLED, "Enabled");
		language("STR_DISABLED", STR_DISABLED, "Disabled");

		language("STR_FANCH0", STR_FANCH0, "Fan setting changed:");
		language("STR_FANCH1", STR_FANCH1, "MAX TEMP: ");
		language("STR_FANCH2", STR_FANCH2, "FAN SPEED: ");
		language("STR_FANCH3", STR_FANCH3, "MIN FAN SPEED: ");

		language("STR_NOTFOUND", STR_NOTFOUND, "Not found!");

		language("COVERS_PATH", COVERS_PATH, COVERS_PATH);
		language("IP_ADDRESS", local_ip, local_ip);
		language("SEARCH_URL", search_url, "http://google.com/search?q=");

		strcpy(STR_TRADBY, "<br>");
/*
#ifdef COBRA_ONLY
		language("STR_DISCOBRA", STR_DISCOBRA);
#endif
#ifdef REX_ONLY
		language("STR_RBGMODE", STR_RBGMODE);
		language("STR_RBGNORM", STR_RBGNORM);
		language("STR_RBGMENU", STR_RBGMENU);
#endif
*/
	}

	close_language();

	*html_base_path = NULL;

	// TITLE_XX

	*TITLE_XX = NULL; u8 id = 99, lang = webman_config->lang;

	if(lang ==  1) id = 2;  // fr
	if(lang ==  2) id = 5;  // it
	if(lang ==  3) id = 3;  // es
	if(lang ==  4) id = 4;  // de
    if(lang ==  5) id = 6;  // nl
	if(lang ==  6) id = 7;  // pt
	if(lang ==  7) id = 8;  // ru
	if(lang ==  9) id = 16; // pl
	if(lang == 14) id = 19; // tr
	if(lang == 16) id = 11; // zh
	if(lang == 17) id = 9;  // ko
	if(lang == 18) id = 0;  // jp
	if(lang == 19) id = 10; // ch
	if(lang == 20) id = 14; // da
	if(  id == 99) return;

	sprintf(TITLE_XX, "TITLE_%02i", id);
}
#endif //#ifndef ENGLISH_ONLY
