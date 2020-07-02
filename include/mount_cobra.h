// PS3GAME.INI flag or [gd] tag -> auto-map /dev_hdd0/game to dev_usbxxx/GAMEI for external game data / patches
// mount hdd0   -> /GAMES /GAMEZ /PS3ISO /PSXISO /PSXGAMES /PS2ISO /PSPISO /BDISO /DVDISO
// mount usb*   -> /GAMES /GAMEZ /PS3ISO /PSXISO /PSXGAMES /PSPISO /BDISO /DVDISO
//       iso.*  -> support split-iso
// mount ntfs   -> .ntfs[PS3ISO] .ntfs[PSXISO] .ntfs[PSPISO] .ntfs[BDISO] .ntfs[DVDISO] .ntfs[BDFILE]
//          ps2 -> are cached to hdd0
//       psxiso -> use rawseciso plugin by default on NTFS due multi-disc support
// mount net    -> /net0/PS3ISO /net0/PSXISO /net0/PSXGAMES /net0/PSPISO /net0/BDISO /net0/DVDISO /net0/GAMES /net0/PKG
//          ps2 -> are cached to hdd0
//    Dump with /copy.ps3/net0/***PS3***/GAMES/BLES12345  -> /dev_hdd0/PS3ISO/BLES12345.iso
//    Dump with /copy.ps3/net0/***DVD***/folder           -> /dev_hdd0/DVDISO/folder.iso

#ifdef COBRA_ONLY
	{
		// --------------------------------------------
		// auto-map /dev_hdd0/game to dev_usbxxx/GAMEI
		// --------------------------------------------
		 #ifdef EXT_GDATA
		{
			// auto-enable external GD
			if(action != MOUNT_NORMAL) ;

			else if(strstr(_path, "/GAME"))
			{
				char extgdfile[strlen(_path) + 24], *extgdini = extgdfile;
				sprintf(extgdfile, "%s/PS3_GAME/PS3GAME.INI", _path); check_ps3_game(extgdfile);
				if(read_file(extgdfile, extgdini, 12, 0))
				{
					if((extgd == 0) &&  (extgdini[10] & (1<<1))) set_gamedata_status(1, false); else
					if((extgd == 1) && !(extgdini[10] & (1<<1))) set_gamedata_status(0, false);
				}
				else if(extgd) set_gamedata_status(0, false);
			}
			else if((strstr(_path, "PS3ISO") != NULL) && (strstr(_path, "[gd]") != NULL))
			{
				if(extgd == 0) set_gamedata_status(1, false);
			}
			else if(extgd)
				set_gamedata_status(0, false);
		}
		 #endif //#ifdef EXT_GDATA

		// ------------
		// get /net id
		// ------------

		if(islike(_path, "/net")) netid = _path[4];

	mount_again:

		// ---------------------
		// unmount current game
		// ---------------------

		do_umount(false);

		// ----------
		// mount iso
		// ----------
		if(!isDir(_path))
		{
			if( strstr(_path, "/PSXISO") || strstr(_path, "/PSXGAMES") || !extcmp(_path, ".ntfs[PSXISO]", 13) || mount_unk == EMU_PSX) {mount_unk = EMU_PSX; select_ps1emu(_path);}

			//if(_next || _prev)
				sys_ppu_thread_sleep(1);
			//else
			//	sys_ppu_thread_usleep(50000);


			// --------------
			// get ISO parts
			// --------------

			u8 iso_parts = 1;
			char templn[MAX_LINE_LEN];

			size_t path_len = sprintf(templn, "%s", _path);

			CD_SECTOR_SIZE_2352 = 2352;

			if(!extcasecmp(_path, ".iso.0", 6))
			{
				for(; iso_parts < MAX_ISO_PARTS; iso_parts++)
				{
					sprintf(templn + path_len - 2, ".%i", iso_parts);
					if(not_exists(templn)) break;
				}
				templn[path_len - 2] = NULL;
			}

			char *cobra_iso_list[iso_parts], iso_list[iso_parts][path_len + 2];

			sprintf(iso_list[0], "%s", _path);
			cobra_iso_list[0] = (char*)iso_list[0];

			for(u8 n = 1; n < iso_parts; n++)
			{
				sprintf(iso_list[n], "%s.%i", templn, n);
				cobra_iso_list[n] = (char*)iso_list[n];
			}

			// -----------------------
			// mount extFAT / NTFS ISO
			// -----------------------

			char *ntfs_ext = strstr(_path, ".ntfs[");
			if(ntfs_ext)
			{
	#ifdef USE_INTERNAL_PLUGIN
				// ------------------------------------------------------------------------------------------------------------
				// launch ntfs psx & isos tagged [raw] with external rawseciso sprx (if available) (due support for multi PSX)
				// ------------------------------------------------------------------------------------------------------------
				if(islike(ntfs_ext, ".ntfs[PSXISO]") || (!strstr(_path, "[raw]")))
	#endif
				{
					u8 n;
					const char *rawseciso_sprx[4] = { WM_RES_PATH "/raw_iso.sprx",
													  VSH_MODULE_DIR "raw_iso.sprx",
													  _HDD0_GAME_DIR "IRISMAN00/sprx_iso",
													  WMTMP "/res/sman.ntf" };

					for(n = 0; n < 4; n++)
						if(file_exists(rawseciso_sprx[n])) break;

					if(n < 4)
					{
						cellFsChmod(_path, MODE);

						sys_addr_t addr = 0;
						if(sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &addr) == CELL_OK)
						{
							char *rawseciso_data = (char *)addr;
							u64 msiz = read_file(_path, rawseciso_data, _64KB_, 0);
							if(msiz > sizeof(rawseciso_args))
							{
								ret = (cobra_load_vsh_plugin(0, (char*)rawseciso_sprx[n], rawseciso_data, msiz) == CELL_OK);
								sys_ppu_thread_sleep(1);
							}
							else
								ret = false;

							sys_memory_free(addr);

	#ifdef USE_INTERNAL_PLUGIN
							if(ret) goto mounted_ntfs;
	#endif
						}
					}
				}

	#ifdef USE_INTERNAL_PLUGIN
				sys_addr_t addr = 0;
				if(sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &addr) == CELL_OK)
				{
					char *rawseciso_data = (char*)addr;
					if(read_file(_path, rawseciso_data, _64KB_, 0) > sizeof(rawseciso_args))
					{
						sys_ppu_thread_create(&thread_id_ntfs, rawseciso_thread, (u64)addr, THREAD_PRIO, THREAD_STACK_SIZE_NTFS_ISO, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_NTFS);

						sys_ppu_thread_sleep(1);
					}
					else
						ret = false;

					sys_memory_free(addr);
				}
	#endif //#ifdef USE_INTERNAL_PLUGIN

		mounted_ntfs:

				if(ret)
				{
					wait_for("/dev_bdvd", 3);

					if(IS(ntfs_ext, ".ntfs[PS3ISO]"))
					{
						get_name(templn, _path, NO_EXT);
						cache_icon0_and_param_sfo(templn);
	#ifdef FIX_GAME
						fix_game(_path, title_id, webman_config->fixgame);
	#endif
					}

#ifdef PKG_HANDLER
					if(IS(ntfs_ext, ".ntfs[BDFILE]") && islike(ntfs_ext - 4, ".pkg"))
					{
						installPKG_all((char*)"/dev_bdvd", false);
						goto exit_mount;
					}
#endif
					// cache PS2ISO or PSPISO to HDD0
					bool is_ps2 = IS(ntfs_ext, ".ntfs[PS2ISO]");
					bool is_psp = IS(ntfs_ext, ".ntfs[PSPISO]");

					if(is_psp || is_ps2)
					{
						int fd;

						if(cellFsOpendir("/dev_bdvd", &fd) == CELL_FS_SUCCEEDED)
						{
							CellFsDirent entry; u64 read_e;

							while((cellFsReaddir(fd, &entry, &read_e) == CELL_FS_SUCCEEDED) && (read_e > 0))
							{
								if(entry.d_name[0] != '.') break;
							}
							cellFsClosedir(fd);

							if(entry.d_name[0] == NULL) goto exit_mount;

							if(is_psp)
							{
								*ntfs_ext = 0;
								sprintf(templn, "%s", _path);
								*ntfs_ext = '.';
							}

							sprintf(_path, "/dev_bdvd/%s", entry.d_name);

							if(not_exists(_path)) goto exit_mount;

							if(is_psp)
							{
								ret = (cobra_set_psp_umd(_path, (char*)templn, (char*)"/dev_hdd0/tmp/wm_icons/psp_icon.png") == CELL_FS_SUCCEEDED);
								//goto copy_pspiso_to_hdd0;
							}
							else //if(is_ps2)
								goto copy_ps2iso_to_hdd0;
						}
					}
				}
				goto exit_mount;
			}

	 #ifdef NET_SUPPORT

			// -----------------------
			// mount /net ISO or path
			// -----------------------

			if(netid >= '0' && netid <= '4')
			{
				netiso_svrid = (netid & 0x0F);
				memset((void*)&netiso_args, 0, sizeof(_netiso_args));

				if(is_netsrv_enabled(netiso_svrid) == false)
				{
					ret = false;
					goto exit_mount;
				}

				if(_path[5] == NULL) strcat(_path, "/.");

				char *netpath = _path + 5, *pkg_slash = NULL;

				size_t len = sprintf(netiso_args.path, "%s", netpath);

				bool is_iso = false;
				char *ext = strrchr(netpath, '.'); if(ext) is_iso = (strcasestr(ISO_EXTENSIONS, ext) != NULL);

				if(islike(netpath, "/PS3ISO") && is_iso) mount_unk = netiso_args.emu_mode = EMU_PS3; else
				if(islike(netpath, "/BDISO" ) && is_iso) mount_unk = netiso_args.emu_mode = EMU_BD;  else
				if(islike(netpath, "/DVDISO") && is_iso) mount_unk = netiso_args.emu_mode = EMU_DVD; else
				if(islike(netpath, "/PS2ISO") && is_iso) goto copy_ps2iso_to_hdd0;                   else
			//	if(islike(netpath, "/PSPISO") && is_iso) goto copy_pspiso_to_hdd0;                   else
				if(islike(netpath, "/PSPISO") && is_iso)
				{
					mount_unk = netiso_args.emu_mode = EMU_BD;
					sprintf(netiso_args.path, "/***DVD***%s", "/PSPISO");
				}
				else if(islike(netpath, "/PSX") && is_iso)
				{
					TrackDef tracks[MAX_TRACKS];
					unsigned int num_tracks = 1;

					int ns = connect_to_remote_server(netiso_svrid);
					if(ns >= 0)
					{
						cellFsUnlink(TEMP_NET_PSXCUE);
						const char *cue_ext[4] = {".cue", ".ccd", ".CUE", ".CCD"};
						for(u8 e = 0; e < 4; e++)
						{
							strcpy(netiso_args.path + len - 4, cue_ext[e]);
							if(copy_net_file(TEMP_NET_PSXCUE, netiso_args.path, ns, _8KB_) == CELL_OK) break;
						}
						sclose(&ns);

						if(file_exists(TEMP_NET_PSXCUE))
						{
							char *cue_buf = malloc(_4KB_);
							if(cue_buf)
							{
								s64 cue_size = read_file(TEMP_NET_PSXCUE, cue_buf, _4KB_, 0);
								cellFsUnlink(TEMP_NET_PSXCUE);

								num_tracks = parse_cue(templn, cue_buf, cue_size, tracks);
								free(cue_buf);
							}
						}
					}

					mount_unk = netiso_args.emu_mode = EMU_PSX;
					netiso_args.num_tracks = num_tracks;
					sprintf(netiso_args.path, "%s", netpath);

					ScsiTrackDescriptor *scsi_tracks = (ScsiTrackDescriptor *)netiso_args.tracks;

					if(num_tracks <= 1)
					{
						scsi_tracks[0].adr_control = 0x14;
						scsi_tracks[0].track_number = 1;
						scsi_tracks[0].track_start_addr = 0;
					}

					else
					{
						for(unsigned int t = 0; t < num_tracks; t++)
						{
							scsi_tracks[t].adr_control = (tracks[t].is_audio) ? 0x10 : 0x14;
							scsi_tracks[t].track_number = t + 1;
							scsi_tracks[t].track_start_addr = tracks[t].lba;
						}
					}

				}
				else if((islike(netpath, "/GAMES") || islike(netpath, "/GAMEZ")) && (strstr(netpath + 5, "/") != NULL))
				{
					mount_unk = netiso_args.emu_mode = EMU_PS3;
					sprintf(netiso_args.path, "/***PS3***%s", netpath);
				}
				else if(islike(netpath, "/ROMS/"))
				{
					netiso_args.emu_mode = EMU_BD;
					mount_unk = EMU_ROMS;

					sprintf(netiso_args.path, "/***DVD***%s", "/ROMS");

					sprintf(templn, "/dev_bdvd/%s", netpath + 6);
					save_file(PKGLAUNCH_DIR "/USRDIR/launch.txt", templn, SAVE_ALL);
					copy_rom_media(templn);
				}
				else
				{
					mount_unk = netiso_args.emu_mode = EMU_DVD;
					if(!extcasecmp(netpath, ".pkg", 4))
					{
						pkg_slash = strrchr(netpath, '/'); if(pkg_slash) *pkg_slash = NULL;
					}
					if(!extcasecmp(netpath, ".iso", 4) || !extcasecmp(netpath, ".mdf", 4) || !extcasecmp(netpath, ".img", 4) || !extcasecmp(netpath, ".bin", 4)) ;
					else
						sprintf(netiso_args.path, "/***DVD***%s", netpath);
				}

				sprintf(netiso_args.server, "%s", webman_config->neth[netiso_svrid]);
				netiso_args.port = webman_config->netp[netiso_svrid];


				u8 n;
				const char *netiso_sprx[3] = { WM_RES_PATH "/netiso.sprx",
											   VSH_MODULE_DIR "netiso.sprx",
											   WMTMP "/res/sman.net"};

				for(n = 0; n < 3; n++)
					if(file_exists(netiso_sprx[n])) break;

				if((n < 3) && (!strstr(_path, "[net]")))
				{
					ret = (cobra_load_vsh_plugin(0, (char*)netiso_sprx[n], &netiso_args, sizeof(_netiso_args)) == CELL_OK);
				}
#ifdef USE_INTERNAL_PLUGIN
				else
				sys_ppu_thread_create(&thread_id_net, netiso_thread, 0, THREAD_PRIO, THREAD_STACK_SIZE_NET_ISO, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_NET);
#endif
				if(ret && (netiso_args.emu_mode == EMU_PS3))
				{
					wait_for("/dev_bdvd", 15);

					get_name(templn, _path, GET_WMTMP);
					cache_icon0_and_param_sfo(templn);

					#ifdef FIX_GAME
					fix_game(_path, title_id, webman_config->fixgame);
					#endif
				}

				if(ret && is_iso && islike(netpath, "/PSPISO") )
				{
					sprintf(templn, "/dev_bdvd/%s", netpath + 8);
					sprintf(_path,  "/dev_bdvd/%s", netpath + 8);

					sys_ppu_thread_sleep(1);

					ret = (cobra_set_psp_umd(_path, templn, (char*)"/dev_hdd0/tmp/wm_icons/psp_icon.png") == CELL_FS_SUCCEEDED);
				}

				if(ret && islike(netpath, "/ROMS/"))
				{
					wait_for("/dev_bdvd", 15);

					sys_map_path(PKGLAUNCH_DIR, NULL);
					set_app_home (PKGLAUNCH_DIR "/PS3_GAME");

					sys_map_path("/dev_bdvd/PS3_GAME", PKGLAUNCH_DIR "/PS3_GAME");
					sys_map_path("/dev_bdvd/PS3_GAME/USRDIR/cores", isDir( RETROARCH_DIR1 ) ?
																			RETROARCH_DIR1 "/USRDIR/cores" :
																			RETROARCH_DIR2 "/USRDIR/cores" );
				}
#ifdef PKG_HANDLER
				if(ret && (pkg_slash != NULL))
				{
					sprintf(_path, "/dev_bdvd/%s", pkg_slash + 1);
					installPKG(_path, templn);
				}
#endif
				goto exit_mount;
			}

	#endif // #ifdef NET_SUPPORT

			// ------------------------------------------------------------------
			// mount PS3ISO / PSPISO / PS2ISO / DVDISO / BDISO stored on hdd0/usb
			// ------------------------------------------------------------------
			{
				ret = file_exists(cobra_iso_list[0]); if(!ret) goto exit_mount;

				// --------------
				// mount PS3 ISO
				// --------------

				if(strstr(_path, "/PS3ISO") || mount_unk == EMU_PS3)
				{
	#ifdef FIX_GAME
					if(webman_config->fixgame != FIX_GAME_DISABLED)
					{
						fix_in_progress=true; fix_aborted = false;
						fix_iso(_path, 0x100000UL, true);
						fix_in_progress=false;
					}
	#endif //#ifdef FIX_GAME

					mount_unk = EMU_PS3;

					cobra_mount_ps3_disc_image(cobra_iso_list, iso_parts);
					sys_ppu_thread_usleep(2500);
					cobra_send_fake_disc_insert_event();

					{
						get_name(templn, _path, GET_WMTMP);
						cache_icon0_and_param_sfo(templn);
					}
				}

				// --------------
				// mount PSP ISO
				// --------------

				else if(strstr(_path, "/PSPISO") || strstr(_path, "/ISO/") || mount_unk == EMU_PSP)
				{
					if(netid)
					{
	//copy_pspiso_to_hdd0:
						cache_file_to_hdd(_path, iso_list[0], "/PSPISO", templn);
					}

					mount_unk = EMU_PSP;

					cobra_unset_psp_umd();

					if(file_exists(iso_list[0]))
					{
						sprintf(templn, "%s.MINIS.EDAT", iso_list[0]);
						if(file_exists(templn))
						{
							if(isDir(PSP_LAUNCHER_MINIS))		sys_map_path(PSP_LAUNCHER_MINIS     "/USRDIR/MINIS.EDAT", templn);
							if(isDir(PSP_LAUNCHER_REMASTERS))	sys_map_path(PSP_LAUNCHER_REMASTERS "/USRDIR/MINIS.EDAT", templn);
						}

						sprintf(templn, "%s.MINIS2.EDAT", iso_list[0]);
						if(file_exists(templn))
						{
							if(isDir(PSP_LAUNCHER_REMASTERS)) sys_map_path(PSP_LAUNCHER_MINIS "/USRDIR/MINIS2.EDAT", templn);
						}

						int result = cobra_set_psp_umd(iso_list[0], NULL, (char*)"/dev_hdd0/tmp/wm_icons/psp_icon.png");

						if(result) ret = false;
					}
					else
						ret = false;
				}

				// --------------
				// mount PS2 ISO
				// --------------

				else if(strstr(_path, "/PS2ISO") || mount_unk == EMU_PS2_DVD)
				{
					if(!islike(_path, "/dev_hdd0"))
					{
	copy_ps2iso_to_hdd0:
						cache_file_to_hdd(_path, iso_list[0], "/PS2ISO", templn);
					}

					webman_config->ps2emu = pad_select_netemu(_path, webman_config->ps2emu);

					enable_ps2netemu_cobra(webman_config->ps2emu);

					mount_unk = EMU_PS2_DVD;

					if(file_exists(iso_list[0]))
					{
						cellFsUnlink(TMP_DIR "/loadoptical"); //Cobra 8.x

						#ifndef LITE_EDITION
						// Auto-copy CONFIG from ManaGunZ
						sprintf(templn, "%s", _path);
						sprintf(_path, "%s.CONFIG", iso_list[0]);
						if(!webman_config->ps2config && not_exists(_path))
						{
							mount_ps_disc_image(templn, cobra_iso_list, iso_parts, EMU_PS2_DVD);
							sys_ppu_thread_usleep(2500);
							cobra_send_fake_disc_insert_event();

							wait_for("/dev_bdvd", 2);

							int fd;
							if(cellFsOpendir("/dev_bdvd", &fd) == CELL_FS_SUCCEEDED)
							{
								CellFsDirectoryEntry dir; u32 read_e;
								char *entry_name = dir.entry_name.d_name;

								while(working && (!cellFsGetDirectoryEntries(fd, &dir, sizeof(dir), &read_e) && read_e))
								{
									if( ((entry_name[0] | 0x20) == 's') && (dir.entry_name.d_namlen == 11) )
									{
										char *tempID = to_upper(entry_name);
										if (
											(tempID[1] == 'L' || tempID[1] == 'C') &&
											(tempID[2] == 'U' || tempID[2] == 'E' || tempID[2] == 'P' || tempID[2] == 'A' || tempID[2] == 'H' || tempID[2] == 'J' || tempID[2] == 'K') &&
											(tempID[3] == 'S' || tempID[3] == 'M' || tempID[3] == 'J' || tempID[3] == 'A') &&
											(tempID[4] == '_' && tempID[8] == '.') &&
											(tempID[5] >= '0' && tempID[5] <= '9') &&
											(tempID[6] >= '0' && tempID[6] <= '9') &&
											(tempID[7] >= '0' && tempID[7] <= '9') &&
											(tempID[9] >= '0' && tempID[9] <= '9')
										   )
										{
											char temp[STD_PATH_LEN];
											sprintf(temp, "%s/%s.CONFIG", PS2CONFIG_PATH, tempID);
											if(file_exists(temp))
												_file_copy(temp, _path);
											else
											{
												char config_path[4][8] = {"CUSTOM", "NET", "GX", "SOFT"};
												for(u8 i = 0; i < 4; i++)
												{
													sprintf(temp, "%s/CONFIG/%s/%s.CONFIG", PS2CONFIG_PATH, config_path[i], tempID);
													if(file_exists(temp)) {_file_copy(temp, _path); break;}
													sprintf(temp, "%s/sys/CONFIG/%s/%s.CONFIG", MANAGUNZ, config_path[i], tempID);
													if(file_exists(temp)) {_file_copy(temp, _path); break;}
													sprintf(temp, "%s/CONFIG/%s/%s.CONFIG", WM_INSTALLER_PATH, config_path[i], tempID);
													if(file_exists(temp)) {_file_copy(temp, _path); break;}
												}
											}
											break;
										}
									}
								}
								cellFsClosedir(fd);
							}

							if(file_exists(_path)) {do_umount(false); wait_path("/dev_bdvd", 5, false);} else mount_unk = EMU_PS2_CD; // prevent mount ISO again if CONFIG was not created
						}
						#endif

						// mount PS2 ISO
						if(mount_unk == EMU_PS2_DVD)
							mount_ps_disc_image(templn, cobra_iso_list, iso_parts, EMU_PS2_DVD);

						// set fan to PS2 mode (constant fan speed)
						if(webman_config->fanc) restore_fan(SET_PS2_MODE); //set_fan_speed( ((webman_config->ps2temp*255)/100), 0);

						// create "wm_noscan" to avoid re-scan of XML returning to XMB from PS2
						save_file(WMNOSCAN, NULL, SAVE_ALL);

						if(mount_unk == EMU_PS2_CD) goto exit_mount; // don't call cobra_send_fake_disc_insert_event again
					}
					else
						ret = false;
				}

				// --------------
				// mount PSX ISO
				// --------------

				else if(strstr(_path, "/PSXISO") || strstr(_path, "/PSXGAMES") || mount_unk == EMU_PSX)
				{
					ret = mount_ps_disc_image(_path, cobra_iso_list, 1, EMU_PSX);
				}

				// -------------------
				// mount DVD / BD ISO
				// ------------------

				else if(strstr(_path, "/DVDISO") || mount_unk == EMU_DVD)
					cobra_mount_dvd_disc_image(cobra_iso_list, iso_parts);
				else if(strstr(_path, "/BDISO")  || mount_unk == EMU_BD)
					cobra_mount_bd_disc_image(cobra_iso_list, iso_parts);
				else
				{
					// mount iso as data
					cobra_mount_bd_disc_image(cobra_iso_list, iso_parts);
					sys_ppu_thread_usleep(2500);
					cobra_send_fake_disc_insert_event();

					if(!mount_unk)
					{
						wait_for("/dev_bdvd", 5);

						// re-mount using proper media type
						if(isDir("/dev_bdvd/PS3_GAME")) mount_unk = EMU_PS3; else
						if(isDir("/dev_bdvd/PS3_GM01")) mount_unk = EMU_PS3; else
						if(isDir("/dev_bdvd/VIDEO_TS")) mount_unk = EMU_DVD; else
						if(file_exists("/dev_bdvd/SYSTEM.CNF") || strcasestr(_path, "PS2")) mount_unk = EMU_PS2_DVD; else
						if(strcasestr(_path, "PSP")!=NULL && !extcasecmp(_path, ".iso", 4)) mount_unk = EMU_PSP; else
						if(!isDir("/dev_bdvd")) mount_unk = EMU_PSX; // failed to mount PSX CD as bd disc

						if(mount_unk) goto mount_again;
					}

					mount_unk = EMU_BD;
				}

				// ----------------------------------------------------------------------------------------
				// send_fake_disc_insert_event for mounted ISOs (PS3ISO/PS2ISO/PSXISO/PSPISO/BDISO/DVDISO)
				// ----------------------------------------------------------------------------------------
				sys_ppu_thread_usleep(2500);
				cobra_send_fake_disc_insert_event();

				//goto exit_mount;
			}
		}

		// ------------------
		// mount folder (JB)
		// ------------------

		else
		{
			int special_mode = 0;

		#ifdef EXTRA_FEAT
			CellPadData pad_data = pad_read();

			if(pad_data.len > 0 && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_SELECT)) special_mode = true; //mount also app_home / eject disc

			if(special_mode) eject_insert(1, 0);
		#endif

			// -- fix game & get TitleID from PARAM.SFO
		#ifdef FIX_GAME
			fix_game(_path, title_id, webman_config->fixgame);
		#else
			char filename[STD_PATH_LEN + 20];

			sprintf(filename, "%s/PS3_GAME/PARAM.SFO", _path);
			getTitleID(filename, title_id, GET_TITLE_ID_ONLY);
		#endif
			// ----

			// -- reset USB bus
			if(!webman_config->bus)
			{
				if(islike(_path, "/dev_usb") && isDir(_path))
				{
					reset_usb_ports(_path);
				}
			}

			// -- mount game folder
			if((*title_id > ' ') && (title_id[8] >= '0'))
				cobra_map_game(_path, title_id, &special_mode);
			else
				cobra_map_game(_path, "TEST00000", &special_mode);
		}

		//goto exit_mount;
	}
#endif
