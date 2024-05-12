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
if(!cobra_version) {ret = false; goto finish;}
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
		else if(strstr(_path, "PS3ISO") && strstr(_path, "[gd]"))
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
		char templn[MAX_LINE_LEN];

		#ifdef MOUNT_PNG
		bool is_PNG = is_ext(_path, ".png");

		if(is_PNG && (mount_unk == EMU_OFF)) {read_file(_path, templn, 1, 0xFFE0); mount_unk = *templn;}
		#endif

		if( strstr(_path, "/PSXISO") || strstr(_path, "/PSXGAMES") || !extcmp(_path, ".ntfs[PSXISO]", 13) || (mount_unk == EMU_PSX)) {mount_unk = EMU_PSX; select_ps1emu(_path);}

		sys_ppu_thread_sleep(1);

		// --------------
		// get ISO parts
		// --------------

		u8 iso_parts = 1;

		size_t path_len = sprintf(templn, "%s", _path);

		CD_SECTOR_SIZE_2352 = 2352;

		if(is_iso_0(_path))
		{
			// count iso_parts
			for(; iso_parts < MAX_ISO_PARTS; iso_parts++)
			{
				#ifdef MOUNT_PNG
				if(is_PNG)
					sprintf(templn + path_len - 6, ".%i.PNG", iso_parts);
				else
				#endif
					sprintf(templn + path_len - 2, ".%i", iso_parts);

				if(not_exists(templn)) break;
			}
			#ifdef MOUNT_PNG
			if(is_PNG)
				templn[path_len - 6] = '\0'; // remove .0.PNG
			else
			#endif
				templn[path_len - 2] = '\0'; // remove .0
		}

		char *cobra_iso_list[iso_parts], iso_list[iso_parts][path_len + 2];

		change_cue2iso(_path);
		strcpy(iso_list[0], _path);
		cobra_iso_list[0] = (char*)iso_list[0];

		for(u8 n = 1; n < iso_parts; n++)
		{
			sprintf(iso_list[n], "%s.%i", templn, n);
			#ifdef MOUNT_PNG
			if(is_PNG) strcat(iso_list[n], ".PNG"); // .iso.#.PNG
			#endif
			cobra_iso_list[n] = (char*)iso_list[n];
		}

		#include "mount_rawiso.h"
		#include "mount_net.h"

		// ------------------------------------------------------------------
		// mount PS3ISO / PSPISO / PS2ISO / DVDISO / BDISO stored on hdd0/usb
		// ------------------------------------------------------------------
		{
			ret = file_exists(iso_list[0]); if(!ret) goto exit_mount;

			// --------------
			// mount PS3 ISO
			// --------------

			if(strstr(_path, "/PS3ISO") || (mount_unk == EMU_PS3))
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

			else if(strstr(_path, "/PSPISO") || strstr(_path, "/ISO/") || (mount_unk == EMU_PSP))
			{
				if(netid)
				{
					cache_file_to_hdd(_path, iso_list[0], "/PSPISO", templn);
				}

				mount_unk = EMU_PSP;

				cobra_unset_psp_umd();

				if(file_exists(iso_list[0]))
				{
					int edat = 0;

					sprintf(templn, "%s.MINIS.EDAT", iso_list[0]);
					if(file_exists(templn))
					{
						if(isDir(PSP_LAUNCHER_MINIS))
						{
							sprintf(iso_list[1], "/%s%s", PSP_LAUNCHER_MINIS, "/USRDIR/MINIS.EDAT");
							force_copy(templn, iso_list[1]);
							edat = read_file(iso_list[1], templn, 4, 0);
						}

						if(isDir(PSP_LAUNCHER_REMASTERS))
						{
							sprintf(iso_list[1], "/%s%s", PSP_LAUNCHER_REMASTERS, "/USRDIR/MINIS.EDAT");
							force_copy(templn, iso_list[1]);
							edat = read_file(iso_list[1], templn, 4, 0);
						}
					}

					sprintf(templn, "%s.MINIS2.EDAT", iso_list[0]);
					if(file_exists(templn))
					{
						if(isDir(PSP_LAUNCHER_REMASTERS))
						{
							sprintf(iso_list[1], "/%s%s", PSP_LAUNCHER_REMASTERS, "/USRDIR/MINIS2.EDAT");
							force_copy(templn, iso_list[1]);
							edat = read_file(iso_list[1], templn, 4, 0);
						}
					}

					#ifndef LITE_EDITION
					// restore original psp_emulator.self (if it's swapped)
					swap_file(PSP_EMU_PATH, "psp_emulator.self", "psp_emulator.self.dec_edat", "psp_emulator.self.original");

					// check if decrypted MINIS.EDAT is detected
					if(edat)
					{
						if(!islike(templn, "NPD") && !payload_ps3hen)
						{
							// install psp_emulator.self with support for decrypted MINIS.EDAT
							if((c_firmware >= 4.82f) && file_exists(WM_RES_PATH "/psp_emulator.self"))
							{
								if(not_exists("/dev_flash/pspemu/psp_emulator.self.dec_edat")
								&& not_exists("/dev_flash/pspemu/psp_emulator.self.original"))
								{
									enable_dev_blind(NULL);
									force_copy(WM_RES_PATH "/psp_emulator.self", (char*)"/dev_blind/pspemu/psp_emulator.self.dec_edat");
								}
							}

							// swap psp_emulator.self if decrypted MINIS.EDAT is detected & psp_emulator.self.dec_edat is installed
							swap_file(PSP_EMU_PATH, "psp_emulator.self", "psp_emulator.self.original", "psp_emulator.self.dec_edat");
							show_msg("MINIS.EDAT is decrypted!");
						}
					}
					#endif

					int result = cobra_set_psp_umd(iso_list[0], NULL, (char*)"/dev_hdd0/tmp/wm_icons/psp_icon.png");

					if(result) ret = false;
				}
				else
					ret = false;
			}

			// --------------
			// mount PS2 ISO
			// --------------

			else if(strstr(_path, "/PS2ISO") || (mount_unk == EMU_PS2_DVD))
			{
				if(!islike(_path, drives[0]))
				{
copy_ps2iso_to_hdd0:
					if(!payload_ps3hen)
						cache_file_to_hdd(_path, iso_list[0], "/PS2ISO", templn);
				}

				if(payload_ps3hen)
				{
					show_error("HEN does not support PS2 ISO\nUse encrypted game.");
					ret = false; goto exit_mount;
				}

				webman_config->ps2emu = pad_select_netemu(_path, webman_config->ps2emu);

				enable_ps2netemu_cobra(webman_config->ps2emu); // 0 = ps2emu, 1 = ps2_netemu

				mount_unk = EMU_PS2_DVD;

				if(file_exists(iso_list[0]))
				{
					cellFsUnlink(TMP_DIR "/loadoptical"); //Cobra 8.x

					#ifndef LITE_EDITION
					// Auto-copy CONFIG from ManaGunZ
					char templn[STD_PATH_LEN];
					sprintf(templn, "%s.CONFIG", iso_list[0]);
					if(!webman_config->ps2config && not_exists(templn))
					{
						mount_ps_disc_image(_path, cobra_iso_list, iso_parts, EMU_PS2_DVD);
						sys_ppu_thread_usleep(2500);
						cobra_send_fake_disc_insert_event();

						bool done = false;
						const char *id = strstr(iso_list[0], " [S"); if(!id) id = strstr(iso_list[0], " (S");
						if(id)
						{
							char title_id[12], game_id[12];
							get_ps_titleid_from_path(title_id, iso_list[0]);
							sprintf(game_id, "%.4s_%.3s.%.2s", title_id, title_id + 4, title_id + 7);
							done = copy_ps2config_iso(game_id, templn);
						}
						if(!done)
						{
							wait_for("/dev_bdvd", 5);

							int fd;
							if(cellFsOpendir("/dev_bdvd", &fd) == CELL_FS_SUCCEEDED)
							{
								CellFsDirectoryEntry dir; u32 read_e;
								char *entry_name = dir.entry_name.d_name;

								while(working && (!cellFsGetDirectoryEntries(fd, &dir, sizeof(dir), &read_e) && read_e))
								{
									if( (LCASE(*entry_name) == 's') && (dir.entry_name.d_namlen == 11) ) // SLUS_123.45
									{
										if(copy_ps2config_iso(entry_name, templn)) break;
									}
								}
								cellFsClosedir(fd);
							}
						}

						if(file_exists(templn)) {do_umount(false); wait_path("/dev_bdvd", 5, false);} else mount_unk = EMU_PS2_CD; // prevent mount ISO again if CONFIG was not created
					}
					#endif

					// mount PS2 ISO
					if(mount_unk == EMU_PS2_DVD)
						mount_ps_disc_image(_path, cobra_iso_list, iso_parts, EMU_PS2_DVD);

					// create "wm_noscan" to avoid re-scan of XML returning to XMB from PS2
					create_file(WM_NOSCAN_FILE);

					if(mount_unk == EMU_PS2_CD) goto exit_mount; // don't call cobra_send_fake_disc_insert_event again
				}
				else
					ret = false;
			}

			// --------------
			// mount PSX ISO
			// --------------

			else if(strstr(_path, "/PSXISO") || strstr(_path, "/PSXGAMES") || (mount_unk == EMU_PSX))
			{
				if(strcasestr(_path, "[bios]")) remap_ps1bios(true);

				mount_unk = EMU_PSX;
				ret = mount_ps_disc_image(_path, cobra_iso_list, 1, EMU_PSX); if(multiCD) check_multipsx = !isDir("/dev_usb000"); // check eject/insert USB000 in mount_on_insert_usb()
			}

			// -------------------
			// mount DVD / BD ISO
			// ------------------

			else if(strstr(_path, "/BDISO")  || (mount_unk == EMU_BD))
			{
				mount_unk = EMU_BD;
				cobra_mount_bd_disc_image(cobra_iso_list, iso_parts);
			}
			else if(strstr(_path, "/DVDISO") || (mount_unk == EMU_DVD))
			{
				mount_unk = EMU_DVD;
				cobra_mount_dvd_disc_image(cobra_iso_list, iso_parts);
			}
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
					if(strcasestr(_path, "PSP")!=NULL && is_ext(_path, ".iso")) mount_unk = EMU_PSP; else
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
		#ifdef EXTRA_FEAT
		// hold SELECT to eject disc
		pad_data = pad_read();
		int special_mode = (pad_data.len > 0 && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_SELECT));

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
		bool is_gameid = BETWEEN('A', *title_id, 'Z') && ISDIGIT(title_id[8]);
		if (!is_gameid)
			sprintf(title_id, "TEST00000");

		cobra_map_game(_path, title_id, mount_app_home | !(webman_config->app_home));

		mount_unk = EMU_PS3;
	}

	//goto exit_mount;
}
#endif
