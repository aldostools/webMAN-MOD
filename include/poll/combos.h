/*
 FAIL SAFE    : SELECT+L3+L2+R2 - delete boot_plugins.txt only for sys_admin on XMB
 RESET SAFE   : SELECT+R3+L2+R2 - reset config only for sys_admin on XMB

 REFRESH XML  : SELECT+L3 (+R2=profile1, +L2=profile2, +R1=profile3, +L1=profile4, +L1+R1=Reload XMB, +R2+L2=FAIL SAFE)
                                                *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_l3
 UNLOAD WM    : R2+L3+R3 / L3+R3+R2 / L3+R2+R3

 FPS INFO     : L3+R3                           *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_l3_r3

 GOTO_HOME    : L2+L3+R3 / L3+R3+L2 / L3+L2+R3  *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_l3_r3_l2

 PLAY_DISC    : L2+START                        *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_l2_start
 PLAY APP_HOME: R2+START                        *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_r2_start

 PREV GAME    : SELECT+L1                       *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_l1
 NEXT GAME    : SELECT+R1                       *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_r1
 UMNT_GAME    : SELECT+O (unmount)              *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_circle

 RESERVED     : SELECT+TRIANGLE                 *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_triangle
 RESERVED     : SELECT+CROSS                    *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_cross

 EXT GAME DATA: SELECT+□                        *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_square
 MOUNT net0/  : SELECT+R2+□
 MOUNT net1/  : SELECT+L2+□

 SHUTDOWN     : L3+R2+X
 SHUTDOWN  *2 : L3+R1+X (vsh shutdown) <- alternative shutdown method
 RESTART      : L3+R2+O (lpar restart)
 RESTART   *2 : L3+R1+O (vsh restart)  <- alternative restart method

 FAN CNTRL    : L3+R2+START    (enable/disable fancontrol)
 FAN CNTRL    : L3+L2+START    (enable auto #2)
 FAN CNTRL    : L3+L2+R2+START (enable ps2 fan mode)
 SHOW TEMP    : SELECT+START   (SELECT+START+R2 will show only copy progress) / SELECT+R3 (if rec video flag is disabled)

 DYNAMIC TEMP : SELECT+LEFT/RIGHT               *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_left
                                                *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_right
 MANUAL TEMP  : SELECT+UP/DOWN                  *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_up
                                                *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_down

 REC VIDEO    : SELECT+R3          Record video using internal plugin (IN-GAME ONLY)
                                   *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_r3
 REC VIDEO PLG: SELECT+R3+L2+R2    Unload webMAN & Record video with video_rec plugin (IN-GAME ONLY)
 REC VIDEO SET: SELECT+R3+L2       Select video rec setting
 REC VIDEO VAL: SELECT+R3+R2       Change value of video rec setting
 XMB SCRNSHOT : L2+R2+SELECT+START              *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_l2_r2_select_start

 TOGGLE PLUGIN: L1+R1+TRIANGLE                  *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_l1_r1_triangle
 USER/ADMIN   : L2+R2+TRIANGLE                  *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_l2_r2_triangle

 SYSCALLS     : R2+TRIANGLE                     *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_r2_triangle
 SHOW IDPS    : R2+O  (Abort copy/fix process)  *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_r2_circle
 OFFLINE MODE : R2+□                            *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_r2_square

 QUICK INSTALL: SELECT+R2+O                     *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_r2_circle

 TOGGLE PS2CLASSIC    : SELECT+L2+TRIANGLE
 SWITCH PS2EMU        : SELECT+L2+R2

 COBRA TOGGLE         : L3+L2+TRIANGLE
 REBUG  Mode Switcher : L3+L2+□
 Normal Mode Switcher : L3+L2+O
 DEBUG  Menu Switcher : L3+L2+X

 SKIP AUTO-MOUNT   : L2+R2  (at startup only)   *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_l2_r2

 VSH MENU          : SELECT (hold down for few seconds on XMB only)
 sLaunch MENU      : L2+R2 or START (hold down for few seconds on XMB only)

 Open File Manager : L2+R2+O                    *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_l2_r2_circle
 Open Games List   : L2+R2+R1+O                 *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_l2_r2_r1_circle
 Open System Info  : L2+R2+L1+O                 *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_l2_r2_l1_circle
 Open Setup        : L2+R2+L1+R1+O              *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_l2_r2_l1_r1_circle
*/
{
	bool reboot = false;

	u8 n;

	init_delay = 0;

	CellPadInfo2 padinfo;

	// CELL_PAD_BTN_OFFSET_DIGITAL1: UP     | DOWN  | LEFT   | RIGHT    | SELECT | START | L3 | R3
	// CELL_PAD_BTN_OFFSET_DIGITAL2: CIRCLE | CROSS | SQUARE | TRIANGLE |   L1   |   R1  | L2 | R2

	#define PERSIST  248
	#define break_and_wait	{n = 0; break;}

	for(n = 0; n < 10; n++)
	{
		if(!working) break;

		if(show_persistent_popup == PERSIST) {goto show_persistent_popup;}
		if(show_info_popup) {show_info_popup = false; goto show_popup;}

		//if(!webman_config->nopad)
		{
			pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] = pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] = pad_data.len = 0;

			#ifdef VIRTUAL_PAD
			if(vcombo)
			{
				pad_data.len = 16; pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] = (vcombo & 0xFF); pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] = (vcombo & 0xFF00) >> 8; vcombo = 0;
			}
			else
			#endif
			if(cellPadGetInfo2(&padinfo) == CELL_OK)
			{
				for(u8 p = 0; p < 8; p++)
					if((padinfo.port_status[p] == CELL_PAD_STATUS_CONNECTED) && (cellPadGetData(p, &pad_data) == CELL_PAD_OK) && (pad_data.len > 0)) break;
			}

			if(pad_data.len > 0)
			{
				#ifdef COBRA_ONLY
				if( ((!(webman_config->combo2 & C_SLAUNCH)) && (((pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == CELL_PAD_CTRL_START) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == 0)) ||                    // START  = SLAUNCH MENU
																((pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == 0) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2)))))   // L2+R2  = SLAUNCH MENU
				||	((!(webman_config->combo2 & C_VSHMENU)) &&  ((pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == CELL_PAD_CTRL_SELECT) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == 0))) )                   // SELECT = VSH MENU
				{
					// hold SELECT or START for 3-5 seconds
					if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] && (++init_delay < 5)) {sys_ppu_thread_usleep(100000); continue;}

					#ifdef WM_CUSTOM_COMBO
					// call script override
					if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2)) // L2+R2
					{
						if(do_custom_combo("l2_r2")) continue;
					}
					#endif

					// START = sLaunch / SELECT = VSH Menu
					start_vsh_gui(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == CELL_PAD_CTRL_SELECT);

					// wait until pad buttons are released
					while(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] | pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2]) pad_data = pad_read();

					break;
				}
				if(((webman_config->combo & UNLOAD_WM) == from_reboot) &&
						(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == 0) &&
						(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L1 | CELL_PAD_CTRL_R1 | CELL_PAD_CTRL_TRIANGLE))) //  L1+R1+TRIANGLE
				{
					#ifdef WM_CUSTOM_COMBO
					if(do_custom_combo("l1_r1_triangle")) continue;
					#endif

					toggle_wmm_lite(); break;
				}
				#endif
				if(!(webman_config->combo2 & PLAY_DISC) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == CELL_PAD_CTRL_START))
				{
					// L2+START = Play Disc
					// R2+START = Play app_home/PS3_GAME
					if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_L2)
					{
						#ifdef WM_CUSTOM_COMBO
						if(do_custom_combo("l2_start")) continue;
						#endif
						launch_disc(true); // L2+START

						break_and_wait;
					}
					#ifdef COBRA_ONLY
					if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R2)
					{
						#ifdef WM_CUSTOM_COMBO
						if(do_custom_combo("r2_start")) continue;
						#endif
						if(not_exists("/app_home/PS3_GAME/USRDIR/EBOOT.BIN"))
						{
							if(is_app_dir(webman_config->home_url, "."))
								set_app_home(webman_config->home_url);
							else if(islike(webman_config->home_url, "http"))
								open_browser(webman_config->home_url, 0);
							#ifdef WM_REQUEST
							else if(*webman_config->home_url == '/') handle_file_request(webman_config->home_url); // web command
							#endif
						}

						launch_app_home_icon(true);

						break_and_wait;
					}
					#endif
				}

				if((pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_SELECT))
				{
					#ifdef COBRA_NON_LITE
					if( !(webman_config->combo & FAIL_SAFE) &&
						(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == (CELL_PAD_CTRL_SELECT | CELL_PAD_CTRL_L3)) && // fail-safe mode
						(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2))        // SELECT+L3+L2+R2
						)
					{
						//// startup time /////
						CellRtcTick pTick; cellRtcGetCurrentTick(&pTick);
						u32 ss = (u32)((pTick.tick - rTick.tick)/1000000);
						///////////////////////

						if(!sys_admin || (ss > 60) || IS_INGAME) continue; // allow delete boot_plugins.txt only on XMB to sys_admin in the first minute after boot

						cellFsUnlink("/dev_hdd0/boot_plugins.txt");
						cellFsUnlink("/dev_hdd0/boot_plugins_nocobra.txt");
						cellFsUnlink("/dev_hdd0/boot_plugins_nocobra_dex.txt");
						goto reboot; // vsh reboot
					}
					else
					if( (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == (CELL_PAD_CTRL_SELECT | CELL_PAD_CTRL_R3)) &&       // reset-safe mode
						(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2)) && IS_ON_XMB // SELECT+R3+L2+R2
						)
					{
						if(!sys_admin || IS_INGAME) continue; // allow reset config only for sys_admin on XMB

						#ifndef ENGLISH_ONLY
						char STR_RMVWMCFG[96];//	= "webMAN config reset in progress...";
						char STR_RMVWMCFGOK[112];//	= "Done! Restart within 3 seconds";

						language("STR_RMVWMCFG", STR_RMVWMCFG, WM_APPNAME " config reset in progress...");
						language("STR_RMVWMCFGOK", STR_RMVWMCFGOK, "Done! Restart within 3 seconds");

						close_language();
						#endif

						cellFsUnlink(WM_CONFIG_FILE);
						{ BEEP1 }
						show_msg(STR_RMVWMCFG);
						sys_ppu_thread_sleep(2);
						show_msg(STR_RMVWMCFGOK);
						sys_ppu_thread_sleep(3);
						goto reboot; // vsh reboot
					}
					else
					if( !(webman_config->combo2 & PS2TOGGLE)
						&& (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_TRIANGLE)) // SELECT+L2+TRIANGLE
						&& (c_firmware >= 4.65f) )
					{
						bool classic_ps2_enabled = file_exists(PS2_CLASSIC_TOGGLER);

						if(classic_ps2_enabled)
						{
							disable_classic_ps2_mode();
						}
						else
						{
							enable_classic_ps2_mode();
						}

						show_status("PS2 Classic", classic_ps2_enabled);

						break_and_wait;
					}
					else
					if( !(webman_config->combo2 & PS2SWITCH)
						&& (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2)) // SELECT+L2+R2
						&& (c_firmware>=4.53f) )
					{
							toggle_ps2emu(); // Clone ps2emu habib's switcher
					}
					else
					#endif // #ifdef COBRA_NON_LITE
					if(!(webman_config->combo2 & XMLREFRSH) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == (CELL_PAD_CTRL_SELECT | CELL_PAD_CTRL_L3))) // SELECT+L3 refresh XML
					{
						// SELECT+L3       = refresh XML
						// SELECT+L3+R2    = refresh XML profile 1
						// SELECT+L3+L2    = refresh XML profile 2
						// SELECT+L3+R1    = refresh XML profile 3
						// SELECT+L3+L1    = refresh XML profile 4
						// SELECT+L3+L1+R1 = refresh XML + Reload XMB
						#ifdef WM_CUSTOM_COMBO
						if(do_custom_combo("select_l3")) break;
						#endif

						#ifndef LITE_EDITION
						if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R2) profile = 1; else
						if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_L2) profile = 2; else
						if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R1) profile = 3; else
						if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_L1) profile = 4; else profile = 0;
						#endif

						if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_R1 | CELL_PAD_CTRL_L1)) n = 11;

						refresh_xml(msg);
						if(n > 10) reload_xmb(0);
					}
					#ifdef VIDEO_REC
					else
					if(!(webman_config->combo2 & VIDRECORD) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == (CELL_PAD_CTRL_SELECT | CELL_PAD_CTRL_R3))) // SELECT + R3
					{
						// SELECT+R3+L2+R2  = Record video with video_rec plugin (IN-GAME ONLY)
						// SELECT+R3+L2     = Select video rec setting
						// SELECT+R3+R2     = Change value of video rec setting
						// SELECT+R3        = Toggle Record Video
						#ifdef WM_CUSTOM_COMBO
						if(do_custom_combo("select_r3")) break;
						#endif
						#ifdef COBRA_ONLY
						if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2)) // SELECT+R3+L2+R2  Record video with video_rec plugin (IN-GAME ONLY)
						{
							// SELECT+R3+L2+R2 = Record video with video_rec plugin (IN-GAME ONLY)

							#define VIDEO_REC_PLUGIN  WM_RES_PATH "/video_rec.sprx"

							if((!recording) && (IS_INGAME) && file_exists(VIDEO_REC_PLUGIN))
							{
								unsigned int slot = load_vsh_plugin(VIDEO_REC_PLUGIN);
								if(slot < 7) unload_me(1); // unload webMAN to free resources
							}
						}
						else
						#endif // #ifdef COBRA_ONLY
						if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_L2)	// SELECT+R3+L2  Select video rec setting
						{
							// SELECT+R3+L2 = Select video rec setting

							if(++rec_setting_to_change > 5) rec_setting_to_change = 0;
							set_setting_to_change(msg, "Change : ");

							strcat(msg, "\n\nCurrent recording format:");
							show_rec_format(msg);
						}
						else
						if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R2)	// SELECT+R3+R2  Change value of video rec setting
						{
							// SELECT+R3+R2 = Change value of video rec setting

							set_setting_to_change(msg, "Changed : ");

							strcat(msg, "\n\nCurrent recording format:");
							change_rec_format(msg);
						}
						else
							{_memset(msg, sizeof(msg)); toggle_video_rec(msg);} // SELECT+R3  Record Video

						break_and_wait;
					}
					#endif
					else
					if( !(webman_config->combo & SHOW_TEMP) && ((pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == (CELL_PAD_CTRL_SELECT | CELL_PAD_CTRL_START)) // SELECT+START show temperatures / hdd space
					#ifndef VIDEO_REC
															||  (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == (CELL_PAD_CTRL_R3 | CELL_PAD_CTRL_START))
					#endif
						))
					{
						// SELECT+START+L2+R2 = screenshot of XMB
						// SELECT+START       = show temp or copy progress + show temp (hold SELECT+START for 5 seconds to toggle persistent popup)
						// SELECT+START+R2    = show only copy progress
						// SELECT+R3          = show temp (if no VIDEO_REC)
						#ifdef XMB_SCREENSHOT
						if((pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_L2)) && IS_ON_XMB)
						{
							#ifdef WM_CUSTOM_COMBO
							if(do_custom_combo("l2_r2_select_start")) break;
							#endif
							{BEEP2; _memset(msg, sizeof(msg)); saveBMP(msg, true, false); break_and_wait} // L2 + R2 + SELECT + START
						}
						else
						#endif
						{
							if(show_persistent_popup == 0)        show_persistent_popup = 1;               else
							if(show_persistent_popup  < PERSIST) {BEEP1; show_persistent_popup = PERSIST;} else
																 {BEEP2; show_persistent_popup = 0;}
 show_persistent_popup:
							/////////////////////////////
							#if defined(FIX_GAME) || defined(COPY_PS3)
							if(copy_in_progress || fix_in_progress)
							{
								#ifdef FIX_GAME
								if(fix_in_progress)
									concat_text(msg, STR_FIXING, current_file);
								else
								#endif
									get_copy_stats(msg, STR_COPYING);

								show_msg(msg);
								sys_ppu_thread_sleep(2);
								if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & (CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_L2) ) break;
							}
							#endif
							/////////////////////////////
 show_popup:
							if(is_custom_popup)
							{
								strncopy(msg, 200, custom_popup_msg);
								#ifdef COBRA_NON_LITE
								parse_tags(msg);
								#endif
							}
							else
								get_sys_info(msg, (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R2), false);
							show_msg(msg);
							sys_ppu_thread_sleep(2);
						}
					}
					else
					if(webman_config->fanc && !(webman_config->combo & MANUALFAN) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == (CELL_PAD_CTRL_SELECT | CELL_PAD_CTRL_UP))) // SELECT+UP increase TEMP/FAN
					{
						// SELECT+UP      = increase TEMP of dynamic fan control / manual FAN SPEED +1
						// SELECT+UP + R2 = increase TEMP of dynamic fan control / manual FAN SPEED +5
						// SELECT+UP + L2 = increase GPU Core clock speed
						#ifdef WM_CUSTOM_COMBO
						if(do_custom_combo("select_up")) break;
						#endif
						#ifdef OVERCLOCKING
						if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2)
						{
							u16 mhz = get_rsxclock(GPU_CORE_CLOCK);
							overclock(mhz + 50, 0); show_rsxclock(msg);
						}
						else
						#endif
						{
							if(webman_config->fanc == DISABLED) enable_fan_control(ENABLE_SC8);

							if(max_temp) //auto mode
							{
								if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) max_temp+=5; else max_temp+=1;
								if(max_temp > MAX_TEMPERATURE) max_temp = MAX_TEMPERATURE;
								webman_config->dyn_temp = max_temp;
								sprintf(msg, "%s\n%s %i°C", STR_FANCH0, STR_FANCH1, max_temp);
							}
							else
							{
								if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) webman_config->man_rate += 5; else webman_config->man_rate += 1;
								webman_config->man_rate = RANGE(webman_config->man_rate, MIN_FANSPEED, webman_config->maxfan); //%
								webman_config->man_speed = PERCENT_TO_8BIT(webman_config->man_rate);
								webman_config->man_speed = RANGE(webman_config->man_speed, MIN_FANSPEED_8BIT, MAX_FANSPEED_8BIT);
								set_fan_speed(webman_config->man_speed + 1);
								sprintf(msg, "%s\n%s %i%%", STR_FANCH0, STR_FANCH2, webman_config->man_rate);
							}

							save_settings();
							show_msg(msg);
						}

						break_and_wait;
					}
					else
					if(webman_config->fanc && !(webman_config->combo & MANUALFAN) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == (CELL_PAD_CTRL_SELECT | CELL_PAD_CTRL_DOWN))) // SELECT+DOWN decrease TEMP/FAN
					{
						// SELECT+DOWN    = decrease TEMP of dynamic fan control / manual FAN SPEED -1
						// SELECT+DOWN+R2 = decrease TEMP of dynamic fan control / manual FAN SPEED -5
						// SELECT+DOWN+L2 = decrease GPU Core clock speed
						#ifdef WM_CUSTOM_COMBO
						if(do_custom_combo("select_down")) break;
						#endif
						#ifdef OVERCLOCKING
						if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2)
						{
							u16 mhz = get_rsxclock(GPU_CORE_CLOCK);
							overclock(mhz - 50, 0); show_rsxclock(msg);
						}
						else
						#endif
						{
							if(webman_config->fanc == DISABLED) enable_fan_control(ENABLE_SC8);

							if(max_temp) //auto mode
							{
								if(max_temp > 30) {if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) max_temp-=5; else max_temp-=1;}
								webman_config->dyn_temp = max_temp;
								sprintf(msg, "%s\n%s %i°C", STR_FANCH0, STR_FANCH1, max_temp);
							}
							else
							{
								if(webman_config->man_rate>20) {if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) webman_config->man_rate -= 5; else webman_config->man_rate -= 1;}
								webman_config->man_speed = PERCENT_TO_8BIT(webman_config->man_rate);
								webman_config->man_speed = RANGE(webman_config->man_speed, MIN_FANSPEED_8BIT, MAX_FANSPEED_8BIT);
								set_fan_speed(webman_config->man_speed + 1);
								sprintf(msg, "%s\n%s %i%%", STR_FANCH0, STR_FANCH2, webman_config->man_rate);
							}

							save_settings();
							show_msg(msg);
						}

						break_and_wait;
					}
					else
					if(webman_config->minfan && !(webman_config->combo & MINDYNFAN) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == (CELL_PAD_CTRL_SELECT | CELL_PAD_CTRL_LEFT))) // SELECT+LEFT decrease Minfan
					{
						// SELECT+LEFT = decrease Minfan
						// SELECT+LEFT+L2 = decrease GPU VRAM clock speed
						#ifdef WM_CUSTOM_COMBO
						if(do_custom_combo("select_left")) break;
						#endif
						#ifdef OVERCLOCKING
						if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2)
						{
							u16 mhz = get_rsxclock(GPU_VRAM_CLOCK);
							overclock(mhz - 25, 1); show_rsxclock(msg);
						}
						else
						#endif
						{
							if(webman_config->fanc == DISABLED) enable_fan_control(ENABLE_SC8);

							if(webman_config->minfan-5 >= MIN_FANSPEED) webman_config->minfan -= 5;
							sprintf(msg, "%s\n%s %i%%", STR_FANCH0, STR_FANCH3, webman_config->minfan);

							save_settings();
							show_msg(msg);
						}

						break_and_wait;
					}
					else
					if(webman_config->minfan && !(webman_config->combo & MINDYNFAN) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == (CELL_PAD_CTRL_SELECT | CELL_PAD_CTRL_RIGHT))) // SELECT+RIGHT increase Minfan
					{
						// SELECT+RIGHT = increase Minfan
						// SELECT+RIGHT+L2 = increase GPU VRAM clock speed
						#ifdef WM_CUSTOM_COMBO
						if(do_custom_combo("select_right")) break;
						#endif
						#ifdef OVERCLOCKING
						if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2)
						{
							u16 mhz = get_rsxclock(GPU_VRAM_CLOCK);
							overclock(mhz + 25, 1); show_rsxclock(msg);
						}
						else
						#endif
						{
							if(webman_config->fanc == DISABLED) enable_fan_control(ENABLE_SC8);

							if(webman_config->minfan < webman_config->maxfan) webman_config->minfan += 5;
							sprintf(msg, "%s\n%s %i%%", STR_FANCH0, STR_FANCH3, webman_config->minfan);

							save_settings();
							show_msg(msg);
						}

						break_and_wait;
					}
					else if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == CELL_PAD_CTRL_SELECT)
					{
						// SELECT+SQUARE    = Toggle External Game Data
						// SELECT+SQUARE+R2 = Mount net0
						// SELECT+SQUARE+L2 = Mount net1

						// SELECT+L1 = mount previous title
						// SELECT+R1 = mount next title
						// SELECT+O+R2 = Install PKG
						// SELECT+O = unmount
						// SELECT+TRIANGLE = RESERVED

						if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_SQUARE) // SELECT+SQUARE
						{
							#ifdef COBRA_NON_LITE
							if(!(webman_config->combo2 & MOUNTNET0) &&
								(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_SQUARE | CELL_PAD_CTRL_R2)))
								{if(is_netsrv_enabled(0)) mount_game("/net0", EXPLORE_CLOSE_ALL);} // SELECT+SQUARE+R2 / SELECT+R2+SQUARE
							else
							if(!(webman_config->combo2 & MOUNTNET1) &&
								(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_SQUARE | CELL_PAD_CTRL_L2)))
								{if(is_netsrv_enabled(1)) mount_game("/net1", EXPLORE_CLOSE_ALL);} // SELECT+SQUARE+L2 / SELECT+L2+SQUARE
							else
							if(!(webman_config->combo2 & EXTGAMDAT) &&       // Toggle External Game Data
								(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_SQUARE))
							#else
							if(!(webman_config->combo2 & EXTGAMDAT))         // Toggle External Game Data
							#endif
							{
								#ifdef WM_CUSTOM_COMBO
								if(do_custom_combo("select_square")) break;
								#endif
								{
									#ifdef EXT_GDATA
									if((extgd == 0) && isDir("/dev_bdvd/GAMEI"))
										set_gamedata_status(2, true); // enable external gameDATA (if GAMEI exists on /bdvd)
									else
										set_gamedata_status(extgd^1, true); // SELECT+SQUARE
									#endif
									break_and_wait;
								}
							}
						}
						else
						if(!(webman_config->combo & PREV_GAME) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_L1) ) // SELECT+L1 (previous title)
						{
							 // SELECT+L1 = mount previous title
							#ifdef WM_CUSTOM_COMBO
							if(do_custom_combo("select_l1")) break;
							#endif
							{
								mount_game("_prev", EXPLORE_CLOSE_ALL);

								break_and_wait;
							}
						}
						else
						if(!(webman_config->combo & NEXT_GAME) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R1) ) // SELECT+R1 (next title)
						{
							// SELECT+R1 = mount next title
							#ifdef WM_CUSTOM_COMBO
							if(do_custom_combo("select_r1")) break;
							#endif
							{
								mount_game("_next", EXPLORE_CLOSE_ALL);

								break_and_wait;
							}
						}
						#ifdef PKG_HANDLER
						else
						if(!(webman_config->combo2 & INSTALPKG) && ( pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CIRCLE_BTN | CELL_PAD_CTRL_R2) ))  // SELECT+R2+O (Install PKG)
						{
							// SELECT+R2+O = Install PKG
							// SELECT+R2+X = Install PKG (JAP)
 							#ifdef WM_CUSTOM_COMBO
							if(do_custom_combo("select_r2_circle")) break;
			 				#endif
							installPKG_all(DEFAULT_PKG_PATH, true);

							break_and_wait;
						}
						#endif // #ifdef PKG_HANDLER
						else
						if(!(webman_config->combo2 & UMNT_GAME) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CIRCLE_BTN)) // SELECT+O (unmount)
						{
							// SELECT+O = unmount
							// SELECT+X = unmount (JAP)
							#ifdef WM_CUSTOM_COMBO
							if(do_custom_combo("select_circle")) break;
							#endif
							#ifdef ALLOW_DISABLE_MAP_PATH
							{
								u64 open_hook_symbol = (dex_mode) ? open_hook_dex : open_hook_cex;

								bool map_path_enabled = (peekq(open_hook_symbol) != original_open_hook);

								do_umount(true);

								if(webman_config->app_home)
								{
									show_status("map_path", disable_map_path(map_path_enabled));
								}
							}
							#else
								do_umount(true);
							#endif
							#ifdef ARTEMIS_PRX
							clear_codelist();
							#endif
							#ifdef COBRA_ONLY
							gm = 00; // resets PS3_GMxx counter in map_app_home() of mount.h
							#endif
							break_and_wait;
						}
						#ifdef WM_CUSTOM_COMBO
						else
						if(!(webman_config->combo2 & UMNT_GAME) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_TRIANGLE) ) // SELECT+TRIANGLE
						{
							// SELECT+TRIANGLE = RESERVED for custom combos

							//if((pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_TRIANGLE)) && do_custom_combo("select_l2_triangle")) break; // RESERVED
							//if((pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_TRIANGLE)) && do_custom_combo("select_r2_triangle")) break; // RESERVED
							if(do_custom_combo("select_triangle")) break;    // RESERVED
						}
						else
						if(!(webman_config->combo2 & UMNT_GAME) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_CROSS) ) // SELECT+CROSS
						{
							// SELECT+CROSS = RESERVED for custom combos

							//if((pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_CROSS)) && do_custom_combo("select_l2_cross")) break; // RESERVED
							//if((pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_CROSS)) && do_custom_combo("select_r2_cross")) break; // RESERVED
							if(do_custom_combo("select_cross")) break;    // RESERVED
						}
						#endif
					}
				} // SELECT
				else if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == (CELL_PAD_CTRL_L3 | CELL_PAD_CTRL_R3))
				{
					if(!(webman_config->combo & UNLOAD_WM) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R2)) //  R2+L3+R3 / L3+R3+R2 / L3+R2+R3 (Quit / Unload webMAN)
					{
						// R2+L3+R3 / L3+R3+R2 / L3+R2+R3 = Quit / Unload webMAN
						unload_me(2); // keep fan control running
					}
					#ifdef COBRA_ONLY
					else if(!(webman_config->combo & C_FPSINFO) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == 0)) //  L3+R3 = FPS Counter
					{
						#ifdef WM_CUSTOM_COMBO
						if(do_custom_combo("l3_r3")) break;
						#endif
						if(syscalls_removed || is_mounting || refreshing_xml) {BEEP3; continue;}
						#ifdef FPS_OVERLAY
						if(overlay_enabled)
							disable_progress();
						else
						{
							unload_vsh_plugin("VshFpsCounter"); // unload any loaded VshFpsCounter
							show_msg2("FPS", STR_ENABLED);
							if(!webman_config->nobeep) play_sound_id(5); // trophy sound
						}
						#else
						if(!webman_config->nobeep) BEEP1;
						#endif
						if(isCobraDebug)
							toggle_vsh_plugin("/dev_hdd0/tmp/wm_res/VshFpsCounterM.sprx");
						else
							toggle_vsh_plugin("/dev_hdd0/tmp/wm_res/VshFpsCounter.sprx");
						sys_ppu_thread_sleep(3);
						break_and_wait;
					}
					#endif
					else if(!(webman_config->combo & GOTO_HOME) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2)) //  L2+L3+R3 / L3+R3+L2 / L3+L2+R3
					{
						// L3+R3+L2    = Go to webMAN Games
						// L3+R3+L2+R2 = Go to webMAN Games + Reload Game column
						bool reload_game = (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) == CELL_PAD_CTRL_R2;
						#ifdef WM_CUSTOM_COMBO
						if(do_custom_combo(reload_game ? "l3_r3_l2_r2" : "l3_r3_l2")) break;
						#endif
						goto_xmb_home(reload_game);
						break_and_wait;
					}
				}
				else
				if((pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == CELL_PAD_CTRL_L3) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2))
				{
					// L3+R2+X     = Shutdown
					// L3+R2+O     = Lpar restart

					if(!(webman_config->combo & SHUT_DOWN) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_CROSS))) // L3+R2+X (shutdown)
					{
						// L3+R2+X = power off
						working = 0;
						del_turnoff(1); // 1 beep

						vsh_shutdown();

						sys_ppu_thread_exit(0);
					}
					else if(!(webman_config->combo & RESTARTPS) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_CIRCLE))) // L3+R2+O (lpar restart)
					{
						// L3+R2+O = lpar restart
						working = 0;
						del_turnoff(2); // 2 beeps

						#ifdef COBRA_ONLY
						if(is_mamba)
							vsh_reboot();
						else
						#endif
							{system_call_3(SC_SYS_POWER, SYS_REBOOT, NULL, 0);}

						sys_ppu_thread_exit(0);
					}
				}
				else if(!(webman_config->combo & DISABLEFC) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == (CELL_PAD_CTRL_L3 | CELL_PAD_CTRL_START)))
				{
					if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2)) // L3+R2+L2+START (set PS2 fan mode)
					{
						// L3+L2+R2+START = Enable ps2 fan control mode
						restore_fan(SET_PS2_MODE);
					}
					else if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_L2) // L3+L2+START (enable auto #2)
					{
						// L3+L2+START = Enable Auto #2
						enable_fan_control(ENABLE_AUTO2);
					}
					else if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R2) // L3+R2+START (enable/disable fancontrol)
					{
						// L3+R2+START = Enable/disable fancontrol
						enable_fan_control(TOGGLE_MODE);
					}
					break_and_wait;
				}
				else
				if((pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == CELL_PAD_CTRL_L3) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R1))
				{
					// L3+R1+X = vsh shutdown
					// L3+R1+O = vsh restart

					if(!(webman_config->combo & SHUT_DOWN) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_R1 | CELL_PAD_CTRL_CROSS))) // L3+R1+X (vsh shutdown)
					{
						// L3+R1+X = vsh shutdown
						working = 0;
						del_turnoff(1); // 1 beep

						vsh_shutdown();

						sys_ppu_thread_exit(0);
					}

					if(!(webman_config->combo & RESTARTPS) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_R1 | CELL_PAD_CTRL_CIRCLE))) // L3+R1+O (vsh restart)
					{
						// L3+R1+O = vsh reboot
						working = 0;
						del_turnoff(2); // 2 beeps

						vsh_reboot();

						sys_ppu_thread_exit(0);
					}
				}
				else
				if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2)
				{
					// R2+L2+TRIANGLE = Toggle user/admin mode
					// R2+L2+O + [L1/R1/L1+R1]
					// R2+TRIANGLE = disable_cfw_syscalls
					// R2+SQUARE   = block_online_servers
					// R2+CIRCLE   = show_idps / abort copy/gamefix / enable_ingame_screenshot

					#ifdef SYS_ADMIN_MODE
					if((webman_config->combo & SYS_ADMIN) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_TRIANGLE) )) // L2+R2+TRIANGLE
					{
						// R2+L2+TRIANGLE = Toggle user/admin mode
						#ifdef WM_CUSTOM_COMBO
						if(do_custom_combo("l2_r2_triangle")) break;
		 				#endif
						{
							sys_admin ^= 1, pwd_tries = 0;

							show_status("ADMIN", sys_admin);

							if(sys_admin) { BEEP1 } else { BEEP2 }
						}
						break_and_wait;
					}
					else
					#endif
					if(!(webman_config->combo & SHOW_IDPS) && ( (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2 | CELL_PAD_CIRCLE_BTN)) == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2 | CELL_PAD_CIRCLE_BTN) ) && IS_ON_XMB) // L2+R2+O
					{
						// L2+R2+O + [L1/R1/L1+R1] = Open Browser file manager / cpursx / games / setup
						// L2+R2+X + [L1/R1/L1+R1] = Open Browser file manager / cpursx / games / setup (JAP)
						#ifdef WM_CUSTOM_COMBO
							 if(do_custom_combo("l2_r2_circle")) ;
						else if(do_custom_combo("l2_r2_l1_circle")) ;
						else if(do_custom_combo("l2_r2_r1_circle")) ;
						else if(do_custom_combo("l2_r2_l1_r1_circle")) ;
						else
						#endif
						{
							#ifdef PS3_BROWSER
							do_umount(false); // prevent system freeze on disc icon

							if( pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L1 | CELL_PAD_CTRL_R1 | CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2 | CELL_PAD_CIRCLE_BTN) )
								{open_browser("http://127.0.0.1/setup.ps3", 0); show_msg(STR_WMSETUP);}     // L2+R2+L1+R1+O  ||  L2+R2+L1+R1+X (JAP)
							else if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R1)
								{open_browser("http://127.0.0.1/index.ps3", 0); show_msg(STR_MYGAMES);}     // L2+R2+R1+O  ||  L2+R2+R1+X (JAP)
							else if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L1)
								{open_browser("http://127.0.0.1/cpursx.ps3", 0); show_msg(WM_APPNAME " Info");}  // L2+R2+L1+O || L2+R2+L1+X (JAP)
							else
								{open_browser("http://127.0.0.1/", 0); show_msg(WM_APP_VERSION);}           // L2+R2+O || L2+R2+X (JAP)
							#endif
						}
					}
					else
					if((copy_in_progress || fix_in_progress) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_R2 | CELL_PAD_CIRCLE_BTN)) )  // R2+O Abort copy process
					{
						fix_aborted = copy_aborted = true;
					}
					else
					if(!( webman_config->combo & DISABLESH ) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_TRIANGLE))) // R2+TRIANGLE Disable CFW Sycalls
					{
						#ifdef WM_CUSTOM_COMBO
						if(do_custom_combo("r2_triangle")) break;
						#endif
						#ifdef REMOVE_SYSCALLS
						disable_cfw_syscalls(webman_config->keep_ccapi);
						#endif
					}
					else
					if(!(webman_config->combo2 & CUSTOMCMB) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_SQUARE))) // R2+SQUARE
					{
						#ifdef WM_CUSTOM_COMBO
						if(do_custom_combo("r2_square")) break;
						#endif
						#ifdef WM_REQUEST
						if(do_custom_combo(WM_COMBO_PATH)) break;
						#endif
						block_online_servers(true);
					}
					else
					if(!(webman_config->combo & SHOW_IDPS) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_R2 | CELL_PAD_CIRCLE_BTN)) )  // R2+O Show IDPS EID0+LV2 (JAP)
					{
						#ifdef WM_CUSTOM_COMBO
						if(do_custom_combo("r2_circle")) break;
						#endif
						{
							char path[STD_PATH_LEN];
							set_param_sfo(path);

							refreshing_xml = 0; // cancel content scanning

							if(file_exists(path) || IS_INGAME)
							{
								char title[128], title_id[12], app_ver[8];

								get_game_version(path, title, title_id, app_ver);
								get_last_game(path);
								if(not_exists(path))
									concat2(path, HDD0_GAME_DIR, title_id);

								sprintf(msg, "ID: %s - v%s\n%s\n\n%s", title_id, app_ver, title, path);
								show_msg(msg);
								sys_ppu_thread_sleep(5);
							}
							else if(file_exists("/dev_bdvd/SYSTEM.CNF"))
							{
								read_file("/dev_bdvd/SYSTEM.CNF", (void*)msg, 200, 0);

								get_last_game(path);

								strcat(msg, "\n"); strcat(msg, path);
								show_msg(msg);
								sys_ppu_thread_sleep(5);
							}

							#ifdef OVERCLOCKING
							// Show RSX Core & VRAM clock speeds
							if(get_rsxclock(GPU_CORE_CLOCK))
							{
								show_rsxclock(msg);
								sys_ppu_thread_sleep(5);
							}
							#endif

							#ifdef SPOOF_CONSOLEID
							show_idps(msg);
							#endif

							#ifdef ARTEMIS_PRX
							if(webman_config->artemis && IS_INGAME)
							{
								if(attachedPID)
									show_msg2("Artemis", "Attached");
								else
									show_status("Artemis", artemis_working);
							}
							#endif

							// backup / restore act.bak -> act.dat
							backup_act_dat();
						}
					}
					break_and_wait;
				}
				else
				if((pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == CELL_PAD_CTRL_L3) && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2))
				{
					// L3+L2+TRIANGLE = COBRA Toggle
					// L3+L2+SQUARE   = REBUG Mode Switcher
					// L3+L2+CIRCLE   = Normal Mode Switcher
					// L3+L2+X        = DEBUG Menu Switcher

					if(!sys_admin || IS_INGAME) continue;
					#ifdef COBRA_NON_LITE
					if(!(webman_config->combo & DISACOBRA)
						&& (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_TRIANGLE)))
					{ // L3+L2+TRIANGLE COBRA Toggle
						reboot = toggle_cobra();
					}
					#endif // #ifdef COBRA_NON_LITE

					#ifdef REX_ONLY
					if(!(webman_config->combo2 & REBUGMODE)
						&& (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_SQUARE)))
					{ // L3+L2+SQUARE REBUG Mode Switcher
						reboot = toggle_rebug_mode();
					}
					else
					if(!(webman_config->combo2 & NORMAMODE)
						&& (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_CIRCLE)))
					{ // L3+L2+O Normal Mode Switcher
						reboot = toggle_normal_mode();
					}
					else
					if(!(webman_config->combo2 & DEBUGMENU)
						&& (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_CROSS)))
					{ // L3+L2+X DEBUG Menu Switcher
						toggle_debug_menu();
					}
					#endif // #ifdef REX_ONLY
				}
			}

			if(reboot)
			{
				sys_ppu_thread_sleep(1);
				// reboot
				show_msg("Switching successful! Reboot now...");
				sys_ppu_thread_sleep(3);
				disable_dev_blind();
#ifdef COBRA_NON_LITE
reboot:
#endif
				// vsh reboot
				working = 0;

				del_turnoff(0); // no beep
				create_file(WM_NOSCAN_FILE);

				vsh_reboot(); // VSH reboot

				sys_ppu_thread_exit(0);
			}
		}

		sys_ppu_thread_usleep(300000);

		if(show_persistent_popup)
		{
			show_persistent_popup++;
			if(show_persistent_popup > 10 && show_persistent_popup < PERSIST) show_persistent_popup = 0; else
			if(show_persistent_popup > PERSIST + 6) show_persistent_popup = PERSIST;
		}
	}

	if(working && (n < 10)) sys_ppu_thread_usleep((12 - n) * 150000);
}