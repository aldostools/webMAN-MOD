#define _IS_ON_XMB_		((sec & 1) && (gTick.tick == rTick.tick))
#define _IS_IN_GAME_	((sec & 1) && (gTick.tick >  rTick.tick))

static bool toggle_snd0 = false;

static void poll_start_play_time(void)
{
	#ifdef OFFLINE_INGAME
	s32 status = 0;
	#endif

	if(IS_ON_XMB)
	{
		//if(gTick.tick != rTick.tick) vshnet_setUpdateUrl("http://127.0.0.1/dev_hdd0/ps3-updatelist.txt"); // re-apply redirection of custom update file returning to XMB
	#ifdef COBRA_ONLY
	 #ifdef WM_PROXY_SPRX
		if(gTick.tick != rTick.tick)
		{
			gTick = rTick;

			if(payload_ps3hen) sys_ppu_thread_sleep(6);

			if(!max_temp && webman_config->fanc) reset_fan_mode();

			apply_remaps(); // re-apply remaps returning to XMB

			#ifdef VISUALIZERS
			char *param = html_base_path;
			randomize_vsh_resources(false, param);
			*param = NULL;
			#endif

			#ifdef OVERCLOCKING
			set_rsxclocks(webman_config->gpu_core, webman_config->gpu_vram); // set xmb gpu clock speed
			#endif

			#ifdef WM_REQUEST
			start_event(EVENT_ON_XMB);
			#endif

			#ifdef ARTEMIS_PRX
			if(!is_artemis_app)
				clear_codelist();
			#endif

			// auto-reload full edition after return to XMB
			#ifdef AUTO_TOGGLE_ON_XMB
			#define TOGGLE_PLUGIN	"/dev_hdd0/plugins/webftp_server.sprx"
			#define unload_me(mode)	{wm_unload_combo = mode; wwwd_stop();}

			if(!syscalls_removed && !is_mounting && !refreshing_xml && file_exists(TOGGLE_PLUGIN))
			{
				create_file(WM_RELOAD_FILE); // create semaphore file

				load_vsh_module(TOGGLE_PLUGIN);

				unload_me(3);
				return;
			}
			#endif
		}
	 #endif
	#endif
		// fix startup time < FIX_CLOCK_DATE
		if(rTick.tick < FIX_CLOCK_DATE)
			cellRtcGetCurrentTick(&rTick);
		gTick = rTick;

		#ifdef OFFLINE_INGAME
		if(net_status >= 0)
		{
			xnet()->GetSettingNet_enable(&status);
			xnet()->SetSettingNet_enable(net_status);
			net_status = NONE; if(net_status && !status) show_msg_with_icon(ICON_NETWORK, ONLINE_TAG);
			cellFsUnlink(WM_NETDISABLED);
		}
		#endif

		if(toggle_snd0 && webman_config->nosnd0) { toggle_snd0 = false; cellFsChmod("/dev_bdvd/PS3_GAME/SND0.AT3", NOSND); } /* disable SND0.AT3 on XMB */
	}
	else if(gTick.tick == rTick.tick) /* the game started a moment ago */
	{
		cellRtcGetCurrentTick(&gTick);

		if(!toggle_snd0 && webman_config->nosnd0) { toggle_snd0 = true; cellFsChmod("/dev_bdvd/PS3_GAME/SND0.AT3", MODE); } /* re-enable SND0.AT3 in-game */

		close_ftp_sessions_idle(); 

		#ifdef PS3MAPI
		// unmap gameboot audio
		unmap_path("/dev_flash/vsh/resource/gameboot_multi.ac3");
		unmap_path("/dev_flash/vsh/resource/gameboot_stereo.ac3");
		#endif
		#ifdef PATCH_GAMEBOOT
		patched_address1 = patched_address2 = patched_address3 = patched_address4 = BASE_PATCH_ADDRESS;
		#endif

		#ifdef OFFLINE_INGAME
		if((webman_config->spp & 4) || (net_status >= 0))
		{
			get_game_info();

			if(strlen(_game_TitleID) == 9 && View_Find("nas_plugin_module") == 0)
			{
				bool set_net_setatus = true;
				if(net_status < 0)
				{
					char online_title_ids[0x200];
					read_file(WM_OFFLINE_IDS_FILE, online_title_ids, sizeof(online_title_ids), 0); // auto-disable network only on these title ids
					if(*online_title_ids) set_net_setatus = strstr(online_title_ids, _game_TitleID);
					else
					{
						read_file(WM_ONLINE_IDS_FILE, online_title_ids, sizeof(online_title_ids), 0);  // auto-disable network except on these title ids
						set_net_setatus = (strstr(online_title_ids, _game_TitleID) == NULL);
					}
				}

				if(set_net_setatus)
				{
					xnet()->GetSettingNet_enable(&status);
					xnet()->SetSettingNet_enable(net_status < 0 ? 0 : net_status);
					if(status && (net_status <= 0)) {create_file(WM_NETDISABLED); show_msg_with_icon(ICON_NETWORK, OFFLINE_TAG);}
					net_status = status;
				}
			}
		}
		#endif

		#ifdef OVERCLOCKING
		if(!pergame_overclocking)
			set_rsxclocks(webman_config->gpu2_core, webman_config->gpu2_vram); // set in-game gpu clock speed
		#endif

		#ifdef WM_REQUEST
		start_event(EVENT_INGAME);
		wait_for_title_id();
		start_event(EVENT_PER_GAME);
		#endif

		#ifdef ARTEMIS_PRX
		is_artemis_app = IS(_game_TitleID, "ARTPS3001");
		if(webman_config->artemis)
		{
			if(*_game_TitleID)
			{
				char codelist[48], app_ver[12], *sfo, *version;
				sfo = codelist, *app_ver = '_', version = app_ver + 1, *version = NULL;

				set_param_sfo(sfo);
				if(file_exists(sfo))
					getTitleID(sfo, version, GET_VERSION);
				get_local_app_ver(version, _game_TitleID, sfo); *sfo = NULL;

				if(*version)
				{
					strcat(version, ".ncl");
					snprintf(codelist, sizeof(codelist), "%s/%s/%s%s", TMP_DIR, "artemis", _game_TitleID, app_ver);
				}

				if(not_exists(codelist))
					snprintf(codelist, sizeof(codelist), "%s/%s/%s%s", TMP_DIR, "artemis", _game_TitleID, ".ncl");
				if(not_exists(codelist))
					snprintf(codelist, sizeof(codelist), "%s%s/%s%s", HDD0_GAME_DIR, _game_TitleID, "artemis", ".ncl");

				init_codelist(codelist); // copy codelist for PSN or hdd0/tmp/artemis
			}
			if(file_exists(ARTEMIS_CODES_FILE))
				start_artemis();
		}
		#endif

		// increase fan speed in manual mode if webman_config->man_ingame > 0
		if(!max_temp && webman_config->fanc)
		{
			while(is_ingame_first_15_seconds()) sys_ppu_thread_sleep(1);
			reset_fan_mode();
		}
	}
}

static void poll_thread(__attribute__((unused)) u64 arg)
{
	u8 sec = 0;

	// fancontrol2.h
	u8 t1 = 0, t2 = 0;
	u8 lasttemp = 0;
	const u8 step = 3; // polling every 3 seconds
	const u8 step_up = 5;
	u8 stall = 0;
	u8 smoothstep = 0;
	int delta = 0;
	u8 oc = 0; // overheat control timer

	char msg[0x100];

	check_reload();

	old_fan = 0;
	while(working)
	{
		#ifdef REMOVE_SYSCALLS
		// check if syscalls were restored
		if(_IS_ON_XMB_ && syscalls_removed != CFW_SYSCALLS_REMOVED(TOC))
		{
			syscalls_removed = CFW_SYSCALLS_REMOVED(TOC);
			#ifdef PS3MAPI
			if(!syscalls_removed) restore_cfw_syscalls();
			#else
			if(!syscalls_removed) disable_signin_dialog();
			#endif
		}
		#endif

		// detect aprox. time when a game is launched & set network connect status
		poll_start_play_time();

		// dynamic fan control
		#include "fancontrol2.h"

		// Poll combos for 3 seconds
		#include "combos.h"

		if(!working) break;

		#ifdef FPS_OVERLAY
		if(overlay_info)
		{
			get_sys_info(msg, overlay_info % 100, (overlay_info >= 100));
			show_progress(msg, OV_SHOW);
		}
		#endif

		// USB Polling
		if((sec >= 120) && !webman_config->poll) // check USB drives each 120 seconds
		{
			usb_keep_awake(oc);
			sec = 0;
		}
		sec += step;

		#ifdef PKG_HANDLER
		// Poll downloaded pkg files (if is on XMB)
		if(_IS_ON_XMB_) poll_downloaded_pkg_files(msg);
		#endif

		#ifdef COBRA_ONLY
		// Poll insert USB
		mount_on_insert_usb(_IS_ON_XMB_, msg);
		#endif

		#ifdef PHOTO_GUI
		// Poll requests via local file
		if(webman_config->launchpad_xml) continue; // poll wm_request file only if PhotoGUI is enabled

		if(_IS_IN_GAME_) continue; // slow down poll in-game

		if(is_ingame_first_15_seconds()) continue; // do not poll within the first 15 seconds ingame

		if(file_exists(WM_REQUEST_FILE))
		{
			loading_html++;
			sys_ppu_thread_t t_id;
			if(working) sys_ppu_thread_create(&t_id, handleclient_www, WM_FILE_REQUEST, THREAD_PRIO, THREAD_STACK_SIZE_WEB_CLIENT, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_WEB);
		}
		#endif
	}

	//thread_id_poll = SYS_PPU_THREAD_NONE;
	sys_ppu_thread_exit(0);
}
