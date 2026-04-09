#define SYS_NET_EURUS_POST_COMMAND		(726)
#define CMD_GET_MAC_ADDRESS				0x103f

#define FAHRENHEIT(celsius)	((int)(1.8f * (float)celsius + 32.f))

/*
static u32 in_cobra(u32 *mode)
{
	system_call_2(SC_COBRA_SYSCALL8, (u32) 0x7000, (u32)mode);
	return_to_user_prog(u32);
}
*/

static s32 sys_sm_request_be_count(s32 *arg_1, s32 *total_time_in_sec, s32 *power_on_ctr, s32 *power_off_ctr)
{
	system_call_4(0x187, (u32)arg_1, (u32)total_time_in_sec, (u32)power_on_ctr, (u32)power_off_ctr);
	return_to_user_prog(s32);
}

static void sys_get_cobra_version(void)
{
	#ifdef COBRA_ONLY
	if(payload_ps3hen)
		{system_call_1(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_HEN_REV); cobra_version = (int)p1;}
	else
	{
		if(!is_mamba) {system_call_1(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_GET_MAMBA); is_mamba = ((int)p1 == 0x666);}
		sys_get_version2(&cobra_version);
	}
	#endif
}

static void get_cobra_version(char *cfw_info)
{
	// returns cfw_info[32]

#ifdef COBRA_ONLY
	syscalls_removed = CFW_SYSCALLS_REMOVED(TOC);
	if(!syscalls_removed) disable_signin_dialog();

	if(!cobra_version && !syscalls_removed) sys_get_cobra_version();

	char cobra_ver[12];
	if((cobra_version & 0x0F) == 0)
		sprintf(cobra_ver, "%X.%X", cobra_version>>8, (cobra_version & 0xF0) >> 4);
	else
		sprintf(cobra_ver, "%X.%02X", cobra_version>>8, (cobra_version & 0xFF));

	if(payload_ps3hen) {sprintf(cfw_info, "%s %s %.3s.%X", dex_mode ? "DEX" : "CEX", "PS3HEN", cobra_ver, cobra_version & 0xF); return;}

	#if defined(DECR_SUPPORT)
		sprintf(cfw_info, "%s %s %s", IS_DEH ? "DECR" : dex_mode ? "DEX" : "CEX", is_qcfw ? "qCFW" : is_mamba ? "Mamba" : "Cobra", cobra_ver);
		if(pex_mode) *cfw_info = 'P';
	#elif defined(DEX_SUPPORT)
		sprintf(cfw_info, "%s %s %s", dex_mode ? "DEX" : "CEX", is_qcfw ? "qCFW" : is_mamba ? "Mamba" : "Cobra", cobra_ver);
		if(pex_mode) *cfw_info = 'P';
	#else
		sprintf(cfw_info, "%s %s %s", "CEX", is_qcfw ? "qCFW" : is_mamba ? "Mamba" : "Cobra", cobra_ver);
	#endif

	if(!cobra_version) {char *cfw = strchr(cfw_info, ' '); *cfw = NULL;}
#elif defined(DEX_SUPPORT)
	#if defined(DECR_SUPPORT)
		strcopy(cfw_info, IS_DEH ? "DECR" : dex_mode ? "DEX" : "CEX");
	#else
		strcopy(cfw_info, dex_mode ? "DEX" : "CEX");
	#endif
	if(pex_mode) *cfw_info = 'P';
#else
		strcopy(cfw_info, "CEX");
#endif
#ifndef COBRA_ONLY
		strcopy(cfw_info, " nonCobra");
#endif

	// noBD LV1 4.75 - 4.92
	if(isNOBD)  // ori: 0x78630020409E0018ULL
		strcat(cfw_info, " noBD");
#ifdef COBRA_ONLY
	else if(isCobraDebug)
		strcat(cfw_info, "D");
#endif
}

static void get_net_info(char *net_type, char *ip)
{
	// returns net_type[8], ip[ip_size]

	s32 status = 0; xnet()->GetSettingNet_enable(&status);

	if(status == 0) {strcpy(net_type, "OFFLINE"); *ip = NULL; return;}

	net_info info;
	_memset(&info, sizeof(net_info));
	xnet()->sub_44A47C(&info); //info.ipAddress

	if (info.device == 0) strcpy(net_type, "LAN"); else
	if (info.device == 1) strcpy(net_type, "WLAN");

	netctl_main_9A528B81(ip_size, ip);
}

static void poll_start_play_time(void);
static void get_sys_info(char *msg, u8 op, bool nolabel)
{
	s32 arg_1, total_time_in_sec, power_on_ctr, power_off_ctr;
	u8 t1 = 0, t2 = 0, ndx = 0; bool R2 = (op % 20) == 1 || (op == 23);

	{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

	CellRtcTick pTick; u32 dd, hh, mm, ss;

	cellRtcGetCurrentTick(&pTick);

	if(op < 20)
	{
		u8 st, mode, unknown;
		sys_sm_get_fan_policy(0, &st, &mode, &fan_speed, &unknown);
	}

	get_meminfo();

	// detect aprox. time when a game is launched
	poll_start_play_time();

	///// startup/play time /////
	bool bb = (!R2 && gTick.tick > rTick.tick); // show play time
	ss = (u32)((pTick.tick - (bb ? gTick.tick : rTick.tick)) / 1000000); if(ss > 864000) ss = 0;
	if(op == 23)
	{
		// get runtime data by @3141card
		sys_sm_request_be_count(&arg_1, &total_time_in_sec, &power_on_ctr, &power_off_ctr);
		ss += (u32)total_time_in_sec;
	}
	dd = (u32)(ss / 86400); ss %= 86400; hh = (u32)(ss / 3600); ss %= 3600; mm = (u32)(ss / 60); ss %= 60;
	/////////////////////////////

	char net_type[8] = "", ip[ip_size] = "-";
	get_net_info(net_type, ip);

	char cfw_info[32];
	get_cobra_version(cfw_info);

	char fan_mode[24];
	if(fan_ps2_mode || ps2_classic_mounted)
		strcopy(fan_mode, "   PS2 Mode");
	else if(webman_config->fanc == FAN_AUTO2)
		strcopy(fan_mode, "   MAX: AUTO");
	else if(max_temp)
		sprintf(fan_mode, "   MAX: %i°C", max_temp);
	else if(webman_config->fanc == DISABLED)
		strcopy(fan_mode, "   SYSCON");
	else
		_memset(fan_mode, sizeof(fan_mode));

	get_temperature(0, &t1); // CPU
	get_temperature(1, &t2); // RSX

	const char *RSX = " RSX:";
	if(webman_config->lang == 99) RSX = "/";

	char days[6]; *days = NULL;
	if(dd) sprintf(days, "%id ", dd);

	syscalls_removed = CFW_SYSCALLS_REMOVED(TOC);
	if(!syscalls_removed) disable_signin_dialog();

	u16 len =
	sprintf(msg, "CPU: %i°C %s %i°C  FAN: %i%%   \n"
				 "%s: %s%02d:%02d:%02d%s\n"
				 "%s : %s %s\n"
				 "IP: %s  %s  %s\n",
				 t1, RSX, t2, (int)(((int)fan_speed*100)/255),
				 bb ? "Play" : "Startup", days, hh, mm, ss, fan_mode,
				 STR_FIRMWARE, fw_version, cfw_info, ip, net_type, syscalls_removed ? "[noSC]" :
					  (webman_config->combo & SYS_ADMIN) ? (sys_admin ? "[ADMIN]":"[USER]") : "");

	if(op >= 10)
	{
		#ifndef LITE_EDITION
		if(op == 18) // syscalls (@info8)
			concat_text(msg, "Syscalls:", syscalls_removed ? STR_DISABLED : STR_ENABLED);
		if(op == 19) // temp (@info9)
			{char *p = strchr(msg, '\n'); _memset(p, len-30); return;}
		if(op == 20) // fan mode (@info10)
			{strcopy(msg, fan_mode + 3); return;}
		#endif
		if(op == 21) // Startup time (@info11)
			sprintf(msg, "%s: %s%02d:%02d:%02d", "Startup", days, hh, mm, ss);
		if(op == 22) // Play time (@info12)
			sprintf(msg, "%s: %s%02d:%02d:%02d", "Play", days, hh, mm, ss);
		if(op == 23) // Runtime (@info13)
		{
			len = sprintf(msg, "%s: %s%02d:%02d:%02d", "Runtime", days, hh, mm, ss);
			sprintf(msg + len, " • %'i ON • %'i OFF (%i)", power_on_ctr, power_off_ctr, power_on_ctr - power_off_ctr);
		}
		#ifndef LITE_EDITION
		if(op == 24) // Time (@info14)
		{
			CellRtcDateTime t;
			cellRtcGetCurrentClockLocalTime(&t);
			sprintf(msg, "%d/%d %02d:%02d:%02d", t.month, t.day, t.hour, t.minute, t.second);
			return;
		}
		if(op == 25) // id+title (@info15)
			{get_game_info(); concat_text(msg, _game_TitleID, _game_Title); return;}
		#ifdef COBRA_ONLY
		if(op == 26) // pid (@info16)
			sprintf(msg, "PID: 0x%x", get_current_pid());
		#endif
		#endif
		#ifdef SPOOF_CONSOLEID
		if(op == 27) // psid (@info17)
			sprintf(msg, "%s: %016llX%016llX", "PSID LV2 ", PSID[0], PSID[1]);
		if(op == 28) // idps (@info18)
			sprintf(msg, "%s: %016llX%016llX", "IDPS LV2 ", IDPS[0], IDPS[1]);
		if(op == 29) // idps eid0  (@info19)
			sprintf(msg, "%s: %016llX%016llX", "IDPS EID0", eid0_idps[0], eid0_idps[1]);
		#endif
		if(op == 30) // fw  (@info20)
			sprintf(msg, "%s: %s %s", STR_FIRMWARE, fw_version, cfw_info);
		if(op == 31) // mac (@info21)
		{
			u8 mac_address[0x13];
			{system_call_3(SYS_NET_EURUS_POST_COMMAND, CMD_GET_MAC_ADDRESS, (u64)(u32)mac_address, 0x13);}
			sprintf(msg, "MAC Addr : %02X:%02X:%02X:%02X:%02X:%02X", mac_address[13], mac_address[14], mac_address[15], mac_address[16], mac_address[17], mac_address[18]);
		}
		#ifndef LITE_EDITION
		if(op == 32) // ip (@info22)
			sprintf(msg, "%s: %s %s", "IP", ip, net_type);
		if(op == 33) // home (@info23)
			sprintf(msg, "%s: %s/%08i", STR_HOME, HDD0_HOME_DIR, xusers()->GetCurrentUserNumber());
		#endif
		if(op == 34) // wm version (@info24)
			sprintf(msg, "%s MO: %.7s %s", WM_APPNAME, WM_VERSION, EDITION);

		#ifdef BDINFO
		if(op == 35) // bdinfo (@info25)
			get_bdvd_info("", msg);
		#endif

		#ifdef OVERCLOCKING
		if(op == 36) // GPU Core Clock speed (@info26)
		{
			sprintf(msg, "GPU: %i Mhz", get_rsxclock(GPU_CORE_CLOCK));
		}
		else if(op == 37) // GPU VRAM Clock speed (@info27)
		{
			sprintf(msg, "VRAM: %i Mhz", get_rsxclock(GPU_VRAM_CLOCK));
		}
		#endif

		if(op >= 18)
		{
			if(nolabel)
			{
				char *sep = strchr(msg, ':');
				if(sep) strcopy(msg, sep + 2);
			}
			else if(op == 34) replace_char(msg, ':', 'D');
			return;
		}
		// op: 10=hdd0, 11=usb0, 12=usb1, 13=usb2, 14=usb3, 15=ntfs0
		ndx = op % 10; if(ndx == 5) ndx = NTFS;
	}

	if(R2 && (gTick.tick > rTick.tick))
	{
		////// play time //////
		ss = (u32)((pTick.tick-gTick.tick)/1000000); if(ss > 864000) ss = 0;
		dd = (u32)(ss / 86400); ss %= 86400; hh = (u32)(ss / 3600); ss %= 3600; mm = (u32)(ss / 60); ss %= 60;

		if(dd < 100)
		{
			get_game_info(); char tmp[200]; strcpy(tmp, msg);
			*days = NULL; if(dd) sprintf(days, "%id ", dd);
			snprintf(msg, 200,  "%s %s\n\n"
								"Play: %s%02d:%02d:%02d\n"
								"%s", _game_TitleID, _game_Title, days, hh, mm, ss, tmp);
		}
	}
	else
	{
		int mem_free = (int)(meminfo.avail>>10);
		char hdd_free[40];
		free_size(drives[ndx], hdd_free);
		sprintf(msg + len,  "%s: %s\n"
							"%s: %i %s %s\n",
							STR_STORAGE, hdd_free,
							STR_MEMORY,  mem_free, STR_KBFREE, _EDITION);

		#ifndef LITE_EDITION
		if(op >= 10 && op <= 15) // storage (@info0 - @info5: 0=hdd0, 1=usb0, 2=usb1, 3=usb2, 4=usb3, 5=ntfs0)
		{
			if(nolabel)
			{
				u64 siz = get_free_space(drives[ndx]);
				sprintf(msg, "%llu", siz);
			}
			else
				sprintf(msg, "%s: %s", drives[ndx], hdd_free);
		}
		if(op == 16) // mem free (@info6)
		{
			if(nolabel)
				sprintf(msg, "%i", meminfo.avail);
			else
				sprintf(msg, "%s: %i %s", STR_MEMORY,  mem_free, STR_KBFREE);
		}
		if(op == 17) // mem_usage (@info7)
		{
			int mem_total = (int)(meminfo.total>>10);
			int mem_usage = (int)((meminfo.total - meminfo.avail)>>10);
			if(nolabel)
				sprintf(msg, "%i", (meminfo.total - meminfo.avail));
			else
				sprintf(msg, "%s: %i%% %i %s / %i %s", STR_MEMORY, (int)(100 * mem_usage / mem_total), mem_usage, STR_KILOBYTE, mem_total, STR_KILOBYTE);
		}
		#endif
	}

	{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }
}

static void get_cpursx(char *cpursx)
{
	u8 t1 = 0, t2 = 0;
	get_temperature(0, &t1); // CPU // 3E030000 -> 3E.03°C -> 62.(03/256)°C
	get_temperature(1, &t2); // RSX

	sprintf(cpursx, "CPU: %i°C | RSX: %i°C", t1, t2);
}

static void qr_code(char *html, const char *url, const char *prefix, bool small, char *buffer)
{
	u16 len, sz = snprintf(html, 160,	"%s<div id='qrcode'%s></div>"
										"<script src='/xmlhost/game_plugin/qrcode.min.js'></script>"
										"<script>new QRCode(document.getElementById('qrcode'), \"", prefix, small ? " style='zoom:0.5'" : "");
	char *text = html + sz;

	if(*url == '/')
	{
		char net_type[8] = "", ip[ip_size] = "-"; get_net_info(net_type, ip);
		len = snprintf(text, 800, "http://%s%s", ip, url);
	}
	else if(*url == '?')
		len = strncopy(text, 800, url + 1);
	else
		len = strncopy(text, 800, url);

	strcpy(text + len, "\");</script><br>");

	if(small)
		concat(buffer, html);
	else
	{
		sz = strcopy(buffer, html);

		// add caption
		text[len] = 0;
		strcat(buffer + sz, text);
	}
}

static void add_game_info(char *buffer, char *html, u8 is_cpursx)
{
	if(IS_INGAME)
	{
		if(!is_cpursx && sys_admin)
		{
			if(GetCurrentRunningMode() == 1)
			{
			#ifdef GET_KLICENSEE
				concat(buffer, " [<a href=\"/klic.ps3\">KLIC</a>]");
			#endif
			#ifdef SYS_BGM
			concat(buffer, " [<a href=\"/sysbgm.ps3\">BGM</a>]");
				#endif
			#ifdef VIDEO_REC
				concat(buffer, " [<a href=\"/videorec.ps3\">REC</a>]");
			#endif
			#ifdef ARTEMIS_PRX
				if(webman_config->artemis) concat(buffer, " [<a href=\"/artemis.ps3\">Artemis</a>]");
			#endif
				concat(buffer, " [<a href=\"/xmb.ps3$reloadgame\">Reload</a>]");
			}
			concat(buffer, " [<a href=\"/xmb.ps3$exit\">Exit</a>]");
		}

		if(!is_cpursx)
		{
			if(syscalls_removed)
				{sprintf(html, "%s<h1>%s</h1>", HTML_RED_SEPARATOR, STR_CFWSYSALRD); concat(buffer, html);}
			else if(!cobra_version)
				{sprintf(html, "%s<h1>%s %s</h1>", HTML_RED_SEPARATOR, "Cobra", STR_DISABLED); concat(buffer, html);}
		}

		get_game_info();

		if(strlen(_game_TitleID) == 9)
		{
			sprintf(html, "<hr><span style=\"position:relative;top:-20px;\"><H2><a href=\"%s/%s/%s-ver.xml\" target=\"_blank\">%s</a>", "https://a0.ww.np.dl.playstation.net/tpl/np", _game_TitleID, _game_TitleID, _game_TitleID); concat(buffer, html);

			char *title = html + 0x10, *title_id = html;
			char path[MAX_PATH_LEN], app_ver[8];

			set_param_sfo(path);
			get_game_version(path, title, title_id, app_ver);
			remove_filename(path);

			sprintf(html, " <a href=\"%s%s\">%s %s</a> &nbsp; ", search_url, _game_Title, _game_Title, app_ver); concat(buffer, html);

			#ifdef PS3MAPI
			sprintf(html,	"<a href=\"%s\"><img src=\"%s/ICON0.PNG\" height=\"60\" border=0%s></a> "
							"<a href=\"/%s.ps3mapi?proc=0x%x\"><small>pid=%08x</small></a>",
					path, path, " style=\"position:relative;top:20px;\"", (is_cpursx < 3) ? "gameplugin" : "getmem",  GetGameProcessID(), GetGameProcessID()); concat(buffer, html);
			#else
			sprintf(html,	"<a href=\"%s\"><img src=\"%s/ICON0.PNG\" height=\"60\" border=0%s></a> ",
					path, path, " style=\"position:relative;top:20px;\""); concat(buffer, html);
			#endif

			concat(buffer, "</H2></span>");
		}
	}
	else if(!is_cpursx)
	{
		if(syscalls_removed)
			{sprintf(html, "%s<h1>%s</h1>", HTML_RED_SEPARATOR, STR_CFWSYSALRD); concat(buffer, html);}
		else if(!cobra_version)
			{sprintf(html, "%s<h1>%s %s</h1>", HTML_RED_SEPARATOR, "Cobra", STR_DISABLED); concat(buffer, html);}
	}
}

static void cpu_rsx_stats(char *buffer, char *html, char *param, u8 is_ps3_http)
{
	{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

	u8 t1 = 0, t2 = 0, t1f, t2f;
	get_temperature(0, &t1); // CPU // 3E030000 -> 3E.03°C -> 62.(03/256)°C
	get_temperature(1, &t2); // RSX

	t1f = FAHRENHEIT(t1);
	t2f = FAHRENHEIT(t2);

	get_meminfo();

	if((webman_config->fanc == DISABLED) && (get_fan_policy_offset > 0))
	{
		u8 st, mode, unknown;
		sys_sm_get_fan_policy(0, &st, &mode, &fan_speed, &unknown);

		if(strstr(param, "?u") || strstr(param, "?d")) enable_fan_control(ENABLE_SC8);
	}

#ifdef SPOOF_CONSOLEID
	get_eid0_idps();
#endif

	if(sys_admin && !webman_config->sman && !strstr(param, "/sman.ps3"))
	{
		sprintf(html, " [<a href=\"/shutdown.ps3\">%s</a>] [<a href=\"/restart.ps3\">%s</a>]", STR_SHUTDOWN, STR_RESTART ); concat(buffer, html);
	}

	add_game_info(buffer, html, 0); *html = 0;

#ifdef COPY_PS3
	if(ftp_state)
	{
		sprintf(html, "<font size=2>FTP: %s %s</font>", (ftp_state == 1) ? "Sending " : "Receiving ", current_file);
	}
	else
	if(copy_in_progress)
	{
		get_copy_stats(html + 100, "");
		sprintf(html, "<font size=2><a href=\"%s$abort\">&#9746 %s</a> %s</font>", "/copy.ps3", STR_COPYING, html + 100);
	}
	else if(fix_in_progress)
	{
		sprintf(html + 100, "%s (%i %s)", current_file, fixed_count, STR_FILES);
		sprintf(html, "<font size=2><a href=\"%s$abort\">&#9746 %s</a> %s</font>", "/fixgame.ps3", STR_FIXING, html + 100);
	}
	else
#endif
	if(IS_ON_XMB && ((View_Find("game_plugin") != 0) || (View_Find("download_plugin") != 0)) )
	{
		sprintf(html,	"<font size=2>&starf; Status: %s %s</font>",
						View_Find("download_plugin") ? "Downloading file" : "",
						View_Find("game_plugin")     ? "Installing PKG"   : "");
	}
	else if( games_found )
	{
		sprintf(html, "<font size=2>%s%s: %'i %s</font>", STR_SCAN2, SUFIX2(profile), games_found, STR_GAMES);
	}
	/*else
	{
		unsigned int real_disctype, effective_disctype, iso_disctype;
		cobra_get_disc_type(&real_disctype, &effective_disctype, &iso_disctype);
		sprintf(html, "<font size=2>Disc Type: real=%i, effective=%i, iso=%i</font><br>", real_disctype, effective_disctype, iso_disctype);
	}*/

	if(*html) concat(buffer, html);

	if(strchr(param, '?'))
	{
		const char *pos = strstr(param, "fan=");  // 0 = SYSCON, 1 = DYNAMIC, 2 = FAN_AUTO2
		if(pos)
		{
			u32 new_speed = get_valuen(param, "fan=", 0, webman_config->maxfan); max_temp = 0;
			if(new_speed <= ENABLED)
			{
				webman_config->fanc = new_speed;
				enable_fan_control(new_speed);
			}
			else if(new_speed == FAN_AUTO2)
			{
				webman_config->fanc = FAN_AUTO2;
				enable_fan_control(ENABLE_AUTO2);
			}
			else
			{
				max_temp = FAN_MANUAL;
				webman_config->man_rate = RANGE(new_speed, webman_config->minfan, webman_config->maxfan);
				webman_config->man_speed = (u8)(((float)(webman_config->man_rate + 1) * 255.f)/100.f); // manual fan speed
				if(webman_config->fanc == DISABLED) enable_fan_control(ENABLE_SC8);
				set_fan_speed(webman_config->man_speed);
			}
		}
		else if(webman_config->fanc || strstr(param, "?m"))
		{
			pos = strstr(param, "max=");
			if(pos)
				max_temp = get_valuen(param, "max=", 40, MAX_TEMPERATURE);
			else
			{
				pos = strstr(param, "?m");
				if(pos)
				{
					if(webman_config->fanc == FAN_AUTO2) enable_fan_control(ENABLED);

					if((max_temp && !strstr(param, "dyn")) || strstr(param, "man"))
						max_temp = FAN_MANUAL;
					else
						max_temp = webman_config->dyn_temp;

					if(webman_config->fanc == DISABLED) enable_fan_control(ENABLE_SC8);
				}
			}

			if(strstr(param, "?mode=s"))
				enable_fan_control(DISABLED);
			else if(strstr(param, "?mode=a"))
				enable_fan_control(ENABLE_AUTO2);
			else if(max_temp) //auto mode
			{
				if(strstr(param, "?u")) max_temp++;
				if(strstr(param, "?d")) max_temp--;
				webman_config->dyn_temp = RANGE(max_temp, 40, MAX_TEMPERATURE); // dynamic fan max temperature in °C
				webman_config->man_speed = FAN_AUTO;

				fan_ps2_mode = false;
			}
			else
			{
				if(strstr(param, "?u")) webman_config->man_rate++;
				if(strstr(param, "?d")) webman_config->man_rate--;
				webman_config->man_rate = RANGE(webman_config->man_rate, 20, 95); //%

				reset_fan_mode();
			}
		}

		save_settings();
	}

	{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

	char max_temp1[128], max_temp2[16]; *max_temp2 = NULL;

	if(fan_ps2_mode || ps2_classic_mounted)
	{
		sprintf(max_temp1, " (PS2 Mode: %i%%)", webman_config->ps2_rate);
	}
	else if((webman_config->fanc == DISABLED) || (!webman_config->man_speed && !max_temp))
		sprintf(max_temp1, "<small>[%s %s]</small>", STR_FANCTRL3, "SYSCON");
	else if(webman_config->fanc == FAN_AUTO2)
	{
		strcopy(max_temp1, "(AUTO)");
		strcopy(max_temp2, "(AUTO)");
	}
	else if(max_temp)
	{
		sprintf(max_temp1, "(MAX: %i°C)", max_temp);
		sprintf(max_temp2, "(MAX: %i°F)", FAHRENHEIT(max_temp));
	}
	else
		sprintf(max_temp1, "<small>[FAN: %i%% %s]</small>", webman_config->man_rate, STR_MANUAL);

	char hdd_free[40];

	#ifdef OVERCLOCKING
	if(get_rsxclock(GPU_CORE_CLOCK))
		sprintf(hdd_free, "<br>GPU: %i Mhz &bull; VRAM: %i Mhz", get_rsxclock(GPU_CORE_CLOCK), get_rsxclock(GPU_VRAM_CLOCK));
	else
		*hdd_free = NULL;

	sprintf(html,	"<hr><font size=\"42px\">"
					"<b><a class=\"s\" href=\"/cpursx.ps3?up\">"
					"CPU: %i°C %s<br>"
					"RSX: %i°C</a><hr>"
					"<a class=\"s\" href=\"/cpursx.ps3?dn\">"
					"CPU: %i°F %s<br>"
					"RSX: %i°F</a>"
					"%s<hr>",
					t1, max_temp1, t2,
					t1f, max_temp2, t2f, hdd_free); concat(buffer, html);
	#else
	sprintf(html,	"<hr><font size=\"42px\">"
					"<b><a class=\"s\" href=\"/cpursx.ps3?up\">"
					"CPU: %i°C %s<br>"
					"RSX: %i°C</a><hr>"
					"<a class=\"s\" href=\"/cpursx.ps3?dn\">"
					"CPU: %i°F %s<br>"
					"RSX: %i°F</a><hr>",
					t1, max_temp1, t2,
					t1f, max_temp2, t2f); concat(buffer, html);
	#endif

	if(IS_ON_XMB && file_exists(WM_RES_PATH "/slaunch.sprx"))
		strcopy(max_temp1, "/browser.ps3$slaunch");
	else
		strcopy(max_temp1, "/games.ps3");

	*html = NULL;

	#ifndef LITE_EDITION
	u16 len = 0;
	for(u8 d = 1; d < 7; d++)
	{
		if(isDir(drives[d]))
		{
			free_size(drives[d], hdd_free);
			len += sprintf(html + len, "<br><a href=\"%s\">USB%.3s: %s</a>", drives[d], drives[d] + 8, hdd_free);
		}
	}
	#endif

	free_size(drives[0], hdd_free);

	char *html2 = param;
	sprintf(html2,	"<a class=\"s\" href=\"%s\">"
					"MEM: %'d KB %s</a><br>"
					"<a href=\"%s\">HDD: %s</a>%s<hr>"
					"<a class=\"s\" href=\"/cpursx.ps3?mode\">"
					"%s %i%% (0x%X)</a><br>",
					max_temp1, (meminfo.avail>>10), IS_ON_XMB ? "(XMB)" : "",
					drives[0], hdd_free, html,
					STR_FANCH2, (int)((int)fan_speed * 100) / 255, fan_speed); concat(buffer, html2);

	if(!max_temp && webman_config->fanc && !is_ps3_http )
	{
		sprintf(html,	"<input type=\"range\" value=\"%i\" min=\"%i\" max=\"%i\" style=\"width:600px\""
						" onchange=\"self.location='/cpursx.ps3?fan='+this.value\">",
						webman_config->man_rate, webman_config->minfan, webman_config->maxfan);
		concat(buffer, html);
	}

	strcat(buffer, "<hr>");

	CellRtcTick pTick; cellRtcGetCurrentTick(&pTick);

	// detect aprox. time when a game is launched
	if(IS_ON_XMB) gTick=rTick; else if(gTick.tick==rTick.tick) cellRtcGetCurrentTick(&gTick);

	////// play time //////
	if(gTick.tick > rTick.tick)
	{
		char *play_time = param; get_sys_info(play_time, 22, true);
		sprintf( html, "<label title=\"Play\">&#9737;</label> %s<br>", play_time); concat(buffer, html);
	}
	///////////////////////


	//// startup time /////
	char *startup_time = param; get_sys_info(startup_time, 21, true);

	if(webman_config->chart)
		sprintf(html, "<a href=\"%s\">", CPU_RSX_CHART);
	else
		sprintf(html, "<a href=\"%s/%08i\">", HDD0_HOME_DIR, xusers()->GetCurrentUserNumber()); concat(buffer, html);

	sprintf(html, "<label title=\"Startup\"><img src='%s/time.png' border=0 style='position:relative;top:8px;'></label> %s</a>", WM_ICONS_PATH, startup_time); concat(buffer, html);
	///////////////////////

	char *runtime_info = param; get_sys_info(runtime_info, 23, true);
	sprintf(html, "</font><H1><img src='%s/power.png' style='position:relative;top:8px;'> %s</H1>", WM_ICONS_PATH, runtime_info); concat(buffer, html);

	if(isDir("/dev_bdvd"))
	{
		char *last_game_path = param;
		get_last_game(last_game_path);

		if(*last_game_path == '/')
		{
			sprintf( html, "<hr><font size=\"3\">" HTML_URL " -> ", IS_ON_XMB ? "/play.ps3" : "/dev_bdvd", "/dev_bdvd");
			concat(buffer, html); add_breadcrumb_trail(buffer, last_game_path); concat(buffer, "</font>");
		}
	}
	#ifdef BDINFO
	else
	{
		concat(buffer, "<hr><font size=\"3\">"); get_bdvd_info("", param); concat(buffer, param);
	}
	#endif

	// Get mac address [0xD-0x12]
	if(sys_admin)
	{
		char *fw_info = param;         get_sys_info(fw_info, 30, false);
		#ifdef SPOOF_CONSOLEID
		char *psid    = param + 0x100; get_sys_info(psid, 27, false);
		char *idps1   = param + 0x140; get_sys_info(idps1, 28, false);
		char *idps2   = param + 0x180; get_sys_info(idps2, 29, false);
		#endif
		char mac_address[32];          get_sys_info(mac_address, 31, false);

		char net_type[8] = "", ip[ip_size] = "-";
		get_net_info(net_type, ip);

		sprintf(html, "<hr><h2>"
						"<input type=button onclick=\"$$('ht').style.display='block';window.stop();\" value='&#x25BC;'> "
						"<a class=\"s\" href=\"/setup.ps3\">"
						"%s %s<br>" // fw_info
						"<span id='ht' style='display:none;'>"
		#ifdef SPOOF_CONSOLEID
						"%s<hr>" // psid
						"%s<br>" // idps1
						"%s<br>" // idps2
		#endif
						"%s - %s %s"
						"</span></h2></a></b>",
		#ifdef SPOOF_CONSOLEID
						IS_NAND ? "NAND" : "NOR", fw_info,
						psid,
						idps1,
						idps2,
		#else
						"", fw_info,
		#endif
						mac_address, ip, net_type); concat(buffer, html);
	}

	/////////////////////////////
	#ifdef COPY_PS3
	if(copy_in_progress)
	{
		concat(buffer, "<hr>");
		get_copy_stats(html, STR_COPYING);
		concat(buffer, html);
	}
	else
	if(fix_in_progress)
	{
		sprintf( html, "<hr>%s %s (%i %s)", STR_FIXING, current_file, fixed_count, STR_FILES); concat(buffer, html);
	}
	#endif
	/////////////////////////////

	concat(buffer, HTML_BLU_SEPARATOR
							 WM_APP_VERSION " - Simple Web Server" EDITION " ");

	if(webman_config->combo & SYS_ADMIN) strcat(buffer, sys_admin ? "[ADMIN]":"[USER]");

	strcat(buffer, "<p>");

	// show qr code
	if(webman_config->qr_code)
	{
		qr_code(html, "/cpursx.ps3", "<hr>", true, buffer);
	}

	{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }
}

static void show_wm_version(char *param)
{
	char cfw_info[32];
	get_cobra_version(cfw_info);

	if(payload_ps3hen)
	{
		sprintf(param,	"%s\n"
						"%s %s", WM_APP_VERSION, cfw_info + 4, STR_ENABLED);
	}
	else
	{
		if(!cobra_version) strcopy(cfw_info, "[nonCobra]");
		sprintf(param,	"%s\n"
						"%s %s" EDITION, WM_APP_VERSION, fw_version, cfw_info);
	}
	show_msg(param);
}
