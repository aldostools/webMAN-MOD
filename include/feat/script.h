// # remmark
// if/else if/else/end if
//   if exist <path>
//   if not exist <path>
//   if L1
//   if R1
//   if login/logout
//   if xmb/ingame
//   if count <value>
//   if copying/mounting
//   if cobra/nocobra/debug/mamba/ps3hen/dex/firmware x.xx/overclock
//   if titleid <titleid/pattern>
//   abort if exist <path>
//   abort if not exist <path>

// while <if-condition>/continue/break/loop (nested loops are not supported)

// :label
// goto label
// map <path>=<path>
// unmap <path>
// md <path>
// del <path>
// copy <path>=<path>
// fcopy <path>=<path>
// swap <path>=<path>
// ren <path>=<path>
// call <path>

// wait user
// wait xmb
// wait <1-9 secs>
// wait <path>  (timeout 5 seconds)
// lwait <path> (timeout 30 seconds)

// logfile <pah>
// log debug
// log <text>
// popup <text>
// beep1 / beep2 / beep3 / ... beep9
// mute coldboot

// <webman_cmd> e.g. /mount.ps3<path>

#define EVENT_BOOT_INIT	0
#define EVENT_AUTOEXEC	1
#define	EVENT_ON_XMB	2
#define EVENT_INGAME	3
#define EVENT_PER_GAME	4

#ifdef COPY_PS3

#if defined(WM_CUSTOM_COMBO) || defined(WM_REQUEST)

#define line	buffer	/* "line", "path" and "buffer" are synonyms */
#define path	buffer	/* "line", "path" and "buffer" are synonyms */
#define IS_WEB_COMMAND(line)	(islike(line, "/mount") || strstr(line, ".ps3") || strstr(line, "_ps3") || strstr(line, ".lv1?") || strstr(line, ".lv2?"))
#define EXIT_LOOP	{mloop = NULL, count = 0; pos = strcasestr(pos + 1, "loop"); if(!pos) break;} // exit loop
#define log_cmd(line)	if(sc_debug) save_file(log_file, line, APPEND_TEXT)

static void handle_file_request(const char *wm_url)
{
	if(wm_url || file_exists(WM_REQUEST_FILE))
	{
		do_web_command(WM_FILE_REQUEST, wm_url);
	}
}

static void parse_script(const char *script_file, bool check_running)
{
	if(check_running && script_running) return;

#ifdef FIX_CLOCK
	if(file_exists(script_file) || islike(script_file, "http://"))
#else
	if(file_exists(script_file))
#endif
	{
		u32 max_size = _64KB_;
		sys_addr_t sysmem  = sys_mem_allocate(max_size);
		if(!sysmem) return;

		char *buffer, *sep, *pos, *mloop = NULL, *dest = NULL, label[24]; u16 l, count = 0;
		u8 do_else = 0, enable_db = true, sc_debug = false; size_t buffer_size;
		char log_file[STD_PATH_LEN]; strcpy(log_file, SC_LOG_FILE); *label = NULL;

	reload_script:
		buffer = (char*)sysmem; buffer_size = read_file(script_file, buffer, max_size, 0); buffer[buffer_size] = 0;
		if(*buffer) strcat(buffer + buffer_size, "\n");

		l = 0, script_running = true;

		if(mloop) {buffer = mloop; mloop = NULL;}
		if(*label) {pos = strcasestr(buffer, label); if(pos) buffer = pos; else *buffer = NULL; *label = NULL;}

		while(*buffer)
		{
			if(++l > 9999 || !working) break;

			while( *buffer && (*buffer <= ' ')) buffer++; // skip blank chars \n \r \t

			if(*buffer == '\0') break;

			// process line
			pos = strchr(line, '\n'); if(!pos) pos = line;

			// process line separator ;
			if(BETWEEN('A', *line, 'z'))  {sep = strchr(line, ';'); if(sep && (sep < pos)) pos = sep;}

			if(pos)
			{
				*pos = NULL; //EOL
				replace_char(line, '\r', 0); // crlf

				log_cmd(line);

				//if(exec_mode)
				{
					if(IS_WEB_COMMAND(line))
						dest = NULL;
					else
					{
						dest = strstr(line, "=/");
						if(!dest) dest = strstr(line, ",/");
						if(!dest) dest = strstr(line, ", /");
					}
					if(enable_db && (strstr(line, "/dev_blind"))) {enable_dev_blind(NO_MSG); enable_db = false;}
				}

				if(dest)
				{
					*dest++ = NULL; while(*dest == ' ') dest++; //split parameters
					char *wildcard = strchr(line, '*');
	#ifdef COBRA_ONLY
					if(_islike(line, "map /"))  {line += 4;}
					if(*line == '/') {check_path_tags(path); if(IS_WEB_COMMAND(line)) handle_file_request(line); else if(IS(path, "/app_home")) set_app_home(path); else sys_map_path(path, dest);} else
	#else
					if(*line == '/') {check_path_tags(path); if(IS_WEB_COMMAND(line)) handle_file_request(line);} else
	#endif
					if(_islike(line, "ren /"))  {path += 4; check_path_tags(path); if(wildcard) {*wildcard++ = NULL;  scan(path, true, wildcard, SCAN_RENAME, dest);} else rename_file(path, dest);} else
					if(_islike(line, "copy /")) {path += 5; check_path_tags(path); if(wildcard) {*wildcard++ = NULL;  scan(path, true, wildcard, SCAN_COPY, dest);}  else if(isDir(path)) folder_copy(path, dest); else file_copy(path, dest);} else
					if(_islike(line, "fcopy /")){path += 6; check_path_tags(path); if(wildcard) {*wildcard++ = NULL;  scan(path, true, wildcard, SCAN_FCOPY, dest);} else if(isDir(path)) force_folder_copy(path, dest); else force_copy(path, dest);} else
					if(_islike(line, "swap /")) {path += 5; check_path_tags(path); strcpy(cp_path, path); char *slash = get_filename(cp_path); sprintf(slash, "/~swap"); rename_file(path, cp_path); rename_file(dest, path); rename_file(cp_path, dest);} else
					if(_islike(line, "move /")) {path += 5; check_path_tags(path); if(wildcard) {*wildcard++ = NULL;  scan(path, true, wildcard, SCAN_MOVE, dest);} else rename_file(path, dest);} else
					if(_islike(line, "list /")) {path += 5; check_path_tags(path); if(wildcard) {*wildcard++ = NULL;} scan(path, true, wildcard, SCAN_LIST, dest);} else
					if(_islike(line, "cpbk /")) {path += 5; check_path_tags(path); if(wildcard) {*wildcard++ = NULL;} scan(path, true, wildcard, SCAN_COPYBK, dest);}
				}
				else if(*line == '#' || *line == ':' || *line == ';' || *line == '*') ; // remark

				else if(do_else && _islike(line, "else"))
				{
					--do_else;

					// exit 'if' (true condition)
					for(char *i, *n;;)
					{
						buffer = pos + 1;
						n = strcasestr(buffer, "end if"); if(!n) break;
						i = strcasestr(buffer, "if ");
						
						// jump to 'end if'
						if((n[6] <= ' ') &&  (*(--n) <= ' '))
						{
							// skip 'end if' for nested if's (find next 'end if')
							if(i && (*(--i) <= ' ') && (i < n)) {pos = strchr(++n, '\n'); if(!pos) pos = n + 6; continue;}

							// go to end of line for 'end if'
							log_cmd("end if"); pos = strchr(++n, '\n'); if(!pos) pos = n + 6; break;
						}
						else
							pos = n + 6;
					}
				}
				else if(do_else && _islike(line, "end if"))
				{
					--do_else;
				}
				else //if(exec_mode)
				{
					if(*line == '/')               {if(islike(line, "/dev_blind?0")) disable_dev_blind(); else if(IS_WEB_COMMAND(line)) handle_file_request(line);} else
					if(_islike(line, "goto "))     {mloop = NULL, snprintf(label, 24, ":%s", line + 5); goto reload_script;} else
					if(_islike(line, "loop") || _islike(line, "continue")) goto reload_script; else
					if(_islike(line, "break"))     {EXIT_LOOP;} else
					if(_islike(line, "call "))     {path += 5, parse_script(path, false);} else
					if(_islike(line, "del /"))     {path += 4; check_path_tags(path); char *wildcard = strchr(path, '*'); if(wildcard) {*wildcard++ = NULL; scan(path, true, wildcard, SCAN_DELETE, NULL);} else del(path, RECURSIVE_DELETE);} else
					if(_islike(line, "md /"))      {path += 3; check_path_tags(path); mkdir_tree(path);} else
					if(_islike(line, "wait xmb"))  {wait_for_xmb();} else
					if(_islike(line, "wait user")) {wait_for_user();} else
					if(_islike(line, "wait /"))    {path += 5; check_path_tags(path); wait_for(path, 5);} else
					if(_islike(line, "wait "))     {line += 5; sys_ppu_thread_sleep((u8)val(line));} else
					if(_islike(line, "lwait /"))   {path += 6; check_path_tags(path); wait_for(path, 30);} else
					#ifdef PS3MAPI
					if(_islike(line, "beep"))      {play_sound_id((u8)(line[4]));} else
					#else
					if(_islike(line, "beep"))      {if(line[4] == '3') {BEEP3;} else if(line[4] == '2') {BEEP2;} else {BEEP1;}} else
					#endif
					if(_islike(line, "popup "))    {line += 6; parse_tags(line); show_msg(line);} else
					if(_islike(line, "log "))      {line += 4; parse_tags(line); save_file(log_file, line, APPEND_TEXT); sc_debug = _IS(line, "Debug");} else
					if(_islike(line, "logfile /")) {path += 8; check_path_tags(path); strcpy(log_file, path);} else
	#ifdef UNLOCK_SAVEDATA
					if(_islike(line, "unlock /"))  {path += 7; scan(path, true, "/PARAM.SFO", SCAN_UNLOCK_SAVE, NULL);} else
	#endif
	#ifdef COBRA_ONLY
					if(_islike(line, "unmap /"))   {path += 6; unmap_path(path);} else
					if(_islike(line, "mute coldboot"))
					{
						sys_map_path(VSH_RESOURCE_DIR "coldboot_stereo.ac3", SYSMAP_EMPTY_DIR);
						sys_map_path(VSH_RESOURCE_DIR "coldboot_multi.ac3", SYSMAP_EMPTY_DIR);
					}
					else
	#endif
					if(_islike(line, "if ") || _islike(line, "abort if ") || _islike(line, "while "))
					{
						#define DO_WHILE	6
						#define ABORT_IF	9

						u8 ifmode;
						if(_islike(line, "while ")) {ifmode = DO_WHILE, mloop = line;} else ifmode = _islike(line, "if ") ? 3 : ABORT_IF; 
						line += ifmode;

						bool ret = false, isnot = false;

						if(_islike(line, "not ")) {isnot = true, line += 4;}

						if(_islike(line, "exist /"))     {path +=  6; check_path_tags(path); ret = file_exists(path);} else
						if(_islike(line, "Firmware")) {line += 9; ret = IS(fw_version, line);} else
						if(_islike(line, "noCobra")) {ret = !cobra_version;} else
						if(_islike(line, "xmb"))	{ret = IS_ON_XMB;} else
						if(_islike(line, "ingame")) {ret = IS_INGAME;} else
						if(_islike(line, "count "))  {if(!count) {count = (u16)val(line + 6); ret = count;} else ret = --count;} else
						if(_islike(line, "copying")) {ret = copy_in_progress;} else
						if(_islike(line, "mounting")) {ret = is_mounting;} else
						if(_islike(line, "log")) {ret = USER_LOGGEDIN; if(strstr(line, "out")) ret = !ret;} else
						#if defined(PS3MAPI) || defined(DEBUG_MEM)
						if(_islike(line, "titleid ")){path += 8; get_game_info(); ret = (strlen(_game_TitleID) >= strlen(path)) ? !bcompare(_game_TitleID, path, strlen(path), path) : 0;} else
						#endif
						#ifdef VIRTUAL_PAD
						if(_islike(line, "singleUser"))   {ret = has_one_user_dir();} else
						#endif
						#ifdef COBRA_ONLY
						if(_islike(line, "Cobra"))   {ret = cobra_version;} else
						if(_islike(line, "Debug"))   {ret = isCobraDebug;} else
						if(_islike(line, "Mamba"))   {ret = is_mamba;} else
						if(_islike(line, "PS3HEN"))  {ret = payload_ps3hen;} else
						#endif
						#ifdef OVERCLOCKING
						if(_islike(line, "Overclock")) {ret = !((get_rsxclock(GPU_CORE_CLOCK) == 500) && (get_rsxclock(GPU_VRAM_CLOCK) == 650));} else
						#endif
						if(_islike(line, "DEX"))     {ret = dex_mode;} else
						{
							// check for hold L1 or R1 in script
							if(_islike(line, "L1")) ret = is_pressed(CELL_PAD_CTRL_L1);
							if(_islike(line, "R1")) ret = is_pressed(CELL_PAD_CTRL_R1);
						}

						if(isnot) ret = !ret;

						if(ifmode == ABORT_IF)
						{
							if(ret) break; // about script if true
						}
						else if(ret) // if condition is true
						{
							++do_else;
						}
						else // if condition is false
						{
							if(ifmode == DO_WHILE)
							{
								EXIT_LOOP; log_cmd("end while");
							}
							else
							{
								for(char *e, *i, *n;;)
								{
									buffer = pos + 1;
									n = strcasestr(buffer, "end if"); if(!n) break;
									i = strcasestr(buffer, "if ");
									e = strcasestr(buffer, "else");
									// jump to 'else'
									if(e && (e[4] <= ' ') && (*(--e) <= ' '))
									{
										// skip 'else' for nested if's (find next 'else')
										if(i && BETWEEN(i, e, n)) {pos = strchr(n, '\n'); if(!pos) pos = n + 6; buffer = pos; continue;}

										if(_islike(++e, "else if")) {log_cmd("else if"); pos = e + 4; break;}

										// go to end of line for 'else'
										if(e < n) {log_cmd("else"); pos = strchr(e, '\n'); if(!pos) pos = e + 4; break;}
									}
									// jump to 'end if'
									if((n[6] <= ' ') && (*(--n) <= ' '))
									{
										// skip 'end if' for nested if's (find next 'end if')
										if(i && (i < n)) {pos = strchr(++n, '\n'); if(!pos) pos = n + 6; buffer = pos; continue;}

										// go to end of line for 'end if'
										log_cmd("end if"); pos = strchr(++n, '\n'); if(!pos) pos = n + 6; break;
									}
									else
										pos = n + 6;
								}
							}
						}
					}
				}

				buffer = pos + 1, dest = NULL;
			}
			else
				break;
		}
		sys_memory_free(sysmem);

		script_running = !check_running; // false
		disable_progress();
	}
}

static void script_thread(u64 event_id)
{
	if(event_id == EVENT_PER_GAME)
	{
		get_game_info();
		if(strlen(_game_TitleID) >= 9)
		{
			char script_file[40];
			snprintf(script_file, sizeof(script_file), "%s/%s%s", WM_INGAME_PATH, _game_TitleID, ".bat");
			parse_script(script_file, true);
		}
	}
	else
		parse_script(script_events[event_id], true);

	sys_ppu_thread_exit(0);
}

static void start_event(u8 event_id)
{
	if((event_id < EVENT_PER_GAME) && not_exists(script_events[event_id])) return;

	if(wm_reload) return;

	if(event_id == EVENT_ON_XMB)
	{
		wait_for_xmb();
		sys_ppu_thread_sleep(3);
	}

	sys_ppu_thread_t t_id;
	sys_ppu_thread_create(&t_id, script_thread, event_id, THREAD_PRIO, THREAD_STACK_SIZE_SCRIPT, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_CMD);
}

#undef line
#undef path
#undef IS_WEB_COMMAND

static bool do_custom_combo(const char *filename)
{
 #ifdef WM_CUSTOM_COMBO
	char combo_file[128];

	if(*filename == '/')
		strncopy(combo_file, sizeof(combo_file), filename);
	else
		snprintf(combo_file, sizeof(combo_file), "%s%s", WM_CUSTOM_COMBO, filename); // use default path
 #else
	const char *combo_file = filename;
 #endif

	if(file_exists(combo_file))
	{
		parse_script(combo_file, true);
		sys_ppu_thread_sleep(2);
		return true;
	}
	return false;
}
#endif
#else
#define start_event(a)
#endif // #ifdef COPY_PS3
