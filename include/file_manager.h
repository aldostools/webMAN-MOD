#define SC_FS_DISK_FREE		840

#define ICON_STYLE			"style=\"position:fixed;top:%ipx;right:%ipx;max-height:176px;z-index:-1;display:none\" onerror=\"this.style.display='none';\""

u16 _LINELEN = LINELEN;
u16 _MAX_PATH_LEN = MAX_PATH_LEN;
u16 _MAX_LINE_LEN = MAX_LINE_LEN;

#define _48GB_	0xC00000000ULL

#define TABLE_ITEM_PREFIX  "<tr><td><a class=\""
#define TABLE_ITEM_SUFIX   "</td></tr>"

#define TABLE_ITEM_SIZE  28  // strlen(TABLE_ITEM_PREFIX + TABLE_ITEM_SUFIX) = (18 + 10)

#define FILE_MGR_KEY_LEN	10

#define ext5   name + MAX(flen - 5, 0)
#define ext8   name + MAX(flen - 8, 0)
#define ext13  name + MAX(flen - 13, 0)

static void set_file_type(const char *path, const char *filename, char *ftype)
{
	*ftype = NULL; if(!wm_icons_exists) return;

	if(strstr(path, "/PSX") || strstr(filename, ".ntfs[PSX"))
		sprintf(ftype, " psx");
#ifdef MOUNT_ROMS
	else if(strstr(path, "/ROMS/"))
		sprintf(ftype, " rom");
#endif
	else if(!extcmp(filename, ".BIN.ENC", 8) || strstr(path, "/PS2") || strstr(filename, ".ntfs[PS2"))
		sprintf(ftype, " ps2");
	else if(strstr(path, "/PSP") || strstr(filename, ".ntfs[PSP"))
		sprintf(ftype, " psp");
	else if(strstr(path, "/DVD") || strstr(filename, ".ntfs[DVD"))
		sprintf(ftype, " dvd");
	else
		sprintf(ftype, " ps3");
}

static int add_list_entry(char *param, int plen, char *tempstr, bool is_dir, char *ename, char *templn, char *name, char *fsize, CellRtcDateTime rDate, unsigned long long sz, char *sf, u8 is_net, u8 show_icon0, u8 is_ps3_http, u8 skip_cmd, u8 sort_by)
{
	bool is_root = (plen < 4);
	if(is_root) sz = 0, is_dir = true; // force folders in root -> fix: host_root, app_home

	unsigned long long sbytes = sz; u16 flen, slen;

	//////////////////
	// build labels //
	//////////////////

	char ftype[5]; *ftype = 0;

	if(!is_dir)
	{
		if(sz < 10240)	{sprintf(sf, "%s", STR_BYTE);} else
		if(sz < _2MB_)	{sprintf(sf, "%s", STR_KILOBYTE); sz >>= 10;} else
		if(sz < _48GB_) {sprintf(sf, "%s", STR_MEGABYTE); sz >>= 20;} else
						{sprintf(sf, "%s", STR_GIGABYTE); sz >>= 30;}
	}

	// encode file name for html
	if(is_net)
		flen = htmlenc(tempstr, name, 1);
	else
		flen = strlen(name);

	char *ext = name + MAX(flen - 4, 0); *fsize = NULL;


#ifndef LITE_EDITION
	//////////////////////////////////////////
	// show title & title ID from PARAM.SFO //
	//////////////////////////////////////////

	if( !is_dir && IS(name, "PARAM.SFO") )
	{
		char titleid[10], version[8], title[128]; snprintf(title, 128, "%s", templn);

		//get title & app version from PARAM.SFO
		getTitleID(templn, version, GET_VERSION);
		getTitleID(title, titleid, GET_TITLE_AND_ID); char *p = strstr(title, " ["); if(p) *p = NULL; p = strstr(title, "\n"); if(p) *p = NULL;
		get_cover_from_name(tempstr, strrchr(param, '/') + 1, titleid); // get title id from path (if title ID was not found in PARAM.SFO)
		if(*version >= '0') {strcat(title, " v"); strcat(title, version);}
		sprintf(tempstr, "%s%s", HDD0_GAME_DIR, titleid); bool has_updates_dir = file_exists(tempstr);

		sprintf(tempstr, "<label title=\"%s\">%s</label></a>", title, name); snprintf(name, MAX_PATH_LEN - 1, "%s", tempstr);

		// show title & link to patches folder
		if(has_updates_dir)
			snprintf(fsize, MAX_PATH_LEN - 1, HTML_URL2, HDD0_GAME_DIR, titleid, title);
		else
			snprintf(fsize, MAX_PATH_LEN - 1, "%s", title);

		// show title id & link to updates
		if(*titleid && !islike(titleid, "NPWR"))
			sprintf(tempstr, "<div class='sfo'>%s [<a href=\"%s/%s/%s-ver.xml\">%s</a>]</div>", fsize, "https://a0.ww.np.dl.playstation.net/tpl/np", titleid, titleid, titleid);
		else
			sprintf(tempstr, "<div class='sfo'>%s</div>", fsize);

		snprintf(fsize, MAX_PATH_LEN - 1, "%'llu %s%s", sz, sf, tempstr);

		#ifdef FIX_GAME
		if(has_updates_dir) snprintf(fsize, MAX_PATH_LEN - 1, "<a href=\"/fixgame.ps3%s%s\">%'llu %s</a>%s", HDD0_GAME_DIR, titleid, sz, sf, tempstr);
		#endif
	}
#endif

	/////////////////////////
	// encode url for html //
	/////////////////////////

	if(urlenc(tempstr, templn)) {tempstr[_MAX_LINE_LEN] = NULL; sprintf(templn, "%s", tempstr);}

	// is image?
	u8 show_img = (!is_dir && (_IS(ext, ".png") || _IS(ext, ".jpg") || _IS(ext, ".bmp")));

	char sclass = ' ', dclass = 'w'; // file class

	///////////////////////
	// build size column //
	///////////////////////

	if(is_dir)
	{
		dclass = 'd'; // dir class
		sbytes = 0;

		if(flen == 9 && IS(templn, "/host_root")) strcpy(templn, "/dev_hdd0/packages"); // replace host_root with a shortcut to /dev_hdd0/packages

		if(*name == '.')
		{
			sprintf(fsize, HTML_URL, templn, HTML_DIR);
		}
		else if(is_root)
		{
			bool show_play = ((flen == 8) && IS(templn, "/dev_bdvd") && IS_ON_XMB);

			if(show_play && isDir("/dev_bdvd/PS3_GAME"))
			{
				sprintf(fsize, HTML_URL, "/play.ps3", "&lt;Play>");
				if(wm_icons_exists) sprintf(ftype, " ps3");
			}
			else if(show_play && file_exists("/dev_bdvd/SYSTEM.CNF"))
			{
				sprintf(fsize, HTML_URL, "/play.ps3", "&lt;Play>");
				if(wm_icons_exists) sprintf(ftype, " ps2");
			}
			else if(show_play && isDir("/dev_bdvd/BDMV"))
			{
				sprintf(fsize, HTML_URL, "/play.ps3", "&lt;BDV>");
				if(wm_icons_exists) sprintf(ftype, " ps3");
			}
			else if(show_play && isDir("/dev_bdvd/VIDEO_TS"))
			{
				sprintf(fsize, HTML_URL, "/play.ps3", "&lt;DVD>");
				if(wm_icons_exists) sprintf(ftype, " dvd");
			}
			else if(show_play && isDir("/dev_bdvd/AVCHD"))
			{
				sprintf(fsize, HTML_URL, "/play.ps3", "&lt;AVCHD>");
				if(wm_icons_exists) sprintf(ftype, " ps3");
			}
			else if(IS(templn, "/app_home"))
			{
				sprintf(tempstr, "%s/%08i", "/dev_hdd0/home", xsetting_CC56EB2D()->GetCurrentUserNumber()); sprintf(fsize, HTML_URL, tempstr, HTML_DIR);
			}
			else
#if defined(LITE_EDITION) // || defined(USE_NTFS)
			{
				if(sys_admin && IS(templn, "/dev_flash"))
					sprintf(fsize, HTML_URL2, "/dev_blind", "?1", HTML_DIR);
				else if(IS(templn, "/dev_blind"))
					sprintf(fsize, HTML_URL2, "/dev_blind", "?0", HTML_DIR);
				else
					sprintf(fsize, "<a href=\"/mount.ps3%s\">%s</a>", templn, HTML_DIR);
			}
#else
			{
				u64 freeSize = 0, devSize = 0;

#ifdef USE_NTFS
				if(is_ntfs_path(templn))
				{
					struct statvfs vbuf;
					ps3ntfs_statvfs(templn + 5, &vbuf);
					freeSize = (u64)vbuf.f_bfree * (u64)vbuf.f_bsize, devSize = (u64)vbuf.f_blocks * (u64)vbuf.f_bsize;
				}
				else
#endif
				{system_call_3(SC_FS_DISK_FREE, (u64)(u32)(templn), (u64)(u32)&devSize, (u64)(u32)&freeSize);}

				unsigned long long	free_mb    = (unsigned long long)(freeSize>>20),
									free_kb    = (unsigned long long)(freeSize>>10),
									devsize_mb = (unsigned long long)(devSize >>20);

				if(sys_admin && IS(templn, "/dev_flash"))
					sprintf(tempstr, "%s%s", "/dev_blind", "?1");
				else if(IS(templn, "/dev_blind"))
					sprintf(tempstr, "%s%s", "/dev_blind", "?0");
#ifdef USE_NTFS
				else if(is_ntfs_path(templn))
					sprintf(tempstr, "/refresh.ps3?prepntfs");
#endif
				else
					sprintf(tempstr, "/mount.ps3%s", templn);

				// show graphic bar of device size & free space
				if(devSize > 0)
					sprintf(fsize,  "<div class='bf' style='height:18px;text-align:left;overflow:hidden;'><div class='bu' style='height:18px;width:%i%%'></div><div style='position:relative;top:-%ipx;text-align:right'>"
								"<a href=\"%s\" title=\"%'llu %s (%'llu %s) / %'llu %s (%'llu %s)\">&nbsp; %'8llu %s &nbsp;</a>"
								"</div></div>", (int)(100.0f * (float)(devSize - freeSize) / (float)devSize), is_ps3_http ? 20 : 18, tempstr, free_mb, STR_MBFREE, freeSize, STR_BYTE, devsize_mb, STR_MEGABYTE, devSize, STR_BYTE, (freeSize < _2MB_) ? free_kb : free_mb, (freeSize < _2MB_) ? STR_KILOBYTE : STR_MEGABYTE);
				else
					sprintf(fsize, "<a href=\"%s\">%s</a>", templn, HTML_DIR);
			}
#endif
			if(strstr(fsize, "&lt;")) strcat(fsize, " &nbsp; ");
		}
#ifdef COPY_PS3
 #ifdef USE_NTFS
		else if(is_ntfs_path(templn) || (!is_net && ( (flen == 5 && (_IS(name, "VIDEO") || strcasestr(name, "music"))) || (flen == 6 && _IS(name, "covers")) || islike(param, "/dev_hdd0/home") )))
 #else
		else if(!is_net && ( (flen == 5 && (_IS(name, "VIDEO") || strcasestr(name, "music"))) || (flen == 6 && _IS(name, "covers")) || islike(param, "/dev_hdd0/home") ))
 #endif
		{
			sprintf(fsize, "<a href=\"/copy.ps3%s\" title=\"copy to %s\">%s</a>", islike(templn, param) ? templn + plen : templn, islike(templn, "/dev_hdd0") ? drives[usb] : "/dev_hdd0", HTML_DIR);
		}
#endif
#ifdef FIX_GAME
		else if(islike(templn, HDD0_GAME_DIR) || (strstr(templn + 10, "/PS3_GAME" ) != NULL))
		{
			sprintf(fsize, HTML_URL2, "/fixgame.ps3", templn, HTML_DIR);
		}
#endif
		else
#ifdef PS2_DISC
			sprintf(fsize, "<a href=\"/mount%s%s\">%s</a>", strstr(name, "[PS2")?".ps2":".ps3", templn, HTML_DIR);
#else
			sprintf(fsize, "<a href=\"/mount.ps3%s\">%s</a>", templn, HTML_DIR);
#endif

		// links to home folders
		if((plen == 18 || plen == 26) && islike(templn, "/dev_bdvd/PS3_GAME"))
		{
			*tempstr = NULL;

			if(IS(name, "LICDIR"))
			{
				sprintf(tempstr, "%s/%08i/savedata", "/dev_hdd0/home", xsetting_CC56EB2D()->GetCurrentUserNumber());
			}
			else if(islike(name, "NPWR"))
			{
				sprintf(tempstr, "%s/%08i/trophy/%s", "/dev_hdd0/home", xsetting_CC56EB2D()->GetCurrentUserNumber(), name);
			}

			if(isDir(tempstr)) sprintf(fsize, "<a href=\"%s\">%s</a>", tempstr, HTML_DIR);
		}
	}

	else if(*fsize) ;

	else if(skip_cmd)
		sprintf(fsize, "%'llu %s", sz, sf);

#ifdef COBRA_ONLY
 #ifdef USE_NTFS
	else if(is_ntfs_path(templn))
	{
		sprintf(fsize, "<a href=\"/copy.ps3%s\" title=\"%'llu %s copy to %s\">%'llu %s</a>", islike(templn, param) ? templn + plen : templn, sbytes, STR_BYTE, "/dev_hdd0", sz, sf);
	}
 #endif
	else if( ((!is_net) && ( strstr(ext13, ".ntfs[") || _IS(ext8, ".BIN.ENC") )) || ((flen > 4) && (strcasestr(ISO_EXTENSIONS, ext) != NULL) && !islike(templn, HDD0_GAME_DIR)) )
	{
		if( (strcasestr(name, ".iso.") != NULL) && extcasecmp(name, ".iso.0", 6) && ( !strstr(ext13, ".ntfs[") ))
			sprintf(fsize, "<label title=\"%'llu %s\"> %'llu %s</label>", sbytes, STR_BYTE, sz, sf);
		else
			sprintf(fsize, "<a href=\"/mount.ps3%s\" title=\"%'llu %s\">%'llu %s</a>", templn, sbytes, STR_BYTE, sz, sf);

		set_file_type(param, templn + plen, ftype);
	}
#endif

#ifndef LITE_EDITION
	else if( (sbytes <= MAX_TEXT_LEN) && ( (strcasestr(".txt|.ini|.log|.sfx|.xml|.cfg|.his|.hip|.bup|.css|.html|conf|name|.bat", ext) != NULL) || islike(name, "wm_custom_") || (strcasestr(ext, ".js") != NULL) ) && !is_net && !islike(templn, DEV_NTFS) )
	{
		sprintf(fsize, "<a href=\"/edit.ps3%s\">%'llu %s</a>", templn, sz, sf);
		if(wm_icons_exists) sprintf(ftype, " pkg");
	}
#endif

#ifdef PKG_HANDLER
	else if( (f1_len > 11) && (strcasestr(templn, ".SELF") != NULL) )
		sprintf(fsize, "<a href=\"/mount.ps3%s\" title=\"%'llu %s\">%'llu %s</a>", templn, sbytes, STR_BYTE, sz, sf);
	else if( (f1_len > 11) && ((strstr(templn, "/ROMS/") != NULL) || (strcasestr(ROMS_EXTENSIONS, ext) != NULL)) )
	{
		sprintf(fsize, "<a href=\"/mount.ps3%s\" title=\"%'llu %s\">%'llu %s</a>", templn, sbytes, STR_BYTE, sz, sf);
		if(wm_icons_exists) sprintf(ftype, show_img ? " pic" : " rom");
	}
	else if( IS(ext, ".pkg") || IS(ext, ".PKG") || IS(ext, ".p3t") )
	{
		sprintf(fsize, "<a href=\"/install.ps3%s\">%'llu %s</a>", templn, sz, sf);
		if(wm_icons_exists) sprintf(ftype, " pkg");
	}
#endif

#ifdef COPY_PS3
	else if( IS(ext, ".bak") )
		sprintf(fsize, "<a href=\"/rename.ps3%s|\">%'llu %s</a>", templn, sz, sf);
	else if( show_img || _IS(ext, ".mp4") || _IS(ext, ".mkv") || _IS(ext, ".avi") || _IS(ext, ".mp3") || IS(ext, ".AT3") || IS(ext, ".PAM") )
	{
		sprintf(fsize, "<a href=\"/copy.ps3%s\" title=\"%'llu %s copy to %s\">%'llu %s</a>", islike(templn, param) ? templn + plen : templn, sbytes, STR_BYTE, islike(templn, "/dev_hdd0") ? drives[usb] : "/dev_hdd0", sz, sf);
		if(!wm_icons_exists) ;
		else if(show_img)
			sprintf(ftype, " pic");
		else if(_IS(ext, ".mp3") || IS(ext, ".AT3"))
			sprintf(ftype, " snd");
		else
			sprintf(ftype, " vid");
	}
	else if( IS(ext5, ".edat")
 #ifndef PKG_HANDLER
			|| IS(ext,  ".pkg") ||  IS(ext, ".p3t")
 #endif
			|| IS(ext,  ".rco") ||  IS(ext, ".qrc")
			|| !memcmp(name, "webftp_server", 13) || !memcmp(name, "coldboot", 8)
 #ifdef SWAP_KERNEL
			|| !memcmp(name, "lv2_kernel", 10)
 #endif
			)
		sprintf(fsize, "<a href=\"/copy.ps3%s\" title=\"%'llu %s copy to %s\">%'llu %s</a>", islike(templn, param) ? templn + plen : templn, sbytes, STR_BYTE, islike(templn, "/dev_hdd0") ? drives[usb] : "/dev_hdd0", sz, sf);
#endif //#ifdef COPY_PS3

#ifdef LOAD_PRX
	else if(!is_net && ( IS(ext5, ".sprx")))
		sprintf(fsize, "<a href=\"/loadprx.ps3?slot=6&prx=%s\">%'llu %s</a>", templn, sz, sf);
#endif
	else if(sbytes < 10240)
		sprintf(fsize, "%'llu %s", sz, sf);
	else
		sprintf(fsize, "<label title=\"%'llu %s\"> %'llu %s</label>", sbytes, STR_BYTE, sz, sf);

	////////////////////
	// build sort key //
	////////////////////

	if(sort_by == 's')
	{	// convert sbyte to base 255 to avoid nulls that would break strncmp in sort
		memset(ename, 1, FILE_MGR_KEY_LEN -1); u8 index = 6;
		while (sbytes > 0)
		{
			ename[index] = 1 + (sbytes % 255);
			sbytes = sbytes / 255ULL;
			if(index == 0) break; else index--;
		}
	}
	else if(sort_by == 'd')
		snprintf(ename, FILE_MGR_KEY_LEN, "%c%c%c%c%c", ((rDate.year - 1968) % 223) + 0x20, rDate.month+0x20, rDate.day+0x20, rDate.hour+0x20, rDate.minute+0x20);
	else
	{
		//if(*name == '0' && flen == 8 && IS(param, "/dev_hdd0/home"))
		//	snprintf(ename, FILE_MGR_KEY_LEN, "%s", name + 3);
		//else
		snprintf(ename, FILE_MGR_KEY_LEN, "%s         ", name); sclass = dclass;

		if(flen > FILE_MGR_KEY_LEN -2) {char c = name[flen - 1]; if(ISDIGIT(c)) ename[FILE_MGR_KEY_LEN - 2] = c;} // sort isos
	}

	if((plen > 1) && memcmp(templn, param, plen) == 0) sprintf(templn, "%s", templn + plen + 1); // remove path from templn (use relative path)


	//////////////////////
	// build list entry //
	//////////////////////

	const u16 dlen = FILE_MGR_KEY_LEN + 21; // key length + length of date column

	// -- key
	*tempstr = sclass; memcpy(tempstr + 1, ename, FILE_MGR_KEY_LEN -1);

	// -- name column
	flen = sprintf(tempstr + FILE_MGR_KEY_LEN,
							 "%c%s\" href=\"%s\"%s>%s</a></td>",
							 dclass, ftype, templn,
#ifndef LITE_EDITION
							 show_img ? " onmouseover=\"s(this,0);\"" : (is_dir && show_icon0) ? " onmouseover=\"s(this,1);\"" :
#endif
							 "", name);

	// -- size column
	slen =  sprintf(templn, "<td> %s%s</td>",
							fsize, is_root ? "" : " &nbsp; ");

	// -- reduce features if html code is too long
	if((FILE_MGR_KEY_LEN + flen + slen + dlen) >= _LINELEN)
	{
		// -- remove link from size column
		if(is_dir) sprintf(fsize, HTML_DIR); else sprintf(fsize, "%llu %s", sz, sf);

		// -- rebuild size column without link
		slen = sprintf(templn, "<td> %s%s</td>",
								fsize, is_root ? "" : " &nbsp; ");

		// -- rebuild name without link if html code is still too long
		if((FILE_MGR_KEY_LEN + flen + slen + dlen) >= _LINELEN)
		{
			flen = sprintf(tempstr + FILE_MGR_KEY_LEN,
									 "%c%s\">%s</a></td>",
									 dclass, ftype, name);
		}
	}

	// append size column
	sprintf(tempstr + FILE_MGR_KEY_LEN + flen, "%s", templn);

	// append date column (21 bytes)
	sprintf(tempstr + FILE_MGR_KEY_LEN + flen + slen, "<td>%02i-%s-%04i %02i:%02i",
														rDate.day, smonth[rDate.month-1], rDate.year, rDate.hour, rDate.minute);

	flen += slen + dlen; // size of key + name column + size column + date column

	if(FILE_MGR_KEY_LEN + flen >= _LINELEN) {flen = 0, *tempstr = NULL;} //ignore file if it is still too long

	return flen;
}

static int add_breadcrumb_trail(char *pbuffer, const char *param)
{
	int slen, tlen = 0;

	char swap[MAX_PATH_LEN], templn[MAX_PATH_LEN], url[MAX_PATH_LEN], *slash, *buffer = pbuffer;

	sprintf(templn, "%s", param);

	// add links to path
	prev_dest = last_dest = NULL; // init fast concat

	while((slash = strchr(templn + 1, '/')))
	{
		*slash = NULL;
		tlen+=(slash - templn) + 1; //strlen(templn) + 1;

		strcpy(swap, param);
		swap[tlen] = NULL;

		buffer += concat(buffer, "<a class=\"f\" href=\"");
		urlenc(url, swap);
		buffer += concat(buffer, url);

		htmlenc(url, templn, 1);
		sprintf(swap, "\">%s</a>/", templn);
		buffer += concat(buffer, swap);

		strcpy(templn, param + tlen);
	}

	// add link to file or folder
	if(!param[1]) slen = sprintf(swap, "/");
	else if((param[1] != 'n') && not_exists(param)) slen = sprintf(swap, "%s", strrchr(param, '/') + 1);
	else
	{
		tlen = strlen(param) - 4; if(tlen < 0) tlen = 0;

		char label[_MAX_PATH_LEN];
		urlenc(url, param); if(islike(param, "/net")) htmlenc(label, templn, 0); else strcpy(label, templn);

#ifdef USE_NTFS
		if(is_ntfs_path(param)) slen = sprintf(swap, HTML_URL, WMTMP, label);
		else
#endif
		slen = sprintf(swap, HTML_URL2,
						strstr(pbuffer, "To: ") ? "" :
						islike(param + 23, "/trophy/NPWR") ? "/delete.ps3" :
						islike(param, "/dev_hdd0/home") ? "/copy.ps3" :
#ifdef FIX_GAME
						islike(param, HDD0_GAME_DIR) ? "/fixgame.ps3" :
#endif
#ifdef PKG_HANDLER
						!extcmp(param + tlen, ".pkg", 4) ? "/install.ps3" :
#endif
						islike(param, "/dev_hdd0/GAMES/covers") ? "" :
						((isDir(param) || strcasestr(ISO_EXTENSIONS, param + tlen) != NULL) || (strstr(param, "/GAME") != NULL) || (strstr(param, ".ntfs[") != NULL) || (strstr(param, "/GAME") != NULL) || islike(param, "/net") || !extcmp(param + MAX(tlen - 4, 0), ".BIN.ENC", 8)) ? "/mount.ps3" :
						"", url, label);
	}

	// add code to buffer

	strcat(buffer, swap);

	return slen + (buffer - pbuffer);
}

static bool folder_listing(char *buffer, u32 BUFFER_SIZE_HTML, char *templn, char *param, int conn_s, char *tempstr, char *header, u8 is_ps3_http, s8 sort_by, s8 sort_order, char *file_query)
{
	int fd; t_string sout; _set(&sout, buffer, strlen(buffer));

	CellRtcDateTime rDate;

	if(sys_admin && islike(param, "/dev_blind?"))
	{
		if( param[11] & 1) enable_dev_blind(NO_MSG); else //enable
		if(~param[11] & 1) disable_dev_blind();           //disable

		if( param[11] ) {sprintf(templn, HTML_REDIRECT_TO_URL, "/", HTML_REDIRECT_WAIT); _concat(&sout, templn);}

		sprintf(templn, "/dev_blind: %s", isDir("/dev_blind") ? STR_ENABLED : STR_DISABLED); _concat(&sout, templn); return true; //goto send_response;
	}

	absPath(templn, param, "/"); // auto mount /dev_blind & /dev_hdd1

	u8 is_net = (param[1] == 'n'), skip_cmd = 0;

	bool is_ntfs = false;

	int plen = strlen(param);
	bool is_root = (plen < 4);

#ifdef USE_NTFS
	struct stat bufn;
	DIR_ITER *pdir = NULL;

	is_ntfs = is_ntfs_path(param);
	if((is_root && root_check) | is_ntfs) check_ntfs_volumes();

	if(is_ntfs)
	{
		pdir = ps3ntfs_opendir(param); // /dev_ntfs1v -> ntfs1:
		if(!pdir) is_ntfs = false;
		cellRtcSetTime_t(&rDate, 0);
		if(!param[12]) param[11] = 0;
	}
#endif

#if defined(FIX_GAME) || defined(COPY_PS3)
	if(copy_aborted) _concat(&sout, STR_CPYABORT);    //  /copy.ps3$abort
	else
	if(fix_aborted)  _concat(&sout, "Fix aborted!");  //  /fixgame.ps3$abort

	if(copy_aborted | fix_aborted) {_concat(&sout, "<p>"); sys_ppu_thread_usleep(100000); copy_aborted = fix_aborted = false;}
#endif

	_LINELEN = LINELEN;
	_MAX_PATH_LEN = MAX_PATH_LEN;
	_MAX_LINE_LEN = MAX_LINE_LEN;

	if(is_ntfs || is_net || cellFsOpendir(param, &fd) == CELL_FS_SUCCEEDED)
	{
		if(!extcmp(param + MAX(plen - 7, 0), "/exdata", 7)) {_LINELEN = _MAX_LINE_LEN = _MAX_PATH_LEN = 200; skip_cmd = 1;}

		unsigned long long sz = 0, dir_size = 0;
		char ename[16], sf[8];
		char fsize[MAX_PATH_LEN], *swap = fsize;
		u16 idx = 0, dirs = 0, flen; bool is_dir;
		u32 tlen = 0;
		char *sysmem_html = buffer + (webman_config->sman ? _12KB_ : _6KB_);

		typedef struct
		{
			char path[_LINELEN];
		} t_line_entries;

		t_line_entries *line_entry = (t_line_entries *)sysmem_html;
		u16 max_entries = ((BUFFER_SIZE_HTML - _12KB_) / _MAX_LINE_LEN) - 1;

		BUFFER_SIZE_HTML -= _2KB_;

		u8 jb_games = (strstr(param, "/GAMES") || strstr(param, "/GAMEZ"));
		u8 show_icon0 = jb_games || (islike(param, "/dev_hdd0/game") || islike(param, "/dev_hdd0/home/"));

#ifndef LITE_EDITION
		sprintf(templn, "<img id=\"icon\" " ICON_STYLE ">"
						"<script>"
						// show icon of item pointed with mouse
						"function s(o,d){u=o.href;p=u.indexOf('.ps3');if(p>0)u=u.substring(p+4);if(d){p=u.indexOf('/PS3_');if(p<0)p=u.indexOf('/USRDIR');if(p>0)u=u.substring(0,p);u+='%s/ICON0.PNG';}icon.src=u;icon.style.display='block';}"
						"</script>", webman_config->sman ? 98 : 118, webman_config->sman ? 25 : 10, (jb_games ? "/PS3_GAME" : "")); _concat(&sout, templn);
#endif

		// breadcrumb trail //
		sout.size += add_breadcrumb_trail(buffer, param); if(param[10] != ':') _concat(&sout, ":");

		if((param[7] == 'v' || param[1] == 'a') && IS_ON_XMB && (isDir("/dev_bdvd/PS3_GAME") || file_exists("/dev_bdvd/SYSTEM.CNF") || isDir("/dev_bdvd/BDMV") || isDir("/dev_bdvd/VIDEO_TS") || isDir("/dev_bdvd/AVCHD")))
			_concat(&sout, " <a href=\"/play.ps3\">&lt;Play>&nbsp;</a><br>");
		else
			_concat(&sout, "<br>");

#ifdef COPY_PS3
		if(cp_mode) {sprintf(tempstr, "<font size=2><a href=\"/paste.ps3%s\">&#128203;</a> ", is_net ? "/dev_hdd0/packages" : param); add_breadcrumb_trail(tempstr, cp_path); _concat(&sout, tempstr); _concat(&sout, "</font><p>"); }

		usb = get_default_usb_drive(0);
#endif
		_concat(&sout, "<style>.sfo{position:absolute;top:300px;right:10px;font-size:14px}td+td{text-align:right}</style>"
									  "<table id=\"files\" class=\"propfont\">");
/*
		if(file_exists("/dev_hdd0/xmlhost/game_plugin/sort.js"))
			_concat(&sout, "<script src=\"/dev_hdd0/xmlhost/game_plugin/sort.js\"></script>"
									 "<thead><tr><th align=left>Name</th><th>Size</th><th>Date</th></tr></thead>");
		else
*/
			_concat(&sout, "<tr><td colspan=3><col width=\"220\"><col width=\"98\">");

		tlen = sout.size;

#ifdef NET_SUPPORT
		if(is_net)
		{
			int ns = FAILED, abort_connection = 0; char netid = param[4]; netiso_svrid = (netid & 0x0F);

			if(netid >= '0' && netid <= '4') ns = connect_to_remote_server(netid);

			if(ns >= 0)
			{
				strcat(param, "/");
				if(open_remote_dir(ns, param + 5, &abort_connection) >= 0)
				{
					strcpy(templn, param); while(templn[plen] == '/') templn[plen--] = NULL; plen++;
					char *p = strrchr(templn, '/'); if(p) *p = NULL; if(strlen(templn) < 6 && strlen(param) < 8) {templn[0] = '/', templn[1] = NULL;}

					urlenc(swap, templn);
					flen = sprintf(line_entry[idx].path,  "!         " // <-- size should be = FILE_MGR_KEY_LEN
														  "f\" href=\"%s\">..</a></td>"
														  "<td> " HTML_URL HTML_ENTRY_DATE
														, swap, swap, HTML_DIR);

					if(flen >= _MAX_LINE_LEN) return false; //path is too long

					idx++, dirs++;
					tlen += flen;

					sys_addr_t data = 0;
					netiso_read_dir_result_data *dir_items = NULL;
					int v3_entries = read_remote_dir(ns, &data, &abort_connection);
					if(data != NULL)
					{
						dir_items = (netiso_read_dir_result_data*)data;

						for(int n = 0; n < v3_entries; n++)
						{
							if(dir_items[n].name[0] == '.' && dir_items[n].name[1] == 0) continue;
							if(tlen > BUFFER_SIZE_HTML) break;
							if(idx >= (max_entries-3)) break;

							if(*file_query && (strcasestr(dir_items[n].name, file_query) == NULL)) continue;

							if(param[1] == 0)
								flen = sprintf(templn, "/%s", dir_items[n].name);
							else
							{
								flen = sprintf(templn, "%s%s", param, dir_items[n].name);
							}
							if(templn[flen - 1] == '/') templn[flen--] = NULL;

							cellRtcSetTime_t(&rDate, dir_items[n].mtime);

							sz = (unsigned long long)dir_items[n].file_size; dir_size += sz;

							is_dir = dir_items[n].is_directory; if(is_dir) dirs++;

							flen = add_list_entry(param, plen, tempstr, is_dir, ename, templn, dir_items[n].name, fsize, rDate, sz, sf, true, show_icon0, is_ps3_http, skip_cmd, sort_by);

							if((flen == 0) || (flen >= _MAX_LINE_LEN)) continue; //ignore lines too long
							memcpy(line_entry[idx].path, tempstr, FILE_MGR_KEY_LEN + flen + 1); idx++;
							tlen += (flen + TABLE_ITEM_SIZE);

							if(!working) break;
						}
						sys_memory_free(data);
					}
				}
				else //may be a file
				{
					if(param[plen] == '/') param[plen] = NULL;

					int is_directory = 0;
					s64 file_size;
					u64 mtime, ctime, atime;
					if(remote_stat(ns, param + 5, &is_directory, &file_size, &mtime, &ctime, &atime, &abort_connection) == 0)
					{
						if(file_size && !is_directory)
						{
							if(open_remote_file(ns, param + 5, &abort_connection) > 0)
							{
								size_t header_len = prepare_header(header, param, 1);
								header_len += sprintf(header + header_len, "Content-Length: %llu\r\n\r\n", (unsigned long long)file_size);

								send(conn_s, header, header_len, 0);

								u32 bytes_read; s64 boff = 0;
								while(boff < file_size)
								{
									bytes_read = read_remote_file(ns, (char*)buffer, boff, _64KB_, &abort_connection);
									if(bytes_read)
									{
										if(send(conn_s, buffer, bytes_read, 0) < 0) break;
									}
									boff+=bytes_read;
									if(bytes_read < _64KB_ || boff >= file_size) break;
								}
								open_remote_file(ns, "/CLOSEFILE", &abort_connection);
								sclose(&ns);
								sclose(&conn_s);

								return false; // net file
							}
						}
					}
				}
				sclose(&ns);
			}
		}
		else
#endif
		{
			CellFsDirectoryEntry entry; u32 read_f;
			CellFsDirent entry_s; u64 read_e; // list root folder using the slower readdir
			char *entry_name = (is_root) ? entry_s.d_name : entry.entry_name.d_name;

#ifdef USE_NTFS
			if(is_ntfs && !param[11])
			{
				flen = sprintf(line_entry[idx].path,  "!     "
													  "f\" href=\"%s\">..</a></td>"
													  "<td> " HTML_URL HTML_ENTRY_DATE
													, "/", "/", HTML_DIR);
				idx++, dirs++;
				tlen += flen;
			}
#endif
			while(working)
			{
#ifdef USE_NTFS
				if(is_ntfs)
				{
					if(ps3ntfs_dirnext(pdir, entry_name, &bufn)) break;
					if(entry_name[0] == '$' && param[12] == 0) continue;

					entry.attribute.st_mode = bufn.st_mode, entry.attribute.st_size = bufn.st_size, entry.attribute.st_mtime = bufn.st_mtime;
				}
				else
#endif
				if(is_root) {if((cellFsReaddir(fd, &entry_s, &read_e) != CELL_FS_SUCCEEDED) || (read_e == 0)) break;}
				else
				if(cellFsGetDirectoryEntries(fd, &entry, sizeof(entry), &read_f) || !read_f) break;

				if(entry_name[0] == '.' && entry_name[1] == 0) continue;
				if(tlen > BUFFER_SIZE_HTML) break;
				if(idx >= (max_entries-3)) break;

				if(*file_query && (strcasestr(entry_name, file_query) == NULL)) continue;

#ifdef USE_NTFS
				// use host_root to expand all /dev_ntfs entries in root
				bool is_host = is_root && ((mountCount > 0) && IS(entry_name, "host_root") && mounts);

				u8 ntmp = 1;
				if(is_host) ntmp = mountCount + 1;

				for (u8 u = 0; u < ntmp; u++)
				{
					if(u) {sprintf(entry_name, "dev_%s", mounts[u-1].name);}
#endif
					if(is_root)
					{
						flen = sprintf(templn, "/%s", entry_name);
					}
					else
					{
						flen = sprintf(templn, "%s/%s", param, entry_name);
					}
					if(templn[flen - 1] == '/') templn[flen--] = NULL;

					if(is_root)
					{
						struct CellFsStat buf;
						cellFsStat(templn, &buf);
						entry.attribute.st_mode  = buf.st_mode;
						entry.attribute.st_size  = buf.st_size;
						entry.attribute.st_mtime = buf.st_mtime;
					}

					cellRtcSetTime_t(&rDate, entry.attribute.st_mtime);

					sz = (unsigned long long)entry.attribute.st_size; dir_size += sz;

					is_dir = (entry.attribute.st_mode & S_IFDIR); if(is_dir) dirs++;

					flen = add_list_entry(param, plen, tempstr, is_dir, ename, templn, entry_name, fsize, rDate, sz, sf, false, show_icon0, is_ps3_http, skip_cmd, sort_by);

					if((flen == 0) || (flen >= _MAX_LINE_LEN)) continue; //ignore lines too long
					memcpy(line_entry[idx].path, tempstr, FILE_MGR_KEY_LEN + flen + 1); idx++;
					tlen += (flen + TABLE_ITEM_SIZE);

					if(!working) break;
#ifdef USE_NTFS
				}
#endif
			}

#ifdef USE_NTFS
			if(is_ntfs)
			{
				if(pdir) ps3ntfs_dirclose(pdir);
			}
			else
#endif
			cellFsClosedir(fd);
		}

		/////////////////////////////
		// add net entries to root //
		/////////////////////////////

#ifdef NET_SUPPORT
		if(is_root)
		{
			for(u8 n = 0; n < 5; n++)
			{
				if(is_netsrv_enabled(n))
				{
					sprintf(line_entry[idx].path, "dnet%i     " // <-- size should be = FILE_MGR_KEY_LEN
												  "d\" href=\"/net%i\">net%i (%s:%i)</a></td>"
												  "<td> <a href=\"/mount.ps3/net%i\">%s</a>" HTML_ENTRY_DATE
												  , n, n, n, webman_config->neth[n], webman_config->netp[n], n, HTML_DIR); idx++;
				}
			}
		}
#endif

		///////////////////////
		// sort list entries //
		///////////////////////

		if(idx)
		{   // sort html file entries
			u16 n, m;
			t_line_entries swap;
			for(n = 0; n < (idx - 1); n++)
				for(m = (n + 1); m < idx; m++)
					if(sort_order * strncmp(line_entry[n].path, line_entry[m].path, FILE_MGR_KEY_LEN) > 0)
					{
						swap = line_entry[n];
						line_entry[n] = line_entry[m];
						line_entry[m] = swap;
					}
		}

		//////////////////////
		// add list entries //
		//////////////////////

		for(u16 m = 0; m < idx; m++)
		{
			if(tlen > BUFFER_SIZE_HTML) break;

			_concat(&sout, TABLE_ITEM_PREFIX);
			_concat(&sout, (line_entry[m].path) + FILE_MGR_KEY_LEN);
			_concat(&sout, TABLE_ITEM_SUFIX);
		}

		_concat(&sout, "</table>");

		//////////////////
		// build footer //
		//////////////////

		if(!is_root)
		{
			///////////
#ifdef COBRA_ONLY
			unsigned int real_disctype, iso_disctype, effective_disctype = 1;
#endif
			if(!is_ps3_http)
			{
				bool show_icon = false;
				if(is_net && jb_games)
				{
					char *p = strchr(param + 12, '/'); if(p) *p = NULL; sprintf(templn, "%s/PS3_GAME/ICON0.PNG", param); show_icon = true;
				}

				if(!show_icon)
				{
					sprintf(templn, "%s/ICON0.PNG", param); show_icon = file_exists(templn);                    // current folder
					if(!show_icon) sprintf(templn, "%s/ICON2.PNG", param); show_icon = file_exists(templn);     // ps3_extra folder
					if(!show_icon)
					{
						char *p = strchr(param + 18, '/'); if(p) *p = NULL;
						sprintf(templn, "%s/PS3_GAME/ICON0.PNG", param); show_icon = file_exists(templn);       // dev_bdvd or jb folder
						if(!show_icon) sprintf(templn, "%s/ICON0.PNG", param); show_icon = file_exists(templn); // game dir
					}
				}

				if(!show_icon && islike(param, "/dev_bdvd"))
				{
					enum icon_type dt = iPS3;
#ifdef COBRA_ONLY
					cobra_get_disc_type(&real_disctype, &effective_disctype, &iso_disctype);
					if(iso_disctype == DISC_TYPE_PSX_CD) dt = iPSX; else
					if(iso_disctype == DISC_TYPE_PS2_DVD
					|| iso_disctype == DISC_TYPE_PS2_CD) dt = iPS2; else
					if(iso_disctype == DISC_TYPE_DVD)    dt = iDVD;
#endif
					sprintf(templn, "%s", wm_icons[dt]); show_icon = true;
				}

				for(u16 m = idx; m < 8; m++) _concat(&sout, "<BR>");

				if(show_icon || show_icon0)
					{urlenc(swap, templn); sprintf(templn, "<script>icon.src=\"%s\";icon.style.display='block';</script>", swap); _concat(&sout, templn);}
			}
			///////////

#ifdef COPY_PS3
 #ifdef EMBED_JS
			sprintf(tempstr, // popup menu
							"<div id='mnu' style='position:fixed;width:140px;background:#333;display:none;padding:5px;'>"
  #ifdef PKG_HANDLER
							"<a id='m0'>%s</a>"
  #endif
							"<a id='m1'>%s</a><a id='m2'>%s</a><hr><a id='m3'>%s<br></a><a href=\"javascript:t=prompt('%s',self.location.pathname);if(t.indexOf('/dev_')==0)self.location='/mkdir.ps3'+t\">%s</a><hr>"
							"<a id='m4'>%s<br></a><a id='m5'>%s<br></a><a id='m6'>%s</a><hr><a id='m7'>%s<br></a><a id='m8'>%s</a>"
							"</div>"
							"<script>var s,m;window.addEventListener('contextmenu',function(e){if(s)s.color='#ccc';t=e.target,s=t.style,c=t.className,m=mnu.style,p=t.pathname;if(c=='w'||c=='d'){e.preventDefault();s.color='#fff',b='block',n='none';m.display=b;m.left=(e.clientX+12)+'px';y=e.clientY;w=window.innerHeight;m.top=(((y+220)<w)?(y+12):(w-220))+'px';"
  #ifdef PKG_HANDLER
							"m0.href='/install.ps3'+p;m0.style.display=(p.indexOf('.pkg')>0)?b:n;"
  #endif
							"m1.href='/mount.ps3'+p;m1.style.display=(p.toLowerCase().indexOf('.iso')>0||c=='d'||p.indexOf('/GAME')>0)?b:n;"
							"m2.href=p;m2.text=(c=='w')?'Download':'Open';"
							"m3.href='/delete.ps3'+p;"
							"m4.href='/cut.ps3'+p;"
							"m5.href='/cpy.ps3'+p;"
							"m6.href='/paste.ps3'+self.location.pathname;"
							"m7.href='javascript:rn(\"'+p+'\")';m7.style.display=(p.substring(0,5)=='/dev_')?b:n;"
							"m8.href='/copy.ps3'+p}},false);"
							"window.onclick=function(e){if(m)m.display=n;};"

							// F2 = rename/move item pointed with mouse
							"document.addEventListener('keyup',ku,false);"
							"function rn(f){if(f.substring(0,5)=='/dev_'){f=unescape(f);t=prompt('Rename to:',f);if(t&&t!=f)self.location='/rename.ps3'+f+'|'+escape(t)}}"
							"function ku(e){e=e||window.event;if(e.keyCode==113){var a=document.querySelectorAll('a:hover')[0].pathname;rn(a);}}"

						 	"</script>",
  #ifdef PKG_HANDLER
							"Install PKG", //m0
  #endif
							"Mount", "Open", "Delete", "New Folder", "New Folder", "Cut", "Copy", "Paste", "Rename", "Copy To"); _concat(&sout, tempstr);
 #else
			// add fm.js script
			if(file_exists(FM_SCRIPT_JS))
			{
				sprintf(templn, SCRIPT_SRC_FMT, FM_SCRIPT_JS); _concat(&sout, templn);
			}
 #endif // #ifdef EMBED_JS
#endif // #ifdef COPY_PS3

			*tempstr = NULL;

#ifdef COBRA_ONLY
			// show last mounted game
			if(effective_disctype != DISC_TYPE_NONE && IS(param, "/dev_bdvd"))
			{
				// get last game path
				get_last_game(templn);

				if(*templn == '/') {sprintf(tempstr, HTML_SHOW_LAST_GAME); add_breadcrumb_trail(tempstr, templn); strcat(tempstr, HTML_SHOW_LAST_GAME_END);}
			}
#endif
			///////////
			char *slash = strchr(param + 1, '/');
			if(slash) *slash = NULL;

			sprintf(templn, "<hr>"
							"<b>" HTML_URL "%c", param, param, (param[10] == ':') ? 0 : ':');

			_concat(&sout, templn);

			if(param[1] != 'n')
			{
				sprintf(templn, " %'d %s", (int)(get_free_space(param)>>20), STR_MBFREE);
				_concat(&sout, templn);
			}

			// summary
			sprintf(templn, "</b> &nbsp; <font color=\"#C0C0C0\">%'i Dir(s) %'d %s %'d %s</font>%s",
							MAX(dirs - 1, 0), (idx-dirs), STR_FILES,
							dir_size<(_1MB_) ? (int)(dir_size>>10):(int)(dir_size>>20),
							dir_size<(_1MB_) ? STR_KILOBYTE:STR_MEGABYTE, tempstr);

			_concat(&sout, templn);
			///////////
		}
		else
		{

			_concat(&sout,	HTML_BLU_SEPARATOR);
#ifndef LITE_EDITION
			_concat(&sout,	webman_config->sman ? "<a class='tg' " : "<a ");
			_concat(&sout,	"onclick=\"o=lg.style,o.display=(o.display=='none')?'block':'none';\" style=\"cursor:pointer;\"> ");
#endif
			_concat(&sout,	WM_APPNAME " - Simple Web Server" EDITION "</a>");

#ifndef LITE_EDITION
			_concat(&sout, "<div id=\"lg\" style=\"display:none\">");

			_lastgames lastgames;

			if(read_file(LAST_GAMES_BIN, (char*)&lastgames, sizeof(_lastgames), 0))
			{
				u8 n, m;
				for(n = 0; n < MAX_LAST_GAMES; n++)
				{
					if((lastgames.game[n].path[1] != 'n') && not_exists(lastgames.game[n].path)) *lastgames.game[n].path = NULL;
				}

				t_path_entries swap;
				for(n = 0; n < (MAX_LAST_GAMES - 1); n++)
					for(m = (n + 1); m < MAX_LAST_GAMES; m++)
						if(*lastgames.game[n].path == '/' && *lastgames.game[m].path == '/' && (strcasecmp(strrchr(lastgames.game[n].path, '/'), strrchr(lastgames.game[m].path, '/')) > 0))
						{
							swap = lastgames.game[n];
							lastgames.game[n] = lastgames.game[m];
							lastgames.game[m] = swap;
						}

				char ftype[5];
				for(n = 0; n < MAX_LAST_GAMES; n++)
				{
					if(*lastgames.game[n].path)
					{
						char *name = strrchr(lastgames.game[n].path, '/'); if(!name) name = lastgames.game[n].path; else name++;

						set_file_type(lastgames.game[n].path, name, ftype);

						sprintf(tempstr, "<a class=\"%c%s\" href=\"/mount.ps3%s\">%s</a><br>", (isDir(lastgames.game[n].path) || strstr(lastgames.game[n].path, "/GAME")) ? 'd' : 'w', ftype, lastgames.game[n].path, name);
						_concat(&sout, tempstr);
					}
				}
			}
			_concat(&sout, "</div>");
#endif
		}
	}
	return true;
}

#undef ext5
#undef ext8
#undef ext13
