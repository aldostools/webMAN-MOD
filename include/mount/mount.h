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
// [psn]     Map /dev_hdd0/game/[GAME_ID] to /dev_bdvd/PS3_GAME (if the folder does not exist)
// [bios]    Use external ps1_bios.bin

// hold CROSS = force Auto-Play
// hold CIRCLE = cancel Auto-Play
// hold L2 = mount PSX with ps1_emu / PS2 with ps2_emu
// hold R2 = mount PSX with ps1_netemu / PS2 with ps2_netemu / disable syscalls on PS3
// hold R1 = toggle emu / netemu

#define TITLEID_LEN		10

static int8_t check_multipsx = NONE;

static u8 force_ap = 0;
static u8 mount_app_home = false; // force mount JB folder in /app_home [false = use webman_config->app_home]
static u8 silent_mode = false;

#ifdef MOUNT_GAMEI
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
#define MOUNT_NEXT_CD	4	// MOUNT_NORMAL + mount next CD (PSXISO)

// /mount_ps3/<path>[?random=<x>[&emu={ ps1_netemu.self / ps1_emu.self / ps2_netemu.self / ps2_emu.self }][offline={0/1}]
// /mount.ps3/<path>[?random=<x>[&emu={ ps1_netemu.self / ps1_emu.self / ps2_netemu.self / ps2_emu.self }][offline={0/1}]
// /mount.ps3/unmount
// /mount.ps2/<path>[?random=<x>]
// /mount.ps2/unmount
// /copy.ps3/<path>[&to=<destination>]

#ifdef COPY_PS3
static u8 usb = 1; // first connected usb drive [used by /copy.ps3 & in the tooltips for /copy.ps3 links in the file manager]. 1 = /dev_usb000
#endif

static void auto_play(char *param, u8 force_autoplay)
{
#ifdef OFFLINE_INGAME
	if(strstr(param, OFFLINE_TAG)) net_status = 0;
#endif
	u8 autoplay = (webman_config->autoplay) || force_autoplay;

	if(autoplay)
	{
		u8 timeout = 0;
		while(IS_INGAME)
		{
			sys_ppu_thread_sleep(1);
			if(++timeout > 15) break; // wait until 15 seconds to return to XMB
		}
	}

	if(IS_ON_XMB)
	{
		// check if holding L2 to cancel auto-play
 #ifdef REMOVE_SYSCALLS
		bool l2 = is_pressed(CELL_PAD_CTRL_L2);
 #else
		bool l2 = is_pressed(CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2);
 #endif

		if(autoplay && wait_for_abort(webman_config->boots)) return;

		if(!get_explore_interface()) return;

		bool mount_ps3 = (strstr(param, "_ps3") != NULL);

		if(!l2 && is_BIN_ENC(param))
		{
			if(mount_ps3 && XMB_GROUPS && webman_config->ps2l && file_exists(PS2_CLASSIC_ISO_PATH))
			{
				if(explore_exec_push(250, true))					// move to ps2_launcher folder and open it
					if(autoplay && !explore_exec_push(500, false))	// start ps2_launcher
						autoplay = false;
			}
		}
		else
 #ifdef COBRA_ONLY
		if(!l2 && (strstr(param, "/PSPISO") || strstr(param, ".ntfs[PSPISO]")))
		{
			if(mount_ps3 && XMB_GROUPS && webman_config->pspl && (isDir(PSP_LAUNCHER_MINIS) || isDir(PSP_LAUNCHER_REMASTERS)))
			{
				if(explore_exec_push(250, true))					// move to psp_launcher folder and open it
					if(autoplay && !explore_exec_push(500, false))	// start psp_launcher
						autoplay = false;
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
					exec_xmb_command("close_all_list");
					exec_xmb_command2("focus_segment_index %s", "seg_package_files");
				}
				else
				{
					exec_xmb_command("focus_index rx_video");

					// open rx_video folder
					if(!explore_exec_push(200, true) || !autoplay || strcasestr(param, ".mkv")) {is_busy = false; return;}

					explore_exec_push(2, true); // open Data Disc
				}
			}
		}
		else
 #endif
		{
			bool atag = strcasestr(param, AUTOPLAY_TAG) || (autoplay);

			if(!(webman_config->combo2 & PLAY_DISC) || atag) {sys_ppu_thread_sleep(1); launch_disc((atag && !l2) || (!atag && l2));}		// L2 + X

			autoplay = false;
		}

		if(strstr(param, "BDISO") || strstr(param, "DVDISO"))
		{
			char path[20];
			const char *folder[8] = {"VIDEO", "MOVIES", "MUSIC", "PICTURE", "PHOTOS", "COMICS", "EBOOK", "MAGAZINE"};
			for(u8 i = 0; i < 8; i++)
			{
				sprintf(path, "/dev_bdvd/%s", folder[i]);
				if(isDir(path))
				{
					const char *category =  (i < 2) ? "video" :
											(i < 3) ? "music" : "photo";
					exec_xmb_command2("focus_category %s", category);
					if(!wait_for_abort(3))
						exec_xmb_command("focus_segment_index seg_data_device");
					break;
				}
			}
		}

		if(autoplay)
		{
			explore_exec_push(2, false);
		}
	}
}

static bool game_mount(char *buffer, char *templn, char *param, char *tempstr, bool mount_ps3, bool forced_mount)
{
	bool mounted = false, umounted = false, check_alias = true;

	// ---------------------
	// unmount current game
	// ---------------------
	const char *uparam = param + 7;
	const char *params = param + MOUNT_CMD;

	if(islike(param, "/mount") && (islike(uparam, "ps3/unmount") || islike(params, "/dev_bdvd") || islike(params, "/app_home")))
	{
		if(mount_ps3 || forced_mount || IS_ON_XMB)
		{
			do_umount(true);
			if(mount_ps3) {mount_app_home = false; return false;}
			umounted = true;
		}
		check_alias = false;
	}

	// -----------------
	// unmount ps2_disc
	// -----------------
	#ifdef PS2_DISC
	else if(islike(param, "/mount") && islike(uparam, "ps2/unmount"))
	{
		do_umount_ps2disc(false);

		if(mount_ps3) {mount_app_home = false; return false;}
		umounted = true;
		check_alias = false;
	}
	#endif

	if(umounted)
	{
		strcat(buffer, STR_GAMEUM);
		sprintf(templn, HTML_REDIRECT_TO_URL, "/cpursx.ps3", HTML_REDIRECT_WAIT);
		strcat(buffer, templn);
	}


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
		char target[STD_PATH_LEN]; *target = '\0';
		if(islike(param, "/copy.ps3"))
		{
			char *pos = strchr(param, '|'); plen = 1;
			if(!pos) {pos = strstr(param, "&to="); plen = 4;}
			if( pos) {strcpy(target, pos + plen); *pos = '\0';}
			plen = COPY_CMD;
		}

		bool is_copy = ((plen == COPY_CMD) && (copy_in_progress == false));
		char *wildcard = NULL;

		if(islike(param + plen, "/net")) ; else {wildcard = strchr(param, '*'); if(wildcard) *wildcard++ = NULL;}

		if(is_copy)
		{
			if(islike(param,  "/dev_blind") || islike(param,  "/dev_hdd1")) mount_device(param,  NULL, NULL); // auto-mount source device
			if(islike(target, "/dev_blind") || islike(target, "/dev_hdd1")) mount_device(target, NULL, NULL); // auto-mount destination device
		}
		#endif

		char enc_dir_name[STD_PATH_LEN * 3], *source = param + plen;
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

		get_flag(source, "?random=");
		get_flag(source, "?/sman.ps3");

		// ----------------------------------------
		// Try relative path if source is not found
		// ----------------------------------------
		if(check_alias) check_path_alias(source);

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
			if(isDir(source)) get_flag(param, "/PS3_GAME"); // remove /PS3_GAME

			int discboot = 0xff;
			xsettings()->GetSystemDiscBootFirstEnabled(&discboot);

			if(discboot == 1)
				xsettings()->SetSystemDiscBootFirstEnabled(0);

			{
				// prevent auto focus before execute PKGLAUNCH
				const char *ext = get_ext(source);
				if(mount_ps3 && (!strstr(source, "/ROMS")) && strcasestr(ARCHIVE_EXTENSIONS, ext))
					param[6] = '.';
			}

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
				if(islike(_game_TitleID, "PKGLAUNCH"))
				{
					mounted = mount_game(source, MOUNT_NORMAL);
				}
				else
					sprintf(tempstr, "<H3>%s : <a href=\"/mount.ps3/unmount\">%s %s</a></H3><hr><a href=\"/mount_ps3%s\">", STR_UNMOUNTGAME, _game_TitleID, _game_Title, templn); strcat(buffer, tempstr);
			}
			else
			{
				mounted = mount_game(source, MOUNT_NORMAL);
			}

			if(discboot == 1)
				xsettings()->SetSystemDiscBootFirstEnabled(1);
		}

		mount_app_home = false;

		// -------------------
		// exit mount from XMB
		// -------------------
		if(mount_ps3)
		{
			if(mounted && islike(source, "/net") && strstr(source, "/ROMS")) launch_app_home_icon(webman_config->autoplay);

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
			char *filename = get_filename(_path), *icon = tempstr;
			{
				char title_id[TITLEID_LEN], *d_name; *icon = *title_id = NULL;
				u8  f0 = strstr(filename, ".ntfs[") ? NTFS : 0, is_dir = isDir(source),
					f1 = strstr(_path, "PS2") ? id_PS2ISO :
						 strstr(_path, "PSX") ? id_PSXISO :
						 strstr(_path, "PSP") ? id_PSPISO : id_PS3ISO;

				check_cover_folders(templn);

				// get iso name
				*filename = NULL; // sets _path
				d_name = filename + 1;

				char *buf = malloc(_4KB_);
				if(buf)
				{
					if(is_dir)
					{
						sprintf(templn, "%s/%s/PS3_GAME/PARAM.SFO", _path, d_name); check_ps3_game(templn);
						get_title_and_id_from_sfo(templn, title_id, d_name, icon, buf, 0); f1 = id_GAMES;
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
				bool is_copying_from_hdd = islike(source, drives[0]);

				usb = get_default_usb_drive(0);

				#ifdef USE_UACCOUNT
				if(!webman_config->uaccount[0])
				#endif
					sprintf(webman_config->uaccount, "%08i", xusers()->GetCurrentUserNumber());

				if(cp_mode)
				{
					dont_copy_same_size = false; // force copy
					normalize_path(source, false);
					sprintf(target, "%s%s", source, get_filename(cp_path));
					strcpy(source, cp_path);
				}
				else
				if(*target) {if(!isDir(source) && isDir(target)) strcat(target, filename);} // &to=<destination>
				else
				{
					const char *ext = get_ext(source);
					#ifdef SWAP_KERNEL
					if(strstr(source, "/lv2_kernel"))
					{
						swap_kernel(source, target, tempstr);
						plen = 0; //do not copy if don't reboot
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
					else if(IS(ext, ".pkg"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/Packages", drives[usb]);
						else
							sprintf(target, "/dev_hdd0/packages");

						strcat(target, filename);
					}
					else if(_IS(ext, ".bmp") || _IS(ext, ".gif"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/PICTURE", drives[usb]);
						else
							sprintf(target, "%s/PICTURE", drives[0]);

						strcat(target, filename);
					}
					else if(_IS(ext, ".jpg") || _IS(ext, ".png"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/PICTURE", drives[usb]);
						else if(strstr(source, "BL") || strstr(param, "BC") || strstr(source, "NP"))
							sprintf(target, "/dev_hdd0/GAMES/covers");
						else
							sprintf(target, "%s/PICTURE", drives[0]);

						strcat(target, filename);
					}
					else if(strcasestr(source, "/covers"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/COVERS", drives[usb]);
						else
							sprintf(target, "/dev_hdd0/GAMES/covers");
					}
					else if(_IS(ext, ".mp4") || _IS(ext, ".mkv") || _IS(ext, ".avi"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/VIDEO", drives[usb]);
						else
							sprintf(target, "/dev_hdd0/VIDEO");

						strcat(target, filename);
					}
					else if(_IS(ext, ".mp3"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/MUSIC", drives[usb]);
						else
							sprintf(target, "%s/MUSIC", drives[0]);

						strcat(target, filename);
					}
					else if(IS(ext, ".p3t"))
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
							sprintf(target, "%s/%s/exdata", HDD0_HOME_DIR, webman_config->uaccount);

						strcat(target, filename);
					}
					else if(IS(ext, ".rco") || strstr(source, "/coldboot"))
					{
						enable_dev_blind(NO_MSG);
						sprintf(target, "/dev_blind/vsh/resource");

						if(IS(ext, ".raf"))
							strcat(target, "/coldboot.raf");
						else
							strcat(target, filename);
					}
					else if(IS(ext, ".qrc"))
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
							sprintf(target, "%s/%s/exdata", HDD0_HOME_DIR, webman_config->uaccount);
					}
					else if(strstr(source, "/PS3/THEME"))
						sprintf(target, "/dev_hdd0/theme");
					else if(strcasestr(source, "/savedata/"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/PS3/SAVEDATA", drives[usb]);
						else
							sprintf(target, "%s/%s/savedata", HDD0_HOME_DIR, webman_config->uaccount);

						strcat(target, filename);
					}
					else if(strcasestr(source, "/trophy/"))
					{
						if(is_copying_from_hdd)
							sprintf(target, "%s/PS3/TROPHY", drives[usb]);
						else
							sprintf(target, "%s/%s/trophy", HDD0_HOME_DIR, webman_config->uaccount);

						strcat(target, filename);
					}
					else if(strstr(source, "/webftp_server"))
					{
						sprintf(target, "%s/webftp_server.sprx", "/dev_hdd0/plugins");
						if(not_exists(target)) sprintf(target + 31, "_ps3mapi.sprx");
						if(not_exists(target)) sprintf(target + 10, "webftp_server.sprx");
						if(not_exists(target)) sprintf(target + 23, "_ps3mapi.sprx");
					}
					else if(strstr(source, "/boot_plugins"))
					{
						sprintf(target, "/dev_hdd0/boot_plugins.txt");
						if(cobra_version == 0)
							sprintf(target + 22, "_nocobra.txt");
					}
					else if(is_copying_from_hdd)
						sprintf(target, "%s%s", drives[usb], source + 9);
					else if(islike(source, "/dev_usb"))
						sprintf(target, "%s%s", drives[0], source + 11);
					else if(islike(source, "/net"))
						sprintf(target, "%s%s", drives[0], source + 5);
					else
					{
						if(islike(source, "/dev_bdvd"))
						{
							sysLv2FsBdDecrypt(); // decrypt dev_bdvd files

							int cnt = 0;
							do {
								sprintf(target, "%s/%s %i", "/dev_hdd0/GAMES", "My Disc Backup", ++cnt);
							} while (isDir(target));

							char title[128];
							sprintf(title, "/dev_bdvd/PS3_GAME/PARAM.SFO"); check_ps3_game(title);
							if(file_exists(title))
							{
								char title_id[TITLEID_LEN];
								getTitleID(title, title_id, GET_TITLE_AND_ID);

								filepath_check(title); // remove "<|>:*?
								replace_char(title, '/', ' '); // replace /
								for(u8 c = 1; c < 31; c++)
									replace_char(title, c, ' ');

								if(*title_id && (title_id[8] >= '0'))
								{
									if(*title == '\0')
										strcpy(target + 16, title_id);
									else if(strstr(title, " ["))
										strcpy(target + 16, title);
									else
										sprintf(target + 16, "%s [%s]", title, title_id);
								}
							}
						}
						else
							strcpy(target, drives[0]);

						const char *p = strchr(source + 1, '/');
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
				add_breadcrumb_trail2(buffer, STR_COPYING, source); concat(buffer, "<hr>");

				// show image
				urlenc(_path, target);
				sprintf(tempstr, "<a href=\"%s\"><img src=\"%s\" border=0></a><hr>%s %s: ",
								 _path, enc_dir_name, is_error ? STR_ERROR : "", STR_CPYDEST); strcat(buffer, tempstr);

				// show target path
				use_open_path = true; add_breadcrumb_trail(buffer, target); *tempstr = NULL;

				if(strstr(target, "/webftp_server")) {sprintf(tempstr, "<HR>%s", STR_SETTINGSUPD);} else
				if(cp_mode) {dont_copy_same_size = true; char *p = get_filename(_path); *p = NULL; sprintf(tempstr, HTML_REDIRECT_TO_URL, _path, HTML_REDIRECT_WAIT);}

				if(g_sysmem) {sys_memory_free(g_sysmem); g_sysmem = NULL;}

				if(is_error) {show_msg_with_icon(ICON_EXCLAMATION, STR_CPYABORT); cp_mode = CP_MODE_NONE; return false;}
			}
			else
			#endif // #ifdef COPY_PS3

			// ------------------
			// show mounted game
			// ------------------
			{
				size_t mlen;
				bool is_movie = strstr(param, "/BDISO") || strstr(param, "/DVDISO") || !extcmp(param, ".ntfs[BDISO]", 12) || !extcmp(param, ".ntfs[DVDISO]", 13);

				#ifndef ENGLISH_ONLY
				char buf[296];
				char *STR_GAMETOM  = buf; //[48];//	= "Game to mount";
				char *STR_MOVIETOM = buf; //[48];//	= "Movie to mount";
				if(is_movie)	language("STR_MOVIETOM", STR_MOVIETOM, "Movie to mount");
				else			language("STR_GAMETOM",  STR_GAMETOM,  "Game to mount");
				close_language();
				#endif

				syscalls_removed = CFW_SYSCALLS_REMOVED(TOC);
				if(syscalls_removed)
				{
					sprintf(tempstr, "<h1>%s</h1>%s", STR_CFWSYSALRD, HTML_RED_SEPARATOR); concat(buffer, tempstr);
				}
				#ifdef COBRA_ONLY
				else if(!cobra_version)
				{
					sprintf(tempstr, "<h1>%s %s</h1>%s", "Cobra", STR_DISABLED, HTML_RED_SEPARATOR); concat(buffer, tempstr);
				}
				#endif

				strcat(buffer, "<div id=\"mount\">");
				add_breadcrumb_trail2(buffer, is_movie ? STR_MOVIETOM : STR_GAMETOM, source);

				//if(strstr(param, "PSX")) {sprintf(tempstr, " <font size=2>[CD %i • %s]</font>", CD_SECTOR_SIZE_2352, (webman_config->ps1emu) ? "ps1_netemu.self" : "ps1_emu.self"); strcat(buffer, tempstr);}

				if(is_movie)
				{
					#ifndef ENGLISH_ONLY
					char *STR_MOVIELOADED = buf; //[272];//= "Movie loaded successfully. Start the movie from the disc icon<br>under the Video column.</a><hr>Click <a href=\"/mount.ps3/unmount\">here</a> to unmount the movie.";
					sprintf(STR_MOVIELOADED, "Movie %s%s%smovie.",
											 "loaded successfully. Start the ", "movie from the disc icon<br>under the Video column", ".</a><hr>Click <a href=\"/mount.ps3/unmount\">here</a> to unmount the ");
					language("STR_MOVIELOADED", STR_MOVIELOADED, STR_MOVIELOADED);
					#endif

					mlen = sprintf(tempstr, "<hr><a href=\"/play.ps3\"><img src=\"%s\" onerror=\"this.src='%s';\" border=0></a>"
											"<hr><a href=\"/dev_bdvd\">%s</a>", enc_dir_name, wm_icons[strstr(param,"BDISO") ? iBDVD : iDVD], mounted ? STR_MOVIELOADED : STR_ERROR);
				}
				else if(is_BIN_ENC(param) || islike(param, "/mount.ps2") || islike(param, "/mount_ps2"))
				{
					#ifndef ENGLISH_ONLY
					char *STR_PS2LOADED = buf; //[240]; //	= "Game loaded successfully. Start the game using <b>PS2 Classic Launcher</b>.<hr>";
					sprintf(STR_PS2LOADED,   "Game %s%s%s</b>.<hr>",
											 "loaded successfully. Start the ", "game using <b>", "PS2 Classic Launcher");
					language("STR_PS2LOADED", STR_PS2LOADED, STR_PS2LOADED);

					if(!is_BIN_ENC(param))
					{
						char *pos = strchr(STR_PS2LOADED, '.'); if(pos) sprintf(pos, ": " HTML_URL, "/dev_ps2disc", "/dev_ps2disc");
					}
					#endif

					mlen = sprintf(tempstr, "<hr><img src=\"%s\" onerror=\"this.src='%s';\" height=%i>"
											"<hr>%s", enc_dir_name, wm_icons[iPS2], 300, mounted ? STR_PS2LOADED : STR_ERROR);
				}
				else if((strstr(param, "/PSPISO") || strstr(param, "/ISO/")) && is_ext(param, ".iso"))
				{
					#ifndef ENGLISH_ONLY
					char *STR_PSPLOADED = buf; //[232]; //	= "Game loaded successfully. Start the game using <b>PSP Launcher</b>.<hr>";
					sprintf(STR_PSPLOADED,   "Game %s%s%s</b>.<hr>",
											 "loaded successfully. Start the ", "game using <b>", "PSP Launcher");
					language("STR_PSPLOADED", STR_PSPLOADED, STR_PSPLOADED);
					#endif

					mlen = sprintf(tempstr, "<hr><img src=\"%s\" onerror=\"this.src='%s';\" height=%i>"
											"<hr>%s", enc_dir_name, wm_icons[iPSP], strcasestr(enc_dir_name,".png") ? 200 : 300, mounted ? STR_PSPLOADED : STR_ERROR);
				}
				else
				{
					#ifndef ENGLISH_ONLY
					char *STR_GAMELOADED = buf; //[288];//	= "Game loaded successfully. Start the game from the disc icon<br>or from <b>/app_home</b>&nbsp;XMB entry.</a><hr>Click <a href=\"/mount.ps3/unmount\">here</a> to unmount the game.";
					sprintf(STR_GAMELOADED,  "Game %s%s%s",
											 "loaded successfully. Start the ", "game from the disc icon<br>or from <b>/app_home</b>&nbsp;XMB entry", ".</a><hr>Click <a href=\"/mount.ps3/unmount\">here</a> to unmount the ");
					language("STR_GAMELOADED", STR_GAMELOADED, STR_GAMELOADED);
					#endif
					mlen = sprintf(tempstr, "<hr><a href=\"/play.ps3\"><img src=\"%s\" onerror=\"this.src='%s';\" border=0></a>"
											"<hr><a href=\"/dev_bdvd\">%s</a>", (!mounted && IS_INGAME) ? XMB_DISC_ICON : enc_dir_name, wm_icons[default_icon], mounted ? STR_GAMELOADED : STR_ERROR);
				}

				if(!mounted)
				{
					if(!c_firmware || syscalls_removed)
						mlen += sprintf(tempstr + mlen, " %s", STR_CFWSYSALRD);
					else
					#ifndef ENGLISH_ONLY
					if(webman_config->lang == 0 || webman_config->lang == LANG_CUSTOM)
					#endif
					{
						if(is_mounting)
							mlen += sprintf(tempstr + mlen, " A previous mount is in progress.");
						else if(IS_INGAME)
							mlen += sprintf(tempstr + mlen, " Click to quit the game.");
						else if(!cobra_version)
							mlen += sprintf(tempstr + mlen, " Cobra payload not available.");
					}
					#ifdef COBRA_NON_LITE
					if(islike(params, "/net") && !is_netsrv_enabled(param[4 + MOUNT_CMD])) mlen += sprintf(tempstr + mlen, " /net%c %s", param[4 + MOUNT_CMD], STR_DISABLED);
					#endif
					if(!forced_mount && IS_INGAME)
					{
						sprintf(tempstr + mlen, " <a href=\"/mount_ps3%s\">/mount_ps3%s</a>", params, params);
					}
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

				strcpy(target, source);
				if(strstr(target, "Sing"))
				{
					char *pos = strstr(target, "/PS3ISO");
					if(pos)
						{strcpy(pos, "/PS2DISC"); is_iso = 1;}
					else if(strstr(target, ".ntfs[PS3ISO]"))
						{strcpy(target, "/dev_hdd0/PS2DISC"); is_iso = 1;}
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

						if(is_iso || strstr(entry_name, "[PS2"))
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

			// auto-focus Data Disc
			if(mounted && (strstr(source, "PSX") || strstr(source, "DVDISO") || strstr(source, "BDISO"))) auto_play(param, false);
		}

		// -------------
		// perform copy
		// -------------
		#ifdef COPY_PS3
		if(sys_admin && is_copy)
		{
			if(islike(target, source))
				{sprintf(templn, "<hr>%s %s %s", STR_ERROR, STR_CPYDEST, source); strcat(buffer, templn);}
			else if((!islike(source, "/net")) && not_exists(source))
				{sprintf(templn, "<hr>%s %s %s", STR_ERROR, source, STR_NOTFOUND); strcat(buffer, templn);}
			else
			{
				setPluginActive();

				// show msg begin
				if(!silent_mode)
				{
					sprintf(templn, "%s %s\n%s %s", STR_COPYING, source, STR_CPYDEST, target);
					show_msg(templn);
				}

				if(islike(target, "/dev_blind")) enable_dev_blind(NO_MSG);

				check_path_tags(target);

				if(isDir(source)) normalize_path(target, true);

				// make target dir tree
				bool fast_move = (cp_mode == CP_MODE_MOVE) && is_same_dev(source, target);
				mkdir_tree(target);

				bool recursive = true;
				if(IS(source, INT_HDD_ROOT_PATH)) recursive = (strchr(source + 10, '/') != NULL); // prevent recursive copy from root of HDD

				// copy folder to target
				if(strstr(source, "/exdata"))
				{
					scan(source, false, ".edat", islike(source, "/dev_usb") ? SCAN_COPYBK : SCAN_COPY, target);
				}
				else if(wildcard)
					scan(source, recursive, wildcard, SCAN_COPY, target); 
				else if(fast_move)
					{rename_file(source, target); cp_mode = CP_MODE_COPY;}
				else if(isDir(source))
					folder_copy(source, target);
				else
					file_copy(source, target);

				// show msg end
				if(copy_aborted)
					show_msg_with_icon(ICON_EXCLAMATION, STR_CPYABORT);
				else
				{
					if(!silent_mode)
					{
						show_msg_with_icon(ICON_CHECK, STR_CPYFINISH);
						if(!webman_config->nobeep && copied_size > _1GB_)
							play_sound_id(5);
					}
					if(do_restart) { del_turnoff(2); vsh_reboot();}
				}

				setPluginInactive();
			}

			if(!copy_aborted && (cp_mode == CP_MODE_MOVE) && file_exists(target)) del(source, true);
			if(cp_mode) {cp_mode = CP_MODE_NONE, *cp_path = NULL;}
		}
		#endif //#ifdef COPY_PS3
	}

	is_busy = false;
	return mounted;
}

#ifdef COBRA_ONLY

static void unmap_app_home(void)
{
	// force remove "/app_home", "/app_home/PS3_GAME"
	for(u8 retry = 0; retry < 3; retry++)
	{
		sys_map_path("/app_home", NULL);
		sys_map_path(APP_HOME_DIR, NULL);
	}
}

static u8 gm = 00;	// remembers last multi-game folder.
					// If PS3_GM01 exists, it's mapped to app_home and gm is incremented.
					// if the same game is mounted again PS3_GM02 is mapped and gm is incremented again
					// if PS3_GMxx doesn't exists, PS3_GAME is mapped and gm resets to 01.

void map_app_home(const char *path)
{
	unmap_app_home();

	// remap app_home for multi-game discs
	char *mpath = (char *)malloc(strlen(path) + sizeof("/PS3_GAME") + 1);
	if(mpath)
	{
		sprintf(mpath, "%s/PS3_GM%02i", path, gm);
		if(not_exists(mpath))
		{
			gm = 01;
			sys_map_path("/app_home", path);
		}
		else
		{
			sys_map_path(APP_HOME_DIR, mpath); gm++;
		}

		free(mpath);
	}
}

static void set_app_home(const char *game_path)
{
	apply_remaps();

	unmap_app_home();

	if(!game_path) // default action on plugin startup or game unmount
	{
		if(webman_config->homeb && is_app_dir(webman_config->home_url, ".")) // map /app_home/PS3_GAME to home_url, if available
			sys_map_path(APP_HOME_DIR, webman_config->home_url);
		else // otherwise, map /app_home to the packages dir
			sys_map_path("/app_home", isDir("/dev_hdd0/packages") ?
											"/dev_hdd0/packages" : NULL); // Enable install all packages on HDD when game is unmounted
	}
	else
	{
		if(IS(game_path, PKGLAUNCH_DIR))
			sys_map_path(APP_HOME_DIR, PKGLAUNCH_PS3_GAME);
		else if(is_app_dir(game_path, ".")) // map /app_home/PS3_GAME to game_path if it points to a game in /dev_hdd0/game/
			sys_map_path(APP_HOME_DIR, game_path);
		else
			sys_map_path("/app_home", game_path);
	}
}

static void do_umount_iso(bool clean)
{
	unsigned int real_disctype, effective_disctype, iso_disctype;
	cobra_get_disc_type(&real_disctype, &effective_disctype, &iso_disctype);

	// If there is an effective disc in the system, it must be ejected
	if(effective_disctype != DISC_TYPE_NONE)
	{
		cobra_send_fake_disc_eject_event();
		wait_path("/dev_bdvd", 1, false);
	}

	if(iso_disctype != DISC_TYPE_NONE)
		cobra_umount_disc_image();

	// If there is a real disc in the system, issue an insert event
	if(real_disctype != DISC_TYPE_NONE)
	{
		cobra_send_fake_disc_insert_event();
		wait_for("/dev_bdvd", 2);
		cobra_disc_auth();
	}

	char filename[MAX_PATH_LEN];
	if(clean && read_file(DEL_CACHED_ISO, filename, MAX_PATH_LEN, 0))
	{
		cellFsUnlink(DEL_CACHED_ISO);
		cellFsUnlink(filename);
	}

	#ifdef VISUALIZERS
	randomize_vsh_resources(false, filename);
	#endif
}
#endif // #ifdef COBRA_ONLY

static void do_umount(bool clean)
{
	if(clean)
	{
		cellFsUnlink(LAST_GAME_TXT); mount_unk = EMU_OFF;

		for(u8 i = id_GAMES; i < id_ISO; i++)
			unlink_file(INT_HDD_ROOT_PATH, paths[i], "/~tmp.iso");

		#ifdef COPY_PS3
		if(webman_config->deliso)
			del(WM_EXTRACT_PATH, RECURSIVE_DELETE);
		#endif
	}

	#ifdef USE_NTFS
	root_check = true;
	#endif

	check_multipsx = NONE;

	cellFsUnlink("/dev_hdd0/tmp/game/ICON0.PNG"); // remove XMB disc icon

	ps2_classic_mounted = false;
	if(fan_ps2_mode) reset_fan_mode(); // restore normal fan mode

	#ifdef COBRA_ONLY
	{
		{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

		cobra_unset_psp_umd(); // eject PSPISO

		#ifndef LITE_EDITION
		swap_file(PSP_EMU_PATH, "psp_emulator.self", "psp_emulator.self.dec_edat", "psp_emulator.self.original"); // restore original psp_emulator.self
		#endif
		do_umount_iso(clean);	// unmount iso
		#ifdef PS2_DISC
		do_umount_ps2disc(false); // unmount ps2disc
		#endif
		sys_ppu_thread_usleep(20000);

		cobra_unload_vsh_plugin(0); // unload external rawseciso / netiso plugins

		cellFsChmod("/dev_bdvd/PS3_GAME/SND0.AT3", MODE); // restore SND0 permissions of game mounted (JB folder)

		apply_remaps();

		sys_map_path("/dev_bdvd/PS3/UPDATE", NULL); // unmap UPDATE from bdvd

		// map PKGLAUNCH cores folder to RETROARCH
		sys_map_path(PKGLAUNCH_DIR, NULL);
		sys_map_path("/dev_bdvd/PS3_GAME/USRDIR/cores", NULL);
		sys_map_path("/app_home/PS3_GAME/USRDIR/cores", NULL);
		sys_map_path("/dev_bdvd/PS3_GAME", NULL);
		sys_map_path("/app_home/PS3_GAME", NULL);

		// unmap bdvd & apphome
		sys_map_path("/dev_bdvd", NULL);
		sys_map_path("//dev_bdvd", NULL);

		set_app_home(NULL); // unmap app_home

		// unmap GAMEI & PKGLAUNCH
		#ifdef MOUNT_GAMEI
		if(*map_title_id)
		{
			char gamei_mapping[32];
			sprintf(gamei_mapping, "%s%s", HDD0_GAME_DIR, map_title_id);
			sys_map_path(gamei_mapping, NULL);
			sys_map_path(PKGLAUNCH_DIR, NULL);
			*map_title_id = NULL;
		}
		#endif

		// unload internal netiso or rawiso plugin
		{
		#if defined(USE_INTERNAL_NET_PLUGIN) || defined(USE_INTERNAL_NTFS_PLUGIN)
			sys_ppu_thread_t t_id;
		#endif
		#ifdef USE_INTERNAL_NET_PLUGIN
			sys_ppu_thread_create(&t_id, netiso_stop_thread, NULL, THREAD_PRIO_STOP, THREAD_STACK_SIZE_STOP_THREAD, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
			thread_join(t_id);
		#endif
		#ifdef USE_INTERNAL_NTFS_PLUGIN
			sys_ppu_thread_create(&t_id, rawseciso_stop_thread, NULL, THREAD_PRIO_STOP, THREAD_STACK_SIZE_STOP_THREAD, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
			thread_join(t_id);
		#endif

			// wait for unload of netiso or rawiso plugin
		#if defined(USE_INTERNAL_NET_PLUGIN) && defined(USE_INTERNAL_NTFS_PLUGIN)
			while(netiso_loaded || rawseciso_loaded) {sys_ppu_thread_usleep(100000); if(is_mounting) break;}
		#elif defined(USE_INTERNAL_NET_PLUGIN)
			while(netiso_loaded) {sys_ppu_thread_usleep(100000); if(is_mounting) break;}
		#elif defined(USE_INTERNAL_NTFS_PLUGIN)
			while(rawseciso_loaded) {sys_ppu_thread_usleep(100000); if(is_mounting) break;}
		#endif
		}

		// send fake eject
		sys_ppu_thread_usleep(4000);
		cobra_send_fake_disc_eject_event();
		sys_ppu_thread_usleep(4000);

		disable_progress();

		{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }
	}
	#else
	{
		pokeq(0x8000000000000000ULL + MAP_ADDR, 0x0000000000000000ULL);
		pokeq(0x8000000000000008ULL + MAP_ADDR, 0x0000000000000000ULL);

		//eject_insert(1, 1);

		if(isDir("/dev_flash/pkg"))
			mount_game("/dev_flash/pkg", MOUNT_SILENT);
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
		cellFsMkdir(target, DMODE);

		strcat(target, get_filename(source)); // add file name

		char filename[MAX_PATH_LEN];
		if(read_file(DEL_CACHED_ISO, filename, MAX_PATH_LEN, 0) && !IS(target, filename))
		{
			cellFsUnlink(DEL_CACHED_ISO);
			cellFsUnlink(filename);
		}

		if((copy_in_progress | fix_in_progress) == false && not_exists(target))
		{
			sprintf(msg, "%s %s\n"
						 "%s %s", STR_COPYING, source, STR_CPYDEST, basepath);

			show_msg(msg);

			show_progress(msg, OV_COPY);

			dont_copy_same_size = true;
			file_copy(source, target);

			if(copy_aborted)
			{
				cellFsUnlink(target);
				show_msg_with_icon(ICON_EXCLAMATION, STR_CPYABORT);
			}
		}

		if(file_exists(target))
		{
			strcpy(source, target);
			
			if(webman_config->deliso)
			{
				save_file(DEL_CACHED_ISO, target, SAVE_ALL);
			}
		}
	}

	do_umount(false); // umount + delete previous cached file
}

static void cache_icon0_and_param_sfo(char *destpath)
{
	wait_for("/dev_bdvd", 15);

	char *ext = destpath + strlen(destpath);
	strcpy(ext, ".SFO"); // append .SFO
	dont_copy_same_size = false;

	// cache PARAM.SFO
	if(not_exists(destpath))
	{
		for(u8 retry = 0; retry < 10; retry++)
		{
			if(file_copy("/dev_bdvd/PS3_GAME/PARAM.SFO", destpath) >= CELL_FS_SUCCEEDED) break;
			if(file_copy("/dev_bdvd/PS3_GM01/PARAM.SFO", destpath) >= CELL_FS_SUCCEEDED) break;
			sys_ppu_thread_usleep(500000);
		}
	}

	// cache ICON0.PNG
	strcpy(ext, ".PNG");
	if((webman_config->nocov!=2) && not_exists(destpath))
	{
		for(u8 retry = 0; retry < 10; retry++)
		{
			if(file_copy("/dev_bdvd/PS3_GAME/ICON0.PNG", destpath) >= CELL_FS_SUCCEEDED) break;
			if(file_copy("/dev_bdvd/PS3_GM01/ICON0.PNG", destpath) >= CELL_FS_SUCCEEDED) break;
			sys_ppu_thread_usleep(500000);
		}
	}

	dont_copy_same_size = true;
}
#endif // #ifdef COBRA_ONLY

#ifdef COBRA_ONLY
static bool mount_ps_disc_image(char *_path, char *cobra_iso_list[], u8 iso_parts, int emu_type)
{
	bool ret = false;

	const char *ext = get_ext(_path);
	bool mount_iso = false;
	s32 cue_offset = 0;

	if(_IS(ext, ".cue") || _IS(ext, ".ccd"))
	{
		change_cue2iso(cobra_iso_list[0]);
	}
	#ifdef MOUNT_PNG
	else if(_IS(ext, ".png"))
	{
		cue_offset = 0xE000UL;
	}
	#endif
	else if(*ext == '.')
	{
		change_ext(_path, 4, cue_ext);
		if(not_exists(_path)) strcpy(_path, cobra_iso_list[0]);
	}

	mount_iso |= file_exists(cobra_iso_list[0]); ret = mount_iso; mount_unk = emu_type;

	// patch PS2 demo disc if title_id is ***D_###.##
	if(emu_type != EMU_PSX)
	{
		if(mount_iso) patch_ps2_demo(cobra_iso_list[0]);

		// set fan to PS2 mode (constant fan speed)
		if(webman_config->fanc) restore_fan(SET_PS2_MODE); //set_fan_speed( ((webman_config->ps2temp*255)/100), 0);
	}

	if(_IS(ext, ".cue") || _IS(ext, ".ccd") || (cue_offset == 0xE000UL))
	{
		sys_addr_t sysmem = sys_mem_allocate(_64KB_);
		if(sysmem)
		{
			char *cue_buf = (char*)sysmem;
			int cue_size = read_file(_path, cue_buf, _8KB_, cue_offset);

			#ifdef MOUNT_PNG
			if(cue_offset) cue_size = strlen(cue_buf);
			#endif

			if(cue_size > 16)
			{
				TrackDef tracks[MAX_TRACKS];
				unsigned int num_tracks = parse_cue(_path, cue_buf, cue_size, tracks);

				if(emu_type == EMU_PSX)
					cobra_mount_psx_disc_image(cobra_iso_list[0], tracks, num_tracks);
				else
					cobra_mount_ps2_disc_image(cobra_iso_list, iso_parts, tracks, num_tracks);

				mount_iso = false;
			}
			sys_memory_free(sysmem);
		}
	}

	if(mount_iso)
	{
		TrackDef tracks[1];
		tracks[0].lba = 0;
		tracks[0].is_audio = 0;

		if(emu_type == EMU_PSX)
			cobra_mount_psx_disc_image(cobra_iso_list[0], tracks, 1);
		else
			cobra_mount_ps2_disc_image(cobra_iso_list, iso_parts, tracks, 1);
	}

	return ret;
}

static void mount_on_insert_usb(bool on_xmb, char *msg)
{
	// Auto-mount x:\AUTOMOUNT.ISO or JB game found on root of USB devices (dev_usb00x, dev_sd, dev_ms, dev_cf)
	if(is_mounting) return;

	if(on_xmb)
	{
		if(webman_config->poll) ;

		else if(!isDir("/dev_bdvd"))
		{
			if(fan_ps2_mode && !ps2_classic_mounted) enable_fan_control(PS2_MODE_OFF); 

			if(webman_config->autob)
				for(u8 f0 = 1; f0 < 16; f0++)
				{
					if(IS_NET || IS_NTFS) continue;

					if(!check_drive(f0))
					{
						if(automount != f0)
						{
							char *game_path = msg;
							sprintf(game_path, "%s/AUTOMOUNT.ISO", drives[f0]);
							if(file_exists(game_path)) {mount_game(game_path, MOUNT_SILENT); automount = f0; break;}
							else
							{
								sprintf(game_path, "%s/PS3_GAME/PARAM.SFO", drives[f0]); check_ps3_game(game_path);
								if(file_exists(game_path)) {mount_game(game_path, MOUNT_SILENT); automount = f0; break;}
							}
						}
						else if(!isDir(drives[f0])) automount = 0;
					}
				}
			else
			{
				automount = 0;
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
			if((effective_disctype == DISC_TYPE_PS2_DVD) || (effective_disctype == DISC_TYPE_PS2_CD)
			|| (real_disctype      == DISC_TYPE_PS2_DVD) || (real_disctype      == DISC_TYPE_PS2_CD)
			|| (iso_disctype       == DISC_TYPE_PS2_DVD) || (iso_disctype       == DISC_TYPE_PS2_CD))
			{
				if(webman_config->fanc) restore_fan(SET_PS2_MODE); //set_fan_speed( ((webman_config->ps2temp*255)/100), 0);

				fan_ps2_mode = true;

				// create "wm_noscan" to avoid re-scan of XML returning to XMB from PS2
				create_file(WM_NOSCAN_FILE);
			}
			else if(fan_ps2_mode) enable_fan_control(PS2_MODE_OFF);

			automount = 99;
		}
	}
	else if(fan_ps2_mode && IS_INGAME)
	{
		automount = 0; enable_fan_control(PS2_MODE_OFF);
	}
	else if((check_multipsx >= 0) && IS_INGAME)
	{
		if(isDir("/dev_usb000") == check_multipsx)
		{
			check_multipsx = NONE;
			show_msg(STR_GAMEUM); play_rco_sound("snd_trophy");

			wait_for("/dev_usb000", 5); // wait for user reinsert the USB device

			mount_game("_next", MOUNT_NEXT_CD);
		}
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

	// prevent auto-mount on startup if L2+R2 is pressed
	if(is_pressed(CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2)) { if(!webman_config->nobeep) BEEP2; return;}

	if(from_reboot && *path && strstr(path, "/PS2")) return; //avoid re-launch PS2 returning to XMB

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
		if(webman_config->delay) {sys_ppu_thread_sleep(3); wait_for(path, 2 * (webman_config->boots + webman_config->bootd));}
		#ifndef COBRA_ONLY
		if(islike(path, "/net") || strstr(path, ".ntfs[")) return;
		#endif
		#ifdef COBRA_NON_LITE
		if(islike(path, "/net") && !is_netsrv_enabled(path[4])) return;
		#endif

		if(is_last_game)
		{
			// prevent auto-launch game on boot (last game only). AUTOBOOT.ISO is allowed to auto-launch on boot
			int discboot = 0xff;
			xsettings()->GetSystemDiscBootFirstEnabled(&discboot);
			if(discboot) xsettings()->SetSystemDiscBootFirstEnabled(0); // disable Disc Boot
			if(!islike(path, "/dev_hdd0/game")) mount_game(path, MOUNT_NORMAL);
			sys_ppu_thread_sleep(5);
			if(discboot) xsettings()->SetSystemDiscBootFirstEnabled(1); // restore Disc Boot setting
		}
		else
		{
			if(mount_game(path, MOUNT_NORMAL)) auto_play(path, false);
		}
	}
}

/***********************************************/
/* mount_thread parameters                     */
/***********************************************/
static char _path0[MAX_PATH_LEN];
static bool mount_ret = false;
/***********************************************/

static void mount_thread(u64 action)
{
	bool ret = false;
	bool multiCD = false; // mount_lastgames.h

	automount = 0;

	// --------------------------------------------
	// show message if syscalls are fully disabled
	// --------------------------------------------
	#ifdef COBRA_ONLY
	if(syscalls_removed && peekq(TOC) != SYSCALLS_UNAVAILABLE) {syscalls_removed = false; disable_signin_dialog();}

	if(syscalls_removed || peekq(TOC) == SYSCALLS_UNAVAILABLE)
	{
		syscalls_removed = true;
		{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

		int ret_val = NONE; { system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PCHECK_SYSCALL8); ret_val = (int)p1;}

		if(ret_val < 0) { show_msg_with_icon(ICON_EXCLAMATION, STR_CFWSYSALRD); { PS3MAPI_DISABLE_ACCESS_SYSCALL8 } goto finish; }
		if(ret_val > 1) { system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, 1); }
	}
	else if(!cobra_version)
		{ show_error("[nonCobra]"); goto finish; }
	#else

	if(syscalls_removed || peekq(TOC) == SYSCALLS_UNAVAILABLE) { show_msg_with_icon(ICON_EXCLAMATION, STR_CFWSYSALRD); goto finish; }

	#endif

	// -----------------
	// fix mount errors
	// -----------------

	ps2_classic_mounted = false;
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

	#ifdef PKG_HANDLER
	if(is_ext(_path0, ".pkg") && file_exists(_path0))
	{
		mount_ret = (installPKG(_path0, _path) == CELL_OK);
		is_mounting = false;
		sys_ppu_thread_exit(0);
	}
	#endif

	ret = true;

	mount_unk = EMU_OFF;

	led(GREEN, BLINK_FAST);

	// ----------------
	// open url & exit
	// ----------------
	if(islike(_path0 + 1, "http") || islike(_path0, "http") || is_ext(_path0, ".htm"))
	{
		char *url = strstr(_path0, "http");

		if(!url) {url = _path; sprintf(url, "http://%s%s", local_ip, _path0);}

		if(IS_ON_XMB)
		{
			wait_for_xmb(); // wait for explore_plugin

			do_umount(false);
			open_browser(url, 0);
		}
		else
			ret = false;

		goto finish;
	}
	#ifdef COPY_PS3
	else if(is_ext(_path0, ".txt") || is_ext(_path0, ".bat"))
	{
		parse_script(_path0);
		ret = true;
		goto finish;
	}
	#endif

	// -----------------
	// remove /PS3_GAME
	// -----------------

	normalize_path(_path0, false);

	strcpy(_path, _path0);

	if(*_path == '/')
	{
		if(isDir(_path)) get_flag(_path, "/PS3_GAME"); // remove PS3_GAME
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
	cellFsUnlink(WM_NOSCAN_FILE); // remove wm_noscan if a PS2ISO has been mounted

	if(action == EXPLORE_CLOSE_ALL) {action = MOUNT_NORMAL; explore_close_all(_path);}

	// check that path exists
	if(islike(_path, "/dev_") && not_exists(_path))
	{
		if(action) show_error(_path);
		do_umount(false); ret = false;
		patch_gameboot(0); // None
		goto finish;
	}

	if(action && !(webman_config->minfo & 1)) show_msg_with_icon(ICON_MOUNT, _path);

	///////////////////////
	// MOUNT ISO OR PATH //
	///////////////////////
	set_mount_type(_path);

	const char *ps = strstr(_path, "/ROMS"); // check is not like /ROMS*/PS*
	if(!(ps && (ps[5] && ps[6] && ps[7] && (islike(ps + 5, "/PS") || islike(ps + 7, "/PS")))))
	{
		#include "mount_misc.h" // mount GAMEI, NPDRM, ROMS, PS2 Classic
	}
	#include "mount_cobra.h"
	#include "mount_noncobra.h"

exit_mount:

	// ---------------
	// delete history
	// ---------------

	delete_history(false);

	// -------------------------------
	// show 2nd message: "xxx" loaded
	// -------------------------------

	if(action == MOUNT_SILENT) goto mounting_done;

	if(webman_config->minfo & 2) ;

	else if(ret && *_path == '/')
	{
		char msg[STD_PATH_LEN + 48], *pos;

		// get file name (without path)
		pos = get_filename(_path);
		sprintf(msg, "\"%s", pos + 1);

		// remove file extension
		remove_ext(msg);
		if(msg[1] == NULL) sprintf(msg, "\"%s", _path);

		// show loaded path
		strcat(msg, "\" "); strcat(msg, STR_LOADED2);

		if(mount_unk == EMU_PSP)
			show_msg_with_icon(ICON_PSP_UMD, "Use PSP Launcher to play the game");
		else if((mount_unk == EMU_PS2_CD) || (mount_unk == EMU_PS2_DVD))
		{
			if(is_BIN_ENC(msg))
				show_msg_with_icon(ICON_PS2_DISC, "Use PS2 Launcher to play the game");
			else
				show_msg_with_icon(ICON_PS2_DISC, msg);
		}
		else if((mount_unk >= EMU_PS3) && (mount_unk < EMU_PSP))
			show_msg_with_icon(40 + mount_unk, msg);
		else
			show_msg(msg);
	}

	if(mount_unk >= EMU_MAX) goto mounting_done;

	// -------------------------------------------
	// wait few seconds until the bdvd is mounted
	// -------------------------------------------

	if(ret && !is_BIN_ENC(_path))
	{
		wait_for("/dev_bdvd", (islike(_path, drives[0]) ? 6 : netid ? 20 : 15));
		if(!isDir("/dev_bdvd")) ret = false;
	}

	// -------------------------------------------------------
	// re-check PARAM.SFO to notify if game needs to be fixed
	// -------------------------------------------------------

	if(ret)
	{
		char filename[64];
		sprintf(filename, "/dev_bdvd/PS3_GAME/PARAM.SFO");
		if(file_exists(filename))
		{
			getTitleID(filename, title_id, GET_TITLE_ID_ONLY);

			// check for PARAM.SFO in hdd0/game folder
			sprintf(filename, "%s%s%s", HDD0_GAME_DIR, title_id, "/PARAM.SFO");

			if(not_exists(filename))
			{
				#ifdef COBRA_ONLY
				if(strcasestr(_path0, "[PSN]"))
				{
					// Map /dev_hdd0/game/[GAME_ID] to /dev_bdvd/PS3_GAME (if the folder does not exist)
					if(strcasestr(_path0, ".iso") || strstr(_path0, ".ntfs[PS3ISO]"))
					{
						strcpy(map_title_id, title_id);
						sprintf(filename, "%s%s", HDD0_GAME_DIR, map_title_id);
						sys_map_path(filename, "/dev_bdvd/PS3_GAME");
					}
				}
				#endif
				sprintf(filename, "/dev_bdvd/PS3_GAME/PARAM.SFO");
			}

			#ifdef FIX_GAME
			if(c_firmware < LATEST_CFW)
				getTitleID(filename, title_id, SHOW_WARNING);
			#endif
		}
	}

	// -----------------------------------
	// show error if bdvd was not mounted
	// -----------------------------------

	if(!ret && !isDir("/dev_bdvd")) show_error(_path);

	// -------------------------------------------------------------------------------------
	// remove syscalls hodling R2 (or prevent remove syscall if path contains [online] tag)
	// -------------------------------------------------------------------------------------

	#ifdef REMOVE_SYSCALLS
	else if(mount_unk == EMU_PS3)
	{
		// check if holding R2 to disable syscalls on PS3
		bool r2 = is_pressed(CELL_PAD_CTRL_R2);
		bool otag = (strcasestr(_path, ONLINE_TAG) != NULL);
		if((!r2 && otag) || (r2 && !otag)) disable_cfw_syscalls(webman_config->keep_ccapi);
	}
	#endif

mounting_done:

#ifdef COBRA_ONLY

	if((mount_unk == EMU_PS3) && (mount_app_home | !(webman_config->app_home)) && _path0 && strcasestr(_path0, ".iso"))
		map_app_home("/dev_bdvd");

	// ------------------------------------------------------------------
	// copy .ncl to /dev_hdd0/tmp/art.txt
	// ------------------------------------------------------------------
	#ifdef ARTEMIS_PRX
	if(webman_config->artemis)
		init_codelist(_path);
	#endif

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

		if(ret)
		{
			if(file_exists("/dev_bdvd/PS3UPDAT.PUP"))
			{
				sys_map_path("/dev_bdvd/PS3/UPDATE", "/dev_bdvd"); //redirect root of bdvd to /dev_bdvd/PS3/UPDATE (allows update from mounted /net folder or fake BDFILE)
			}

			if((!netid) && isDir("/dev_bdvd/PKG"))
			{
				sys_map_path("/app_home", "/dev_bdvd/PKG"); //redirect net_host/PKG to app_home
			}

			// mounted NPDRM folder
			if(file_exists("/dev_bdvd/USRDIR/EBOOT.BIN") && isDir(_path0))
			{
				if(!(islike(_path0, HDD0_GAME_DIR) || islike(_path0, _HDD0_GAME_DIR)))
				{
					set_app_home(_path0);

					sprintf(_path, "%s/PARAM.SFO", "/dev_bdvd");
					getTitleID(_path, map_title_id, GET_TITLE_ID_ONLY);

					sprintf(_path, "%s%s", HDD0_GAME_DIR, map_title_id);
					sys_map_path(_path, _path0);

					sys_ppu_thread_sleep(1);
					launch_app_home_icon(webman_config->autoplay | force_ap);
				}
			}
		}

		{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }
	}
#endif // #ifdef COBRA_ONLY

	// --------------
	// exit function
	// --------------

finish:
	disable_progress();

	#ifdef COBRA_NON_LITE
	if(!ret)
		patch_gameboot(0); // None
	#endif // #ifdef COBRA_ONLY

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
{ /*
 #ifdef COBRA_NON_LITE
	if(islike(path, "/net") && !is_netsrv_enabled(path[4])) return false;
 #endif
*/
	if(is_mounting) return false; is_mounting = true;

	strcpy(_path0, path);

	sys_ppu_thread_t t_id;
	sys_ppu_thread_create(&t_id, mount_thread, (u64)action, THREAD_PRIO_HIGH, THREAD_STACK_SIZE_MOUNT_GAME, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_CMD);
	thread_join(t_id);

	while(is_mounting && working) sys_ppu_thread_sleep(1); // wait until thread mount game

	close_ftp_sessions_idle();

	_memset(_path0, sizeof(_path0));

	return mount_ret;
}
