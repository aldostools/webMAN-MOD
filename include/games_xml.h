#define AVG_ITEM_SIZE			420

#define QUERY_XMB(key, src) 	"<Query class=\"type:x-xmb/folder-pixmap\" key=\"" key "\" attr=\"" key "\" src=\"" src "\"/>"
#define ADD_XMB_ITEM(key)		"<Item class=\"type:x-xmb/module-action\" key=\"" key "\" attr=\"" key "\"/>"

#define XML_HEADER				"<?xml version=\"1.0\" encoding=\"UTF-8\"?><XMBML version=\"1.0\">"
#define XML_PAIR(key, value) 	"<Pair key=\"" key "\"><String>" value "</String></Pair>"

#define WEB_LINK_PAIR			XML_PAIR("module_name", "webbrowser_plugin")
#define STR_NOITEM_PAIR			XML_PAIR("str_noitem", "msg_error_no_content") "</Table>"

#define LAUNCHPAD_FILE_XML		"/dev_hdd0/tmp/wm_launchpad.xml"
#define LAUNCHPAD_MAX_ITEMS		500
#define LAUNCHPAD_COVER_SVR		"http://xmbmods.co/wmlp/covers"
//#define LAUNCHPAD_COVER_SVR	"http://ps3extra.free.fr/covers"

static void refresh_xml(char *msg)
{
	webman_config->profile=profile; save_settings();

	sprintf(msg, "%s XML%s: %s", STR_REFRESH, SUFIX2(profile), STR_SCAN2);
	show_msg((char*) msg);
	init_running = 1;
	sys_ppu_thread_t id3;
	sys_ppu_thread_create(&id3, handleclient, (u64)REFRESH_CONTENT, THREAD_PRIO, THREAD_STACK_SIZE_64KB, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_CMD);
	while(init_running && working) sys_timer_usleep(300000);
	sprintf(msg, "%s XML%s: OK", STR_REFRESH, SUFIX2(profile));
	show_msg((char*) msg);
}

#ifdef LAUNCHPAD
static u32 mtrl_items = 0;

static void add_launchpad_header(void)
{
	int fd; mtrl_items = 0;

	if(cellFsOpen(LAUNCHPAD_FILE_XML, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fd, NULL, 0) == CELL_OK)
	{
		char tempstr[200];
		sprintf(tempstr, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
						 "<nsx anno=\"\" lt-id=\"131\" min-sys-ver=\"1\" rev=\"1093\" ver=\"1.0\">\n"
						 "<spc anno=\"csxad=1&amp;adspace=9,10,11,12,13\" id=\"33537\" multi=\"o\" rep=\"t\">\n\n");

		uint64_t size = strlen(tempstr);
		cellFsWrite(fd, tempstr, size, &size);

		cellFsClose(fd);
	}
}

static void add_launchpad_entry(char *tempstr, char *templn, const char *url, char *tempID)
{
	int fd;

	if(cellFsOpen(LAUNCHPAD_FILE_XML, CELL_FS_O_RDWR | CELL_FS_O_CREAT | CELL_FS_O_APPEND, &fd, NULL, 0) == CELL_OK)
	{
		// add entry
		if(tempID[0]==NULL) sprintf(tempID, "NOID");

		// fix &
		if(strstr(templn, "&"))
		{
			u32 j = 0, tlen = strlen(templn);
			for(u32 i = 0; i <= tlen; i++, j++)
			{
				tempstr[j] = templn[i];

				if(templn[i] == '&')
				{
					sprintf(&tempstr[j], "&amp;"); j += 4;
				}
			}
			strncpy(templn, tempstr, j);
		}

		sprintf(tempstr, "<mtrl id=\"%lu\" until=\"2100-12-31T23:59:00.000Z\">\n"
						 "<desc>%s</desc>\n"
						 "<url type=\"2\">%s/%s%s</url>\n"
						 "<target type=\"u\">%s</target>\n", (1080000000UL + mtrl_items), templn, LAUNCHPAD_COVER_SVR, tempID, strstr(tempID, ".png") ? "" : ".JPG", url);

		strcat(tempstr, "<cntry agelmt=\"0\">all</cntry>\n"
						"<lang>all</lang></mtrl>\n\n");

		uint64_t size = strlen(tempstr);
		cellFsWrite(fd, tempstr, size, &size);

		cellFsClose(fd);

		mtrl_items++; tempstr[0] = NULL;
	}
}

static void add_launchpad_extras(char *tempstr, char *url)
{
	// --- launchpad extras
	sprintf(url, "http://%s/setup.ps3", local_ip);
	add_launchpad_entry(tempstr, (char*)"WebMAN Setup", url, (char*)"setup.png");

	sprintf(url, "http://%s/mount.ps3/unmount", local_ip);
	add_launchpad_entry(tempstr, (char*)"Unmount", url, (char*)"eject.png");

	sprintf(url, "http://%s/mount_ps3/303/***CLEAR RECENTLY PLAYED***", local_ip);
	add_launchpad_entry(tempstr, (char*)"Clear Recently Played", url, (char*)"clear.png");

	sprintf(url, "http://%s/refresh.ps3", local_ip);
	add_launchpad_entry(tempstr, (char*)"Refresh My WebMAN Games", url, (char*)"refresh.png");

	sprintf(url, "http://%s/restart.ps3", local_ip);
	add_launchpad_entry(tempstr, (char*)"Restart PS3", url, (char*)"restart.png");

	sprintf(url, "http://%s/delete.ps3%s", local_ip, "/dev_hdd0/tmp/explore/nsx/");
	add_launchpad_entry(tempstr, (char*)"Clear LaunchPad Cache", url, (char*)"cache.png");
}

static void add_launchpad_footer(char *tempstr)
{
	int fd;

	if(cellFsOpen(LAUNCHPAD_FILE_XML, CELL_FS_O_RDWR | CELL_FS_O_CREAT | CELL_FS_O_APPEND, &fd, NULL, 0) == CELL_OK)
	{
		// --- add scroller placeholder
		sprintf(tempstr, "<mtrl id=\"1081000000\" lastm=\"9999-12-31T23:59:00.000Z\" until=\"2100-12-31T23:59:00.000Z\">\n"
						 "<desc></desc>\n"
						 "<url type=\"2\"></url>\n"
						 "<target type=\"u\"></target>\n"
						 "<cntry agelmt=\"0\">all</cntry>\n"
						 "<lang>all</lang></mtrl>\n\n"
						 "</spc></nsx>");

		uint64_t size = strlen(tempstr);
		cellFsWrite(fd, tempstr, size, &size);

		cellFsClose(fd);
	}
}
#endif //#ifdef LAUNCHPAD

static bool add_xmb_entry(u8 f0, u8 f1, char *param, char *tempstr, char *templn, char *skey, u32 key, char *myxml_ps3, char *myxml_ps2, char *myxml_psx, char *myxml_psp, char *myxml_dvd, char *entry_name, u16 *item_count, u32 *xml_len)
{
	if(strlen(templn)<6) strcat(templn, "      ");

	u8 c = 0;
	if(templn[0]=='[' && templn[4]==']') {c = (templn[5]!=' ') ? 5 : 6;} // ignore tag prefixes. e.g. [PS3] [PS2] [PSX] [PSP] [DVD] [BDV] [ISO] etc.
	sprintf(skey, "!%c%c%c%c%c%c%04i", templn[c], templn[c+1], templn[c+2], templn[c+3], templn[c+4], templn[c+5], key);

	char *p = strstr(templn, "CD");
	if(p) {if(ISDIGIT(p[2])) skey[6]=p[2]; if(ISDIGIT(p[3])) skey[6]=p[3];} // sort by CD#
	else
	{
		u16 tlen = strlen(templn), i = 5; if(tlen > 64) tlen = 64;
		while(i < tlen) {if(ISDIGIT(templn[i])) {skey[6]=templn[i]; break;} else i++;} // sort by game number (if possible)
	}

	u16 xlen = strlen(tempstr);

	#define ITEMS_BUFFER(a)  (64 * (item_count[a] + 8))

	if( !(webman_config->nogrp) )
	{
		if((IS_PS2_FOLDER) && xml_len[2] < (BUFFER_SIZE_PS2 - ITEMS_BUFFER(2)))
		{strcat(myxml_ps2+xml_len[2], tempstr); skey[0]=PS2; ++item_count[2]; xml_len[2]+=xlen;}
#ifdef COBRA_ONLY
		else
		if((IS_PSP_FOLDER) && xml_len[4] < (BUFFER_SIZE_PSP - ITEMS_BUFFER(4)))
		{strcat(myxml_psp+xml_len[4], tempstr); skey[0]=PSP; ++item_count[4]; xml_len[4]+=xlen;}
		else
		if(((IS_PSX_FOLDER) || ((f0==NTFS) && !extcmp(entry_name, ".ntfs[PSXISO]", 13))) && xml_len[1] < (BUFFER_SIZE_PSX - ITEMS_BUFFER(1)))
		{strcat(myxml_psx+xml_len[1], tempstr); skey[0]=PS1; ++item_count[1]; xml_len[1]+=xlen;}
		else
		if(((IS_BLU_FOLDER) || (IS_DVD_FOLDER) || ((f0==NTFS) && (!extcmp(entry_name, ".ntfs[DVDISO]", 13) || !extcmp(entry_name, ".ntfs[BDISO]", 12) || !extcmp(entry_name, ".ntfs[BDFILE]", 13)))) && xml_len[0] < (BUFFER_SIZE_DVD - ITEMS_BUFFER(0)))
		{strcat(myxml_dvd+xml_len[0], tempstr); skey[0]=BLU; ++item_count[0]; xml_len[0]+=xlen;}
#endif
		else
		if((IS_PS3_TYPE) && xml_len[3] < (BUFFER_SIZE  - ITEMS_BUFFER(3) - _4KB_))
		{strcat(myxml_ps3+xml_len[3], tempstr); skey[0]=PS3_; ++item_count[3]; xml_len[3]+=xlen;}
		else
			return (false);
	}
	else
	{
		if(xml_len[3] < (BUFFER_SIZE  - ITEMS_BUFFER(3) - _4KB_))
			{strcat(myxml_ps3+xml_len[3], tempstr); ++item_count[3]; xml_len[3] += xlen;}
		else
			return (false);
	}

	return (true);
}

static void make_fb_xml(char *myxml, char *templn)
{
	sprintf(templn, WM_ICONS_PATH "icon_wm_root.png");

	if(file_exists(templn))
		sprintf(templn, XML_PAIR("icon", WM_ICONS_PATH "icon_wm_root.png"));
	else
		sprintf(templn, XML_PAIR("icon_rsc", "item_tex_ps3util"));

	sprintf(myxml,  "%s"
					"<View id=\"seg_fb\">"
					"<Attributes><Table key=\"mgames\">%s"
					XML_PAIR("icon_notation","WNT_XmbItemSavePS3")
					XML_PAIR("title","%s%s")
					XML_PAIR("info","%s")
					"</Table>"
					"</Attributes>"
					"<Items>"
					QUERY_XMB("mgames", "xmb://localhost%s#seg_mygames")
					"%s</XMBML>", XML_HEADER, templn, STR_MYGAMES, SUFIX2(profile), STR_LOADGAMES, MY_GAMES_XML, "</Items></View>");

	savefile((char*)FB_XML, (char*)myxml, strlen(myxml));
}

static u32 get_buffer_size(int footprint)
{
	if(footprint==1) //MIN
	{
#ifndef LITE_EDITION
		return ( 320*KB);
#else
		return ( _256KB_);
#endif
	}
	else
	if(footprint == 2 || footprint >= 4) //MAX
	{
		return ( 1280*KB);
	}
	else
	if(footprint==3) //MIN+
	{
		return ( 512*KB);
	}
	else	//STANDARD
	{
		return ( 896*KB);
	}
}

static void set_buffer_sizes(int footprint)
{
	BUFFER_SIZE_ALL = get_buffer_size(footprint);
	BUFFER_SIZE_FTP	= ( _128KB_);

	BUFFER_SIZE_PSP	= (  _32KB_);
	BUFFER_SIZE_PS2	= (  _64KB_);
	BUFFER_SIZE_DVD	= (  _64KB_);

	if(footprint==1) //MIN
	{
		//BUFFER_SIZE	= ( _128KB_);
		BUFFER_SIZE_PSX	= (  _32KB_);
	}
	else
	if(footprint==2) //MAX
	{
		BUFFER_SIZE_FTP	= ( _256KB_);
		//BUFFER_SIZE	= ( 512*KB);
		BUFFER_SIZE_PSX	= ( _256KB_);
		BUFFER_SIZE_PSP	= (  _64KB_);
		BUFFER_SIZE_PS2	= ( _128KB_);
		BUFFER_SIZE_DVD	= ( _192KB_);

		if((webman_config->cmask & PS1)) BUFFER_SIZE_PSX	= (_64KB_);
		if((webman_config->cmask & PS2)) BUFFER_SIZE_PS2	= (_64KB_);
		if((webman_config->cmask & (BLU | DVD)) == (BLU | DVD)) BUFFER_SIZE_DVD = (_64KB_);
	}
	else
	if(footprint==3) //MIN+
	{
		//BUFFER_SIZE	= ( 320*KB);
		BUFFER_SIZE_PSX	= (  _32KB_);
	}
	else
	if(footprint==4) //MAX PS3+
	{
		//BUFFER_SIZE	= ( 1088*KB);
		BUFFER_SIZE_PSX	= (  _32KB_);
	}
	else
	if(footprint==5) //MAX PSX+
	{
		//BUFFER_SIZE	= (  368*KB);
		BUFFER_SIZE_PSX	= (  720*KB);
		BUFFER_SIZE_PSP	= (  _64KB_);
	}
	else
	if(footprint==6) //MAX BLU+
	{
		//BUFFER_SIZE	= (  368*KB);
		BUFFER_SIZE_PSX	= (  _64KB_);
		BUFFER_SIZE_PSP	= (  _64KB_);
		BUFFER_SIZE_DVD	= (  720*KB);
	}
	else	//STANDARD
	{
		BUFFER_SIZE_ALL = ( 896*KB);
		//BUFFER_SIZE	= ( 448*KB);
		BUFFER_SIZE_PSX	= ( 160*KB);
		BUFFER_SIZE_DVD	= ( _192KB_);

		if((webman_config->cmask & PS1)) BUFFER_SIZE_PSX	= (_32KB_);
		if((webman_config->cmask & (BLU | DVD)) == (BLU | DVD)) BUFFER_SIZE_DVD = (_64KB_);
	}

	BUFFER_SIZE = BUFFER_SIZE_ALL - (BUFFER_SIZE_PSX + BUFFER_SIZE_PSP + BUFFER_SIZE_PS2 + BUFFER_SIZE_DVD);
}

static bool update_mygames_xml(u64 conn_s_p)
{
	char xml[48]; sprintf(xml, MY_GAMES_XML);

	if(conn_s_p==START_DAEMON)
	{
		if((webman_config->refr==1) || from_reboot)
		{
			cellFsUnlink((char*)WMNOSCAN);

			if(file_exists(xml))
			{
				if(file_exists(FB_XML)) return true; // skip refresh xml & mount autoboot
			}
		}

		// start a new thread for refresh xml content at start up
		if(!webman_config->refr || file_exists(xml)==false)
		{
			sys_ppu_thread_t id3;
			sys_ppu_thread_create(&id3, handleclient, (u64)REFRESH_CONTENT, THREAD_PRIO, THREAD_STACK_SIZE_64KB, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_CMD);
		}

		return true; // mount autoboot & refresh xml
	}

	set_buffer_sizes(webman_config->foot);

	_meminfo meminfo;
	{system_call_1(SC_GET_FREE_MEM, (uint64_t)(u32) &meminfo);}
	if((meminfo.avail)<( (BUFFER_SIZE_ALL) + MIN_MEM)) set_buffer_sizes(3); //MIN+
	if((meminfo.avail)<( (BUFFER_SIZE_ALL) + MIN_MEM)) set_buffer_sizes(1); //MIN
	if((meminfo.avail)<( (BUFFER_SIZE_ALL) + MIN_MEM))
	{
		return false;  //leave if less than min memory
	}

	sys_addr_t sysmem = 0;

#ifdef USE_VM
	if(sys_vm_memory_map(_32MB_, _1MB_, SYS_MEMORY_CONTAINER_ID_INVALID, SYS_MEMORY_PAGE_SIZE_64K, SYS_VM_POLICY_AUTO_RECOMMENDED, &sysmem)!=CELL_OK)
	{
		return false;  //leave if cannot allocate memory
	}
#else
	if(sys_memory_allocate((BUFFER_SIZE_ALL), SYS_MEMORY_PAGE_SIZE_64K, &sysmem)!=0)
	{
		return false;  //leave if cannot allocate memory
	}
#endif

	BUFFER_SIZE		= BUFFER_SIZE;
	BUFFER_SIZE_PSX	= BUFFER_SIZE_PSX;
	BUFFER_SIZE_PSP	= BUFFER_SIZE_PSP;
	BUFFER_SIZE_PS2	= BUFFER_SIZE_PS2;
	BUFFER_SIZE_DVD	= BUFFER_SIZE_DVD;
	BUFFER_SIZE_ALL	= BUFFER_SIZE_ALL;

	sys_addr_t sysmem1 = sysmem  + (BUFFER_SIZE);
	sys_addr_t sysmem2 = sysmem1 + (BUFFER_SIZE_PSX) + (BUFFER_SIZE_PSP);
	sys_addr_t sysmem3 = sysmem2 + (BUFFER_SIZE_PS2);

	char *myxml_ps3   = (char*)sysmem;
	char *myxml_psx   = NULL;
	char *myxml_psp   = NULL;
	char *myxml_ps2   = NULL;
	char *myxml_dvd   = NULL;
	char *myxml       = NULL;
	char *myxml_items = NULL;

	myxml_psx = (char*)sysmem1;
	myxml_psp = (char*)sysmem1+(BUFFER_SIZE_PSX);
	myxml_ps2 = (char*)sysmem2;

	myxml_dvd	= (char*)sysmem3;
	myxml		= (char*)sysmem+(BUFFER_SIZE)-4300;
	myxml_items = (char*)sysmem3;

	cellFsMkdir((char*)"/dev_hdd0/xmlhost", MODE);
	cellFsMkdir((char*)"/dev_hdd0/xmlhost/game_plugin", MODE);

	u32 key=0, max_xmb_items = ((u32)(BUFFER_SIZE_ALL / AVG_ITEM_SIZE));

	char templn[1024];

	make_fb_xml(myxml, templn);

	// --- build group headers ---
	char *tempstr, *folder_name, *url; tempstr=myxml; memset(tempstr, 0, _4KB_); folder_name=myxml+(3*KB); url=myxml+_2KB_;

	u16 item_count[5], xlen; u32 xml_len[5];
	for(u8 i = 0; i < 5; i++) item_count[i] = xml_len[i] = 0;

	cellFsUnlink(xml);

	key=0;

	if(!(webman_config->nogrp))
	{
		if(!(webman_config->cmask & PS3)) strcpy(myxml_ps3, "<View id=\"seg_wm_ps3_items\"><Attributes>");
		if(!(webman_config->cmask & PS2))
		{
			strcpy(myxml_ps2, "<View id=\"seg_wm_ps2_items\"><Attributes>");
			if(webman_config->ps2l && file_exists("/dev_hdd0/game/PS2U10000"))
			{
				sprintf(templn, "<Table key=\"ps2_classic_launcher\">"
								XML_PAIR("icon","/dev_hdd0/game/PS2U10000/ICON0.PNG")
								XML_PAIR("title","PS2 Classic Launcher")
								XML_PAIR("info","%s") "%s",
								STR_LAUNCHPS2, "</Table>"); strcat(myxml_ps2, templn);
			}
		}
#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1)) strcpy(myxml_psx, "<View id=\"seg_wm_psx_items\"><Attributes>");
		if(!(webman_config->cmask & PSP))
		{
			strcpy(myxml_psp, "<View id=\"seg_wm_psp_items\"><Attributes>");
			if(webman_config->pspl && file_exists("/dev_hdd0/game/PSPC66820"))
			{
				sprintf(templn, "<Table key=\"cobra_psp_launcher\">"
								XML_PAIR("icon","/dev_hdd0/game/PSPC66820/ICON0.PNG")
								XML_PAIR("title","PSP Launcher")
								XML_PAIR("info","%s") "%s",
								STR_LAUNCHPSP, "</Table>"); strcat(myxml_psp, templn);
			}
		}
		if(!(webman_config->cmask & DVD) || !(webman_config->cmask & BLU))
		{
			strcpy(myxml_dvd, "<View id=\"seg_wm_dvd_items\"><Attributes>");
			if(webman_config->rxvid)
			{
				sprintf(templn, "<Table key=\"rx_video\">"
								XML_PAIR("icon","%s")
								XML_PAIR("title","%s")
								XML_PAIR("child","segment") "%s",
								wm_icons[4], STR_VIDLG, STR_NOITEM_PAIR); strcat(myxml_dvd, templn);
			}
		}
#endif
	}

	CellRtcTick pTick;
	cellRtcGetCurrentTick(&pTick);

	int fd;
	char skey[max_xmb_items][12];

	char param[MAX_PATH_LEN];
	char icon[MAX_PATH_LEN], enc_dir_name[1024], subpath[MAX_PATH_LEN];

	u8 is_net=0;
	int abort_connection=0;

	xml_len[0] = strlen(myxml_dvd);
	xml_len[1] = strlen(myxml_psx);
	xml_len[2] = strlen(myxml_ps2);
	xml_len[3] = strlen(myxml_ps3);
	xml_len[4] = strlen(myxml_psp);

	// --- scan xml content ---

	led(YELLOW, BLINK_FAST);

#ifdef LAUNCHPAD
	bool launchpad_xml = isDir("/dev_flash/rebug") && !(webman_config->launchpad_xml);

	if(launchpad_xml)
	{
		del("/dev_hdd0/tmp/explore/nsx/", true); // Clear LaunchPad Cache
		add_launchpad_header();
	}
#endif

	int ns=-2; u8 uprofile=profile;

	for(u8 f0=0; f0<16; f0++)  // drives: 0="/dev_hdd0", 1="/dev_usb000", 2="/dev_usb001", 3="/dev_usb002", 4="/dev_usb003", 5="/dev_usb006", 6="/dev_usb007", 7="/net0", 8="/net1", 9="/net2", 10="/net3", 11="/net4", 12="/ext", 13="/dev_sd", 14="/dev_ms", 15="/dev_cf"
	{
		if(!webman_config->usb0 && (f0==1)) continue;
		if(!webman_config->usb1 && (f0==2)) continue;
		if(!webman_config->usb2 && (f0==3)) continue;
		if(!webman_config->usb3 && (f0==4)) continue;
		if(!webman_config->usb6 && (f0==5)) continue;
		if(!webman_config->usb7 && (f0==6)) continue;

		// f0 -> 7 to 11 (net), 12 ntfs/ext

		if(!webman_config->dev_sd && (f0==13)) continue;
		if(!webman_config->dev_ms && (f0==14)) continue;
		if(!webman_config->dev_cf && (f0==15)) continue;

		if( f0==NTFS && (!webman_config->usb0 && !webman_config->usb1 && !webman_config->usb2 &&
						 !webman_config->usb3 && !webman_config->usb6 && !webman_config->usb7)) continue;

		if(conn_s_p==START_DAEMON)
		{
			if(webman_config->bootd && (f0==1))
			{
				waitfor((char*)"/dev_usb", webman_config->bootd); // wait for any usb
			}

			if(webman_config->boots && (f0>=1 && f0<=6)) // usb000->007
			{
				if( (webman_config->usb0 && (f0==1)) ||
					(webman_config->usb1 && (f0==2)) ||
					(webman_config->usb2 && (f0==3)) ||
					(webman_config->usb3 && (f0==4)) ||
					(webman_config->usb6 && (f0==5)) ||
					(webman_config->usb7 && (f0==6)) )
				{
					waitfor((char*)drives[f0], webman_config->boots);
				}
			}
		}

		if(( f0<7 || f0>NTFS) && file_exists(drives[f0])==false) continue;

		ns=-2; uprofile=profile;
		for(u8 f1=0; f1<11; f1++) // paths: 0="GAMES", 1="GAMEZ", 2="PS3ISO", 3="BDISO", 4="DVDISO", 5="PS2ISO", 6="PSXISO", 7="PSXGAMES", 8="PSPISO", 9="ISO", 10="video"
		{
#ifndef COBRA_ONLY
			if(IS_ISO_FOLDER && !(IS_PS2_FOLDER)) continue; // 0="GAMES", 1="GAMEZ", 5="PS2ISO", 10="video"
#endif
			if(key>=max_xmb_items) break;

			//if(IS_PS2_FOLDER && f0>0)  continue; // PS2ISO is supported only from /dev_hdd0
			if(f1>=10) {if(f0<7 || f0>NTFS) strcpy(paths[10], f0==0 ? "video" : "GAMES_DUP"); else continue;}
			if(f0==NTFS) {if(f1>6 || !cobra_mode) break; else if(IS_JB_FOLDER || IS_PS2_FOLDER) continue;}

			is_net=(f0>=7 && f0<NTFS);

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
			if(is_net)
			{
				if(f1>8 || !cobra_mode) break;
				if(f0==7  && !webman_config->netd0) break; //net0
				if(f0==8  && !webman_config->netd1) break; //net1
				if(f0==9  && !webman_config->netd2) break; //net2
 #ifdef NET3NET4
				if(f0==10 && !webman_config->netd3) break; //net3
				if(f0==11 && !webman_config->netd4) break; //net4
 #else
				if(f0==10 || f0==11) break;
 #endif
			}
 #endif
#endif
			if( (webman_config->cmask & PS3) && IS_PS3_TYPE   ) continue; // 0="GAMES", 1="GAMEZ", 2="PS3ISO", 10="video"
			if( (webman_config->cmask & BLU) && IS_BLU_FOLDER ) continue;
			if( (webman_config->cmask & DVD) && IS_DVD_FOLDER ) continue;
			if( (webman_config->cmask & PS2) && IS_PS2_FOLDER ) continue;
			if( (webman_config->cmask & PS1) && IS_PSX_FOLDER ) continue;
			if( (webman_config->cmask & PSP) && IS_PSP_FOLDER ) continue;

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
			if(ns==-2 && is_net) ns=connect_to_remote_server(f0-7);
 #endif
#endif
			if(is_net && (ns<0)) break;

//
			bool ls; u8 li, subfolder; li=subfolder=0; ls=false; // single letter folder

		subfolder_letter_xml:
			subfolder = 0; uprofile = profile;
		read_folder_xml:
//
#ifndef LITE_EDITION
			if(is_net)
			{
				char ll[4]; if(li) sprintf(ll, "/%c", '@'+li); else ll[0] = NULL;
				sprintf(param, "/%s%s%s",    paths[f1], SUFIX(uprofile), ll);

				if(li==99) sprintf(param, "/%s %s", paths[f1], AUTOPLAY_TAG);
			}
			else
#endif
			{
				if(f0==NTFS) //ntfs
					sprintf(param, "%s", WMTMP);
				else
					sprintf(param, "%s/%s%s", drives[f0], paths[f1], SUFIX(uprofile));

				if(li==99) sprintf(param, "%s/%s %s", drives[f0], paths[f1], AUTOPLAY_TAG);
			}

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
			if(is_net && open_remote_dir(ns, param, &abort_connection) < 0) goto continue_reading_folder_xml; //continue;
 #endif
#endif
			//led(YELLOW, ON);
			{
				CellFsDirent entry;
				u64 read_e;
				u8 is_iso=0;
				int fd2=0;
				char tempID[12];
				cellRtcGetCurrentTick(&pTick);

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
				sys_addr_t data2 = 0;
				int v3_entries, v3_entry; v3_entries=v3_entry=0;
				netiso_read_dir_result_data *data=NULL; char neth[8];
				if(is_net)
				{
					v3_entries = read_remote_dir(ns, &data2, &abort_connection);
					if(data2==NULL) goto continue_reading_folder_xml; //continue;
					data=(netiso_read_dir_result_data*)data2; sprintf(neth, "/net%i", (f0-7));
				}
 #endif
#endif
				if(!is_net && file_exists( param) == false) goto continue_reading_folder_xml; //continue;
				if(!is_net && cellFsOpendir( param, &fd) != CELL_FS_SUCCEEDED) goto continue_reading_folder_xml; //continue;

				while((!is_net && cellFsReaddir(fd, &entry, &read_e) == 0 && read_e > 0)
#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
					|| (is_net && (v3_entry < v3_entries))
 #endif
#endif
					)
				{
					if(key>=max_xmb_items) break;
#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
					if(is_net)
					{
						if((ls==false) && (li==0) && (f1>1) && (data[v3_entry].is_directory) && (strlen(data[v3_entry].name)==1)) ls=true;

						if(add_net_game(ns, data, v3_entry, neth, param, templn, tempstr, enc_dir_name, icon, tempID, f1, 0)==FAILED) {v3_entry++; continue;}

						sprintf(tempstr, "<Table key=\"%04i\">"
										 XML_PAIR("icon","%s")
										 XML_PAIR("title","%s") "%s"
										 XML_PAIR("module_action","http://%s/mount_ps3%s%s/%s?random=%x")
										 XML_PAIR("info","%s%s%s") "</Table>",
								key, icon,
								templn, WEB_LINK_PAIR, local_ip, neth, param, enc_dir_name, (u16)pTick.tick, neth, param, "");

						if(add_xmb_entry(f0, f1, param, tempstr, templn, skey[key], key, myxml_ps3, myxml_ps2, myxml_psx, myxml_psp, myxml_dvd, data[v3_entry].name, item_count, xml_len)) key++;

 #ifdef LAUNCHPAD
						if(launchpad_xml && (mtrl_items < LAUNCHPAD_MAX_ITEMS))
						{
							sprintf(url, "http://%s/mount_ps3%s%s/%s", local_ip, neth, param, enc_dir_name);
							add_launchpad_entry(tempstr, templn, url, tempID);
						}
 #endif

						v3_entry++;
					}
					else
 #endif
#endif
					{
						if(entry.d_name[0]=='.') continue;

//////////////////////////////
						subfolder = 0;
						sprintf(subpath, "%s/%s", param, entry.d_name);
						if(IS_ISO_FOLDER && isDir(subpath) && cellFsOpendir(subpath, &fd2) == CELL_FS_SUCCEEDED)
						{
							strcpy(subpath, entry.d_name); subfolder = 1;
next_xml_entry:
							cellFsReaddir(fd2, &entry, &read_e);
							if(read_e<1) {cellFsClosedir(fd2); fd2 = 0; continue;}
							if(entry.d_name[0]=='.') goto next_xml_entry;
							sprintf(templn, "%s/%s", subpath, entry.d_name); strcpy(entry.d_name, templn);
						}
						int flen = strlen(entry.d_name);
//////////////////////////////

						if(key>=max_xmb_items) break;

#ifdef COBRA_ONLY
						is_iso = (f0==NTFS && flen>13 && strstr(entry.d_name + flen - 13, ".ntfs[")!=NULL) ||
								 (IS_ISO_FOLDER && flen > 4 && (
								 (            !strncasecmp(entry.d_name + flen - 4, ".iso", 4)) ||
								 (flen > 6 && !strncasecmp(entry.d_name + flen - 6, ".iso.0", 6)) ||
								 ((IS_PS2_FOLDER) && strcasestr(".bin|.img|.mdf|.enc", entry.d_name + flen - 4)) ||
								 ((IS_PSX_FOLDER || IS_DVD_FOLDER || IS_BLU_FOLDER) && strcasestr(".bin|.img|.mdf", entry.d_name + flen - 4))
								 ));
#else
						is_iso = (IS_PS2_FOLDER && flen > 8 && !strncmp(entry.d_name + flen - 8, ".BIN.ENC", 8));
#endif
						if(IS_JB_FOLDER && !is_iso)
						{
							sprintf(templn, "%s/%s/PS3_GAME/PARAM.SFO", param, entry.d_name);
						}

						if(is_iso || (IS_JB_FOLDER && file_exists(templn)))
						{
							icon[0] = tempID[0] = NULL;

							if(!is_iso)
							{
								get_title_and_id_from_sfo(templn, tempID, entry.d_name, icon, tempstr, 0);
							}
							else
							{
								get_name(templn, entry.d_name, 0);
#ifdef COBRA_ONLY
								if(f0==NTFS)
								{   // ntfs
									if(f1< 2 || f1>6) continue; //2="PS3ISO", 3="BDISO", 4="DVDISO", 5="PS2ISO", 6="PSXISO"

									// skip non-matching extended content
									if((uprofile >0) && (strstr(entry.d_name, ").ntfs[")!=NULL) && !strstr(entry.d_name, SUFIX3(uprofile))) continue;

									// skip extended content of ntfs cached in /wmtmp if current user profile is 0
									if((uprofile==0 && flen>17)) {u8 u; for(u=1;u<5;u++) if(strstr(entry.d_name + flen - 17, SUFIX3(u))) break; if(u!=5) continue;}

									flen-=13; if(flen<0) continue;

									if(IS_PS3_FOLDER && strcmp(entry.d_name+flen, ".ntfs[PS3ISO]")) continue;
									if(IS_BLU_FOLDER &&!strstr(entry.d_name+flen, ".ntfs[BD"     )) continue;
									if(IS_DVD_FOLDER && strcmp(entry.d_name+flen, ".ntfs[DVDISO]")) continue;
									if(IS_PSX_FOLDER && strcmp(entry.d_name+flen, ".ntfs[PSXISO]")) continue;
								}

								if((IS_PS3_FOLDER) && ((f0!=NTFS) || (f0==NTFS && !extcmp(entry.d_name, ".ntfs[PS3ISO]", 13))))
								{
									get_name(templn, entry.d_name, 1); strcat(templn, ".SFO\0");
									if(f0!=NTFS && file_exists(templn)==false)
									{
										get_name(tempstr, entry.d_name, 0);
										sprintf(templn, "%s/%s.SFO", param, tempstr);
									}

									if(get_title_and_id_from_sfo(templn, tempID, entry.d_name, icon, tempstr, f0)==FAILED)
									{
										if( f0!=NTFS && is_iso)
										{
											int fs; char *ps3iso = icon;
											sprintf(ps3iso, "%s/%s", param, entry.d_name);
											if(cellFsOpen(ps3iso, CELL_FS_O_RDONLY, &fs, NULL, 0) == CELL_FS_SUCCEEDED)
											{
												uint64_t msiz = 0;
												if(cellFsLseek(fs, 0x810, CELL_FS_SEEK_SET, &msiz) == CELL_FS_SUCCEEDED)
												{
													if(cellFsRead(fs, (void *)&tempID, 11, &msiz) == CELL_FS_SUCCEEDED)
													{
														strncpy(&tempID[4], &tempID[5], 5); tempID[9] = NULL;
													}
												}
												cellFsClose(fs);
											}
											ps3iso[0] = NULL;
										}
									}
								}
#endif
							}

							get_folder_icon(icon, f1, is_iso, param, entry.d_name, tempID);

							get_default_icon(icon, param, entry.d_name, 0, tempID, ns, abort_connection);

							if(webman_config->tid && tempID[0]>'@' && strlen(templn) < 50 && strstr(templn, " [")==NULL) {strcat(templn, " ["); strcat(templn, tempID); strcat(templn, "]");}

							urlenc(enc_dir_name, entry.d_name, 0);

							// subfolder name
							if(f0==NTFS && entry.d_name[0]=='[')
							{
								strcpy(folder_name, entry.d_name); folder_name[0]='/'; char *p=strchr(folder_name, ']'); if(p) p[0] = NULL;
							}
							else
							{
								char *p=strchr(entry.d_name, '/');
								folder_name[0] = NULL; if(p) {p[0] = NULL; sprintf(folder_name, "/%s", entry.d_name); p[0]='/';}
							}

							sprintf(tempstr, "<Table key=\"%04i\">"
											 XML_PAIR("icon","%s")
											 XML_PAIR("title","%s") "%s"
											 XML_PAIR("module_action","http://%s/mount_ps3%s%s/%s?random=%x")
											 XML_PAIR("info","%s%s%s") "</Table>",
								key, icon,
								templn, WEB_LINK_PAIR, local_ip, "", param, enc_dir_name, (u16)pTick.tick, (f0==NTFS?(char*)"/ntfs/":param), (f0==NTFS?paths[f1]:""), folder_name);

							if(add_xmb_entry(f0, f1, param, tempstr, templn, skey[key], key, myxml_ps3, myxml_ps2, myxml_psx, myxml_psp, myxml_dvd, entry.d_name, item_count, xml_len)) key++;

 #ifdef LAUNCHPAD
							if(launchpad_xml && (mtrl_items < LAUNCHPAD_MAX_ITEMS))
							{
								sprintf(url, "http://%s/mount_ps3%s/%s", local_ip, param, enc_dir_name);
								add_launchpad_entry(tempstr, templn, url, tempID);
							}
 #endif
						}
//////////////////////////////
						if(subfolder) goto next_xml_entry;
//////////////////////////////
					}
				}
				if(!is_net) cellFsClosedir(fd);

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
				if(data2) sys_memory_free(data2);
 #endif
#endif
			}

//
continue_reading_folder_xml:

			if((uprofile>0) && (f1<9)) {subfolder=uprofile=0; goto read_folder_xml;}
			if(is_net && ls && li<27) {li++; goto subfolder_letter_xml;} else if(li<99 && f1<7) {li=99; goto subfolder_letter_xml;}
//
		}
		if(is_net && ns>=0) {shutdown(ns, SHUT_RDWR); socketclose(ns); ns=-2;}
	}

	if( !(webman_config->nogrp))
	{
		if(!(webman_config->cmask & PS3)) {strcat(myxml_ps3, "</Attributes><Items>");}
		if(!(webman_config->cmask & PS2)) {strcat(myxml_ps2, "</Attributes><Items>"); if(webman_config->ps2l && file_exists(PS2_CLASSIC_PLACEHOLDER)) strcat(myxml_ps2, QUERY_XMB("ps2_classic_launcher", "xcb://127.0.0.1/query?limit=1&cond=Ae+Game:Game.titleId PS2U10000"));}

#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1)) {strcat(myxml_psx, "</Attributes><Items>");}
		if(!(webman_config->cmask & PSP)) {strcat(myxml_psp, "</Attributes><Items>"); if(webman_config->pspl && file_exists("/dev_hdd0/game/PSPC66820")) strcat(myxml_psp, QUERY_XMB("cobra_psp_launcher", "xcb://127.0.0.1/query?limit=1&cond=Ae+Game:Game.titleId PSPC66820"));}
		if(!(webman_config->cmask & DVD) || !(webman_config->cmask & BLU)) {strcat(myxml_dvd, "</Attributes><Items>"); if(webman_config->rxvid) strcat(myxml_dvd, QUERY_XMB("rx_video", "#seg_wm_bdvd"));}
#endif
	}
	else
		myxml_dvd[0] = NULL;

	// --- sort scanned content

	led(YELLOW, OFF);
	led(GREEN, ON);

	if(key)
	{   // sort xmb items
		char swap[16]; u16 m, n;

		if((webman_config->nogrp))
		{
			for(n=0; n<(key-1); n++)
				for(m=(n+1); m<key; m++)
					if(strcasecmp(skey[n]+1, skey[m]+1)>0)
					{
						strcpy(swap, skey[n]);
						strcpy(skey[n], skey[m]);
						strcpy(skey[m], swap);
					}
		}
		else
			for(n=0; n<(key-1); n++)
				for(m=(n+1); m<key; m++)
					if(strcasecmp(skey[n], skey[m])>0)
					{
						strcpy(swap, skey[n]);
						strcpy(skey[n], skey[m]);
						strcpy(skey[m], swap);
					}
	}

	// --- add eject & setup/xmbm+ menu
#ifdef ENGLISH_ONLY
	bool add_xmbm_plus = file_exists("/dev_hdd0/game/XMBMANPLS/USRDIR/FEATURES/webMAN.xml");
#else
	bool add_xmbm_plus = false;

	while(true)
	{
		sprintf(templn, "/dev_hdd0/game/XMBMANPLS/USRDIR/FEATURES/webMAN%s.xml", lang_code);
		add_xmbm_plus = file_exists(templn);
		if(add_xmbm_plus || lang_code[0]==0) break; lang_code[0] = NULL;
	}
#endif

	if( (webman_config->nogrp))
	{
		if(!add_xmbm_plus) strcat(myxml_items, ADD_XMB_ITEM("eject"));

		if(!webman_config->noset)
		{
			if(add_xmbm_plus)
#ifdef ENGLISH_ONLY
				strcat(myxml_items, QUERY_XMB("setup", "xmb://localhost/dev_hdd0/game/XMBMANPLS/USRDIR/FEATURES/webMAN.xml#seg_webman_links_items"));
#else
			{
				sprintf(tempstr, QUERY_XMB("setup", "xmb://localhost%s#seg_webman_links_items"), templn);
				strcat(myxml_items, tempstr);
			}
#endif
			else
				strcat(myxml_items, ADD_XMB_ITEM("setup"));
		}
	}

	// --- add sorted items to xml
	if(webman_config->nogrp) xml_len[3]=strlen(myxml_items);
	else
	{
		xml_len[0]=strlen(myxml_dvd);
		xml_len[1]=strlen(myxml_psx);
		xml_len[2]=strlen(myxml_ps2);
		xml_len[3]=strlen(myxml_ps3);
		xml_len[4]=strlen(myxml_psp);
	}

	for(u16 a=0; a<key; a++)
	{
		sprintf(templn, ADD_XMB_ITEM("%s"), skey[(a)]+7, skey[(a)]+7); xlen = strlen(templn);
		if( !(webman_config->nogrp))
		{
#ifdef COBRA_ONLY
			if(skey[(a)][0]==PSP && xml_len[4]<(BUFFER_SIZE_PSP-128))
				{strcat(myxml_psp + xml_len[4], templn); xml_len[4] += xlen;}
			else
			if(skey[(a)][0]==PS1 && xml_len[1]<(BUFFER_SIZE_PSX-128))
				{strcat(myxml_psx + xml_len[1], templn); xml_len[1] += xlen;}
			else
			if(skey[(a)][0]==BLU && xml_len[0]<(BUFFER_SIZE_DVD-1200))
				{strcat(myxml_dvd + xml_len[0], templn); xml_len[0] += xlen;}
			else
#endif
			if(skey[(a)][0]==PS2 && xml_len[2]<(BUFFER_SIZE_PS2-128))
				{strcat(myxml_ps2 + xml_len[2], templn); xml_len[2] += xlen;}
			else
			if(skey[(a)][0]==PS3_&& xml_len[3]<(BUFFER_SIZE-5000))
				{strcat(myxml_ps3 + xml_len[3], templn); xml_len[3] += xlen;}

		}
		else
			if(xml_len[3]<(BUFFER_SIZE-1000))
				{strcat(myxml_items + xml_len[3], templn); xml_len[3] += xlen;}
	}

	// --- close xml items

	if( !(webman_config->nogrp))
	{
		if(!(webman_config->cmask & PS3)) strcat(myxml_ps3, "</Items></View>");
		if(!(webman_config->cmask & PS2)) strcat(myxml_ps2, "</Items></View>");
#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1)) strcat(myxml_psx, "</Items></View>");
		if(!(webman_config->cmask & PSP)) strcat(myxml_psp, "</Items></View>");
		if(!(webman_config->cmask & DVD) || !(webman_config->cmask & BLU))
		{
			strcat(myxml_dvd, "</Items></View>");
			if(webman_config->rxvid)
			{
				strcat(myxml_dvd,	"<View id=\"seg_wm_bdvd\">"
									"<Items>"
									QUERY_XMB("rx_video1", "xcb://localhost/query?table=MMS_MEDIA_TYPE_SYSTEM&genre=Video&sort=+StorageMedia:StorageMedia.sortOrder+StorageMedia:StorageMedia.timeInserted&cond=Ae+StorageMedia:StorageMedia.stat.mediaStatus %xCB_MEDIA_INSERTED+Ae+StorageMedia:StorageMedia.mediaFormat %xCB_MEDIA_FORMAT_DATA+AGL+StorageMedia:StorageMedia.type %xCB_MEDIA_TYPE_BDROM %xCB_MEDIA_TYPE_WM")
									QUERY_XMB("rx_video2", "xcb://localhost/query?sort=+Game:Common.titleForSort&cond=AGL+Game:Game.titleId RXMOV0000 RXMOVZZZZ+An+Game:Game.category 2D+An+Game:Game.category BV+An+Game:Game.category HG")
									"</Items>"
									"</View>");
			}
		}
#endif
	}

	// --- build xml headers
	memset(myxml, 0, 4300);
	sprintf(myxml,  "%s"
					"<View id=\"seg_mygames\">"
					"<Attributes>", XML_HEADER);

	if(!add_xmbm_plus)
	{
		sprintf(templn, "<Table key=\"eject\">"
						XML_PAIR("icon","%s")
						XML_PAIR("title","%s")
						XML_PAIR("info","%s") "%s"
						XML_PAIR("module_action","http://%s/mount_ps3/unmount") "%s",
						wm_icons[11], STR_EJECTDISC, STR_UNMOUNTGAME, WEB_LINK_PAIR, local_ip, "</Table>"); strcat(myxml, templn);
	}

	if( !(webman_config->nogrp))
	{
		if( !(webman_config->cmask & PS3)) {sprintf(templn, "<Table key=\"wm_ps3\">"
															XML_PAIR("icon","%s")
															XML_PAIR("title","PLAYSTATION\xC2\xAE\x33")
															XML_PAIR("info","%'i %s") "%s",
															wm_icons[0], item_count[3], STR_PS3FORMAT, STR_NOITEM_PAIR); strcat(myxml, templn);}
		if( !(webman_config->cmask & PS2)) {sprintf(templn, "<Table key=\"wm_ps2\">"
															XML_PAIR("icon","%s")
															XML_PAIR("title","PLAYSTATION\xC2\xAE\x32")
															XML_PAIR("info","%'i %s") "%s",
															wm_icons[2], item_count[2], STR_PS2FORMAT, STR_NOITEM_PAIR); strcat(myxml, templn);}
#ifdef COBRA_ONLY
		if( !(webman_config->cmask & PS1)) {sprintf(templn, "<Table key=\"wm_psx\">"
															XML_PAIR("icon","%s")
															XML_PAIR("title","PLAYSTATION\xC2\xAE")
															XML_PAIR("info","%'i %s") "%s",
															wm_icons[1], item_count[1], STR_PS1FORMAT, STR_NOITEM_PAIR);strcat(myxml, templn);}
		if( !(webman_config->cmask & PSP)) {sprintf(templn, "<Table key=\"wm_psp\">"
															XML_PAIR("icon","%s")
															XML_PAIR("title","PLAYSTATION\xC2\xAEPORTABLE")
															XML_PAIR("info","%'i %s") "%s",
															wm_icons[3], item_count[4], STR_PSPFORMAT, STR_NOITEM_PAIR);strcat(myxml, templn);}
		if( !(webman_config->cmask & DVD) ||
            !(webman_config->cmask & BLU)) {sprintf(templn, "<Table key=\"wm_dvd\">"
															XML_PAIR("icon","%s")
															XML_PAIR("title","%s")
															XML_PAIR("info","%'i %s") "%s",
															wm_icons[4], STR_VIDFORMAT, item_count[0], STR_VIDEO, STR_NOITEM_PAIR); strcat(myxml, templn);}
#endif
	}

	if(!webman_config->noset)
	{
		sprintf(templn, "<Table key=\"setup\">"
						 XML_PAIR("icon","%s")
						 XML_PAIR("title","%s")
						 XML_PAIR("info","%s") "%s",
						 add_xmbm_plus ? "/dev_hdd0/game/XMBMANPLS/USRDIR/IMAGES/multiman.png" : wm_icons[10],
						 STR_WMSETUP, STR_WMSETUP2, WEB_LINK_PAIR); strcat(myxml, templn);

		if(add_xmbm_plus)
			strcat(myxml, XML_PAIR("child","segment"));
		else
			{sprintf(templn, XML_PAIR("module_action","http://%s/setup.ps3"), local_ip); strcat(myxml, templn);}

		strcat(myxml, "</Table>");
	}

	if(!(webman_config->nogrp))
	{
        strcat(myxml, "</Attributes><Items>");
		if( !(webman_config->noset) )
		{
			if(add_xmbm_plus)
#ifdef ENGLISH_ONLY
				strcat(myxml, QUERY_XMB("setup", "xmb://localhost/dev_hdd0/game/XMBMANPLS/USRDIR/FEATURES/webMAN.xml#seg_webman_links_items"));
#else
			{
				sprintf(templn, QUERY_XMB("setup", "xmb://localhost/dev_hdd0/game/XMBMANPLS/USRDIR/FEATURES/webMAN%s.xml#seg_webman_links_items"), lang_code);
				strcat(myxml, templn);
			}
#endif
			else
				strcat(myxml, ADD_XMB_ITEM("setup"));
		}

		if(!add_xmbm_plus) strcat(myxml, ADD_XMB_ITEM("eject"));

		if(!(webman_config->cmask & PS3)) strcat(myxml, QUERY_XMB("wm_ps3", "#seg_wm_ps3_items"));
		if(!(webman_config->cmask & PS2)) strcat(myxml, QUERY_XMB("wm_ps2", "#seg_wm_ps2_items"));
#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1)) strcat(myxml, QUERY_XMB("wm_psx", "#seg_wm_psx_items"));
		if(!(webman_config->cmask & PSP)) strcat(myxml, QUERY_XMB("wm_psp", "#seg_wm_psp_items"));
		if(!(webman_config->cmask & DVD) ||
		   !(webman_config->cmask & BLU)) strcat(myxml, QUERY_XMB("wm_dvd", "#seg_wm_dvd_items"));
#endif

		strcat(myxml, "</Items></View>");
	}

	// --- save xml file
	int fdxml=0;
	cellFsOpen(xml, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fdxml, NULL, 0);
	cellFsWrite(fdxml, (char*)myxml, strlen(myxml), NULL);

	if( (webman_config->nogrp))
	{
		cellFsWrite(fdxml, (char*)myxml_ps3, strlen(myxml_ps3), NULL);
		cellFsWrite(fdxml, (char*)"</Attributes><Items>", 20, NULL);
		cellFsWrite(fdxml, (char*)myxml_items, strlen(myxml_items), NULL);
		sprintf(myxml, "</Items></View></XMBML>\r\n");
	}
	else
	{
		if(!(webman_config->cmask & PS3)) cellFsWrite(fdxml, (char*)myxml_ps3, strlen(myxml_ps3), NULL);
		if(!(webman_config->cmask & PS2)) cellFsWrite(fdxml, (char*)myxml_ps2, strlen(myxml_ps2), NULL);
#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1)) cellFsWrite(fdxml, (char*)myxml_psx, strlen(myxml_psx), NULL);
		if(!(webman_config->cmask & PSP)) cellFsWrite(fdxml, (char*)myxml_psp, strlen(myxml_psp), NULL);
		if(!(webman_config->cmask & DVD) || !(webman_config->cmask & BLU)) cellFsWrite(fdxml, (char*)myxml_dvd, strlen(myxml_dvd), NULL);
#endif
		sprintf(myxml, "</XMBML>\r\n");
	}

	cellFsWrite(fdxml, (char*)myxml, strlen(myxml), NULL);
	cellFsClose(fdxml);
	cellFsChmod(xml, MODE);

	// --- replace & with ^ for droidMAN

	if(cellFsOpen(xml, CELL_FS_O_RDONLY, &fdxml, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		u64 read_e = 0;
		u32 xmlsize=BUFFER_SIZE_ALL;
		cellFsRead(fdxml, (void *)myxml_ps3, xmlsize, &read_e);
		cellFsClose(fdxml);
		for(u32 n=0;n<read_e;n++) if(myxml_ps3[n]=='&') myxml_ps3[n]='^';

		strcpy(xml+37, ".droid\0"); // .xml -> .droid
		savefile(xml, myxml_ps3, read_e);
	}

#ifdef LAUNCHPAD
	// --- launchpad footer
	if(launchpad_xml)
	{
		add_launchpad_extras(tempstr, url);
		add_launchpad_footer(tempstr);
	}
#endif

	// --- release allocated memory

	led(GREEN, ON);

#ifdef USE_VM
	sys_vm_unmap(sysmem);
#else
	sys_memory_free(sysmem);
#endif

	return false;
}

static void update_xml_thread(u64 conn_s_p)
{
	init_running = 1;

	if(View_Find("game_plugin") == 0)
		while(View_Find("explore_plugin") == 0) sys_timer_usleep(100000);

	if(update_mygames_xml(conn_s_p)) mount_autoboot();

	init_running = 0;
	sys_ppu_thread_exit(0);
}
