#define MAX_WWW_CC		(255)

#define CODE_HTTP_OK            200
#define CODE_BLANK_PAGE         204
#define CODE_BAD_REQUEST        400
#define CODE_FORBIDDEN          403
#define CODE_PATH_NOT_FOUND     404
#define CODE_NOT_IMPLEMENTED    501
#define CODE_SERVER_BUSY        503
#define CODE_VIRTUALPAD        1200
#define CODE_INSTALL_PKG       1201
#define CODE_DOWNLOAD_FILE     1202
#define CODE_RETURN_TO_ROOT    1203
#define CODE_HTTP_NOCSS        1210
#define CODE_BREADCRUMB_TRAIL  1220
#define CODE_GOBACK            1222
#define CODE_CLOSE_BROWSER     1223
#define CODE_PLAIN_TEXT        1224
#define CODE_PREVIEW_FILE      1225
#define CODE_JSON_RESPONSE     1226

////////////////////////////////
static bool css_exists = false;
static bool common_js_exists = false;

#ifdef SYS_ADMIN_MODE
static u8 check_password(char *param)
{
	u8 ret = 0;

	if((pwd_tries < 3) && webman_config->ftp_password[0])
	{
		char *pos = strstr(param, "pwd=");
		if(pos)
		{
			pwd_tries++;
			char pwd[24]; _memset(pwd, sizeof(pwd)); memcpy(pwd, webman_config->ftp_password, 20);
			if(IS(pos + 4, pwd)) {pwd_tries = 0, ret = 1;}
			--pos; *pos = NULL;
		}
	}

	return ret;
}
#endif

static int http_response(int conn_s, char *header, const char *url, int code, const char *msg)
{
	if(conn_s == (int)WM_FILE_REQUEST) return 0;

	u32 slen, done = true;

	if(code == CODE_VIRTUALPAD || code == CODE_GOBACK || code == CODE_CLOSE_BROWSER)
	{
		slen = sprintf(header,  HTML_RESPONSE_FMT,
								CODE_HTTP_OK, url, HTML_BODY, HTML_RESPONSE_TITLE, msg);
	}
	else if(code == CODE_PLAIN_TEXT)
	{
		slen = snprintf(header, HTML_RECV_SIZE, HTML_RESPONSE_FMT,
								CODE_HTTP_OK, url, msg, "", "");
	}
	#ifdef PS3MAPI
	else if(code == CODE_JSON_RESPONSE)
	{
		slen = sprintf(header,  HTML_RESPONSE_FMT,
								CODE_HTTP_OK, url, msg, "", "");
	}
	#endif
	#ifdef VIEW_PARAM_SFO
	else if(code == CODE_HTTP_NOCSS)
	{
		header = (char*)msg;
		slen = strlen(msg);
	}
	#endif
	else
	{
		char body[_2KB_];
		char *filename = (char*)msg;
		char *label = header; // used by CODE_PREVIEW_FILE

		if(code != CODE_PREVIEW_FILE) *header = NULL;

		#ifdef PATCH_ROS
		done = IS(url, "/ros.ps3") ? false : true;
		#endif

		if(*filename == '/')
		{
			const char *cmd = filename + 1;
			sprintf(body, "%s : OK", cmd);
			if(code == CODE_BREADCRUMB_TRAIL || code == CODE_PREVIEW_FILE)
			{
				const char *p = strchr(filename, '/');
				if(p) {body[p - filename] = '\0'; add_breadcrumb_trail2(body, label, p);}
			}
			else if(silent_mode) ;
			else if(!(webman_config->minfo & 2)) show_msg(body);

			if(code == CODE_PREVIEW_FILE)
			{
				const char *p = strrchr(filename, '.'); //if(islike(filename, "/dev_hdd0/tmp/gameboot")) p = strrchr(filename, '/');
				if(p) get_image_file(filename, p - filename); else strcat(filename, "/PS3_GAME/ICON0.PNG");
				if(file_exists(filename))
				{
					char *next = strstr(html_base_path, "?next");
					if(next)
					{
						add_url(body, "<a href=\"", html_base_path, "\">");
					}
					add_url(body, "<hr><center><img src=\"", filename, "\" height=\"50%\" border=0>");
					if(next)
					{
						strcpy(next, "?prev"); // change url to ?prev
						add_url(body, "<a href=\"", html_base_path, "\">"); *html_base_path = NULL;
					}
				}
			}
		}
		else if(islike(filename, "http"))
			sprintf(body, "<a style=\"%s\" href=\"%s\">%s</a>", HTML_URL_STYLE, filename, filename);
#ifdef PKG_HANDLER
		else if(code == CODE_BLANK_PAGE)
		{
			slen = sprintf(header, "<body bgcolor=\"#000\">");
			goto exit_http_response;
		}
		else if(code == CODE_INSTALL_PKG || code == CODE_DOWNLOAD_FILE)
		{
			sprintf(body, "<style>a{%s}</style>%s", HTML_URL_STYLE, (code == CODE_INSTALL_PKG) ? "Installing " : "");
			char *p = strchr(filename, '\n');
			if(p)
			{
				*p = NULL;
				if(code == CODE_INSTALL_PKG) add_breadcrumb_trail(body, filename + 11); else strcat(body, filename);
				if(code == CODE_DOWNLOAD_FILE || !is_ext(pkg_path, ".p3t")) add_breadcrumb_trail2(body, "<p>To:", p + 5);
			}
			else
				strcat(body, filename);

			code = CODE_HTTP_OK;
		}
#endif
		else
			strcopy(body, msg);

		if(code == CODE_PATH_NOT_FOUND)
		{
			add_breadcrumb_trail2(body, "<p>", url + (islike(url, "/mount") ? MOUNT_CMD : 0));
		}

		//if(ISDIGIT(*msg) && ( (code == CODE_SERVER_BUSY || code == CODE_BAD_REQUEST) )) show_msg(body + 4);

		if(css_exists)
		{
			sprintf(header, "<LINK href=\"%s\" rel=\"stylesheet\" type=\"text/css\">", COMMON_CSS); concat(body, header);
		}
		if(common_js_exists)
		{
			sprintf(header, SCRIPT_SRC_FMT, COMMON_SCRIPT_JS); concat(body, header);
		}

		#ifdef PATCH_ROS
		if(done)
		{
			sprintf(header, "<hr>" HTML_BUTTON_FMT "%s",
							HTML_BUTTON, " &#9664;  ", HTML_ONCLICK, ((code == CODE_RETURN_TO_ROOT) ? "/" : "javascript:history.back();"), HTML_BODY_END); concat(body, header);
		}
		else
			concat(body, "<hr></font>");
		#else
			sprintf(header, "<hr>" HTML_BUTTON_FMT "%s",
							HTML_BUTTON, " &#9664;  ", HTML_ONCLICK, ((code == CODE_RETURN_TO_ROOT) ? "/" : "javascript:history.back();"), HTML_BODY_END); concat(body, header);
		#endif

		slen = sprintf(header,  HTML_RESPONSE_FMT,
								(code == CODE_RETURN_TO_ROOT) ? CODE_HTTP_OK : code, url, HTML_BODY, HTML_RESPONSE_TITLE, body);
	}

#ifdef PKG_HANDLER
exit_http_response:
#endif

	send(conn_s, header, slen, 0);
	if(done)
	{
		sclose(&conn_s);
		if(loading_html) loading_html--;
	}
	return 0;
}

static size_t prepare_html(char *buffer, char *html, char *param, u8 is_ps3_http, u8 is_cpursx, bool mount_ps3)
{
	t_string sbuffer; _alloc(&sbuffer, buffer);

	if((webman_config->sman || strstr(param, "/sman.ps3")) && file_exists(HTML_BASE_PATH "/sman.htm"))
	{
		sbuffer.size = read_file(HTML_BASE_PATH "/sman.htm", sbuffer.str, _16KB_, 0);

		if(is_cpursx)
			_concat2(&sbuffer, HTML_REFRESH, "15;URL=/cpursx.ps3?/sman.ps3\">");

		// add javascript
		{
			sprintf(html, "<noscript><h1 style='color:#f03'>%s Javascript %s</h1></noscript>", STR_ERROR, STR_DISABLED); _concat(&sbuffer, html);
			#ifndef ENGLISH_ONLY
			if(webman_config->lang)
			{
				_concat(&sbuffer, "<script>");
				_concat(&sbuffer, strfmt("l('%s','%s');", "games",		STR_GAMES));
				_concat(&sbuffer, strfmt("l('%s','%s');", "files",		STR_FILES));
				_concat(&sbuffer, strfmt("l('%s','%s');", "setup",		STR_SETUP));
				_concat(&sbuffer, strfmt("l('%s','%s');", "eject",		STR_EJECT));
				_concat(&sbuffer, strfmt("l('%s','%s');", "insert",		STR_INSERT));
				_concat(&sbuffer, strfmt("l('%s','%s');", "refresh",	STR_REFRESH));
				_concat(&sbuffer, strfmt("l('%s','%s');", "restart",	STR_RESTART));
				_concat(&sbuffer, strfmt("l('%s','%s');", "shutdown",	STR_SHUTDOWN));
				_concat(&sbuffer, strfmt("l('msg2','%s ...');"
								"</script>",							STR_MYGAMES));
			}
			#endif

			#ifdef LITE_EDITION
			_concat(&sbuffer, "<script>document.getElementsByClassName('b_ps3mapi')[0].style.display='none';</script>");
			#endif

			if(webman_config->homeb && islike(webman_config->home_url, "http"))
			{
				sprintf(html, "<script>hurl=\"%s\";</script>", webman_config->home_url);  _concat(&sbuffer, html);
			}
		}

		if(param[1] && !strstr(param, ".ps3")) {urlenc(html, param); _concat3(&sbuffer, "<base href=\"", html, "/\">");}

		return sbuffer.size;
	}

	sbuffer.size = sprintf(sbuffer.str, HTML_HEADER);

	if(is_cpursx)
		_concat2(&sbuffer, HTML_REFRESH, "10;URL=/cpursx.ps3\">");

	if(mount_ps3) {_concat(&sbuffer, HTML_BODY); return sbuffer.size;}

	_concat(&sbuffer,
						"<head><title>" WEBMAN_MOD "</title>"
//						"<style>"
//						"a{" HTML_URL_STYLE "}"
//						"#rxml,#rhtm,#rcpy,#wmsg{position:fixed;top:40%;left:30%;width:40%;height:90px;z-index:5;border:5px solid #ccc;border-radius:25px;padding:10px;color:#fff;text-align:center;background-image:-webkit-gradient(linear,0 0,0 100%,color-stop(0,#999),color-stop(0.02,#666),color-stop(1,#222));background-image:-moz-linear-gradient(top,#999,#666 2%,#222);display:none;}"
//						"</style>" // fallback style if external css fails
	);

	if(param[1] == 0)
	{
		// minimize times that these files are checked (at startup & root)
		css_exists = css_exists || file_exists(COMMON_CSS);
		common_js_exists = common_js_exists || file_exists(COMMON_SCRIPT_JS);
	}
	if(css_exists)
	{
		sprintf(html, "<LINK href=\"%s\" rel=\"stylesheet\" type=\"text/css\">", COMMON_CSS); _concat(&sbuffer, html);
	}

	if(param[1] && !strstr(param, ".ps3")) {_concat(&sbuffer, "<base href=\""); urlenc(html, param); strcat(html, "/\">"); _concat(&sbuffer, html);}

	if(is_ps3_http == 1)
		{sprintf(html, "<style>%s</style>", ".gi{height:210px;width:267px"); _concat(&sbuffer, html);}

	_concat2(&sbuffer, "</head>", HTML_BODY);

	const char *coverflow = NULL; if(file_exists(MOBILE_HTML)) coverflow = (char*)" [<a href=\"/games.ps3\">Coverflow</a>]";

	size_t tlen = sprintf(html, "<b>" WM_APP_VERSION "<br>"
								"<font style=\"font-size:18px\">[<a href=\"/\">%s</a>] [<a href=\"%s\">%s</a>]%s", STR_FILES,
								(webman_config->sman && file_exists(HTML_BASE_PATH "/sman.htm")) ? "/sman.ps3" : "/index.ps3", STR_GAMES, coverflow);

#ifdef SYS_ADMIN_MODE
	if(sys_admin)
	{
#endif
#ifdef PS3MAPI
	#ifdef WEB_CHAT
		sprintf(html + tlen, " [<a href=\"/chat.ps3\">Chat</a>] [<a href=\"/home.ps3mapi\">PS3MAPI</a>] [<a href=\"/setup.ps3\">%s</a>]</b>", STR_SETUP);
	#else
		sprintf(html + tlen, " [<a href=\"/home.ps3mapi\">PS3MAPI</a>] [<a href=\"/setup.ps3\">%s</a>]</b>", STR_SETUP);
	#endif
#else
	#ifdef WEB_CHAT
		sprintf(html + tlen, " [<a href=\"/chat.ps3\">Chat</a>] [<a href=\"/setup.ps3\">%s</a>]</b>", STR_SETUP);
	#else
		sprintf(html + tlen, " [<a href=\"/setup.ps3\">%s</a>]</b>", STR_SETUP );
	#endif
#endif
#ifdef SYS_ADMIN_MODE
	}
	else
		strcat(html + tlen, "</b>");
#endif

	_concat(&sbuffer, html);
	return sbuffer.size;
}

static void do_web_command(u64 conn_s_p, const char *wm_url)
{
	int conn_s = (int)conn_s_p;

	bool is_ntfs = false;
	bool is_local = true;
	size_t header_len;
	sys_net_sockinfo_t conn_info;
	sys_addr_t sysmem = NULL;

	u8 max_cc = 0; // count client connections per persistent connection
	u8 keep_alive = 0, use_keep_alive = 0;
	u8 ap_param = 0; force_ap = 0; // 0 = do nothing, 1 = use webman_config->autoplay, 2 = force auto_play

	fast_concat.str = NULL; silent_mode = false;

	char cmd[16], header[HTML_RECV_SIZE];
	char *mc = NULL, *mc_param = NULL;

	char param[HTML_RECV_SIZE];
	char *file_query = param + HTML_RECV_LAST; *file_query = NULL;

 #ifdef WM_REQUEST
	struct CellFsStat buf; u8 wm_request = wm_url || (cellFsStat(WM_REQUEST_FILE, &buf) == CELL_FS_SUCCEEDED);

	if(!wm_request)
 #endif
	{
		sys_net_get_sockinfo(conn_s, &conn_info, 1);

		char *remote_ip = cmd;
		strcopy(remote_ip, inet_ntoa(conn_info.remote_adr));

		// check remote access
		if(webman_config->bind && is_remote_ip && !islike(remote_ip, webman_config->allow_ip))
		{
			keep_alive = http_response(conn_s, header, param, CODE_FORBIDDEN, "403 Forbidden");

			goto exit_handleclient_www;
		}

		is_local = (conn_info.local_adr.s_addr == conn_info.remote_adr.s_addr);

		if(!webman_config->netd[0] && !webman_config->neth[0][0]) strcpy(webman_config->neth[0], remote_ip); // show client IP if /net0 is empty
		if(!webman_config->bind) strcpy(webman_config->allow_ip, remote_ip); // default allowed remote ip
	}

/*
	// check available free memory
	{
		_meminfo meminfo;
		u8 retries = 0;

again3:
		get_meminfo();

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
			if(wm_request) cellFsUnlink(WM_REQUEST_FILE);
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
	u8 retry, served, is_binary;	// served http request?, is_binary: 0 = http command, 1 = file, 2 = folder listing
	s8 sort_order, sort_by;
	u64 c_len;

	u8 is_cpursx;
	u8 is_popup, auto_mount;
	u8 is_ps3_http;

	#ifdef USE_NTFS
	skip_prepntfs = false;
	#endif

	retry = served = 0; is_binary = WEB_COMMAND;
	is_ps3_http  = auto_mount = is_popup = is_cpursx = 0; c_len = 0;
	sort_order = 1, sort_by = 0;

//// process commands ////

	while(!served && working)
	{
		served++;
		_memset(header, HTML_RECV_SIZE);
		keep_alive = 0;

		if(!mc)
		{
			#ifdef USE_DEBUG
			ssend(debug_s, "ACC - ");
			#endif

			#ifdef WM_REQUEST
			if(wm_request)
			{
				// Set the content of WM_REQUEST_FILE as header
				if(wm_url || (buf.st_size > 5 && buf.st_size < HTML_RECV_SIZE && read_file(WM_REQUEST_FILE, header, buf.st_size, 0) > 4))
				{
					if(wm_url)
					{
						is_busy = false;

						// Add GET verb
						if(*wm_url == '/')
							buf.st_size = snprintf(header, HTML_RECV_SIZE, "GET %s", wm_url);
						else
							buf.st_size = strncopy(header, HTML_RECV_SIZE, wm_url);
					}

					replace_char(header, '\t', ' ');

					#ifdef PHOTO_GUI
					#define photo_path	header
					///// Process PhotoGUI request /////
					if(!(webman_config->launchpad_xml) && islike(photo_path, "/dev_hdd0/photo/"))
					{
						char *filename = get_filename(photo_path); if(filename) filename++;

						play_rco_sound("snd_trophy");

						#ifdef MOUNT_PNG
						if(file_size(photo_path) >= _2MB_)
						{
							bool is_ISO = (read_file(photo_path, param, 5, 0x18801) == 5) && IS(param, "CD001"); // BD/DVD
							if(!IS(param, "CD001"))
								 is_ISO = (read_file(photo_path, param, 5, 0x19319) == 5) && IS(param, "CD001"); // CD

							if(is_ISO)
							{
								// show filename
								if(!(webman_config->minfo & 1)) show_msg(filename);

								// add /mount_ps3 prefix
								strcpy(param, photo_path);
								concat2(photo_path, "/mount_ps3", param);
							}
						}
						else
						#endif // #ifdef MOUNT_PNG
						if(filename)
						{
							char *pos1 = strcasestr(filename, ".jpg"); if(pos1) *pos1 = '\0';
							char *pos2 = strcasestr(filename, ".png"); if(pos2) *pos2 = '\0';

							// Find filename in SLAUNCH_FILE
							find_slaunch_game(filename, 0);

							// show filename
							if(!(webman_config->minfo & 1)) show_msg(filename);
							init_delay = -5; // prevent show Not in XMB message

							// replace header with found full path
							strcpy(photo_path, filename);
						}
					}
					///// End PhotoGUI request /////
					#endif // #ifdef PHOTO_GUI

					// Add GET verb
					if(*header == '/') {strcpy(param, header); buf.st_size = snprintf(header, HTML_RECV_SIZE, "GET %s", param);}

					// make proper URL replacing spaces with +
					replace_char(header + 4, ' ', '+');

					// Retry for 30 seconds /play.ps3 command while IS_INGAME
					if(islike(header + 4, "/play.ps3")) {if(IS_INGAME && (++retry < 30)) {sys_ppu_thread_sleep(1); served = 0; is_ps3_http = 1; continue;}}
				}

				// Delete WM_REQUEST_FILE
				cellFsUnlink(WM_REQUEST_FILE);
			}
			#endif // #ifdef WM_REQUEST
		}
		else if(mc_param)
		{
			concat_text(header, "GET", mc_param); // next multi-command param
			free(mc_param); mc_param = NULL;
		}

		mc = NULL;

		if((*header == 'G') || ((recv(conn_s, header, HTML_RECV_SIZE, 0) > 0) && (*header == 'G') && (header[4] == '/'))) // serve only GET /xxx requests
		{
			if(strstr(header, "Connection: keep-alive"))
			{
				use_keep_alive = keep_alive = 1;
			}

			if(strstr(header, "x-ps3-browser")) is_ps3_http = 1; else
			if(strstr(header, "Gecko/36"))  	is_ps3_http = 2; else
												is_ps3_http = 0;

			for(size_t n = 0; header[n]; n++) {if(header[n] == '\r' || header[n] == '\n' || n >= HTML_RECV_SIZE) {header[n] = 0; break;}}

			ssplit(header, cmd, 15, header, HTML_RECV_LAST);
			ssplit(header, param, HTML_RECV_LAST, cmd, 15);

			char *param_original = header; // used in /download.ps3

			#include "../cmd/refresh_ps3.h"

			bool is_setup = islike(param, "/setup.ps3?");

			if(!is_setup)
			{
				mc = strstr(param, ";/");
				if(mc)
				{
					mc_param = malloc(strlen(mc));
					if(mc_param) strcpy(mc_param, mc + 1); // store next multi-command param
					*mc = NULL; strcpy(header, param); // process first command
					silent_mode = true;
				}
			}

			bool allow_retry_response = true; u8 mobile_mode = false;

			#ifdef USE_DEBUG
			ssend(debug_s, param);
			ssend(debug_s, "\r\n");
			#endif

			#include "www_admin.h" // security check

			#ifdef VIRTUAL_PAD
			bool is_pad = islike(param, "/pad.ps3");

			if(!is_pad)
			#endif
			{
	redirect_url:
				urldec(param, param_original);

				if(islike(param, "/setup.ps3")) goto html_response;
			}

			#include "../cmd/toggle_fps.h"
			#include "../cmd/pad_combo_play.h"
			#include "../cmd/gpuclock_memclock.h"
			#include "../cmd/cpursx_tempc.h"
			#include "../cmd/admin.h"

			if(get_flag(param, "?restart.ps3")) do_restart = sys_admin;

			#include "../cmd/dev_blind.h"
			#include "../cmd/download.h"
			#include "../cmd/install.h"
			#include "../cmd/reloadxmb.h"
			#include "../cmd/xmb_browser.h"
			#include "../cmd/games.h"
			#include "../cmd/popup.h"
			#include "../cmd/restart.h"
			#include "../cmd/shutdown.h"
			#include "../cmd/quit.h"

#ifndef LITE_EDITION
			#include "../cmd/abort.h"
			#include "../cmd/wait.h"
			#include "../cmd/edit.h"
			#include "../cmd/artemis.h"
			#include "../cmd/view.h"
			#include "../cmd/chat.h"
			#include "../cmd/mount_search.h"
			if(sys_admin)
			{
				#include "../cmd/remap_unmap.h"
				#include "../cmd/rename_swap_move.h"
				#include "../cmd/write_trunc_unzip.h"
				#include "../cmd/cpy_cut_paste.h"
				#include "../cmd/mkdir_rmdir.h"
				#include "../cmd/randomizers.h" // /gameboot.ps3, /wallpaper.ps3, etc.
				#include "../cmd/stat_chmod.h"
				#include "../cmd/md5.h"
				#include "../cmd/minver.h"
				#include "../cmd/ros.h"
				#include "../cmd/recovery.h"
				#include "../cmd/rebuild.h"
				#include "../cmd/netstatus.h"
				#include "../cmd/syscall.h"
				#include "../cmd/consoleid.h"
				#include "../cmd/secureid.h"
				#include "../cmd/klic.h"
				#include "../cmd/fixgame.h"
				#include "../cmd/unlockhdd.h"
				#include "../cmd/unlocksave.h"
				#include "../cmd/bdinfo.h"
				#include "../cmd/err.h"
			}
			#ifdef COPY_PS3
			if(islike(param, "/copy")) {dont_copy_same_size = (param[5] == '.'); param[5] = '.';} //copy_ps3 -> force copy files of the same file size
			else
			#endif // #ifdef COPY_PS3
#endif // #ifndef LITE_EDITION

			#include "www_isadmin.h" // check is admin / is_busy
			{
 html_listing:
				#include "www_listing.h" // sort / search / error / check if is binary
			}

 html_response:
			header_len = prepare_header(header, param, is_binary);

			#include "www_binary.h"

			char templn[1024]; char *html = templn;
			{u16 ulen = strlen(param); if((ulen > 1) && (param[ulen - 1] == '/')) ulen--, param[ulen] = '\0';}
			//sprintf(templn, "X-PS3-Info: %llu [%s]\r\n", (unsigned long long)c_len, param); strcat(header, templn);

			#include "www_profile.h"
			#include "www_memory.h"

			//else	// text page
			{
				if((is_binary == WEB_COMMAND) && is_setup)
				{
					setup_parse_settings(param + 11);
				}

				bool mount_ps3 = !is_popup && islike(param, "/mount_ps3"), forced_mount = false;

				if(is_mounting) {is_mounting = false; do_umount(false);}

				if(mount_ps3 && IS_INGAME) {mount_ps3 = false, forced_mount = true, param[6] = '.';}

				if(conn_s_p != WM_FILE_REQUEST)
					sbuffer.size = prepare_html(buffer, html, param, is_ps3_http, is_cpursx, mount_ps3);

				char *pbuffer = buffer + sbuffer.size;
				char *tempstr = buffer + BUFFER_SIZE_HTML - _4KB_;

				if(is_cpursx)
				{
					// /cpursx.ps3?fan=<n> 0=SYSCON, 1=DYNAMIC, 2=AUTO#2, n>=20 = % fan speed
					// /cpursx.ps3?mode=<syscon|manual|dynamic|auto>
					// /cpursx.ps3?mode <- Toggle between dynamic/manual
					// /cpursx.ps3?max=<temp> <- Target temperature for dynamic fan control
					// /cpursx.ps3?up
					// /cpursx.ps3?dn

					cpu_rsx_stats(pbuffer, html, param, is_ps3_http);

					loading_html = keep_alive = is_cpursx = 0; goto send_response;

					//CellGcmConfig config; cellGcmGetConfiguration(&config);
					//sprintf(templn, "localAddr: %x", (u32) config.localAddress); _concat(&sbuffer, templn);
				}
				else if((conn_s_p == WM_FILE_REQUEST)) ;
				else if(webman_config->sman || strstr(param, "/sman.ps3"))
				{
					_concat(&sbuffer, "<div id='toolbox'>");
					if(islike(param, "/dev_")) goto skip_code1; else goto skip_code2;
				}
				else if(!mount_ps3)
				{
					#include "www_page.h"
				}

#ifdef LITE_EDITION
continue_rendering:
#endif
				#include "../cmd/edit_chat_popup.h"

				////////////////////////////////////

				fast_concat.str = NULL;

				if(is_binary == FOLDER_LISTING) // folder listing
				{
					if(folder_listing(buffer, BUFFER_SIZE_HTML, html, param, conn_s, tempstr, header, is_ps3_http, sort_by, sort_order, file_query) == false)
					{
						goto exit_handleclient_www;
					}
				}
				else // process web command
				{
					{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }
					#ifndef LITE_EDITION
					if(!get_flag(param, "$nobypass")) { PS3MAPI_REENABLE_SYSCALL8 }
					#endif
					is_busy = true;

					#include "../cmd/refresh.h"
					#include "../cmd/setup.h"
					#include "../cmd/insert_eject.h"
					#include "../cmd/delete.h"
#ifndef LITE_EDITION
					#include "../cmd/ps3mapi.h"
					#include "../cmd/install_list.h"
					#include "../cmd/videorec.h"
					#include "../cmd/sysbgm.h"
					#include "../cmd/extgd.h"
					#include "../cmd/nobd.h"
					#include "../cmd/debug_mem.h" // /hexview.ps3, /find.lv2, /peek.lv1, etc.
#endif
					#include "../cmd/loadprx_unloadprx.h"
					#include "../cmd/mount_copy.h"
					#include "../cmd/index.h"

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
					_concat(&sbuffer, "<div style=\"position:fixed;right:20px;bottom:10px;opacity:0.2\">");
					if(isDir("/dev_bdvd")) {sprintf(html, "<a href=\"%s\"><img src=\"%s\" height=\"12\"></a> ", "/dev_bdvd", wm_icons[iPS3]); _concat(&sbuffer, html);}
					_concat(&sbuffer, "<a href=\"#Top\">&#9650;</a></div><b>");

					// extend web content using custom javascript
					if(common_js_exists)
					{
						sprintf(html, SCRIPT_SRC_FMT, COMMON_SCRIPT_JS); _concat(&sbuffer, html);
					}

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
		else if(++retry < 250) // if(loading_html && (*header == 0) && (++retry < 5))
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
	goto exit_handleclient_www;
  }

#ifdef COBRA_ONLY
exit_nocobra_error:

	if(!mc) keep_alive = http_response(conn_s, header, param, CODE_FORBIDDEN, " Cobra payload not available.");
#endif

exit_handleclient_www:

	if(sysmem) {sys_memory_free(sysmem); sysmem = NULL;}

	if(ap_param)
		{auto_play(param, --ap_param); ap_param = 0;} // ap_param: 1=from /mount.ps3, 2=from /play.ps3

	if(mc || (use_keep_alive && keep_alive && loading_html && (++max_cc < MAX_WWW_CC))) goto parse_request;

	if(mc_param) free(mc_param);

	#ifdef USE_DEBUG
	ssend(debug_s, "Request served.\r\n");
	#endif
}

static void handleclient_www(u64 conn_s_p)
{
	int conn_s = (int)conn_s_p;

	#ifdef USE_DEBUG
	ssend(debug_s, "waiting...");
	#endif

	if(loading_html > 10) loading_html = 0;

	do_web_command(conn_s_p, NULL);

	if(loading_html) loading_html--;

	sclose(&conn_s);
	sys_ppu_thread_exit(0);
}
