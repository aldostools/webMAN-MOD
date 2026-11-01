static void start_www(u64 conn_s_p)
{
	if(conn_s_p == START_DAEMON || conn_s_p == REFRESH_CONTENT)
	{
		bool do_sleep = true;

		#ifdef WM_PROXY_SPRX
		apply_remaps();
		#endif

		char full_path[MAX_LINE_LEN]; char *msg = full_path;

		if(conn_s_p == START_DAEMON)
		{
			#ifdef FIX_CLOCK
			fix_clock(NULL);
			#endif

			#ifndef ENGLISH_ONLY
			update_language();
			#endif
			make_fb_xml();

			if(profile || !(webman_config->wmstart))
			{
				if(!payload_ps3hen) while(not_xmb()) sys_ppu_thread_sleep(1);

				show_wm_version(msg);
				do_sleep = false;
			}

			if(webman_config->bootd) wait_for("/dev_usb", webman_config->bootd); // wait for any usb

			// is JAP?
			int enter_button = 1;
			xsettings()->GetEnterButtonAssign(&enter_button);
			CELL_PAD_CIRCLE_BTN = enter_button ? CELL_PAD_CTRL_CIRCLE : CELL_PAD_CTRL_CROSS;
		}
		else //if(conn_s_p == REFRESH_CONTENT)
		{
			{DELETE_CACHED_GAMES} // refresh XML will force "refresh HTML" to rebuild the cache file
		}

		mkdirs(full_path); // make hdd0 dirs GAMES, PS3ISO, PS2ISO, packages, etc.


		//////////// usb ports ////////////
		if(webman_config->usb6 || webman_config->usb7)
		{
			char *dev_name = full_path;
			for(u8 indx = 5, d = 6; d < 128; d++)
			{
				sprintf(dev_name, "/dev_usb%03i", d);
				if(isDir(dev_name))
				{
					if(d == 7 && indx == 5) continue; // avoids duplicated /dev_usb007
					strcpy(drives[indx++], dev_name); if(indx > 6) break;
				}
			}
		}
		///////////////////////////////////

		check_cover_folders(full_path);

		#ifndef LITE_EDITION
		// Use system icons if wm_icons don't exist at /dev_hdd0/tmp/wm_icons/
		char *icon_path = full_path;
		for(u8 i = 0; i < 14; i++)
		{
			if(not_exists(wm_icons[i]))
			{	// Check /dev_flash/vsh/resource/explore/icon/
				concat_path(icon_path, VSH_RESOURCE_DIR "explore/icon", wm_icons[i] + 23); strcpy(wm_icons[i], icon_path);
				if(file_exists(icon_path)) continue;

				// If not found, use /dev_flash/vsh/resource/explore/user/
				char *icon = wm_icons[i] + 32;
				if(i == gPS3 || i == iPS3)	sprintf(icon, "user/0%i.png", 24); else // ps3
				if(i == gPSX || i == iPSX)	sprintf(icon, "user/0%i.png", 26); else // psx
				if(i == gPS2 || i == iPS2)	sprintf(icon, "user/0%i.png", 25); else // ps2
				if(i == gPSP || i == iPSP)	sprintf(icon, "user/0%i.png", 22); else // psp
				if(i == gDVD || i == iDVD)	sprintf(icon, "user/0%i.png", 23); else // dvd
				if(i == iROM || i == iBDVD)	strcopy(wm_icons[i], wm_icons[iPS3]); else
											strcopy(icon + 5, "icon_home.png"); // setup / eject
			}
		}
		#endif

		css_exists = file_exists(COMMON_CSS);
		common_js_exists = file_exists(COMMON_SCRIPT_JS);

		#ifdef NOSINGSTAR
		no_singstar_icon();
		#endif

		#ifndef LITE_EDITION
		chart_init = 0;
		#endif

		sys_ppu_thread_t t_id;
		sys_ppu_thread_create(&t_id, update_xml_thread, conn_s_p, THREAD_PRIO, THREAD_STACK_SIZE_UPDATE_XML, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_CMD);

		if(conn_s_p == START_DAEMON)
		{
			if(wm_reload)
			{
				sys_ppu_thread_sleep(3);
				wm_reload = from_reboot = false;
				sys_ppu_thread_exit(0);
			}

			#ifdef COBRA_NON_LITE
			cobra_read_config(cobra_config);

			// cobra spoofer not working since 4.53
			if(webman_config->nospoof || (c_firmware >= 4.53f))
			{
				cobra_config->spoof_version  = 0;
				cobra_config->spoof_revision = 0;
			}
			else
			{
				cobra_config->spoof_version = 0x0492;
				cobra_config->spoof_revision = 0x00010b72; // 4.91 = 0x00010b28; // 4.90 = 0x00010b14; //4.89 = 0x00010aad; // 4.88 = 0x000109f5; // 4.87 = 0x000109a5 // 4.86 = 0x00010938 // 4.85 = 0x0001091d
			}

			if( cobra_config->ps2softemu == 0 && cobra_get_ps2_emu_type() == PS2_EMU_SW )
				cobra_config->ps2softemu =  1;

			cobra_write_config(cobra_config);
			#endif

			#ifdef SPOOF_CONSOLEID
			spoof_idps_psid();
			#endif

			#ifdef COBRA_ONLY
			#ifdef REMOVE_SYSCALLS
			disable_signin_dialog();

			if(webman_config->spp & 1) //remove syscalls & history
			{
				if(!payload_ps3hen) sys_ppu_thread_sleep(5); do_sleep = false;

				remove_cfw_syscalls(webman_config->keep_ccapi);
				delete_history(true);
			}
			else
			#endif
			if(webman_config->spp & 2) //remove history & block psn servers (offline mode)
			{
				delete_history(false);
				block_online_servers(false);
			}
			#ifdef OFFLINE_INGAME
			if(file_exists(WM_NETDISABLED)) //re-enable network (force offline in game)
			{
				net_status = 1;
				poll_start_play_time();
			}
			#endif
			#endif //#ifdef COBRA_ONLY

			if(!payload_ps3hen) { ENABLE_INGAME_SCREENSHOT }

			// backup / restore act.bak -> act.dat
			backup_act_dat();

			if(do_sleep) sys_ppu_thread_sleep(1);

			#ifdef COBRA_ONLY
			unmap_path("/dev_flash/vsh/resource/coldboot_stereo.ac3");
			unmap_path("/dev_flash/vsh/resource/coldboot_multi.ac3");
			#ifdef VISUALIZERS
			randomize_vsh_resources(true, full_path);
			#endif
			#endif

			// wait for xmb
			while(not_xmb()) sys_ppu_thread_sleep(1);

			if(file_exists("/dev_hdd0/ps3-updatelist.txt"))
				vshnet_setUpdateUrl("http://127.0.0.1/dev_hdd0/ps3-updatelist.txt"); // custom update file

			#ifdef OVERCLOCKING
			// overclock on startup (hold L2 or R2 to disable auto-overclocking on startup)
			if(is_pressed(CELL_PAD_CTRL_L2) || is_pressed(CELL_PAD_CTRL_R2))
			{
				// Reset XMB GPU Core & VRAM frequencies and notify user about canceling overclocking with 2 beeps
				if(webman_config->gpu_core || webman_config->gpu_vram)
				{
					webman_config->gpu_core = webman_config->gpu_vram = 0; // prevent accidental restore of XMB GPU Core & VRAM frequencies mounting a game
					save_settings(); show_rsxclock(msg); BEEP2;
				}
			}
			else
				set_rsxclocks(webman_config->gpu_core, webman_config->gpu_vram); // set xmb gpu clock speed
			#endif

			#ifdef WM_REQUEST
			start_event(EVENT_ON_XMB);
			#endif

			#ifdef PS3MON_SPRX
			if(webman_config->ps3mon)
				load_vsh_plugin(PS3MON_SPRX);
			#endif

			#ifdef ARTEMIS_PRX
			clear_codelist();
			#endif

			#ifdef FIX_CLOCK
			if(webman_config->auto_fixclock)
			{
				#ifdef VIRTUAL_PAD
				// auto accept date/time page
				while(working && (View_Find("explore_plugin") == 0)) {press_accept_button(); sys_ppu_thread_sleep(2);}
				#endif

				if(working)
				{
					static int (*_cellRtcGetCurrentTick)(u64 *pTick) = NULL;
					_cellRtcGetCurrentTick = getNIDfunc("cellRtc", 0x9DAFC0D9, 0);

					u64 currentTick;
					_cellRtcGetCurrentTick(&currentTick);

					#define date_time full_path
					if(currentTick < FIX_CLOCK_DATE) update_clock_from_server_time(date_time);
				}
			}
			#endif

			#ifdef PLAY_MUSIC
			if(BETWEEN(1, webman_config->music, 2) && USER_LOGGEDIN)
			{
				if(wait_for_abort(1)) sys_ppu_thread_exit(0);

				bool is_video = (webman_config->music == 2);
				sprintf(msg, "Starting %s...\n"
							 "Press O to abort",
							 is_video ? "Video" : "Music");

				show_msg_with_icon(is_video ? ICON_VIDEO : ICON_MUSIC, msg);

				if(!wait_for_abort(8 + webman_config->boots))
					start_xmb_player(is_video ? "video" : "music");
				else
					play_rco_sound("snd_system_ng");
			}
			#endif
		}
	}

	sys_ppu_thread_exit(0);
}
