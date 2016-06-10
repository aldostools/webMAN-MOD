/*
 FAIL SAFE    : SELECT+L3+L2+R2
 RESET SAFE   : SELECT+R3+L2+R2

 REFRESH XML  : SELECT+L3 (+R2=profile1, +L2=profile2)
 UNLOAD WM    : L3+R3+R2

 PLAY_DISC    : L2+START

 PREV GAME    : SELECT+L1                       *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_l1
 NEXT GAME    : SELECT+R1                       *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_r1
 UMNT_GAME    : SELECT+O (unmount)              *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_circle

 SHUTDOWN     : L3+R2+X
 SHUTDOWN  *2 : L3+R1+X (vsh shutdown)
 RESTART      : L3+R2+O (lpar restart)
 RESTART   *2 : L3+R1+O (vsh restart)

 FAN CNTRL    : L3+R2+START  (enable/disable fancontrol)
 SHOW TEMP    : SELECT+START (SELECT+START+R2 will show only copy progress) / SELECT+R3 (if rec video flag is disabled)
 DYNAMIC TEMP : SELECT+LEFT/RIGHT
 MANUAL TEMP  : SELECT+UP/DOWN

 REC VIDEO    : SELECT+R3
 XMB SCRNSHOT : L2+R2+SELECT+START

 SYSCALLS     : R2+TRIANGLE                     *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_r2_triangle
 SHOW IDPS    : R2+O  (Abort copy/fix process)  *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_r2_circle
 OFFLINE MODE : R2+□                            *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_r2_square

 EXT GAME DATA: SELECT+□                        *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_select_square
 MOUNT net0/  : SELECT+R2+□
 MOUNT net1/  : SELECT+L2+□

 TOGGLE PS2CLASSIC    : SELECT+L2+TRIANGLE
 SWITCH PS2EMU        : SELECT+L2+R2

 COBRA TOGGLE         : L3+L2+TRIANGLE
 REBUG  Mode Switcher : L3+L2+□
 Normal Mode Switcher : L3+L2+O
 DEBUG  Menu Switcher : L3+L2+X

 Open File Manager : L2+R2+O                    *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_l2_r2_circle
 Open Games List   : L2+R2+R1+O                 *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_l2_r2_r1_circle
 Open System Info  : L2+R2+L1+O                 *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_l2_r2_l1_circle
 Open Setup        : L2+R2+L1+R1+O              *or* Custom Combo -> /dev_hdd0/tmp/wm_combo/wm_custom_l2_r2_l1_r1_circle
*/
		bool reboot = false;

		u8 n;

		CellPadData data;

		#define PERSIST  100

		for(n = 0; n < 10; n++)
		{
			if(show_persistent_popup == PERSIST) {goto show_persistent_popup;}
			if(show_info_popup) {show_info_popup = false; goto show_popup;}

			if(!webman_config->nopad)
			{
#ifdef VIRTUAL_PAD
				if(vcombo)
				{
					data.len=16; data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] = (vcombo & 0xFF); data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] = (vcombo & 0xFF00) >> 8; vcombo = 0;
				}
				else
#endif
				{
					data.len=0;
					if(cellPadGetData(0, &data) != CELL_PAD_OK || data.len == 0)
						if(cellPadGetData(1, &data) != CELL_PAD_OK || data.len == 0)
							if(cellPadGetData(2, &data) != CELL_PAD_OK) {sys_timer_usleep(300000); continue;}
				}

				if(data.len > 0)
				{
					if(!(webman_config->combo2 & PLAY_DISC) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] == CELL_PAD_CTRL_START) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_L2))
					{
						char category[16] = "game", seg_name[80] = "seg_device";
						launch_disc(category, seg_name); // L2+START
						break;
					}

					if((data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_SELECT))
					{
						if( !(webman_config->combo2 & (EXTGAMDAT | MOUNTNET0 | MOUNTNET1))         // Toggle External Game Data
                            && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_SQUARE)) // SELECT+SQUARE
						{
#ifndef LITE_EDITION
							if(!(webman_config->combo2 & MOUNTNET0) &&
								(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2))
							{if(webman_config->netp0 && webman_config->neth0[0]) mount_with_mm((char*)"/net0", 1);}
							else
							if(!(webman_config->combo2 & MOUNTNET1) &&
								(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2))
							{if(webman_config->netp1 && webman_config->neth1[0]) mount_with_mm((char*)"/net1", 1);}
							else
#endif
							{
#ifdef WM_CUSTOM_COMBO
								if(file_exists(WM_CUSTOM_COMBO "select_square"))
								{
									filecopy(WM_CUSTOM_COMBO "select_square", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE); break;
								}
								else
#endif
								{
#ifdef EXT_GDATA
									set_gamedata_status(extgd^1, true);
#endif
									sys_timer_sleep(2);
									break;
								}
							}
						}
						else
						if( !(webman_config->combo & FAIL_SAFE) &&
							(data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_L3) &&                    // fail-safe mode
							(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2)) // SELECT+L3+L2+R2
							)
						{
							cellFsUnlink((char*)"/dev_hdd0/boot_plugins.txt");
							goto reboot; // vsh reboot
						}
						else
						if( (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_R3) &&                    // reset-safe mode
							(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2)) // SELECT+R3+L2+R2
							)
						{
							cellFsUnlink(WMCONFIG);
							{ BEEP1 }
							show_msg((char*)STR_RMVWMCFG);
							sys_timer_sleep(2);
							show_msg((char*)STR_RMVWMCFGOK);
							sys_timer_sleep(3);
							goto reboot; // vsh reboot
						}
#ifdef COBRA_ONLY
						else
						if( !(webman_config->combo2 & PS2TOGGLE)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_TRIANGLE) // SELECT+L2+TRIANGLE
							&& (c_firmware>=4.65f) )
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

							sprintf((char*) msg, (char*)"PS2 Classic %s", classic_ps2_enabled ? STR_DISABLED : STR_ENABLED);
							show_msg((char*) msg);
							sys_timer_sleep(3);
						}
						else
						if( !(webman_config->combo2 & PS2SWITCH)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2) // Clone ps2emu habib's switcher
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) // SELECT+L2+R2
							&& (c_firmware>=4.53f) )
						{
								toggle_ps2emu();
						}
#endif //#ifdef COBRA_ONLY

						else
						if(!(webman_config->combo2 & XMLREFRSH) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_L3) ) // SELECT+L3 refresh XML
						{
							if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R2) profile=1; else
							if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_L2) profile=2; else
							if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R1) profile=3; else
							if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_L1) profile=4; else profile=0;

							refresh_xml((char*)msg);
						}
                        else
						if( (!(webman_config->combo & SHOW_TEMP) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & (CELL_PAD_CTRL_R3 | CELL_PAD_CTRL_START)))) // SELECT+START show temperatures / hdd space
						{
#ifdef VIDEO_REC
							if(!(webman_config->combo2 & VIDRECORD) && data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_R3) // SELECT + R3
							{
								if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2)
								{
									rec_setting_to_change++; if(rec_setting_to_change>5) rec_setting_to_change = 0;
									set_setting_to_change(msg, (char*)"Change : ");

									strcat(msg, "\n\nCurrent recording format:");
									show_rec_format(msg);
								}
								else
								if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2)
								{
									set_setting_to_change(msg, (char*)"Changed : ");

									strcat(msg, "\n\nCurrent recording format:");
									if(rec_setting_to_change == 0)
									{
										rec_audio_format = CELL_REC_PARAM_AUDIO_FMT_AAC_64K;
										if(rec_video_format == CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS)   {rec_video_format = CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_2048K_30FPS; } else
										if(rec_video_format == CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_2048K_30FPS)  {rec_video_format = CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_768K_30FPS; rec_audio_format = CELL_REC_PARAM_AUDIO_FMT_PCM_768K;} else
										if(rec_video_format == CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_768K_30FPS) {rec_video_format = CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_512K_30FPS;  } else
										if(rec_video_format == CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_512K_30FPS)   {rec_video_format = CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_11000K_30FPS; rec_audio_format = CELL_REC_PARAM_AUDIO_FMT_AAC_96K;} else
																												  {rec_video_format = CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS;  }

										show_rec_format(msg);
									}
									if(rec_setting_to_change == 1) {rec_video_format += 0x1000; if((rec_video_format & 0xF000) > 0x4000) rec_video_format &= 0x0FFF;} else
									if(rec_setting_to_change == 2) {rec_video_format += 0x0100; if((rec_video_format & 0x0F00) > 0x0300) {rec_video_format += 0x0200; if((rec_video_format & 0x0F00) > 0x0600) rec_video_format &= 0xF0FF;}} else
									if(rec_setting_to_change == 3) {rec_video_format += 0x0010; if((rec_video_format & 0x00F0) > 0x0090) rec_video_format &= 0xFF0F; else if((rec_video_format & 0x00F0) == 0x0050) rec_video_format += 0x0010;} else
									if(rec_setting_to_change == 4) {rec_audio_format += 0x1000; if((rec_audio_format & 0xF000) > 0x2000) rec_audio_format &= 0x0FFF;} else
									if(rec_setting_to_change == 5) {rec_audio_format += 0x0001; if((rec_audio_format & 0x000F) > 0x0002) {rec_audio_format += 0x0004; if((rec_audio_format & 0x000F) > 0x0009) rec_audio_format &= 0xFFF0;}}

									show_rec_format(msg);
								}
								else
									{memset(msg, 0, 256); toggle_video_rec(msg); sys_timer_sleep(2);}

								break;
							}
#endif

#ifdef XMB_SCREENSHOT
							if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_L2) && View_Find("game_plugin")==0)
								{memset(msg, 0, 256); saveBMP(msg, true); sys_timer_sleep(2);} // L2 + R2 + SELECT + START
							else
#endif
							{
								if(show_persistent_popup == 0)        show_persistent_popup = 1;               else
								if(show_persistent_popup  < PERSIST) {BEEP1; show_persistent_popup = PERSIST;} else
								if(show_persistent_popup >= PERSIST) {BEEP2; show_persistent_popup = 0;}
show_persistent_popup:
								/////////////////////////////
#ifdef COPY_PS3
								if(copy_in_progress)
								{
									sprintf(msg, "%s %s (%i %s)", STR_COPYING, current_file, copied_count, STR_FILES);
									show_msg(msg);
									sys_timer_sleep(2);
									if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & (CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_L2) ) break;
								}
								else
								if(fix_in_progress)
								{
									sprintf((char*) msg, "%s %s", STR_FIXING, current_file);
									show_msg((char*) msg);
									sys_timer_sleep(2);
									if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & (CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_L2) ) break;
								}
#endif
								/////////////////////////////
show_popup:
								{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

								CellRtcTick pTick; u32 dd, hh, mm, ss; char tmp[256];

								cellRtcGetCurrentTick(&pTick);
								get_temperature(0, &t1);
								get_temperature(1, &t2);

								uint32_t blockSize;
								uint64_t freeSize;
								cellFsGetFreeSize((char*)"/dev_hdd0", &blockSize, &freeSize);

								u8 speed = fan_speed;
								if(fan_ps2_mode) speed=(int)(255.f*(float)(webman_config->ps2temp+1)/100.f); else
								if((webman_config->fanc==0) && (get_fan_policy_offset>0))
								{
									u8 st, mode, unknown;
									backup[2]=peekq(get_fan_policy_offset);
									lv2poke32(get_fan_policy_offset, 0x38600001); // sys 409 get_fan_policy
									sys_sm_get_fan_policy(0, &st, &mode, &fan_speed, &unknown);
									pokeq(get_fan_policy_offset, backup[2]);
									speed = fan_speed;
								}

								_meminfo meminfo;
								{system_call_1(SC_GET_FREE_MEM, (uint64_t)(u32) &meminfo);}

								// detect aprox. time when a game is launched
								if(View_Find("game_plugin")==0) gTick=rTick; else if(gTick.tick==rTick.tick) cellRtcGetCurrentTick(&gTick);

								bool R2 = (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R2), bb;

								///// startup/play time /////
								bb = (!R2 && gTick.tick>rTick.tick); // show play time
								ss = (u32)((pTick.tick-(bb?gTick.tick:rTick.tick))/1000000); dd = (u32)(ss / 86400);
								if(dd>100) {bb=false; ss = (u32)((pTick.tick-rTick.tick)/1000000); dd = (u32)(ss / 86400);}
								ss = ss % 86400; hh = (u32)(ss / 3600); ss = ss % 3600; mm = (u32)(ss / 60); ss = ss % 60;
								/////////////////////////////

								net_info info;
								memset(&info, 0, sizeof(net_info));
								xsetting_F48C0548()->sub_44A47C(&info);

								char net_type[8] = "";
								if (info.device == 0) strcpy(net_type, "LAN"); else
								if (info.device == 1) strcpy(net_type, "WLAN");

								int32_t ip_size = 0x10;
								char ip[ip_size];
								netctl_main_9A528B81(ip_size, ip);

								char cfw_info[20];
#ifdef COBRA_ONLY
								if(!is_mamba && !syscalls_removed) {system_call_1(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_GET_MAMBA); is_mamba = ((int)p1 ==0x666);}

								if(!cobra_version) sys_get_version2(&cobra_version);

								char cobra_ver[8];
								if((cobra_version & 0x0F) == 0)
									sprintf(cobra_ver, "%X.%X", cobra_version>>8, (cobra_version & 0xFF) >> 4);
								else
									sprintf(cobra_ver, "%X.%02X", cobra_version>>8, (cobra_version & 0xFF));
 #if defined(DECR_SUPPORT)
								sprintf(cfw_info, "%s %s: %s", (dex_mode == 1) ? "DECR" : dex_mode ? "DEX" : "CEX", is_mamba ? "Mamba" : "Cobra", cobra_ver);
 #elif defined(DEX_SUPPORT)
								sprintf(cfw_info, "%s %s: %s", dex_mode ? "DEX" : "CEX", is_mamba ? "Mamba" : "Cobra", cobra_ver);
 #else
								sprintf(cfw_info, "%s %s: %s", "CEX", is_mamba ? "Mamba" : "Cobra", cobra_ver);
 #endif
#elif DEX_SUPPORT
								sprintf(cfw_info, "%s", (dex_mode == 1) ? "DECR" : dex_mode ? "DEX" : "CEX");
#else
								sprintf(cfw_info, "CEX");
#endif
								char smax[32]; if(fan_ps2_mode) sprintf(smax, "   PS2 Mode"); else if(max_temp) sprintf(smax, "   MAX: %i°C", max_temp); else if(webman_config->fanc==0) sprintf(smax, "   SYSCON"); else memset(smax, 0, 16);

								sprintf((char*)tmp, "CPU: %i°C  RSX: %i°C  FAN: %i%%   \r\n"
													"%s: %id %02d:%02d:%02d%s\r\n"
													"Firmware : %i.%02i %s\r\n"
													"IP: %s  %s%s",
													t1>>24, t2>>24, (int)(((int)speed*100)/255),
													bb?"Play":"Startup", dd, hh, mm, ss, smax,
													(int)c_firmware, ((u32)(c_firmware * 1000.0f) % 1000) / 10, cfw_info, ip, net_type, syscalls_removed ? "  [noSC]" : "");

								sprintf((char*)msg, "%s\r\n%s: %'i %s\r\n"
													"%s: %'i %s\r\n", tmp,
													STR_STORAGE, (int)((blockSize*freeSize)>>20), STR_MBFREE,
													STR_MEMORY, meminfo.avail>>10, STR_KBFREE);

								if(R2 && gTick.tick>rTick.tick)
								{
									////// play time //////
									ss = (u32)((pTick.tick-gTick.tick)/1000000);
									dd = (u32)(ss / 86400); ss = ss % 86400; hh = (u32)(ss / 3600); ss = ss % 3600; mm = (u32)(ss / 60); ss = ss % 60;

									if(dd<100) {char gname[200]; get_game_info(); sprintf(gname, "%s %s\r\n\r\n", _game_TitleID, _game_Title); sprintf((char*) msg, "%sPlay: %id %02d:%02d:%02d\r\n%s", gname, dd, hh, mm, ss, tmp); }
								}

								{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }

								show_msg((char*) msg);
								sys_timer_sleep(2);
							}
						}
						else
						if(webman_config->fanc && !(webman_config->combo & MANUALFAN) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_UP) ) // SELECT+UP increase TEMP/FAN
						{
#ifdef WM_CUSTOM_COMBO
							if(file_exists(WM_CUSTOM_COMBO "select_up"))
							{
								filecopy(WM_CUSTOM_COMBO "select_up", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE); break;
							}
							else
#endif
							{
								if(webman_config->fanc==0) enable_fan_control(3, msg);

								if(max_temp) //auto mode
								{
									if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) max_temp+=5; else max_temp+=1;
									if(max_temp>85) max_temp=85;
									webman_config->temp1=max_temp;
									sprintf((char*) msg, "%s\r\n%s %i°C", STR_FANCH0, STR_FANCH1, max_temp);
								}
								else
								{
									if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) webman_config->manu+=5; else webman_config->manu+=1;
									webman_config->manu=RANGE(webman_config->manu, 20, 95); //%
									webman_config->temp0= (u8)(((float)(webman_config->manu+1) * 255.f)/100.f);
									webman_config->temp0=RANGE(webman_config->temp0, 0x33, MAX_FANSPEED);
									fan_control(webman_config->temp0, 0);
									sprintf((char*) msg, "%s\r\n%s %i%%", STR_FANCH0, STR_FANCH2, webman_config->manu);
								}
								save_settings();
								show_msg((char*) msg);
								sys_timer_sleep(2);
							}
						}
						else
						if(webman_config->fanc && !(webman_config->combo & MANUALFAN) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_DOWN) ) // SELECT+DOWN increase TEMP/FAN
						{
#ifdef WM_CUSTOM_COMBO
							if(file_exists(WM_CUSTOM_COMBO "select_down"))
							{
								filecopy(WM_CUSTOM_COMBO "select_down", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE); break;
							}
							else
#endif
							{
								if(webman_config->fanc==0) enable_fan_control(3, msg);

								if(max_temp) //auto mode
								{
									if(max_temp>30) {if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) max_temp-=5; else max_temp-=1;}
									webman_config->temp1=max_temp;
									sprintf((char*) msg, "%s\r\n%s %i°C", STR_FANCH0, STR_FANCH1, max_temp);
								}
								else
								{
									if(webman_config->manu>20) {if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) webman_config->manu-=5; else webman_config->manu-=1;}
									webman_config->temp0= (u8)(((float)(webman_config->manu+1) * 255.f)/100.f);
									if(webman_config->temp0<0x33) webman_config->temp0=0x33;
									if(webman_config->temp0>MAX_FANSPEED) webman_config->temp0=MAX_FANSPEED;
									fan_control(webman_config->temp0, 0);
									sprintf((char*) msg, "%s\r\n%s %i%%", STR_FANCH0, STR_FANCH2, webman_config->manu);
								}
								save_settings();
								show_msg((char*) msg);
								sys_timer_sleep(2);
							}
						}
						else
						if(webman_config->minfan && !(webman_config->combo & MINDYNFAN) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_LEFT) ) // SELECT+LEFT decrease Minfan
						{
#ifdef WM_CUSTOM_COMBO
							if(file_exists(WM_CUSTOM_COMBO "select_left"))
							{
								filecopy(WM_CUSTOM_COMBO "select_left", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE); break;
							}
							else
#endif
							{
								if(webman_config->fanc==0) enable_fan_control(3, msg);

								if(webman_config->minfan-5>=MIN_FANSPEED) webman_config->minfan-=5;
								sprintf((char*) msg, "%s\r\n%s %i%%", STR_FANCH0, STR_FANCH3, webman_config->minfan);

								save_settings();
								show_msg((char*) msg);
								sys_timer_sleep(2);
							}
						}
						else
						if(webman_config->minfan && !(webman_config->combo & MINDYNFAN) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_RIGHT) ) // SELECT+RIGHT increase Minfan
						{
#ifdef WM_CUSTOM_COMBO
							if(file_exists(WM_CUSTOM_COMBO "select_right"))
							{
								filecopy(WM_CUSTOM_COMBO "select_right", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE); break;
							}
							else
#endif
							{
								if(webman_config->fanc==0) enable_fan_control(3, msg);

								if(webman_config->minfan+5<100) webman_config->minfan+=5;
								sprintf((char*) msg, "%s\r\n%s %i%%", STR_FANCH0, STR_FANCH3, webman_config->minfan);

								save_settings();
								show_msg((char*) msg);
								sys_timer_sleep(2);
							}
						}
						else
						if(!(webman_config->combo & PREV_GAME) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_L1) ) // SELECT+L1 (previous title)
						{
#ifdef WM_CUSTOM_COMBO
							if(file_exists(WM_CUSTOM_COMBO "select_l1"))
							{
								filecopy(WM_CUSTOM_COMBO "select_l1", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE); break;
							}
							else
#endif
							{
								led(GREEN, BLINK_FAST);
								mount_with_mm((char*)"_prev", 1);
								sys_timer_sleep(3);
								led(GREEN, ON);
							}
						}
						else
						if(!(webman_config->combo & NEXT_GAME) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R1) ) // SELECT+R1 (next title)
						{
#ifdef WM_CUSTOM_COMBO
							if(file_exists(WM_CUSTOM_COMBO "select_r1"))
							{
 								filecopy(WM_CUSTOM_COMBO "select_r1", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE); break;
							}
							else
#endif
							{
 								led(GREEN, BLINK_FAST);
								mount_with_mm((char*)"_next", 1);
								sys_timer_sleep(3);
								led(GREEN, ON);
							}
						}
						else
						if(!(webman_config->combo & UMNT_GAME) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_CIRCLE) ) // SELECT+O (unmount)
						{
#ifdef WM_CUSTOM_COMBO
							if(file_exists(WM_CUSTOM_COMBO "select_circle"))
							{
								filecopy(WM_CUSTOM_COMBO "select_circle", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE); break;
							}
							else
#endif
								do_umount(true);
						}
#ifdef WM_CUSTOM_COMBO
						else
						if(!(webman_config->combo & UMNT_GAME) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_TRIANGLE) ) // SELECT+TRIANGLE
						{
							if(file_exists(WM_CUSTOM_COMBO "select_triangle"))
							{
								filecopy(WM_CUSTOM_COMBO "select_triangle", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE); break;
							}
						}
#endif
					}
					else
					if((data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_L3) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2))
					{
						if(!(webman_config->combo & SHUT_DOWN) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CROSS) ) // L3+R2+X (shutdown)
						{
							// power off
							working = 0;
							{ DELETE_TURNOFF } { BEEP1 }

							{system_call_4(SC_SYS_POWER, SYS_SHUTDOWN, 0, 0, 0);}

							sys_ppu_thread_exit(0);
						}
						else if(!(webman_config->combo & RESTARTPS) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CIRCLE) ) // L3+R2+O (lpar restart)
						{
							// lpar restart
							working = 0;
							{ DELETE_TURNOFF }{ BEEP2 }

							{system_call_3(SC_SYS_POWER, SYS_REBOOT, NULL, 0);}

							sys_ppu_thread_exit(0);
						}
						else if(!(webman_config->combo & UNLOAD_WM) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_R3) ) // L3+R3+R2 (quit webMAN)
						{
#ifdef COBRA_ONLY
							get_vsh_plugin_slot_by_name((char *)"VSH_MENU", true); // unload vsh menu
#endif
							if(!webman_config->fanc || webman_config->ps2temp<33)
								restore_fan(0); //restore syscon fan control mode
							else
								restore_fan(1); //set ps2 fan control mode

							working = 0;
							wm_unload_combo = 1;

							stop_prx_module();
							sys_ppu_thread_exit(0);
							break;
						}
						else if(!(webman_config->combo & DISABLEFC) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_START) ) // L3+R2+START (enable/disable fancontrol)
						{
							enable_fan_control(2, msg);
							sys_timer_sleep(2);
							break;
						}
					}
					else
					if((data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_L3) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R1))
					{
						if(!(webman_config->combo & SHUT_DOWN) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CROSS)) // L3+R1+X (vsh shutdown)
						{
							// vsh shutdown
							working = 0;
							{ DELETE_TURNOFF }{ BEEP1 }

							vshmain_87BB0001(1); // VSH shutdown

							sys_ppu_thread_exit(0);
						}
						else
						if(!(webman_config->combo & RESTARTPS) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CIRCLE)) // L3+R1+O (vsh restart)
						{
							// vsh reboot
							working = 0;
							{ DELETE_TURNOFF }{ BEEP2 }

							vshmain_87BB0001(2); // VSH reboot

							sys_ppu_thread_exit(0);
						}
					}
					else
					if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2)
					{
						if(!(webman_config->combo & SHOW_IDPS) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_CIRCLE))==(CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_CIRCLE) && View_Find("game_plugin")==0) // L2+R2+O
						{
#ifdef WM_CUSTOM_COMBO
							if(file_exists(WM_CUSTOM_COMBO "l2_r2_circle"))
							{
								filecopy(WM_CUSTOM_COMBO "l2_r2_circle", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE);
							}
							else
							if(file_exists(WM_CUSTOM_COMBO "l2_r2_l1_circle"))
							{
								filecopy(WM_CUSTOM_COMBO "l2_r2_l1_circle", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE);
							}
							else
							if(file_exists(WM_CUSTOM_COMBO "l2_r2_r1_circle"))
							{
								filecopy(WM_CUSTOM_COMBO "l2_r2_r1_circle", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE);
							}
							else
							if(file_exists(WM_CUSTOM_COMBO "l2_r2_l1_r1_circle"))
							{
								filecopy(WM_CUSTOM_COMBO "l2_r2_l1_r1_circle", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE);
							}
							else
#endif
							{
#ifdef PS3_BROWSER
								do_umount(false); // prevent system freeze on disc icon

								if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L1 | CELL_PAD_CTRL_R1 | CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_CIRCLE))
									{vshmain_AE35CF2D((char*)"http://127.0.0.1/setup.ps3", 0); show_msg((char*)STR_WMSETUP);}     // L2+R2+L1+R1+O
								else if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R1)
									{vshmain_AE35CF2D((char*)"http://127.0.0.1/index.ps3", 0); show_msg((char*)STR_MYGAMES);}     // L2+R2+R1+O
								else if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L1)
									{vshmain_AE35CF2D((char*)"http://127.0.0.1/cpursx.ps3", 0); show_msg((char*)"webMAN Info");}  // L2+R2+L1+O
								else
									{vshmain_AE35CF2D((char*)"http://127.0.0.1/", 0); show_msg((char*)"webMAN " WM_VERSION);}     // L2+R2+O
#endif
								sys_timer_sleep(3);
							}
							break;
						}
						if((copy_in_progress || fix_in_progress) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CIRCLE)) // R2+O Abort copy process
						{
							fix_aborted=copy_aborted=true;
						}
						else
						if(!(webman_config->combo & DISABLESH) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_TRIANGLE) ) // R2+TRIANGLE Disable CFW Sycalls
						{
#ifdef WM_CUSTOM_COMBO
							if(file_exists(WM_CUSTOM_COMBO "r2_triangle"))
							{
								filecopy(WM_CUSTOM_COMBO "r2_triangle", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE); break;
							}
							else
#endif
							{
#ifdef REMOVE_SYSCALLS
								disable_cfw_syscalls(webman_config->keep_ccapi);
#endif
							}
						}
						else
						if(!(webman_config->combo2 & CUSTOMCMB) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_SQUARE) ) // R2+SQUARE
						{
#ifdef WM_CUSTOM_COMBO
							if(file_exists(WM_CUSTOM_COMBO "r2_square"))
							{
								filecopy(WM_CUSTOM_COMBO "r2_square", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE); break;
							}
							else
#endif
#ifdef WM_REQUEST
							if(file_exists("/dev_hdd0/tmp/wm_custom_combo"))
							{
								filecopy("/dev_hdd0/tmp/wm_custom_combo", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE); break;
							}
							else
#endif
							{
								block_online_servers(true);
							}
						}
						else
						if(!(webman_config->combo & SHOW_IDPS) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CIRCLE) ) // R2+O Show IDPS EID0+LV2
						{
#ifdef WM_CUSTOM_COMBO
							if(file_exists(WM_CUSTOM_COMBO "r2_circle"))
							{
								filecopy(WM_CUSTOM_COMBO "r2_circle", (char*)"/dev_hdd0/tmp/wm_request", COPY_WHOLE_FILE); break;
							}
							else
#endif
							{
								enable_ingame_screenshot();

								show_idps(msg);
							}
						}
					}
					else
					if((data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_L3) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2))
					{
#ifdef COBRA_ONLY
						if(!(webman_config->combo & DISACOBRA)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_TRIANGLE))
						{ // L3+L2+TRIANGLE COBRA Toggle
							reboot = toggle_cobra();
						}
#endif //#ifdef COBRA_ONLY

#ifdef REX_ONLY
						if(!(webman_config->combo2 & REBUGMODE)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_SQUARE))
						{ // L3+L2+□ REBUG Mode Switcher
							reboot = toggle_rebug_mode();
						}
						else
						if(!(webman_config->combo2 & NORMAMODE)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CIRCLE))
						{ // L3+L2+O Normal Mode Switcher
							reboot = toggle_normal_mode();
						}
						else
						if(!(webman_config->combo2 & DEBUGMENU)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CROSS))
						{ // L3+L2+X DEBUG Menu Switcher
							toggle_debug_menu();
						}
#endif //#ifdef REX_ONLY
					}
				}

				if(reboot)
				{
					sys_timer_sleep(1);
					// reboot
					show_msg((char*)"Switching successful! Reboot now...");
					sys_timer_sleep(3);
					{system_call_3(SC_FS_UMOUNT, (u64)(char*)"/dev_blind", 0, 1);}
reboot:
					// vsh reboot
					working = 0;
					{ DELETE_TURNOFF }
					savefile((char*)WMNOSCAN, NULL, 0);

					vshmain_87BB0001(2); // VSH reboot

					sys_ppu_thread_exit(0);
				}

			}
			//sys_timer_sleep(step);
			sys_timer_usleep(300000);

			if(show_persistent_popup)
			{
				show_persistent_popup++;
				if(show_persistent_popup > 10 && show_persistent_popup < PERSIST) show_persistent_popup = 0; else
				if(show_persistent_popup > PERSIST + 6) show_persistent_popup = PERSIST;
			}
		}

		if(n<10) sys_timer_usleep((11-n)*150000);
