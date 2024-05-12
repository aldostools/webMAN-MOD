#ifdef COBRA_ONLY

// -----------------------
// mount /net ISO or path
// -----------------------

if(BETWEEN('0', netid, '4'))
{
	netiso_svrid = (netid & 0x0F);
	_memset((void*)&netiso_args, sizeof(_netiso_args));

	if(_path[5] == '\0') strcat(_path, "/.");

	char *netpath = _path + 5, *pkg_slash = NULL;

	// check remote file exists
	int ns = connect_to_remote_server(netid);
	if(ns >= 0)
	{
		bool found = remote_file_exists(ns, netpath);
		sclose(&ns);

		if(!found)
		{
			ret = false;
			goto exit_mount;
		}
	}

	size_t len = sprintf(netiso_args.path, "%s", netpath);

	const char *ext = strrchr(netpath, '.');
	bool is_iso = ext && strcasestr(ISO_EXTENSIONS, ext);

	// allow mount any file type on ROMS
	if(islike(netpath, "/ROMS")) ; else

	// check mount ISOs
	if(ext)
	{
		if(!is_iso) ext = NULL;
	}

	// check mount folders
	if(!ext)
	{
		int ns = connect_to_remote_server(netid);
		if(ns >= 0)
		{
			bool is_dir = remote_is_dir(ns, netpath);
			sclose(&ns);

			if(!is_dir)
			{
				ret = false;
				goto exit_mount;
			}
		}
	}

	mount_unk = netiso_args.emu_mode = EMU_BD;

	if(strstr(netpath, "/PS3ISO") && is_iso) mount_unk = netiso_args.emu_mode = EMU_PS3; else
	if(strstr(netpath, "/BDISO" ) && is_iso) mount_unk = netiso_args.emu_mode = EMU_BD;  else
	if(strstr(netpath, "/DVDISO") && is_iso) mount_unk = netiso_args.emu_mode = EMU_DVD; else
	if(strstr(netpath, "/PS2ISO") && is_iso) goto copy_ps2iso_to_hdd0;                   else
	if(strstr(netpath, "/PSPISO") && is_iso)
	{
		snprintf(netiso_args.path, sizeof(netiso_args.path), "/***DVD***%s", netpath);
		char *slash = strstr(netiso_args.path, "/PSPISO");
		if(slash) { slash = strchr(slash + 1, '/'); if(slash) *slash = '\0'; }
	}
	else if(strstr(netpath, "/PSX") && is_iso)
	{
		TrackDef tracks[MAX_TRACKS];
		unsigned int num_tracks = 1;

		int ns = connect_to_remote_server(netiso_svrid);
		if(ns >= 0)
		{
			// load cuesheet
			cellFsUnlink(TEMP_NET_PSXCUE);
			{
				// change .cue / .ccd extension to .bin / .iso
				if(is_ext(netpath, ".cue") || is_ext(netpath, ".ccd"))
					for(u8 e = 0; e < 10; e++)
					{
						strcpy(netpath + len - 4, iso_ext[e]);
						if(remote_file_exists(ns, netpath)) break;
					}

				// copy .cue / .ccd to local TEMP_NET_PSXCUE
				for(u8 e = 0; e < 4; e++)
				{
					strcpy(netiso_args.path + len - 4, cue_ext[e]);
					if(copy_net_file(TEMP_NET_PSXCUE, netiso_args.path, ns) == CELL_OK) break;
				}
			}
			sclose(&ns);

			size_t cue_size = file_size(TEMP_NET_PSXCUE);
			if(cue_size > 0x10)
			{
				char *cue_buf = malloc(cue_size);
				if(cue_buf)
				{
					u16 cue_size = read_sfo(TEMP_NET_PSXCUE, cue_buf);
					cellFsUnlink(TEMP_NET_PSXCUE);

					num_tracks = parse_cue(templn, cue_buf, cue_size, tracks);
					free(cue_buf);
				}
			}
		}

		mount_unk = netiso_args.emu_mode = EMU_PSX;
		netiso_args.num_tracks = num_tracks;
		strcpy(netiso_args.path, netpath);

		ScsiTrackDescriptor *scsi_tracks = (ScsiTrackDescriptor *)netiso_args.tracks;

		scsi_tracks[0].adr_control = 0x14;
		scsi_tracks[0].track_number = 1;
		scsi_tracks[0].track_start_addr = 0;

		for(unsigned int t = 1; t < num_tracks; t++)
		{
			scsi_tracks[t].adr_control = 0x10;
			scsi_tracks[t].track_number = t + 1;
			scsi_tracks[t].track_start_addr = tracks[t].lba;
		}
	}
	else if((islike(netpath, "/GAMES") || islike(netpath, "/GAMEZ") || islike(netpath, "/PS3ISO")) && strchr(netpath + 5, '/'))
	{
		mount_unk = netiso_args.emu_mode = EMU_PS3;
		if(!is_iso) sprintf(netiso_args.path, "/***PS3***%s", netpath);
	}
	else if(islike(netpath, "/ROMS"))
	{
		//netiso_args.emu_mode = EMU_BD;
		mount_unk = EMU_ROMS;

		sprintf(netiso_args.path, "/***DVD***%s", "/ROMS");

		sprintf(templn, "/dev_bdvd/%s", netpath + 6);
		save_file(PKGLAUNCH_DIR "/USRDIR/launch.txt", templn, SAVE_ALL);
		copy_rom_media(templn);
	}
	else
	{
		//mount_unk = netiso_args.emu_mode = EMU_BD;
		if(is_ext(netpath, ".pkg"))
		{
			pkg_slash = remove_filename(netpath);
		}
		if(is_iso) ;
		else
			sprintf(netiso_args.path, "/***DVD***%s", netpath);
	}

	strcpy(netiso_args.server, webman_config->neth[netiso_svrid]);
	netiso_args.port = webman_config->netp[netiso_svrid];

	u8 n;
	const char *netiso_sprx[3] = { WM_RES_PATH "/netiso.sprx",
								   VSH_MODULE_DIR "netiso.sprx",
								   WMTMP "/res/sman.net"};

	for(n = 0; n < 3; n++)
		if(file_exists(netiso_sprx[n])) break;
retry_net:
	if((n < 3) && (!strstr(_path, "[net]")))
	{
		ret = (cobra_load_vsh_plugin(0, (char*)netiso_sprx[n], &netiso_args, sizeof(_netiso_args)) == CELL_OK);
	}
#ifdef USE_INTERNAL_NET_PLUGIN
	else
	{
		sys_ppu_thread_create(&thread_id_net, netiso_thread, 0, THREAD_PRIO, THREAD_STACK_SIZE_NET_ISO, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_NET);
	}
#endif
	if(ret)
	{
		if((netiso_args.emu_mode == EMU_BD) || (netiso_args.emu_mode == EMU_DVD))
		{
			wait_for("/dev_bdvd", 15);
			if(isDir("/dev_bdvd/PS3_GAME"))
			{
				do_umount(false);
				if(!is_iso) sprintf(netiso_args.path, "/***PS3***%s", netpath);
				netiso_args.emu_mode = mount_unk = EMU_PS3;
				goto retry_net; // mount as PS3
			}
		}

		const char *pspiso = strstr(netpath, "/PSPISO");

		if(netiso_args.emu_mode == EMU_PS3)
		{
			wait_for("/dev_bdvd", 15);

			get_name(templn, _path, GET_WMTMP);
			cache_icon0_and_param_sfo(templn);

			#ifdef FIX_GAME
			fix_game(_path, title_id, webman_config->fixgame);
			#endif
		}

		else if(is_iso && pspiso)
		{
			mount_unk = EMU_PSP;

			pspiso = strchr(pspiso + 1, '/');
			if(pspiso)
			{
				sprintf(templn,  "%s%s", "/dev_bdvd", pspiso);
				strcpy(_path, templn);

				sys_ppu_thread_sleep(1);

				ret = (cobra_set_psp_umd(_path, templn, (char*)"/dev_hdd0/tmp/wm_icons/psp_icon.png") == CELL_FS_SUCCEEDED);
			}
		}

		else if(islike(netpath, "/ROMS"))
		{
			mount_unk = EMU_ROMS;

			wait_for("/dev_bdvd", 15);

			sys_map_path(PKGLAUNCH_DIR, NULL);
			set_app_home (PKGLAUNCH_PS3_GAME);

			sys_map_path("/dev_bdvd/PS3_GAME", PKGLAUNCH_PS3_GAME);
			sys_map_path("/dev_bdvd/PS3_GAME/USRDIR/cores", isDir( RETROARCH_DIR0 ) ? RETROARCH_DIR0 "/USRDIR/cores" :
															isDir( RETROARCH_DIR1 ) ? RETROARCH_DIR1 "/USRDIR/cores" :
																					  RETROARCH_DIR2 "/USRDIR/cores" );
			launch_app_home_icon(webman_config->autoplay);
		}

		else if(islike(netpath, "/GAMEI/"))
		{
			mount_unk = EMU_PS3;

			wait_for("/dev_bdvd", 15);
			set_app_home("/dev_bdvd"); // sys_map_path(APP_HOME_DIR, "/dev_bdvd");

			sprintf(templn, "%s/PARAM.SFO", "/dev_bdvd");
			getTitleID(templn, map_title_id, GET_TITLE_ID_ONLY);

			if(*map_title_id)
			{
				sprintf(templn, "%s/%s", "/dev_hdd0/game", map_title_id);
				sys_map_path(templn, "/dev_bdvd");
			}

			sys_ppu_thread_sleep(1);
			launch_app_home_icon(webman_config->autoplay);

			mount_unk = EMU_GAMEI;
			goto exit_mount;
		}
#ifdef PKG_HANDLER
		else if(!(webman_config->auto_install_pkg) && pkg_slash)
		{
			installPKG_all("/dev_bdvd", false);
		}
#endif
	}
	goto exit_mount;
}

#endif // #ifdef COBRA_ONLY
