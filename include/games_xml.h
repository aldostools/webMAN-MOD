#define AVG_ITEM_SIZE			420

#define QUERY_XMB(key, src) 	"<Query class=\"type:x-xmb/folder-pixmap\" key=\"" key "\" attr=\"" key "\" src=\"" src "\"/>"
#define QUERY_XMB2(key, src) 	"<Query class=\"type:x-xmb/folder-pixmap\" key=\"" key "\" src=\"" src "\"/>"
#define ADD_XMB_ITEM(key)		"<Item class=\"type:x-xmb/module-action\" key=\"" key "\" attr=\"" key "\"/>"

#define XML_HEADER				"<?xml version=\"1.0\" encoding=\"UTF-8\"?><XMBML version=\"1.0\">"
#define XML_PAIR(key, value) 	"<Pair key=\"" key "\"><String>" value "</String></Pair>"

#define XAI_LINK_PAIR			XML_PAIR("module_name", WM_PROXY_SPRX) XML_PAIR("bar_action", "none")
#define WEB_LINK_PAIR			XML_PAIR("module_name", "webbrowser_plugin")

#define STR_NOITEM_PAIR			XML_PAIR("str_noitem", "msg_error_no_content") "</Table>"

#define XML_KEY_LEN				7 /* 1 + 6 = Group + 6 chars */

#define XMB_GROUPS				(!webman_config->nogrp)
#define ADD_SETUP				(!webman_config->nosetup)

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

bool scanning_roms = false;

#ifdef COBRA_ONLY
static void apply_remaps(void)
{
 #ifdef WM_PROXY_SPRX
	{sys_map_path(VSH_MODULE_DIR WM_PROXY_SPRX ".sprx", file_exists(WM_RES_PATH "/wm_proxy.sprx") ? WM_RES_PATH "/wm_proxy.sprx" : NULL);}
 #endif

	if(payload_ps3hen)
	{
		{sys_map_path((char *)FB_XML,			(char *)"/dev_hdd0/xmlhost/game_plugin/fb-hen.xml");}
	//	{sys_map_path((char *)HEN_HFW_SETTINGS, (char *)"/dev_hdd0/hen/xml/hfw_settings.xml");}
	}

	{sys_map_path((char*)"/dev_bdvd/PS3_UPDATE", SYSMAP_EMPTY_DIR);} // redirect firmware update on BD disc to empty folder
}
#endif

static void make_fb_xml(void)
{
	sys_addr_t sysmem = NULL;
	if(sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK)
	{
		char *myxml = (char*)sysmem;
		u16 size = sprintf(myxml, "%s" // XML_HEADER
								  "<View id=\"seg_fb\">"
								  "<Attributes>"
								  "<Table key=\"mgames\">%s"
								  XML_PAIR("icon_notation","WNT_XmbItemSavePS3")
								  XML_PAIR("ingame","disable")
								  XML_PAIR("title","%s%s")
								  XML_PAIR("info","%s")
								  "</Table>"
								//"</Attributes><Items>"
								  "%s" QUERY_XMB("mgames", "xmb://localhost%s#seg_mygames") "%s",
								//"</Items></View>"
								//"</XMBML>"
								  XML_HEADER,
								  file_exists(WM_ICONS_PATH "/icon_wm_root.png") ?
										XML_PAIR("icon", WM_ICONS_PATH "/icon_wm_root.png") :
										XML_PAIR("icon_rsc", "item_tex_ps3util"),
								  STR_MYGAMES, SUFIX2(profile),
								  STR_LOADGAMES,
								  "</Attributes><Items>", MY_GAMES_XML, "</Items></View></XMBML>\r\n");

		char *fb_xml = (char *)FB_XML;
		#ifdef COBRA_ONLY
		if(payload_ps3hen)
		{
			fb_xml = (char *)"/dev_hdd0/xmlhost/game_plugin/fb-hen.xml";
			cellFsUnlink(FB_XML);
			sys_map_path((char *)FB_XML, (char *)fb_xml);
		}
		#endif

		save_file(fb_xml, myxml, size);
		sys_memory_free(sysmem);

		if(payload_ps3hen && IS_ON_XMB)
		{
			int view = View_Find("explore_plugin");
			if(view)
			{
				explore_interface = (explore_plugin_interface *)plugin_GetInterface(view, 1);
				explore_interface->ExecXMBcommand("reload_category game",0,0);
				explore_interface->ExecXMBcommand("reload_category network",0,0);
			}
		}
	}
}

static bool add_custom_xml(char *query_xmb)
{
	char *custom_xml = query_xmb + 800;
	for(u8 d = 0; d < 7; d++)
	{
		sprintf(custom_xml,  "%s/wm_custom.xml", drives[d]);
		if(file_exists(custom_xml))
		{
			sprintf(query_xmb, QUERY_XMB("wm_custom", "xmb://localhost%s#wm_root"), custom_xml);
			return true;
		}
	}
	return false;
}

static bool add_xmb_entry(u8 f0, u8 f1, int plen, const char *tempstr, char *templn, char *skey, u16 key, t_string *myxml_ps3, t_string *myxml_ps2, t_string *myxml_psx, t_string *myxml_psp, t_string *myxml_dvd, char *entry_name, u16 *item_count, u8 subfolder)
{
	set_sort_key(skey, templn, key, subfolder, f1);

	#define ITEMS_BUFFER(a)  (64 * (item_count[a] + 10))

	if( !scanning_roms && XMB_GROUPS )
	{
#ifdef COBRA_ONLY
		if(((IS_PS3_TYPE)   || ((IS_NTFS) && !extcmp(entry_name + plen, ".ntfs[PS3ISO]", 13))) && myxml_ps3->size < (BUFFER_SIZE - _4KB_ - ITEMS_BUFFER(gPS3)))
		{_concat(myxml_ps3, tempstr); *skey=PS3_, ++item_count[gPS3];}
		else
		if(((IS_PS2_FOLDER) || ((IS_NTFS) && !extcmp(entry_name + plen, ".ntfs[PS2ISO]", 13))) && myxml_ps2->size < (BUFFER_SIZE_PS2 - ITEMS_BUFFER(gPS2)))
		{_concat(myxml_ps2, tempstr); *skey=PS2, ++item_count[gPS2];}
		else
		if(((IS_PSX_FOLDER) || ((IS_NTFS) && !extcmp(entry_name + plen, ".ntfs[PSXISO]", 13))) && myxml_psx->size < (BUFFER_SIZE_PSX - ITEMS_BUFFER(gPSX)))
		{_concat(myxml_psx, tempstr); *skey=PS1, ++item_count[gPSX];}
		else
		if(((IS_PSP_FOLDER) || ((IS_NTFS) && !extcmp(entry_name + plen, ".ntfs[PSPISO]", 13))) && myxml_psp->size < (BUFFER_SIZE_PSP - ITEMS_BUFFER(gPSP)))
		{_concat(myxml_psp, tempstr); *skey=PSP, ++item_count[gPSP];}
		else
		if(((IS_BLU_FOLDER) || (IS_DVD_FOLDER) || ((IS_NTFS) && (!extcmp(entry_name + plen, ".ntfs[DVDISO]", 13) || !extcmp(entry_name + plen, ".ntfs[BDISO]", 12) || !extcmp(entry_name + plen, ".ntfs[BDFILE]", 13)))) && myxml_dvd->size < (BUFFER_SIZE_DVD - ITEMS_BUFFER(gDVD)))
		{_concat(myxml_dvd, tempstr); *skey=BLU, ++item_count[gDVD];}
#else
		if((IS_PS3_TYPE) && myxml_ps3->size < (BUFFER_SIZE - _4KB_ - ITEMS_BUFFER(gPS3)))
		{_concat(myxml_ps3, tempstr); *skey=PS3_, ++item_count[gPS3];}
		else
		if((IS_PS2_FOLDER) && myxml_ps2->size < (BUFFER_SIZE_PS2 - ITEMS_BUFFER(gPS2)))
		{_concat(myxml_ps2, tempstr); *skey=PS2, ++item_count[gPS2];}
#endif
		else
			return (false);
	}
	else
	{
		if(myxml_ps3->size < (BUFFER_SIZE  - ITEMS_BUFFER(gPS3) - _4KB_))
			{_concat(myxml_ps3, tempstr); ++item_count[gPS3];}
		else
			return (false);
	}

	return (true);
}

static bool scan_mygames_xml(u64 conn_s_p)
{
	if(conn_s_p == START_DAEMON)
	{
		if(webman_config->refr || from_reboot)
		{
			cellFsUnlink(WMNOSCAN);

			if(file_exists(MY_GAMES_XML))
			{
				make_fb_xml();
				return true; // skip refresh xml & mount autoboot
			}
		}

		// start a new thread for refresh xml content at start up
		if(!webman_config->refr || not_exists(MY_GAMES_XML))
		{
			sys_ppu_thread_t t_id;
			sys_ppu_thread_create(&t_id, handleclient_www, (u64)REFRESH_CONTENT, THREAD_PRIO, THREAD_STACK_SIZE_WEB_CLIENT, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_CMD);
		}

		return true; // mount autoboot & refresh xml
	}

	sys_addr_t sysmem = NULL;

#ifdef USE_VM
	if(sys_vm_memory_map(_32MB_, _1MB_, SYS_MEMORY_CONTAINER_ID_INVALID, SYS_MEMORY_PAGE_SIZE_64K, SYS_VM_POLICY_AUTO_RECOMMENDED, &sysmem) != CELL_OK)
	{
		return false;  //leave if cannot allocate memory
	}
#else
	if(webman_config->vsh_mc)
	{
		sys_memory_container_t vsh_mc = get_vsh_memory_container();
		if(vsh_mc && sys_memory_allocate_from_container(_3MB_, vsh_mc, SYS_MEMORY_PAGE_SIZE_1M, &sysmem) == CELL_OK) set_buffer_sizes(USE_MC);
	}

	if(!sysmem)
	{
		_meminfo meminfo;
		set_buffer_sizes(webman_config->foot);

		{system_call_1(SC_GET_FREE_MEM, (u64)(u32) &meminfo);}
		if( meminfo.avail<(BUFFER_SIZE_ALL+MIN_MEM)) set_buffer_sizes(3); //MIN+
		if (meminfo.avail<(BUFFER_SIZE_ALL+MIN_MEM)) set_buffer_sizes(1); //MIN
		if((meminfo.avail<(BUFFER_SIZE_ALL+MIN_MEM)) || sys_memory_allocate((BUFFER_SIZE_ALL), SYS_MEMORY_PAGE_SIZE_64K, &sysmem) != CELL_OK)
		{
			return false;  //leave if cannot allocate memory
		}
	}
#endif

	sys_addr_t sysmem_psx = sysmem + (BUFFER_SIZE);
	sys_addr_t sysmem_psp = sysmem + (BUFFER_SIZE) + (BUFFER_SIZE_PSX);
	sys_addr_t sysmem_ps2 = sysmem + (BUFFER_SIZE) + (BUFFER_SIZE_PSX) + (BUFFER_SIZE_PSP);
	sys_addr_t sysmem_dvd = sysmem + (BUFFER_SIZE) + (BUFFER_SIZE_PSX) + (BUFFER_SIZE_PSP) + (BUFFER_SIZE_PS2);
#ifdef MOUNT_ROMS
	sys_addr_t sysmem_igf = sysmem + (BUFFER_SIZE) + (BUFFER_SIZE_PSX) + (BUFFER_SIZE_PSP) + (BUFFER_SIZE_PS2) + (BUFFER_SIZE_DVD);

	char *ignore_files = NULL;
	if(file_exists(WMIGNORE_FILES))
	{
		ignore_files = (char*)sysmem_igf;
		read_file(WMIGNORE_FILES, ignore_files, _2KB_, 0);
		ignore_files[_2KB_] = NULL;
	}
#endif

#if defined(LAUNCHPAD) || defined(MOUNT_ROMS)
	char *sysmem_buf = (char*)sysmem;
#endif
	char *sysmem_xml = (char*)sysmem + (BUFFER_SIZE) - 4300;

	cellFsMkdir("/dev_hdd0/xmlhost", DMODE);
	cellFsMkdir("/dev_hdd0/xmlhost/game_plugin", DMODE);

	const u16 max_xmb_items = ((u16)(BUFFER_SIZE_ALL / AVG_ITEM_SIZE));

	make_fb_xml();

	#if defined(PKG_LAUNCHER) || defined(MOUNT_ROMS)
	bool c_roms = webman_config->roms;
	char *RETROARCH_DIR = NULL;
	if(c_roms)
	{
		RETROARCH_DIR = file_exists(RETROARCH_DIR1) ? (char*)RETROARCH_DIR1 : (char*)RETROARCH_DIR2;
		c_roms = isDir(PKGLAUNCH_DIR) && isDir(RETROARCH_DIR);
	}
	#endif

	u16 key;
	int fdxml; char *xml_file = (char*)MY_GAMES_XML;

	#ifdef PKG_LAUNCHER
	bool pkg_launcher = webman_config->ps3l && isDir(PKGLAUNCH_DIR);
	#endif
	bool ps2_launcher = webman_config->ps2l && isDir(PS2_CLASSIC_PLACEHOLDER);
	#ifdef COBRA_ONLY
	bool psp_launcher = webman_config->pspl && (isDir(PSP_LAUNCHER_MINIS) || isDir(PSP_LAUNCHER_REMASTERS));
	#endif
	char templn[1024];

	led(YELLOW, BLINK_FAST);

	check_cover_folders(templn);

	#ifdef MOUNT_ROMS
	#define ROM_PATHS	70
	const char roms_path[ROM_PATHS][12] = { "2048", "CAP32", "MAME", "MAME078", "MAME2000", "MAME2003", "FBA", "FBA2012", "FBNEO", "ATARI", "ATARI2600", "ATARI5200", "ATARI7800", "JAGUAR", "LYNX", "HANDY", "HATARI", "BOMBER", "NXENGINE", "AMIGA", "VICE", "DOSBOX", "GW", "DOOM", "QUAKE", "QUAKE2", "JAVAME", "LUA", "O2EM", "INTV", "BMSX", "FMSX", "NEOCD", "PCE", "PCFX", "SGX", "NGP", "NES", "FCEUMM", "NESTOPIA", "QNES", "GB", "GBC", "GAMBATTE", "TGBDUAL", "GBA", "GPSP", "VBOY", "VBA", "MGBA", "PALM", "POKEMINI", "GENESIS", "GEN", "MEGAD", "PICO", "GG", "GEARBOY", "ZX81", "FUSE", "SNES", "MSNES", "SNES9X", "SNES9X2005", "SNES9X2010", "SNES9X_NEXT", "THEODORE", "UZEM", "VECX", "WSWAM" };
	u16 roms_count[ROM_PATHS];
	u8 roms_index = 0;
	#endif

	#ifdef SLAUNCH_FILE
	int fdsl = create_slaunch_file();
	#endif

#ifdef MOUNT_ROMS
scan_roms:
#endif
	fdxml = key = 0;

	u16 item_count[6];
	for(u8 i = 0; i < 6; i++) item_count[i] = 0;

	t_string myxml_ps3; _alloc(&myxml_ps3, (char*)sysmem);
	t_string myxml_psx; _alloc(&myxml_psx, (char*)sysmem_psx);
	t_string myxml_ps2; _alloc(&myxml_ps2, (char*)sysmem_ps2);
	t_string myxml_psp; _alloc(&myxml_psp, (char*)sysmem_psp);
	t_string myxml_dvd; _alloc(&myxml_dvd, (char*)sysmem_dvd);
	t_string myxml    ; _alloc(&myxml,     (char*)sysmem_xml);
	t_string myxml_ngp; _alloc(&myxml_ngp, (char*)sysmem_dvd);

	// --- build group headers ---
	char *tempstr, *folder_name; tempstr = sysmem_xml; folder_name = sysmem_xml + (3*KB);

	if( !scanning_roms && XMB_GROUPS )
	{
		if(!(webman_config->cmask & PS3))
		{
			_concat(&myxml_ps3, "<View id=\"seg_wm_ps3_items\"><Attributes>");
			#ifdef PKG_LAUNCHER
			if(pkg_launcher)
			{
				sprintf(templn, "<Table key=\"pkg_launcher\">"
								XML_PAIR("icon", PKGLAUNCH_ICON)
								XML_PAIR("title","PKG Launcher")
								XML_PAIR("info","%s") "%s",
								"PKG Launcher", "</Table>"); _concat(&myxml_ps3, templn);
			}
			#endif
		}
		if(!(webman_config->cmask & PS2))
		{
			_concat(&myxml_ps2, "<View id=\"seg_wm_ps2_items\"><Attributes>");
			if(ps2_launcher)
			{
				#ifndef ENGLISH_ONLY
				char *STR_LAUNCHPS2 =  tempstr; //[48];//	= "Launch PS2 Classic";
				language("STR_LAUNCHPS2", STR_LAUNCHPS2, "Launch PS2 Classic");
				#endif
				sprintf(templn, "<Table key=\"ps2_classic_launcher\">"
								XML_PAIR("icon", PS2_CLASSIC_ISO_ICON)
								XML_PAIR("title","PS2 Classic Launcher")
								XML_PAIR("info","%s") "%s",
								STR_LAUNCHPS2, "</Table>"); _concat(&myxml_ps2, templn);
			}
		}
		#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1))
		{
			_concat(&myxml_psx, "<View id=\"seg_wm_psx_items\"><Attributes>");
		}
		if(!(webman_config->cmask & PSP))
		{
			_concat(&myxml_psp, "<View id=\"seg_wm_psp_items\"><Attributes>");
			if(psp_launcher)
			{
				#ifndef ENGLISH_ONLY
				char *STR_LAUNCHPSP =  tempstr; //[144];//	= "Launch PSP ISO mounted through webMAN or mmCM";
				language("STR_LAUNCHPSP", STR_LAUNCHPSP, "Launch PSP ISO mounted through webMAN or mmCM");
				#endif
				sprintf(templn, "<Table key=\"cobra_psp_launcher\">"
								XML_PAIR("icon","/dev_hdd0//game/%s/ICON0.PNG")
								XML_PAIR("title","PSP Launcher")
								XML_PAIR("info","%s") "%s",
								isDir(PSP_LAUNCHER_REMASTERS) ? PSP_LAUNCHER_REMASTERS_ID : PSP_LAUNCHER_MINIS_ID,
								STR_LAUNCHPSP, "</Table>"); _concat(&myxml_psp, templn);
			}
		}
		if(!(webman_config->cmask & DVD) || !(webman_config->cmask & BLU))
		{
			_concat(&myxml_dvd, "<View id=\"seg_wm_dvd_items\"><Attributes>");
			if(webman_config->rxvid)
			{
				sprintf(templn, "<Table key=\"rx_video\">"
								XML_PAIR("icon","%s")
								XML_PAIR("title","%s")
								XML_PAIR("child","segment") "%s",
								wm_icons[gDVD], STR_VIDLG, STR_NOITEM_PAIR); _concat(&myxml_dvd, templn);
			}
		}
		#endif
	}

	int fd;
	t_keys skey[max_xmb_items];

	char param[STD_PATH_LEN], icon[STD_PATH_LEN], subpath[STD_PATH_LEN], enc_dir_name[1024];

	u8 i0, is_net = 0;

	// --- scan xml content ---
	char localhost[24]; sprintf(localhost, "http://%s", local_ip);
	char *proxy_plugin = (char*)WEB_LINK_PAIR;
	#ifdef WM_PROXY_SPRX
	if(file_exists(WM_RES_PATH "/wm_proxy.sprx") && !(webman_config->wm_proxy)) {proxy_plugin = (char*)XAI_LINK_PAIR, *localhost = NULL;}
	#endif

	#if defined(PKG_LAUNCHER) || defined(MOUNT_ROMS)
	f1_len = ((webman_config->nogrp && c_roms) ? id_ROMS : webman_config->ps3l ? id_GAMEI : id_VIDEO) + 1;
	#endif

	int ns = NONE; u8 uprofile = profile;

	bool is_npdrm = webman_config->npdrm && (!isDir("/dev_hdd0/GAMEZ") && is_app_home_onxmb()); strcpy(paths[id_GAMEZ], is_npdrm ? "game" : "GAMEZ");

	#ifdef NET_SUPPORT
	int abort_connection = 0;
	if(g_socket >= 0 && open_remote_dir(g_socket, "/", &abort_connection) < 0) do_umount(false);
	#endif

	for(u8 f0 = 0; f0 < 16; f0++)  // drives: 0="/dev_hdd0", 1="/dev_usb000", 2="/dev_usb001", 3="/dev_usb002", 4="/dev_usb003", 5="/dev_usb006", 6="/dev_usb007", 7="/net0", 8="/net1", 9="/net2", 10="/net3", 11="/net4", 12="/ext", 13="/dev_sd", 14="/dev_ms", 15="/dev_cf"
	{
		if(check_drive(f0)) continue;

		i0 = f0, is_net = IS_NET;

		if(conn_s_p == START_DAEMON)
		{
			if(webman_config->boots && (f0 >= 1 && f0 <= 6)) // usb000->007
			{
				wait_for(drives[f0], webman_config->boots);
			}
		}

		#ifdef USE_NTFS
		if(!scanning_roms && IS_NTFS && webman_config->ntfs) prepNTFS(0);
		#endif

		if(!(is_net || IS_NTFS) && (isDir(drives[f0]) == false)) continue;

		#ifdef NET_SUPPORT
		if((ns >= 0) && (ns!=g_socket)) sclose(&ns);
		#endif

		ns = NONE; uprofile = profile;
		for(u8 f1 = 0; f1 < f1_len; f1++) // paths: 0="GAMES", 1="GAMEZ", 2="PS3ISO", 3="BDISO", 4="DVDISO", 5="PS2ISO", 6="PSXISO", 7="PSXGAMES", 8="PSPISO", 9="ISO", 10="video", 11="GAMEI", 12="ROMS"
		{
			if(scanning_roms)
			{
				f1 = id_ROMS;
			}
			else
			{
#ifndef COBRA_ONLY
				if(IS_ISO_FOLDER && !(IS_PS2_FOLDER)) continue; // 0="GAMES", 1="GAMEZ", 5="PS2ISO", 10="video"
#endif
				if(key >= max_xmb_items) break;

				//if(IS_PS2_FOLDER && f0>0)  continue; // PS2ISO is supported only from /dev_hdd0
#ifdef PKG_LAUNCHER
				if(IS_GAMEI_FOLDER) {if(is_net || (IS_HDD0) || (IS_NTFS)) continue;}
#endif
				if(IS_VIDEO_FOLDER) {if(is_net) continue; else strcpy(paths[id_VIDEO], (IS_HDD0) ? "video" : "GAMES_DUP");}
				if(IS_NTFS)  {if(f1 >= id_ISO) break; else if(IS_JB_FOLDER || (f1 == id_PSXGAMES)) continue;} // 0="GAMES", 1="GAMEZ", 7="PSXGAMES", 9="ISO", 10="video", 11="GAMEI", 12="ROMS"

				if(is_npdrm && !IS_HDD0) {is_npdrm = false; strcpy(paths[id_GAMEZ], "GAMEZ");}

#ifdef NET_SUPPORT
				if(is_net)
				{
					if(f1 >= id_ISO) break; // ignore 9="ISO", 10="video", 11="GAMEI"
				}
#endif
				if(check_content_type(f1)) continue;
			}

#ifdef NET_SUPPORT
			if(is_net && (netiso_svrid == (f0-7)) && (g_socket != -1)) ns = g_socket; /* reuse current server connection */ else
			if(is_net && (ns<0)) ns = connect_to_remote_server(f0-7);
#endif
			if(is_net && (ns<0)) break;

			bool ls; u8 li, subfolder; li=subfolder=0; ls=false; // single letter folder

		subfolder_letter_xml:
			subfolder = 0; uprofile = profile;
		read_folder_xml:
//
			#ifdef MOUNT_ROMS
			if(scanning_roms)
			{
				if(is_net)
					{sprintf(param, "/ROMS%s/%s", SUFIX(uprofile), roms_path[roms_index]);}
				else if(IS_NTFS)
					{sprintf(param, "%s/USRDIR/cores/roms/%s", RETROARCH_DIR, roms_path[roms_index]); i0 = 0;}
				else
					{sprintf(param, "%s/ROMS%s/%s", drives[f0], SUFIX(uprofile), roms_path[roms_index]);}
			}
			else
			#endif
			#ifdef NET_SUPPORT
			if(is_net)
			{
				char ll[4]; if(li) sprintf(ll, "/%c", '@'+li); else *ll = NULL;
				sprintf(param, "/%s%s%s",    paths[f1], SUFIX(uprofile), ll);

				if(li == 99) sprintf(param, "/%s%s", paths[f1], AUTOPLAY_TAG);
			}
			else
			#endif
			{
				if(IS_NTFS)
					sprintf(param, "%s", WMTMP);
				else
				{
					sprintf(param, "%s/%s%s", drives[f0], paths[f1], SUFIX(uprofile));
					if(li == 99) sprintf(param, "%s/%s%s", drives[f0], paths[f1], AUTOPLAY_TAG);
				}
			}

#ifdef NET_SUPPORT
			if(is_net && open_remote_dir(ns, param, &abort_connection) < 0) goto continue_reading_folder_xml; //continue;
#endif
			{
				CellFsDirectoryEntry entry; u32 read_e;
				int fd2 = 0, flen, plen;
				char title_id[12];
				u8 is_iso = 0;

#ifdef NET_SUPPORT
				sys_addr_t data2 = NULL;
				int v3_entries, v3_entry; v3_entries=v3_entry = 0;
				netiso_read_dir_result_data *data = NULL; char neth[8];
				if(is_net)
				{
					v3_entries = read_remote_dir(ns, &data2, &abort_connection);
					if(!data2) goto continue_reading_folder_xml; //continue;
					data = (netiso_read_dir_result_data*)data2; sprintf(neth, "/net%i", (f0-7));
				}
#endif
				if(!is_net && isDir(param) == false) goto continue_reading_folder_xml; //continue;
				if(!is_net && cellFsOpendir(param, &fd) != CELL_FS_SUCCEEDED) goto continue_reading_folder_xml; //continue;

				plen = strlen(param);

				while((!is_net && (!cellFsGetDirectoryEntries(fd, &entry, sizeof(entry), &read_e) && read_e > 0))
#ifdef NET_SUPPORT
					|| (is_net && (v3_entry < v3_entries))
#endif
					)
				{
					if(key >= max_xmb_items) break; if(is_npdrm && (f1 == id_GAMEZ) && !islike(entry.entry_name.d_name, "NP")) continue;
#ifdef NET_SUPPORT
					if(is_net)
					{
						#ifdef MOUNT_ROMS
						if(scanning_roms && ignore_files && (strstr(ignore_files, data[v3_entry].name) != NULL)) continue;
						#endif

						if((ls == false) && (li==0) && (f1>1) && (data[v3_entry].is_directory) && (data[v3_entry].name[1] == NULL)) ls = true; // single letter folder was found

						if(add_net_game(ns, data, v3_entry, neth, param, templn, tempstr, enc_dir_name, icon, title_id, f1, 0) == FAILED) {v3_entry++; continue;}
#ifdef SLAUNCH_FILE
						if(key < MAX_SLAUNCH_ITEMS) add_slaunch_entry(fdsl, neth, param, data[v3_entry].name, icon, templn, title_id, f1);
#endif
						read_e = sprintf(tempstr, "<Table key=\"%04i\">"
										 XML_PAIR("icon","%s")
										 XML_PAIR("title","%s") "%s"
										 XML_PAIR("module_action","%s/mount_ps3%s%s/%s"),
										 key, icon,
										 templn, proxy_plugin, localhost, neth, param, enc_dir_name);

						// info level: 0=Path, 1=Path | titleid, 2=titleid | drive, 3=none
						if(webman_config->info <= 1)
						{
							if((webman_config->info == 1) & HAS_TITLE_ID) {sprintf(folder_name, " | %s" , title_id);} else *folder_name = NULL;
							read_e += sprintf(tempstr + read_e, XML_PAIR("info","%s%s%s"), neth, param, folder_name);
						}
						else if(webman_config->info == 2)
						{
							if(HAS_TITLE_ID)
								read_e += sprintf(tempstr + read_e, XML_PAIR("info","%s | %s"), title_id, drives[f0] + 1);
							else
								read_e += sprintf(tempstr + read_e, XML_PAIR("info","%s"), drives[f0] + 1);

							if(f1 < 2) read_e += sprintf(tempstr + read_e, " | JB");
						}

						sprintf(tempstr + read_e, "</Table>");

						if(add_xmb_entry(f0, f1, plen + 6, tempstr, templn, skey[key].value, key, &myxml_ps3, &myxml_ps2, &myxml_psx, &myxml_psp, &myxml_dvd, data[v3_entry].name, item_count, 0)) key++;

						v3_entry++;
					}
					else
#endif // #ifdef NET_SUPPORT
					{
						if(entry.entry_name.d_name[0] == '.') continue;

						#ifdef MOUNT_ROMS
						if(scanning_roms && ignore_files && (strstr(ignore_files, entry.entry_name.d_name) != NULL)) continue;
						#endif

//////////////////////////////
						subfolder = 0;
						if(IS_ISO_FOLDER || IS_VIDEO_FOLDER)
						{
							sprintf(subpath, "%s/%s", param, entry.entry_name.d_name);
							if(isDir(subpath) && cellFsOpendir(subpath, &fd2) == CELL_FS_SUCCEEDED)
							{
								strcpy(subpath, entry.entry_name.d_name); subfolder = 1;
next_xml_entry:
								cellFsGetDirectoryEntries(fd2, &entry, sizeof(entry), &read_e);
								if(read_e < 1) {cellFsClosedir(fd2); fd2 = 0; continue;}
								if(entry.entry_name.d_name[0] == '.') goto next_xml_entry;
								entry.entry_name.d_namlen = sprintf(templn, "%s/%s", subpath, entry.entry_name.d_name);
								strcpy(entry.entry_name.d_name, templn);
							}
						}
//////////////////////////////

						if(key >= max_xmb_items) break;

						flen = entry.entry_name.d_namlen; is_iso = is_iso_file(entry.entry_name.d_name, flen, f1, f0);

						if(IS_JB_FOLDER)
						{
#ifdef PKG_LAUNCHER
							if(IS_GAMEI_FOLDER)
							{
								// create game folder in /dev_hdd0/game and copy PARAM.SFO to prevent deletion of XMB icon when gameDATA is disabled
								char *param_sfo = tempstr;
								sprintf(param_sfo, "%s/%s/PARAM.SFO", _HDD0_GAME_DIR, entry.entry_name.d_name);
								if(not_exists(param_sfo))
								{
									char *_param_sfo = templn; // GAMEI
									sprintf(_param_sfo, "%s/%s/PARAM.SFO", param, entry.entry_name.d_name);
									mkdir_tree(param_sfo); file_copy(_param_sfo, param_sfo, COPY_WHOLE_FILE);
								}

								if(!webman_config->ps3l) continue;

								sprintf(templn, "%s/%s/USRDIR/EBOOT.BIN", param, entry.entry_name.d_name); if(not_exists(templn)) continue;
								sprintf(templn, "%s/%s/PARAM.SFO", param, entry.entry_name.d_name);
							}
							else
#endif
							if(is_npdrm && (f1 == id_GAMEZ))
								read_e = sprintf(templn, "%s/%s/USRDIR/EBOOT.BIN", param, entry.entry_name.d_name);
							else
							{
								sprintf(templn, "%s/%s/PS3_GAME/PARAM.SFO", param, entry.entry_name.d_name);
								check_ps3_game(templn);
							}
						}

						if(is_iso || (IS_JB_FOLDER && file_exists(templn)))
						{
							*icon = *title_id = NULL;

							if(!is_iso)
							{
								if(is_npdrm && (f1 == id_GAMEZ)) sprintf(templn + read_e - 17, "/PARAM.SFO");
								get_title_and_id_from_sfo(templn, title_id, entry.entry_name.d_name, icon, tempstr, 0);
							}
							else
							{
#ifndef COBRA_ONLY
								get_name(templn, entry.entry_name.d_name, NO_EXT);
#else
								if(get_name_iso_or_sfo(templn, title_id, icon, param, entry.entry_name.d_name, f0, f1, uprofile, flen, tempstr) == FAILED) continue;
#endif
							}

							get_default_icon(icon, param, entry.entry_name.d_name, !is_iso, title_id, ns, f0, f1);
#ifdef SLAUNCH_FILE
							if(key < MAX_SLAUNCH_ITEMS) add_slaunch_entry(fdsl, "", param, entry.entry_name.d_name, icon, templn, title_id, f1);
#endif
							if(webman_config->tid && HAS_TITLE_ID && strlen(templn) < 50 && strstr(templn, " [") == NULL) {sprintf(enc_dir_name, " [%s]", title_id); strcat(templn, enc_dir_name);}

							urlenc(enc_dir_name, entry.entry_name.d_name);

							// subfolder name
							if((IS_NTFS) && entry.entry_name.d_name[0] == '[')
							{
								strcpy(folder_name, entry.entry_name.d_name); *folder_name = '/'; char *p = strstr(folder_name, "] "); if(p) *p = NULL;
							}
							else
							{
								*folder_name = NULL;
								char *p = strchr(entry.entry_name.d_name, '/'); if(p) {*p = NULL; sprintf(folder_name, "/%s", entry.entry_name.d_name); *p = '/';}
							}

							read_e = sprintf(tempstr, "<Table key=\"%04i\">"
											 XML_PAIR("icon","%s")
											 XML_PAIR("title","%s") "%s"
											 XML_PAIR("module_action","%s/mount_ps3%s%s/%s"),
											 key, icon,
											 templn, proxy_plugin, localhost, "", param, enc_dir_name);

							// info level: 0=Path, 1=Path | titleid, 2=titleid | drive, 3=none
							if(webman_config->info <= 1)
							{
								if((webman_config->info == 1) & HAS_TITLE_ID) {strcat(folder_name, " | "); strcat(folder_name, title_id);}
								read_e += sprintf(tempstr + read_e, XML_PAIR("info","%s/%s%s"), drives[i0], paths[f1], folder_name);
							}
							else if(webman_config->info == 2)
							{
								if(HAS_TITLE_ID)
									read_e += sprintf(tempstr + read_e, XML_PAIR("info","%s | %s"), title_id, drives[i0] + 5);
								else
									read_e += sprintf(tempstr + read_e, XML_PAIR("info","%s"), drives[i0] + 5);

								if(f1 < 2) read_e += sprintf(tempstr + read_e, " | JB");
							}

							sprintf(tempstr + read_e, "</Table>");

							if(add_xmb_entry(f0, f1, plen + flen - 13, tempstr, templn, skey[key].value, key, &myxml_ps3, &myxml_ps2, &myxml_psx, &myxml_psp, &myxml_dvd, entry.entry_name.d_name, item_count, subfolder)) key++;
						}
//////////////////////////////
						if(subfolder) goto next_xml_entry;
//////////////////////////////
					}
				}

				if(!is_net) cellFsClosedir(fd);

#ifdef NET_SUPPORT
				if(data2) {sys_memory_free((sys_addr_t)data2); data2 = NULL;}
#endif
			}
//
continue_reading_folder_xml:
			if(f1 < id_ISO && !IS_NTFS)
			{
				if(uprofile > 0) {subfolder = uprofile = 0; goto read_folder_xml;}
				if(is_net && (f1 > id_GAMEZ))
				{
					if(ls && (li < 27)) {li++; goto subfolder_letter_xml;} else if(li < 99) {li = 99; goto subfolder_letter_xml;}
				}
			}
//
		}
#ifdef NET_SUPPORT
		if(is_net && (ns >= 0) && (ns!=g_socket)) sclose(&ns);
#endif
	}

	if( !scanning_roms && XMB_GROUPS )
	{
#ifndef PKG_LAUNCHER
		if(!(webman_config->cmask & PS3)) {_concat(&myxml_ps3, "</Attributes><Items>");}
#else
		if(!(webman_config->cmask & PS3)) {_concat(&myxml_ps3, "</Attributes><Items>"); if(pkg_launcher) _concat(&myxml_ps3, QUERY_XMB("pkg_launcher", "xcb://localhost/query?limit=1&cond=Ae+Game:Common.dirPath /dev_hdd0//game+Ae+Game:Common.fileName " PKGLAUNCH_ID));}
#endif
		if(!(webman_config->cmask & PS2)) {_concat(&myxml_ps2, "</Attributes><Items>"); if(ps2_launcher) _concat(&myxml_ps2, QUERY_XMB("ps2_classic_launcher", "xcb://127.0.0.1/query?limit=1&cond=Ae+Game:Game.titleId PS2U10000"));}

#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1)) {_concat(&myxml_psx, "</Attributes><Items>");}
		if(!(webman_config->cmask & PSP)) {_concat(&myxml_psp, "</Attributes><Items>"); if(psp_launcher) _concat(&myxml_psp, QUERY_XMB("cobra_psp_launcher", "xcb://127.0.0.1/query?cond=AGL+Game:Game.titleId " PSP_LAUNCHER_REMASTERS_ID " " PSP_LAUNCHER_MINIS_ID));}
		if(!(webman_config->cmask & DVD) ||
		   !(webman_config->cmask & BLU)) {_concat(&myxml_dvd, "</Attributes><Items>"); if(webman_config->rxvid) _concat(&myxml_dvd, QUERY_XMB("rx_video", "#seg_wm_bdvd"));}
#endif
	}
	else
		_alloc(&myxml_ngp, (char*)sysmem_dvd);

	// --- sort scanned content

	led(YELLOW, OFF);
	led(GREEN, ON);

	if(key)
	{   // sort xmb items
		u16 m, n;
		t_keys swap;
		for(n = 0; n < (key - 1); n++)
			for(m = (n + 1); m < key; m++)
				if(strncmp(skey[n].value, skey[m].value, XML_KEY_LEN) > 0)
				{
					swap    = skey[n];
					skey[n] = skey[m];
					skey[m] = swap;
				}
	}

	// --- add eject & setup/xmbm+ menu
#ifdef ENGLISH_ONLY
	bool add_xmbm_plus = file_exists(XMBMANPLS_PATH "/FEATURES/webMAN.xml");
#else
	bool add_xmbm_plus = false;

	while(true)
	{
		sprintf(templn, "%s/FEATURES/webMAN%s.xml", XMBMANPLS_PATH, lang_code);
		add_xmbm_plus = file_exists(templn);
		if(add_xmbm_plus || *lang_code == NULL) break; *lang_code = NULL;
	}
#endif

	if(!scanning_roms && webman_config->nogrp)
	{
		if(!add_xmbm_plus) _concat(&myxml_ngp, ADD_XMB_ITEM("eject"));

		if( ADD_SETUP )
		{
			if(add_xmbm_plus)
#ifdef ENGLISH_ONLY
				_concat(&myxml_ngp, QUERY_XMB("setup", "xmb://localhost" XMBMANPLS_PATH "/FEATURES/webMAN.xml#seg_webman_links_items"));
#else
			{
				sprintf(tempstr, QUERY_XMB("setup", "xmb://localhost%s#seg_webman_links_items"), templn);
				_concat(&myxml_ngp, tempstr);
			}
#endif
			else
				_concat(&myxml_ngp, ADD_XMB_ITEM("setup"));
		}

		if(add_custom_xml(templn)) _concat(&myxml_ngp, templn);
	}

	// --- add sorted items to xml
	if( webman_config->nogrp || scanning_roms)
	{
		u32 max_size = (BUFFER_SIZE - 1000);

		for(u16 a = 0; a < key; a++)
		{
			if(myxml_ngp.size >= max_size) break;
			sprintf(templn, ADD_XMB_ITEM("%s"), skey[a].value + XML_KEY_LEN, skey[a].value + XML_KEY_LEN);
			_concat(&myxml_ngp, templn);
		}
	}
	else
	{
		u32 max_size = (BUFFER_SIZE - 5000);

		for(u16 a = 0; a < key; a++)
		{
			sprintf(templn, ADD_XMB_ITEM("%s"), skey[a].value + XML_KEY_LEN, skey[a].value + XML_KEY_LEN);
			if(*skey[a].value == PS3_&& myxml_ps3.size < max_size)
				_concat(&myxml_ps3, templn);
			else
			if(*skey[a].value == PS2 && myxml_ps2.size < (BUFFER_SIZE_PS2 - 128))
				_concat(&myxml_ps2, templn);
#ifdef COBRA_ONLY
			else
			if(*skey[a].value == PS1 && myxml_psx.size < (BUFFER_SIZE_PSX - 128))
				_concat(&myxml_psx, templn);
			else
			if(*skey[a].value == PSP && myxml_psp.size < (BUFFER_SIZE_PSP - 128))
				_concat(&myxml_psp, templn);
			else
			if(*skey[a].value == BLU && myxml_dvd.size < (BUFFER_SIZE_DVD - 1200))
				_concat(&myxml_dvd, templn);
#endif
		}
	}

	// --- build xml headers
#ifdef MOUNT_ROMS
	myxml.size = sprintf(myxml.str, "%s"
						"<View id=\"%s%s\">"
						"<Attributes>", XML_HEADER, scanning_roms ? "seg_wm_rom_" : "seg_mygames", scanning_roms ? roms_path[roms_index] : "" );

	if(scanning_roms)
	{
		xml_file = enc_dir_name;
		sprintf(xml_file, "%s/ROMS_%s.xml", HTML_BASE_PATH, roms_path[roms_index]);
		cellFsUnlink(xml_file);
		goto save_xml;
	}
#else
	myxml.size = sprintf(myxml.str, "%s"
						"<View id=\"%s\">"
						"<Attributes>", XML_HEADER, "seg_mygames");
#endif

	// --- close view of xml groups
	if( XMB_GROUPS )
	{
		if(!(webman_config->cmask & PS3)) _concat(&myxml_ps3, "</Items></View>");
		if(!(webman_config->cmask & PS2)) _concat(&myxml_ps2, "</Items></View>");
		#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1)) _concat(&myxml_psx, "</Items></View>");
		if(!(webman_config->cmask & PSP)) _concat(&myxml_psp, "</Items></View>");
		if(!(webman_config->cmask & DVD) || !(webman_config->cmask & BLU))
		{
			_concat(&myxml_dvd, "</Items></View>");
			if(webman_config->rxvid)
			{
				_concat(&myxml_dvd,
					"<View id=\"seg_wm_bdvd\">"
					"<Items>"
					QUERY_XMB("rx_video1",	"xcb://localhost/query?table=MMS_MEDIA_TYPE_SYSTEM"
											"&genre=Video&sort=+StorageMedia:StorageMedia.sortOrder+StorageMedia:StorageMedia.timeInserted"
											"&cond=Ae+StorageMedia:StorageMedia.stat.mediaStatus %xCB_MEDIA_INSERTED"
												 "+Ae+StorageMedia:StorageMedia.mediaFormat %xCB_MEDIA_FORMAT_DATA"
												 "+AGL+StorageMedia:StorageMedia.type %xCB_MEDIA_TYPE_BDROM %xCB_MEDIA_TYPE_WM")
					QUERY_XMB("rx_video2",	"xcb://localhost/query?sort=+Game:Common.titleForSort"
											"&cond=AGL+Game:Game.titleId RXMOV0000 RXMOVZZZZ+An+Game:Game.category 2D"
												 "+An+Game:Game.category BV"
												 "+An+Game:Game.category HG")
					"</Items>"
					"</View>");
			}
		}
		#endif
	}

	char *buffer = enc_dir_name;

	// --- Add Eject Disc if webMAN.xml does not exist
	if(!add_xmbm_plus)
	{
		#ifndef ENGLISH_ONLY
		char *STR_EJECTDISC = buffer; //[32];//	= "Eject Disc";
		language("STR_EJECTDISC", STR_EJECTDISC, "Eject Disc");
		#endif

		sprintf(templn, "<Table key=\"eject\">"
						XML_PAIR("icon","%s")
						XML_PAIR("title","%s")
						XML_PAIR("info","%s") "%s"
						XML_PAIR("module_action","%s/mount_ps3/unmount") "</Table>",
						wm_icons[11], STR_EJECTDISC, STR_UNMOUNTGAME, proxy_plugin, localhost);
		_concat(&myxml, templn);
	}

	// --- Add Groups attribute tables
	if( XMB_GROUPS )
	{
		#ifndef ENGLISH_ONLY
		char *STR_PS3FORMAT = buffer; //[40];//	= "PS3 format games";
		language("STR_PS3FORMAT", STR_PS3FORMAT, "PS3 format games");
		#endif
		if( !(webman_config->cmask & PS3)) {sprintf(templn, "<Table key=\"wm_ps3\">"
													XML_PAIR("icon","%s")
													XML_PAIR("title","PLAYSTATION\xC2\xAE\x33")
													XML_PAIR("info","%'i %s") "%s",
													wm_icons[gPS3], item_count[gPS3], STR_PS3FORMAT,
													STR_NOITEM_PAIR); _concat(&myxml, templn);}
		#ifdef MOUNT_ROMS
		if(c_roms)
		{
			#ifndef ENGLISH_ONLY
			char *pos = strstr(buffer, "PS3"); if(pos) strncpy(pos, "ROM", 3); else *buffer = NULL;
			#else
			*buffer = NULL;
			#endif
											sprintf(templn, "<Table key=\"wm_rom\">"
													XML_PAIR("icon%s", "%s")
													XML_PAIR("title","ROMS")
													XML_PAIR("info","%s") "%s",
													covers_exist[7] ? "" : "_rsc",
													covers_exist[7] ? WM_ICONS_PATH "/icon_wm_album_emu.png" : "item_tex_ps3util",
													buffer, STR_NOITEM_PAIR); _concat(&myxml, templn);
		}
		#endif
		#ifndef ENGLISH_ONLY
		char *STR_PS2FORMAT = buffer; //[48];//	= "PS2 format games";
		language("STR_PS2FORMAT", STR_PS2FORMAT, "PS2 format games");
		#endif
		if( !(webman_config->cmask & PS2)) {sprintf(templn, "<Table key=\"wm_ps2\">"
													XML_PAIR("icon","%s")
													XML_PAIR("title","PLAYSTATION\xC2\xAE\x32")
													XML_PAIR("info","%'i %s") "%s",
													wm_icons[gPS2], item_count[gPS2], STR_PS2FORMAT,
													STR_NOITEM_PAIR); _concat(&myxml, templn);}
	 #ifdef COBRA_ONLY

		#ifndef ENGLISH_ONLY
		char *STR_PS1FORMAT = buffer; //[48];//	= "PSOne format games";
		language("STR_PS1FORMAT", STR_PS1FORMAT, "PSOne format games");
		#endif
		if( !(webman_config->cmask & PS1)) {sprintf(templn, "<Table key=\"wm_psx\">"
													XML_PAIR("icon","%s")
													XML_PAIR("title","PLAYSTATION\xC2\xAE")
													XML_PAIR("info","%'i %s") "%s",
													wm_icons[gPSX], item_count[gPSX], STR_PS1FORMAT,
													STR_NOITEM_PAIR); _concat(&myxml, templn);}
		#ifndef ENGLISH_ONLY
		char *STR_PSPFORMAT = buffer; //[48];//	= "PSP\xE2\x84\xA2 format games";
		language("STR_PSPFORMAT", STR_PSPFORMAT, "PSP\xE2\x84\xA2 format games");
		#endif
		if( !(webman_config->cmask & PSP)) {sprintf(templn, "<Table key=\"wm_psp\">"
													XML_PAIR("icon","%s")
													XML_PAIR("title","PLAYSTATION\xC2\xAEPORTABLE")
													XML_PAIR("info","%'i %s") "%s",
													wm_icons[gPSP], item_count[gPSP], STR_PSPFORMAT,
													STR_NOITEM_PAIR); _concat(&myxml, templn);}
		#ifndef ENGLISH_ONLY
		char *STR_VIDFORMAT = buffer; //[56];//	= "Blu-ray\xE2\x84\xA2 and DVD";
		char *STR_VIDEO = buffer + 100; //[40];//	= "Video content";

		language("STR_VIDFORMAT", STR_VIDFORMAT, "Blu-ray\xE2\x84\xA2 and DVD");
		language("STR_VIDEO", STR_VIDEO, "Video content");
		#endif
		if( !(webman_config->cmask & DVD) ||
			!(webman_config->cmask & BLU)) {sprintf(templn, "<Table key=\"wm_dvd\">"
													XML_PAIR("icon","%s")
													XML_PAIR("title","%s")
													XML_PAIR("info","%'i %s") "%s",
													wm_icons[gDVD], STR_VIDFORMAT, item_count[gDVD], STR_VIDEO,
													STR_NOITEM_PAIR); _concat(&myxml, templn);}
	 #endif
		if(add_custom_xml(templn)) {sprintf(templn, "<Table key=\"wm_custom\">"
											XML_PAIR("icon_rsc","item_tex_ps3util")
											XML_PAIR("title","%s") "</Table>",
											"XML"); _concat(&myxml, templn);}
	}

	// --- Add Setup
	if( ADD_SETUP )
	{
		#ifndef ENGLISH_ONLY
		char *STR_WMSETUP2 = buffer; //[56];//	= "Setup webMAN options";
		language("STR_WMSETUP2", STR_WMSETUP2, "Setup webMAN options");
		#endif
		sprintf(templn, "<Table key=\"setup\">"
						 XML_PAIR("icon","%s")
						 XML_PAIR("title","%s")
						 XML_PAIR("info","%s") "%s",
						 add_xmbm_plus ? XMBMANPLS_PATH "/IMAGES/multiman.png" : wm_icons[10],
						 STR_WMSETUP, STR_WMSETUP2, WEB_LINK_PAIR); _concat(&myxml, templn);

		if(add_xmbm_plus)
			_concat(&myxml, XML_PAIR("child","segment"));
		else
			{sprintf(templn, XML_PAIR("module_action","%s/setup.ps3"), localhost); _concat(&myxml, templn);}

		_concat(&myxml, "</Table>");
	}

	#ifndef ENGLISH_ONLY
	close_language();
	#endif

	// --- Add groups queries (setup + eject + categories)
	if( XMB_GROUPS )
	{
		_concat(&myxml, "</Attributes><Items>");
		if( ADD_SETUP )
		{
			if(add_xmbm_plus)
			#ifdef ENGLISH_ONLY
				_concat(&myxml, QUERY_XMB("setup", "xmb://localhost" XMBMANPLS_PATH "/FEATURES/webMAN.xml#seg_webman_links_items"));
			#else
			{
				sprintf(templn, QUERY_XMB("setup", "xmb://localhost" XMBMANPLS_PATH "/FEATURES/webMAN%s.xml#seg_webman_links_items"), lang_code);
				_concat(&myxml, templn);
			}
			#endif
			else
				_concat(&myxml, ADD_XMB_ITEM("setup"));
		}

		if(!add_xmbm_plus) _concat(&myxml, ADD_XMB_ITEM("eject"));

		if(!(webman_config->cmask & PS3)) _concat(&myxml, QUERY_XMB("wm_ps3", "#seg_wm_ps3_items"));
		if(!(webman_config->cmask & PS2)) _concat(&myxml, QUERY_XMB("wm_ps2", "#seg_wm_ps2_items"));
	 #ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1)) _concat(&myxml, QUERY_XMB("wm_psx", "#seg_wm_psx_items"));
		if(!(webman_config->cmask & PSP)) _concat(&myxml, QUERY_XMB("wm_psp", "#seg_wm_psp_items"));
		if(!(webman_config->cmask & DVD) ||
		   !(webman_config->cmask & BLU)) _concat(&myxml, QUERY_XMB("wm_dvd", "#seg_wm_dvd_items"));
		#ifdef MOUNT_ROMS
		if(  c_roms                     ) _concat(&myxml, QUERY_XMB("wm_rom", "xmb://localhost" HTML_BASE_PATH "/ROMS.xml#seg_wm_rom_items"));
		#endif
	 #endif
		if(add_custom_xml(templn)) _concat(&myxml, templn);
		_concat(&myxml, "</Items></View>");
	}

#ifdef MOUNT_ROMS
save_xml:
	if(scanning_roms && (key == 0)) ; // do not create empty xml
	else
#endif
	// --- save xml file
	if(cellFsOpen(xml_file, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fdxml, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		cellFsWrite(fdxml, (char*)myxml.str, myxml.size, NULL);

		int slen;

		if(scanning_roms || webman_config->nogrp)
		{
			cellFsWrite(fdxml, (char*)myxml_ps3.str, myxml_ps3.size, NULL);
			cellFsWrite(fdxml, (char*)"</Attributes><Items>", 20, NULL);
			cellFsWrite(fdxml, (char*)myxml_ngp.str, myxml_ngp.size, NULL);

			slen = sprintf(buffer, "</Items></View></XMBML>\r\n");
		}
		else
		{
			if(!(webman_config->cmask & PS3)) cellFsWrite(fdxml, (char*)myxml_ps3.str, myxml_ps3.size, NULL);
			if(!(webman_config->cmask & PS2)) cellFsWrite(fdxml, (char*)myxml_ps2.str, myxml_ps2.size, NULL);
#ifdef COBRA_ONLY
			if(!(webman_config->cmask & PS1)) cellFsWrite(fdxml, (char*)myxml_psx.str, myxml_psx.size, NULL);
			if(!(webman_config->cmask & PSP)) cellFsWrite(fdxml, (char*)myxml_psp.str, myxml_psp.size, NULL);
			if(!(webman_config->cmask & DVD) ||
			   !(webman_config->cmask & BLU)) cellFsWrite(fdxml, (char*)myxml_dvd.str, myxml_dvd.size, NULL);
#endif
			slen = sprintf(buffer, "</XMBML>\r\n");
		}

		cellFsWrite(fdxml, (char*)buffer, slen, NULL);
		cellFsClose(fdxml);

		cellFsChmod(MY_GAMES_XML, MODE);
	}

#ifdef MOUNT_ROMS
	if(scanning_roms || (c_roms && XMB_GROUPS))
	{
		if(scanning_roms)
		{
			roms_count[roms_index] = key;
			roms_index++;
		}
		else
		{
			scanning_roms = true;
			cellFsUnlink(HTML_BASE_PATH "/ROMS.xml");
		}

		if(roms_index < ROM_PATHS) goto scan_roms; // loop scanning_roms
		scanning_roms = false;


		// ---- Build ROMS.xml
		_alloc(&myxml, sysmem_buf);
		_concat(&myxml, XML_HEADER);
		_concat(&myxml, "<View id=\"seg_wm_rom_items\"><Attributes>");

		#ifndef ENGLISH_ONLY
		close_language(); lang_roms = 1;
		#endif

		// ---- Add roms categories
		for(u8 i = 0; i < ROM_PATHS; i++)
		{
			if(roms_count[i])
			{
				#ifndef ENGLISH_ONLY
				language(roms_path[i], tempstr, roms_path[i]);
				#endif

				sprintf(templn, "<Table key=\"%s\">"
								XML_PAIR("icon%s", "%s")
								XML_PAIR("title","%s")
								XML_PAIR("info","%'i %s")
								"</Table>",
								roms_path[i],
								covers_exist[7] ? "" : "_rsc",
								covers_exist[7] ? WM_ICONS_PATH "/icon_wm_album_emu.png" : "item_tex_ps3util",
								#ifndef ENGLISH_ONLY
								fh ? tempstr :
								#endif
								roms_path[i], roms_count[i], (roms_count[i] == 1) ? "ROM" : "ROMS"); _concat(&myxml, templn);
			}
		}

		#ifndef ENGLISH_ONLY
		close_language();
		#endif

		_concat(&myxml, "</Attributes><Items>");

		// ---- Add roms queries
		for(u8 i = 0; i < ROM_PATHS; i++)
		{
			if(roms_count[i])
			{
				sprintf(templn, QUERY_XMB("%s", "xmb://localhost%s/ROMS_%s.xml#seg_wm_rom_%s"), roms_path[i], roms_path[i], HTML_BASE_PATH, roms_path[i], roms_path[i]);
				_concat(&myxml, templn);
			}
		}

		_concat(&myxml, "</Items></View></XMBML>\r\n");

		save_file(HTML_BASE_PATH "/ROMS.xml", myxml.str, myxml.size);
	}
#endif

#ifdef SLAUNCH_FILE
	close_slaunch_file(fdsl);
#endif

#ifdef LAUNCHPAD
	bool launchpad_xml = !(webman_config->launchpad_xml) && file_exists(LAUNCHPAD_FILE_XML);

	if(launchpad_xml)
	{
		*sysmem_buf = *param = *tempstr = *templn = NULL;
		game_listing(sysmem_buf, templn, param, tempstr, LAUNCHPAD_MODE, false);
	}
#endif

	led(GREEN, ON);

	// --- release allocated memory
#ifdef USE_VM
	sys_vm_unmap(sysmem);
#else
	sys_memory_free(sysmem);
#endif

	return false;
}

static void update_xml_thread(u64 conn_s_p)
{
	refreshing_xml = 1;

	if(IS_ON_XMB)
		while(View_Find("explore_plugin") == 0) sys_ppu_thread_sleep(1); // wait for explore_plugin

	if(scan_mygames_xml(conn_s_p)) mount_autoboot();

	refreshing_xml = 0;
	sys_ppu_thread_exit(0);
}

static void refresh_xml(char *msg)
{
	if(refreshing_xml) return;
	refreshing_xml = 1;

#ifdef USE_NTFS
	root_check = true;
#endif

	setPluginActive();

	webman_config->profile = profile; save_settings();

	sprintf(msg, "%s XML%s: %s", STR_REFRESH, SUFIX2(profile), STR_SCAN2);
	show_msg(msg);

	// refresh XML
	sys_ppu_thread_t t_id;
	sys_ppu_thread_create(&t_id, handleclient_www, (u64)REFRESH_CONTENT, THREAD_PRIO_HIGH, THREAD_STACK_SIZE_WEB_CLIENT, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_CMD);

	// refresh SND0 settings for new installed games only with combo SELECT+R3+L1+R1 (reload_xmb)
	CellPadData pad_data = pad_read();
	if(pad_data.len > 0 && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L1 | CELL_PAD_CTRL_L1)))
	{
		mute_snd0(true);
	}

	// wait until complete
	while(refreshing_xml && working) sys_ppu_thread_sleep(1);

	sprintf(msg, "%s XML%s: OK", STR_REFRESH, SUFIX2(profile));
	show_msg(msg);

	setPluginInactive();
}
