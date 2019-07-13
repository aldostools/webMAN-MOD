// if/elseif/else/end
//   if exist <path>
//   if not exist <path>
//   if L1
//   if R1
//   abort if exist <path>
//   abort if not exist <path>

// map <path>=<path>
// unmap <path>
// md <path>
// del <path>
// copy <path>=<path>
// swap <path>=<path>
// ren <path>=<path>

// wait <1-9 secs>
// wait <path>
// lwait <path>

// logfile <pah>
// log <text>
// popup <text>
// beep1 / beep2 / beep3
// mute coldboot

// <webman_cmd> e.g. /mount.ps3<path>

#ifdef COPY_PS3

#define line	buffer	/* "line", "path" and "buffer" are synonyms */
#define path	buffer	/* "line", "path" and "buffer" are synonyms */
#define IS_WEB_COMMAND(line)	(islike(line, "/mount") || (strstr(line, ".ps3") != NULL) || (strstr(line, "_ps3") != NULL))

static void parse_script(const char *script_file)
{
	if(script_running) return;

	if(file_exists(script_file))
	{
		sys_addr_t sysmem = NULL;
		if(sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &sysmem)) return; 

		char *buffer = (char*)sysmem, *cr, *pos, *dest = NULL; u16 l = 0; 
		u8 exec_mode = true, enable_db = true, do_else = true;
		size_t buffer_size = read_file(script_file, buffer, _64KB_, 0); buffer[buffer_size] = 0;
		char log_file[STD_PATH_LEN + 1] = TMP_DIR "/log.txt";

		script_running = true;

		while(*buffer)
		{
			parse_cmd:
			if(++l > 9999 || !working) break;

			while( *buffer && (*buffer <= ' ')) buffer++; // skip blank chars \n \r \t

			if(*buffer == 0) break;

			// process line
			pos = strstr(line, "\n");
			if(pos)
			{
				if(pos) *pos = NULL; //EOL
				cr = strstr(line, "\r"); if(cr) *cr = NULL;

				if(exec_mode)
				{
					dest = strstr(line, "=/");
					if(!dest) dest = strstr(line, ",/");
					if(!dest) dest = strstr(line, ", /");
					if(enable_db && (strstr(line, "/dev_blind") != NULL)) {enable_dev_blind(NO_MSG); enable_db = false;}
				}
				if(dest)
				{
					*dest++ = NULL; if(*dest == ' ') dest++; //split parameters
					char *wildcard = strstr(line, "*");
	#ifdef COBRA_ONLY
					if(_islike(line, "map /"))  {line += 4;}
		#ifndef WM_REQUEST
					if(*line == '/') {if(IS(path, "/app_home")) set_apphome(path); else sys_map_path(path, dest);} else
		#else
					if(*line == '/') {if(IS_WEB_COMMAND(line)) handle_file_request(line); else if(IS(path, "/app_home")) set_apphome(path); else sys_map_path(path, dest);} else
		#endif
	#elif defined(WM_REQUEST)
					if(*line == '/') {if(IS_WEB_COMMAND(line)) handle_file_request(line);} else
	#endif
					if(_islike(line, "ren /"))  {path += 4; if(wildcard) {*wildcard++ = NULL;  scan(path, true, wildcard, SCAN_RENAME, dest);} else cellFsRename(path, dest);} else
					if(_islike(line, "copy /")) {path += 5; if(wildcard) {*wildcard++ = NULL;  scan(path, true, wildcard, SCAN_COPY, dest);} else if(isDir(path)) folder_copy(path, dest); else _file_copy(path, dest);} else
					if(_islike(line, "swap /")) {path += 5; sprintf(cp_path, "%s", path); char *slash = strrchr(cp_path, '/'); sprintf(slash, "/~swap"); cellFsRename(path, cp_path); cellFsRename(dest, path); cellFsRename(cp_path, dest);} else
					if(_islike(line, "move /")) {path += 5; if(wildcard) {*wildcard++ = NULL;  scan(path, true, wildcard, SCAN_MOVE, dest);} else cellFsRename(path, dest);} else
					if(_islike(line, "list /")) {path += 5; if(wildcard) {*wildcard++ = NULL;} scan(path, true, wildcard, SCAN_LIST, dest);} else
					if(_islike(line, "cpbk /")) {path += 5; if(wildcard) {*wildcard++ = NULL;} scan(path, true, wildcard, SCAN_COPYBK, dest);}
				}
				else if(*line == '#' || *line == ';' || *line == '*') ; // remark
				else if(_islike(line, "else") && do_else) {if(exec_mode) do_else = false; exec_mode ^= 1; line += 4; if(exec_mode && *line) goto parse_cmd;}
				else if(_islike(line, "end")) {exec_mode = do_else = true;}
				else if(exec_mode)
				{
	#ifdef WM_REQUEST
					if(*line == '/')               {if(IS_WEB_COMMAND(line)) handle_file_request(line);} else
	#endif
					if(_islike(line, "del /"))     {path += 4; char *wildcard = strstr(path, "*"); if(wildcard) {*wildcard++ = NULL; scan(path, true, wildcard, SCAN_DELETE, NULL);} else del(path, RECURSIVE_DELETE);} else
					if(_islike(line, "md /"))      {path += 3; mkdir_tree(path);} else
					if(_islike(line, "wait /"))    {path += 5; wait_for(path, 5);} else
					if(_islike(line, "lwait /"))   {path += 6; wait_for(path, 10);} else
					if(_islike(line, "wait "))     {line += 5; wait_path("/dev_hdd0", (u8)val(line), false);} else
					if(_islike(line, "beep"))      {if(line[4] == '3') {BEEP3;} else if(line[4] == '2') {BEEP2;} else {BEEP1;}} else
					if(_islike(line, "popup "))    {line += 6; show_msg(line);} else
					if(_islike(line, "log "))      {line += 4; save_file(log_file, line, APPEND_TEXT);} else
					if(_islike(line, "logfile /")) {path += 8; sprintf(log_file, "%s", path);} else
	#ifdef COBRA_ONLY
					if(_islike(line, "unmap /"))   {path += 6; sys_map_path(path, NULL);} else
					if(_islike(line, "mute coldboot"))
					{
						sys_map_path(VSH_RESOURCE_DIR "coldboot_stereo.ac3", SYSMAP_EMPTY_DIR);
						sys_map_path(VSH_RESOURCE_DIR "coldboot_multi.ac3", SYSMAP_EMPTY_DIR);
					}
					else
	#endif
					if(_islike(line, "if ") || _islike(line, "abort if "))
					{
						#define ABORT_IF	9

						bool ret = false; u8 ifmode = (_islike(line, "if ")) ? 3 : ABORT_IF; line += ifmode; do_else = true;

						if(_islike(line, "exist /"))     {path +=  6; ret =  file_exists(path);} else
						if(_islike(line, "not exist /")) {path += 10; ret = !file_exists(path);}
						else
						{
							CellPadData pad_data = pad_read();
							if(pad_data.len > 0)
							{
								if(_islike(line, "L1")) {ret = (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L1);}
								if(_islike(line, "R1")) {ret = (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R1);}
							}
						}

						if(ifmode == ABORT_IF) {if(ret) break;} else if(!ret) exec_mode = false;
					}
				}

				buffer = pos + 1, dest = NULL;
			}
			else
				break;
		}
		sys_memory_free(sysmem);

		script_running = false;
	}
}

#undef line
#undef path
#undef IS_WEB_COMMAND

#endif // #ifdef COPY_PS3
