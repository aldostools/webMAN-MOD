// mount NPDRM game  -> /dev_hdd0/game/<titleid>
// mount GAMEI game  -> /dev_usb*/GAMEI/<titleid>
// mount ROMS game   -> /dev_***/ROMS/ or *.SELF or *.ext (.ZIP.GBA.NES.UNIF.GB.GBC.DMG.MD.SMD.GEN.SMS.GG.SG.IOS.FLAC.NGP.NGC.PCE.SGX.VB.VBOY.WS.WSC.FDS.EXE.WAD.IWAD.SMC.FIG.SFC.GD3.GD7.DX2.BSX.SWC.A26.PAK.LUA.ADF.DMS.FDI.IPF.UAE.A78.MGW.LNX.VEC.J64.JAG.PRG.XFD.XEX)
// mount PS2 Classic -> *.BIN.ENC

#ifdef COBRA_ONLY
	// ------------------
	// mount GAMEI game
	// ------------------
 #ifdef PKG_LAUNCHER
	{
		char *pos = strstr(_path, "/GAMEI/");
		if(pos)
		{
			sys_map_path(PKGLAUNCH_DIR, _path0);
			get_value(map_title_id, pos + 7, TITLE_ID_LEN);
			sprintf(_path, "/dev_hdd0/game/%s", map_title_id);
			sys_map_path(_path, _path0);

			mount_unk = EMU_GAMEI;
			goto exit_mount;
		}
	}
 #endif

	// ------------------
	// mount NPDRM game
	// ------------------
	if(islike(_path, HDD0_GAME_DIR))
	{
		set_apphome(_path);

		char col[8], seg[16]; *col = NULL, *seg = NULL; ret = isDir(_path);
		if(is_app_home_onxmb()) {mount_unk = APP_GAME; launch_disc(col, seg, true); ret = true;}

		mount_unk = EMU_MAX;
		goto exit_mount;
	}

	// ------------------
	// mount ROMS game
	// ------------------
 #ifdef MOUNT_ROMS
	if(isDir(PKGLAUNCH_DIR))
	{
		int plen = strlen(_path) - 4;
		if(plen < 0) plen = 0;
		else if(_path[plen + 2] == '.') plen+=2;
		else if(_path[plen + 1] == '.') plen++;

		if((strstr(_path, "/ROMS/") != NULL) || (strcasestr(_path, ".SELF") != NULL) || (strcasestr(ROMS_EXTENSIONS, _path + plen) != NULL))
		{
			do_umount(false);

			sys_map_path(PKGLAUNCH_DIR, NULL);
			sys_map_path(PKGLAUNCH_DIR "/PS3_GAME/USRDIR/cores", RETROARCH_DIR "/USRDIR/cores");

			cobra_map_game(PKGLAUNCH_DIR, "PKGLAUNCH", 0);

			save_file(PKGLAUNCH_DIR "/USRDIR/launch.txt", _path, 0);
			mount_unk = EMU_ROMS;
			goto mounting_done; //goto exit_mount;
		}
	}
 #endif // #ifdef MOUNT_ROMS

#endif // #ifdef COBRA_ONLY

	// ------------------
	// mount PS2 Classic
	// ------------------
	if(!extcmp(_path, ".BIN.ENC", 8))
	{
		char temp[STD_PATH_LEN + 16];

		if(isDir(PS2_CLASSIC_PLACEHOLDER))
		{
			copy_in_progress = true, copied_count = 0;

			sprintf(temp, "PS2 Classic\n%s", strrchr(_path, '/') + 1);
			show_msg(temp);

 #ifndef LITE_EDITION
			if(c_firmware >= 4.65f)
			{   // Auto create "classic_ps2 flag" for PS2 Classic (.BIN.ENC) on rebug 4.65.2
				do_umount(false);
				enable_classic_ps2_mode();
			}
 #endif
			cellFsUnlink(PS2_CLASSIC_ISO_CONFIG);
			cellFsUnlink(PS2_CLASSIC_ISO_PATH);

			if(file_copy(_path, (char*)PS2_CLASSIC_ISO_PATH, COPY_WHOLE_FILE) >= 0)
			{
				copy_ps2config(temp, _path);

				copy_ps2savedata(temp, _path);

				copy_ps2icon(temp, _path);

				if(webman_config->fanc) restore_fan(SET_PS2_MODE); //set_fan_speed( ((webman_config->ps2temp*255)/100), 0);

				// create "wm_noscan" to avoid re-scan of XML returning to XMB from PS2
				save_file(WMNOSCAN, NULL, 0); ret = true;

				sprintf(temp, "\"%s\" %s", strrchr(_path, '/') + 1, STR_LOADED2);
			}
			else
				{sprintf(temp, "PS2 Classic\n%s", STR_ERROR); ret = false;}

			copy_in_progress = false;
		}
		else
		{
			sprintf(temp, "PS2 Classic Placeholder %s", STR_NOTFOUND);
			ret = false;
		}

		show_msg(temp);

		goto exit_mount;
	}

 #ifndef LITE_EDITION
	if((c_firmware >= 4.65f) && strstr(_path, "/PS2ISO")!=NULL)
	{   // Auto remove "classic_ps2" flag for PS2 ISOs on rebug 4.65.2
		disable_classic_ps2_mode();
	}
 #endif
