#define MAX_LAST_GAMES			(5)
#define LAST_GAMES_UPPER_BOUND	(4)

// File name TAGS:
// [auto]    Auto-play
// [online]  Auto-disable syscalls
// [offline] Auto-disable network
// [gd]      Auto-enable external gameDATA
// [raw]     Use internal rawseciso to mount the ISO (ntfs)
// [net]     Use internal netiso to mount the ISO (netiso)
// [PS2]     PS2 extracted folders in /PS2DISC (needs PS2_DISC compilation flag)
// [netemu]  Mount ps2/psx game with netemu

#define TITLEID_LEN		10

#ifdef PKG_LAUNCHER
static char map_title_id[TITLEID_LEN];
#endif

typedef struct
{
	char path[MAX_PATH_LEN];
}
t_path_entries;

typedef struct
{
	u8 last;
	t_path_entries game[MAX_LAST_GAMES];
} __attribute__((packed)) _lastgames;

#define COPY_CMD		9
#define MOUNT_CMD		10

// mount_game actions:
#define MOUNT_SILENT	0	// mount game/folder
#define MOUNT_NORMAL	1	// mount game/folder + store last game + show msg + allow Auto-enable external gameDATA
// MOUNT_EXT_GDATA		2	// mount /dev_usb/GAMEI as /dev_hdd0/game on non-Cobra edition
// EXPLORE_CLOSE_ALL	3	// MOUNT_NORMAL + close all first


// /mount_ps3/<path>[?random=<x>[&emu={ ps1_netemu.self / ps1_emu.self / ps2_netemu.self / ps2_emu.self }][offline={0/1}]
// /mount.ps3/<path>[?random=<x>[&emu={ ps1_netemu.self / ps1_emu.self / ps2_netemu.self / ps2_emu.self }][offline={0/1}]
// /mount.ps3/unmount
// /mount.ps2/<path>[?random=<x>]
// /mount.ps2/unmount
// /copy.ps3/<path>[&to=<destination>]

#ifdef COPY_PS3
u8 usb = 1; // first connected usb drive [used by /copy.ps3 & in the tooltips for /copy.ps3 links in the file manager]. 1 = /dev_usb000
#endif

static void auto_play(char *param)
{
#ifdef OFFLINE_INGAME
	if((strstr(param, OFFLINE_TAG) != NULL)) net_status = 0;
#endif
	if(IS_ON_XMB)
	{
		u8 autoplay = webman_config->autoplay;

		CellPadData pad_data = pad_read();
		bool atag = (strcasestr(param, AUTOPLAY_TAG)!=NULL) || (autoplay);
 #ifdef REMOVE_SYSCALLS
		bool l2 = (pad_data.len > 0 && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2));
 #else
		bool l2 = (pad_data.len > 0 && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2)));
 #endif

		int view = View_Find("explore_plugin");
		if(view) 
			explore_interface = (explore_plugin_interface *)plugin_GetInterface(view, 1);
		else
			return;

		bool mount_ps3 = (strstr(param, "_ps3") != NULL);

		if(!l2 && !extcmp(param, ".BIN.ENC", 8))
		{
			if(mount_ps3 && XMB_GROUPS && webman_config->ps2l && file_exists(PS2_CLASSIC_ISO_PATH))
			{
				if(explore_exec_push(250000, true))	// move to ps2_launcher folder and open it
				if(autoplay && !explore_exec_push(500000, false))	// start ps2_launcher
					autoplay = false;
			}
		}
		else
 #if defined(FAKEISO) || defined(PKG_LAUNCHER)
		if(!l2 && strstr(param, "/PSPISO"))
		{
			if(mount_ps3 && XMB_GROUPS && webman_config->pspl && (isDir(PSP_LAUNCHER_MINIS) || isDir(PSP_LAUNCHER_REMASTERS)))
			{
				if(explore_exec_push(250000, true))	// move to psp_launcher folder and open it
				if(autoplay && !explore_exec_push(500000, false))	// start psp_launcher
					autoplay = false;
			}
		}
		else
 #endif
 #ifdef PKG_LAUNCHER
		if(strstr(param, "/GAMEI/"))
		{
			if(XMB_GROUPS && webman_config->ps3l)
			{
				explore_interface->ExecXMBcommand("focus_index pkg_launcher", 0, 0);
				explore_exec_push(200000, true); // open pkg_launcher folder
			}
		}
		else
 #endif
 #ifdef FAKEISO
		if(!l2 && !extcmp(param, ".ntfs[BDFILE]", 13))
		{
			if(XMB_GROUPS && webman_config->rxvid)
			{
				if(strcasestr(param, ".pkg"))
				{
					explore_interface->ExecXMBcommand("close_all_list", 0, 0);
					explore_interface->ExecXMBcommand("focus_segment_index seg_package_files", 0, 0);
				}
				else
				{
					explore_interface->ExecXMBcommand("focus_index rx_video", 0, 0);

					// open rx_video folder
					if(!explore_exec_push(200000, true) || !autoplay || strcasestr(param, ".mkv")) {is_busy = false; return;}

					explore_exec_push(2000000, true); // open Data Disc
				}
			}
		}
		else
 #endif
		{
			char category[16], seg_name[40]; *category = *seg_name = NULL;
			//if((atag && !l2) || (!atag && l2)) {sys_ppu_thread_sleep(1); launch_disc(category, seg_name, true);} // L2 + X
			if(!(webman_config->combo2 & PLAY_DISC) || atag) {sys_ppu_thread_sleep(1); launch_disc(category, seg_name, ((atag && !l2) || (!atag && l2)));}		// L2 + X

			autoplay = false;
		}

		if(autoplay)
		{
			explore_exec_push(2000000, false);
		}
	}
}

static bool game_mount(char *buffer, char *templn, char *param, char *tempstr, bool mount_ps3, bool forced_mount)
{
	bool mounted = false, umounted = false;

	// ---------------------
	// unmount current game
	// ---------------------
	if(islike(param, "/mount") && (strstr(param, "ps3/unmount") != NULL || strstr(param, "ps3/dev_bdvd") != NULL || strstr(param, "ps3/app_home") != NULL))
	{
		if(mount_ps3 || forced_mount || IS_ON_XMB)
		{
			do_umount(true);
			if(mount_ps3) {is_busy = false; return false;}
			umounted = true;
		}
	}

	// -----------------
	// unmount ps2_disc
	// -----------------
#ifdef PS2_DISC
	else if(islike(param, "/mount") && (strstr(param, "ps2/unmount") != NULL))
	{
		do_umount_ps2disc(false);

		if(mount_ps3) {is_busy = false; return false;}
		umounted = true;
	}
#endif

	if(umounted) {strcat(buffer, STR_GAMEUM); sprintf(templn, HTML_REDIRECT_TO_URL, "javascript:window.history.back();", HTML_REDIRECT_WAIT); strcat(buffer, templn);}


	// -----------------------
	// mount game / copy file
	// -----------------------
	else
	{
		// ---------------
		// init variables
		// ---------------
		u8 plen = MOUNT_CMD; // /mount.ps3
		enum icon_type default_icon = iPS3;

#ifdef COPY_PS3
		char target[STD_PATH_LEN], *pos; *target = NULL;
		if(islike(param, "/copy.ps3")) {plen = COPY_CMD; pos = strstr(param, "&to="); if(pos) {strcpy(target, pos + 4); *pos = NULL;}}
		bool is_copy = ((plen == COPY_CMD) && (copy_in_progress == false));
		char *wildcard = strstr(param, "*"); if(wildcard) *wildcard++ = NULL;

		if(is_copy)
		{
			if(islike(param,  "/dev_blind") || islike(param,  "/dev_hdd1")) mount_device(param,  NULL, NULL); // auto-mount source device
			if(islike(target, "/dev_blind") || islike(target, "/dev_hdd1")) mount_device(target, NULL, NULL); // auto-mount destination device
		}
#endif
		char enc_dir_name[STD_PATH_LEN*3], *source = param + plen;
		max_mapped = 0;

		// ----------------------------
		// remove url query parameters
		// ----------------------------
		char *purl = strstr(source, "emu="); // e.g. ?emu=ps1_netemu.self / ps1_emu.self / ps2_netemu.self / ps2_emu.self
		if(purl)
		{
			char *is_netemu = strstr(purl, "net");
			if(strcasestr(source, "ps2"))
				webman_config->ps2emu = is_netemu ? 1 : 0;
			else
				webman_config->ps1emu = is_netemu ? 1 : 0;
			purl--, *purl = NULL;
		}

#ifdef OFFLINE_INGAME
		purl = strstr(source, "offline=");
		if(purl) net_status = (*(purl + 8) == '0') ? 1 : 0;
#endif
		purl = strstr(source, "?random=");
		if(purl) *purl = NULL;

		// -------------------------
		// use relative source path
		// -------------------------
		if(!islike(source, "/net") && file_exists(source) == false) {sprintf(templn, "%s/%s", html_base_path, source + 1); if(file_exists(templn)) sprintf(source, "%s", templn);}

		// --------------
		// set mount url
		// --------------
		urlenc(templn, source);

		// -----------
		// mount game
		// -----------
#ifdef COPY_PS3
		if(script_running) forced_mount = true;

		if(!is_copy)
#endif
		{
			char *p = strstr(param, "/PS3_"); if(p) *p = NULL;

			int discboot = 0xff;
			xsetting_0AF1F161()->GetSystemDiscBootFirstEnabled(&discboot);

			if(discboot == 1)
				xsetting_0AF1F161()->SetSystemDiscBootFirstEnabled(0);

#ifdef PS2_DISC
			if(islike(param, "/mount.ps2"))
			{
				do_umount(true);
				mounted = mount_ps2disc(source);
			}
			else
			if(islike(param, "/mount_ps2"))
			{
				do_umount_ps2disc(false);
				mounted = mount_ps2disc(source);
			}
			else
#endif
			if(!mount_ps3 && !forced_mount && get_game_info())
			{
				sprintf(tempstr, "<H3>%s : <a href=\"/mount.ps3/unmount\">%s %s</a></H3><hr><a href=\"/mount_ps3%s\">", STR_UNMOUNTGAME, _game_TitleID, _game_Title, templn); strcat(buffer, tempstr);
			}
			else
			{
				mounted = mount_game(source, MOUNT_NORMAL);
			}

			if(discboot == 1)
				xsetting_0AF1F161()->SetSystemDiscBootFirstEnabled(1);
		}

		// -------------------
		// exit mount from XMB
		// -------------------
		if(mount_ps3)
		{
			is_busy = false;
			return mounted;
		}

		/////////////////
		// show result //
		/////////////////
		else if(*source == '/')
		{
			char _path[STD_PATH_LEN];
			size_t slen = 0;

			// ----------------
			// set mount label
			// ----------------

			if(islike(templn, "/net"))
			{
				utf8enc(_path, source, 0);
				slen = strlen(source);
			}
			else
			{
				slen = sprintf(_path, "%s", source);
			}

			// -----------------
			// get display icon
			// -----------------
			char *filename = strrchr(_path, '/'), *icon = tempstr;
			{
				char title_id[TITLEID_LEN], *d_name; *icon = *title_id = NULL;
				u8 f0 = strstr(filename, ".ntfs[") ? NTFS : 0, f1 = strstr(_path, "PS2") ? 5 : strstr(_path, "PSX") ? 6 : strstr(_path, "PSP") ? 8 : 2, is_dir = isDir(source);

				check_cover_folders(templn);

				// get iso name
				*filename = NULL; // sets _path
				d_name = filename + 1;

				char *buf = malloc(_4KB_);
				if(buf)
				{
					if(is_dir)
					{
						sprintf(templn, "%s/%s/PS3_GAME/PARAM.SFO", _path, d_name);
						get_title_and_id_from_sfo(templn, title_id, d_name, icon, buf, 0); f1 = 0;
					}
#ifdef COBRA_ONLY
					else
					{
						get_name_iso_or_sfo(templn, title_id, icon, _path, d_name, f0, f1, FROM_MOUNT, strlen(d_name), buf);
					}
#endif
					free(buf);
				}
				default_icon = get_default_icon(icon, _path, d_name, is_dir, title_id, NONE, f0, f1);

				*filename = '/';
			}

			urlenc(enc_dir_name, icon);
			htmlenc(_path, source, 0);

			// ----------------
			// set target path
			// ----------------
#ifdef COPY_PS3
			if(plen == COPY_CMD)
			{
				bool is_copying_from_hdd = islike(source, "/dev_hdd0");

				usb = get_default_usb_drive(0);

				#ifdef USE_UACCOUNT
				if(!webman_config->uaccount[0])
				#endif
					sprintf(webman_config->uaccount, "%08i", xsetting_CC56EB2D()->GetCurrentUserNumber());

				if(cp_mode)
				{
					sprintf(target, "%s", cp_path);
				}
				else
				if(*target) {if(!isDir(source) && isDir(target)) strcat(target, filename);} // &to=<destination>
				else
				{
					char *ext4 = source + slen - 4;
 #ifdef SWAP_KERNEL
					if(strstr(source, "/lv2_kernel"))
					{
						struct CellFsStat buf;
						if(cellFsStat(source, &buf) != CELL_FS_SUCCEEDED)
							sprintf(target, "%s", STR_ERROR);
						else
						{
							u64 size = buf.st_size;

							enable_dev_blind(source);

							// for cobra req: /dev_flash/sys/stage2.bin & /dev_flash/sys/lv2_self
							sprintf(target, SYS_COBRA_PATH "stage2.bin");
							if(isDir("/dev_flash/rebug/cobra"))
							{
								if(IS(ext4, ".dex"))
									sprintf(target, "%s/stage2.dex", "/dev_flash/rebug/cobra");
								else if(IS(ext4, ".cex"))
									sprintf(target, "%s/stage2.cex", "/dev_flash/rebug/cobra");
							}

							if(file_exists(target) == false)
							{
								sprintf(tempstr, "%s", source);
								strcpy(strrchr(tempstr, '/'), "/stage2.bin");
								if(file_exists(tempstr)) _file_copy(tempstr, target);
							}

							// copy: /dev_flash/sys/lv2_self
							sprintf(target, "/dev_blind/sys/lv2_self");
							if((cellFsStat(target, &buf) != CELL_FS_SUCCEEDED) || (buf.st_size != size))
								_file_copy(source, target);

							if((cellFsStat(target, &buf) == CELL_FS_SUCCEEDED) && (buf.st_size == size))
							{
								#define FLH_OS		0x2F666C682F6F732FULL
								#define OS_LV2		0x2F6F732F6C76325FULL

								#define LOCAL_S		0x2F6C6F63616C5F73ULL
								#define YS0_SYS		0x7973302F7379732FULL
								#define LV2_SELF	0x6C76325F73656C66ULL

								u64 lv2_offset = 0x15DE78; // 4.xx CFW LV1 memory location for: /flh/os/lv2_kernel.self
								if(peek_lv1(lv2_offset) != FLH_OS)
									for(u64 addr = 0x100000ULL; addr < 0xFFFFF8ULL; addr += 4) // Find in 16MB
										if(peek_lv1(addr) == OS_LV2)      // /os/lv2_
										{
											lv2_offset = addr - 4; break; // 0x12A2C0 on 3.55
										}

								if(peek_lv1(lv2_offset) == FLH_OS)  // Original: /flh/os/lv2_kernel.self
								{
									poke_lv1(lv2_offset + 0x00, LOCAL_S); // replace -> /local_sys0/sys/lv2_self
									poke_lv1(lv2_offset + 0x08, YS0_SYS);
									poke_lv1(lv2_offset + 0x10, LV2_SELF);

									working = 0;
									{ del_turnoff(0); }
									save_file(WMNOSCAN, NULL, 0);
									{system_call_3(SC_SYS_POWER, SYS_REBOOT, NULL, 0);} /*load LPAR id 1*/
									sys_ppu_thread_exit(0);
								}
							}
						}
						plen = 0; //do not copy
					}
					else
 #endif // #ifdef SWAP_KERNEL
					if(strstr(source, "/***PS3***/"))
					{
						sprintf(target, "/dev_hdd0/PS3ISO%s.iso", filename); // /copy.ps3/net0/***PS3***/GAMES/BLES12345  -> /dev_hdd0/PS3ISO/BLES12345.iso
					}
					else
					if(strstr(source, "/***DVD***/"))
					{
						sprintf(target, "/dev_hdd0/DVDISO%s.iso", filename); // /copy.ps3/net0/***DVD***/folder  -> /dev_hdd0/DVDISO/folder.iso
					}
					else if(IS(ext4, ".pkg"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/Packages", drives[usb]);
						else
							sprintf(target, "/dev_hdd0/packages");

						strcat(target, filename);
					}
					else if(_IS(ext4, ".bmp") || _IS(ext4, ".gif"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/PICTURE", drives[usb]);
						else
							sprintf(target, "%s/PICTURE", "/dev_hdd0");

						strcat(target, filename);
					}
					else if(_IS(ext4, ".jpg") || _IS(ext4, ".png"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/PICTURE", drives[usb]);
						else if(strstr(source, "BL") || strstr(param, "BC") || strstr(source, "NP"))
							sprintf(target, "/dev_hdd0/GAMES/covers");
						else
							sprintf(target, "%s/PICTURE", "/dev_hdd0");

						strcat(target, filename);
					}
					else if(strcasestr(source, "/covers"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/COVERS", drives[usb]);
						else
							sprintf(target, "/dev_hdd0/GAMES/covers");
					}
					else if(_IS(ext4, ".mp4") || _IS(ext4, ".mkv") || _IS(ext4, ".avi"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/VIDEO", drives[usb]);
						else
							sprintf(target, "/dev_hdd0/VIDEO");

						strcat(target, filename);
					}
					else if(_IS(ext4, ".mp3"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/MUSIC", drives[usb]);
						else
							sprintf(target, "%s/MUSIC", "/dev_hdd0");

						strcat(target, filename);
					}
					else if(IS(ext4, ".p3t"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/PS3/THEME", drives[usb]);
						else
							sprintf(target, "/dev_hdd0/theme");

						strcat(target, filename);
					}
					else if(!extcmp(source, ".edat", 5))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/exdata", drives[usb]);
						else
							sprintf(target, "%s/%s/exdata", "/dev_hdd0/home", webman_config->uaccount);

						strcat(target, filename);
					}
					else if(IS(ext4, ".rco") || strstr(source, "/coldboot"))
					{
						enable_dev_blind(NO_MSG);
						sprintf(target, "/dev_blind/vsh/resource");

						if(IS(ext4, ".raf"))
							strcat(target, "/coldboot.raf");
						else
							strcat(target, filename);
					}
					else if(IS(ext4, ".qrc"))
					{
						enable_dev_blind(NO_MSG);
						sprintf(target, "%s/qgl", "/dev_blind/vsh/resource");

						if(strstr(param, "/lines"))
							strcat(target, "/lines.qrc");
						else
							strcat(target, filename);
					}
					else if(strstr(source, "/exdata"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/exdata", drives[usb]);
						else
							sprintf(target, "%s/%s/exdata", "/dev_hdd0/home", webman_config->uaccount);
					}
					else if(strstr(source, "/PS3/THEME"))
						sprintf(target, "/dev_hdd0/theme");
					else if(strcasestr(source, "/savedata/"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/PS3/SAVEDATA", drives[usb]);
						else
							sprintf(target, "%s/%s/savedata", "/dev_hdd0/home", webman_config->uaccount);

						strcat(target, filename);
					}
					else if(strcasestr(source, "/trophy/"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/PS3/TROPHY", drives[usb]);
						else
							sprintf(target, "%s/%s/trophy", "/dev_hdd0/home", webman_config->uaccount);

						strcat(target, filename);
					}
					else if(strstr(source, "/webftp_server"))
					{
						sprintf(target, "%s/webftp_server.sprx", "/dev_hdd0/plugins");
						if(file_exists(target) == false) sprintf(target + 31, "_ps3mapi.sprx");
						if(file_exists(target) == false) sprintf(target + 10, "webftp_server.sprx");
						if(file_exists(target) == false) sprintf(target + 23, "_ps3mapi.sprx");
					}
					else if(strstr(source, "/boot_plugins"))
					{
						if(cobra_version == 0)
							sprintf(target, "/dev_hdd0/boot_plugins_nocobra.txt");
						else
							sprintf(target, "/dev_hdd0/boot_plugins.txt");
					}
					else if(is_copying_from_hdd)
						sprintf(target, "%s%s", drives[usb], source + 9);
					else if(islike(source, "/dev_usb"))
						sprintf(target, "%s%s", "/dev_hdd0", source + 11);
					else if(islike(source, "/net"))
						sprintf(target, "%s%s", "/dev_hdd0", source + 5);
					else
					{
						if(islike(source, "/dev_bdvd"))
						{
							{system_call_1(36, (u64) "/dev_bdvd");} // decrypt dev_bdvd files

							sprintf(target, "%s/%s", "/dev_hdd0/GAMES", "My Disc Backup");

							char title[80];
							sprintf(title, "/dev_bdvd/PS3_GAME/PARAM.SFO");
							if(file_exists(title))
							{
								char title_id[TITLEID_LEN];
								getTitleID(title, title_id, GET_TITLE_AND_ID);
								if(*title_id && (title_id[8] >= '0'))
								{
									if(strstr(title, " ["))
										sprintf(target, "%s/%s", "/dev_hdd0/GAMES", title);
									else
										sprintf(target, "%s/%s [%s]", "/dev_hdd0/GAMES", title, title_id);
								}
							}
						}
						else
							sprintf(target, "/dev_hdd0");

						char *p = strstr(source + 9, "/");
						if(p) strcat(target, p);
					}
				}

				// ------------------
				// show copying file
				// ------------------
				filepath_check(target);
#ifdef USE_NTFS
				filepath_check(source);
#endif

				bool is_error = ((islike(target, drives[usb]) && isDir(drives[usb]) == false)) || islike(target, source) || !sys_admin;

				// show source path
				strcat(buffer, STR_COPYING); strcat(buffer, " ");
				add_breadcrumb_trail(buffer, source); strcat(buffer, "<hr>");

				// show image
				urlenc(_path, target);
				sprintf(tempstr, "<a href=\"%s\"><img src=\"%s\" border=0></a><hr>%s %s: ",
								 _path, enc_dir_name, is_error ? STR_ERROR : "", STR_CPYDEST); strcat(buffer, tempstr);

				// show target path
				add_breadcrumb_trail(buffer, target); *tempstr = NULL;

				if(strstr(target, "/webftp_server")) {sprintf(tempstr, "<HR>%s", STR_SETTINGSUPD);} else
				if(cp_mode) {char *p = strrchr(_path, '/'); *p = NULL; sprintf(tempstr, HTML_REDIRECT_TO_URL, _path, HTML_REDIRECT_WAIT);}

				if(is_error) {show_msg((char*)STR_CPYABORT); cp_mode = CP_MODE_NONE; return false;}
			}
			else
#endif // #ifdef COPY_PS3

			// ------------------
			// show mounted game
			// ------------------
			{
				size_t mlen;
				bool is_gamei = false;
				bool is_movie = strstr(param, "/BDISO") || strstr(param, "/DVDISO") || !extcmp(param, ".ntfs[BDISO]", 12) || !extcmp(param, ".ntfs[DVDISO]", 13);

#ifndef ENGLISH_ONLY
				char buf[296];
				char *STR_GAMETOM  = buf; //[48];//	= "Game to mount";
				char *STR_MOVIETOM = buf; //[48];//	= "Movie to mount";
				if(is_movie)	language("STR_MOVIETOM", STR_MOVIETOM, "Movie to mount");
				else			language("STR_GAMETOM",  STR_GAMETOM,  "Game to mount");
				close_language();
#endif
				strcat(buffer, "<div id=\"mount\">");
				strcat(buffer, is_movie ? STR_MOVIETOM : STR_GAMETOM); strcat(buffer, ": "); add_breadcrumb_trail(buffer, source);

				//if(strstr(param, "PSX")) {sprintf(tempstr, " <font size=2>[CD %i • %s]</font>", CD_SECTOR_SIZE_2352, (webman_config->ps1emu) ? "ps1_netemu.self" : "ps1_emu.self"); strcat(buffer, tempstr);}
#ifdef PKG_LAUNCHER
				is_gamei = strstr(param, "/GAMEI/");
#endif
				if(is_movie)
				{
#ifndef ENGLISH_ONLY
					char *STR_MOVIELOADED = buf; //[272];//= "Movie loaded successfully. Start the movie from the disc icon<br>under the Video column.</a><hr>Click <a href=\"/mount.ps3/unmount\">here</a> to unmount the movie.";
					sprintf(STR_MOVIELOADED, "Movie %s%s%smovie.",
											 "loaded successfully. Start the ", "movie from the disc icon<br>under the Video column"                , ".</a><hr>Click <a href=\"/mount.ps3/unmount\">here</a> to unmount the ");
					language("STR_MOVIELOADED", STR_MOVIELOADED, STR_MOVIELOADED);
#endif
					mlen = sprintf(tempstr, "<hr><a href=\"/play.ps3\"><img src=\"%s\" onerror=\"this.src='%s';\" border=0></a>"
											"<hr><a href=\"/dev_bdvd\">%s</a>", enc_dir_name, wm_icons[strstr(param,"BDISO") ? iBDVD : iDVD], mounted ? STR_MOVIELOADED : STR_ERROR);
				}
				else if(!extcmp(param, ".BIN.ENC", 8))
				{
#ifndef ENGLISH_ONLY
					char *STR_PS2LOADED = buf; //[240]; //	= "Game loaded successfully. Start the game using <b>PS2 Classic Launcher</b>.<hr>";
					sprintf(STR_PS2LOADED,   "Game %s%s%s</b>.<hr>",
											 "loaded successfully. Start the ", "game using <b>", "PS2 Classic Launcher");
					language("STR_PS2LOADED", STR_PS2LOADED, STR_PS2LOADED);
#endif
					mlen = sprintf(tempstr, "<hr><img src=\"%s\" onerror=\"this.src='%s';\" height=%i>"
											"<hr>%s", enc_dir_name, wm_icons[iPS2], 300, mounted ? STR_PS2LOADED : STR_ERROR);
				}
				else if(((strstr(param, "/PSPISO") || strstr(param, "/ISO/")) && !extcasecmp(param, ".iso", 4)) || is_gamei)
				{
#ifndef ENGLISH_ONLY
					char *STR_PSPLOADED = buf; //[232]; //	= "Game loaded successfully. Start the game using <b>PSP Launcher</b>.<hr>";
					sprintf(STR_PSPLOADED,   "Game %s%s%s</b>.<hr>",
											 "loaded successfully. Start the ", "game using <b>", "PSP Launcher");
					language("STR_PSPLOADED", STR_PSPLOADED, STR_PSPLOADED);

#endif
#ifdef PKG_LAUNCHER
					if(is_gamei)
					{
						char *pos = strstr(STR_PSPLOADED, "PSP Launcher"); if(pos) strcpy(pos, "PKG Launcher");
					}
#endif
					mlen = sprintf(tempstr, "<hr><img src=\"%s\" onerror=\"this.src='%s';\" height=%i>"
											"<hr>%s", enc_dir_name, wm_icons[iPSP], strcasestr(enc_dir_name,".png") ? 200 : 300, mounted ? STR_PSPLOADED : STR_ERROR);
				}
				else
				{
#ifndef ENGLISH_ONLY
					char *STR_GAMELOADED = buf; //[288];//	= "Game loaded successfully. Start the game from the disc icon<br>or from <b>/app_home</b>&nbsp;XMB entry.</a><hr>Click <a href=\"/mount.ps3/unmount\">here</a> to unmount the game.";
					sprintf(STR_GAMELOADED,  "Game %s%s%sgame.",
											 "loaded successfully. Start the ", "game from the disc icon<br>or from <b>/app_home</b>&nbsp;XMB entry", ".</a><hr>Click <a href=\"/mount.ps3/unmount\">here</a> to unmount the ");
					language("STR_GAMELOADED", STR_GAMELOADED, STR_GAMELOADED);
#endif
					mlen = sprintf(tempstr, "<hr><a href=\"/play.ps3\"><img src=\"%s\" onerror=\"this.src='%s';\" border=0></a>"
											"<hr><a href=\"/dev_bdvd\">%s</a>", enc_dir_name, wm_icons[default_icon], mounted ? STR_GAMELOADED : STR_ERROR);
				}

				if(!mounted)
				{
#ifndef ENGLISH_ONLY
					if(webman_config->lang == 0)
#endif
						if(is_mounting) mlen += sprintf(tempstr + mlen, " A previous mount is in progress.");
#ifndef LITE_EDITION
 #ifdef COBRA_ONLY
					if(islike(param + MOUNT_CMD, "/net") && !is_netsrv_enabled(param[4 + MOUNT_CMD])) mlen += sprintf(tempstr + mlen, " /net%c %s", param[4 + MOUNT_CMD], STR_DISABLED);
 #endif
#endif
					if(!forced_mount && IS_INGAME) sprintf(tempstr + mlen, " <a href=\"/mount_ps3%s\">/mount_ps3%s</a>", param + MOUNT_CMD, param + MOUNT_CMD);
				}
#ifndef ENGLISH_ONLY
				close_language();
#endif
			}

			strcat(buffer, tempstr);

			// ----------------------------
			// show associated [PS2] games
			// ----------------------------
#ifdef PS2_DISC
			if(mounted && (strstr(source, "/GAME") || strstr(source, "/PS3ISO") || strstr(source, ".ntfs[PS3ISO]")))
			{
				int fd2; u16 pcount = 0; u32 tlen = strlen(buffer) + 8; u8 is_iso = 0;

				sprintf(target, "%s", source);
				if(strstr(target, "Sing"))
				{
					if(strstr(target, "/PS3ISO")) {strcpy(strstr(target, "/PS3ISO"), "/PS2DISC\0"); is_iso = 1;}
					if(strstr(target, ".ntfs[PS3ISO]")) {strcpy(target, "/dev_hdd0/PS2DISC\0"); is_iso = 1;}
				}

				// -----------------------------
				// get [PS2] extracted folders
				// -----------------------------
				if(cellFsOpendir(target, &fd2) == CELL_FS_SUCCEEDED)
				{
					CellFsDirectoryEntry dir; u32 read_e;
					char *entry_name = dir.entry_name.d_name;

					while(working && (!cellFsGetDirectoryEntries(fd2, &dir, sizeof(dir), &read_e) && read_e))
					{
						if((entry_name[0] == '.')) continue;

						if(is_iso || strstr(entry_name, "[PS2") != NULL)
						{
							if(pcount == 0) strcat(buffer, "<br><HR>");
							urlenc(enc_dir_name, entry_name);
							tlen += sprintf(templn, "<a href=\"/mount.ps2%s/%s\">%s</a><br>", target, enc_dir_name, entry_name);

							if(tlen > (BUFFER_SIZE - _2KB_)) break;
							strcat(buffer, templn); pcount++;
						}
					}
					cellFsClosedir(fd2);
				}
			}
#endif // #ifdef PS2_DISC
		}

		// -------------
		// perform copy
		// -------------
#ifdef COPY_PS3
		if(sys_admin && is_copy)
		{
			if(islike(target, source))
				{sprintf(templn, "<hr>%s %s %s", STR_ERROR, STR_CPYDEST, source); strcat(buffer, templn);}
			else if((!islike(source, "/net")) && file_exists(source) == false)
				{sprintf(templn, "<hr>%s %s %s", STR_ERROR, source, STR_NOTFOUND); strcat(buffer, templn);}
			else
			{
				setPluginActive();

				// show msg begin
				sprintf(templn, "%s %s\n%s %s", STR_COPYING, source, STR_CPYDEST, target);
				show_msg(templn);
				copy_in_progress = true, copied_count = 0;

				if(islike(target, "/dev_blind")) enable_dev_blind(NO_MSG);

				if(isDir(source) && (strlen(target) > 3) && target[strlen(target)-1] != '/') strcat(target, "/");

				// make target dir tree
				mkdir_tree(target);

				// copy folder to target
				if(strstr(source,"/exdata"))
				{
					scan(source, false, ".edat", islike(source, "/dev_usb") ? SCAN_COPYBK : SCAN_COPY, target);
				}
				else if(wildcard)
					scan(source, true, wildcard, SCAN_COPY, target);
				else if(isDir(source))
					folder_copy(source, target);
				else
					file_copy(source, target, COPY_WHOLE_FILE);

				copy_in_progress = false;

				// show msg end
				if(copy_aborted)
					show_msg((char*)STR_CPYABORT);
				else
				{
					show_msg((char*)STR_CPYFINISH);
					if(do_restart) { { del_turnoff(2); } vsh_reboot();}
				}

				setPluginInactive();
			}

			if(!copy_aborted && (cp_mode == CP_MODE_MOVE) && file_exists(target)) del(source, true);
			if(cp_mode) {cp_mode = CP_MODE_NONE, *cp_path = NULL;}
		}
#endif //#ifdef COPY_PS3
	}

	return mounted;
}

#ifdef COBRA_ONLY
static void set_apphome(const char *game_path)
{
	if(game_path)
		sys_map_path("/app_home", game_path);
	else
		sys_map_path("/app_home", isDir("/dev_hdd0/packages") ? (char*)"/dev_hdd0/packages" : NULL); // Enable install all packages on HDD

	sys_map_path("/app_home/USRDIR/", NULL);
	sys_map_path("/app_home/USRDIR", NULL);
	sys_map_path("/app_home/PS3_GAME/", game_path);
	sys_map_path("/app_home/PS3_GAME", game_path);
}

static void do_umount_iso(void)
{
	unsigned int real_disctype, effective_disctype, iso_disctype;
	cobra_get_disc_type(&real_disctype, &effective_disctype, &iso_disctype);

	// If there is an effective disc in the system, it must be ejected
	if(effective_disctype != DISC_TYPE_NONE)
	{
		cobra_send_fake_disc_eject_event();
		wait_path("/dev_bdvd", 1, false);
	}

	if(iso_disctype != DISC_TYPE_NONE) cobra_umount_disc_image();

	// If there is a real disc in the system, issue an insert event
	if(real_disctype != DISC_TYPE_NONE)
	{
		cobra_send_fake_disc_insert_event();
		wait_for("/dev_bdvd", 1);
		cobra_disc_auth();
	}
}
#endif

static void do_umount(bool clean)
{
	if(clean) cellFsUnlink(LAST_GAME_TXT);

#ifdef USE_NTFS
	root_check = true;
#endif

	cellFsUnlink("/dev_hdd0/tmp/game/ICON0.PNG"); // remove XMB disc icon

	if(fan_ps2_mode) reset_fan_mode(); // restore normal fan mode

#ifdef COBRA_ONLY
	{
		{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

		cobra_unset_psp_umd(); // eject PSPISO

		do_umount_iso();	// unmount iso
 #ifdef PS2_DISC
		do_umount_ps2disc(false); // unmount ps2disc
 #endif
		sys_ppu_thread_usleep(20000);

		cobra_unload_vsh_plugin(0); // unload external rawseciso / netiso plugins

		cellFsChmod((char*)"/dev_bdvd/PS3_GAME/SND0.AT3", MODE); // restore SND0 permissions of game mounted (JB folder)

		sys_map_path("/dev_bdvd/PS3/UPDATE", NULL); // unmap UPDATE from bdvd

		// unmap bdvd & apphome
		sys_map_path("/dev_bdvd", NULL);
		sys_map_path("//dev_bdvd", NULL);

		set_apphome(NULL); // unmap app_home

		// unmap GAMEI & PKGLAUNCH
 #ifdef PKG_LAUNCHER
		if(*map_title_id)
		{
			char gamei_mapping[32];
			sprintf(gamei_mapping, HDD0_GAME_DIR "%s", map_title_id);
			sys_map_path(gamei_mapping, NULL);
			sys_map_path(PKGLAUNCH_DIR, NULL);
		}
 #endif

 #ifdef USE_INTERNAL_PLUGIN
		// unload internal netiso or rawiso plugin
		{
			sys_ppu_thread_t t_id;
			u64 exit_code;
		#ifdef NET_SUPPORT
			sys_ppu_thread_create(&t_id, netiso_stop_thread, NULL, THREAD_PRIO_STOP, THREAD_STACK_SIZE_STOP_THREAD, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
			sys_ppu_thread_join(t_id, &exit_code);
		#endif
			sys_ppu_thread_create(&t_id, rawseciso_stop_thread, NULL, THREAD_PRIO_STOP, THREAD_STACK_SIZE_STOP_THREAD, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
			sys_ppu_thread_join(t_id, &exit_code);

			// wait for unload of netiso or rawiso plugin
		#ifdef NET_SUPPORT
			while(netiso_loaded || rawseciso_loaded) {sys_ppu_thread_usleep(100000);}
		#else
			while(rawseciso_loaded) {sys_ppu_thread_usleep(100000);}
		#endif
		}
 #endif

		// send fake eject
		sys_ppu_thread_usleep(4000);
		cobra_send_fake_disc_eject_event();
		sys_ppu_thread_usleep(4000);

		{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }
	}
#else
	{
		pokeq(0x8000000000000000ULL + MAP_ADDR, 0x0000000000000000ULL);
		pokeq(0x8000000000000008ULL + MAP_ADDR, 0x0000000000000000ULL);

		//eject_insert(1, 1);

		if(isDir("/dev_flash/pkg"))
			mount_game((char*)"/dev_flash/pkg", MOUNT_SILENT);
	}

#endif //#ifdef COBRA_ONLY
}

static void get_last_game(char *last_path)
{
	read_file(LAST_GAME_TXT, last_path, STD_PATH_LEN, 0);
}

#ifdef COBRA_ONLY
static void cache_file_to_hdd(char *source, char *target, const char *basepath, char *msg)
{
	if(*source == '/')
	{
		sprintf(target, "/dev_hdd0%s", basepath);
		cellFsMkdir(basepath, DMODE);

		strcat(target, strrchr(source, '/')); // add file name

		if((copy_in_progress || fix_in_progress) == false && file_exists(target) == false)
		{
			sprintf(msg, "%s %s\n"
						 "%s %s", STR_COPYING, source, STR_CPYDEST, basepath);
			show_msg(msg);

			dont_copy_same_size = true;
			copy_in_progress = true, copied_count = 1;
			file_copy(source, target, COPY_WHOLE_FILE);
			copy_in_progress = false;

			if(copy_aborted)
			{
				cellFsUnlink(target);
				show_msg((char*)STR_CPYABORT);
			}
		}

		if(file_exists(target)) strcpy(source, target);
	}

	do_umount(false);
}

static void cache_icon0_and_param_sfo(char *destpath)
{
	wait_for("/dev_bdvd", 15);

	char *ext = destpath + strlen(destpath);
	strcat(ext, ".SFO\0");
	dont_copy_same_size = false;

	// cache PARAM.SFO
	if(file_exists(destpath) == false)
	{
		for(u8 retry = 0; retry < 10; retry++)
		{
			if(file_copy((char*)"/dev_bdvd/PS3_GAME/PARAM.SFO", destpath, _4KB_) >= CELL_FS_SUCCEEDED) break;
			sys_ppu_thread_usleep(500000);
		}
	}

	// cache ICON0.PNG
	strcpy(ext, ".PNG");
	if((webman_config->nocov!=2) && file_exists(destpath) == false)
	{
		for(u8 retry = 0; retry < 10; retry++)
		{
			if(file_copy((char*)"/dev_bdvd/PS3_GAME/ICON0.PNG", destpath, COPY_WHOLE_FILE) >= CELL_FS_SUCCEEDED) break;
			sys_ppu_thread_usleep(500000);
		}
	}

	dont_copy_same_size = true;
}
#endif

#ifdef COBRA_ONLY
static void mount_on_insert_usb(bool on_xmb, char *msg)
{
	// Auto-mount x:\AUTOMOUNT.ISO or JB game found on root of USB devices (dev_usb00x, dev_sd, dev_ms, dev_cf)
	if(is_mounting) ;

	else if(on_xmb)
	{
		if(!isDir("/dev_bdvd"))
		{
			if(webman_config->autob)
				for(u8 f0 = 1; f0 < 16; f0++)
				{
					if(IS_NET || IS_NTFS) continue;

					if(!check_drive(f0))
					{
						if(automount != f0)
						{
							sprintf(msg, "%s/AUTOMOUNT.ISO", drives[f0]);
							if(file_exists(msg)) {mount_game(msg, MOUNT_SILENT); automount = f0; break;}
							else
							{
								sprintf(msg, "%s/PS3_GAME/PARAM.SFO", drives[f0]);
								if(file_exists(msg)) {mount_game(msg, MOUNT_SILENT); automount = f0; break;}
							}
						}
						else if(!isDir(drives[f0])) automount = 0;
					}
				}
			else
			{
				automount = 0; if(fan_ps2_mode) enable_fan_control(PS2_MODE_OFF, msg);
			}
		}
		else if((automount == 0) && IS_ON_XMB)
		{
			unsigned int real_disctype, effective_disctype, iso_disctype;
			cobra_get_disc_type(&real_disctype, &effective_disctype, &iso_disctype);

#ifdef REMOVE_SYSCALLS
			if((iso_disctype == DISC_TYPE_NONE) && (real_disctype == DISC_TYPE_PS3_BD))
			{
				if(!syscalls_removed && webman_config->dsc) disable_cfw_syscalls(webman_config->keep_ccapi);
			}
#endif
			if((effective_disctype == DISC_TYPE_PS2_DVD) || (effective_disctype == DISC_TYPE_PS2_CD) || (real_disctype == DISC_TYPE_PS2_DVD) || (real_disctype == DISC_TYPE_PS2_CD) || (iso_disctype == DISC_TYPE_PS2_DVD) || (iso_disctype == DISC_TYPE_PS2_CD))
			{
				if(webman_config->fanc) restore_fan(SET_PS2_MODE); //set_fan_speed( ((webman_config->ps2temp*255)/100), 0);

				// create "wm_noscan" to avoid re-scan of XML returning to XMB from PS2
				save_file(WMNOSCAN, NULL, 0);
			}
			else if(fan_ps2_mode) enable_fan_control(PS2_MODE_OFF, msg);

			automount = 99;
		}
	}
	else if(fan_ps2_mode && IS_INGAME)
	{
		automount = 0; enable_fan_control(PS2_MODE_OFF, msg);
	}
}
#endif

static void mount_autoboot(void)
{
	char path[STD_PATH_LEN + 1]; bool is_last_game = false;

	// get autoboot path
	if(webman_config->autob && (
#ifdef COBRA_ONLY
		islike(webman_config->autoboot_path, "/net") ||
#endif
		islike(webman_config->autoboot_path, "http") || file_exists(webman_config->autoboot_path)
	)) // autoboot
		strcpy(path, (char *) webman_config->autoboot_path);
	else if(webman_config->lastp)
	{
		get_last_game(path); is_last_game = true;
	}
	else return;

	bool do_mount = false;

	CellPadData pad_data = pad_read();

	// prevent auto-mount on startup if L2+R2 is pressed
	if(pad_data.len > 0 && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2))) { if(!webman_config->nobeep) BEEP2; return;}

	if(from_reboot && *path && (strstr(path, "/PS2") != NULL)) return; //avoid re-launch PS2 returning to XMB

	// wait few seconds until path becomes ready
#ifdef COBRA_ONLY
	if((strlen(path) > 8) || islike(path, "/net"))
#else
	if(strlen(path) > 8)
#endif
	{
		wait_for(path, 2 * (webman_config->boots + webman_config->bootd));
#ifdef COBRA_ONLY
		do_mount = ( islike(path, "/net") || islike(path, "http") || file_exists(path) );
#else
		do_mount = ( islike(path, "http") || file_exists(path) );
#endif
	}

	if(do_mount)
	{   // add some delay
		if(webman_config->delay)      {sys_ppu_thread_sleep(3); wait_for(path, 2 * (webman_config->boots + webman_config->bootd));}
#ifndef COBRA_ONLY
		if(islike(path, "/net") || (strstr(path, ".ntfs[") != NULL)) return;
#endif

		if(is_last_game)
		{
			// prevent auto-launch game on boot (last game only). AUTOBOOT.ISO is allowed to auto-launch on boot
			int discboot = 0xff;
			xsetting_0AF1F161()->GetSystemDiscBootFirstEnabled(&discboot);
			if(discboot) xsetting_0AF1F161()->SetSystemDiscBootFirstEnabled(0); // disable Disc Boot
			if(!islike(path, "/dev_hdd0/game")) mount_game(path, MOUNT_NORMAL);
			sys_ppu_thread_sleep(5);
			if(discboot) xsetting_0AF1F161()->SetSystemDiscBootFirstEnabled(1); // restore Disc Boot setting
		}
		else
		{
			if(mount_game(path, MOUNT_NORMAL)) auto_play(path);
		}
	}
}

/***********************************************/
/* mount_thread parameters                     */
/***********************************************/
static char *_path0;
static bool mount_ret = false;
/***********************************************/

static void mount_thread(u64 action)
{
	bool ret = false;

	automount = 0;

	// --------------------------------------------
	// show message if syscalls are fully disabled
	// --------------------------------------------
#ifdef COBRA_ONLY
	if(syscalls_removed && peekq(TOC) != SYSCALLS_UNAVAILABLE) syscalls_removed = false;

	if(syscalls_removed || peekq(TOC) == SYSCALLS_UNAVAILABLE)
	{
		syscalls_removed = true;
		{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

		int ret_val = NONE; { system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PCHECK_SYSCALL8); ret_val = (int)p1;}

		if(ret_val < 0) { show_msg((char*)STR_CFWSYSALRD); { PS3MAPI_DISABLE_ACCESS_SYSCALL8 } goto finish; }
		if(ret_val > 1) { system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, 1); }
	}

#else

	if(syscalls_removed || peekq(TOC) == SYSCALLS_UNAVAILABLE) { show_msg(STR_CFWSYSALRD); goto finish; }

#endif

	// -----------------
	// fix mount errors
	// -----------------

	if(fan_ps2_mode) reset_fan_mode();

	// -----------------------------
	// fix mount errors (patch lv2)
	// -----------------------------

	patch_lv2();


	// ---------------------------------------
	// exit if mounting a path from /dev_bdvd
	// ---------------------------------------

	if(islike(_path0, "/dev_bdvd")) {do_umount(false); goto finish;}


	// ---------------
	// init variables
	// ---------------

	char netid = NULL;
	char _path[STD_PATH_LEN], title_id[TITLEID_LEN];

	ret = true;

	mount_unk = EMU_OFF;

	led(GREEN, BLINK_FAST);

	// ----------------
	// open url & exit
	// ----------------
	if(islike(_path0 + 1, "http") || islike(_path0, "http") || !extcmp(_path0, ".htm", 4))
	{
		char *url = strstr(_path0, "http");

		if(!url) {url = _path; sprintf(url, "http://%s%s", local_ip, _path0);}

		if(IS_ON_XMB)
		{
			while(View_Find("explore_plugin") == 0) sys_ppu_thread_sleep(1); // wait for explore_plugin

			do_umount(false);
			open_browser(url, 0);
		}
		else
			ret = false;

		goto finish;
	}
#ifdef COPY_PS3
	else if(!extcmp(_path0, ".txt", 4) || !extcmp(_path0, ".bat", 4))
	{
		parse_script(_path0);
		ret = false;
		goto finish;
	}
#endif

	// -----------------
	// remove /PS3_GAME
	// -----------------

	sprintf(_path, "%s", _path0);

	if(*_path == '/')
	{
		char *p = strstr(_path, "/PS3_"); if(p) *p = NULL;
	}

	// ---------------------------------------------
	// skip last game if mounting /GAMEI (nonCobra)
	// ---------------------------------------------

#ifndef COBRA_ONLY
 #ifdef EXT_GDATA
	if(action == MOUNT_EXT_GDATA) goto install_mm_payload;
 #endif
#endif

	#include "mount_lastgames.h"

	// ----------------------------------------
	// show start mounting message (game path)
	// ----------------------------------------

	if(action == EXPLORE_CLOSE_ALL) {action = MOUNT_NORMAL; explore_close_all(_path);}

	if(action) show_msg(_path);

	cellFsUnlink(WMNOSCAN); // remove wm_noscan if a PS2ISO has been mounted

	///////////////////////
	// MOUNT ISO OR PATH //
	///////////////////////

	#include "mount_misc.h"
	#include "mount_cobra.h"
	#include "mount_noncobra.h"

exit_mount:

	// -------------------------------
	// show 2nd message: "xxx" loaded
	// -------------------------------

	if(action == MOUNT_SILENT) goto mounting_done;

	if(ret && *_path == '/')
	{
		char msg[STD_PATH_LEN + 48], *pos;

		// get file name (without path)
		pos = strrchr(_path, '/');
		sprintf(msg, "\"%s", pos + 1);

		// remove file extension
		pos = strstr(msg, ".ntfs["); if(pos) *pos = NULL;
		pos = strrchr(msg, '.'); if(pos) *pos = NULL;
		if(msg[1] == NULL) sprintf(msg, "\"%s", _path);

		// show loaded path
		strcat(msg, "\" "); strcat(msg, STR_LOADED2);
		show_msg(msg);
	}

	// ---------------
	// delete history
	// ---------------

	delete_history(false);

	if(mount_unk >= EMU_MAX) goto mounting_done;

	// -------------------------------------------
	// wait few seconds until the bdvd is mounted
	// -------------------------------------------

	if(ret && extcmp(_path, ".BIN.ENC", 8))
	{
		wait_for("/dev_bdvd", (islike(_path, "/dev_hdd0") ? 6 : netid ? 20 : 15));
		if(!isDir("/dev_bdvd")) ret = false;
	}

#ifdef FIX_GAME
	// -------------------------------------------------------
	// re-check PARAM.SFO to notify if game needs to be fixed
	// -------------------------------------------------------

	if(ret && (c_firmware < LATEST_CFW))
	{
		char filename[64];
		sprintf(filename, "/dev_bdvd/PS3_GAME/PARAM.SFO");
		getTitleID(filename, title_id, GET_TITLE_ID_ONLY);

		// check update folder
		sprintf(filename, "%s%s%s", HDD0_GAME_DIR, title_id, "/PARAM.SFO");

		if(file_exists(filename) == false)
			sprintf(filename, "/dev_bdvd/PS3_GAME/PARAM.SFO");

		getTitleID(filename, title_id, SHOW_WARNING);
	}
#endif

	// -----------------------------------
	// show error if bdvd was not mounted
	// -----------------------------------

	if(!ret && !isDir("/dev_bdvd")) {char msg[STD_PATH_LEN + 20]; sprintf(msg, "%s %s", STR_ERROR, _path); show_msg(msg);}

	// -------------------------------------------------------------------------------------
	// remove syscalls hodling R2 (or prevent remove syscall if path contains [online] tag)
	// -------------------------------------------------------------------------------------

#ifdef REMOVE_SYSCALLS
	else if(mount_unk == EMU_PS3)
	{
		CellPadData pad_data = pad_read();
		bool otag = (strcasestr(_path, ONLINE_TAG)!=NULL);
		bool r2 = (pad_data.len > 0 && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2));
		if((!r2 && otag) || (r2 && !otag)) disable_cfw_syscalls(webman_config->keep_ccapi);
	}
#endif

mounting_done:

#ifdef COBRA_ONLY

	// ------------------------------------------------------------------
	// auto-enable gamedata on bdvd if game folder or ISO contains GAMEI
	// ------------------------------------------------------------------

 #ifdef EXT_GDATA
	if((extgd == 0) && isDir("/dev_bdvd/GAMEI")) set_gamedata_status(2, true); // auto-enable external gameDATA (if GAMEI exists on /bdvd)
 #endif

	// -----------------------------------------------
	// redirect system files (PUP, net/PKG, SND0.AT3)
	// -----------------------------------------------
	{
		cellFsChmod("/dev_bdvd/PS3_GAME/SND0.AT3", webman_config->nosnd0 ? NOSND : MODE);

		apply_remaps();

		if(ret && file_exists("/dev_bdvd/PS3UPDAT.PUP"))
		{
			sys_map_path("/dev_bdvd/PS3/UPDATE", "/dev_bdvd"); //redirect root of bdvd to /dev_bdvd/PS3/UPDATE (allows update from mounted /net folder or fake BDFILE)
		}

		if(ret && ((!netid) && isDir("/dev_bdvd/PKG")))
		{
			sys_map_path("/app_home", "/dev_bdvd/PKG"); //redirect net_host/PKG to app_home
		}

		{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }
	}
#endif

	// --------------
	// exit function
	// --------------

finish:

#ifdef VIRTUAL_PAD
	unregister_ldd_controller();
#endif

	led(GREEN, ON);
	max_mapped = 0;
	mount_ret = ret;
	mount_unk = EMU_OFF;

	is_mounting = false;
	sys_ppu_thread_exit(0);
}

static bool mount_game(const char *path, u8 action)
{
#ifndef LITE_EDITION
 #ifdef COBRA_ONLY
	if(islike(path, "/net") && !is_netsrv_enabled(path[4])) return false;
 #endif
#endif
	if(is_mounting) return false; is_mounting = true;

	_path0 = (char*)path;

	sys_ppu_thread_t t_id;
	sys_ppu_thread_create(&t_id, mount_thread, (u64)action, THREAD_PRIO_HIGH, THREAD_STACK_SIZE_MOUNT_GAME, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_CMD);

	while(is_mounting && working) sys_ppu_thread_sleep(1); // wait until thread mount game

	close_ftp_sessions_idle();

	_path0 = NULL;

	return mount_ret;
}
