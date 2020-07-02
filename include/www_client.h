#define MAX_WWW_CC		(255)

#define CODE_HTTP_OK            200
#define CODE_BAD_REQUEST        400
#define CODE_PATH_NOT_FOUND     404
#define CODE_SERVER_BUSY        503
#define CODE_VIRTUALPAD        1200
#define CODE_INSTALL_PKG       1201
#define CODE_DOWNLOAD_FILE     1202
#define CODE_RETURN_TO_ROOT    1203
#define CODE_BREADCRUMB_TRAIL  1220
#define CODE_GOBACK            1222
#define CODE_CLOSE_BROWSER     1223
#define CODE_PLAIN_TEXT        1224

////////////////////////////////
#ifndef EMBED_JS
static bool css_exists = false;
static bool common_js_exists = false;
#endif

#ifdef SYS_ADMIN_MODE
static u8 check_password(char *param)
{
	u8 ret = 0;

	if((pwd_tries < 3) && (webman_config->ftp_password[0] != NULL))
	{
		char *pos = strstr(param, "pwd=");
		if(pos > param)
		{
			pwd_tries++;
			if(IS(pos + 4, webman_config->ftp_password)) {pwd_tries = 0, ret = 1;}
			--pos; *pos = NULL;
		}
	}

	return ret;
}
#endif

static void restore_settings(void)
{
#ifdef COBRA_ONLY
	unload_vsh_plugin("VSH_MENU"); // unload vsh menu
	unload_vsh_plugin("sLaunch");  // unload sLaunch
#endif

	for(u8 n = 0; n < 4; n++)
		if(active_socket[n]>NONE) sys_net_abort_socket(active_socket[n], SYS_NET_ABORT_STRICT_CHECK);

	if(webman_config->fanc == DISABLED || webman_config->man_speed == FAN_AUTO)
	{
		bool set_ps2mode = (webman_config->fanc == ENABLED) && (webman_config->ps2_rate >= MIN_FANSPEED);

		if(set_ps2mode)
			restore_fan(SET_PS2_MODE); //set ps2 fan control mode
		else
			restore_fan(SYSCON_MODE);  //restore syscon fan control mode
	}

#ifdef WM_PROXY_SPRX
	{sys_map_path(VSH_MODULE_DIR WM_PROXY_SPRX ".sprx", NULL);}
#endif

	#ifdef AUTO_POWER_OFF
	setAutoPowerOff(false);
	#endif

#ifdef COBRA_ONLY
	if(cobra_config->fan_speed) cobra_read_config(cobra_config);
#endif
	working = plugin_active = 0;
	sys_ppu_thread_usleep(500000);
}

static int http_response(int conn_s, char *header, const char *url, int code, const char *msg)
{
	if(conn_s == (int)WM_FILE_REQUEST) return 0;

	u16 slen; *header = NULL;

	if(code == CODE_VIRTUALPAD || code == CODE_GOBACK || code == CODE_CLOSE_BROWSER)
	{
		slen = sprintf(header,  HTML_RESPONSE_FMT,
								CODE_HTTP_OK, url, HTML_BODY, HTML_RESPONSE_TITLE, msg);
	}
	else if(code == CODE_PLAIN_TEXT)
	{
		slen = sprintf(header,  HTML_RESPONSE_FMT,
								CODE_HTTP_OK, url, msg, "", "");
	}
	else
	{
		char body[_2KB_];

		if(*msg == '/')
		{
			char *cmd = (char*)msg + 1;
			sprintf(body, "%s : OK", cmd); if(!(webman_config->minfo & 2)) show_msg(body);
			if(code == CODE_BREADCRUMB_TRAIL)
			{
				char *p = strchr(cmd, '/');
				if(p) {body[p - cmd] = NULL; add_breadcrumb_trail(body, p);}
			}
		}
		else if(islike(msg, "http"))
			sprintf(body, "<a style=\"%s\" href=\"%s\">%s</a>", HTML_URL_STYLE, msg, msg);
#ifdef PKG_HANDLER
		else if(code == CODE_INSTALL_PKG || code == CODE_DOWNLOAD_FILE)
		{
			sprintf(body, "<style>a{%s}</style>%s", HTML_URL_STYLE, (code == CODE_INSTALL_PKG) ? "Installing " : "");
			char *p = strchr((char*)msg, '\n');
			if(p)
			{
				*p = NULL;
				if(code == CODE_INSTALL_PKG) add_breadcrumb_trail(body, (char *)msg + 11); else strcat(body, msg);
				if(code == CODE_DOWNLOAD_FILE || extcasecmp(pkg_path, ".p3t", 4) != 0) strcat(body, "<p>To: \0"); add_breadcrumb_trail(body, p + 5);
			}
			else
				strcat(body, msg);

			code = CODE_HTTP_OK;
		}
#endif
		else
			sprintf(body, "%s", msg);

		if(code == CODE_PATH_NOT_FOUND)
		{
			strcat(body, "<p>"); add_breadcrumb_trail(body, url + (islike(url, "/mount") ? MOUNT_CMD : 0));
		}

		//if(ISDIGIT(*msg) && ( (code == CODE_SERVER_BUSY || code == CODE_BAD_REQUEST) )) show_msg((char*)body + 4);

#ifndef EMBED_JS
		if(css_exists)
		{
			sprintf(header, "<LINK href=\"%s\" rel=\"stylesheet\" type=\"text/css\">", COMMON_CSS); strcat(body, header);
		}
		if(common_js_exists)
		{
			sprintf(header, SCRIPT_SRC_FMT, COMMON_SCRIPT_JS); strcat(body, header);
		}
#endif

		sprintf(header, "<hr>" HTML_BUTTON_FMT "%s",
						HTML_BUTTON, " &#9664;  ", HTML_ONCLICK, ((code == CODE_RETURN_TO_ROOT) ? "/" : "javascript:history.back();"), HTML_BODY_END); strcat(body, header);

		slen = sprintf(header,  HTML_RESPONSE_FMT,
								(code == CODE_RETURN_TO_ROOT) ? CODE_HTTP_OK : code, url, HTML_BODY, HTML_RESPONSE_TITLE, body);
	}

	send(conn_s, header, slen, 0);
	sclose(&conn_s);
	if(loading_html) loading_html--;
	return 0;
}

static size_t prepare_html(char *buffer, char *templn, char *param, u8 is_ps3_http, u8 is_cpursx, bool mount_ps3)
{
	t_string sbuffer; _alloc(&sbuffer, buffer);

	if((webman_config->sman || strstr(param, "/sman.ps3")) && file_exists(HTML_BASE_PATH "/sman.htm"))
	{
		sbuffer.size = read_file(HTML_BASE_PATH "/sman.htm", sbuffer.str, _16KB_, 0);

		if(is_cpursx)
			_concat(&sbuffer, "<meta http-equiv=\"refresh\" content=\"15;URL=/cpursx.ps3?/sman.ps3\">");

		// add javascript
		{
			#ifndef ENGLISH_ONLY
			if(webman_config->lang)
			{
				_concat(&sbuffer, "<script>");
				sprintf(templn, "l('%s','%s');", "games",    STR_GAMES);    _concat(&sbuffer, templn);
				sprintf(templn, "l('%s','%s');", "files",    STR_FILES);    _concat(&sbuffer, templn);
				sprintf(templn, "l('%s','%s');", "setup",    STR_SETUP);    _concat(&sbuffer, templn);
				sprintf(templn, "l('%s','%s');", "eject",    STR_EJECT);    _concat(&sbuffer, templn);
				sprintf(templn, "l('%s','%s');", "insert",   STR_INSERT);   _concat(&sbuffer, templn);
				sprintf(templn, "l('%s','%s');", "refresh",  STR_REFRESH);  _concat(&sbuffer, templn);
				sprintf(templn, "l('%s','%s');", "restart",  STR_RESTART);  _concat(&sbuffer, templn);
				sprintf(templn, "l('%s','%s');", "shutdown", STR_SHUTDOWN); _concat(&sbuffer, templn);
				sprintf(templn, "l('msg2','%s ...');"
								"</script>",                 STR_MYGAMES);  _concat(&sbuffer, templn);
			}
			#endif
			if(webman_config->homeb && islike(webman_config->home_url, "http"))
			{
				sprintf(templn, "<script>hurl=\"%s\";</script>", webman_config->home_url);  _concat(&sbuffer, templn);
			}
		}

		if(param[1] != NULL && !strstr(param, ".ps3")) {_concat(&sbuffer,  "<base href=\""); urlenc(templn, param); strcat(templn, "/\">"); _concat(&sbuffer, templn);}

		return sbuffer.size;
	}

	sbuffer.size = sprintf(sbuffer.str, HTML_HEADER);

	if(is_cpursx)
		_concat(&sbuffer, "<meta http-equiv=\"refresh\" content=\"10;URL=/cpursx.ps3\">");

	if(mount_ps3) {_concat(&sbuffer, HTML_BODY); return sbuffer.size;}

	_concat(&sbuffer,
					"<head><title>" WEBMAN_MOD "</title>"
					"<style>"
					"a{" HTML_URL_STYLE "}"
					"#rxml,#rhtm,#rcpy,#wmsg{position:fixed;top:40%;left:30%;width:40%;height:90px;z-index:5;border:5px solid #ccc;border-radius:25px;padding:10px;color:#fff;text-align:center;background-image:-webkit-gradient(linear,0 0,0 100%,color-stop(0,#999),color-stop(0.02,#666),color-stop(1,#222));background-image:-moz-linear-gradient(top,#999,#666 2%,#222);display:none;}"
					"</style>"); // fallback style if external css fails

#ifndef EMBED_JS
	if(param[1] == 0)
	{
		// minimize times that these files are checked (at startup & root)
		css_exists = css_exists || file_exists(COMMON_CSS);
		common_js_exists = common_js_exists || file_exists(COMMON_SCRIPT_JS);
	}
	if(css_exists)
	{
		sprintf(templn, "<LINK href=\"%s\" rel=\"stylesheet\" type=\"text/css\">", COMMON_CSS); _concat(&sbuffer, templn);
	}
	else
#endif
	{
		_concat(&sbuffer,
						"<style type=\"text/css\"><!--\r\n"

						"a.s:active{color:#F0F0F0;}"
						"a:link{color:#909090;}"

						"a.f:active{color:#F8F8F8;}"
						"a,a.f:link,a:visited{color:#D0D0D0;}");

		if(!is_cpursx)
				_concat(&sbuffer,
						"a.d:link,a.d:visited{background:0px 2px url('data:image/gif;base64,R0lGODlhEAAMAIMAAOenIumzLbmOWOuxN++9Me+1Pe+9QvDAUtWxaffKXvPOcfTWc/fWe/fWhPfckgAAACH5BAMAAA8ALAAAAAAQAAwAAARQMI1Agzk4n5Sa+84CVNUwHAz4KWzLMo3SzDStOkrHMO8O2zmXsAXD5DjIJEdxyRie0KfzYChYr1jpYVAweb/cwrMbAJjP54AXwRa433A2IgIAOw==') no-repeat;padding:0 0 0 20px;}"
						"a.w:link,a.w:visited{background:url('data:image/gif;base64,R0lGODlhDgAQAIMAAAAAAOfn5+/v7/f39////////////////////////////////////////////wAAACH5BAMAAA8ALAAAAAAOABAAAAQx8D0xqh0iSHl70FxnfaDohWYloOk6papEwa5g37gt5/zO475fJvgDCW8gknIpWToDEQA7') no-repeat;padding:0 0 0 20px;}");

		_concat(&sbuffer,
						"a:active,a:active:hover,a:visited:hover,a:link:hover{color:#FFFFFF;}"
						".list{display:inline;}"
#ifdef PS3MAPI
						"table{border-spacing:0;border-collapse:collapse;}"
						".la{text-align:left;float:left}.ra{text-align:right;float:right;}"
#endif
						"input:focus{border:2px solid #0099FF;}"
						".propfont{font-family:\"Courier New\",Courier,monospace;text-shadow:1px 1px #101010;}"
						"body{background-color:#101010}body,a.s,td,th{color:#F0F0F0;white-space:nowrap");

		if(file_exists("/dev_hdd0/xmlhost/game_plugin/background.jpg"))
			_concat(&sbuffer, "background-image: url(\"/dev_hdd0/xmlhost/game_plugin/background.jpg\");");

		if(is_ps3_http == 2)
			_concat(&sbuffer, "width:800px;}");
		else
			_concat(&sbuffer, "}");

		if(!islike(param, "/setup.ps3")) _concat(&sbuffer, "td+td{text-align:right;white-space:nowrap}");

		if(islike(param, "/index.ps3"))
		{
			_concat(&sbuffer,
							".gc{float:left;overflow:hidden;position:relative;text-align:center;width:280px;height:260px;margin:3px;border:1px dashed grey;}"
							".ic{position:absolute;top:5px;right:5px;left:5px;bottom:40px;}");

			if(is_ps3_http == 1)
				_concat(&sbuffer, ".gi{height:210px;width:267px;");
			else
				_concat(&sbuffer, ".gi{max-height:210px;max-width:260px;");

			_concat(&sbuffer,
							"position:absolute;bottom:0px;top:0px;left:0px;right:0px;margin:auto;}"
							".gn{position:absolute;height:38px;bottom:0px;right:7px;left:7px;text-align:center;}");
		}

		_concat(&sbuffer, ".bu{background:#444;}.bf{background:#121;}--></style>");
	}

	if(param[1] != NULL && !strstr(param, ".ps3")) {_concat(&sbuffer, "<base href=\""); urlenc(templn, param); strcat(templn, "/\">"); _concat(&sbuffer, templn);}

	if(is_ps3_http == 1)
		{sprintf(templn, "<style>%s</style>", ".gi{height:210px;width:267px"); _concat(&sbuffer, templn);}

	sprintf(templn, "</head>%s", HTML_BODY); _concat(&sbuffer, templn);

	char coverflow[40]; if(file_exists(MOBILE_HTML)) sprintf(coverflow, " [<a href=\"/games.ps3\">Coverflow</a>]"); else *coverflow = NULL;

	size_t tlen = sprintf(templn, "<b>" WM_APP_VERSION " %s <font style=\"font-size:18px\">[<a href=\"/\">%s</a>] [<a href=\"%s\">%s</a>]%s", STR_TRADBY, STR_FILES, (webman_config->sman && file_exists(HTML_BASE_PATH "/sman.htm")) ? "/sman.ps3" : "/index.ps3", STR_GAMES, coverflow);

#ifdef SYS_ADMIN_MODE
	if(sys_admin)
	{
#endif
#ifdef PS3MAPI
	#ifdef WEB_CHAT
		sprintf(templn + tlen, " [<a href=\"/chat.ps3\">Chat</a>] [<a href=\"/home.ps3mapi\">PS3MAPI</a>] [<a href=\"/setup.ps3\">%s</a>]</b>", STR_SETUP);
	#else
		sprintf(templn + tlen, " [<a href=\"/home.ps3mapi\">PS3MAPI</a>] [<a href=\"/setup.ps3\">%s</a>]</b>", STR_SETUP);
	#endif
#else
	#ifdef WEB_CHAT
		sprintf(templn + tlen, " [<a href=\"/chat.ps3\">Chat</a>] [<a href=\"/setup.ps3\">%s</a>]</b>", STR_SETUP);
	#else
		sprintf(templn + tlen, " [<a href=\"/setup.ps3\">%s</a>]</b>", STR_SETUP );
	#endif
#endif
#ifdef SYS_ADMIN_MODE
	}
	else
		strcat(templn, "</b>");
#endif

	_concat(&sbuffer, templn);
	return sbuffer.size;
}

static void handleclient_www(u64 conn_s_p)
{
	int conn_s = (int)conn_s_p; // main communications socket

	size_t header_len;
	sys_addr_t sysmem = NULL;

	prev_dest = last_dest = NULL; // init fast concat

	bool is_ntfs = false;
	char param[HTML_RECV_SIZE];

	char *file_query = param + HTML_RECV_LAST; *file_query = NULL;

	if(conn_s_p == START_DAEMON || conn_s_p == REFRESH_CONTENT)
	{
		bool do_sleep = true;

#ifdef WM_PROXY_SPRX
		apply_remaps();
#endif

		if(conn_s_p == START_DAEMON)
		{
			if(file_exists("/dev_hdd0/ps3-updatelist.txt") || !payload_ps3hen)
				vshnet_setUpdateUrl("http://127.0.0.1/dev_hdd0/ps3-updatelist.txt"); // custom update file

 #ifndef ENGLISH_ONLY
			update_language();
 #endif
			make_fb_xml();

			if(profile || !(webman_config->wmstart))
			{
				char cfw_info[20];
				get_cobra_version(cfw_info);

				if(payload_ps3hen)
				{
					sprintf(param,	"%s\n"
									"%s %s", WM_APP_VERSION, cfw_info + 4, STR_ENABLED);
				}
				else
				{
					sys_ppu_thread_sleep(9);
					sprintf(param,	"%s\n"
									"%s %s" EDITION, WM_APP_VERSION, fw_version, cfw_info);
				}
				show_msg(param); do_sleep = false;
			}

			if(webman_config->bootd) wait_for("/dev_usb", webman_config->bootd); // wait for any usb

			// is JAP?
			int enter_button = 1;
			xsetting_0AF1F161()->GetEnterButtonAssign(&enter_button);
			CELL_PAD_CIRCLE_BTN = enter_button ? CELL_PAD_CTRL_CIRCLE : CELL_PAD_CTRL_CROSS;
		}
		else //if(conn_s_p == REFRESH_CONTENT)
		{
			{DELETE_CACHED_GAMES} // refresh XML will force "refresh HTML" to rebuild the cache file
		}

		mkdirs(param); // make hdd0 dirs GAMES, PS3ISO, PS2ISO, packages, etc.


		//////////// usb ports ////////////
		for(u8 indx = 5, d = 6; d < 128; d++)
		{
			sprintf(param, "/dev_usb%03i", d);
			if(isDir(param)) {strcpy(drives[indx++], param); if(indx > 6) break;}
		}
		///////////////////////////////////


		check_cover_folders(param);

		// Use system icons if wm_icons don't exist
		for(u8 i = 0; i < 14; i++)
		{
			if(not_exists(wm_icons[i]))
			{
				sprintf(param, VSH_RESOURCE_DIR "explore/icon/%s", wm_icons[i] + 23); strcpy(wm_icons[i], param);
				if(file_exists(param)) continue;

				char *icon = wm_icons[i] + 32;
				if(i == gPS3 || i == iPS3)	sprintf(icon, "user/024.png"); else // ps3
				if(i == gPSX || i == iPSX)	sprintf(icon, "user/026.png"); else // psx
				if(i == gPS2 || i == iPS2)	sprintf(icon, "user/025.png"); else // ps2
				if(i == gPSP || i == iPSP)	sprintf(icon, "user/022.png"); else // psp
				if(i == gDVD || i == iDVD)	sprintf(icon, "user/023.png"); else // dvd
				if(i == iROM || i == iBDVD)	strcpy(wm_icons[i], wm_icons[iPS3]); else
											sprintf(icon + 5, "icon_home.png"); // setup / eject
			}
		}

#ifndef EMBED_JS
		css_exists = file_exists(COMMON_CSS);
		common_js_exists = file_exists(COMMON_SCRIPT_JS);
#endif

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
#ifdef COBRA_ONLY
			cobra_read_config(cobra_config);

			// cobra spoofer not working since 4.53
			if(webman_config->nospoof || (c_firmware >= 4.53f))
			{
				cobra_config->spoof_version  = 0;
				cobra_config->spoof_revision = 0;
			}
			else
			{
				cobra_config->spoof_version = 0x0486;
				cobra_config->spoof_revision = 67896; //0x00010938 // 4.85 = 67869; // 0x0001091d
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
			if(file_exists(WMNET_DISABLED)) //re-enable network (force offline in game)
			{
				net_status = 1;
				poll_start_play_time();
			}
			#endif
 #endif
			if(do_sleep) sys_ppu_thread_sleep(1);

#ifdef COBRA_ONLY
			{sys_map_path((char*)"/dev_flash/vsh/resource/coldboot_stereo.ac3", NULL);}
			{sys_map_path((char*)"/dev_flash/vsh/resource/coldboot_multi.ac3",  NULL);}
			map_earth(0, param);
#endif
		}

		sys_ppu_thread_exit(0);
	}

 #ifdef USE_DEBUG
	ssend(debug_s, "waiting...");
 #endif

	if(loading_html > 10) loading_html = 0;

	bool is_local = true;
	sys_net_sockinfo_t conn_info_main;

	u8 max_cc = 0; // count client connections per persistent connection
	u8 keep_alive = 0;
	u8 ap_param = 0; // 0 = do nothing, 1 = use webman_config->autoplay, 2 = force auto_play

	char cmd[16], header[HTML_RECV_SIZE], *mc = NULL;

 #ifdef WM_REQUEST
	struct CellFsStat buf; u8 wm_request = (cellFsStat(WMREQUEST_FILE, &buf) == CELL_FS_SUCCEEDED);

	if(!wm_request)
 #endif
	{
		sys_net_get_sockinfo(conn_s, &conn_info_main, 1);

		char *ip_address = cmd;
		is_local = (conn_info_main.local_adr.s_addr == conn_info_main.remote_adr.s_addr);

		sprintf(ip_address, "%s", inet_ntoa(conn_info_main.remote_adr));
		if(webman_config->bind && (!is_local) && !islike(ip_address, webman_config->allow_ip))
		{
			keep_alive = http_response(conn_s, header, param, CODE_BAD_REQUEST, STR_ERROR);

			goto exit_handleclient_www;
		}

		if(!webman_config->netd[0] && !webman_config->neth[0][0]) strcpy(webman_config->neth[0], ip_address); // show client IP if /net0 is empty
		if(!webman_config->bind) strcpy(webman_config->allow_ip, ip_address);
	}

/*
	// check available free memory
	{
		_meminfo meminfo;
		u8 retries = 0;

again3:
		{ system_call_1(SC_GET_FREE_MEM, (u64)(u32) &meminfo); }

		if((meminfo.avail) < ( _64KB_ + MIN_MEM )) //leave if less than min memory
		{
			#ifdef USE_DEBUG
			ssend(debug_s, "!!! NOT ENOUGH MEMORY!\r\n");
			#endif

			retries++;
			sys_ppu_thread_sleep(1);
			if((retries < 5) && working) goto again3;

			keep_alive = http_response(conn_s, header, param, CODE_SERVER_BUSY, STR_ERROR); BEEP3;

			#ifdef WM_REQUEST
			if(wm_request) cellFsUnlink(WMREQUEST_FILE);
			#endif

			goto exit_handleclient_www;
		}
	}
*/
	#ifdef WM_REQUEST
	if(!wm_request)
	#endif
	{
		struct timeval tv;
		tv.tv_usec = 0;
		tv.tv_sec  = 3;
		setsockopt(conn_s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		//tv.tv_sec  = 8;
		//setsockopt(conn_s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

		int optval = HTML_RECV_SIZE;
		setsockopt(conn_s, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
	}

parse_request:

  {
	u8 retry = 0, served = 0, is_binary = WEB_COMMAND;	// served http request?, is_binary: 0 = http command, 1 = file, 2 = folder listing
	s8 sort_order = 1, sort_by = 0;
	u64 c_len = 0;

	u8 is_cpursx = 0;
	u8 is_popup = 0, auto_mount = 0;
	u8 is_ps3_http = 0;

	#ifdef USE_NTFS
	skip_prepntfs = false;
	#endif

//// process commands ////

	while(!served && working)
	{
		served++;
		*header = NULL;
		keep_alive = 0;

		if(!mc)
		{
			#ifdef USE_DEBUG
			ssend(debug_s, "ACC - ");
			#endif

 #ifdef WM_REQUEST
  #ifdef SLAUNCH_FILE
			if(wm_request)
			{
				if(buf.st_size > 5 && buf.st_size < HTML_RECV_SIZE && read_file(WMREQUEST_FILE, header, buf.st_size, 0) > 4)
				{
					if(!(webman_config->launchpad_xml) && islike(header, "/dev_hdd0/photo/"))
					{
						char *filename = strrchr(header, '/'); if(filename) filename++;
						if(filename)
						{
							char *pos1 = strcasestr(filename, ".jpg"); if(pos1) *pos1 = NULL;
							char *pos2 = strcasestr(filename, ".png"); if(pos2) *pos2 = NULL;

							int fsl = 0;
							if(cellFsOpen(SLAUNCH_FILE, CELL_FS_O_RDONLY, &fsl, NULL, 0) == CELL_FS_SUCCEEDED)
							{
								_slaunch slaunch; u64 read_e;

								while(cellFsRead(fsl, &slaunch, sizeof(_slaunch), &read_e) == CELL_FS_SUCCEEDED && read_e > 0)
								{
									char *path = slaunch.name + slaunch.path_pos;
									char *templn = slaunch.name;
									if((strcasestr(path, filename) == NULL) && (strcasestr(templn, filename) == NULL)) continue;
									sprintf(filename, "%s", path); break;
								}
								cellFsClose(fsl);
							}
							if(!(webman_config->minfo & 1)) show_msg(filename);
							sprintf(header, "%s", filename);
							init_delay = -10; // prevent show Not in XMB message
						}
					}

					if(*header == '/') {strcpy(param, header); buf.st_size = sprintf(header, "GET %s", param);}
					for(size_t n = buf.st_size; n > 4; n--) if(header[n] == ' ') header[n] = '+';
					if(islike(header, "GET /play.ps3")) {if(IS_INGAME && (++retry < 30)) {sys_ppu_thread_sleep(1); served = 0; is_ps3_http = 1; continue;}}
				}
				cellFsUnlink(WMREQUEST_FILE);
			}
   #endif //#ifdef SLAUNCH_FILE
 #endif //#ifdef WM_REQUEST
		}
		else sprintf(header, "GET %s", mc + 1);

		mc = NULL;

		if((*header == 'G') || ((recv(conn_s, header, HTML_RECV_SIZE, 0) > 0) && (*header == 'G') && (header[4] == '/'))) // serve only GET /xxx requests
		{
			if(strstr(header, "Connection: keep-alive"))
			{
				keep_alive = 1;
			}

			if(strstr(header, "x-ps3-browser")) is_ps3_http = 1; else
			if(strstr(header, "Gecko/36"))  	is_ps3_http = 2; else
												is_ps3_http = 0;

			for(size_t n = 0; header[n]; n++) {if(header[n] == '\r' || header[n] == '\n' || n >= HTML_RECV_SIZE) {header[n] = 0; break;}}

			ssplit(header, cmd, 15, header, HTML_RECV_LAST);
			ssplit(header, param, HTML_RECV_LAST, cmd, 15);

			char *param_original = header; // used in /download.ps3

 #ifdef WM_REQUEST
			if(wm_request) { for(size_t n = 0; param[n]; n++) {if(param[n] == '\t') param[n] = ' ';} } wm_request = 0;
 #endif

			if((refreshing_xml == 0) && islike(param, "/refresh"))
			{
				if(islike(param + 8, "_ps3"))
				{
					refresh_xml(param);

					if(IS_ON_XMB && file_exists(RELOADXMB_EBOOT))
					{
						reload_xmb();
						sys_ppu_thread_sleep(3);
						if(IS_ON_XMB) launch_app_home_icon();
					}

					#ifdef WM_REQUEST
					if(!wm_request)
					#endif
					keep_alive = http_response(conn_s, header, param, CODE_HTTP_OK, param);
					goto exit_handleclient_www;
				}

				#ifdef USE_NTFS
				if(webman_config->ntfs)
				{
					get_game_info();
					skip_prepntfs = (strcmp(_game_TitleID, "BLES80616") == 0); // /refresh.ps3 called from prepNTFS application
				}
				#endif
			}

			bool is_setup = islike(param, "/setup.ps3?");

			if(!is_setup) { mc = strstr(param, ";/"); if(mc) {*mc = NULL; strcpy(header, param);} }

			bool allow_retry_response = true; u8 mobile_mode = false;

 #ifdef USE_DEBUG
	ssend(debug_s, param);
	ssend(debug_s, "\r\n");
 #endif

 #ifdef SYS_ADMIN_MODE
			if(!sys_admin)
			{
				bool accept = false;
				if(*param != '/') accept = false; else
				if(islike(param, "/admin.ps3")) accept = true; else
				{
					accept = ( IS(param, "/")

							|| islike(param, "/cpursx")

							|| islike(param, "/mount")
							|| islike(param, "/refresh.ps3")
							|| islike(param, "/reloadxmb.ps3")
							|| islike(param, "/index.ps3")
							|| islike(param, "/sman.ps3")
							|| islike(param, "/games.ps3")
							|| islike(param, "/play.ps3")

							|| islike(param, "/eject.ps3")
							|| islike(param, "/insert.ps3")
	#ifdef EXT_GDATA
							|| islike(param, "/extgd.ps3")
	#endif
	#ifdef WEB_CHAT
							|| islike(param, "/chat.ps3")
	#endif
	#ifndef LITE_EDITION
							|| islike(param, "/popup.ps3")
	#endif
							 );

					if(!accept)
					{
	#ifndef LITE_EDITION
						if(check_password(param)) {is_local = true, sys_admin = 1;}
	#endif
						if(is_local)
							accept = ( islike(param, "/setup.ps3")
	#ifdef PKG_HANDLER
									|| islike(param, "/install")
									|| islike(param, "/download.ps3")
	#endif
	#ifdef PS3_BROWSER
									|| islike(param, "/browser.ps3")
	#endif
									|| islike(param, "/restart.ps3")
									|| islike(param, "/shutdown.ps3"));
					}

					if(!accept)
					{
						int ext_pos = strlen(param) - 4; if(ext_pos < 0) ext_pos = 0; sys_admin = 0;
						char *ext = param + ext_pos;
						if(!accept && (islike(param, "/net") || file_exists(param)) && (_IS(ext, ".jpg") || _IS(ext, ".png") || IS(ext, ".css") || strstr(ext, ".js") || IS(ext, "html")) ) accept = true;
					}
				}

				if(!accept)
				{
					sprintf(param, "%s\nADMIN %s", STR_ERROR, STR_DISABLED);
					keep_alive = http_response(conn_s, header, param, CODE_BAD_REQUEST, param);
					goto exit_handleclient_www;
				}
			}
 #endif

 #ifdef VIRTUAL_PAD
			bool is_pad = islike(param, "/pad.ps3");

			if(!is_pad)
 #endif
			{
	redirect_url:
				urldec(param, param_original);

				if(islike(param, "/setup.ps3")) goto html_response;
			}

 #ifdef VIRTUAL_PAD
			if(is_pad || islike(param, "/combo.ps3") || islike(param, "/play.ps3"))
			{
				// /pad.ps3                      (see vpad.h for details)
				// /combo.ps3                    simulates a combo without actually send the buttons
				// /play.ps3                     start game from disc icon
				// /play.ps3?col=<col>&seg=<seg>  click item on XMB
				// /play.ps3<path>               mount <path> and start game from disc icon
				// /play.ps3<script-path>        execute script. path must be a .txt or .bat file
				// /play.ps3?<titleid>           mount npdrm game and start game from app_home icon (e.g IRISMAN00, MANAGUNZ0, NPUB12345, etc.)
				// /play.ps3?<appname>           play movian, multiman, retroArch, rebug toolbox, remoteplay

				u8 ret = 0, is_combo = (param[2] == 'a') ? 0 : (param[1] == 'c') ? 2 : 1; // 0 = /pad.ps3   1 = /play.ps3   2 = /combo.ps3

				char *buttons = param + 9 + is_combo;

				if(is_combo != 1) {if(!webman_config->nopad) ret = parse_pad_command(buttons, is_combo);} // do /pad.ps3 || /combo.ps3
				else // do /play.ps3
				{
					char *param2 = param + 9; if(*param2 == '?') param2++;

					// check /play.ps3<path>
					if(file_exists(param2))
					{
						if(IS(param2, "/app_home"))
						{
							launch_app_home_icon();
							sprintf(param, "/cpursx.ps3");
						}
						else
						{
							strcpy(header, param2);
							sprintf(param, "/mount.ps3%s", header);
							ap_param = 2; // force auto_play
						}
						is_binary = WEB_COMMAND;
						goto html_response;
					}

					// default: play.ps3?col=game&seg=seg_device
					char col[16], seg[80]; *col = *seg = NULL; bool execute = true;
#ifndef LITE_EDITION
					if(_islike(param2, "movian") || IS(param2, "HTSS00003"))
													 {sprintf(param2, "col=tv&seg=HTSS00003"); mount_unk = APP_GAME;} else
					if(_islike(param2, "remoteplay")){sprintf(param2, "col=network&seg=seg_premo");} else
					if(_islike(param2, "retro"))     {sprintf(param2, "SSNE10000");} else
					if(_islike(param2, "multiman"))  {sprintf(param2, "BLES80608");} else
					if(_islike(param2, "rebug"))     {sprintf(param2, "RBGTLBOX2");}
#endif
#ifdef COBRA_ONLY
					sprintf(header, "%s%s", HDD0_GAME_DIR, param2);

					if((*param2 != NULL) && isDir(header))
					{
						set_app_home(header);

						if(is_app_home_onxmb()) mount_unk = APP_GAME; else execute = false;
					}
					else
#endif
					{
						get_param("col=", col, param2, 16); // game / video / friend / psn / network / music / photo / tv
						get_param("col=", seg, param2, 80);
					}

					exec_xmb_item(col, seg, execute);
					mount_unk = EMU_OFF;
				}

				if(is_combo == 1 && param[10] != '?') sprintf(param, "/cpursx.ps3");
				else
				{
					if((ret == 'X') && IS_ON_XMB) goto reboot;

					if(!mc) keep_alive = http_response(conn_s, header, param, CODE_VIRTUALPAD, buttons);

					goto exit_handleclient_www;
				}
			}
 #elif defined(LITE_EDITION)
			if(islike(param, "/play.ps3"))
			{
				// /play.ps3                     start game from disc icon

				// default: play.ps3?col=game&seg=seg_device
				launch_disc(true);

				sprintf(param, "/cpursx.ps3");
			}
 #endif //  #ifdef VIRTUAL_PAD

			if(islike(param, "/reloadxmb.ps3") && refreshing_xml == 0)
			{
				reload_xmb();
				sprintf(param, "/index.ps3");
			}
			else
			if(islike(param, "/crossdomain.xml"))
			{
				sprintf(param, "%s%s", HTML_BASE_PATH, "/crossdomain.xml");
			}

 #ifdef SYS_ADMIN_MODE
			if(islike(param, "/admin.ps3"))
			{
				// /admin.ps3?enable&pwd=<password>  enable admin mode
				// /admin.ps3?disable                disable admin mode
				// /admin.ps3?0                      disable admin mode

				if(param[10] == 0 || param[11] == 0) ; else
				if(~param[11] & 1) sys_admin = 0; else
				{
					sys_admin = check_password(param);
				}

				sprintf(param, "ADMIN %s", sys_admin ? STR_ENABLED : STR_DISABLED);

				if(!mc) keep_alive = http_response(conn_s, header, param, CODE_RETURN_TO_ROOT, param);

				goto exit_handleclient_www;
			}
 #endif
			{char *pos = strstr(param, "?restart.ps3"); if(pos) {*pos = NULL; do_restart = sys_admin;}}

			if(islike(param, "/cpursx_ps3"))
			{
				char *cpursx = header, *buffer = param; get_cpursx(cpursx);
 /*				// prevents flickering but cause error 80710336 in ps3 browser (silk mode)
				sprintf(buffer, "<meta http-equiv=\"refresh\" content=\"15;URL=%s\">"
								"<script>parent.document.getElementById('lbl_cpursx').innerHTML = \"%s\";</script>",
								"/cpursx_ps3", cpursx);
 */
				int buf_len = sprintf(buffer, "<meta http-equiv=\"refresh\" content=\"15;URL=%s\">"
											  "%s"
											  "<a href=\"%s\" target=\"_parent\" style=\"text-decoration:none;\">"
											  "<font color=\"#fff\">%s</a>",
											  "/cpursx_ps3", HTML_BODY, "/cpursx.ps3", cpursx);
				buf_len = sprintf(header, HTML_RESPONSE_FMT,
										  CODE_HTTP_OK, "/cpursx_ps3", HTML_HEADER, buffer, HTML_BODY_END);

				send(conn_s, header, buf_len, 0); keep_alive = 0;

				goto exit_handleclient_www;
			}

			if(sys_admin && islike(param, "/dev_blind"))
			{
				// /dev_blind          auto-enable & access /dev_blind
				// /dev_blind?         shows status of /dev_blind
				// /dev_blind?1        mounts /dev_blind
				// /dev_blind?enable   mounts /dev_blind
				// /dev_blind?0        unmounts /dev_blind
				// /dev_blind?disable  unmounts /dev_blind

				is_binary = FOLDER_LISTING;
				if(file_exists(param)) goto retry_response;
				goto html_response;
			}

 #ifdef PKG_HANDLER
			if(islike(param, "/download.ps3"))
			{
				// /download.ps3?url=<url>            (see pkg_handler.h for details)
				// /download.ps3?to=<path>&url=<url>

				char msg[MAX_LINE_LEN], filename[STD_PATH_LEN + 1]; memset(msg, 0, MAX_LINE_LEN); *filename = NULL;

				setPluginActive();

				int ret = download_file(strchr(param_original, '%') ? (param_original + 13) : (param + 13), msg);

				char *dlpath = strchr(msg, '\n'); // get path in "...\nTo: /path/"

				if(dlpath)
				{
					*dlpath = NULL; // limit string to url in "Downloading http://blah..."

					char *dlfile = strrchr(msg, '/');
					if(dlfile) snprintf(filename, STD_PATH_LEN, "%s%s", dlpath + 5, dlfile);

					*dlpath = '\n'; // restore line break
				}

				#ifdef WM_REQUEST
				if(!wm_request)
				#endif
				{
					if(!mc) keep_alive = http_response(conn_s, header, param, (ret == FAILED) ? CODE_BAD_REQUEST : CODE_DOWNLOAD_FILE, msg);
				}

				if(!(webman_config->minfo & 1)) show_msg(msg);

				wait_for_xml_download(filename, param);

				setPluginInactive();
				goto exit_handleclient_www;
			}

			if(islike(param, "/install.ps3") || islike(param, "/install_ps3"))
			{
				// /install.ps3<pkg-path>  (see pkg_handler.h for details)
				// /install.ps3<pkg-path>? conditional install
				// /install_ps3<pkg-path>  install & keep pkg
				// /install.ps3?url=<url>  download, auto-install pkg & delete pkg
				// /install_ps3?url=<url>  download, auto-install pkg & keep pkg in /dev_hdd0/packages
				// /install.ps3<pkg-path>?restart.ps3
				// /install.ps3?url=<url>?restart.ps3
				// /install.ps3<p3t-path>  install ps3 theme
				// /install.ps3<directory> install pkgs in directory (GUI)
				// /install.ps3$           install webman addons (GUI)
				// /install.ps3$all        install all pkg files in /dev_hdd0/packages & delete pkg
				// /install_ps3            install all pkg files in /dev_hdd0/packages
				// /install_ps3$           install all webman addons
				// /install_ps3<path>      install pkg files in <path>

				char *pkg_file = param + 12;

				bool install_ps3 = (param[8] == '_');

				if(islike(pkg_file, "/net"))
				{
					sprintf(header, "%s%s", "/mount_ps3", pkg_file); // use /mount.ps3 command to install the remote pkg
					strcpy(param, header);
					is_binary = WEB_COMMAND;
					goto html_response;
				}

				pkg_delete_after_install = islike(pkg_file, "$all");

				if(!*pkg_file || pkg_delete_after_install)
				{
					strcpy(pkg_file, DEFAULT_PKG_PATH);
				}
				else if( *pkg_file == '$' )
				{
					strcpy(pkg_file, WM_RES_PATH);
				}

				if(isDir(pkg_file))
				{
					if(install_ps3) // /install_ps3/<path> = install all files
					{
						installPKG_all(pkg_file, false); // keep files
						sys_ppu_thread_sleep(1); // wait to copy "pkgfile" variable in installPKG_combo_thread
					}
					is_binary = WEB_COMMAND;
					goto html_response;
				}

				size_t last_char = strlen(param) - 1;
				if(param[last_char] == '?')
				{
					param[last_char] = NULL;
					get_pkg_size_and_install_time(param + 12);
					if(isDir(install_path))
					{
						strcpy(param, install_path);
						is_binary = FOLDER_LISTING;
						goto html_response;
					}
				}

				char msg[MAX_LINE_LEN]; memset(msg, 0, MAX_LINE_LEN);

				setPluginActive();

				pkg_delete_after_install = (install_ps3 == false);

				int ret = installPKG(param + 12, msg);

				#ifdef WM_REQUEST
				if(!wm_request)
				#endif
				{
					if(!mc) keep_alive = http_response(conn_s, header, param, (ret == FAILED) ? CODE_BAD_REQUEST : CODE_INSTALL_PKG, msg);
				}

				if(!(webman_config->minfo & 1)) show_msg(msg);

				if(pkg_delete_after_install || do_restart)
				{
					if(loading_html) loading_html--;

					wait_for_pkg_install();

					setPluginInactive();
					if(do_restart) goto reboot;
					if(mc) goto exit_handleclient_www;
					sys_ppu_thread_exit(0);
				}

				setPluginInactive();

				goto exit_handleclient_www;
			}
 #endif // #ifdef PKG_HANDLER

 #ifdef PS3_BROWSER
			if(islike(param, "/browser.ps3"))
			{
				// /browser.ps3$rsx_pause                  pause rsx processor
				// /browser.ps3$rsx_continue               continue rsx processor
				// /browser.ps3$block_servers              block url of PSN servers in lv2
				// /browser.ps3$restore_servers            restore url of PSN servers in lv2
				// /browser.ps3$show_idps                  show idps/psid (same as R2+O)
				// /browser.ps3$ingame_screenshot          enable screenshot in-game on CFW without the feature (same as R2+O)
				// /browser.ps3$disable_syscalls           disable CFW syscalls
				// /browser.ps3$toggle_rebug_mode          toggle rebug mode (swap VSH REX/DEX)
				// /browser.ps3$toggle_normal_mode         toggle normal mode (swap VSH REX/CEX)
				// /browser.ps3$toggle_debug_menu          toggle debug menu (DEX/CEX)
				// /browser.ps3$toggle_cobra               toggle Cobra (swap stage2)
				// /browser.ps3$toggle_ps2emu              toggle ps2emu
				// /browser.ps3$enable_classic_ps2_mode    creates 'classic_ps2_mode' to enable PS2 classic in PS2 Launcher (old rebug)
				// /browser.ps3$disable_classic_ps2_mode   deletes 'classic_ps2_mode' to enable PS2 ISO in PS2 Launcher (old rebug)
				// /browser.ps3$screenshot_xmb<path>       capture XMB screen
				// /browser.ps3?<url>                      open url on PS3 browser
				// /browser.ps3/<webman_cmd>               execute webMAN command on PS3 browser
				// /browser.ps3$<explore_plugin_command>   execute explore_plugin command on XMB (http://www.psdevwiki.com/ps3/Explore_plugin#Example_XMB_Commands)
				// /browser.ps3*<xmb_plugin_command>       execute xmb_plugin commands on XMB (http://www.psdevwiki.com/ps3/Xmb_plugin#Function_23_Examples)
				// /browser.ps3$slaunch                    start slaunch
				// /browser.ps3$vsh_menu                   start vsh_menu

				char *param2 = param + 12, *url = param + 13;

				if(islike(param2, "$rsx"))
				{
					static u8 rsx = 1;
					if(strstr(param2, "pause")) rsx = 1;
					if(strstr(param2, "cont"))  rsx = 0;
					rsx_fifo_pause(rsx); // rsx_pause / rsx_continue
					rsx ^= 1;
				}
				else
				if(islike(param2, "$block_servers"))
				{
					block_online_servers(true);
				}
				else
 				if(islike(param2, "$restore_servers"))
				{
					restore_blocked_urls();
				}
				else
#ifdef SPOOF_CONSOLEID
				if(islike(param2, "$show_idps"))
				{
					show_idps(header);
				}
				else
#endif
/*
				if(islike(param2, "$registryInt(0x"))
				{
					int id, value;
					id = convertH(param2 + 15);

					char *pos = strstr(param2 + 16, "=");
					if(pos)
					{
						value = val(pos + 1);
						xsetting_D0261D72()->saveRegistryIntValue(id, value);
					}

					xsetting_D0261D72()->loadRegistryIntValue(id, &value);
					sprintf(param2 + strlen(param2), " => %i", value);
				}
				else
				if(islike(param2, "$registryString(0x"))
				{
					int id, len, size = 0;
					id = convertH(param2 + 18);

					char *pos = strstr(param2 + 19, "=");
					if(pos)
					{
						pos++, len = strlen(pos);
						xsetting_D0261D72()->saveRegistryStringValue(id, pos, len);
					}

					len = strlen(param2); char *value = param2 + len + 8;
					char *pos2 = strstr(param2 + 19, ","); if(pos2) size = val(pos2 + 1); if(size <= 0) size = 0x80;
					xsetting_D0261D72()->loadRegistryStringValue(id, value, size);
					sprintf(param2 + len, " => %s", value);
				}
				else
*/
   #ifndef LITE_EDITION
				if(islike(param2, "$ingame_screenshot"))
				{
					enable_ingame_screenshot();
				}
				else
   #endif
   #ifdef REMOVE_SYSCALLS
				if(islike(param2, "$disable_syscalls"))
				{
					disable_cfw_syscalls(strcasestr(param, "ccapi")!=NULL);
				}
				else
   #endif
   #ifdef REX_ONLY
				if(islike(param2, "$toggle_rebug_mode"))
				{
					if(toggle_rebug_mode()) goto reboot;
				}
				else
				if(islike(param2, "$toggle_normal_mode"))
				{
					if(toggle_normal_mode()) goto reboot;
				}
				else
				if(islike(param2, "$toggle_debug_menu"))
				{
					toggle_debug_menu();
				}
				else
   #endif
   #ifdef COBRA_ONLY
    #ifndef LITE_EDITION
				if(islike(param2, "$toggle_cobra"))
				{
					if(toggle_cobra()) goto reboot;
				}
				else
				if(islike(param2, "$toggle_ps2emu"))
				{
					toggle_ps2emu();
				}
				else
				if(strstr(param2, "le_classic_ps2_mode"))
				{
					bool classic_ps2_enabled;

					if(islike(param2, "$disable_"))
					{
						// $disable_classic_ps2_mode
						classic_ps2_enabled = true;
					}
					else
					if(islike(param2, "$enable_"))
					{
						// $enable_classic_ps2_mode
						classic_ps2_enabled = false;
					}
					else
					{
						// $toggle_classic_ps2_mode
						classic_ps2_enabled = file_exists(PS2_CLASSIC_TOGGLER);
					}

					if(classic_ps2_enabled)
						disable_classic_ps2_mode();
					else
						enable_classic_ps2_mode();

					sprintf(header, "PS2 Classic %s", classic_ps2_enabled ? STR_DISABLED : STR_ENABLED);
					show_msg(header);
					sys_ppu_thread_sleep(3);
				}
				else
    #endif //#ifndef LITE_EDITION
   #endif // #ifdef COBRA_ONLY
				if(IS_ON_XMB || *param2 == '?' || *param2 == '/')
				{   // in-XMB
   #ifdef COBRA_ONLY
					if(islike(param2, "$vsh_menu")) {start_vsh_gui(true); sprintf(param, "/cpursx.ps3"); goto html_response;}
					else
					if(islike(param2, "$slaunch")) {start_vsh_gui(false); sprintf(param, "/cpursx.ps3"); goto html_response;}
					else
   #endif
   #ifdef XMB_SCREENSHOT
					if(islike(param2, "$screenshot_xmb")) {sprintf(header, "%s", param + 27); saveBMP(header, false); *url = 0; add_breadcrumb_trail(url, header);} else
   #endif
					{
						if(*param2 == NULL) sprintf(param2, "/");
						if(*param2 == '/' ) {do_umount(false); sprintf(header, "http://%s%s", local_ip, param2); open_browser(header, 0);} else
						if(*param2 == '$' ) {int view = View_Find("explore_plugin"); if(view) {explore_interface = (explore_plugin_interface *)plugin_GetInterface(view, 1); explore_interface->ExecXMBcommand(url,0,0);}} else
						if(*param2 == '?' ) {do_umount(false);  open_browser(url, 0);} else
											{					open_browser(url, 1);} // example: /browser.ps3*regcam:reg?   More examples: http://www.psdevwiki.com/ps3/Xmb_plugin#Function_23

						if(!(webman_config->minfo & 1)) show_msg(url);
					}
				}
				else
					sprintf(url, "ERROR: Not in XMB!");

				if(!mc) keep_alive = http_response(conn_s, header, param, CODE_HTTP_OK, url);

				goto exit_handleclient_www;
			}
 #endif // #ifdef PS3_BROWSER

 #if defined(FIX_GAME) || defined(COPY_PS3)
			if(strstr(param, ".ps3$abort"))
			{
				// /copy.ps3$abort
				// /fixgame.ps3$abort
				do_restart = false;

				if(copy_in_progress) {copy_aborted = true; show_msg((char*)STR_CPYABORT);}   // /copy.ps3$abort
				else
				if(fix_in_progress)  {fix_aborted = true;  show_msg((char*)"Fix aborted!");} // /fixgame.ps3$abort

				sprintf(param, "/");
			}
 #endif

 #ifdef GET_KLICENSEE
			if(islike(param, "/klic.ps3"))
			{
				// /klic.ps3           Show status of auto log klicensees
				// /klic.ps3?log       Toggle auto log of klicensees
				// /klic.ps3?auto      Auto-Log: Started
				// /klic.ps3?off       Auto-Log: Stopped
				// /dev_hdd0/klic.log  Log file

				if(npklic_struct_offset == 0)
				{
					// get klicensee struct
					vshnet_5EE098BC = getNIDfunc("vshnet", 0x5EE098BC, 0);
					int* func_start = (int*)(*((int*)vshnet_5EE098BC));
					npklic_struct_offset = (((*func_start) & 0x0000FFFF) << 16) + ((*(func_start + 5)) & 0x0000FFFF) + 0xC;//8;
				}

				#define KL_OFF     0
				#define KL_GET     1
				#define KL_AUTO    2

				u8 klic_polling_status = klic_polling;

				if(param[10] == 'o') klic_polling = KL_OFF;  else
				if(param[10] == 'a') klic_polling = KL_AUTO; else
				if(param[10] == 'l') klic_polling = klic_polling ? KL_OFF : KL_GET; // toggle

				if((klic_polling_status == KL_OFF) && (klic_polling == KL_AUTO))
				{
					if(IS_ON_XMB) keep_alive = http_response(conn_s, header, param, CODE_HTTP_OK, (char*)"/KLIC: Waiting for game...");

					// wait until game start
					while((klic_polling == KL_AUTO) && IS_ON_XMB && working) sys_ppu_thread_usleep(500000);
				}

				char kl[0x120], prev[0x200], buffer[0x200]; memset(kl, 0, 120);

				if(IS_INGAME)
				{
					#define KLICENSEE_SIZE          0x10
					#define KLICENSEE_OFFSET        (npklic_struct_offset)
					#define KLIC_PATH_OFFSET        (npklic_struct_offset+0x10)
					#define KLIC_CONTENT_ID_OFFSET  (npklic_struct_offset-0xA4)

					hex_dump(kl, KLICENSEE_OFFSET, KLICENSEE_SIZE);
					get_game_info(); sprintf(buffer, "%s %s</H2>"
													 "%s%s<br>"
													 "%s%s<br>"
													 "%s%s<p>",
													 _game_TitleID, _game_Title,
													 "KLicensee: ",  hex_dump(kl, KLICENSEE_OFFSET, KLICENSEE_SIZE),
													 "Content ID: ", (char*)(KLIC_CONTENT_ID_OFFSET),
													 "File: ",       (char*)(KLIC_PATH_OFFSET));
				}
				else
					{sprintf(buffer, "ERROR: <a style=\"%s\" href=\"play.ps3\">%s</a><p>", HTML_URL_STYLE, "KLIC: Not in-game!"); klic_polling = KL_OFF; show_msg((char*)"KLIC: Not in-game!");}

				sprintf(prev, "%s", ((klic_polling_status) ? (klic_polling ? "Auto-Log: Running" : "Auto-Log: Stopped") :
															((klic_polling == KL_GET)  ? "Added to Log" :
															 (klic_polling == KL_AUTO) ? "Auto-Log: Started" : "Enable Auto-Log")));

				sprintf(header, "<a style=\"%s\" href=\"%s\">%s</a>", HTML_URL_STYLE,
							(klic_polling_status > 0 && klic_polling > 0) ? "klic.ps3?off"  :
							( (klic_polling_status | klic_polling)  == 0) ? "klic.ps3?auto" : "dev_hdd0/klic.log", prev); strcat(buffer, header);

				keep_alive = http_response(conn_s, header, param, CODE_HTTP_OK, buffer);

				if(*kl && (klic_polling != KL_OFF))
				{
					get_game_info(); sprintf(header, "%s [%s]", _game_Title, _game_TitleID);

					sprintf(buffer, "%s\n\n%s", header, (char*)(KLIC_PATH_OFFSET));
					show_msg(buffer);

					if(klic_polling == KL_GET)
					{
						sprintf(buffer, "%s%s\n%s%s", "KLicensee: ", kl, "Content ID: ", (char*)(KLIC_CONTENT_ID_OFFSET));
						show_msg(buffer);
					}

					if(klic_polling_status == KL_OFF)
					{
						while((klic_polling != KL_OFF) && IS_INGAME && working)
						{
							hex_dump(kl, (int)KLICENSEE_OFFSET, KLICENSEE_SIZE);
							sprintf(buffer, "%s %s %s %s", kl, (char*)(KLIC_CONTENT_ID_OFFSET), header, (char*)(KLIC_PATH_OFFSET));

							if(klic_polling == KL_AUTO && IS(buffer, prev)) {sys_ppu_thread_usleep(10000); continue;}

							save_file("/dev_hdd0/klic.log", buffer, APPEND_TEXT);

							if(klic_polling == KL_GET) break; strcpy(prev, buffer);
						}

						klic_polling = KL_OFF;
					}
				}

				goto exit_handleclient_www;
			}
 #endif

 #ifndef LITE_EDITION

			#ifdef WEB_CHAT
			if(islike(param, "/chat.ps3"))
			{
				// /chat.ps3    webchat

				is_popup = 1;
				goto html_response;
			}
			#endif
			if(islike(param, "/popup.ps3"))
			{
				// /popup.ps3
				// /popup.ps3<msg>

				if(param[10] == NULL) show_info_popup = true; else is_popup = 1;

				goto html_response;
			}
#ifdef COBRA_ONLY
			if(islike(param, "/remap.ps3") || islike(param, "/remap_ps3") || islike(param, "/unmap.ps3"))
			{
				// /remap.ps3<path1>&to=<path2>       files on path1 is accessed from path2
				// /remap.ps3?src=<path1>&to=<path2>
				// /remap_ps3<path1>&to=<path2>       files on path1 is accessed from path2 (no check if folder exists)
				// /remap_ps3?src=<path1>&to=<path2>
				// /unmap.ps3<path>
				// /unmap.ps3?src=<path>

				char *path1 = header, *path2 = header + MAX_PATH_LEN, *url = header + 2 * MAX_PATH_LEN, *title = header + 2 * MAX_PATH_LEN;

				memset(header, 0, HTML_RECV_SIZE);

				if(param[10] == '/') get_value(path1, param + 10, MAX_PATH_LEN); else
				if(param[11] == '/') get_value(path1, param + 11, MAX_PATH_LEN); else
				{
					get_param("src=", path1, param, MAX_PATH_LEN);
				}

				bool isremap = (param[1] == 'r');
				bool nocheck = (param[6] == '_');

				if(isremap)
				{
					sprintf(path2, "/dev_bdvd");
					get_param("to=", path2, param, MAX_PATH_LEN);
				}

				if(param[10] == 0)
				{
					sys_map_path(NULL, NULL);
					apply_remaps();
				}
				else if(*path1 && (path1[1] != NULL) && (nocheck || file_exists(isremap ? path2 : path1)))
				{
					sys_map_path(path1, path2);

					htmlenc(url, path2, 1); urlenc(url, path1); htmlenc(title, path1, 0);

					if(isremap && (*path2 != NULL))
					{
						htmlenc(path1, path2, 0);
						sprintf(param,  "Remap: " HTML_URL "<br>"
										"To: "    HTML_URL "<p>"
										"Unmap: " HTML_URL2, url, title, path1, path2, "/unmap.ps3", url, title);
					}
					else
					{
						sprintf(param, "Unmap: " HTML_URL, url, title);
					}
				}
				else
					sprintf(param, "%s: %s %s", STR_ERROR, (isremap ? path2 : path1), STR_NOTFOUND);

				if(!mc) keep_alive = http_response(conn_s, header, param, CODE_HTTP_OK, param);

				goto exit_handleclient_www;
			}
			else if(islike(param, "/earth.ps3"))
			{
				if(param[10] == '?')
					map_earth((u8)val(param + 11), param);
				else
					map_earth(0, param);

				keep_alive = http_response(conn_s, header, param, CODE_HTTP_OK, param);
				goto exit_handleclient_www;
			}
#endif
			if(islike(param, "/wait.ps3"))
			{
				// /wait.ps3?<secs>
				// /wait.ps3/<path>

				if(param[9] == '/')
					wait_for(param + 9, 30);
				else
					sys_ppu_thread_sleep(val(param + 10));

				if(!mc) keep_alive = http_response(conn_s, header, param, CODE_HTTP_OK, param);

				goto exit_handleclient_www;
			}
			if(islike(param, "/netstatus.ps3"))
			{
				// /netstatus.ps3          toggle network access in registry
				// /netstatus.ps3?1        enable network access in registry
				// /netstatus.ps3?enable   enable network access in registry
				// /netstatus.ps3?0        disable network access in registry
				// /netstatus.ps3?disable  disable network access in registry

				// /netstatus.ps3?ftp      ftp is running?
				// /netstatus.ps3?netsrv   netsrv is running?
				// /netstatus.ps3?ps3mapi  ps3mapi is running?

				// /netstatus.ps3?stop-ftp      stop ftp server
				// /netstatus.ps3?stop-netsrv   stop net server
				// /netstatus.ps3?stop-ps3mapi  stop ps3mapi server
				// /netstatus.ps3?stop          stop ps3mapi+net+ftp servers

				s32 status = 0; char *label = NULL;

				if(param[15] == 'f') {label = param + 15, status = ftp_working;} else //ftp
#ifdef PS3NET_SERVER
				if(param[15] == 'n') {label = param + 15, status = net_working;} else //netsrv
#endif
#ifdef PS3MAPI
				if(param[15] == 'p') {label = param + 15, status = ps3mapi_working;} else //ps3mapi
#endif
				if( param[15] == 's') //stop
				{
					if( param[19] == 0 || param[20] == 'f') {label = param + 20, ftp_working = 0;} //ftp
#ifdef PS3NET_SERVER
					if( param[19] == 0 || param[20] == 'n') {label = param + 20, net_working = 0;} //netsrv
#endif
#ifdef PS3MAPI
					if( param[19] == 0 || param[20] == 'p') {label = param + 20, ps3mapi_working = 0;} //ps3mapi
#endif
				}
				else
				{
					if(param[14] == 0) {status ^= 1; xsetting_F48C0548()->SetSettingNet_enable(status);} else
					if(param[15] == 0) ; else // query status
					if( param[15] & 1) xsetting_F48C0548()->SetSettingNet_enable(1); else //enable
					if(~param[15] & 1) xsetting_F48C0548()->SetSettingNet_enable(0);      //disable

					xsetting_F48C0548()->GetSettingNet_enable(&status);
				}

				sprintf(param, "%s : %s", label ? to_upper(label) : "Network", status ? STR_ENABLED : STR_DISABLED);

				#ifdef WM_REQUEST
				if(!wm_request)
				#endif
				{
					if(!mc) keep_alive = http_response(conn_s, header, "/netstatus.ps3", CODE_HTTP_OK, param);
				}

				show_msg(param);

				goto exit_handleclient_www;
			}
			if(islike(param, "/edit.ps3"))
			{
				// /edit.ps3<file>              open text file (up to 2000 bytes)
				// /edit.ps3?f=<file>&t=<txt>   saves text to file

				is_popup = 1;
				goto html_response;
			}
	#ifdef COPY_PS3
			if(islike(param, "/copy")) {if(!copy_in_progress) dont_copy_same_size = (param[5] == '.'); param[5] = '.';} //copy_ps3 -> force copy files of the same files

			if(islike(param, "/rmdir.ps3"))
			{
				// /rmdir.ps3        deletes history files & remove empty ISO folders
				// /rmdir.ps3<path>  removes empty folder

				if(param[10] == '/')
				{
					sprintf(param, "%s", param + 10);
#ifdef USE_NTFS
					if(is_ntfs_path(param)) {param[10] = ':'; ps3ntfs_unlink(param + 5);}
					else
#endif
					cellFsRmdir(param);
					char *p = strrchr(param, '/'); *p = NULL;
				}
				else
					{delete_history(true); sprintf(param, "/dev_hdd0");}

				is_binary = FOLDER_LISTING;
				goto html_response;
			}
			if(islike(param, "/mkdir.ps3"))
			{
				// /mkdir.ps3        creates ISO folders in hdd0
				// /mkdir.ps3<path>  creates a folder & parent folders

				if(param[10] == '/')
				{
					sprintf(param, "%s", param + 10);

					filepath_check(param);

					mkdir_tree(param);
					cellFsMkdir(param, DMODE);
				}
				else
				{
					mkdirs(param); // make hdd0 dirs GAMES, PS3ISO, PS2ISO, packages, etc.
				}

				is_binary = FOLDER_LISTING;
				goto html_response;
			}
			else
			if(islike(param, "/write.ps3") || islike(param, "/write_ps3"))
			{
				// /write.ps3<path>&t=<text> use | for line break (create text file)
				// /write_ps3<path>&t=<text> use | for line break (add text to file)
				// /write_ps3<path>&t=<text>&line=<num> insert line(s) to text file in line position
				// /write.ps3<path>&t=<text>&line=<num> replace line of text file in line position
				// /write.ps3<path>&t=<hex>&pos=<offset>          (patch file)
				// /write.ps3?f=<path>&t=<text> use | for line break (create text file)
				// /write_ps3?f=<path>&t=<text> use | for line break (add text to file)
				// /write_ps3?f=<path>&t=<text>&line=<num> insert line(s) to text file in line position
				// /write.ps3?f=<path>&t=<text>&line=<num> replace line of text file in line position
				// /write.ps3?f=<path>&t=<hex>&pos=<offset>       (patch file)
				u64 offset = 0; u32 size = 0;
				char *filename = param + ((param[10] == '/') ? 10 : 13);
				char *pos = strstr(filename, "&t=");
				if(pos)
				{
					*pos = NULL;
					char *data = pos + 3;
					filepath_check(filename);

					pos = strstr(data, "&pos=");

					if(pos)
					{
						*pos = NULL, pos += 5;

						//  get data offset
						if(islike(pos, "0x"))
							offset = convertH(pos);
						else
							offset = val(pos);

						//  write binary data
						size = Hex2Bin(data, header);
						write_file(filename, CELL_FS_O_CREAT | CELL_FS_O_WRONLY, header, offset, size, false);
					}
					else
					{
						bool overwrite = islike(param, "/write.ps3");

						// convert pipes to line breaks
						for(pos = data; *pos; ++pos) if(*pos == '|') *pos = '\n';

						pos = strstr(data, "&line=");

						if(pos)
						{
							// write or insert data at line number
							sys_addr_t sysmem = NULL;
							if(sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK)
							{
								*pos = NULL, pos += 6;
								u16 line = val(pos);

								strcat(data, "\n");
								u16 len = strlen(data);

								char *buffer = (char*)sysmem;
								size = read_file(filename, buffer, _64KB_, 0);

								if(line <= 1)
								{
									save_file(filename, data, SAVE_ALL); // write line data
									if(overwrite)
									{
										pos = strstr(buffer, "\n");
										if(pos) {size -= ++pos - buffer; buffer = pos;} // skip first line
									}
									write_file(filename, CELL_FS_O_APPEND | CELL_FS_O_CREAT | CELL_FS_O_WRONLY, buffer, len + 1, size, false);
									//save_file(filename, buffer, -size);  // append rest of file
								}
								else if(size + len < _64KB_)
								{
									u16 i, c = 0, w = 0; --line;
									// find offset of line to write
									for(i = 0; i < size; i++) if(buffer[i] == '\n' && (++c >= line)) {i++; break;}
									// skip line to overwrite
									if(overwrite)
									{
										// find end of current line
										for(w = i; w < size; w++) if(buffer[w] == '\n') {w++; break;}
										// remove current line
										for(c = i; c < size; c++, w++) buffer[c] = buffer[w];
										size -= (w - c);
									}
									// move forward rest of file
									for(c = size; c >= i; c--) buffer[c + len] = buffer[c];
									// set line data
									for(c = 0; c < len; c++) buffer[i + c] = data[c];
									// write file data
									save_file(filename, buffer, SAVE_ALL);
								}
								sys_memory_free(sysmem);
							}
						}
						else if(overwrite)
							save_file(filename, data, SAVE_ALL); // write.ps3 (write file)
						else
							save_file(filename, data, APPEND_TEXT); // write_ps3 (add line)
					}
				}
				keep_alive = http_response(conn_s, header, param, CODE_BREADCRUMB_TRAIL, param);
				goto exit_handleclient_www;
			}
			if(islike(param, "/rename.ps3") || islike(param, "/swap.ps3") || islike(param, "/move.ps3"))
			{
				// /rename.ps3<path>|<dest>       rename <path> to <dest>
				// /rename.ps3<path>&to=<dest>    rename <path> to <dest>
				// /rename.ps3<path>.bak          removes .bak extension
				// /rename.ps3<path>.bak          removes .bak extension
				// /rename.ps3<path>|<dest>?restart.ps3
				// /rename.ps3<path>&to=<dest>?restart.ps3
				// /move.ps3<path>|<dest>         move <path> to <dest>
				// /move.ps3<path>&to=<dest>      move <path> to <dest>
				// /move.ps3<path>|<dest>?restart.ps3
				// /move.ps3<path>&to=<dest>?restart.ps3
				// /swap.ps3<file1>|<file2>       swap <file1> & <file2>
				// /swap.ps3<file1>&to=<file2>    swap <file1> & <file2>
				// /swap.ps3<file1>|<file2>?restart.ps3
				// /swap.ps3<file1>&to=<file2>?restart.ps3

				#define SWAP_CMD	9
				#define RENAME_CMD	11

				size_t cmd_len = islike(param, "/rename.ps3") ? RENAME_CMD : SWAP_CMD;

				char *source = param + cmd_len, *dest = strstr(source, "|");
				if(dest) {*dest++ = NULL;} else {dest = strstr(source, "&to="); if(dest) {*dest = NULL, dest+=4;}}

				if(dest && (*dest != '/') && !extcmp(source, ".bak", 4)) {size_t flen = strlen(source); *dest = *param + flen; strncpy(dest, source, flen - 4);}

				if(islike(source, "/dev_blind") || islike(source, "/dev_hdd1")) mount_device(source, NULL, NULL); // auto-mount source device
				if(islike(dest,   "/dev_blind") || islike(dest,   "/dev_hdd1")) mount_device(dest,   NULL, NULL); // auto-mount destination device

				if(dest && *dest == '/')
				{
					filepath_check(dest);
#ifdef COPY_PS3
					char *wildcard = strstr(source, "*");
					if(wildcard)
					{
						*wildcard++ = NULL;
						scan(source, true, wildcard, SCAN_MOVE, dest);
					}
					else if(islike(param, "/move.ps3"))
					{
						if(isDir(source))
						{
							if(folder_copy(source, dest) >= CELL_OK) del(source, true);
						}
						else
						{
							if(file_copy(source, dest, COPY_WHOLE_FILE) >= CELL_OK) del(source, false);
						}
					}
					else
#endif
					if((cmd_len == SWAP_CMD) && file_exists(source) && file_exists(dest))
					{
						sprintf(header, "%s.bak", source);
						cellFsRename(source, header);
						cellFsRename(dest, source);
						cellFsRename(header, dest);
					}
#ifdef USE_NTFS
					else if(is_ntfs_path(source) || is_ntfs_path(dest))
					{
						u8 ps = 0, pt = 0;
						if(is_ntfs_path(source)) {ps = 5, source[10] = ':';}
						if(is_ntfs_path(dest)) {pt = 5, dest[10] = ':';}

						ps3ntfs_rename(source + ps, dest + pt);
					}
#endif
					else
						cellFsRename(source, dest);

					char *p = strrchr(dest, '/'); *p = NULL;
					sprintf(param, "%s", dest);
					if(do_restart) goto reboot;
				}
				else
				{
					char *p = strrchr(source, '/'); *p = NULL;
					sprintf(param, "%s", source);
					if(!isDir(param)) sprintf(param, "/");
				}

				is_binary = FOLDER_LISTING;
				goto html_response;
			}
			else
			if(islike(param, "/cpy.ps3") || islike(param, "/cut.ps3"))
			{
				// /cpy.ps3<path>  stores <path> in <cp_path> clipboard buffer for copy with /paste.ps3 (cp_mode = 1)
				// /cut.ps3<path>  stores <path> in <cp_path> clipboard buffer for move with /paste.ps3 (cp_mode = 2)

				cp_mode = islike(param, "/cut.ps3") ? CP_MODE_MOVE : CP_MODE_COPY;
				snprintf(cp_path, STD_PATH_LEN, "%s", param + 8);
				sprintf(param, "%s", cp_path);
				char *p = strrchr(param, '/'); *p = NULL;
				if(not_exists(cp_path)) cp_mode = CP_MODE_NONE;

				is_binary = FOLDER_LISTING;
				goto html_response;
			}
			else
			if(islike(param, "/paste.ps3"))
			{
				// /paste.ps3<path>  performs a copy or move of path stored in <cp_path clipboard> to <path> indicated in url

				#define PASTE_CMD	10

				char *source = header, *dest = cp_path;
				if(file_exists(cp_path))
				{
					sprintf(source, "/copy.ps3%s", cp_path);
					sprintf(dest, "%s", param + PASTE_CMD);
					sprintf(param, "%s", source); strcat(dest, strrchr(param, '/'));
					is_binary = WEB_COMMAND;
					goto html_response;
				}
				else
					if(!mc) {keep_alive = http_response(conn_s, header, "/", CODE_GOBACK, HTML_REDIRECT_TO_BACK); goto exit_handleclient_www;}
			}
	#endif // #ifdef COPY_PS3

			if(islike(param, "/minver.ps3"))
			{
				u8 data[0x20];
				memset(data, 0, 0x20);

				int ret = GetApplicableVersion(data);
				if(ret)
					sprintf(param, "Applicable Version failed: %x\n", ret);
				else
					sprintf(param, "Minimum Downgrade: %x.%02x", data[1], data[3]);

				keep_alive = http_response(conn_s, header, param, CODE_HTTP_OK, param);
				goto exit_handleclient_www;
			}
	#ifdef SPOOF_CONSOLEID
			bool is_psid = islike(param, "/psid.ps3");
			bool is_idps = islike(param, "/idps.ps3");

			if(islike(param, "/consoleid.ps3")) is_psid = is_idps = true;

			if(is_psid | is_idps)
			{
				// /idps.ps3        copy idps & act.dat (current user) to /dev_usb000 or /dev_hdd0
				// /idps.ps3<path>  copy idps to <path> (binary file)
				// /psid.ps3        copy psid & act.dat (current user) to /dev_usb000 or /dev_hdd0
				// /psid.ps3<path>  copy psid to <path> (binary file)
				// /consoleid.ps3   copy psid, idps & act.dat (current user) to /dev_usb000 or /dev_hdd0

				save_idps_psid(is_psid, is_idps, header, param);

				keep_alive = http_response(conn_s, header, param, CODE_HTTP_OK, param);
				goto exit_handleclient_www;
			}
	#endif
	#ifdef SECURE_FILE_ID // TO-DO: this feature is broken
			if(islike(param, "/secureid.ps3"))
			{
				hook_savedata_plugin();
				sprintf(param, "Save data plugin: %s", securfileid_hooked ? STR_ENABLED : STR_DISABLED);

				if(file_exists("/dev_hdd0/secureid.log") {sprintf(header, HTML_URL, "/dev_hdd0/secureid.log", "/dev_hdd0/secureid.log"); strcat(param, "<p>Download: "); strcat(param, header);}

				keep_alive = http_response(conn_s, header, param, CODE_HTTP_OK, param);
				goto exit_handleclient_www;
			}
	#endif

			if(islike(param, "/syscall.ps3") || islike(param, "/syscall_ps3"))
			{
				// /syscall.ps3?<syscall-number>|0x<hex-value>|<decimal-value>|<string-value>
				// e.g. http://localhost/syscall.ps3?392|0x1004|0x07|0x036
				//      http://localhost/syscall.ps3?837|CELL_FS_UTILITY:HDD1|CELL_FS_FAT|/dev_hdd1|0|0|0|0
				//      http://localhost/syscall.ps3?838|/dev_hdd1|0|1

				char *p, *params = param + 13;
				u64 sp[10], ret = 0; u8 n;
				u16 sc = (u16)val(params);

				u8 is_plain = islike(param, "/syscall_ps3");

				for(n = 0; n <= 8; n++)
				{
					sp[n] = 0, p = strstr(params, "|"); if(!p) break;
					params = p + 1, p[0] = NULL;
					sp[n] = (*params == '0') ? convertH(params) : (u64)val(params); if(!sp[n] && (*params != '0')) sp[n] = (u64)(u32)(params);
				}

				if(sc == 840)
				{	// FS_DISK_FREE
					ret = get_free_space(params + 4);
					sprintf(param, "%ll bytes free", ret);
				}
				else
				if(sc == 200 || sc == 904)
				{	// ccapi_get_process_mem || dex_get_process_mem
					if(sp[2] > MAX_LINE_LEN) sp[2] = MAX_LINE_LEN; memset(header, 0, sp[2]);
					{system_call_4(sc, sp[0], sp[1], sp[2], (uint64_t)(uint32_t)header); ret = p1;}

					for(int i = 0; i < (u16)sp[2]; i++) sprintf(param + (2 * i), "%02X", (u8)header[i]);
				}
				else
				{
					switch (n)
					{
						case 0: {system_call_0(sc); ret = p1;} break;
						case 1: {system_call_1(sc, sp[0]); ret = p1;} break;
						case 2: {system_call_2(sc, sp[0], sp[1]); ret = p1;} break;
						case 3: {system_call_3(sc, sp[0], sp[1], sp[2]); ret = p1;} break;
						case 4: {system_call_4(sc, sp[0], sp[1], sp[2], sp[3]); ret = p1;} break;
						case 5: {system_call_5(sc, sp[0], sp[1], sp[2], sp[3], sp[4]); ret = p1;} break;
						case 6: {system_call_6(sc, sp[0], sp[1], sp[2], sp[3], sp[4], sp[5]); ret = p1;} break;
						case 7: {system_call_7(sc, sp[0], sp[1], sp[2], sp[3], sp[4], sp[5], sp[6]); ret = p1;} break;
						case 8: {system_call_8(sc, sp[0], sp[1], sp[2], sp[3], sp[4], sp[5], sp[6], sp[7]); ret = p1;} break;
					}

					if(is_plain)
					{
						sprintf(param, "0x%x", ret); if(!ret) param[1] = 0;
					}
					else
						sprintf(param, "syscall%i(%i) => 0x%x", n, sc, ret);
				}

				keep_alive = http_response(conn_s, header, "/syscall.ps3", is_plain ? CODE_PLAIN_TEXT : CODE_HTTP_OK, param);
				goto exit_handleclient_www;
			}

 #endif //#ifndef LITE_EDITION

			if(islike(param, "/quit.ps3"))
			{
				// quit.ps3        Stops webMAN and sets fan based on settings
				// quit.ps3?0      Stops webMAN and sets fan to syscon mode
				// quit.ps3?1      Stops webMAN and sets fan to fixed speed specified in PS2 mode

 #ifdef LOAD_PRX
 quit:
 #endif
				http_response(conn_s, header, param, CODE_HTTP_OK, param);

				if(sysmem) sys_memory_free(sysmem);

				restore_settings();

				if(strstr(param, "?0")) restore_fan(SYSCON_MODE);  //syscon
				if(strstr(param, "?1")) restore_fan(SET_PS2_MODE); //ps2 mode

				stop_prx_module();
				sys_ppu_thread_exit(0);
			}
			if(islike(param, "/shutdown.ps3"))
			{
				// /shutdown.ps3        Shutdown using VSH
				// /shutdown.ps3?vsh    Shutdown using VSH
				// /shutdown.ps3?off    Power off

				#ifndef EMBED_JS
				css_exists = common_js_exists = false;
				#endif

				http_response(conn_s, header, param, CODE_HTTP_OK, param);
				setPluginExit();

				if(sysmem) sys_memory_free(sysmem);

				del_turnoff(1);

				if(param[14] != 'o')
					vsh_shutdown(); // shutdown using VSH
				else
					{system_call_4(SC_SYS_POWER, SYS_SHUTDOWN, 0, 0, 0);}

				sys_ppu_thread_exit(0);
			}
			if(islike(param, "/rebuild.ps3"))
			{
				// /rebuild.ps3  reboots & start rebuilding file system

				cmd[0] = cmd[1] = 0; cmd[2] = 0x03; cmd[3] = 0xE9; // 00 00 03 E9
				save_file("/dev_hdd0/mms/db.err", cmd, 4);
				goto reboot; // hard reboot
			}
			if(islike(param, "/recovery.ps3"))
			{
				// /recovery.ps3  reboots in pseudo-recovery mode

				#define SC_UPDATE_MANAGER_IF				863
				#define UPDATE_MGR_PACKET_ID_READ_EPROM		0x600B
				#define UPDATE_MGR_PACKET_ID_WRITE_EPROM	0x600C
				#define RECOVER_MODE_FLAG_OFFSET			0x48C61

				{system_call_7(SC_UPDATE_MANAGER_IF, UPDATE_MGR_PACKET_ID_WRITE_EPROM, RECOVER_MODE_FLAG_OFFSET, 0x00, 0, 0, 0, 0);} // set recovery mode
				goto reboot; // hard reboot
			}
			if(islike(param, "/restart.ps3") || islike(param, "/reboot.ps3"))
			{
				// /reboot.ps3           Hard reboot
				// /restart.ps3          Reboot using default mode (VSH reboot is the default); skip content scan on reboot
				// /restart.ps3?0        Allow scan content on restart
				// /restart.ps3?quick    Quick reboot (load LPAR id 1)
				// /restart.ps3?vsh      VSH Reboot
				// /restart.ps3?soft     Soft reboot
				// /restart.ps3?hard     Hard reboot
				// /restart.ps3?<mode>$  Sets the default restart mode for /restart.ps3
				// /restart.ps3?min      Reboot & show min version
 reboot:
				#ifndef EMBED_JS
				css_exists = common_js_exists = false;
				#endif

				http_response(conn_s, header, param, CODE_HTTP_OK, param);
				setPluginExit();

				if(sysmem) sys_memory_free(sysmem);

				del_turnoff(2);

				char *allow_scan = strstr(param,"?0");
				if(allow_scan) *allow_scan = NULL; else save_file(WMNOSCAN, NULL, SAVE_ALL);

				bool is_restart = IS(param, "/restart.ps3");

				char mode = 'h', *param2 = strstr(param, "?");
 #ifndef LITE_EDITION
				if(param2) {mode = param2[1] | 0x20; if(strstr(param, "$")) {webman_config->default_restart = mode; save_settings();}} else if(is_restart) mode = webman_config->default_restart;
 #else
				if(param2)  mode = param2[1] | 0x20; else if(is_restart) mode = webman_config->default_restart;
 #endif
				if(mode == 'q')
					{system_call_3(SC_SYS_POWER, SYS_REBOOT, NULL, 0);} // (quick reboot) load LPAR id 1
				else
				if(mode == 's')
					{system_call_3(SC_SYS_POWER, SYS_SOFT_REBOOT, NULL, 0);} // soft reboot
				else
				if(mode == 'h')
					{system_call_3(SC_SYS_POWER, SYS_HARD_REBOOT, NULL, 0);} // hard reboot
 #ifndef LITE_EDITION
				else
				if(mode == 'm')
					reboot_show_min_version(""); // show min version
 #endif
				else //if(mode == 'v' || is_restart)
					vsh_reboot(); // VSH reboot

				goto exit_handleclient_www;
			}

 #ifdef CALC_MD5
			if(islike(param, "/md5.ps3"))
			{
				char *filename = param + 8, *buffer = header;

				sprintf(buffer, "File: ");
				add_breadcrumb_trail(buffer, filename);

				struct CellFsStat buf; cellFsStat(filename, &buf);
				unsigned long long sz = (unsigned long long)buf.st_size;

				char md5[33];
				calc_md5(filename, md5);

				sprintf(param, "%s<p>Size: %llu bytes<p>MD5: %s<p>", buffer, sz, md5);

				keep_alive = http_response(conn_s, header, "/md5.ps3", CODE_HTTP_OK, param);

				goto exit_handleclient_www;
			}
 #endif

 #ifdef FIX_GAME
			if(islike(param, "/fixgame.ps3"))
			{
				// /fixgame.ps3<path>  fix PARAM.SFO and EBOOT.BIN / SELF / SPRX in ISO or folder

				// fix game folder
				char *game_path = param + 12, titleID[10];
				fix_game(game_path, titleID, FIX_GAME_FORCED);

				is_popup = 1;
				goto html_response;
			}
 #endif

 #ifndef LITE_EDITION
			if(islike(param, "/mount.ps3?"))
			{
				// /mount.ps3?<query>  search game & mount if only 1 match is found

				if(islike(param, "/mount.ps3?http"))
				{
					char *url  = param + 11;
					do_umount(false);  open_browser(url, 0);
					keep_alive = http_response(conn_s, header, param, CODE_HTTP_OK, url);
					goto exit_handleclient_www;
				}
				else
					{memcpy(header, "/index.ps3", 10); memcpy(param, "/index.ps3", 10); auto_mount = true;}
			}
 #endif

			if(islike(param, "/games.ps3"))
			{
				// /games.ps3
				// /index.ps3?mobile
				// /dev_hdd0/xmlhost/game_plugin/mobile.html

 mobile_response:
				mobile_mode = true; char *param2 = param + 10;

				if(not_exists(MOBILE_HTML))
					{sprintf(param, "/index.ps3%s", param2); mobile_mode = false;}
				else if(strstr(param, "?g="))
					sprintf(param, MOBILE_HTML);
				else if(strstr(param, "?"))
					sprintf(param, "/index.ps3%s", param2);
				else if(not_exists(GAMELIST_JS))
					sprintf(param, "/index.ps3?mobile");
				else
					sprintf(param, MOBILE_HTML);
			}
			else mobile_mode = false;

retry_response:
			if(!is_busy && (islike(param, "/index.ps3?") || islike(param, "/refresh.ps3"))) ; else

			if(!is_busy && sys_admin && (islike(param, "/mount.ps3?http")
 #ifdef DEBUG_MEM
							|| islike(param, "/peek.lv2?")
							|| islike(param, "/poke.lv2?")
							|| islike(param, "/find.lv2?")
							|| islike(param, "/peek.lv1?")
							|| islike(param, "/poke.lv1?")
							|| islike(param, "/find.lv1?")
							|| islike(param, "/dump.ps3")
 #endif

 #ifndef LITE_EDITION
							|| islike(param, "/delete.ps3")
							|| islike(param, "/delete_ps3")
 #endif

 #ifdef PS3MAPI
							|| islike(param, "/home.ps3mapi")
							|| islike(param, "/setmem.ps3mapi")
							|| islike(param, "/getmem.ps3mapi")
							|| islike(param, "/led.ps3mapi")
							|| islike(param, "/buzzer.ps3mapi")
							|| islike(param, "/notify.ps3mapi")
							|| islike(param, "/syscall.ps3mapi")
							|| islike(param, "/syscall8.ps3mapi")
							|| islike(param, "/setidps.ps3mapi")
							|| islike(param, "/vshplugin.ps3mapi")
							|| islike(param, "/gameplugin.ps3mapi")
 #endif

 #ifdef COPY_PS3
							|| islike(param, "/copy.ps3/")
 #endif
			)) keep_alive = 0;

			else if(islike(param, "/cpursx.ps3")
				||  islike(param, "/sman.ps3")
				||  islike(param, "/mount_ps3/")
				||  islike(param, "/mount.ps3/")
 #ifdef PS2_DISC
				||  islike(param, "/mount.ps2/")
				||  islike(param, "/mount_ps2/")
 #endif

 #ifdef VIDEO_REC
				||  islike(param, "/videorec.ps3")
 #endif

 #ifdef EXT_GDATA
				||  islike(param, "/extgd.ps3")
 #endif

 #ifdef SYS_BGM
				||  islike(param, "/sysbgm.ps3")
 #endif

 #ifdef LOAD_PRX
				||  islike(param, "/loadprx.ps3")
				||  islike(param, "/unloadprx.ps3")
 #endif
				||  islike(param, "/eject.ps3")
				||  islike(param, "/insert.ps3")) keep_alive = 0;
			else if(islike(param, "/index.ps3")) ;
			else
			{
				struct CellFsStat buf; bool is_net = false;

				if(islike(param, "/dev_hdd1")) mount_device("/dev_hdd1", NULL, NULL); // auto-mount /dev_hdd1
#ifdef USE_NTFS
				is_ntfs = is_ntfs_path(param);
#endif
#ifdef COBRA_ONLY
				if(islike(param, "/net") && (param[4] >= '0' && param[4] <= '4')) //net0/net1/net2/net3/net4
				{
					is_binary = FOLDER_LISTING, is_net = true;
				}
#endif
#ifdef USE_NTFS
				else if(is_ntfs)
				{
					struct stat bufn;

					if(mountCount == NTFS_UNMOUNTED) mount_all_ntfs_volumes();

					char *sort = strstr(param, "?sort=");
					if(sort) {sort_by = sort[6]; if(strstr(sort, "desc")) sort_order = -1; *sort = NULL;}

					param[10] = ':';
					if(param[11] != '/') {param[11] = '/', param[12] = 0;}

					if(ps3ntfs_stat(param + 5, &bufn) < 0)
					{
						keep_alive = http_response(conn_s, header, param, CODE_PATH_NOT_FOUND, "404 Not found");
						goto exit_handleclient_www;
					}

					buf.st_size = bufn.st_size;
					buf.st_mode = bufn.st_mode;

					if(bufn.st_mode & S_IFDIR) is_binary = FOLDER_LISTING;
				}
#endif
				else
					is_binary = (*param == '/') && (cellFsStat(param, &buf) == CELL_FS_SUCCEEDED);

				if(is_binary == BINARY_FILE) ;

				else if(*param == '/')
				{
					char *sort = strstr(param, "?sort=");
					if(sort) {sort_by = sort[6]; if(strstr(sort, "desc")) sort_order = -1; *sort = NULL;}

					sort = strchr(param, '?');
					if(sort)
					{
						file_query = sort + 1;
						*sort = NULL;
					}

					if(is_net || not_exists(param))
					{
						sort = strrchr(param, '#');
						if(sort) *sort = NULL;
					}

					if(is_net) goto html_response;

					if(islike(param, "/favicon.ico")) {sprintf(param, "%s", wm_icons[iPS3]);} else
					if(not_exists(param))
					{
						if(!islike(param, "/dev_"))
						{
							strcpy(header, param + 1);
							if(*html_base_path == '/') {sprintf(param, "%s/%s", html_base_path, header);} // use html path (if path is omitted)
							if(not_exists(param)) {sprintf(param, "%s/%s", HTML_BASE_PATH, header);} // try HTML_BASE_PATH
							if(not_exists(param)) {sprintf(param, "%s/%s", webman_config->home_url, header);} // try webman_config->home_url
							if(not_exists(param)) {sprintf(param, "%s%s",  HDD0_GAME_DIR, header);} // try /dev_hdd0/game
							if(not_exists(param)) {sprintf(param, "%s%s", _HDD0_GAME_DIR, header);} // try /dev_hdd0//game
							if(not_exists(param)) {sprintf(param, "%s/%s", "/dev_hdd0", header);} // try hdd0
						}
					}

					is_binary = is_ntfs || (cellFsStat(param, &buf) == CELL_FS_SUCCEEDED); allow_retry_response = true;
				}

				if(is_binary)
				{
					c_len = buf.st_size;
					if(buf.st_mode & S_IFDIR) {is_binary = FOLDER_LISTING;} // folder listing
				}
#ifdef COPY_PS3
				else if(allow_retry_response && islike(param, "/dev_") && strstr(param, "*") != NULL)
				{
					bool reply_html = !strstr(param, "//");
					char *FILE_LIST = reply_html ? (char*)"/dev_hdd0/tmp/wmtmp/filelist.htm" : (char*)"/dev_hdd0/tmp/wmtmp/filelist.txt";
					cellFsUnlink(FILE_LIST);
					if(reply_html)
					{
#ifndef EMBED_JS
						memset(header, HTML_RECV_SIZE, 0);
						sprintf(header, SCRIPT_SRC_FMT, FS_SCRIPT_JS);
						save_file(FILE_LIST, (const char*)HTML_HEADER, SAVE_ALL);
						save_file(FILE_LIST, (const char*)header, APPEND_TEXT);
						save_file(FILE_LIST, (const char*)"<body onload='try{t2lnks()}catch(e){}' bgcolor=#333 text=white vlink=white link=white><pre>", APPEND_TEXT);
#else
						save_file(FILE_LIST, (const char*)HTML_HEADER, SAVE_ALL);
						save_file(FILE_LIST, (const char*)"<body bgcolor=#333 text=white vlink=white link=white><pre>", APPEND_TEXT);
#endif
					}

					char *wildcard = strstr(param, "*"); if(wildcard) *wildcard++ = NULL;
					scan(param, true, wildcard, SCAN_LIST, (char*)FILE_LIST);

					sprintf(param, "%s", FILE_LIST);
					is_busy = false, allow_retry_response = false; goto retry_response;
				}
#endif
				else
				{
					int code =  is_busy ?				 CODE_SERVER_BUSY :
								islike(param, "/dev_") ? CODE_PATH_NOT_FOUND :
														 CODE_BAD_REQUEST;

					http_response(conn_s, header, param, code, is_busy ? "503 Server is Busy" :
										 (code == CODE_PATH_NOT_FOUND) ? "404 Not found" :
																		 "400 Bad Request");

					keep_alive = 0;
					goto exit_handleclient_www;
				}
			}

 html_response:
			header_len = prepare_header(header, param, is_binary);

			char templn[1024];
			{u16 ulen = strlen(param); if((ulen > 1) && (param[ulen - 1] == '/')) ulen--, param[ulen] = NULL;}
			//sprintf(templn, "X-PS3-Info: %llu [%s]\r\n", (unsigned long long)c_len, param); strcat(header, templn);

			//-- select content profile
			if(strstr(param, ".ps3?"))
			{
				u8 uprofile = profile; char url[10];

				bool is_index_ps3 = islike(param, "/index.ps3?");

				if( is_index_ps3 || islike(param, "/refresh.ps3") ) {char mode, *cover_mode = strstr(param, "?cover="); if(cover_mode) {custom_icon = true; mode = *(cover_mode + 7) | 0x20, *cover_mode = NULL; webman_config->nocov = (mode == 'o') ? ONLINE_COVERS : (mode == 'd' || mode == 'n') ? SHOW_DISC : (mode == 'i') ? SHOW_ICON0 : SHOW_MMCOVERS;}}

				for(u8 i = 0; i < 5; i++)
				{
					sprintf(url, "?%i",    i); if(strstr(param, url)) {profile = i; break;}
					sprintf(url, "usr=%i", i); if(strstr(param, url)) {profile = i; break;}

					if(is_index_ps3) {sprintf(url, "_%i", i); if(strstr(param, url)) {profile = i; break;}}
				}

				if (uprofile != profile) {webman_config->profile = profile; save_settings();}
				if((uprofile != profile) || is_index_ps3) {DELETE_CACHED_GAMES}
			}
			//--

			if(is_binary == BINARY_FILE) // binary file
			{
				if(keep_alive)
				{
					header_len += sprintf(header + header_len,  "Keep-Alive: timeout=3,max=250\r\n"
																"Connection: keep-alive\r\n");
				}

				header_len += sprintf(header + header_len, "Content-Length: %llu\r\n\r\n", (unsigned long long)c_len);
				send(conn_s, header, header_len, 0);

				size_t buffer_size = 0; if(sysmem) sys_memory_free(sysmem); sysmem = NULL;

				for(u8 n = MAX_PAGES; n > 0; n--)
					if(c_len >= ((n-1) * _64KB_) && sys_memory_allocate(n * _64KB_, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK) {buffer_size = n * _64KB_; break;}

				//if(!sysmem && sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &sysmem)!=0)
				if(buffer_size < _64KB_)
				{
					keep_alive = http_response(conn_s, header, param, CODE_SERVER_BUSY, STR_ERROR);
					goto exit_handleclient_www;
				}

				if(islike(param, "/dev_bdvd"))
					{system_call_1(36, (u64) "/dev_bdvd");} // decrypt dev_bdvd files

				char *buffer = (char*)sysmem;
				int fd;
#ifdef USE_NTFS
				if(is_ntfs)
				{
					fd = ps3ntfs_open(param + 5, O_RDONLY, 0);
					if(fd <= 0) is_ntfs = false;
				}
#endif
				if(is_ntfs || cellFsOpen(param, CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
				{
					u64 read_e = 0, pos;

#ifdef USE_NTFS
					if(is_ntfs) ps3ntfs_seek64(fd, 0, SEEK_SET);
					else
#endif
					cellFsLseek(fd, 0, CELL_FS_SEEK_SET, &pos);

					while(working)
					{
#ifdef USE_NTFS
						if(is_ntfs) read_e = ps3ntfs_read(fd, (void *)buffer, BUFFER_SIZE_FTP);
#endif
						if(is_ntfs || cellFsRead(fd, (void *)buffer, buffer_size, &read_e) == CELL_FS_SUCCEEDED)
						{
							if(read_e > 0)
							{
								if(send(conn_s, buffer, (size_t)read_e, 0) < 0) break;
							}
							else
								break;
						}
						else
							break;

						//sys_ppu_thread_usleep(1668);
					}
#ifdef USE_NTFS
					if(is_ntfs) ps3ntfs_close(fd);
					else
#endif
					cellFsClose(fd);
				}

				goto exit_handleclient_www;
			}

			u32 BUFFER_SIZE_HTML = _64KB_;

			if(show_info_popup || islike(param, "/cpursx.ps3")) is_cpursx = 1;
			else
			if((is_binary == FOLDER_LISTING) || islike(param, "/index.ps3") || islike(param, "/sman.ps3"))
			{
				sys_memory_container_t vsh_mc = get_vsh_memory_container();
				if(vsh_mc && sys_memory_allocate_from_container(_3MB_, vsh_mc, SYS_MEMORY_PAGE_SIZE_1M, &sysmem) == CELL_OK) BUFFER_SIZE_HTML = _3MB_;

				if(!sysmem)
				{
					BUFFER_SIZE_HTML = get_buffer_size(webman_config->foot);

					_meminfo meminfo;
					{system_call_1(SC_GET_FREE_MEM, (u64)(u32) &meminfo);}

					if((meminfo.avail)<( (BUFFER_SIZE_HTML) + MIN_MEM)) BUFFER_SIZE_HTML = get_buffer_size(3); //MIN+
					if((meminfo.avail)<( (BUFFER_SIZE_HTML) + MIN_MEM)) BUFFER_SIZE_HTML = get_buffer_size(1); //MIN
				}
			}

			while((!sysmem) && (BUFFER_SIZE_HTML > 0) && sys_memory_allocate(BUFFER_SIZE_HTML, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) != CELL_OK) BUFFER_SIZE_HTML -= _64KB_;

			if(!sysmem)
			{
				keep_alive = http_response(conn_s, header, param, CODE_SERVER_BUSY, STR_ERROR);
				goto exit_handleclient_www;
			}

			char *buffer = (char*)sysmem;
			t_string sbuffer; _alloc(&sbuffer, buffer);

			//else	// text page
			{
				if((is_binary != FOLDER_LISTING) && is_setup)
				{
					setup_parse_settings(param + 11);
				}

				bool mount_ps3 = !is_popup && islike(param, "/mount_ps3"), forced_mount = false;

				if(mount_ps3 && IS_INGAME) {mount_ps3 = false, forced_mount = true, param[6] = '.';}

				if(conn_s != (int)WM_FILE_REQUEST)
					sbuffer.size = prepare_html(buffer, templn, param, is_ps3_http, is_cpursx, mount_ps3);

				char *pbuffer = buffer + sbuffer.size;

				char *tempstr = buffer + BUFFER_SIZE_HTML - _4KB_;

				if(is_cpursx)
				{
					cpu_rsx_stats(pbuffer, templn, param, is_ps3_http);

					loading_html = keep_alive = is_cpursx = 0; goto send_response;

					//CellGcmConfig config; cellGcmGetConfiguration(&config);
					//sprintf(templn, "localAddr: %x", (u32) config.localAddress); _concat(&sbuffer, templn);
				}
				else if((conn_s == (int)WM_FILE_REQUEST)) ;
				else if(webman_config->sman || strstr(param, "/sman.ps3")) {_concat(&sbuffer, "<div id='toolbox'>"); goto skip_code;}
				else if(!mount_ps3)
				{
					{
						char cpursx[32]; get_cpursx(cpursx);

						sprintf(templn, " [<a href=\"%s\">"
										// prevents flickering but cause error 80710336 in ps3 browser (silk mode)
										//"<span id=\"lbl_cpursx\">%s</span></a>]<iframe src=\"/cpursx_ps3\" style=\"display:none;\"></iframe>"
										"<span id=\"err\" style=\"display:none\">%s&nbsp;</span>%s</a>]"
										"<script>function no_error(o){try{var doc=o.contentDocument||o.contentWindow.document;}catch(e){err.style.display='inline-block';o.style.display='none';}}</script>"
										//
										"<hr width=\"100%%\">"
										"<div id=\"rxml\"><H1>%s XML ...</H1></div>"
										"<div id=\"rhtm\"><H1>%s HTML ...</H1></div>"
 #ifdef COPY_PS3
										"<div id=\"rcpy\"><H1><a href=\"/copy.ps3$abort\">&#9746;</a> %s ...</H1></div>"
										//"<form action=\"\">", cpursx, STR_REFRESH, STR_REFRESH, STR_COPYING); _concat(&sbuffer, templn);
										"<form action=\"\">", "/cpursx.ps3", cpursx, is_ps3_http ? cpursx : "<iframe src=\"/cpursx_ps3\" style=\"border:0;overflow:hidden;\" width=\"230\" height=\"23\" frameborder=\"0\" scrolling=\"no\" onload=\"no_error(this)\"></iframe>", STR_REFRESH, STR_REFRESH, STR_COPYING);
 #else
										//"<form action=\"\">", cpursx, STR_REFRESH, STR_REFRESH); _concat(&sbuffer, templn);
										"<form action=\"\">", "/cpursx.ps3", cpursx, is_ps3_http ? cpursx : "<iframe src=\"/cpursx_ps3\" style=\"border:0;overflow:hidden;\" width=\"230\" height=\"23\" frameborder=\"0\" scrolling=\"no\" onload=\"no_error(this)\"></iframe>", STR_REFRESH, STR_REFRESH);
 #endif
						_concat(&sbuffer, templn);
					}

					if((webman_config->homeb) && (strlen(webman_config->home_url)>0))
					{sprintf(templn, HTML_BUTTON_FMT, HTML_BUTTON, STR_HOME, HTML_ONCLICK, webman_config->home_url); _concat(&sbuffer, templn);}

					sprintf(templn, HTML_BUTTON_FMT
									HTML_BUTTON_FMT
									HTML_BUTTON_FMT
 #ifdef EXT_GDATA
									HTML_BUTTON_FMT
 #endif
									, HTML_BUTTON, STR_EJECT, HTML_ONCLICK, "/eject.ps3"
									, HTML_BUTTON, STR_INSERT, HTML_ONCLICK, "/insert.ps3"
									, HTML_BUTTON, STR_UNMOUNT, HTML_ONCLICK, "/mount.ps3/unmount"
 #ifdef EXT_GDATA
									, HTML_BUTTON, "gameDATA", HTML_ONCLICK, "/extgd.ps3"
 #endif
					);

					_concat(&sbuffer, templn);

 skip_code:
 #ifdef COPY_PS3
					if(((islike(param, "/dev_") && strlen(param) > 12 && !strstr(param,"?")) || islike(param, "/dev_bdvd")) && !strstr(param,".ps3/") && !strstr(param,".ps3?"))
					{
						if(copy_in_progress)
							sprintf(templn, "%s&#9746; %s\" %s'%s';\">", HTML_BUTTON, STR_COPY, HTML_ONCLICK, "/copy.ps3$abort");
						else
							sprintf(templn, "%s%s\" onclick='rcpy.style.display=\"block\";location.href=\"/copy.ps3%s\";'\">", HTML_BUTTON, STR_COPY, param);

						_concat(&sbuffer, templn);
					}

					if((islike(param, "/dev_") && !strstr(param,"?")) && !islike(param,"/dev_flash") && !strstr(param,".ps3/") && !strstr(param,".ps3?"))
					{	// add buttons + javascript code to handle delete / cut / copy / paste (requires fm.js)
	#ifdef EMBED_JS
						sprintf(templn, "<script>"
										"function tg(b,m,x,c){"
										"var i,p,o,h,l=document.querySelectorAll('.d,.w'),s=m.length,n=1;"
										"for(i=1;i<l.length;i++){o=l[i];"
										"h=o.href;p=h.indexOf('/cpy.ps3');if(p>0){n=0;s=8;bCpy.value='Copy';}"
										"if(p<1){p=h.indexOf('/cut.ps3');if(p>0){n=0;s=8;bCut.value='Cut';}}"
										"if(p<1){p=h.indexOf('/delete.ps3');if(p>0){n=0;s=11;bDel.value='%s';}}"
										"if(p>0){o.href=h.substring(p+s,h.length);o.style.color='#ccc';}"
										"else{p=h.indexOf('/',8);o.href=m+h.substring(p,h.length);o.style.color=c;}"
										"}if(n)b.value=(b.value == x)?x+' %s':x;"
										"}</script>", STR_DELETE, STR_ENABLED); _concat(&sbuffer, templn);
	#else
						if(file_exists(FM_SCRIPT_JS))
	#endif
						{
							sprintf(templn, "%s%s\" id=\"bDel\" onclick=\"tg(this,'%s','%s','red');\">", HTML_BUTTON, STR_DELETE, "/delete.ps3", STR_DELETE); _concat(&sbuffer, templn);
							sprintf(templn, "%s%s\" id=\"bCut\" onclick=\"tg(this,'%s','%s','magenta');\">", HTML_BUTTON, "Cut", "/cut.ps3", "Cut"); _concat(&sbuffer, templn);
							sprintf(templn, "%s%s\" id=\"bCpy\" onclick=\"tg(this,'%s','%s','blue');\">", HTML_BUTTON, "Copy", "/cpy.ps3", "Copy"); _concat(&sbuffer, templn);

							if(cp_mode) {char *url = tempstr, *title = tempstr + MAX_PATH_LEN; urlenc(url, param); htmlenc(title, cp_path, 0); sprintf(templn, "%s%s\" id=\"bPst\" %s'/paste.ps3%s'\" title=\"%s\">", HTML_BUTTON, "Paste", HTML_ONCLICK, url, title); _concat(&sbuffer, templn);}
						}
					}

 #endif // #ifdef COPY_PS3
					if(webman_config->sman || strstr(param, "/sman.ps3")) {_concat(&sbuffer, "</div>"); goto continue_rendering;}

					sprintf(templn,  "%s%s XML%s\" %s'%s';\"> "
									 "%s%s HTML%s\" %s'%s';\">",
									 HTML_BUTTON, STR_REFRESH, SUFIX2(profile), HTML_ONCLICK, "/refresh.ps3';document.getElementById('rxml').style.display='block",
									 HTML_BUTTON, STR_REFRESH, SUFIX2(profile), HTML_ONCLICK, "/index.ps3?html';document.getElementById('rhtm').style.display='block");

					_concat(&sbuffer, templn);

 #ifdef SYS_ADMIN_MODE
					if(sys_admin)
 #endif
					{
						sprintf(templn,  HTML_BUTTON_FMT
										 HTML_BUTTON_FMT,
										 HTML_BUTTON, STR_SHUTDOWN, HTML_ONCLICK, "/shutdown.ps3",
										 HTML_BUTTON, STR_RESTART,  HTML_ONCLICK, "/restart.ps3");

						_concat(&sbuffer, templn);
					}

 #ifndef LITE_EDITION
					char *nobypass = strstr(param, "$nobypass");
					if(!nobypass) { PS3MAPI_REENABLE_SYSCALL8 } else *nobypass = NULL;
 #endif
					sprintf( templn, "</form><hr>");
					_concat(&sbuffer, templn);
 #ifndef LITE_EDITION
 continue_rendering:
 #endif
 #ifdef COPY_PS3
					if(copy_in_progress)
					{
						sprintf(templn, "%s<a href=\"%s$abort\">&#9746 %s</a> %s (%i %s)", "<div id=\"cps\"><font size=2>", "/copy.ps3", STR_COPYING, current_file, copied_count, STR_FILES);
					}
					else if(fix_in_progress)
					{
						sprintf(templn, "%s<a href=\"%s$abort\">&#9746 %s</a> %s (%i %s)", "<div id=\"cps\"><font size=2>", "/fixgame.ps3", STR_FIXING, current_file, fixed_count, STR_FILES);
					}
					if((copy_in_progress || fix_in_progress) && file_exists(current_file))
					{
						strcat(templn, "</font><p></div><script>setTimeout(function(){cps.style.display='none'},15000);</script>"); _concat(&sbuffer, templn);
					}
 #endif
					keep_alive = 0;
				}

 #ifndef LITE_EDITION
				if(is_popup)
				{
					if(islike(param, "/edit.ps3"))
					{
						// /edit.ps3<file>              open text file (up to 2000 bytes)
						// /edit.ps3?f=<file>&t=<txt>   saves text to file

						char *filename = templn, *txt = buffer + BUFFER_SIZE_HTML - _6KB_, *backup = txt; memset(txt, 0, _2KB_); *filename = 0;

						// get file name
						get_value(filename, param + ((param[9] == '/') ? 9 : 12), MAX_PATH_LEN); // /edit.ps3<file>  *or* /edit.ps3?f=<file>&t=<txt>

						filepath_check(filename);

						if(*filename != '/')
						{
							sprintf(filename, "/dev_hdd0/boot_plugins.txt"); // default file
						}

						char *pos = strstr(param, "&t=");
						if(pos)
						{
							// backup the original text file
							sprintf(backup, "%s.bak", filename);
							cellFsUnlink(backup); // delete previous backup
							cellFsRename(filename, backup);

							// save text file
							sprintf(txt, "%s", pos + 3);
							save_file(filename, txt, SAVE_ALL);
						}
						else
						{
							// load text file
							read_file(filename, txt, MAX_TEXT_LEN, 0);
						}

						// show text box
						sprintf(tempstr,"<form action=\"/edit.ps3\">"
										"<input type=hidden name=\"f\" value=\"%s\">"
										"<textarea name=\"t\" maxlength=%i style=\"width:800px;height:400px;\">%s</textarea><br>"
										"<input class=\"bs\" type=\"submit\" value=\" %s \">",
										filename, MAX_TEXT_LEN, txt, STR_SAVE); _concat(&sbuffer, tempstr);

						// show filename link
						char *p = strrchr(filename, '/');
						if(p)
						{
							if(!extcmp(p, ".bat", 4)) {sprintf(tempstr," [<a href=\"/play.ps3%s\">EXEC</a>]", filename); _concat(&sbuffer, tempstr);}
							strcpy(txt, p); *p = NULL; sprintf(tempstr," &nbsp; " HTML_URL HTML_URL2 "</form>", filename, filename, filename, txt, txt); _concat(&sbuffer, tempstr);
						}
					}
					else
  #ifdef WEB_CHAT
					if(islike(param, "/chat.ps3"))
					{
						// /chat.ps3    webchat

						webchat(buffer, templn, param, tempstr, conn_info_main);
					}
					else
  #endif

  #ifdef FIX_GAME
					if(islike(param, "/fixgame.ps3"))
					{
						// /fixgame.ps3<path>  fix PARAM.SFO and EBOOT.BIN / SELF / SPRX in ISO or folder

						char *game_path = param + 12;
						sprintf(templn, "Fixed: %s", game_path);
						show_msg(templn);

						urlenc(templn, game_path);
						sprintf(tempstr, "Fixed: " HTML_URL, templn, game_path); _concat(&sbuffer, tempstr);

						sprintf(tempstr, HTML_REDIRECT_TO_URL, templn, HTML_REDIRECT_WAIT); _concat(&sbuffer, tempstr);
					}
					else
  #endif
					{
						char *msg = (param + 11); // /popup.ps3?<msg>
						show_msg(msg);
						sprintf(templn, "Message sent: %s", msg); _concat(&sbuffer, templn);
					}

					loading_html = keep_alive = is_popup = 0; goto send_response;
				}
 #endif // #ifndef LITE_EDITION

				////////////////////////////////////

				prev_dest = last_dest = NULL; // init fast concat

				if(is_binary == FOLDER_LISTING) // folder listing
				{
					if(folder_listing(buffer, BUFFER_SIZE_HTML, templn, param, conn_s, tempstr, header, is_ps3_http, sort_by, sort_order, file_query) == false)
					{
						goto exit_handleclient_www;
					}
				}
				else // process web command
				{
					{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }
 #ifndef LITE_EDITION
					if(!strstr(param, "$nobypass")) { PS3MAPI_REENABLE_SYSCALL8 }
 #else
 continue_rendering:
 #endif
					is_busy = true;

					if(islike(param, "/refresh.ps3") && refreshing_xml == 0)
					{
						// /refresh.ps3               refresh XML
						// /refresh.ps3?cover=<mode>  refresh XML using cover type (icon0, mm, disc, online)
						// /refresh.ps3?ntfs          refresh NTFS volumes and show NTFS volumes
						// /refresh.ps3?prepntfs      refresh NTFS volumes & scan ntfs ISOs (clear cached .ntfs[PS*ISO] files in /dev_hdd0/tmp/wmtmp)
						// /refresh.ps3?prepntfs(0)   refresh NTFS volumes & scan ntfs ISOs (keep cached files)
#ifdef USE_NTFS
						if(islike(param + 12, "?ntfs") || islike(param + 12, "?prepntfs"))
						{
							check_ntfs_volumes();

							int ngames = 0; *header = NULL;
							if(islike(param + 12, "?prepntfs"))
							{
								bool clear_ntfs = (strstr(param + 12, "ntfs(0)") != NULL);
								ngames = prepNTFS(clear_ntfs);
								sprintf(header, " • <a href=\"%s\">%i</a> <a href=\"/index.ps3?ntfs\">%s</a>", WMTMP, ngames, STR_GAMES);
							}

							sprintf(param, "NTFS VOLUMES: %i%s", mountCount, header); is_busy = false;

							keep_alive = http_response(conn_s, header, param, CODE_HTTP_OK, param);
							goto exit_handleclient_www;
						}
#endif

						refresh_xml(templn);

						if(strstr(param + 12, "xmb")) reload_xmb();

 #ifndef ENGLISH_ONLY
						sprintf(templn, "<br>");

						char *STR_XMLRF = templn + 4;

						sprintf(STR_XMLRF, "Game list refreshed (<a href=\"%s\">mygames.xml</a>).%s", MY_GAMES_XML, "<br>Click <a href=\"/restart.ps3\">here</a> to restart your PLAYSTATION®3 system.");

						language("STR_XMLRF", STR_XMLRF, STR_XMLRF);
						close_language();

						_concat(&sbuffer, templn);
 #else
						sprintf(templn,  "<br>%s", STR_XMLRF); _concat(&sbuffer, templn);
 #endif
						if(IS_ON_XMB && file_exists(RELOADXMB_EBOOT))
						{
							sprintf(templn, " [<a href=\"/reloadxmb.ps3\">%s XMB</a>]", STR_REFRESH);
							_concat(&sbuffer, templn);
						}
					}
					else
					if(is_setup)
					{
						// /setup.ps3?          reset webman settings
						// /setup.ps3?<params>  save settings

						if(strstr(param, "&") == NULL)
						{
							reset_settings();
						}
						if(save_settings() == CELL_FS_SUCCEEDED)
						{
							sprintf(templn, "<br> %s", STR_SETTINGSUPD); _concat(&sbuffer, templn);
						}
						else
							_concat(&sbuffer, STR_ERROR);
					}
					else
					if(islike(param, "/setup.ps3"))
					{
						// /setup.ps3    setup form with webman settings

						setup_form(pbuffer, templn);
					}
					else
					if(islike(param, "/eject.ps3"))
					{
						// /eject.ps3   eject physical disc from bd drive
						eject_insert(1, 0);
						_concat(&sbuffer, STR_EJECTED);
						sprintf(templn, HTML_REDIRECT_TO_URL, "javascript:history.back();", HTML_REDIRECT_WAIT); _concat(&sbuffer, templn);
					}
					else
					if(islike(param, "/insert.ps3"))
					{
						// /insert.ps3   insert physical disc into bd drive
						eject_insert(0, 1);
						if(!isDir("/dev_bdvd")) eject_insert(1, 1);
						_concat(&sbuffer, STR_LOADED);
						if(isDir("/dev_bdvd")) {sprintf(templn, HTML_REDIRECT_TO_URL, "/dev_bdvd", HTML_REDIRECT_WAIT); _concat(&sbuffer, templn);}
					}
 #ifdef LOAD_PRX
  #ifdef COBRA_ONLY
					else
					if(islike(param, "/loadprx.ps3") || islike(param, "/unloadprx.ps3"))
					{
						// /loadprx.ps3<path-sprx>
						// /loadprx.ps3?prx=<path-sprx>
						// /loadprx.ps3?prx=<path-sprx>&slot=<slot>
						// /unloadprx.ps3?prx=<path-sprx>
						// /unloadprx.ps3?slot=<slot>

						unsigned int slot = 7; bool prx_found;

						if(param[12] == '/') sprintf(templn, "%s", param + 12); else
						if(param[14] == '/') sprintf(templn, "%s", param + 14); else
						{
							sprintf(templn, "%s/%s", "/dev_hdd0/plugins", "webftp_server.sprx");
							if(not_exists(templn)) sprintf(templn + 31, "_ps3mapi.sprx");
							if(not_exists(templn)) sprintf(templn + 10, "webftp_server.sprx");
							if(not_exists(templn)) sprintf(templn + 23, "_ps3mapi.sprx");

							get_param("prx=", templn, param, MAX_PATH_LEN);
						}

						prx_found = file_exists(templn);

						if(prx_found || (*templn != '/'))
						{
							if(*templn)
							{
								slot = get_vsh_plugin_slot_by_name(templn, false);
								if(islike(param, "/unloadprx.ps3")) prx_found = false;
							}
							if((slot < 1) || (slot > 6))
							{
								slot = get_valuen(param, "slot=", 0, 6);
								if(!slot) slot = get_free_slot(); // find first free slot if slot == 0
							}

							if(prx_found)
								sprintf(param, "slot: %i<br>load prx: %s%s", slot, templn, HTML_BODY_END);
							else
								sprintf(param, "unload slot: %i%s", slot, HTML_BODY_END);

							_concat(&sbuffer, param);

							if(slot < 7)
							{
								cobra_unload_vsh_plugin(slot);

								if(prx_found)
									{cobra_load_vsh_plugin(slot, templn, NULL, 0); if(strstr(templn, "/webftp_server")) goto quit;}
							}
						}
						else
							_concat(&sbuffer, STR_NOTFOUND);
					}
  #endif // #ifdef COBRA_ONLY
 #endif // #ifdef LOAD_PRX

 #ifdef PKG_HANDLER
					else
					if(islike(param, "/install.ps3") || islike(param, "/install_ps3"))
					{
						char *pkg_file = param + 12;
						strcat(buffer, "Install PKG: <select autofocus onchange=\"window.location='/install.ps3");
						strcat(buffer, pkg_file);
						strcat(buffer, "/'+this.value;\"><option>");

						int fd, len;
						if(cellFsOpendir(pkg_file, &fd) == CELL_FS_SUCCEEDED)
						{
							CellFsDirectoryEntry entry; size_t read_e;
							while(working)
							{
								if(cellFsGetDirectoryEntries(fd, &entry, sizeof(entry), &read_e) || !read_e) break;
								len = entry.entry_name.d_namlen -4; if(len < 0) continue;
								if(!strcmp(entry.entry_name.d_name + len, ".pkg") || !strcmp(entry.entry_name.d_name + len, ".p3t"))
								{
									strcat(buffer, "<option>");
									strcat(buffer, entry.entry_name.d_name);
								}
							}
							cellFsClosedir(fd);
						}
						strcat(buffer, "</select>");
					}
 #endif

 #ifdef VIDEO_REC
					else
					if(islike(param, "/videorec.ps3"))
					{
						// /videorec.ps3
						// /videorec.ps3<path>
						// /videorec.ps3?<video-rec-params> {mp4, jpeg, psp, hd, avc, aac, pcm, 64, 128, 384, 512, 768, 1024, 1536, 2048}
						// /videorec.ps3?<path>&video=<format>&audio=<format>

						toggle_video_rec(param + 13);
						_concat(&sbuffer,	"<a class=\"f\" href=\"/dev_hdd0\">/dev_hdd0/</a><a href=\"/dev_hdd0/VIDEO\">VIDEO</a>:<p>"
										"Video recording: <a href=\"/videorec.ps3\">");
						_concat(&sbuffer, recording ? STR_ENABLED : STR_DISABLED);
						_concat(&sbuffer, "</a><p>");
						if(!recording) {sprintf(param, "<a class=\"f\" href=\"%s\">%s</a><br>", (char*)recOpt[0x6], (char*)recOpt[0x6]); _concat(&sbuffer, param);}
					}
 #endif

 #ifdef EXT_GDATA
					else
					if(islike(param, "/extgd.ps3"))
					{
						// /extgd.ps3          toggle external GAME DATA
						// /extgd.ps3?         show status of external GAME DATA
						// /extgd.ps3?1        enable external GAME DATA
						// /extgd.ps3?enable   enable external GAME DATA
						// /extgd.ps3?0        disable external GAME DATA
						// /extgd.ps3?disable  disable external GAME DATA

						if( param[10] != '?') extgd ^= 1; else //toggle
						if( param[11] & 1)    extgd  = 1; else //enable
						if(~param[11] & 1)    extgd  = 0;      //disable

						_concat(&sbuffer, "External Game DATA: ");

						if(set_gamedata_status(extgd, true))
							_concat(&sbuffer, STR_ERROR);
						else
							_concat(&sbuffer, extgd ? STR_ENABLED : STR_DISABLED);
					}
 #endif
 #ifdef SYS_BGM
					else
					if(islike(param, "/sysbgm.ps3"))
					{
						// /sysbgm.ps3          toggle in-game background music flag
						// /sysbgm.ps3?         show status of in-game background music flag
						// /sysbgm.ps3?1        enable in-game background music flag
						// /sysbgm.ps3?enable   enable in-game background music flag
						// /sysbgm.ps3?0        disable in-game background music flag
						// /sysbgm.ps3?disable  disable in-game background music flag

						if(param[11] == '?')
						{
							if( param[12] & 1) system_bgm = 0; else //enable
							if(~param[12] & 1) system_bgm = 1;      //disable
						}

						if(param[12] != 's')
						{
							int * arg2;
							if(system_bgm)  {BgmPlaybackDisable(0, &arg2); system_bgm = 0;} else
											{BgmPlaybackEnable(0, &arg2);  system_bgm = 1;}
						}

						sprintf(templn, "System BGM: %s", system_bgm ? STR_ENABLED : STR_DISABLED);
						_concat(&sbuffer, templn);
						show_msg(templn);
					}
 #endif

 #ifndef LITE_EDITION
					else
					if(islike(param, "/delete.ps3") || islike(param, "/delete_ps3"))
					{
						// /delete_ps3<path>      deletes <path>
						// /delete.ps3<path>      deletes <path> and recursively delete subfolders
						// /delete_ps3<path>?restart.ps3
						// /delete.ps3<path>?restart.ps3

						// /delete.ps3?wmreset    deletes wmconfig & clear /dev_hdd0/tmp/wmtmp
						// /delete.ps3?wmconfig   deletes wmconfig
						// /delete.ps3?wmtmp      clear /dev_hdd0/tmp/wmtmp
						// /delete.ps3?history    deletes history files & remove empty ISO folders
						// /delete.ps3?uninstall  uninstall webMAN MOD & delete files installed by updater

						bool is_reset = false; char *param2 = param + 11; int ret = 0;
						if(islike(param2, "?wmreset")) is_reset=true;
						if(is_reset || islike(param2, "?wmconfig")) {reset_settings(); sprintf(param, "/delete_ps3%s", WMCONFIG);}
						if(is_reset || islike(param2, "?wmtmp")) {do_umount(true); sprintf(param, "/delete_ps3%s", WMTMP);}

						bool is_dir = isDir(param2);

						if(islike(param2 , "?history"))
						{
							delete_history(true);
							sprintf(tempstr, "%s : history", STR_DELETE);
							sprintf(param, "/");
						}
						else if(islike(param2 , "?uninstall"))
						{
							struct CellFsStat buf;
							if(cellFsStat("/dev_hdd0/boot_plugins.txt", &buf)             == CELL_FS_SUCCEEDED && buf.st_size < 45) cellFsUnlink("/dev_hdd0/boot_plugins.txt");
							if(cellFsStat("/dev_hdd0/boot_plugins_nocobra.txt", &buf)     == CELL_FS_SUCCEEDED && buf.st_size < 46) cellFsUnlink("/dev_hdd0/boot_plugins_nocobra.txt");
							if(cellFsStat("/dev_hdd0/boot_plugins_nocobra_dex.txt", &buf) == CELL_FS_SUCCEEDED && buf.st_size < 46) cellFsUnlink("/dev_hdd0/boot_plugins_nocobra_dex.txt");

							// delete files
							sprintf(param, "plugins/");
							for(u8 d = 0; d < 2; d++)
							{
								unlink_file("/dev_hdd0", param, "webftp_server.sprx");
								unlink_file("/dev_hdd0", param, "webftp_server_ps3mapi.sprx");
								unlink_file("/dev_hdd0", param, "webftp_server_noncobra.sprx");
								unlink_file("/dev_hdd0", param, "wm_vsh_menu.sprx");
								unlink_file("/dev_hdd0", param, "slaunch.sprx");
								unlink_file("/dev_hdd0", param, "raw_iso.sprx");
								*param = NULL;
							}

							unlink_file("/dev_hdd0", "tmp/", "wm_vsh_menu.cfg");
							unlink_file("/dev_hdd0", "tmp/", "wm_custom_combo");

							cellFsUnlink(WMCONFIG);
							cellFsUnlink(WMNOSCAN);
							cellFsUnlink(WMREQUEST_FILE);
							cellFsUnlink(WMNET_DISABLED);
							cellFsUnlink(WMONLINE_GAMES);
							cellFsUnlink(WMOFFLINE_GAMES);
							cellFsUnlink("/dev_hdd0/boot_init.txt");
							cellFsUnlink("/dev_hdd0/autoexec.bat");

							// delete folders & subfolders
							del(WMTMP, RECURSIVE_DELETE);
							del(WM_RES_PATH, RECURSIVE_DELETE);
							del(WM_LANG_PATH, RECURSIVE_DELETE);
							del(WM_ICONS_PATH, RECURSIVE_DELETE);
							del(WM_COMBO_PATH, RECURSIVE_DELETE);
							del(HTML_BASE_PATH, RECURSIVE_DELETE);
							del(VSH_MENU_IMAGES, RECURSIVE_DELETE);

							sprintf(param, "%s%s", "/delete.ps3", "?uninstall");
							goto reboot;
						}
						else
						{
							if(islike(param2, "/dev_hdd1")) mount_device(param2, NULL, NULL); // auto-mount device

							char *wildcard = strstr(param2, "*"); if(wildcard) *wildcard++ = NULL;
							//ret = del(param2, islike(param, "/delete.ps3"));
							ret = scan(param2, islike(param, "/delete.ps3"), wildcard, SCAN_DELETE, NULL);

							sprintf(tempstr, "%s %s : ", STR_DELETE, ret ? STR_ERROR : ""); _concat(&sbuffer, tempstr);
							add_breadcrumb_trail(pbuffer, param2); sprintf(tempstr, "<br>");
							char *pos = strrchr(param2, '/'); if(pos) *pos = NULL;
						}

						_concat(&sbuffer, tempstr);
						sprintf(tempstr, HTML_REDIRECT_TO_URL, param2, (is_dir | ret) ? HTML_REDIRECT_WAIT : 0); _concat(&sbuffer, tempstr);

						if(do_restart) goto reboot;
					}
 #endif

 #ifdef PS3MAPI
					else
					if(islike(param, "/home.ps3mapi"))
					{
						ps3mapi_home(pbuffer, templn);
					}
					else
					if(islike(param, "/buzzer.ps3mapi"))
					{
						ps3mapi_buzzer(pbuffer, templn, param);
					}
					else
					if(islike(param, "/led.ps3mapi"))
					{
						ps3mapi_led(pbuffer, templn, param);
					}
					else
					if(islike(param, "/notify.ps3mapi"))
					{
						ps3mapi_notify(pbuffer, templn, param);
					}
					else
					if(islike(param, "/syscall.ps3mapi"))
					{
						ps3mapi_syscall(pbuffer, templn, param);
					}
					else
					if(islike(param, "/syscall8.ps3mapi"))
					{
						ps3mapi_syscall8(pbuffer, templn, param);
					}
					else
					if(islike(param, "/getmem.ps3mapi"))
					{
						ps3mapi_getmem(pbuffer, templn, param);
					}
					else
					if(islike(param, "/setmem.ps3mapi"))
					{
						ps3mapi_setmem(pbuffer, templn, param);
					}
					else
					if(islike(param, "/setidps.ps3mapi"))
					{
						ps3mapi_setidps(pbuffer, templn, param);
					}
					else
					if(islike(param, "/vshplugin.ps3mapi"))
					{
						ps3mapi_vshplugin(pbuffer, templn, param);
					}
					else
					if(islike(param, "/gameplugin.ps3mapi"))
					{
						ps3mapi_gameplugin(pbuffer, templn, param);
					}

 #endif // #ifdef PS3MAPI

 #ifdef DEBUG_MEM
					else
					if(islike(param, "/dump.ps3"))
					{
						// /dump.ps3?lv1
						// /dump.ps3?lv2
						// /dump.ps3?rsx
						// /dump.ps3?mem
						// /dump.ps3?full
						// /dump.ps3?<start-address>
						// /dump.ps3?<start-address>&size=<mb>

						ps3mapi_mem_dump(pbuffer, templn, param);
					}
					else
					if(islike(param, "/peek.lv") || islike(param, "/poke.lv") || islike(param, "/find.lv"))
					{
						// /peek.lv1?<address>
						// /poke.lv1?<address>=<value>
						// /find.lv1?<value>
						// /find.lv1?<start-address>=<value>
						// /peek.lv2?<address>
						// /poke.lv2?<address>=<value>
						// /find.lv2?<value>
						// /find.lv2?<start-address>=<value>

						ps3mapi_find_peek_poke(pbuffer, templn, param);
					}
 #endif
					else
					if(mount_ps3)
					{
						// /mount_ps3/<path>[?random=<x>[&emu={ps1_netemu.self/ps1_netemu.self}][offline={0/1}]

						struct timeval tv;
						tv.tv_sec = 3;
						setsockopt(conn_s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

						if(!mc) keep_alive = http_response(conn_s, header, param, CODE_CLOSE_BROWSER, HTML_CLOSE_BROWSER); //auto-close browser (don't wait for mount)

						if(IS_ON_XMB && !(webman_config->combo2 & PLAY_DISC) && (strstr(param, ".ntfs[BD") == NULL) && (strstr(param, "/PSPISO") == NULL) && (strstr(param, ".ntfs[PSPISO]") == NULL))
						{
							sys_ppu_thread_sleep(1);
							int view = View_Find("explore_plugin");

							if(view)
							{
								explore_interface = (explore_plugin_interface *)plugin_GetInterface(view, 1);
 #ifdef PKG_LAUNCHER
								if(webman_config->ps3l && strstr(param, "/GAMEI/"))
									focus_first_item();
								else
 #endif
								if(webman_config->ps2l && !extcmp(param, ".BIN.ENC", 8))
									focus_first_item();
								else
									explore_close_all(param);
							}
						}

						if(sysmem) {sys_memory_free(sysmem); sysmem = NULL;}

						if(game_mount(pbuffer, templn, param, tempstr, mount_ps3, forced_mount)) ap_param = 1; // use webman_config->autoplay

						is_busy = false; keep_alive = 0;

						goto exit_handleclient_www;
					}
					else
 #ifdef PS2_DISC
					if(forced_mount || islike(param, "/mount.ps3") || islike(param, "/mount.ps2") || islike(param, "/mount_ps2") || islike(param, "/copy.ps3"))
 #else
					if(forced_mount || islike(param, "/mount.ps3") || islike(param, "/copy.ps3"))
 #endif
					{
						// /mount.ps3/<path>[?random=<x>[&emu={ps1_netemu.self/ps1_netemu.self}][offline={0/1}]
						// /mount.ps3/unmount
						// /mount.ps2/<path>[?random=<x>]
						// /mount.ps2/unmount
						// /mount.ps3/<dev_path>&name=<device-name>&fs=<file-system>
						// /mount.ps3/unmount<dev_path>
						// /copy.ps3/<path>[&to=<destination>]
						// /copy.ps3/<path>[&to=<destination>]?restart.ps3

						keep_alive = 0;

						if(islike(param, "/mount.ps3/unmount"))
						{
							is_mounting = false;

							char *dev_name = (param + 18); // /mount.ps3/unmount<dev_path>
							if(*dev_name == '/')
							{
								if(isDir(dev_name)) {system_call_3(SC_FS_UMOUNT, (uint32_t)dev_name, 0, 1);}
								sprintf(param, "/"); is_binary = FOLDER_LISTING; is_busy = false;
								goto html_response;
							}
						}
						else if(!islike(param + 10, "/net"))
						{
							strcpy(templn, param);

							// /mount.ps3/<dev_path>&name=<device-name>&fs=<file-system>
							char *dev_name = (templn + 10), *sys_dev_name = NULL, *fs = (char*)"CELL_FS_FAT";
							char *pos1 = strstr(dev_name, "&name="), *pos2 = strstr(dev_name, "&fs=");
							if(pos1) {*pos1 = 0, sys_dev_name = (pos1 + 6);}
							if(pos2) {*pos2 = 0, fs = (pos2 + 4);}

							if(not_exists(dev_name) || sys_dev_name)
							{
								mount_device(dev_name, sys_dev_name, fs);

								if(islike(param, "/copy.ps3")) ;

								else if(isDir(dev_name))
								{
									strcpy(param, dev_name); is_binary = FOLDER_LISTING; is_busy = false;
									goto html_response;
								}
								else
								{
									keep_alive = http_response(conn_s, header, param, CODE_PATH_NOT_FOUND, "404 Not found"); is_busy = false;
									goto exit_handleclient_www;
								}
							}
						}

						game_mount(pbuffer, templn, param, tempstr, mount_ps3, forced_mount);

						is_busy = false;
					}

					else
					{
						// /index.ps3                  show game list in HTML (refresh if cache file is not found)
						// /index.ps3?html             refresh game list in HTML
						// /index.ps3?launchpad        refresh game list in LaunchPad xml
						// /index.ps3?mobile           show game list in coverflow mode
						// /index.ps3?<query>          search game by device name, path or name of game
						// /index.ps3?<device>?<name>  search game by device name and name
						// /index.ps3?<query>&mobile   search game by device name, path or name of game in coverflow mode
						// /index.ps3?cover=<mode>     refresh game list in HTML using cover type (icon0, mm, disc, online)

						mobile_mode |= ((strstr(param, "?mob") != NULL) || (strstr(param, "&mob") != NULL));
#ifdef LAUNCHPAD
						char *launchpad = strstr(param, "?launchpad");
						if(launchpad) {*launchpad = NULL; mobile_mode = LAUNCHPAD_MODE, auto_mount = false; sprintf(templn, "%s LaunchPad: %s", STR_REFRESH, STR_SCAN2); show_msg(templn);}
#endif
						if(game_listing(buffer, templn, param, tempstr, mobile_mode, auto_mount) == false)
						{
							{ PS3MAPI_RESTORE_SC8_DISABLE_STATUS }
							{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }

							keep_alive = http_response(conn_s, header, param, CODE_SERVER_BUSY, STR_ERROR);

							is_busy = false;

							goto exit_handleclient_www;
						}

						if(auto_mount && islike(buffer, "/mount.ps3")) {auto_mount = false; sprintf(param, "%s", buffer); goto redirect_url;}

						if(is_ps3_http)
						{
							char *pos = strstr(pbuffer, "&#x1F50D;"); // hide search icon
							if(pos) for(u8 c = 0; c < 9; c++) pos[c] = ' ';
						}
					}

					{ PS3MAPI_RESTORE_SC8_DISABLE_STATUS }
					{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }

					is_busy = false;
#ifdef LAUNCHPAD
					if(mobile_mode == LAUNCHPAD_MODE) {sprintf(templn, "%s LaunchPad: OK", STR_REFRESH); if(!mc) keep_alive = http_response(conn_s, header, param, CODE_HTTP_OK, templn); if(!(webman_config->minfo & 1)) show_msg(templn); goto exit_handleclient_www;}
#endif
				}

send_response:
				if(mobile_mode && allow_retry_response) {allow_retry_response = false; *buffer = NULL; sprintf(param, "/games.ps3"); goto mobile_response;}

				if(islike(param, "/mount.ps3?http"))
					{keep_alive = http_response(conn_s, header, param, CODE_HTTP_OK, param + 11); break;}
				else
				{
					// add bdvd & go to top links to the footer
					sprintf(templn, "<div style=\"position:fixed;right:20px;bottom:10px;opacity:0.2\">"); _concat(&sbuffer, templn);
					if(isDir("/dev_bdvd")) {sprintf(templn, "<a href=\"%s\"><img src=\"%s\" height=\"12\"></a> ", "/dev_bdvd", wm_icons[iPS3]); _concat(&sbuffer, templn);}
					_concat(&sbuffer, "<a href=\"#Top\">&#9650;</a></div><b>");

	#ifndef EMBED_JS
					// extend web content using custom javascript
					if(common_js_exists)
					{
						sprintf(templn, SCRIPT_SRC_FMT, COMMON_SCRIPT_JS); _concat(&sbuffer, templn);
					}
	#endif
					_concat(&sbuffer, HTML_BODY_END); //end-html
				}

				if(!mc)
				{
					c_len = sbuffer.size;

					header_len += sprintf(header + header_len, "Content-Length: %llu\r\n\r\n", (unsigned long long)c_len);
					send(conn_s, header, header_len, 0);

					send(conn_s, buffer, c_len, 0);
				}

				*buffer = NULL;
			}
		}
		else if(loading_html && (*header == 0) && (++retry < 5))
		{
			served = 0; // data no received
			continue;
		}
		else
		#ifdef WM_REQUEST
		if(!wm_request)
		#endif
			keep_alive = http_response(conn_s, header, param, CODE_BAD_REQUEST, STR_ERROR);

		break;
	}
  }

exit_handleclient_www:

	if(sysmem) {sys_memory_free(sysmem); sysmem = NULL;}

	if(ap_param)
		{auto_play(param, --ap_param); ap_param = 0;} // ap_param: 1=from /mount.ps3, 2=from /play.ps3

	if(mc || (keep_alive && loading_html && (++max_cc < MAX_WWW_CC))) goto parse_request;

	#ifdef USE_DEBUG
	ssend(debug_s, "Request served.\r\n");
	#endif

	sclose(&conn_s);

	if(loading_html) loading_html--;

	sys_ppu_thread_exit(0);
}
