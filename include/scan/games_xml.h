// combos.h
static u8 show_persistent_popup = 0;

#define AVG_ITEM_SIZE			360

// XML Tags:
// X = XMBML
// V = View
// T = Table
// A = Attributes
// P = Pair
// Q = Query
// I = Item or Items

#define XML_QUERY				"<Q class=\"type:x-xmb/folder-pixmap\" key=\""
#define SRC(key, src)			key "\" attr=\"" key "\" src=\"" src "\"/>"

#define XML_ITEM				"<I class=\"type:x-xmb/module-action\" key=\""
#define ADD_XMB_ITEM(key)		key "\" attr=\"" key "\"/>"

#define XML_HEADER				"<?xml version=\"1.0\" encoding=\"UTF-8\"?><X version=\"1.0\">"
#define XML_PAIR(key, value)	"<P key=\"" key "\"><>" value "</></P>"
#define XML_BEGIN_ITEMS 		"</A><I>"
#define XML_END_ITEMS			"</I></V>"
#define XML_END_OF_FILE 		"</I></V></X>\r\n"

#define XAI_LINK_INC			"<T key=\"inc\">" XML_PAIR("module_name", WM_PROXY_SPRX) XML_PAIR("bar_action", "none") "</T>"
#define WEB_LINK_INC			"<T key=\"inc\">" XML_PAIR("module_name", "webrender_plugin") "</T>"
#define WEB_LINK_PAIR			XML_PAIR("module_name", "webrender_plugin")

#define STR_NOITEM_PAIR 		XML_PAIR("str_noitem", "msg_error_no_content") "</T>"

#define XML_KEY_LEN				7 /* 1 + 6 = Group + 6 chars */

#define XMB_GROUPS				(!webman_config->nogrp)
#define ADD_SETUP				(!webman_config->nosetup)

#define QUERY_APP_HOME_ICON 	"<T key=\"gamedebug\">" \
								XML_PAIR("icon_rsc","tex_album_icon") \
								XML_PAIR("title_rsc","msg_tool_app_home_ps3_game") \
								XML_PAIR("child","segment") \
								"</T></A><I>" \
								"<Q class=\"type:x-xcb/game-debug\" key=\"game_debug\" attr=\"game_debug\"/>"

#define QUERY_SETUP_ENGLISH 	SRC("setup", \
									"xmb://localhost" XMBMANPLS_PATH "/FEATURES/webMAN.xml#seg_webman_links_items")

#define QUERY_SETUP_LOCALIZED	SRC("setup", \
									"xmb://localhost" XMBMANPLS_PATH "/FEATURES/webMAN%s.xml#seg_webman_links_items")

#define QUERY_PS2_LAUNCHER		SRC("ps2_classic_launcher", \
									"xcb://0/query?limit=1&cond=Ae+Game:Game.titleId PS2U10000")

#define QUERY_PSP_LAUNCHERS 	SRC("cobra_psp_launcher", \
									"xcb://0/query?cond=AGL+Game:Game.titleId " PSP_LAUNCHER_REMASTERS_ID \
																			" " PSP_LAUNCHER_MINIS_ID)

static u16 item_count[6];
static vu16 games_found = 0;

typedef struct
{
	char value[1 + XML_KEY_LEN + 4];
} t_keys;

#define ROM (1<<6)

enum xmb_groups
{
	gPS3 = 0,
	gPSX = 1,
	gPS2 = 2,
	gPSP = 3,
	gDVD = 4,
	gROM = 5,
};

static bool scanning_roms = false;

#ifndef LITE_EDITION
static bool use_imgfont = false;
#endif

#ifdef COBRA_ONLY
static void sys_map_path2(const char *path1, const char *path2)
{
	#ifdef PS3MAPI
	if(is_syscall_disabled(35))
		sys_map_path(path1, path2);
	else
		{system_call_2(35, (u64)(u32)path1, (u64)(u32)path2);}
	#else
	{system_call_2(35, (u64)(u32)path1, (u64)(u32)path2);}
	#endif
}

static void map_patched_modules(void)
{
	// redirect to patched game_ext_plugin.sprx required for gameboot animation & sounds
	if(file_exists(NEW_GAME_EXT_PATH))
		sys_map_path(ORG_GAME_EXT_PATH, NEW_GAME_EXT_PATH);

	// redirect to patched libaudio.sprx
	if(file_exists(NEW_LIBAUDIO_PATH))
		sys_map_path(ORG_LIBAUDIO_PATH, NEW_LIBAUDIO_PATH);

	// redirect to patched explore_plugin.sprx
	if(file_exists(NEW_EXPLORE_PLUGIN_PATH))
		sys_map_path(ORG_EXPLORE_PLUGIN_PATH, NEW_EXPLORE_PLUGIN_PATH);

	#ifndef LITE_EDITION
	use_imgfont = (file_ssize(IMAGEFONT_PATH) > 900000);
	#endif
}

static void remap_ps1bios(bool external)
{
	sys_map_path2(ORG_PS1_ROM_PATH, external && file_exists(NEW_PS1_ROM_PATH) ? NEW_PS1_ROM_PATH : NULL);
}

static void apply_remaps(void)
{
	disable_map_path(false);

 #ifdef WM_PROXY_SPRX
	sys_map_path2(VSH_MODULE_DIR WM_PROXY_SPRX ".sprx", file_exists(WM_PROXY) ? WM_PROXY : NULL);
 #endif

	remap_ps1bios(webman_config->ps1rom);

	//if(payload_ps3hen)
	{
		sys_map_path(FB_XML, FB_HEN_XML);
	//	sys_map_path(HEN_HFW_SETTINGS, "/dev_hdd0/hen/xml/hfw_settings.xml");
	}

	map_patched_modules();

	// redirect firmware update on BD disc to empty folder
	sys_map_path("/dev_bdvd/PS3_UPDATE", SYSMAP_EMPTY_DIR);
}
#endif

static void make_fb_xml(void)
{
	sys_addr_t sysmem = sys_mem_allocate(_64KB_);
	if(sysmem)
	{
		cellFsUnlink(_FB_XML);

		const char *fb_xml = FB_XML;
		#ifdef COBRA_ONLY
		//if(payload_ps3hen)
		{
			fb_xml = FB_HEN_XML;
			sys_map_path(FB_XML, fb_xml);
		}
		#endif

		if(!(webman_config->root))
		{
			char *myxml = (char*)sysmem;
			u16 size = sprintf(myxml, "%s" // XML_HEADER
									  "<V id=\"seg_fb\">"
									  "<A>"
										"<T key=\"mgames\">"
										"%s" // XML_PAIR( icon / icon_rsc )
										XML_PAIR("icon_notation","WNT_XmbItemSavePS3")
										XML_PAIR("ingame","disable")
										XML_PAIR("title","%s%s") // STR_MYGAMES, SUFIX2(profile)
										XML_PAIR("info","%s")	 // STR_LOADGAMES
										"</T>",
										XML_HEADER,
										file_exists(WM_ICONS_PATH "/icon_wm_root.png") ?
											XML_PAIR("icon", WM_ICONS_PATH "/icon_wm_root.png") :
											XML_PAIR("icon_rsc", "item_tex_ps3util"),
										STR_MYGAMES, SUFIX2(profile),
										STR_LOADGAMES);

			if(!(webman_config->app_home) && !is_app_home_onxmb())
			{
				has_app_home = !(webman_config->root);
				size += strcopy(myxml + size, QUERY_APP_HOME_ICON);
			}
			else
				size += strcopy(myxml + size, XML_BEGIN_ITEMS);

			size += sprintf(myxml + size, "%s" SRC("mgames", "xmb://localhost%s#seg_mygames") "%s", // MY_GAMES_XML
										  XML_QUERY, MY_GAMES_XML, XML_END_OF_FILE);

			save_file(fb_xml, myxml, size);
			_memset(myxml, size);
		}

		sys_memory_free(sysmem);

		//if(payload_ps3hen && IS_ON_XMB)
		{
			if(get_explore_interface())
			{
				exec_xmb_command2("reload_category %s", "game");
				exec_xmb_command2("reload_category %s", "network");
			}
		}
	}
}

#ifdef MOUNT_ROMS
static void build_roms_xml(char *sysmem_buf, char *title, char *table_xml, u16 roms_count[], const char *roms_path[], u8 roms_path_count)
{
	t_string myxml; _alloc(&myxml, sysmem_buf);
	_concat2(&myxml, XML_HEADER, "<V id=\"seg_wm_rom_items\"><A>");

	#ifndef ENGLISH_ONLY
	close_language(); lang_roms = true;
	#endif

	u32 t_count = 0;

	// ---- Add roms categories
	for(u8 i = 0; i < roms_path_count; i++)
	{
		if(roms_path[i] && (roms_count[i] > 0))
		{
			#ifndef ENGLISH_ONLY
			language(roms_path[i], title, roms_path[i]);
			#endif

			sprintf(table_xml, "<T key=\"%s\">"
								XML_PAIR("icon%s", "%s")
								XML_PAIR("title","%s")
								XML_PAIR("info","%'i %s")
								"</T>",
								roms_path[i],
								covers_exist[7] ? "" : "_rsc",
								covers_exist[7] ? WM_ICONS_PATH "/icon_wm_album_emu.png" : "item_tex_ps3util",
								#ifndef ENGLISH_ONLY
								fh ? title:
								#endif
								roms_path[i],
								roms_count[i], (roms_count[i] == 1) ? "ROM" : "ROMS"); _concat(&myxml, table_xml);
		}
	}

	#ifndef ENGLISH_ONLY
	close_language();
	#endif

	// ---- Add roms queries
	_concat(&myxml, XML_BEGIN_ITEMS);

	for(u8 i = 0; i < roms_path_count; i++)
	{
		if(roms_path[i] && (roms_count[i] > 0))
		{
			sprintf(table_xml, SRC("%s", "xmb://localhost%s/%s%s.xml#seg_wm_rom_%s"), roms_path[i], roms_path[i], HTML_BASE_PATH, "ROMS_", roms_path[i], roms_path[i]);
			_concat2(&myxml, XML_QUERY, table_xml);
			t_count += roms_count[i];
		}
	}

	_concat(&myxml, XML_END_OF_FILE);

	save_file(HTML_BASE_PATH "/ROMS.xml", myxml.str, myxml.size);

	// patch mygames.xml
	if(read_file(MY_GAMES_XML, table_xml, KB, 200) == KB)
	{
		char *pos = strstr(table_xml, "         ROM");
		if(pos)
		{
			char *roms_count = title;
			sprintf(roms_count, "%'8i", t_count); memcpy(pos, roms_count, 8);
			patch_file(MY_GAMES_XML, table_xml, 200, KB);
		}
	}
}
#endif

#ifndef LITE_EDITION
static void add_tag(char *tags, u16 code)
{
	char tag[5] = {' ', 0xEF, (code >> 8), (code & 0xFF), 0};
	strcat(tags, tag);
}
#endif

static int add_info(char *table_xml, char *folder_name, u8 roms_index, char *filename, char *title_id, u8 f0, u8 f1, u8 s)
{
	char tags[20]; *tags = '\0'; u8 info = webman_config->info & 0xF;

	#ifndef LITE_EDITION
	if(use_imgfont && (webman_config->info & INFO_TAGS))
	{
		// TAG#1
		if(IS_HDD0)  add_tag(tags, 0x918F); else // HDD
		if(BETWEEN(1, f0, 6)) add_tag(tags, 0x919A); else // USB
		if(IS_NTFS)  add_tag(tags, 0x92B3); else // NTFS
		if(IS_NET)   add_tag(tags, 0x9788); else // NET
		if(f0 == 13) add_tag(tags, 0x90BB); else // SD
		if(f0 == 14) add_tag(tags, 0x90BA); else // MS
		if(f0 == 15) add_tag(tags, 0x90B9); 	 // CF

		// TAG#2
		if(f1 <= id_PS3ISO)   add_tag(tags, 0x95A8); else // PS3
		if(f1 == id_PS2ISO)   add_tag(tags, 0x95A7); else // PS2
		if(f1 == id_PSXISO)   add_tag(tags, 0x95A6); else // PS1
		if(f1 == id_PSXGAMES) add_tag(tags, 0x95A6); else // PS1
		if(f1 == id_PSPISO)	  add_tag(tags, 0x95B5); else // PSP
		if(f1 == id_ROMS)	  add_tag(tags, 0x95BC); else // ROM
		if(f1 == id_GAMEI)	  add_tag(tags, 0x96B1); else // GAMEI
		if(f1 == id_NPDRM)	  add_tag(tags, 0x96B0); else // GAME
		if(f1 == id_BDISO)	  add_tag(tags, 0x90BC); else // BD
		if(f1 == id_DVDISO)	  add_tag(tags, 0x90BF);	  // DVD

		#ifdef MOUNT_ROMS
		if(f1 == id_ROMS)
		{
			// TAG#3
//			if(roms_index == 0)             add_tag(tags, 0x95BC); else // 2048
//			if(roms_index == 1)             add_tag(tags, 0x95BC); else // CAP32
			if(BETWEEN( 2, roms_index,  6)) add_tag(tags, 0x97BE); else // MAME
			if(BETWEEN( 7, roms_index,  9)) add_tag(tags, 0x97BD); else // FBA
			if(BETWEEN(10, roms_index, 19)) add_tag(tags, 0x97BA); else // ATARI
//			if(roms_index == 20)            add_tag(tags, 0x95BC); else // CANNONBALL
//			if(roms_index == 21)            add_tag(tags, 0x95BC); else // NXENGINE
//			if(roms_index == 22)            add_tag(tags, 0x95BC); else // COLECO
			if(BETWEEN(23, roms_index, 35)) add_tag(tags, 0x9590); else // AMIGA / VICE
//			if(roms_index == 36)            add_tag(tags, 0x95BC); else // DOSBOX
//			if(roms_index == 37)            add_tag(tags, 0x95BC); else // GME
//			if(roms_index == 38)            add_tag(tags, 0x95BC); else // GW
			if(roms_index == 39)            add_tag(tags, 0x9882); else // DOOM
			if(roms_index == 40)            add_tag(tags, 0x9883); else // QUAKE
//			if(roms_index == 41)            add_tag(tags, 0x95BC); else // JAVAME
//			if(roms_index == 42)            add_tag(tags, 0x95BC); else // JUMP
//			if(roms_index == 43)            add_tag(tags, 0x95BC); else // O2EM
//			if(roms_index == 44)            add_tag(tags, 0x95BC); else // INTV
			if(BETWEEN(45, roms_index, 48)) add_tag(tags, 0x959E); else // MSX
			if(BETWEEN(49, roms_index, 51)) add_tag(tags, 0x95A0); else // NEO
			if(BETWEEN(52, roms_index, 55)) add_tag(tags, 0x95A4); else // PCE
			if(BETWEEN(56, roms_index, 57)) add_tag(tags, 0x95A3); else // NGP / NGPC
			if(BETWEEN(58, roms_index, 61)) add_tag(tags, 0x95A1); else // NES
			if(roms_index == 63)            add_tag(tags, 0x9593); else // GBC
			if(BETWEEN(62, roms_index, 65)) add_tag(tags, 0x9591); else // GB
			if(BETWEEN(66, roms_index, 68)) add_tag(tags, 0x9592); else // GBA
//			if(roms_index == 69)            add_tag(tags, 0x95BC); else // VBOY
//			if(roms_index == 70)            add_tag(tags, 0x95BC); else // PALM
			if(roms_index == 71)            add_tag(tags, 0x95A6); else // PS1
			if(roms_index == 72)            add_tag(tags, 0x95A7); else // PS2
			if(roms_index == 73)            add_tag(tags, 0x95A8); else // PS3
			if(roms_index == 74)            add_tag(tags, 0x95B5); else // PSP
//			if(roms_index == 75)            add_tag(tags, 0x95BC); else // POKEMINI
//			if(roms_index == 76)            add_tag(tags, 0x95BC); else // SCUMMVM
			if(roms_index == 79)            add_tag(tags, 0x95BE); else // SEGACD
			if(BETWEEN(77, roms_index, 81)) add_tag(tags, 0x9594); else // GEN
			if(BETWEEN(82, roms_index, 83)) add_tag(tags, 0x959D); else // GG
			if(roms_index == 84)            add_tag(tags, 0x95BF); else // MASTER
			if(roms_index == 85)            add_tag(tags, 0x9594); else // PICO
//			if(roms_index == 86)            add_tag(tags, 0x95BC); else // SG1000
//			if(BETWEEN(87, roms_index, 88)) add_tag(tags, 0x95BC); else // Sinclair
			if(BETWEEN(89, roms_index, 94)) add_tag(tags, 0x9680); else // SNES
//			if(roms_index == 95)            add_tag(tags, 0x95BC); else // THEODORE
//			if(roms_index == 96)            add_tag(tags, 0x95BC); else // VECX
			if(roms_index == 97)            add_tag(tags, 0x9682); else // WSWAM
			if(roms_index == 98)            add_tag(tags, 0x9683); else // WSWAMC
											add_tag(tags, 0x95BC);      // ROM
		}
		#endif

		// TAG#4
		if(f1 == 0) add_tag(tags, 0x96B2); else // GAMES
		if(f1 == 1) add_tag(tags, 0x97B2); else // GAMEZ
		{
			const char *ext = get_ext(filename);
			if(_IS(ext, ".bin")) add_tag(tags, 0x91BE); else // BIN
			if(_IS(ext, ".iso")) add_tag(tags, 0x9288); else // ISO
			if(_IS(ext, ".img")) add_tag(tags, 0x9287); else // IMG
			if(_IS(ext, ".mdf")) add_tag(tags, 0x928A); else // MDF
			if(_IS(ext, ".zip")) add_tag(tags, 0x90B6); else // ZIP
			if(_IS(ext, ".rar")) add_tag(tags, 0x90B5); else // RAR
			if(_IS(ext, ".7z"))  add_tag(tags, 0x90B0); else // 7Z
			if(IS_NTFS)
			{
				if(_IS(ext, ".pkg")) add_tag(tags, 0x90B3); else // PKG
				if(_IS(ext, ".avi")) add_tag(tags, 0x93A6); else // AVI
				if(_IS(ext, ".mp4")) add_tag(tags, 0x93AE); else // MP4
				if(_IS(ext, ".mp3")) add_tag(tags, 0x93AD); else // MP3
				if(_IS(ext, ".p3t")) add_tag(tags, 0x96A4);		 // P3T
			}
		}
	}
	#endif

	int len = 0;

	// info level: 0=Path, 1=Path | titleid, 2=titleid | drive, 3=none, 0x10 = tags, 0x20 = version
	if(info <= INFO_PATH_ID)
	{
		if((info == INFO_PATH_ID) & HAS_TITLE_ID) {strcat(folder_name, " | "); strcat(folder_name, title_id);}
		len = sprintf(table_xml, XML_PAIR("info","%s/%s%s%s"), drives[f0] + s, (f1 == id_NPDRM) ? "game" : paths[f1], folder_name, tags);
	}
	else if(info == INFO_ID)
	{
		if(HAS_TITLE_ID)
			len = sprintf(table_xml, XML_PAIR("info","%s%s"), title_id, tags);
		else
			len = sprintf(table_xml, XML_PAIR("info","%s"), tags);
	}
	else if(webman_config->info & INFO_TAGS_ONLY)
	{
		len = sprintf(table_xml, XML_PAIR("info","%s"), tags);
	}

	strcat(table_xml, "</T>");
	return (len + 4);
}

static char *eject_table(char *buffer, char *table_xml)
{
	#ifndef ENGLISH_ONLY
	char *STR_EJECTDISC = buffer; //[32];//	= "Eject Disc";
	language("STR_EJECTDISC", STR_EJECTDISC, "Eject Disc");
	#endif

	sprintf(table_xml, "<T key=\"eject\" include=\"inc\">"
						XML_PAIR("icon","%s")
						XML_PAIR("title","%s")
						XML_PAIR("info","%s") //"%s"
						XML_PAIR("module_action","0/mount_ps3/unmount") "</T>",
						wm_icons[11], STR_EJECTDISC, STR_UNMOUNTGAME/*, proxy_include, localhost*/);
	return table_xml;
}

static char *setup_table(char *buffer, char *table_xml, bool add_xmbm_plus)
{
	#ifndef ENGLISH_ONLY
	char *STR_WMSETUP2 = buffer; //[56];//	= "Setup webMAN options";
	language("STR_WMSETUP2", STR_WMSETUP2, "Setup webMAN options");
	#endif
	sprintf(table_xml, "<T key=\"%s\">"
						XML_PAIR("icon","%s")
						XML_PAIR("title","%s")
						XML_PAIR("info","%s") "%s",
						"setup",
						add_xmbm_plus ? XMBMANPLS_PATH "/IMAGES/multiman.png" : wm_icons[10],
						STR_WMSETUP, STR_WMSETUP2, WEB_LINK_PAIR);
	return table_xml;
}

static char *ps2launcher_table(char *buffer, char *table_xml)
{
	#ifndef ENGLISH_ONLY
	char *STR_LAUNCHPS2 =  buffer; //[48];//	= "Launch PS2 Classic";
	language("STR_LAUNCHPS2", STR_LAUNCHPS2, "Launch PS2 Classic");
	#endif
	sprintf(table_xml, "<T key=\"%s\">"
						XML_PAIR("icon","%s")
						XML_PAIR("title","%s")
						XML_PAIR("info","%s") "%s",
						"ps2_classic_launcher", PS2_CLASSIC_ISO_ICON, "PS2 Classic Launcher",
						STR_LAUNCHPS2, STR_NOITEM_PAIR);
	return table_xml;
}

#ifdef COBRA_ONLY
static char *psplauncher_table(char *buffer, char *table_xml)
{
	#ifndef ENGLISH_ONLY
	char *STR_LAUNCHPSP =  buffer; //[144];//	= "Launch PSP ISO mounted through webMAN or mmCM";
	language("STR_LAUNCHPSP", STR_LAUNCHPSP, "Launch PSP ISO mounted through webMAN or mmCM");
	#endif
	char icon[40]; concat2(icon, isDir(PSP_LAUNCHER_REMASTERS) ? PSP_LAUNCHER_REMASTERS : PSP_LAUNCHER_MINIS, "/ICON0.PNG");
	sprintf(table_xml, "<T key=\"%s\">"
						XML_PAIR("icon","%s")
						XML_PAIR("title","%s")
						XML_PAIR("info","%s") "%s",
						"cobra_psp_launcher", icon, "PSP Launcher",
						STR_LAUNCHPSP, STR_NOITEM_PAIR);
	return table_xml;
}

static char *videos_table(char *table_xml)
{
	sprintf(table_xml, "<T key=\"rx_video\">"
						XML_PAIR("icon","%s")
						XML_PAIR("title","%s")
						XML_PAIR("child","segment") "%s",
						wm_icons[gDVD], STR_VIDLG, STR_NOITEM_PAIR);
	return table_xml;
}
#endif

static char *custom_table(char *buffer, char *table_xml)
{
	#ifndef ENGLISH_ONLY
	char *STR_CUSTOMXML = buffer;
	language("STR_CUSTOMXML", STR_CUSTOMXML, "XML");
	#else
	const char *STR_CUSTOMXML = "XML";
	#endif

	sprintf(table_xml, "<T key=\"wm_custom\">"
						XML_PAIR("icon_rsc","item_tex_ps3util")
						XML_PAIR("title","%s") "</T>",
						STR_CUSTOMXML);
	return table_xml;
}

static bool add_custom_xml(char *query_xmb)
{
	char *custom_xml = query_xmb + 800;
	for(u8 d = 0; d < MAX_DRIVES; d++)
	{
		if(d == NET) d = NTFS + 1;
		sprintf(custom_xml,  "%s/wm_custom.xml", drives[d]);
		if(file_exists(custom_xml))
		{
			sprintf(query_xmb, "%s" SRC("wm_custom", "xmb://localhost%s#wm_root"), XML_QUERY, custom_xml);
			return true;
		}
	}
	return false;
}

static void add_group_tables(char *buffer, char *table_xml, t_string *myxml)
{
	#ifndef ENGLISH_ONLY
	char *STR_PS3FORMAT = buffer; //[40];//	= "PS3 format games";
	language("STR_PS3FORMAT", STR_PS3FORMAT, "PS3 format games");
	#endif
	if( !(webman_config->cmask & PS3)) {sprintf(table_xml, "<T key=\"wm_%s\">"
												XML_PAIR("icon","%s")
												XML_PAIR("title","%s")
												XML_PAIR("info","%'i %s"),
												"ps3", wm_icons[gPS3], "PLAYSTATION\xC2\xAE\x33", item_count[gPS3], STR_PS3FORMAT);
										 _concat2(myxml, table_xml, STR_NOITEM_PAIR);}
	#ifdef MOUNT_ROMS
	if(webman_config->roms)
	{
		#ifndef ENGLISH_ONLY
		char *pos = strstr(buffer, "PS3"); if(pos) memcpy(pos, "ROM", 3); else *buffer = NULL;
		#else
		*buffer = NULL;
		#endif
										sprintf(table_xml, "<T key=\"wm_rom\">"
												XML_PAIR("icon%s", "%s")
												XML_PAIR("title","ROMS")
												XML_PAIR("info","         %s"),
												covers_exist[7] ? "" : "_rsc",
												covers_exist[7] ? WM_ICONS_PATH "/icon_wm_album_emu.png" : "item_tex_ps3util",
												buffer);
										_concat2(myxml, table_xml, STR_NOITEM_PAIR);
	}
	#endif
	#ifndef ENGLISH_ONLY
	char *STR_PS2FORMAT = buffer; //[48];//	= "PS2 format games";
	language("STR_PS2FORMAT", STR_PS2FORMAT, "PS2 format games");
	#endif
	if( !(webman_config->cmask & PS2)) {sprintf(table_xml, "<T key=\"wm_%s\">"
												XML_PAIR("icon","%s")
												XML_PAIR("title","%s")
												XML_PAIR("info","%'i %s"),
												"ps2", wm_icons[gPS2], "PLAYSTATION\xC2\xAE\x32", item_count[gPS2], STR_PS2FORMAT);
										_concat2(myxml, table_xml, STR_NOITEM_PAIR);}
 #ifdef COBRA_ONLY

	#ifndef ENGLISH_ONLY
	char *STR_PS1FORMAT = buffer; //[48];//	= "PSOne format games";
	language("STR_PS1FORMAT", STR_PS1FORMAT, "PSOne format games");
	#endif
	if( !(webman_config->cmask & PS1)) {sprintf(table_xml, "<T key=\"wm_%s\">"
												XML_PAIR("icon","%s")
												XML_PAIR("title","%s")
												XML_PAIR("info","%'i %s"),
												"psx", wm_icons[gPSX], "PLAYSTATION\xC2\xAE", item_count[gPSX], STR_PS1FORMAT);
										_concat2(myxml, table_xml, STR_NOITEM_PAIR);}
	#ifndef ENGLISH_ONLY
	char *STR_PSPFORMAT = buffer; //[48];//	= "PSP\xE2\x84\xA2 format games";
	language("STR_PSPFORMAT", STR_PSPFORMAT, "PSP\xE2\x84\xA2 format games");
	#endif
	if( !(webman_config->cmask & PSP)) {sprintf(table_xml, "<T key=\"wm_%s\">"
												XML_PAIR("icon","%s")
												XML_PAIR("title","%s")
												XML_PAIR("info","%'i %s"),
												"psp", wm_icons[gPSP], "PLAYSTATION\xC2\xAEPORTABLE", item_count[gPSP], STR_PSPFORMAT);
										_concat2(myxml, table_xml, STR_NOITEM_PAIR);}
	#ifndef ENGLISH_ONLY
	char *STR_VIDFORMAT = buffer; //[56];//	= "Blu-ray\xE2\x84\xA2 and DVD";
	char *STR_VIDEO = buffer + 100; //[40];//	= "Video content";

	language("STR_VIDFORMAT", STR_VIDFORMAT, "Blu-ray\xE2\x84\xA2 and DVD");
	language("STR_VIDEO", STR_VIDEO, "Video content");
	#endif
	if( !(webman_config->cmask & DVD) ||
		!(webman_config->cmask & BLU)) {sprintf(table_xml, "<T key=\"wm_%s\">"
												XML_PAIR("icon","%s")
												XML_PAIR("title","%s")
												XML_PAIR("info","%'i %s"),
												"dvd", wm_icons[gDVD], STR_VIDFORMAT, item_count[gDVD], STR_VIDEO);
										_concat2(myxml, table_xml, STR_NOITEM_PAIR);}
 #endif
}

static bool add_xmb_entry(u8 f0, u8 f1, u32 table_size, const char *table_xml, char *title, char *skey, u16 key, t_string *myxml_ps3, t_string *myxml_ps2, t_string *myxml_psx, t_string *myxml_psp, t_string *myxml_dvd, char *entry_name, u8 subfolder)
{
	set_sort_key(skey, title, key, subfolder, f1);

	if( !scanning_roms && XMB_GROUPS )
	{
	#ifdef COBRA_ONLY
		const char *ext = get_ext(entry_name);
		if((IS_PS3_TYPE || (IS_NTFS && IS(ext, ".ntfs[PS3ISO]"))) && ((myxml_ps3->size + table_size) < BUFFER_SIZE_PS3))
		{_concat(myxml_ps3, table_xml); *skey=PS3, ++item_count[gPS3];}
		else
		if((IS_PS2ISO || (IS_NTFS && IS(ext, ".ntfs[PS2ISO]"))) && ((myxml_ps2->size + table_size) < BUFFER_SIZE_PS2))
		{_concat(myxml_ps2, table_xml); *skey=PS2, ++item_count[gPS2];}
		else
		if((IS_PSXISO || (IS_NTFS && IS(ext, ".ntfs[PSXISO]"))) && ((myxml_psx->size + table_size) < BUFFER_SIZE_PSX))
		{_concat(myxml_psx, table_xml); *skey=PS1, ++item_count[gPSX];}
		else
		if((IS_PSPISO || (IS_NTFS && IS(ext, ".ntfs[PSPISO]"))) && ((myxml_psp->size + table_size) < BUFFER_SIZE_PSP))
		{_concat(myxml_psp, table_xml); *skey=PSP, ++item_count[gPSP];}
		else
		if((IS_BDISO || IS_DVDISO || (IS_NTFS && (IS(ext, ".ntfs[DVDISO]") || IS(ext, ".ntfs[BDISO]") || IS(ext, ".ntfs[BDFILE]")))) && ((myxml_dvd->size + table_size) < BUFFER_SIZE_DVD))
		{_concat(myxml_dvd, table_xml); *skey=BLU, ++item_count[gDVD];}
	#else
		if(IS_PS3_TYPE && ((myxml_ps3->size + table_size) < BUFFER_SIZE_PS3))
		{_concat(myxml_ps3, table_xml); *skey=PS3, ++item_count[gPS3];}
		else
		if(IS_PS2ISO && ((myxml_ps2->size + table_size) < BUFFER_SIZE_PS2))
		{_concat(myxml_ps2, table_xml); *skey=PS2, ++item_count[gPS2];}
	#endif
		else
			return (false);
	}
	else
	{
		if((myxml_ps3->size + table_size) < BUFFER_SIZE_PS3)
			{_concat(myxml_ps3, table_xml); ++item_count[gPS3];}
		else
			return (false);
	}

	++games_found; return (true);
}

static int qcompare_xml(const void *a, const void *b)
{
	return memcmp((const char*)a, (const char*)b, XML_KEY_LEN);
}

static bool scan_mygames_xml(u64 conn_s_p)
{
	if(conn_s_p == START_DAEMON)
	{
		if(webman_config->root) ;

		// skip scan if rebooting or if scan on startup is disabled
		else if(webman_config->refr || from_reboot)
		{
			if(file_exists(MY_GAMES_XML))
			{
				make_fb_xml();
				return true; // skip refresh xml & mount autoboot
			}
		}

		make_fb_xml();

		// skip scan on startup if L2+R2 is pressed,
		if(is_pressed(CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2)) return true;

		// start a new thread for refresh xml content at start up
		if(!webman_config->refr || not_exists(MY_GAMES_XML))
		{
			sys_ppu_thread_t t_id;
			sys_ppu_thread_create(&t_id, start_www, REFRESH_CONTENT, THREAD_PRIO, THREAD_STACK_SIZE_WEB_CLIENT, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_CMD);
		}

		return true; // mount autoboot & refresh xml
	}

	#ifdef COBRA_ONLY
	apply_remaps();
	#endif

	sys_addr_t sysmem = NULL;

	#ifdef USE_VM
	if(sys_vm_memory_map(_32MB_, _1MB_, SYS_MEMORY_CONTAINER_ID_INVALID, SYS_MEMORY_PAGE_SIZE_64K, SYS_VM_POLICY_AUTO_RECOMMENDED, &sysmem) != CELL_OK)
	{
		return false;  //leave if cannot allocate memory
	}
	#else
	{
		sysmem = sys_mem_allocate(_3MB_); // using MC by default
		if(sysmem)
			set_buffer_sizes(USE_3MB);
		else
			{sysmem = sys_mem_allocate(_2MB_); if(sysmem) set_buffer_sizes(USE_2MB);}
	}

	if(!sysmem)
	{
		set_buffer_sizes(webman_config->foot);

		get_meminfo();
		if( meminfo.avail < (BUFFER_SIZE_ALL + MIN_MEM)) set_buffer_sizes(3); //MIN+
		if( meminfo.avail < (BUFFER_SIZE_ALL + MIN_MEM)) set_buffer_sizes(1); //MIN
		if((meminfo.avail < (BUFFER_SIZE_ALL + MIN_MEM)) || sys_memory_allocate((BUFFER_SIZE_ALL), SYS_MEMORY_PAGE_SIZE_64K, &sysmem) != CELL_OK)
		{
			show_error(STR_SCAN2);
			return false;  //leave if cannot allocate memory
		}
	}
	#endif

	if(!sysmem) return false; // recheck

	sys_addr_t sysmem_psx = sysmem + (BUFFER_SIZE); //BUFFER_SIZE_PSX
	sys_addr_t sysmem_psp = sysmem + (BUFFER_SIZE) + (BUFFER_SIZE_PSX); //BUFFER_SIZE_PSP
	sys_addr_t sysmem_ps2 = sysmem + (BUFFER_SIZE) + (BUFFER_SIZE_PSX) + (BUFFER_SIZE_PSP); //BUFFER_SIZE_PS2
	sys_addr_t sysmem_dvd = sysmem + (BUFFER_SIZE) + (BUFFER_SIZE_PSX) + (BUFFER_SIZE_PSP) + (BUFFER_SIZE_PS2); // BUFFER_SIZE_DVD

	char *ignore_files = NULL;
	if(webman_config->ignore && file_exists(WM_IGNORE_FILES))
	{
		size_t size = file_size(WM_IGNORE_FILES);
		if(size < _32KB_)
		{
			ignore_files = malloc(size + 1);
			if(ignore_files) {read_file(WM_IGNORE_FILES, ignore_files, size, 0); ignore_files[size] = '\0';}
		}
	}

#if defined(LAUNCHPAD) || defined(MOUNT_ROMS)
	char *sysmem_buf = (char*)sysmem;
#endif
	char *sysmem_xml = (char*)sysmem + (BUFFER_SIZE - BUFFER_SIZE_XML);

	cellFsMkdir(XML_HOST_PATH, DMODE);
	cellFsMkdir(HTML_BASE_PATH, DMODE);

	const u16 max_xmb_items = ((u16)(BUFFER_SIZE_ALL / AVG_ITEM_SIZE));

	make_fb_xml();

	#ifdef MOUNT_ROMS
	bool c_roms = webman_config->roms;
	const char *RETROARCH_DIR = NULL;
	const char *cores_roms = NULL;
	if(c_roms)
	{
		RETROARCH_DIR = isDir(RETROARCH_DIR0) ? (const char*)RETROARCH_DIR0 :
						isDir(RETROARCH_DIR1) ? (const char*)RETROARCH_DIR1 :
												(const char*)RETROARCH_DIR2;

		cores_roms = isDir(RETROARCH_DIR0) ? (const char *)"roms/" :
											 (const char *)"cores/roms/";

		webman_config->roms = c_roms = isDir(PKGLAUNCH_DIR) && isDir(RETROARCH_DIR);
	}
	#endif

	u8 _f1_;
	u16 key;
	int fdxml; char *xml_file = (char*)MY_GAMES_XML;

	if(!is_app_home_onxmb()) webman_config->gamei = 0; // do not scan GAMEI if app_home/PS3_GAME icon is not on XMB

	bool ps2_launcher = webman_config->ps2l && isDir(PS2_CLASSIC_PLACEHOLDER);
	#ifdef COBRA_ONLY
	bool psp_launcher = webman_config->pspl && (isDir(PSP_LAUNCHER_MINIS) || isDir(PSP_LAUNCHER_REMASTERS));
	#endif
	char templn[1024]; char *title = templn, *param_sfo = templn, *xmb_item = templn, *entry_name = templn;

	led(YELLOW, BLINK_FAST);

	check_cover_folders(templn);

	// set boundary margins
	if(XMB_GROUPS)
	{
		BUFFER_SIZE_PSX -= 0x10;
		BUFFER_SIZE_PS2 -= ps2_launcher ? 0x100 : 0x10;
		#ifdef COBRA_ONLY
		BUFFER_SIZE_PSP -= psp_launcher ? 0x100 : 0x10;
		BUFFER_SIZE_DVD -= webman_config->rxvid ? 0x100 : 0x10;
		#endif
	}

	#ifdef MOUNT_ROMS

	#define ROM_PATHS	99
	char cur_roms_path[16]; *cur_roms_path = 0;
	const char *roms_path[ROM_PATHS] = { "2048", "CAP32", "MAME", "MAME2000", "MAME2003", "MIDWAY", "MAMEPLUS", "FBA", "FBA2012", "FBNEO", "ATARI", "ATARI2600", "STELLA", "ATARI800", "ATARI5200", "ATARI7800", "JAGUAR", "LYNX", "HANDY", "HATARI", "CANNONBALL", "NXENGINE", "COLECO", "AMIGA", "CD32", "VICE", "X64", "X64SC", "X64DTV", "XSCPU64", "X128", "XCBM2", "XCMB25X0", "XPET", "XPLUS4", "XVIC", "DOSBOX", "GME", "GW", "DOOM", "QUAKE", "JAVAME", "JUMP", "O2EM", "INTV", "MSX", "FMSX", "MSX2", "BMSX", "NEOCD", "NEO", "NEOGEO", "PCE", "PCECD", "PCFX", "SGX", "NGP", "NGPC", "NES", "FCEUMM", "NESTOPIA", "QNES", "GB", "GBC", "GAMBATTE", "TGBDUAL", "GBA", "VBA", "MGBA", "VBOY", "PALM", "PSXISO", "PS2ISO", "PS3ISO", "PSPISO", "POKEMINI", "SCUMMVM", "GENESIS", "GEN", "SEGACD", "MEGAD", "MEGADRIVE", "GG", "GEARBOY", "MASTER", "PICO", "SG1000", "FUSE", "ZX81", "SNES", "MSNES", "SNES9X", "SNES9X2005", "SNES9X2010", "SNES9X_NEXT", "THEODORE", "VECX", "WSWAM", "WSWAMC" };
	u16 roms_count[ROM_PATHS]; u32 count_roms = 0; memset(roms_count, 0, sizeof(roms_count));

	// remove paths not listed in roms_paths.txt
	{
		char *roms_paths = templn;
		u16 len = read_file(WM_ROMS_PATHS, roms_paths, 640, 0);
		if((len > 0) && (len != 632))
		{
			roms_paths[len] = '\0'; to_upper(roms_paths);
			for(u8 i = 0; i < ROM_PATHS; i++)
				if(roms_path[i] && !strstr(roms_paths, roms_path[i])) roms_path[i] = NULL;
		}
	}
	#endif
	u8 roms_index = 0;

	#ifdef SLAUNCH_FILE
	int fdsl = create_slaunch_file();
	#endif

#ifdef MOUNT_ROMS
scan_roms:
#endif
	fdxml = key = 0;

	for(u8 i = 0; i < 6; i++) item_count[i] = 0;

	t_string myxml_ps3; _alloc(&myxml_ps3, (char*)sysmem);		//BUFFER_SIZE_PS3
	t_string myxml    ; _alloc(&myxml,     (char*)sysmem_xml);	//BUFFER_SIZE_XML
	t_string myxml_psx; _alloc(&myxml_psx, (char*)sysmem_psx);	//BUFFER_SIZE_PSX
	t_string myxml_psp; _alloc(&myxml_psp, (char*)sysmem_psp);	//BUFFER_SIZE_PSP
	t_string myxml_ps2; _alloc(&myxml_ps2, (char*)sysmem_ps2);	//BUFFER_SIZE_PS2
	t_string myxml_dvd; _alloc(&myxml_dvd, (char*)sysmem_dvd);	//BUFFER_SIZE_DVD

	// --- build group headers ---
	char *tempstr, *table_xml, *folder_name; table_xml = tempstr = sysmem_xml; folder_name = sysmem_xml + (3*KB);

	char localhost[2]; strcopy(localhost, "0"); //sprintf(localhost, "http://%s", local_ip); // "0" = localhost
	const char *proxy_include = (char*)WEB_LINK_INC;
	#ifdef WM_PROXY_SPRX
	if((cobra_version > 0) && file_exists(WM_PROXY) && !(webman_config->wm_proxy)) {proxy_include = (char*)XAI_LINK_INC, *localhost = NULL;}
	#endif

	if( !scanning_roms && XMB_GROUPS )
	{
		if(!(webman_config->cmask & PS3))
		{
			_concat2(&myxml_ps3, "<V id=\"seg_wm_ps3_items\"><A>", proxy_include);
		}
		if(!(webman_config->cmask & PS2))
		{
			_concat2(&myxml_ps2, "<V id=\"seg_wm_ps2_items\"><A>", proxy_include);
			if(ps2_launcher)
			{
				_concat(&myxml_ps2, ps2launcher_table(title, table_xml));
			}
		}
		#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1))
		{
			_concat2(&myxml_psx, "<V id=\"seg_wm_psx_items\"><A>", proxy_include);
		}
		if(!(webman_config->cmask & PSP))
		{
			_concat2(&myxml_psp, "<V id=\"seg_wm_psp_items\"><A>", proxy_include);
			if(psp_launcher)
			{
				_concat(&myxml_psp, psplauncher_table(title, table_xml));
			}
		}
		if(!(webman_config->cmask & DVD) || !(webman_config->cmask & BLU))
		{
			_concat2(&myxml_dvd, "<V id=\"seg_wm_dvd_items\"><A>", proxy_include);
			if(webman_config->rxvid)
			{
				_concat(&myxml_dvd, videos_table(table_xml));
			}
		}
		#endif
	}

	int fd;
	t_keys skey[max_xmb_items];

	char param[STD_PATH_LEN], icon[STD_PATH_LEN], subpath[STD_PATH_LEN], enc_dir_name[1024];

	u8 is_net = 0;

	// --- scan xml content ---
	#if defined(MOUNT_GAMEI) || defined(MOUNT_ROMS)
	f1_len = ((webman_config->nogrp && c_roms) ? id_ROMS : webman_config->gamei ? id_GAMEI : id_VIDEO) + 1;
	#endif

	int ns = NONE; u8 uprofile, all_profiles = (profile >= 5);

	bool allow_npdrm = webman_config->npdrm && (!isDir("/dev_hdd0/GAMEZ") && is_app_home_onxmb());

	#ifdef NET_SUPPORT
	int abort_connection = 0;
	#ifdef USE_INTERNAL_NET_PLUGIN
	if(g_socket >= 0 && open_remote_dir(g_socket, "/", &abort_connection, false) < 0) do_umount(false);
	#endif
	#endif

	#ifdef MOUNT_ROMS
	if(!scanning_roms || roms_path[roms_index])
	#endif
	for(u8 f0 = 0; f0 < MAX_DRIVES; f0++)  // drives: 0="/dev_hdd0", 1="/dev_usb000", 2="/dev_usb001", 3="/dev_usb002", 4="/dev_usb003", 5="/dev_usb006", 6="/dev_usb007", 7="/net0", 8="/net1", 9="/net2", 10="/net3", 11="/net4", 12="/ext", 13="/dev_sd", 14="/dev_ms", 15="/dev_cf"
	{
		if(!refreshing_xml) break;

		if(check_drive(f0)) continue;

		is_net = IS_NET;

		if(allow_npdrm) {strcpy(paths[id_GAMEZ], IS_HDD0 ? "game" : "GAMEZ"); allow_npdrm = IS_HDD0;}

		if(conn_s_p == START_DAEMON)
		{
			if(webman_config->boots && BETWEEN(1, f0, 6)) // usb000->007
			{
				wait_for(drives[f0], webman_config->boots);
			}
		}

		#ifdef USE_NTFS
		if(!scanning_roms && IS_NTFS && webman_config->ntfs) prepNTFS(0);
		#endif

		if(!(is_net || IS_NTFS) && (isDir(drives[f0]) == false)) continue;

		#ifdef NET_SUPPORT
		if(is_net && (ns >= 0) && (ns!=g_socket)) sclose(&ns);
		#endif

		if(IS_NTFS) drives[NTFS][10] = 0;

		ns = NONE;
		for(u8 f1 = 0; f1 < f1_len; f1++) // paths: 0="GAMES", 1="GAMEZ", 2="PS3ISO", 3="BDISO", 4="DVDISO", 5="PS2ISO", 6="PSXISO", 7="PSXGAMES", 8="PSPISO", 9="ISO", 10="video", 11="GAMEI", 12="ROMS"
		{
			if(!refreshing_xml) break;

			if(key >= max_xmb_items) break;

			_f1_ = f1; // _f1_ changes to id_PS2ISO for CD/DVD folders (9="CD" / 10="DVD")

			if(scanning_roms)
			{
				_f1_ = f1 = id_ROMS;
			}
			else
			{
				#ifndef COBRA_ONLY
				if(IS_ISO_FOLDER && !(IS_PS2ISO)) continue; // 0="GAMES", 1="GAMEZ", 5="PS2ISO", 10="video"
				#endif

				//if(IS_PS2ISO && f0>0)  continue; // PS2ISO is supported only from /dev_hdd0
				if(IS_GAMEI_FOLDER) {if((!webman_config->gamei) || (IS_HDD0) || (IS_NTFS)) continue;}
				if(IS_ISO_DIR     ) {if(is_net) continue; else {concat_path(param, drives[f0], CUSTOM_PATH1); if(isDir(param)) _f1_ = id_PS2ISO; strcpy(paths[id_ISO], (_f1_ == id_PS2ISO) ? CUSTOM_PATH1 : "ISO");}}
				if(IS_VIDEO_FOLDER) {if(is_net) continue; else if(IS_HDD0) strcpy(paths[id_VIDEO], "video"); else {concat_path(param, drives[f0], CUSTOM_PATH2); if(isDir(param)) _f1_ = id_PS2ISO; strcpy(paths[id_VIDEO], (_f1_ == id_PS2ISO) ? CUSTOM_PATH2 : "GAMES_DUP");}}
				if(IS_NTFS)  {if(f1 >= id_ISO) break; else if(IS_JB_FOLDER || (f1 == id_PSXGAMES)) continue;} // 0="GAMES", 1="GAMEZ", 7="PSXGAMES", 9="ISO", 10="video", 11="GAMEI", 12="ROMS"

				#ifdef NET_SUPPORT
				if(is_net)
				{
					if(f1 >= id_ISO) f1 = id_GAMEI; // ignore 9="ISO", 10="video"
				}
				#endif
				if(check_content_type(_f1_)) continue;
			}

			#ifdef NET_SUPPORT
			#ifdef USE_INTERNAL_NET_PLUGIN
			if(is_net && (netiso_svrid == (f0-NET)) && (g_socket != NONE)) ns = g_socket; /* reuse current server connection */ else
			#endif
			if(is_net && (ns<0)) ns = connect_to_remote_server(f0-NET);
			if(is_net && (ns<0)) break;
			#endif

			bool ls; u8 li, subfolder; li = subfolder = 0; ls = false; // single letter folder

		subfolder_letter_xml:
			subfolder = 0; if(all_profiles) uprofile = 1; else uprofile = profile;
		read_folder_xml:
		//
			if(!refreshing_xml || key >= max_xmb_items) break;
			#ifdef MOUNT_ROMS
			if(scanning_roms)
			{
				if(is_net)
					sprintf(param, "%s/ROMS%s/%s", "", SUFIX(uprofile), roms_path[roms_index]);
				else if(IS_NTFS)
					sprintf(param, "%s/USRDIR/%s%s", RETROARCH_DIR, cores_roms, roms_path[roms_index]);
				else
					sprintf(param, "%s/ROMS%s/%s", drives[f0], SUFIX(uprofile), roms_path[roms_index]);

				show_progress(param, OV_SCAN);
			}
			else
			#endif
			set_scan_path(li, f0, f1, is_net, uprofile, param);

			#ifdef NET_SUPPORT
			if(is_net && open_remote_dir(ns, param, &abort_connection, !IS_JB_FOLDER) < 0) goto continue_reading_folder_xml; //continue;
			#endif
			{
				CellFsDirectoryEntry entry; u32 read_e;
				int fd2 = 0, flen;
				char title_id[24], app_ver[8];
				u8 is_iso = 0;

				#ifdef NET_SUPPORT
				sys_addr_t data2 = NULL;
				int v3_entries, v3_entry; v3_entries = v3_entry = 0;
				netiso_read_dir_result_data *data = NULL; char neth[8];
				if(is_net)
				{
					v3_entries = read_remote_dir(ns, &data2, &abort_connection);
					if(!data2) goto continue_reading_folder_xml; //continue;
					data = (netiso_read_dir_result_data*)data2; sprintf(neth, "/net%i", (f0-NET));
				}
				else
				#endif
				#ifdef MOUNT_ROMS
				if(scanning_roms && isDir(param) == false)
				{
					if(!IS_HDD0) goto continue_reading_folder_xml; //continue;

					sprintf(param, "%s/ROMS%s/%s", drives[f0], SUFIX(uprofile), cur_roms_path); // try folder name in lower case (e.g. /ROMS/snes)
					if(isDir(param) == false)
					{
						char *roms_folder = tempstr;
						strcopy(roms_folder, cur_roms_path); *roms_folder ^= 0x20; // capitalize first letter of folder name (e.g. /ROMS/Snes)

						sprintf(param, "%s/ROMS%s/%s", drives[f0], SUFIX(uprofile), roms_folder);
						if(isDir(param) == false) continue;
					}
				}
				#endif
				if(!is_net && cellFsOpendir(param, &fd) != CELL_FS_SUCCEEDED) goto continue_reading_folder_xml; //continue;

				bool is_game_dir = (allow_npdrm && (f1 == id_GAMEZ));

				while((!is_net && (!cellFsGetDirectoryEntries(fd, &entry, sizeof(entry), &read_e) && read_e > 0))
					#ifdef NET_SUPPORT
					|| (is_net && (v3_entry < v3_entries))
					#endif
					)
				{
					if(key >= max_xmb_items) break;

					#ifdef NET_SUPPORT
					if(is_net)
					{
						if(ignore_files && strstr(ignore_files, data[v3_entry].name)) continue;

						bool is_slf = BETWEEN(id_PS3ISO, f1, id_PSPISO) && (data[v3_entry].is_directory) && (data[v3_entry].name[1] == '\0');

						if(is_slf) {ls = true; v3_entry++; continue;} // single letter folder was found

						if(add_net_game(ns, data, v3_entry, neth, param, title, tempstr, enc_dir_name, icon, title_id, app_ver, f1, 0) == FAILED) {v3_entry++; continue;}

						if(ignore_files && HAS_TITLE_ID && strstr(ignore_files, title_id)) {v3_entry++; continue;}

						#ifdef SLAUNCH_FILE
						if(key < MAX_SLAUNCH_ITEMS) add_slaunch_entry(fdsl, neth, param, data[v3_entry].name, icon, title, title_id, f1);
						#endif
						read_e = sprintf(table_xml, "<T key=\"%04i\" include=\"inc\">"
										 XML_PAIR("icon","%s")
										 XML_PAIR("title","%s") //"%s"
										 XML_PAIR("module_action","%s/mount_ps3%s%s/%s"),
										 key, icon,
										 title, //proxy_include,
										 localhost, neth, param, enc_dir_name);

						*folder_name = NULL;

						if(*app_ver) {strcat(title_id, " | v"); strcat(title_id, app_ver);}

						read_e += add_info(table_xml + read_e, folder_name, roms_index, enc_dir_name, title_id, f0, f1, 1);

						if(add_xmb_entry(f0, f1, read_e, table_xml, title, skey[key].value, key, &myxml_ps3, &myxml_ps2, &myxml_psx, &myxml_psp, &myxml_dvd, data[v3_entry].name, 0)) key++; else break;

						v3_entry++;
					}
					else
					#endif // #ifdef NET_SUPPORT
					{
						if((entry.entry_name.d_name[0] == '.') || strchr(entry.entry_name.d_name, '<')) continue;

						if(is_game_dir && !islike(entry.entry_name.d_name, "NP")) continue;

						if(ignore_files && strstr(ignore_files, entry.entry_name.d_name)) continue;

						//////////////////////////////
						subfolder = 0;
						if(BETWEEN(id_PS3ISO, f1, id_ISO) || scanning_roms)
						{
							snprintf(subpath, STD_PATH_LEN, "%s/%s", param, entry.entry_name.d_name); cellFsClosedir(fd2);
							if(cellFsOpendir(subpath, &fd2) == CELL_FS_SUCCEEDED)
							{
								strcpy(subpath, entry.entry_name.d_name); subfolder = 1;
		next_xml_entry:
								cellFsGetDirectoryEntries(fd2, &entry, sizeof(entry), &read_e);
								if(read_e < 1) {cellFsClosedir(fd2); fd2 = 0; continue;}
								if(entry.entry_name.d_name[0] == '.') goto next_xml_entry;

								strcpy(entry_name, entry.entry_name.d_name);
								entry.entry_name.d_namlen = snprintf(entry.entry_name.d_name, D_NAME_LEN, "%s/%s", subpath, entry_name);
							}
						}
						//////////////////////////////

						if(key >= max_xmb_items) break;

						// skip duplicated game in /dev_hdd0
						if(is_dupe(f0, f1, entry.entry_name.d_name, templn)) goto continue_loop; //continue;

						if(IS_JB_FOLDER)
						{
							#ifdef MOUNT_GAMEI
							if(IS_GAMEI_FOLDER)
							{
								if(!webman_config->gamei) continue;

								if(!is_app_dir(param, entry.entry_name.d_name)) continue;
								concat_path2(param_sfo, param, entry.entry_name.d_name, "/PARAM.SFO");

								// create game folder in /dev_hdd0/game and copy PARAM.SFO to prevent deletion of XMB icon when gameDATA is disabled
								char *hdd_game_sfo = tempstr;
								if((entry.entry_name.d_namlen >= 16) && (entry.entry_name.d_name[6] == '-'))
									sprintf(hdd_game_sfo, "%s%.9s/PARAM.SFO", _HDD0_GAME_DIR, entry.entry_name.d_name + 7); // convert content_id => title_id
								else
									concat_path2(hdd_game_sfo, _HDD0_GAME_DIR, entry.entry_name.d_name, "/PARAM.SFO"); // title_id

								if(not_exists(hdd_game_sfo))
								{
									// Create folder in /dev_hdd0/game with PARAM.SFO
									mkdir_tree(hdd_game_sfo);
									file_copy(param_sfo, hdd_game_sfo);
								}

								if(getTitleID(hdd_game_sfo, templn, IS_GAME_DATA)) continue;
							}
							else
							#endif // #ifdef MOUNT_GAMEI
							if(is_game_dir)
								read_e = concat_path2(param_sfo, param, entry.entry_name.d_name, "/USRDIR/EBOOT.BIN");
							else
							{
								concat_path2(param_sfo, param, entry.entry_name.d_name, "/PS3_GAME/PARAM.SFO");
								check_ps3_game(param_sfo);
							}

							is_iso = false;
						}
						else
						{
							flen = entry.entry_name.d_namlen;
							is_iso = is_iso_file(entry.entry_name.d_name, flen, _f1_, f0); if(!is_iso) goto continue_loop; //continue;
						}

						if(is_iso || (IS_JB_FOLDER && file_exists(param_sfo)))
						{
							*app_ver = *icon = *title_id = NULL;

							if(!is_iso) // IS_JB_FOLDER
							{
								if(is_game_dir) strcopy(param_sfo + read_e - 17, "/PARAM.SFO");
								if(webman_config->info & INFO_VER) getTitleID(param_sfo, app_ver, GET_VERSION);

								get_title_and_id_from_sfo(title, title_id, entry.entry_name.d_name, icon, tempstr, 0);
							}
							else // is_iso
							{
								if(IS_HDD0 && IS(entry.entry_name.d_name, "~tmp.iso")) goto continue_loop; //continue;

								#ifdef COBRA_ONLY
								if(get_name_iso_or_sfo(title, title_id, icon, param, entry.entry_name.d_name, f0, f1, uprofile, flen, tempstr) == FAILED) continue;
								#else
								get_name(title, entry.entry_name.d_name, NO_EXT);
								#endif
							}

							#ifdef MOUNT_ROMS
							if(!scanning_roms || get_covers_retro(icon, cur_roms_path, entry.entry_name.d_name) == false)
							#endif
							get_default_icon(icon, param, entry.entry_name.d_name, !is_iso, title_id, ns, f0, f1);

							if(ignore_files && HAS_TITLE_ID && strstr(ignore_files, title_id)) goto continue_loop; //continue;

							// get rom alias. use file name as default title
							#ifdef MOUNT_ROMS
							#ifndef ENGLISH_ONLY
							if(scanning_roms)
							{
								char *key = get_filename(entry.entry_name.d_name);
								rom_alias(key, title, param);
							}
							#endif
							#endif

							#ifdef SLAUNCH_FILE
							if(key < MAX_SLAUNCH_ITEMS) add_slaunch_entry(fdsl, "", param, entry.entry_name.d_name, icon, title, title_id, f1);
							#endif
							add_title_id(title, title_id);
							urlenc(enc_dir_name, entry.entry_name.d_name);

							// subfolder name
							if((IS_NTFS) && entry.entry_name.d_name[0] == '[')
							{
								strcpy(folder_name, entry.entry_name.d_name); *folder_name = '/'; get_flag(folder_name, "] ");
							}
							else
							{
								*folder_name = NULL;
								if(subfolder)
								{
									char *p = strchr(entry.entry_name.d_name, '/');
									if(p)
									{
										*p = NULL; int plen = sprintf(folder_name, "/%s", entry.entry_name.d_name); *p = '/';

										char *file_name = tempstr;
										strcpy(file_name, title + plen);

										if(plen == 2)
											get_name(title, entry.entry_name.d_name + 2, 0);
										else if(islike(file_name, folder_name + 1))
											get_name(title, p + 1, 0);
										else if(!file_name[0])
											strcopy(title, folder_name + 1);
										else
											sprintf(title, "[%s] %s", folder_name + 1, file_name);
									}
								}
							}

							if(webman_config->info & INFO_VER) get_local_app_ver(app_ver, title_id, tempstr);

							read_e = sprintf(table_xml, "<T key=\"%04i\" include=\"inc\">"
											 XML_PAIR("icon","%s")
											 XML_PAIR("title","%s") //"%s"
											 XML_PAIR("module_action","%s/mount_ps3%s%s/%s"),
											 key, icon,
											 title, //proxy_include,
											 localhost, "", param, enc_dir_name);

							if(*app_ver) {strcat(title_id, " | v"); strcat(title_id, app_ver);}

							read_e += add_info(table_xml + read_e, folder_name, roms_index, enc_dir_name, title_id, f0, is_game_dir ? id_NPDRM : f1, 5);

							if(add_xmb_entry(f0, _f1_, read_e, table_xml, title, skey[key].value, key, &myxml_ps3, &myxml_ps2, &myxml_psx, &myxml_psp, &myxml_dvd, entry.entry_name.d_name, subfolder)) key++; else break;
						}
						//////////////////////////////
					continue_loop:
						if(subfolder) goto next_xml_entry;
						//////////////////////////////
					}
				} //while

				if(!is_net) cellFsClosedir(fd); cellFsClosedir(fd2);

				#ifdef NET_SUPPORT
				if(data2) {sys_memory_free(data2); data2 = NULL;}
				#endif
			}
//
		continue_reading_folder_xml:
			if(IS_ROMS_FOLDER || (f1 < id_ISO && !IS_NTFS))
			{
				if(uprofile > 0) {subfolder = 0; if(all_profiles && (uprofile < 4)) ++uprofile; else uprofile = 0; goto read_folder_xml;}
				if(is_net && BETWEEN(id_PS3ISO, f1, id_PSPISO) && !scanning_roms)
				{
					if(ls && (li < 27)) {li++; goto subfolder_letter_xml;} else if(li < LANG_CUSTOM) {li = LANG_CUSTOM; goto subfolder_letter_xml;}
				}
			}
//
		} //f1
		#ifdef NET_SUPPORT
		if(is_net && (ns >= 0) && (ns!=g_socket)) sclose(&ns);
		#endif
	} //f0

	drives[NTFS][10] = ':';

	if( !scanning_roms && XMB_GROUPS )
	{
		if(!(webman_config->cmask & PS3)) {_concat(&myxml_ps3, XML_BEGIN_ITEMS);}
		if(!(webman_config->cmask & PS2)) {_concat(&myxml_ps2, XML_BEGIN_ITEMS); if(ps2_launcher) _concat2(&myxml_ps2, XML_QUERY, QUERY_PS2_LAUNCHER);}

		#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1)) {_concat(&myxml_psx, XML_BEGIN_ITEMS);}
		if(!(webman_config->cmask & PSP)) {_concat(&myxml_psp, XML_BEGIN_ITEMS); if(psp_launcher) _concat2(&myxml_psp, XML_QUERY, QUERY_PSP_LAUNCHERS);}
		if(!(webman_config->cmask & DVD) ||
		   !(webman_config->cmask & BLU)) {_concat(&myxml_dvd, XML_BEGIN_ITEMS); if(webman_config->rxvid) _concat2(&myxml_dvd, XML_QUERY, SRC("rx_video", "#seg_wm_bdvd"));}
		#endif
	}

	led(GREEN, ON);

	// --- sort scanned content
	if(key)
	{   // sort xmb items
		qsort(skey, key, sizeof(t_keys), qcompare_xml);
	}

	// --- add eject & setup/xmbm+ menu
	#ifdef ENGLISH_ONLY
	bool add_xmbm_plus = file_exists(XMBMANPLS_PATH "/FEATURES/webMAN.xml");
	#else
	sprintf(templn, "%s/FEATURES/webMAN%s.xml", XMBMANPLS_PATH, lang_code);
	bool add_xmbm_plus = file_exists(templn);
	#endif

	char *table_xml2 = templn; // needed for myxml
	char *buffer = enc_dir_name;

	// --- build xml headers
	#ifdef MOUNT_ROMS
	myxml.size = sprintf(myxml.str, "%s"
						"<V id=\"%s%s\">"
						"<A>%s", XML_HEADER,
						scanning_roms ? "seg_wm_rom_" : "seg_mygames",
						scanning_roms ? roms_path[roms_index] : "",
						proxy_include);

	if(scanning_roms)
	{
		xml_file = enc_dir_name;
		sprintf(xml_file, "%s/ROMS_%s.xml", HTML_BASE_PATH, roms_path[roms_index]);
		cellFsUnlink(xml_file);
		goto save_xml;
	}
	#else
	myxml.size = sprintf(myxml.str, "%s"
						"<V id=\"%s\">"
						"<A>", XML_HEADER, "seg_mygames");
	#endif

	// --- Add Eject Disc if webMAN.xml does not exist
	if(!add_xmbm_plus)
	{
		_concat(&myxml, eject_table(buffer, table_xml2));
	}

	// --- Add Groups attribute tables
	if( XMB_GROUPS )
	{
		add_group_tables(buffer, table_xml2, &myxml);

		if(add_custom_xml(table_xml2))
		{
			_concat(&myxml, custom_table(buffer, table_xml2));
		}
	}

	// --- Add Setup attribute table
	if( ADD_SETUP )
	{
		_concat(&myxml, setup_table(buffer, table_xml2, add_xmbm_plus));

		if(add_xmbm_plus)
			_concat(&myxml, XML_PAIR("child","segment"));
		else
			{sprintf(table_xml2, XML_PAIR("module_action","http://%s/setup.ps3"), local_ip); _concat(&myxml, table_xml2);}

		_concat(&myxml, "</T>");
	}

	#ifndef ENGLISH_ONLY
	close_language();
	#endif

	// --- Add groups queries (setup + eject + categories)
	if( XMB_GROUPS )
	{
		if(!scanning_roms && webman_config->root && !(webman_config->app_home) && !is_app_home_onxmb())
		{
			has_app_home = true;
			_concat(&myxml, QUERY_APP_HOME_ICON);
		}
		else
			_concat(&myxml, XML_BEGIN_ITEMS);

		// Add Setup item
		if( ADD_SETUP )
		{
			if(add_xmbm_plus)
			#ifdef ENGLISH_ONLY
				_concat2(&myxml, XML_QUERY, QUERY_SETUP_ENGLISH);
			#else
			{
				sprintf(xmb_item, QUERY_SETUP_LOCALIZED, lang_code);
				_concat2(&myxml, XML_QUERY, xmb_item);
			}
			#endif
			else
				_concat2(&myxml, XML_ITEM, ADD_XMB_ITEM("setup"));
		}

		if(!add_xmbm_plus) _concat2(&myxml, XML_ITEM, ADD_XMB_ITEM("eject"));

		if(!(webman_config->cmask & PS3) && item_count[gPS3]) _concat2(&myxml, XML_QUERY, SRC("wm_ps3", "#seg_wm_ps3_items"));
		if(!(webman_config->cmask & PS2) && item_count[gPS2]) _concat2(&myxml, XML_QUERY, SRC("wm_ps2", "#seg_wm_ps2_items"));
		#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1) && item_count[gPSX]) _concat2(&myxml, XML_QUERY, SRC("wm_psx", "#seg_wm_psx_items"));
		if(!(webman_config->cmask & PSP) && item_count[gPSP]) _concat2(&myxml, XML_QUERY, SRC("wm_psp", "#seg_wm_psp_items"));
		if((!(webman_config->cmask & DVD) || !(webman_config->cmask & BLU))
										 && item_count[gDVD]) _concat2(&myxml, XML_QUERY, SRC("wm_dvd", "#seg_wm_dvd_items"));
		#endif
		#ifdef MOUNT_ROMS
		if(c_roms) _concat2(&myxml, XML_QUERY, SRC("wm_rom", "xmb://localhost" HTML_BASE_PATH "/ROMS.xml#seg_wm_rom_items"));
		#endif
		if(add_custom_xml(table_xml2)) _concat(&myxml, table_xml2);
		_concat(&myxml, XML_END_ITEMS);
	}

#ifdef MOUNT_ROMS
save_xml:
	if(scanning_roms && (key == 0)) ; // do not create empty xml
	else
#endif
	// --- save xml file
	if(cellFsOpen(xml_file, CELL_FS_O_CREAT | CELL_FS_O_WRONLY | CELL_FS_O_TRUNC, &fdxml, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		cellFsWrite(fdxml, (char*)myxml.str, myxml.size, NULL);

		int slen;

		if(scanning_roms || webman_config->nogrp)
		{
			cellFsWrite(fdxml, (char*)myxml_ps3.str, myxml_ps3.size, NULL);
			cellFsWrite(fdxml, (char*)XML_BEGIN_ITEMS, 7, NULL);

			if(!scanning_roms)
			{
				t_string myxml_ngp; _alloc(&myxml_ngp, (char*)sysmem);

				if(!add_xmbm_plus) _concat2(&myxml_ngp, XML_ITEM, ADD_XMB_ITEM("eject"));

				if( ADD_SETUP )
				{
					if(add_xmbm_plus)
					#ifdef ENGLISH_ONLY
						_concat2(&myxml_ngp, XML_QUERY, QUERY_SETUP_ENGLISH);
					#else
					{
						sprintf(xmb_item, QUERY_SETUP_LOCALIZED, lang_code);
						_concat2(&myxml_ngp, XML_QUERY, xmb_item);
					}
					#endif
					else
						_concat2(&myxml_ngp, XML_ITEM, ADD_XMB_ITEM("setup"));
				}

				if(add_custom_xml(table_xml)) _concat(&myxml_ngp, table_xml);

				if(myxml_ngp.size)
					cellFsWrite(fdxml, (char*)myxml_ngp.str, myxml_ngp.size, NULL);
			}

			for(u16 len, a = 0; a < key; a++)
			{
				len = sprintf(xmb_item, "%s" ADD_XMB_ITEM("%s"), XML_ITEM, skey[a].value + XML_KEY_LEN, skey[a].value + XML_KEY_LEN);
				cellFsWrite(fdxml, xmb_item, len, NULL);
			}

			slen = sprintf(buffer, XML_END_OF_FILE);
		}
		else
		{
			#ifdef COBRA_ONLY
			const u8 section[5] = {gPS3, gPS2, gPSX, gPSP, gDVD};
			for(u8 n = 0; n < 5; n++)
			#else
			const u8 section[2] = {gPS3, gPS2};
			for(u8 n = 0; n < 2; n++)
			#endif
			{
				if(item_count[section[n]] == 0) continue;

				// --- items to section
				if(n == 0) cellFsWrite(fdxml, (char*)myxml_ps3.str, myxml_ps3.size, NULL);
				if(n == 1) cellFsWrite(fdxml, (char*)myxml_ps2.str, myxml_ps2.size, NULL);
				#ifdef COBRA_ONLY
				if(n == 2) cellFsWrite(fdxml, (char*)myxml_psx.str, myxml_psx.size, NULL);
				if(n == 3) cellFsWrite(fdxml, (char*)myxml_psp.str, myxml_psp.size, NULL);
				if(n == 4) cellFsWrite(fdxml, (char*)myxml_dvd.str, myxml_dvd.size, NULL);
				#endif

				// --- add sorted items to xml
				u16 len = 0;
				for(u16 a = 0; a < key; a++)
					if(*skey[a].value == (1<<n)) // PS3, PS2, PS1, PSP, BLU
					{
						len = sprintf(xmb_item, "%s" ADD_XMB_ITEM("%s"), XML_ITEM, skey[a].value + XML_KEY_LEN, skey[a].value + XML_KEY_LEN);
						cellFsWrite(fdxml, xmb_item, len, NULL);
					}

				// --- close view of xml groups
				if(len)
				{
					cellFsWrite(fdxml, XML_END_ITEMS, 8, NULL);
					#ifdef COBRA_ONLY
					if(webman_config->rxvid && (n == gDVD))
					{
						cellFsWrite(fdxml, "<V id=\"seg_wm_bdvd\"><I>", 23, NULL); *buffer = 0;
						len = add_html(dat_QUERY_VIDEOS1, 0, buffer, xmb_item); // QUERY_VIDEOS1
						cellFsWrite(fdxml, xmb_item, len, NULL); *buffer = 0;
						len = add_html(dat_QUERY_VIDEOS2, 0, buffer, xmb_item); // QUERY_VIDEOS2
						cellFsWrite(fdxml, xmb_item, len, NULL);
					}
					#endif
				}
			}
			slen = strcopy(buffer, "</X>\r\n");
		}

		cellFsWrite(fdxml, (char*)buffer, slen, NULL);
		cellFsClose(fdxml);

		cellFsChmod(xml_file, MODE);

		if(webman_config->root)
		{
			sprintf(templn, "/%s", FB_HEN_XML); // force copy file instead of link (using //dev_hdd0)
			force_copy(xml_file, templn);
			patch_file(FB_HEN_XML, "fb\"     ", 66, 8); // replace seg_mygames with seg_fb

			if(get_explore_interface())
			{
				exec_xmb_command2("focus_category %s", "game");			 // return focus to game column
				exec_xmb_command2("focus_segment_index %s", "xmb_app3"); // focus on webMAN Games
				exec_xmb_command2("reload_category %s", "game");
			}
		}
	}

	#ifdef MOUNT_ROMS
	if(refreshing_xml && (scanning_roms || (c_roms && XMB_GROUPS)))
	{
		if(scanning_roms)
		{
			skip_rom_path:
			roms_count[roms_index] = key, count_roms += key; *cur_roms_path = key = 0;
			roms_index++;
		}
		else
		{
			scanning_roms = true;
			cellFsUnlink(HTML_BASE_PATH "/ROMS.xml");
		}

		if(roms_index < ROM_PATHS)
		{
			if(!roms_path[roms_index])
				goto skip_rom_path; // skip roms path if not listed in roms_paths.txt

			strncpy(cur_roms_path, roms_path[roms_index], sizeof(cur_roms_path)); to_lower(cur_roms_path);
			goto scan_roms; // loop scanning_roms
		}
		scanning_roms = false;

		// ---- Build ROMS.xml
		if(count_roms)
			build_roms_xml(sysmem_buf, title, table_xml, roms_count, roms_path, ROM_PATHS);
		else if(c_roms && XMB_GROUPS)
		{
			char *xml = tempstr;
			u16 len = read_file(MY_GAMES_XML, xml, _4KB_, 200);
			if(len > 83)
			{
				// Patch <Q class="type:x-xmb/folder-pixmap" key="wm_rom" attr="wm_rom" src="xmb://localhost/dev_hdd0/xmlhost/game_plugin/ROMS.xml#seg_wm_rom_items"/>
				char *pos = strstr(xml + 83, HTML_BASE_PATH "/ROMS.xml");
				if(pos)
				{
					pos -= 83; if(*pos == '<') *pos = ' ';
					patch_file(MY_GAMES_XML, xml, 200, len);
				}
			}
		}
	}
	#endif

	#ifdef SLAUNCH_FILE
	close_slaunch_file(fdsl);
	#endif

	#ifdef LAUNCHPAD
	bool launchpad_xml = !(webman_config->launchpad_xml) && file_exists(LAUNCHPAD_FILE_XML);

	if(refreshing_xml && launchpad_xml)
	{
		*sysmem_buf = *param = *tempstr = *templn = NULL;
		game_listing(sysmem_buf, templn, param, tempstr, LAUNCHPAD_MODE, false);
	}
	#endif

	disable_progress();
	led(GREEN, ON);

	// --- release allocated memory
	if(ignore_files) free(ignore_files);
	#ifdef USE_VM
	sys_vm_unmap(sysmem);
	#else
	if(sysmem) sys_memory_free(sysmem);
	#endif

	return false;
}

static void update_xml_thread(u64 conn_s_p)
{
	refreshing_xml = 1;

	if(IS_ON_XMB)
		wait_for_xmb(); // wait for explore_plugin

	if(scan_mygames_xml(conn_s_p)) mount_autoboot();

	refreshing_xml = 0;
	sys_ppu_thread_exit(0);
}

static void refresh_xml(char *msg)
{
	if(refreshing_xml) return;

	sprintf(msg, "%s XML%s: %s", STR_REFRESH, SUFIX2(profile), STR_SCAN2);
	show_msg_with_icon(ICON_NOTIFY, msg);

	refreshing_xml = 1, games_found = 0;

#ifdef USE_NTFS
	root_check = true;
#endif

	setPluginActive();

#ifndef LITE_EDITION
	webman_config->profile = profile; save_settings();
#endif

	// refresh XML
	sys_ppu_thread_t t_id;
	sys_ppu_thread_create(&t_id, start_www, REFRESH_CONTENT, THREAD_PRIO_HIGH, THREAD_STACK_SIZE_WEB_CLIENT, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_CMD);

	// backup / restore act.bak -> act.dat
	backup_act_dat();

	// refresh SND0 settings for new installed games only with combo SELECT+R3+L1+R1 (reload_xmb)

	#ifdef MUTE_SND0
	// check for SELECT+R3+L1+R1 (reload_xmb)
	if(is_pressed(CELL_PAD_CTRL_L1 | CELL_PAD_CTRL_R1))
	{
		mute_snd0(true);
	}
	#endif

	// wait until complete
	while(refreshing_xml && working)
	{
		sys_ppu_thread_sleep(3);

		pad_data = pad_read();
		if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == (CELL_PAD_CTRL_SELECT | CELL_PAD_CTRL_L3) || show_persistent_popup || show_scan_progress)
		{
#ifndef LITE_EDITION
			if(webman_config->lang > 6)
				snprintf(msg, 200,	"%s:\n"
									"%s - %'i", STR_SCAN2, STR_GAMES, games_found);
			else
#endif
				snprintf(msg, 200,	"%s:\n"
									" %'i %s", STR_SCAN2, games_found, STR_GAMES);
			show_msg_with_icon(ICON_WAIT, msg);
		}
	}

#ifndef LITE_EDITION
	if(webman_config->lang > 6)
		sprintf(msg, "%s XML%s: OK\n"
					 "%s - %'i ", STR_REFRESH, SUFIX2(profile), STR_GAMES, games_found);
	else
#endif
		sprintf(msg, "%s XML%s: OK\n"
					 "%'i %s", STR_REFRESH, SUFIX2(profile), games_found, STR_GAMES);
	show_msg_with_icon(ICON_GAME, msg);
	if(!webman_config->nobeep)
		play_sound_id(5);

	setPluginInactive(); games_found = 0;
}
