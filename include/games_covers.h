#define TITLEID_LEN  10

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

#define HAS_TITLE_ID  ((*title_id >= 'A') && (*title_id <= 'Z') && !title_id[TITLE_ID_LEN])

#define NO_ICON       (!*icon)

#define FROM_MOUNT  -99

#define SHOW_COVERS_OR_ICON0  (webman_config->nocov != SHOW_DISC)
#define SHOW_COVERS          ((webman_config->nocov == SHOW_MMCOVERS) || (webman_config->nocov == ONLINE_COVERS))

static u8 ex[4] = {0, 1, 2, 3};

static const char ext[4][5] = {".jpg\0", ".png\0", ".PNG\0", ".JPG\0"};

static const char *cpath[6] = {MM_ROOT_STD, MM_ROOT_STL, MM_ROOT_SSTL, MANAGUNZ, "/dev_hdd0/GAMES", "/dev_hdd0/GAMEZ"};

static bool wm_icons_exists = false;

static bool HAS(char *icon)
{
	return ((*icon == 'h') || ((*icon == '/') && file_exists(icon) && (icon[strlen(icon) - 1] | 0x20) == 'g' ));
}

static void check_cover_folders(char *buffer)
{
#ifndef ENGLISH_ONLY
													covers_exist[0] = isDir(COVERS_PATH); // online url or custom path
#endif
		for(u8 p = 0; p < 6; p++)
		{
			sprintf(buffer, "%s/covers", cpath[p]); covers_exist[p + 1] = isDir(buffer);  // MM_ROOT_STD, MM_ROOT_STL, MM_ROOT_SSTL, "/dev_hdd0/GAMES", "/dev_hdd0/GAMEZ"
		}
													covers_exist[6] = isDir(WMTMP) && SHOW_COVERS_OR_ICON0; // WMTMP

#ifndef ENGLISH_ONLY
	if(!covers_exist[0]) {use_custom_icon_path = strstr(COVERS_PATH, "%s"); use_icon_region = strstr(COVERS_PATH, "%s/%s");} else {use_icon_region = use_custom_icon_path = false;}

	// disable custom icon from web repository if network is disabled //
	if(use_custom_icon_path && islike(COVERS_PATH, "http"))
	{
		char ip[ip_size] = "";
		netctl_main_9A528B81(ip_size, ip);
		if(*ip == NULL) use_custom_icon_path = false;

		is_xmbmods_server = islike(COVERS_PATH, LAUNCHPAD_COVER_SVR);
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

static bool get_image_file(char *icon, int flen)
{
	for(u8 e = 0; e < 4; e++)
	{
		strcpy(icon + flen, ext[ex[e]]);
		if(file_exists(icon)) {if(e>0) {u8 s=ex[e];ex[e]=ex[0],ex[0]=s;}; return true;}
	}
	return false;
}

static size_t get_name(char *name, const char *filename, u8 cache)
{
	// name:
	//   returns file name without extension & without title id (cache == 0 -> file name keeps path, cache == 2 -> remove path first)
	//   returns file name with WMTMP path                      (cache == 1 -> remove path first)

	int flen, pos = 0;
	if(cache) {pos = strlen(filename); while(pos > 0 && filename[pos - 1] != '/') pos--;}
	if(cache == 2) cache = 0;

	if(cache)
		flen = sprintf(name, "%s/%s", WMTMP, filename + pos);
	else
		flen = sprintf(name, "%s", filename + pos);

	if((flen > 2) && name[flen - 2] == '.' ) {flen -= 2; name[flen] = NULL;} // remove file extension (split iso)
	if((flen > 4) && name[flen - 4] == '.' ) {flen -= 4; name[flen] = NULL;} // remove file extension
	else
	if((flen > 3) && name[flen - 3] == '.' ) {flen -= 3; name[flen] = NULL;} // remove file extension for roms (.gb .gg .vb)
	else
	if(strstr(filename + pos, ".ntfs["))
	{
		while(name[flen] != '.') flen--; name[flen] = NULL; pos = flen - 4;
		if((pos > 0) && name[pos] == '.' && (strcasestr(ISO_EXTENSIONS, &name[pos]))) {flen = pos; name[flen] = NULL;} else
		if(!extcmp(name, ".BIN.ENC", 8)) {flen -= 8; name[flen] = NULL;}
	}

	if(cache) return (size_t) flen;

	// remove title id from file name
	if(name[4] == '_' && name[8] == '.' && (*name == 'B' || *name == 'N' || *name == 'S' || *name == 'U') && ISDIGIT(name[9]) && ISDIGIT(name[10])) {flen = sprintf(name, "%s", &name[12]);}// SLES_000.00-Name
	if(name[9] == '-' && name[10]== '[') {flen = sprintf(name, "%s", name + 11) - 1; name[flen] = NULL;} // BLES00000-[Name]
	if(name[10]== '-' && name[11]== '[') {flen = sprintf(name, "%s", name + 12) - 1; name[flen] = NULL;} // BLES-00000-[Name]
	if(!webman_config->tid) {char *p = strstr(name, " ["); if(p) *p = NULL; flen = strlen(name);}        // Name [BLES00000]

	return (size_t) flen;
}

static bool get_cover_by_titleid(char *icon, char *title_id)
{
	if(!HAS_TITLE_ID) return false;

	int flen;

	if(SHOW_COVERS)
	{
#ifndef ENGLISH_ONLY
		// Search covers in custom path
		if(covers_exist[0] && (webman_config->nocov == SHOW_MMCOVERS && *COVERS_PATH == '/'))
		{
			flen = sprintf(icon, "%s/%s", COVERS_PATH, title_id);
			if(get_image_file(icon, flen)) return true;
		}
#endif

		// Search retro covers in MM_ROOT_STD, MM_ROOT_STL, MM_ROOT_SSTL
		for(u8 p = 0; p < 3; p++)
		{
			if(covers_exist[p + 1] && *title_id == 'S')
			{
				flen = sprintf(icon, "%s/covers_retro/psx/%.4s_%.3s.%.2s_COV", cpath[p],
								title_id,
								title_id + 4, title_id + 7);

				if(get_image_file(icon, flen)) return true;
			}
		}

		// Search covers in MM_ROOT_STD, MM_ROOT_STL, MM_ROOT_SSTL, "/dev_hdd0/GAMES", "/dev_hdd0/GAMEZ"
		for(u8 p = 0; p < 6; p++)
			if(covers_exist[p + 1])
			{
				flen = sprintf(icon, "%s/covers/%s", cpath[p], title_id);
				if(get_image_file(icon, flen)) return true;
			}

		// Search covers in WMTMP
		if(covers_exist[6])
		{
			flen = sprintf(icon, "%s/%s", WMTMP, title_id);
			if(get_image_file(icon, flen)) return true;
		}

		// Search online covers
#ifdef ENGLISH_ONLY
		if(webman_config->nocov == ONLINE_COVERS)
		{
			sprintf(icon, COVERS_PATH, title_id);
			return true;
		}
#else
		if(use_custom_icon_path && (webman_config->nocov == ONLINE_COVERS) && (COVERS_PATH[0] == 'h'))
		{
			if(is_xmbmods_server && (*title_id != 'B' && *title_id != 'N')) {*icon = NULL; return false;}

			if(use_icon_region) sprintf(icon, COVERS_PATH,  (title_id[2] == 'U') ? "US" :
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

static bool get_cover_from_name(char *icon, const char *name, char *title_id)
{
	if(HAS(icon)) return true;

	// get cover from title_id in PARAM.SFO
	if(get_cover_by_titleid(icon, title_id)) return true;

	// get titleid from file name
	if(HAS_TITLE_ID) ;

	else if((*name == 'B' || *name == 'N' || *name == 'S' || *name == 'U') && ISDIGIT(name[6]) && ISDIGIT(name[7]))
	{
		if(name[4] == '_' && name[8] == '.')
			sprintf(title_id, "%.4s%.3s%.2s", name, name + 5, name + 9); //SCUS_999.99.filename.iso
		else if(ISDIGIT(name[8]))
			strncpy(title_id, name, TITLEID_LEN);
	}

	if(HAS_TITLE_ID) ;

	else if((strstr(name, "-[") || strstr(name, "[B") || strstr(name, "[NP") || strstr(name, "[SL") || strstr(name, "[SC")))
	{
		char *pos;
		if((pos = strstr(name, " [B")))
			strncpy(title_id, pos + 2, TITLEID_LEN); //BCES/BLES/BCUS/BLUS/etc.
		else if((pos = strstr(name, "[BL")))
			strncpy(title_id, pos + 1, TITLEID_LEN); //BLES/BLUS/etc.
		else if((pos = strstr(name, "[BC")))
			strncpy(title_id, pos + 1, TITLEID_LEN); //BCES/BCUS/etc.
		else if((pos = strstr(name, "[NP")))
			strncpy(title_id, pos + 1, TITLEID_LEN); //NP*
		else if((pos = strstr(name, " [S")))
			get_ps_titleid_from_path(title_id, pos + 2); //SLES/SLUS/SLPM/SLPS/SLAJ/SLKA/SCES/SCUS/SCPS/SCAJ/SCKA
		else if((pos = strstr(name, "[S")))
			get_ps_titleid_from_path(title_id, pos + 1); //SLES/SLUS/SLPM/SLPS/SLAJ/SLKA/SCES/SCUS/SCPS/SCAJ/SCKA
		else
			strncpy(title_id, name, TITLEID_LEN); // TITLEID-[NAME]
	}

	if(title_id[4] == '-') strncpy(&title_id[4], &title_id[5], 5); title_id[TITLE_ID_LEN] = NULL;

	// get cover using titleID obtained from file name
	if(get_cover_by_titleid(icon, title_id)) return true;

	return false;
}

static void get_default_icon_from_folder(char *icon, u8 is_dir, const char *param, const char *entry_name, char *title_id, u8 f0)
{
	//this function is called only from get_default_icon

	if(SHOW_COVERS_OR_ICON0)
	{
			if(is_dir && (webman_config->nocov == SHOW_ICON0)) {sprintf(icon, "%s/%s/PS3_GAME/ICON0.PNG", param, entry_name); if(!HAS(icon)) sprintf(icon, "%s/%s/ICON0.PNG", param, entry_name); return;}

			// get path/name and remove file extension
			int flen = sprintf(icon, "%s/%s", param, entry_name);

#ifdef COBRA_ONLY
			if(f0 == NTFS)
			{
				if(flen > 13 && icon[flen-13] == '.' && (!extcmp(icon, ".ntfs[PS3ISO]", 13) || !extcmp(icon, ".ntfs[PS2ISO]", 13)  || !extcmp(icon, ".ntfs[PSPISO]", 13) || !extcmp(icon, ".ntfs[DVDISO]", 13) || !extcmp(icon, ".ntfs[PSXISO]", 13) || !extcmp(icon, ".ntfs[BDFILE]", 13))) flen -= 13; else
				if(flen > 12 && icon[flen-12] == '.' &&  !extcmp(icon, ".ntfs[BDISO]" , 12)) flen -= 12;
			}
#endif
			if(flen > 2 && icon[flen - 2] == '.') flen -= 2; // remove file extension (split iso)
			if(flen > 4 && icon[flen - 4] == '.') flen -= 4; // remove file extension
			else					   
			if(flen > 3 && icon[flen - 3] == '.') flen -= 3; // remove file extension for roms (.gb .gg .vb)

			// get covers from iso folder
			if((f0 < 7 || f0 > NTFS) || (f0 == NTFS && (webman_config->nocov == SHOW_ICON0)))
			{
				if(get_image_file(icon, flen)) return;
			}

			if(HAS_TITLE_ID && SHOW_COVERS) {get_cover_by_titleid(icon, title_id); if(HAS(icon)) return;}

			// get mm covers & titleID
			get_cover_from_name(icon, entry_name, title_id);

			// get covers named as titleID from iso folder e.g. /PS3ISO/BLES12345.JPG
			if(!is_dir && HAS_TITLE_ID && (f0 < 7 || f0 > NTFS))
			{
				if(HAS(icon)) return;

				char titleid[STD_PATH_LEN];
				char *pos = strchr(entry_name, '/');
				if(pos)
					{*pos = NULL; sprintf(titleid, "%s/%s", entry_name, title_id); *pos = '/';} 
				else 
					sprintf(titleid, "%s", title_id);

				sprintf(icon, "%s/%s.JPG", param, titleid); if(file_exists(icon)) return;
				sprintf(icon, "%s/%s.PNG", param, titleid); if(file_exists(icon)) return;

				*icon = NULL;
			}

			// return ICON0
			if(is_dir) {sprintf(icon, "%s/%s/PS3_GAME/ICON0.PNG", param, entry_name); if(!HAS(icon)) sprintf(icon, "%s/%s/ICON0.PNG", param, entry_name); return;}

			// continue searching for covers
			if(SHOW_COVERS) return;

			// get covers/icons from /dev_hdd0/tmp/wmtmp
			flen = get_name(icon, entry_name, GET_WMTMP);

			if(get_image_file(icon, flen)) return;

			*icon = NULL;
	}
}

static void get_default_icon_for_iso(char *icon, const char *param, char *file, int isdir, int ns)
{
	//this function is called only from get_default_icon

	int flen;

	if(!extcmp(file, ".BIN.ENC", 8))
	{
		flen = sprintf(icon, "%s/%s", param, file);

		if(get_image_file(icon, flen)) return;

		flen -= 8; // remove .BIN.ENC
		if(flen > 0 && icon[flen] == '.')
		{
			if(get_image_file(icon, flen)) return;
		}
	}

	flen = sprintf(icon, "%s/%s", param, file);

	if(file_exists(icon) == false)
	{
		flen = get_name(icon, file, GET_WMTMP); //wmtmp
	}

	char *p = isdir ? NULL : strstr(icon + MAX(flen - 13, 0), ".ntfs[");
	if(p) {flen -= strlen(p), *p = NULL;}

	if((flen > 2) && icon[flen - 2] == '.' ) {flen -= 2, icon[flen] = NULL;} // remove file extension (split iso)
	if((flen > 4) && icon[flen - 4] == '.' ) {flen -= 4, icon[flen] = NULL;} // remove file extension
	else
	if((flen > 3) && icon[flen - 3] == '.' ) {flen -= 3, icon[flen] = NULL;} // remove file extension

	//file name + ext
	if(get_image_file(icon, flen)) return;

	//copy remote file
	if(file_exists(icon) == false)
	{
#ifdef NET_SUPPORT
		if(ns < 0) {*icon = NULL; return;}

		char remote_file[MAX_PATH_LEN];

		if(isdir)
		{
			if(webman_config->nocov == SHOW_DISC) return; // no icon0
			sprintf(remote_file, "%s/%s/PS3_GAME/ICON0.PNG", param, file);
			flen = get_name(icon, file, GET_WMTMP); sprintf(icon + flen, ".png");

			copy_net_file(icon, remote_file, ns, COPY_WHOLE_FILE);
			if(file_exists(icon)) return;
		}
		else
		{
			get_name(icon, file, NO_EXT);
			int tlen = sprintf(remote_file, "%s/%s", param, icon);

			int icon_len = get_name(icon, file, GET_WMTMP); //wmtmp

			for(u8 e = 0; e < 4; e++)
			{
				sprintf(icon + icon_len, "%s", ext[e]);
				if(file_exists(icon)) return;

				sprintf(remote_file + tlen, "%s", ext[e]);

				//Copy remote icon locally
				copy_net_file(icon, remote_file, ns, COPY_WHOLE_FILE);
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
			IS_PSX_FOLDER  ? iPSX :
			IS_PS2_FOLDER  ? iPS2 :
			IS_PSP_FOLDER  ? iPSP :
#ifdef MOUNT_ROMS
			IS_ROMS_FOLDER ? iROM :
#endif
			IS_DVD_FOLDER  ? iDVD : iBDVD;
}

static enum icon_type get_default_icon(char *icon, const char *param, char *file, int is_dir, char *title_id, int ns, u8 f0, u8 f1)
{
	char filename[STD_PATH_LEN];

	if(ns == FROM_MOUNT)
		snprintf(filename, STD_PATH_LEN, "%s", file);
	else
		*filename = NULL;

	enum icon_type default_icon = get_default_icon_by_type(f1);

	if(webman_config->nocov == SHOW_DISC) {if(get_cover_from_name(icon, file, title_id)) return default_icon; goto no_icon0;}

	if(!IS_NET) get_default_icon_from_folder(icon, is_dir, param, file, title_id, f0);

	// continue using cover or default icon0.png
	if(HAS(icon)) return default_icon;

	if(SHOW_COVERS && get_cover_from_name(icon, file, title_id)) return default_icon; // show mm cover

	// get icon from folder && copy remote icon
	get_default_icon_for_iso(icon, param, file, is_dir, ns);

	if(HAS(icon)) return default_icon;

	//use the cached PNG from wmtmp if available
	int flen = get_name(icon, file, GET_WMTMP);

	if(get_image_file(icon, flen)) return default_icon;

no_icon0:
	if(HAS(icon)) return default_icon;

		if((webman_config->nocov == SHOW_ICON0) && get_cover_from_name(icon, file, title_id)) return default_icon; // show mm cover as last option (if it's disabled)

	//show the default icon by type
	{
		sprintf(icon, "%s/%s", param + 6, file);

			 if(strstr(icon, "PSX")) //if(strstr(param, "/PSX") || !extcmp(file, ".ntfs[PSXISO]", 13))
			default_icon = iPSX;
		else if(strstr(icon, "PS2")) //if(strstr(param, "/PS2ISO") || !extcmp(param, ".BIN.ENC", 8) || !extcmp(file, ".ntfs[PS2ISO]", 13))
			default_icon = iPS2;
		else if(strstr(icon, "PSP")) //if(strstr(param, "/PSPISO") || strstr(param, "/ISO/") || !extcmp(file, ".ntfs[PSPISO]", 13))
			default_icon = iPSP;
		else if(strstr(icon, "DVD")) //if(strstr(param, "/DVDISO") || !extcmp(file, ".ntfs[DVDISO]", 13))
			default_icon = iDVD;
		else if(strstr(icon, "BDISO")) //if(strstr(param, "/BDISO") || !extcmp(file, ".ntfs[BDISO]", 13))
			default_icon = iBDVD;
		else if(strstr(icon, "/ROMS"))
			default_icon = iROM;
		else
			default_icon = iPS3;
	}

	if(!HAS(icon)) strcpy(icon, wm_icons[default_icon]);
	return default_icon;
}

static int get_title_and_id_from_sfo(char *templn, char *title_id, const char *entry_name, char *icon, char *data, u8 f0)
{
	int ret = FAILED;

	// read param.sfo
	unsigned char *mem = (u8*)data;
	u64 sfo_size = read_file(templn, data, _4KB_, 0);

	// get titleID & title from PARAM.SFO
	if(is_sfo(mem))
	{
		parse_param_sfo(mem, title_id, templn, (u16)sfo_size);

		if(SHOW_COVERS) get_cover_by_titleid(icon, title_id);

		ret = CELL_FS_SUCCEEDED;
	}

	if(webman_config->use_filename)
	{
		if(NO_ICON && !HAS_TITLE_ID) get_cover_from_name(icon, entry_name, title_id); // get titleID from name

		ret = ~CELL_FS_SUCCEEDED;
	}

	if(ret != CELL_FS_SUCCEEDED)
	{
		get_name(templn, entry_name, NO_PATH); if(!IS_NTFS) utf8enc(data, templn, true); //use file name as title
	}

	return ret;
}

#ifdef COBRA_ONLY
static int get_name_iso_or_sfo(char *templn, char *title_id, char *icon, const char *param, const char *entry_name, u8 f0, u8 f1, u8 uprofile, int flen, char *tempstr)
{
	// check entry path & returns file name without extension or path of sfo (for /PS3ISO) in templn

	if(IS_NTFS)
	{   // ntfs
		char *ntfs_sufix = NULL;

		if(flen > 17) ntfs_sufix = strstr((char*)entry_name + flen - 14, ").ntfs[");

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

		char *ntfs_ext = (char*)entry_name + flen;
		if(IS_PS3_FOLDER && !IS(ntfs_ext, ".ntfs[PS3ISO]")) return FAILED;
		if(IS_PS2_FOLDER && !IS(ntfs_ext, ".ntfs[PS2ISO]")) return FAILED;
		if(IS_PSX_FOLDER && !IS(ntfs_ext, ".ntfs[PSXISO]")) return FAILED;
		if(IS_PSP_FOLDER && !IS(ntfs_ext, ".ntfs[PSPISO]")) return FAILED;
		if(IS_DVD_FOLDER && !IS(ntfs_ext, ".ntfs[DVDISO]")) return FAILED;
		if(IS_BLU_FOLDER && !strstr(ntfs_ext, ".ntfs[BD" )) return FAILED;
	}

	if(IS_PS3_FOLDER)
	{
		get_name(templn, entry_name, GET_WMTMP); strcat(templn, ".SFO\0"); // WMTMP
		if( (!IS_NTFS) && file_exists(templn) == false)
		{
			get_name(tempstr, entry_name, NO_EXT);
			sprintf(templn, "%s/%s.SFO", param, tempstr); // /PS3ISO
		}

		if(get_title_and_id_from_sfo(templn, title_id, entry_name, icon, tempstr, f0) != CELL_FS_SUCCEEDED || !HAS_TITLE_ID)
		{
			if(!IS_NTFS)
			{
				sprintf(templn, "%s/%s", param, entry_name); // get raw title ID from ISO

				if(read_file(templn, title_id, 11, 0x810) == 11)
				{
					strncpy(&title_id[4], &title_id[5], 5); title_id[TITLE_ID_LEN] = NULL;
				}
			}

			get_name(templn, entry_name, NO_EXT);
		}
	}
	else
	{
		get_name(templn, entry_name, NO_EXT);
		if((f1 >= id_BDISO && f1 <= id_ISO) && (webman_config->info == 1 || webman_config->info == 2)) get_cover_from_name(icon, entry_name, title_id);
	}

	return CELL_OK;
}
#endif