// mount NPDRM game  -> /dev_hdd0/game/<titleid>
// mount GAMEI game  -> /dev_usb*/GAMEI/<titleid>
// mount ROMS game   -> /dev_***/ROMS/ or *.SELF or *.ext (.ZIP.GBA.NES.UNIF.GB.GBC.DMG.MD.SMD.GEN.SMS.GG.SG.IOS.FLAC.NGP.NGC.PCE.SGX.VB.VBOY.WS.WSC.FDS.EXE.WAD.IWAD.SMC.FIG.SFC.GD3.GD7.DX2.BSX.SWC.A26.PAK.LUA.ADF.DMS.FDI.IPF.UAE.A78.MGW.LNX.VEC.J64.JAG.PRG.XFD.XEX)
// mount PS2 Classic -> *.BIN.ENC

#ifdef COBRA_ONLY
	// ------------------
	// mount GAMEI game
	// ------------------
	#ifdef MOUNT_GAMEI
	{
		char *pos = strstr(_path, "/GAMEI/");
		if(pos && !islike(_path, "/net"))
		{
			pos += 7; // folder is title_id
			replace_char(pos, '/', 0);

			do_umount(false);

			unmap_app_home();
			sys_map_path(APP_HOME_DIR, _path);
			if(isDir(PKGLAUNCH_DIR)) sys_map_path(PKGLAUNCH_DIR, _path);

			strcat(_path, "/PARAM.SFO");
			if(file_exists(_path))
				getTitleID(_path, map_title_id, GET_TITLE_ID_ONLY);
			else
			{
				if(strstr(pos, "_00-") == pos + 16) pos += 7; // folder is content_id (skip EP0000-)
				get_value(map_title_id, pos, TITLE_ID_LEN);
			}

			sprintf(_path, "%s%s", HDD0_GAME_DIR, map_title_id);
			sys_map_path(_path, _path0);

			ret = is_app_dir("/dev_hdd0/game", map_title_id);

			if(ret)
			{
				sys_ppu_thread_sleep(1);
				launch_app_home_icon(webman_config->autoplay);
			}

			mount_unk = EMU_GAMEI;
			goto exit_mount;
		}
	}
	#endif // #ifdef MOUNT_GAMEI

	// ------------------
	// mount NPDRM game
	// ------------------
	#ifdef PKG_LAUNCHER
	const char *ext = get_ext(_path);

	if(isDir(PKGLAUNCH_DIR) && !islike(_path, "/net"))
	{
		if( !extcasecmp(_path, ".self", 5) ||
			strcasestr(ARCHIVE_EXTENSIONS + 4, ext) ||
			(!strstr(_path, "/ROMS") && _IS(ext, ".zip"))
		)
		{
			// patch title name in PARAM.SFO of PKGLAUNCH
			copy_rom_media("/PKG Launcher"); is_busy = false;

			ret = file_exists(_path);
			cobra_map_game(PKGLAUNCH_DIR, PKGLAUNCH_ID, true);
			set_app_home(PKGLAUNCH_DIR);
			save_file(PKGLAUNCH_DIR "/USRDIR/launch.txt", _path, SAVE_ALL);
			sys_ppu_thread_sleep(2);
			if(ret) launch_app_home_icon(true);

			goto mounting_done; //goto exit_mount;
		}
	}
	#endif // #ifdef PKG_LAUNCHER

	if(islike(_path, HDD0_GAME_DIR) || islike(_path, _HDD0_GAME_DIR) )
	{
		ret = isDir(_path);

		if(!ret)
		{
			char *pos = strstr(_path0, "/USRDIR"); if(pos) *pos = NULL;

			if(islike(_path0, HDD0_GAME_DIR))
			{
				sprintf(_path, "%s%s", HDD0_GAME_DIR, _path0 + 15); // use /dev_hdd0/game/
				if(not_exists(_path))
				{
					sprintf(_path, "%s%s", _HDD0_GAME_DIR, _path0 + 15); // use /dev_hdd0//game/ if GAMEI is enabled
				}
			}
			ret = isDir(_path);
		}


		do_umount(false);
		set_app_home(_path);

		sys_ppu_thread_sleep(1);

		if(launch_app_home_icon(webman_config->autoplay | force_ap)) ret = true;

		mount_unk = EMU_MAX;
		goto exit_mount;
	}

	// ------------------
	// mount ROMS game
	// ------------------
	#ifdef MOUNT_ROMS
	if(isDir(PKGLAUNCH_DIR))
	{
		if(islike(_path, "/net")) ; else // mount ROMS in /net module

		if(strstr(_path, "/ROMS") || _IS(ext, ".SELF") || (ROMS_EXTENSIONS && strcasestr(ROMS_EXTENSIONS + 20, ext)))
		{
			do_umount(false);

			// mount PKGLAUNCH as disc
			cobra_map_game(PKGLAUNCH_DIR, PKGLAUNCH_ID, true);

			if(file_exists(RETROARCH_DIR0))
			{
				//sys_map_path(PKGLAUNCH_DIR "/PS3_GAME/USRDIR/cores", RETROARCH_DIR0 "/USRDIR/cores");
				sys_map_path("/dev_bdvd/PS3_GAME/USRDIR/cores", RETROARCH_DIR0 "/USRDIR/cores");
				sys_map_path("/app_home/PS3_GAME/USRDIR/cores", RETROARCH_DIR0 "/USRDIR/cores");

				force_copy(PKGLAUNCH_PS3_GAME "/USRDIR/retroarch.cce", (char*)PKGLAUNCH_PS3_GAME "/USRDIR/retroarch.cfg");
				cellFsUnlink(PKGLAUNCH_PS3_GAME "/USRDIR/retroarch.cce");
			}
			else
			{
				if(file_exists(RETROARCH_DIR1))
				{
					//sys_map_path(PKGLAUNCH_DIR "/PS3_GAME/USRDIR/cores", RETROARCH_DIR1 "/USRDIR/cores");
					sys_map_path("/dev_bdvd/PS3_GAME/USRDIR/cores", RETROARCH_DIR1 "/USRDIR/cores");
					sys_map_path("/app_home/PS3_GAME/USRDIR/cores", RETROARCH_DIR1 "/USRDIR/cores");
				}
				else
				{
					//sys_map_path(PKGLAUNCH_DIR "/PS3_GAME/USRDIR/cores", RETROARCH_DIR2 "/USRDIR/cores");
					sys_map_path("/dev_bdvd/PS3_GAME/USRDIR/cores", RETROARCH_DIR2 "/USRDIR/cores");
					sys_map_path("/app_home/PS3_GAME/USRDIR/cores", RETROARCH_DIR2 "/USRDIR/cores");
				}

				force_copy(PKGLAUNCH_PS3_GAME "/USRDIR/retroarch.bak", (char*)PKGLAUNCH_PS3_GAME "/USRDIR/retroarch.cfg");
				cellFsUnlink(PKGLAUNCH_PS3_GAME "/USRDIR/retroarch.bak");
			}

			// store rom path for PKGLAUNCH
			save_file(PKGLAUNCH_DIR "/USRDIR/launch.txt", _path, SAVE_ALL);
			copy_rom_media(_path);

			mount_unk = EMU_ROMS;

			if(!(webman_config->app_home) && launch_app_home_icon(webman_config->autoplay)) {set_app_home(PKGLAUNCH_DIR); ret = true;}

			ret = isDir("/dev_bdvd/PS3_GAME/USRDIR/cores") ||
				  isDir("/app_home/PS3_GAME/USRDIR/cores");

			goto mounting_done; //goto exit_mount;
		}
	}
	#endif // #ifdef MOUNT_ROMS

#endif // #ifdef COBRA_ONLY

	// ------------------
	// mount PS2 Classic
	// ------------------
	if(is_BIN_ENC(_path))
	{
		char temp[STD_PATH_LEN + 48]; ret = false;

		#ifdef DEX_SUPPORT
		if(!IS_CEX && !(webman_config->minfo & 1))
		{
			sprintf(temp, "Rebug DEX only supports PS2 games encrypted with CEX keys");
			show_msg_with_icon(ICON_EXCLAMATION, temp);
		}
		#endif

		if(isDir(PS2_CLASSIC_PLACEHOLDER))
		{
			#ifndef LITE_EDITION
			if(c_firmware >= 4.65f)
			{   // Auto create "classic_ps2 flag" for PS2 Classic (.BIN.ENC) on rebug 4.65.2
				do_umount(false);
				enable_classic_ps2_mode();
			}
			#endif

			cellFsUnlink(PS2_CLASSIC_ISO_CONFIG);
			cellFsUnlink(PS2_CLASSIC_ISO_PATH);

			if(file_copy(_path, (char*)PS2_CLASSIC_ISO_PATH) >= 0)
			{
				copy_ps2config(temp, _path);

				copy_ps2savedata(temp, _path);

				copy_ps2icon(temp, _path);

				if(webman_config->fanc) {restore_fan(SET_PS2_MODE); ps2_classic_mounted = true;} //set_fan_speed( ((webman_config->ps2temp*255)/100), 0);

				// create "wm_noscan" to avoid re-scan of XML returning to XMB from PS2
				create_file(WM_NOSCAN_FILE);

				sprintf(temp, "\"%s\" %s", get_filename(_path) + 1, STR_LOADED2); ret = true;
			}
			else
				sprintf(temp, "%s %s", _path, STR_NOTFOUND);
		}
		else
		{
			sprintf(temp, "PS2 Classic Placeholder %s", STR_NOTFOUND);
		}

		if(!(webman_config->minfo & 2))
		{
			if(ret)
				show_msg(temp);
			else
				show_error(temp);
		}

		goto exit_mount;
	}

	#ifndef LITE_EDITION
	if((c_firmware >= 4.65f) && strstr(_path, "/PS2ISO"))
	{   // Auto remove "classic_ps2" flag for PS2 ISOs on rebug 4.65.2
		disable_classic_ps2_mode();
	}
	#endif
