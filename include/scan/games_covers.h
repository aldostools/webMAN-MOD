#define TITLEID_LEN  10

static char wm_icons[14][60] = {WM_ICONS_PATH "/icon_wm_album_ps3.png", //024.png  [0]
								WM_ICONS_PATH "/icon_wm_album_psx.png", //026.png  [1]
								WM_ICONS_PATH "/icon_wm_album_ps2.png", //025.png  [2]
								WM_ICONS_PATH "/icon_wm_album_psp.png", //022.png  [3]
								WM_ICONS_PATH "/icon_wm_album_dvd.png", //023.png  [4]

								WM_ICONS_PATH "/icon_wm_ps3.png",       //024.png  [5]
								WM_ICONS_PATH "/icon_wm_psx.png",       //026.png  [6]
								WM_ICONS_PATH "/icon_wm_ps2.png",       //025.png  [7]
								WM_ICONS_PATH "/icon_wm_psp.png",       //022.png  [8]
								WM_ICONS_PATH "/icon_wm_dvd.png",       //023.png  [9]

								WM_ICONS_PATH "/icon_wm_settings.png",  //icon/icon_home.png  [10]
								WM_ICONS_PATH "/icon_wm_eject.png",     //icon/icon_home.png  [11]

								WM_ICONS_PATH "/icon_wm_bdv.png",       //024.png  [12]
								WM_ICONS_PATH "/icon_wm_retro.png",     //023.png  [13]
								};

enum nocov_options
{
	SHOW_MMCOVERS = 0,
	SHOW_ICON0    = 1,
	SHOW_DISC     = 2,
	ONLINE_COVERS = 3,
};

enum icon_type
{
	iPS3  = 5,
	iPSX  = 6,
	iPS2  = 7,
	iPSP  = 8,
	iDVD  = 9,
	iBDVD = 12,
	iROM  = 13,
};

#define HAS_TITLE_ID		BETWEEN('A', *title_id, 'Z')
#define IS_PSX_TITLE_ID		(*title_id == 'S')
#define IS_PSP_TITLE_ID		((*title_id == 'U') || (*title_id == 'N' && strchr("FGHJXZ", title_id[3])))

#define NO_ICON       (!*icon)

#define FROM_MOUNT  -99

#define SHOW_COVERS_OR_ICON0  (webman_config->nocov != SHOW_DISC)
#define SHOW_COVERS          ((webman_config->nocov == SHOW_MMCOVERS) || (webman_config->nocov == ONLINE_COVERS))

static const char *ext[4] = {".JPG", ".PNG", ".jpg", ".png"};

static const char *cpath[6] = {MM_ROOT_STD, MM_ROOT_STL, MM_ROOT_SSTL, MANAGUNZ, "/dev_hdd0/GAMES", "/dev_hdd0/GAMEZ"};

#ifndef ENGLISH_ONLY
static bool use_custom_icon_path = false, use_icon_region = false;
static bool is_online_server = false;
#endif

static bool covers_exist[9];
static bool covers_retro_exist[4];
static bool wm_icons_exists = false;

static bool HAS(char *icon)
{
	return ((*icon == 'h') || ((*icon == '/') && file_exists(icon) && LCASE(icon[strlen(icon) - 1]) == 'g' ));
}

static void check_cover_folders(char *folder_path)
{
	if(!folder_path) return; // sanity check

	#ifndef ENGLISH_ONLY
		covers_exist[0] = isDir(COVERS_PATH); // online url or custom path
	#endif
	for(u8 p = 0; p < 3; p++)
	{
		if(!isDir(cpath[p]) && isDir(drives[p]))
			cpath[p] = drives[p]; // replace with /dev_usb000 & /dev_usb001 if doesn't exist MM_ROOT_STL or MM_ROOT_SSTL

		concat2(folder_path, cpath[p], "/covers_retro/psx");
		covers_retro_exist[p] = isDir(folder_path) && !is_empty_dir(folder_path);  // MM_ROOT_STD, MM_ROOT_STL, MM_ROOT_SSTL
	}

	#ifdef COBRA_ONLY
	concat2(folder_path, MM_ROOT_STD, "/covers_retro/psp");
	covers_retro_exist[3] = isDir(folder_path) && !is_empty_dir(folder_path);
	#endif

	for(u8 p = 0; p < 5; p++)
	{
		concat2(folder_path, cpath[p], "/covers");
		covers_exist[p + 1] = isDir(folder_path) && !is_empty_dir(folder_path);  // MM_ROOT_STD, MM_ROOT_STL, MM_ROOT_SSTL, MANAGUNZ, "/dev_hdd0/GAMES", "/dev_hdd0/GAMEZ"
	}
		covers_exist[6] = !is_empty_dir(WMTMP_COVERS);
	//	covers_exist[8] = isDir(WMTMP) && SHOW_COVERS_OR_ICON0; // WMTMP

	#ifndef ENGLISH_ONLY
	if(!covers_exist[0]) {use_custom_icon_path = strstr(COVERS_PATH, "%s"); use_icon_region = strstr(COVERS_PATH, "%s/%s");} else {use_icon_region = use_custom_icon_path = false;}

	// disable custom icon from web repository if network is disabled //
	if(use_custom_icon_path && islike(COVERS_PATH, "http"))
	{
		char ip[ip_size] = "";
		netctl_main_9A528B81(ip_size, ip);
		if(*ip == NULL) use_custom_icon_path = false;

		is_online_server = islike(COVERS_PATH, LAUNCHPAD_COVER_SVR);
	}
	#endif

	wm_icons_exists = file_exists(WM_ICONS_PATH "/icon_wm_ps3.png");

	#ifdef MOUNT_ROMS
	covers_exist[7] = file_exists(WM_ICONS_PATH "/icon_wm_album_emu.png");
	#endif

	#ifdef LAUNCHPAD
	nocover_exists = file_exists(WM_ICONS_PATH "/icon_lp_nocover.png");
	#endif
}

static u8 ex[4] = {0, 1, 2, 3};

static void swap_ex(u8 e)
{
	if(e)
	{
		u8 s  = ex[e];
		ex[e] = ex[0];
		ex[0] = s;
	}
}

static bool _get_image_file(char *icon, int flen, bool ucase)
{
	if(!icon || (flen <= 0) || (*icon != '/')) return false; // sanity check

	int max = ucase ? 2 : 4;

	for(u8 e = 0; e < max; e++)
	{
		strcpy(icon + flen, ucase ? ext[e] : ext[ex[e]]);

		if(file_exists(icon)) {swap_ex(e); return true;}
	}
	return false;
}

static bool get_image_file(char *icon, int flen)
{
	return _get_image_file(icon, flen, false);
}

static bool get_image_file2(char *icon, int flen)
{
	return _get_image_file(icon, flen, true);
}

static size_t get_name(char *name, const char *filename, u8 cache)
{
	// name:
	//   returns file name without extension & without title id (cache == 0 -> file name keeps path, cache == 2 -> remove path first)
	//   returns file name with WMTMP path                      (cache == 1 -> remove path first)

	if(!name || !filename) return 0; // sanity check

	int flen, pos = 0;
	if(cache) {pos = strlen(filename); while((pos > 0) && filename[pos - 1] != '/') pos--;}
	if(cache == NO_PATH) cache = 0;

	if(cache == WM_COVERS)
		flen = concat_path(name, WMTMP_COVERS, filename + pos);
	else if(cache)
	{
		#ifdef USE_NTFS
		if(is_ntfs_path(filename) && (pos > 21))
		{
			// convert /dev_ntfs0:/PS3ISO/folder/filename.iso
			//      to /dev_hdd0/tmp/wmtmp/[folder] filename.iso
			int p1 = strchr(filename + 12, '/') - filename + 1;
			int p2 = strchr(filename + p1, '/') - filename;
			sprintf(name, "%s/[%s", WMTMP, filename + p1);
			flen = (p2 - p1) + 21;
			flen += sprintf(name + flen, "] %s", filename + pos);
		}
		else
		#endif
			flen = concat_path(name, WMTMP, filename + pos);
	}
	else
	{
		// remove prepend [PSPISO] or [PS2ISO]
		if((filename[pos] == '[') && (filename[pos + 7] == ']'))
		{
			const char *prefix = filename + pos + 1;
			if(islike(prefix, paths[id_PSPISO])) pos += 9;
			if(islike(prefix, paths[id_PS2ISO])) pos += 9;
		}

		flen = strcopy(name, filename + pos);
	}

	flen = remove_ext(name);
	if(cache != GET_WMTMP)
	{
		if(strstr(filename + pos, ".ntfs["))
		{
			const char *ext = get_ext(name);
			if(ext && (*ext == '.') && strcasestr(ISO_EXTENSIONS, ext))
				flen = remove_ext(name);
		}
	}

	if(cache || (flen <= 12)) return (size_t) flen;

	// remove title id from file name
	if(name[4] == '_' && name[8] == '.' && (*name == 'B' || *name == 'N' || *name == 'S' || *name == 'U') && ISDIGIT(name[9]) && ISDIGIT(name[10])) {flen = strcopy(name, &name[12]);}// SLES_000.00-Name
	if(name[9] == '-' && name[10]== '[') {flen = strcopy(name, name + 11) - 1; name[flen] = '\0';} // BLES00000-[Name]
	if(name[10]== '-' && name[11]== '[') {flen = strcopy(name, name + 12) - 1; name[flen] = '\0';} // BLES-00000-[Name]

	if(!webman_config->tid) // Name [BLES-00000]
	{
		char *p = strstr(name, " [");
		if(p && (p[2] == 'B' || p[2] == 'N' || p[2] == 'S' || p[2] == 'U') && ISDIGIT(p[7])) *p = NULL;
		flen = strlen(name);
	}

	return (size_t) flen;
}

static bool get_cover_by_titleid(char *icon, const char *title_id)
{
	if(!icon || !title_id) return false; // sanity check

	if(!HAS_TITLE_ID) return false;

	int flen;

	if(SHOW_COVERS)
	{
		#ifndef ENGLISH_ONLY
		// Search covers in custom path
		if(covers_exist[0] && ((webman_config->nocov == SHOW_MMCOVERS) && (*COVERS_PATH == '/')))
		{
			flen = concat_path(icon, COVERS_PATH, title_id);
			if(get_image_file2(icon, flen)) return true;
		}
		#endif

		// Search retro covers in MM_ROOT_STD, MM_ROOT_STL, MM_ROOT_SSTL
		if(IS_PSX_TITLE_ID)
		{
			for(u8 p = 0; p < 3; p++)
			{
				if(covers_retro_exist[p])
				{
					flen = sprintf(icon, "%s/covers_retro/psx/%.4s_%.3s.%.2s_COV", cpath[p],
									title_id,
									title_id + 4, title_id + 7);

					if(get_image_file2(icon, flen)) return true;
				}
			}
		}

		#ifdef COBRA_ONLY
		if(IS_PSP_TITLE_ID) // PSP
		{
			if(covers_retro_exist[3])
			{
				flen = sprintf(icon, "%s/covers_retro/psp/%s", MM_ROOT_STD, title_id);
				if(get_image_file2(icon, flen)) return true;
			}
		}
		#endif

		// Search covers in MM_ROOT_STD, MM_ROOT_STL, MM_ROOT_SSTL, "/dev_hdd0/GAMES", "/dev_hdd0/GAMEZ"
		for(u8 p = 0; p < 6; p++)
			if(covers_exist[p + 1])
			{
				flen = sprintf(icon, "%s/covers/%s", cpath[p], title_id);
				if(get_image_file2(icon, flen)) return true;
			}

		// Search covers in WMTMP_COVERS
		if(covers_exist[6])
		{
			flen = concat_path(icon, WMTMP_COVERS, title_id);
			if(get_image_file2(icon, flen)) return true;
		}
/*
		// Search covers in WMTMP
		if(covers_exist[8])
		{
			flen = concat_path(icon, WMTMP, title_id);
			if(get_image_file2(icon, flen)) return true;
		}
*/
		// Search online covers
		#ifdef ENGLISH_ONLY
		if(webman_config->nocov == ONLINE_COVERS)
		{
			if(IS_PSX_TITLE_ID) // PS1/PS2
				sprintf(icon, "%s/PSX/%.4s_%.3s.%.2s_COV.JPG", "http://raw.githubusercontent.com/aldostools/resources/master", title_id, title_id + 4, title_id + 7);
			else if(IS_PSP_TITLE_ID) // PSP
				sprintf(icon, "%s/PSP/%s.PNG", "http://raw.githubusercontent.com/aldostools/resources/master", title_id);
			else
				sprintf(icon, COVERS_PATH, title_id); // PS3
			return true;
		}
		#else
		if(use_custom_icon_path && (webman_config->nocov == ONLINE_COVERS) && (COVERS_PATH[0] == 'h'))
		{
			if(is_online_server && (*title_id != 'B' && *title_id != 'N' && *title_id != 'S')) {*icon = NULL; return false;} // PS3

			if(IS_PSX_TITLE_ID) // PS1/PS2
				sprintf(icon, "%s/PSX/%.4s_%.3s.%.2s_COV.JPG", "http://raw.githubusercontent.com/aldostools/resources/master", title_id, title_id + 4, title_id + 7);
			else if(IS_PSP_TITLE_ID) // PSP
				sprintf(icon, "%s/PSP/%s.PNG", "http://raw.githubusercontent.com/aldostools/resources/master", title_id);
			else if(use_icon_region) sprintf(icon, COVERS_PATH, (title_id[2] == 'U') ? "US" :
																(title_id[2] == 'J') ? "JA" : "EN", title_id);
			else
				sprintf(icon, COVERS_PATH, title_id);
			return true;
		}
		#endif
	}

	*icon = NULL;
	return false;
}

static bool get_cover_from_name(char *icon, const char *name, char *title_id) // get icon & title_id from name
{
	if(!icon || !name || !title_id) return false; // sanity check

	if(HAS(icon)) return true;

	// get title_id from file name
	if(HAS_TITLE_ID)
	{
		// get cover from title_id in PARAM.SFO
		if(get_cover_by_titleid(icon, title_id)) return true;
	}
	else if((*name == 'B' || *name == 'N' || *name == 'S' || *name == 'U') && ISDIGIT(name[6]) && ISDIGIT(name[7]))
	{
		if(name[4] == '_' && name[8] == '.')
			sprintf(title_id, "%.4s%.3s%.2s", name, name + 5, name + 9); //SCUS_999.99.filename.iso
		else if(ISDIGIT(name[8]))
			strncpy(title_id, name, TITLEID_LEN);
	}

	if(HAS_TITLE_ID) ;

	else if(islike(name + TITLEID_LEN, "-["))
		strncpy(title_id, name, TITLEID_LEN); // TITLEID-[NAME]
	else
	{
		const char *pos;
		// search for BLES/UCES/SLES/BCUS/UCUS/SLUS/etc.
		for(pos = (char*)name; *pos; pos++)
			if((*pos == '[' || *pos == '(') && (pos[2] == 'L' || pos[2] == 'C') && ISDIGIT(pos[6]) && ISDIGIT(pos[7]))
			{
				if(pos[1] == 'B' || pos[1] == 'U') // B = PS3, U = PSP
				{
					strncpy(title_id, pos + 1, TITLEID_LEN); //BCES/BLES/BCUS/BLUS/BLJM/etc. || UCES/UCUS/ULES/ULUS/UCAS/UCKS/UCUS/ULJM/ULJS/etc.
					break;
				}
				else
				if(pos[1] == 'S') // S = PS1/PS2
				{
					get_ps_titleid_from_path(title_id, pos + 1); //SLES/SLUS/SLPM/SLPS/SLAJ/SLKA/SCES/SCUS/SCPS/SCAJ/SCKA
					break;
				}
			}

		if(*title_id == 0)
		{
			if((pos = strstr(name, "[NP")))
				strncpy(title_id, pos + 1, TITLEID_LEN); //NP*
			else if((pos = strstr(name, "[KTGS")))
				strncpy(title_id, pos + 1, TITLEID_LEN); //KTGS*
			else if((pos = strstr(name, "[KOEI")))
				strncpy(title_id, pos + 1, TITLEID_LEN); //KOEI*
			else if((pos = strstr(name, "[MRTC")))
				strncpy(title_id, pos + 1, TITLEID_LEN); //MRTC*
		}
	}

	if(title_id[4] == '-') strncpy(&title_id[4], &title_id[5], 5); title_id[TITLE_ID_LEN] = '\0';

	// get cover using title_id obtained from file name
	if(get_cover_by_titleid(icon, title_id)) return true;

	return false;
}

static void get_default_icon_from_folder(char *icon, u8 is_dir, const char *param, const char *entry_name, char *title_id, u8 f0)
{
	//this function is called only from get_default_icon()

	if(SHOW_COVERS_OR_ICON0)
	{
			if(is_dir && (webman_config->nocov == SHOW_ICON0))
			{
				concat_path2(icon, param, entry_name, "/PS3_GAME/ICON0.PNG"); check_ps3_game(icon);
				if(!HAS(icon))
					concat_path2(icon, param, entry_name, "/ICON0.PNG");
				return;
			}

			// get path/name and remove file extension
			concat_path(icon, param, entry_name);

			int flen = remove_ext(icon);
			if((webman_config->nocov == SHOW_ICON0) && get_image_file(icon, flen)) return;

			if(HAS_TITLE_ID && SHOW_COVERS) {get_cover_by_titleid(icon, title_id); if(HAS(icon)) return;}

			// get mm covers & titleID
			get_cover_from_name(icon, entry_name, title_id);

			// get covers named as titleID from iso folder e.g. /PS3ISO/BLES12345.JPG
			if(!is_dir && HAS_TITLE_ID && (f0 < NET || f0 > NTFS))
			{
				if(HAS(icon)) return;

				char titleid[STD_PATH_LEN];
				char *pos = strchr(entry_name, '/');
				if(pos)
					{*pos = NULL; concat_path(titleid, entry_name, title_id); *pos = '/';}
				else
					strcpy(titleid, title_id);

				flen = concat_path(icon, param, titleid);
				if(get_image_file2(icon, flen)) return;

				*icon = NULL;
			}

			// return ICON0
			if(is_dir)
			{
				concat_path2(icon, param, entry_name, "/PS3_GAME/ICON0.PNG"); check_ps3_game(icon);
				if(!HAS(icon))
					concat_path2(icon, param, entry_name, "/ICON0.PNG");
				return;
			}

			// continue searching for covers
			if(SHOW_COVERS) return;

			// Search covers in WMTMP_COVERS
			if(covers_exist[6])
			{
				flen = get_name(icon, entry_name, WM_COVERS);
				if(get_image_file(icon, flen)) return;
			}

			// Search covers in WMTMP
			flen = get_name(icon, entry_name, GET_WMTMP);
			if(get_image_file(icon, flen)) return;

			*icon = NULL;
	}
}

static void get_default_icon_for_iso(char *icon, const char *param, const char *file, int isdir, int ns)
{
	//this function is called only from get_default_icon()

	int flen;

	if(is_BIN_ENC(file))
	{
		flen = concat_path(icon, param, file);

		if(get_image_file(icon, flen)) return;

		flen -= 8; // remove .BIN.ENC
		if(flen > 0 && icon[flen] == '.')
		{
			if(get_image_file(icon, flen)) return;
		}
	}

	flen = concat_path(icon, param, file);

	if(not_exists(icon))
	{
		flen = get_name(icon, file, GET_WMTMP); //wmtmp
	}

	if(!isdir)
	{
		flen = remove_ext(icon);

		//file name + ext
		if(get_image_file(icon, flen)) return;
	}

	//copy remote file
	if(not_exists(icon))
	{
		#ifdef NET_SUPPORT
		if(ns < 0) {*icon = NULL; return;}

		char remote_file[MAX_PATH_LEN];

		if(isdir)
		{
			if(webman_config->nocov == SHOW_DISC) return; // no icon0
			concat_path2(remote_file, param, file, "/PS3_GAME/ICON0.PNG");
			flen = get_name(icon, file, GET_WMTMP); sprintf(icon + flen, ".png");

			copy_net_file(icon, remote_file, ns);
			if(file_exists(icon)) return;
		}
		else
		{
			// preserve title id: Name [BLES-00000].iso -> /net0/PS3ISO/Name [BLES-00000].iso
			concat_path(remote_file, param, file);
			// remove extension: /net0/PS3ISO/Name [BLES-00000].iso -> /net0/PS3ISO/Name [BLES-00000]
			int tlen = remove_ext(remote_file);

			int icon_len = get_name(icon, file, GET_WMTMP); //wmtmp

			for(u8 e = 0; e < 4; e++)
			{
				strcpy(icon + icon_len, ext[ex[e]]);
				if(file_exists(icon)) return;

				strcpy(remote_file + tlen, ext[ex[e]]);

				//Copy remote icon locally
				copy_net_file(icon, remote_file, ns);
				if(file_exists(icon)) return;
			}
		}
		#endif //#ifdef NET_SUPPORT

		*icon = NULL;
	}
}

static enum icon_type get_default_icon_by_type(u8 f1)
{
	return  IS_PS3_TYPE    ? iPS3 :
			IS_PSXISO      ? iPSX :
			IS_PS2ISO      ? iPS2 :
			IS_PSPISO      ? iPSP :
#ifdef MOUNT_ROMS
			IS_ROMS_FOLDER ? iROM :
#endif
			IS_DVDISO      ? iDVD : iBDVD;
}

#ifdef MOUNT_ROMS
static const char *last_roms_path = NULL;

static bool get_covers_retro(char *icon, const char *roms_path, const char *filename)
{
	if(HAS(icon)) return true; if(last_roms_path == roms_path) {*icon = NULL; return false;}

	int nlen = sprintf(icon, "%s/covers_retro/%s", MM_ROOT_STD, roms_path); if(isDir(icon) == false) {*icon = NULL; last_roms_path = roms_path; return false;}
	
	nlen += sprintf(icon + nlen, "/%s", filename);

	for(; nlen > 48; --nlen)
	{
		if(icon[nlen] == '.')
		{
			if (get_image_file(icon, nlen)) return true;
			break;
		}
	}
	*icon = NULL; return false;
}
#endif

static enum icon_type get_default_icon(char *icon, const char *param, char *file, int is_dir, char *title_id, int ns, u8 f0, u8 f1)
{
	enum icon_type default_icon = get_default_icon_by_type(f1);

	if(!icon || !param || !file || !title_id) return default_icon; // sanity check

	char filename[STD_PATH_LEN];

	if(ns == FROM_MOUNT)
		strncopy(filename, STD_PATH_LEN, file);
	else
		*filename = '\0';

	if(webman_config->nocov == SHOW_DISC) {if(get_cover_from_name(icon, file, title_id)) return default_icon; goto no_icon0;}

	if(SHOW_COVERS && get_cover_from_name(icon, file, title_id)) return default_icon; // show mm cover

	// get icon from folder && copy remote icon
	get_default_icon_for_iso(icon, param, file, is_dir, ns);

	if(HAS(icon)) return default_icon;

	if(!IS_NET) get_default_icon_from_folder(icon, is_dir, param, file, title_id, f0);

	// continue using cover or default icon0.png
	if(HAS(icon)) return default_icon;

	//use the cached PNG from wmtmp if available
	int flen = get_name(icon, file, GET_WMTMP);

	if(!is_dir && get_image_file(icon, flen)) return default_icon;

no_icon0:
	if(HAS(icon)) return default_icon;

		if((webman_config->nocov == SHOW_ICON0) && get_cover_from_name(icon, file, title_id)) return default_icon; // show mm cover as last option (if it's disabled)

	//show the default icon by type
	{
		default_icon = iPS3;

		flen = concat_path(icon, param + (IS_NET ? 0 : 6), file);

		char *ps = strstr(icon, "PS");
		if(ps)
		{
				 if(strstr(ps, "PSX")) //if(strstr(param, "/PSX") || !extcmp(file, ".ntfs[PSXISO]", 13))
				default_icon = iPSX;
			else if(strstr(ps, "PS2")) //if(strstr(param, "/PS2ISO") || is_BIN_ENC(param) || !extcmp(file, ".ntfs[PS2ISO]", 13))
				default_icon = iPS2;
			else if(strstr(ps, "PSP")) //if(strstr(param, "/PSPISO") || strstr(param, "/ISO/") || !extcmp(file, ".ntfs[PSPISO]", 13))
				default_icon = iPSP;
		}
		else if(strstr(param, "/ROMS")) //if(strstr(param, "/ROMS"))
			default_icon = iROM;
		else if(strstr(param, "/GAME")) //if(strstr(param, "/GAMES") || strstr(param, "/GAMEZ") || strstr(param, "/GAMEI"))
			default_icon = iPS3;
		else if(strstr(icon, "DVD")) //if(strstr(param, "/DVDISO") || !extcmp(file, ".ntfs[DVDISO]", 13))
			default_icon = iDVD;
		else if(strstr(icon, "BDISO")) //if(strstr(param, "/BDISO") || !extcmp(file, ".ntfs[BDISO]", 13))
			default_icon = iBDVD;
	}

	if(!HAS(icon))
	{
		flen = sprintf(icon, "/dev_bdvd/%s", file);
		if(file_exists(icon))
		{
			if(get_image_file(icon, flen)) return default_icon;
			flen -= 4; icon[flen] = '\0';
			if(get_image_file(icon, flen)) return default_icon;
			sprintf(icon, "/dev_bdvd/%s.ICON0.PNG", file);
			if(file_exists(icon)) return default_icon;
		}
		strcpy(icon, wm_icons[default_icon]);
	}
	return default_icon;
}

static int get_title_and_id_from_sfo(char *param_sfo, char *title_id, const char *entry_name, char *icon, char *data, u8 f0)
{
	int ret = FAILED;

	if(!param_sfo || !title_id || !data) return ret; // sanity check

	bool use_filename = webman_config->use_filename;

	// read param.sfo
	unsigned char *mem = (u8*)data;
	u16 sfo_size = read_sfo(param_sfo, data);
	char *title = param_sfo;

	// get title_id & title from PARAM.SFO
	if(sfo_size && is_sfo(mem))
	{
		if((IS_HDD0 && islike(param_sfo + 9, "/game/")) || islike(param_sfo + 11, "/GAMEI/") || strstr(param_sfo, "_00-")) use_filename = false;

		parse_param_sfo(mem, title_id, title, sfo_size);

		if(SHOW_COVERS) get_cover_by_titleid(icon, title_id);

		ret = CELL_FS_SUCCEEDED;
	}

	if(!entry_name || !icon) return ret; // sanity check

	if(use_filename)
	{
		if(NO_ICON && !HAS_TITLE_ID) get_cover_from_name(icon, entry_name, title_id); // get title_id from name

		ret = ~CELL_FS_SUCCEEDED;
	}

	if(ret != CELL_FS_SUCCEEDED)
	{
		get_name(title, entry_name, NO_PATH); if(!IS_NTFS) utf8enc(data, title, true); //use file name as title
	}

	return ret;
}

#ifdef COBRA_ONLY
static int get_name_iso_or_sfo(char *param_sfo, char *title_id, char *icon, const char *param, const char *entry_name, u8 f0, u8 f1, u8 uprofile, int flen, char *tempstr)
{
	// check entry path & returns file name without extension or path of sfo (for /PS3ISO) in param_sfo

	if(!param_sfo || !title_id || !icon || !param || !entry_name || !tempstr) return FAILED; // sanity check

	if(IS_NTFS)
	{   // ntfs
		const char *ntfs_sufix = NULL;

		if(flen > 17) ntfs_sufix = strstr(entry_name + flen - 14, ").ntfs[");

		if(ntfs_sufix)
		{
			ntfs_sufix--; u8 fprofile = (u8)(*ntfs_sufix); ntfs_sufix -= 2;

			if((fprofile >= '1') && (fprofile <= '4') && islike(ntfs_sufix, " ("))
			{
				// skip extended content of ntfs cached in /wmtmp if current user profile is 0
				if(uprofile == 0) return FAILED;

				fprofile -= '0';

				// skip non-matching extended content
				if(uprofile != fprofile) return FAILED;
			}
		}

		flen-=13; if(flen < 0) return FAILED;

		const char *ntfs_ext = entry_name + flen;
		if(IS_PS3ISO && !IS(ntfs_ext, ".ntfs[PS3ISO]")) return FAILED;
		if(IS_PS2ISO && !IS(ntfs_ext, ".ntfs[PS2ISO]")) return FAILED;
		if(IS_PSXISO && !IS(ntfs_ext, ".ntfs[PSXISO]")) return FAILED;
		if(IS_PSPISO && !IS(ntfs_ext, ".ntfs[PSPISO]")) return FAILED;
		if(IS_DVDISO && !IS(ntfs_ext, ".ntfs[DVDISO]")) return FAILED;
		if(IS_BDISO  && !strstr(ntfs_ext, ".ntfs[BD" )) return FAILED;
	}

	char *title = param_sfo;

	if(IS_PS3ISO)
	{
		flen = get_name(param_sfo, entry_name, GET_WMTMP); strcat(param_sfo + flen, ".SFO"); // WMTMP
		if(not_exists(param_sfo))
		{
			get_name(tempstr, entry_name, NO_EXT);
			if(IS_NTFS || IS_NET)
				concat_path2(param_sfo, WMTMP, tempstr, ".SFO"); // /PS3ISO
			else
				concat_path2(param_sfo, param, tempstr, ".SFO"); // /PS3ISO
		}

		if(get_title_and_id_from_sfo(title, title_id, entry_name, icon, tempstr, f0) != CELL_FS_SUCCEEDED || !HAS_TITLE_ID)
		{
			if(!IS_NTFS)
			{
				char *iso_file = param_sfo;
				concat_path(iso_file, param, entry_name); // get raw title ID from ISO

				if(read_file(iso_file, title_id, 11, 0x810) == 11)
				{
					strncpy(&title_id[4], &title_id[5], 5); title_id[TITLE_ID_LEN] = '\0';
				}
			}

			get_name(title, entry_name, NO_EXT);
		}
	}
	else
	{
		u8 info = webman_config->info & 0xF;

		if(*title_id == 0 && (info == 1 || info == 2) && (f0 < NET || f0 > NTFS))
		{
			char *iso_file = param_sfo;

			if((f1 == id_PSPISO)  && (!extcasecmp(entry_name, ".iso", 4)))
			{
				concat_path(iso_file, param, entry_name);
				read_file(iso_file, title_id, 10, 0x8373); title_id[10] = 0;
				if(title_id[4] == '-') memcpy(title_id + 4, title_id + 5, 6);
			}
			else if(f1 == id_PSXISO || f1 == id_PS2ISO)
			{
				if(get_cover_from_name(icon, entry_name, title_id) == false)
				{
					const char *ext = get_ext(entry_name);
					if(_IS(ext, ".iso") || _IS(ext, ".bin") || _IS(ext, ".img") || _IS(ext, ".mdf"))
					{
						char buf[0x800]; u32 sector;
						concat_path(iso_file, param, entry_name);
						if(read_file(iso_file, buf, 0x200, 0x8000) == 0x200)
						{
							// use CD sector size
							if(memcmp(buf + 1, "CD001", 5))
							{
								if(read_file(iso_file, buf, 0x200, 0x9318) == 0x200)
									sector = *((u32*)(buf + 0xA2)) * 0x930 + 0x18;
								else
									return FAILED;
							}
							else
								sector = *((u32*)(buf + 0xA2)) * 0x800;

							// read root directory
							if(read_file(iso_file, buf, 0x800, sector) != 0x800) return FAILED;

							// find executable
							for(u16 i = 0; i < 0x7D0; i += buf[i])
							{
								if( buf[i] == 0 ) break;
								if( buf[i + 0x20] != 0xD) continue;
								if((buf[i + 0x21] == 'S') && (buf[i + 0x25] == '_') && (buf[i + 0x29] == '.'))
								{
									buf[i + 0x25] = '-'; // replace _ with -
									memcpy(title_id, &buf[i + 0x21], 8);     // copy SLES-123
									memcpy(title_id + 8, &buf[i + 0x2A], 2); // copy 45
									title_id[10] = 0; memcpy(title_id + 4, title_id + 5, 6);
									break;
								}
							}
						}
					}
				}
			}
		}

		get_name(title, entry_name, NO_EXT);

		if(f1 >= id_BDISO && f1 <= id_ISO)
		{
			// info level: 0=Path, 1=Path + ID, 2=ID, 3=None
			if(info == 1 || info == 2)
			{
				get_cover_from_name(icon, entry_name, title_id); // get title_id from name
			}
		}
	}

	return CELL_OK;
}
#endif