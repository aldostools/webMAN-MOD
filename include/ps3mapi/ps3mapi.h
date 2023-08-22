///////////// PS3MAPI BEGIN //////////////

#define SYSCALL8_OPCODE_PS3MAPI						0x7777

//CORE
#define PS3MAPI_SERVER_VERSION						0x0120
#define PS3MAPI_SERVER_MINVERSION					0x0120

#define PS3MAPI_WEBUI_VERSION						0x0121
#define PS3MAPI_WEBUI_MINVERSION					0x0120

#define PS3MAPI_CORE_VERSION						0x0123
#define PS3MAPI_CORE_MINVERSION						0x0111

#define PS3MAPI_OPCODE_GET_CORE_VERSION				0x0011
#define PS3MAPI_OPCODE_GET_CORE_MINVERSION			0x0012

#define PS3MAPI_OPCODE_GET_FW_TYPE					0x0013
#define PS3MAPI_OPCODE_GET_FW_VERSION				0x0014

//PROCESSES
#define PS3MAPI_OPCODE_GET_ALL_PROC_PID				0x0021
#define PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID			0x0022
#define PS3MAPI_OPCODE_GET_PROC_BY_PID				0x0023
#define PS3MAPI_OPCODE_GET_CURRENT_PROC				0x0024
#define PS3MAPI_OPCODE_GET_CURRENT_PROC_CRIT		0x0025

//MEMORY
#define PS3MAPI_OPCODE_GET_PROC_MEM					0x0031
#define PS3MAPI_OPCODE_SET_PROC_MEM					0x0032
#define PS3MAPI_OPCODE_PROC_PAGE_ALLOCATE			0x0033
#define PS3MAPI_OPCODE_PROC_PAGE_FREE				0x0034

//MODULES
#define PS3MAPI_OPCODE_GET_ALL_PROC_MODULE_PID		0x0041
#define PS3MAPI_OPCODE_GET_PROC_MODULE_NAME			0x0042
#define PS3MAPI_OPCODE_GET_PROC_MODULE_FILENAME		0x0043
#define PS3MAPI_OPCODE_LOAD_PROC_MODULE				0x0044
#define PS3MAPI_OPCODE_UNLOAD_PROC_MODULE			0x0045
#define PS3MAPI_OPCODE_UNLOAD_VSH_PLUGIN			0x0046
#define PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO			0x0047
#define PS3MAPI_OPCODE_GET_PROC_MODULE_SEGMENTS		0x0048 // TheRouletteBoi
#define PS3MAPI_OPCODE_GET_VSH_PLUGIN_BY_NAME		0x004F

//THREAD
#define SYSCALL8_OPCODE_PROC_CREATE_THREAD			0x6E03 // not enough params for PS3MAPI_OPCODE

//PSID/IDPS
#define PS3MAPI_OPCODE_GET_IDPS 					0x0081
#define PS3MAPI_OPCODE_SET_IDPS 					0x0082
#define PS3MAPI_OPCODE_GET_PSID 					0x0083
#define PS3MAPI_OPCODE_SET_PSID						0x0084

//SYSCALL
#define SYSCALL8_OPCODE_STEALTH_TEST				0x3993 //KW PSNPatch stealth extension compatibility
#define SYSCALL8_OPCODE_STEALTH_ACTIVATE			0x3995 //KW PSNPatch stealth extension compatibility
#define SYSCALL8_STEALTH_OK							0x5555 //KW PSNPatch stealth extension compatibility
#define PS3MAPI_OPCODE_CHECK_SYSCALL				0x0091
#define PS3MAPI_OPCODE_DISABLE_SYSCALL				0x0092
#define PS3MAPI_OPCODE_PDISABLE_SYSCALL8 			0x0093 //Look in main.c for code.
#define PS3MAPI_OPCODE_PCHECK_SYSCALL8 				0x0094 //Look in main.c for code.
#define PS3MAPI_OPCODE_CREATE_CFW_SYSCALLS			0x0095
#define PS3MAPI_OPCODE_ALLOW_RESTORE_SYSCALLS		0x0096
#define PS3MAPI_OPCODE_GET_RESTORE_SYSCALLS			0x0097

//REMOVE COBRA/MAMBA HOOK
#define PS3MAPI_OPCODE_REMOVE_HOOK					0x0101

//PEEK POKE
#define PS3MAPI_OPCODE_SUPPORT_SC8_PEEK_POKE		0x1000
#define PS3MAPI_OPCODE_LV2_PEEK						0x1006
#define PS3MAPI_OPCODE_LV2_POKE						0x1007
#define PS3MAPI_OPCODE_LV1_PEEK						0x1008
#define PS3MAPI_OPCODE_LV1_POKE						0x1009

#define PS3MAPI_OPCODE_SUPPORT_SC8_PEEK_POKE_OK		0x6789

//SECURITY
#define PS3MAPI_OPCODE_SET_ACCESS_KEY				0x2000
#define PS3MAPI_OPCODE_REQUEST_ACCESS				0x2001

//MISC Cobra 8.3
#define PS3MAPI_OPCODE_AUTO_DEV_BLIND				0x2221
#define PS3MAPI_OPCODE_PHOTO_GUI					0x2222
#define PS3MAPI_OPCODE_RING_BUZZER 					0x2245

//RIF
#define PS3MAPI_OPCODE_GET_SKIP_EXISTING_RIF		0x2240
#define PS3MAPI_OPCODE_SKIP_EXISTING_RIF			0x2241
#define PS3MAPI_OPCODE_CREATE_RIF 		 			0x2249

//ACCOUNT
#define PS3MAPI_OPCODE_SET_FAKE_ACCOUNTID 			0x2247
#define PS3MAPI_OPCODE_ACTIVATE_ACCOUNT 	 		0x2248

//SAVEDATA
#define PS3MAPI_OPCODE_CONVERT_SAVEDATA 			0x2246

//FAN
#define PS3MAPI_OPCODE_GET_FAN_SPEED				0x2233
#define PS3MAPI_OPCODE_SET_FAN_SPEED				0x2235
#define PS3MAPI_OPCODE_SET_PS2_FAN_SPEED			0x2236

//QA
#define PS3MAPI_OPCODE_CHECK_QA 					0x2242
#define PS3MAPI_OPCODE_SET_QA 						0x2243
//

//SC8
#define PS3MAPI_ENABLED								1	// R2+TRIANGLE - CFW syscalls partially disabled - keep syscall 8 (PS3MAPI enabled)
#define PS3MAPI_DISABLED							4	// R2+TRIANGLE - CFW syscalls fully disabled - remove syscall 8 (PS3MAPI disabled)

#define PS3MAPI_FIND_FREE_SLOT						NULL

///////////// PS3MAPI END //////////////

#define HOME_PS3MAPI	"<a href='/home.ps3mapi'>PS3MAPI</a> --> "

#if defined(REMOVE_SYSCALLS) || defined(PS3MAPI)
#define CFW_SYSCALLS 16
static u16 sc_disable[CFW_SYSCALLS] = {200, 201, 202, 203, 204, 1022, 6, 7, 10, 11, 15, 20, 35, 36, 38, 9};
#else
#define CFW_SYSCALLS 17
static u16 sc_disable[CFW_SYSCALLS] = {200, 201, 202, 203, 204, 1022, 6, 7, 10, 11, 15, 20, 35, 36, 38, 8, 9};
#endif

#ifdef COBRA_ONLY

#ifdef DISABLE_SIGNIN_DIALOG
static void disable_signin_dialog(void);
static void enable_signin_dialog(void);
#else
#define disable_signin_dialog() {}
#define enable_signin_dialog() {}
#endif

static u32 get_current_pid(void)
{
	if(IS_INGAME)
		return GetGameProcessID();

	u32 pid_list[16];
	{system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (u64)(u32)pid_list); }

	for(int i = 0; i < 16; i++)
	{
		if(pid_list[i] > 2)
			return pid_list[i];
	}
	return 0;
}

static void ps3mapi_get_vsh_plugin_info(unsigned int slot, char *tmp_name, char *tmp_filename)
{
	_memset(tmp_name, 32);
	_memset(tmp_filename, STD_PATH_LEN);
	system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO, (u64)slot, (u64)(u32)tmp_name, (u64)(u32)tmp_filename);
}

static void ps3mapi_check_unload(unsigned int slot, char *tmp_name, char *tmp_filename)
{
	ps3mapi_get_vsh_plugin_info(slot, tmp_name, tmp_filename);
	if(IS(tmp_name, "WWWD")) wwwd_stop();

	#ifdef FPS_OVERLAY
	if(strstr(tmp_filename, "/VshFpsCounter")) overlay_enabled = 0;
	#endif
}

static int ps3mapi_get_vsh_plugin_slot_by_name(const char *name, int mode)
{
	char tmp_name[32];
	char tmp_filename[STD_PATH_LEN];
	const char *plugin_path = name; // alias

	bool find_free_slot = (!name || (*name == PS3MAPI_FIND_FREE_SLOT));
	bool load_in_new_slot = !find_free_slot && (mode >= 2) && file_exists(plugin_path);
	bool prx_found = false;

	int slot;
	if(!find_free_slot && (mode >= 2))
	{
		slot = ps3mapi_get_vsh_plugin_slot_by_name(plugin_path, 0);
		if(slot < 7)
		{
			if(mode == 3) goto unload_plugin; // 3 = toggle mode (check unload)
			if(mode >= 2) return slot; // 2-5 = load vsh plugin (prevent load again)
		}
	}

	for (slot = 1; slot < 7; slot++)
	{
		ps3mapi_get_vsh_plugin_info(slot, tmp_name, tmp_filename);

		if(find_free_slot || load_in_new_slot) // 0 = find mode, 2 = load mode, 3 = toggle mode (load), 4 = load vsh gui
		{
			if(*tmp_name && (mode != 5)) continue; // slot is busy

			if(load_in_new_slot)
			{
				char arg[2] = {1, 0};
				if(mode == 5)
				{
					// get VSH id
					u32 pid = IS_INGAME ? 0x01000300 : get_current_pid(); // VSH;

					// load VSH module
					{system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LOAD_PROC_MODULE, (u64)pid, (u64)(u32)plugin_path, NULL, 0); }

					prx_found = true; break;
				}
				else if(mode == 4) // 4 = load vsh gui
					cobra_load_vsh_plugin(slot, plugin_path, (u8*)arg, 1);
				else // 2-3 = load vsh plugin
					cobra_load_vsh_plugin(slot, plugin_path, NULL, 0);

				ps3mapi_get_vsh_plugin_info(slot, tmp_name, tmp_filename); // check if plugin was loaded
				if(*tmp_filename) prx_found = true; else continue;
			}
			break;
		}
		else if(IS(tmp_name, name) || strstr(tmp_filename, name))
		{
			if(mode == 1) // 1 = unload mode
			{
				unload_plugin:
					ps3mapi_check_unload(slot, tmp_name, tmp_filename);
					cobra_unload_vsh_plugin(slot);
			}
			else prx_found = true; // 0 = find mode
			break;
		}
	}
	#ifdef FPS_OVERLAY
	if(!find_free_slot && strstr(plugin_path, "/VshFpsCounter")) {overlay_enabled = prx_found, overlay_info = 0;}
	#endif
	return slot;
}

static void unload_vsh_gui(void)
{
	if(syscalls_removed)
		show_msg_with_icon(ICON_EXCLAMATION, STR_CFWSYSALRD);
	else if(!cobra_version)
		show_error("[nonCobra]");

	unload_vsh_plugin("VSH_MENU"); // unload vsh menu
	unload_vsh_plugin("sLaunch");  // unload sLaunch
}

static void start_vsh_gui(bool vsh_menu)
{
	unload_vsh_gui();

	char plugin_path[40];
	sprintf(plugin_path, "%s/%s.sprx", WM_RES_PATH, vsh_menu ? "wm_vsh_menu" : "slaunch");
	load_vsh_gui(plugin_path);
}
#endif
///////////////////////////////

#ifdef PS3MAPI

static uint64_t StartGamePayload(int pid, const char* fileName, int prio, size_t stacksize, uint64_t outPageTable[2], char* error_msg)

static void ps3mapi_syscall8(char *buffer, char *templn, const char *param);
static void ps3mapi_setmem(char *buffer, char *templn, const char *param);

static u64 residence = 0;

static u32 found_offset = 0;
static u8 ps3mapi_working = 0;

static int is_syscall_disabled(u32 sc)
{
	int ret_val = NONE;
	{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_CHECK_SYSCALL, sc); ret_val = (int)p1;}
	if(ret_val<0) {u64 sc_null = peekq(SYSCALL_TABLE); ret_val = (peekq(SYSCALL_PTR(sc)) == sc_null);}

	return ret_val;
}

static void add_sound_list(char *buffer, const char *param)
{
	add_option_item(1, "Simple", IS_MARKED("snd=1"), buffer);
	add_option_item(2, "Double", IS_MARKED("snd=2"), buffer);
	add_option_item(3, "Triple", IS_MARKED("snd=3"), buffer);
	add_option_item(0, "snd_cancel", IS_MARKED("snd=0"), buffer);
	add_option_item(4, "snd_cursor", IS_MARKED("snd=4"), buffer);
	add_option_item(5, "snd_trophy", IS_MARKED("snd=5"), buffer);
	add_option_item(6, "snd_decide", IS_MARKED("snd=6"), buffer);
	add_option_item(7, "snd_option", IS_MARKED("snd=7"), buffer);
	add_option_item(8, "snd_system_ok", IS_MARKED("snd=8"), buffer);
	add_option_item(9, "snd_system_ng", IS_MARKED("snd=9"), buffer);
}

static void ps3mapi_buzzer(char *buffer, char *templn, const char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	if(islike(param, "/buzzer.ps3mapi") && param[15] == '?')
	{
		u8 value = get_valuen(param, "snd=", 0, 9) | get_valuen(param, "mode=", 0, 9);
		play_sound_id(value);
	}
	else if(islike(param, "/beep.ps3"))
	{
		play_sound_id(param[10]);
	}

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s%s</b>"
						HTML_BLU_SEPARATOR,
						HOME_PS3MAPI, "Buzzer");
	else
		sprintf(templn, "<td width=\"260\" class=\"la\"><u>%s:</u><br>", "Buzzer");
	concat(buffer, templn);

	sprintf(templn, "<form id=\"buzzer\" action=\"/buzzer%s<br>"
					"<b>%s:</b>  <select name=\"snd\">", HTML_FORM_METHOD, "Sound");
	concat(buffer, templn);

	add_sound_list(buffer, param);

	sprintf(templn, "</select>   <input type=\"submit\" value=\" %s \"/></form><br>", "Ring");

	if(!is_ps3mapi_home) strcat(templn, HTML_RED_SEPARATOR); else strcat(templn, "");
	concat(buffer, templn);
}

static void ps3mapi_led(char *buffer, char *templn, const char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	if(islike(param, "/led.ps3mapi") && param[12] == '?')
	{
		int color = get_valuen(param, "color=", 0, 2);
		int mode  = get_valuen(param, "mode=", 0, 6);
		int mode2 = mode; if(mode >= 4) color = 2;
		if(mode == 4) {mode = 1, mode2 = 2;}
		if(mode == 5) {mode = 2, mode2 = 3;}
		if(mode == 6) {mode = 3, mode2 = 2;}

		if((color == 0) || (color == 2)) { system_call_2(SC_SYS_CONTROL_LED, RED, mode); }
		if((color == 1) || (color == 2)) { system_call_2(SC_SYS_CONTROL_LED, GREEN, mode2); }
	}

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s%s</b>"
						HTML_BLU_SEPARATOR,
						HOME_PS3MAPI, "Led");
	else
		sprintf(templn, "<td width=\"260\" class=\"la\"><u>%s:</u><br>", "Led");

	concat(buffer, templn);

	sprintf(templn, "<form id=\"led\" action=\"/led%s<br>"
					"<b>%s:</b>  <select name=\"color\">", HTML_FORM_METHOD,  "Color"); concat(buffer, templn);

	add_option_item(0, "Red",				 IS_MARKED("color=0"), buffer);
	add_option_item(1, "Green",				 IS_MARKED("color=1"), buffer);
	add_option_item(2, "Yellow (Red+Green)", IS_MARKED("color=2"), buffer);

	sprintf(templn, "</select>   <b>%s:</b>  <select name=\"mode\">", "Mode"); concat(buffer, templn);

	add_option_item(0, "Off",		 IS_MARKED("mode=0"), buffer);
	add_option_item(1, "On",		 IS_MARKED("mode=1"), buffer);
	add_option_item(2, "Blink fast", IS_MARKED("mode=2"), buffer);
	add_option_item(3, "Blink slow", IS_MARKED("mode=3"), buffer);
	add_option_item(4, "Blink alt1", IS_MARKED("mode=4"), buffer);
	add_option_item(5, "Blink alt2", IS_MARKED("mode=5"), buffer);
	add_option_item(6, "Blink alt3", IS_MARKED("mode=6"), buffer);

	sprintf(templn, "</select>   <input type=\"submit\" value=\" %s \"/></form><br>", "Set");
	if(!is_ps3mapi_home) strcat(templn, HTML_RED_SEPARATOR); else strcat(templn, "</table>");
	concat(buffer, templn);
}

static void ps3mapi_notify(char *buffer, char *templn, const char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	u8 icon_id = (u8)get_valuen32(param, "&icon=");

	const char *snd = strstr(param, "&snd=");

	if(snd && ISDIGIT(snd[5])) play_sound_id(snd[5]);

	char msg[200]; strcpy(msg, "Hello :)");
	if(get_param("?msg=", msg, param, 199))
	{
		if(icon_id) webman_config->msg_icon = 0; // enable icons

		parse_tags(msg);
		show_msg_with_icon(icon_id, msg);
	}

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s%s</b>"
						HTML_BLU_SEPARATOR,
						HOME_PS3MAPI, "Notify");
	else
		sprintf(templn, "<tr><td class=\"la\"><br><u>%s:</u><br><br>", "Notify");

	concat(buffer, templn);

	sprintf(templn, HTML_FORM_METHOD_FMT("/notify")
					"<table width=\"800\">"
					"<tr><td class=\"la\">"
					"<textarea name=\"msg\" cols=\"111\" rows=\"2\" maxlength=\"199\">%s</textarea>"
					"<br>Icon (0-50): " HTML_NUMBER("icon", "%i", "0", "50")
					" Sound: <select name=\"snd\"><option value=''>No Sound", HTML_FORM_METHOD, msg, icon_id);
	concat(buffer, templn);

	add_sound_list(buffer, param);

	sprintf(templn, "</tr>"
					"<tr><td class=\"ra\">"
					"<input class=\"bs\" type=\"submit\" value=\" %s \"/>%s",
					"Send", "</tr></table></form>");

	if(!is_ps3mapi_home) strcat(templn, HTML_RED_SEPARATOR); else strcat(templn, "");
	concat(buffer, templn);
}

static void ps3mapi_mappath(char *buffer, char *templn, const char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	sprintf(templn, "<b>%s%s</b>"
					HTML_BLU_SEPARATOR,
					is_ps3mapi_home ? "" : HOME_PS3MAPI, "Map Path");
	concat(buffer, templn);

	char src[STD_PATH_LEN]; _memset(src, STD_PATH_LEN);
	char dst[STD_PATH_LEN]; _memset(dst, STD_PATH_LEN);

	if(get_param("?src=", src, param, STD_PATH_LEN))
	{
		char *label = templn + 400;
		get_param("&to=", dst, param, STD_PATH_LEN);
		if(*dst == '/')
			{sys_map_path(src, dst); sprintf(label, "%s: %s => %s", "Remap", src, dst);}
		else
			{sys_map_path(src, NULL); sprintf(label, "%s: %s", "Unmap", src);}

		sprintf(templn, "<p><a href=\"%s\" style=\"padding:8px;background:#900;border-radius:8px;\">%s</a><p>", src, label); concat(buffer, templn);
	}

	sprintf(templn, HTML_FORM_METHOD_FMT("/mappath")
					"<table width=\"800\">"
					"<tr><td class=\"la\">"
					HTML_INPUT("src\" style=\"width:400px", "%s", "260", "200") " => "
					HTML_INPUT( "to\" style=\"width:400px", "%s", "260", "200"),
					HTML_FORM_METHOD, src, dst);
	concat(buffer, templn);

	sprintf(templn, "</tr>"
					"<tr><td class=\"ra\">"
					"<input class=\"bs\" type=\"submit\" value=\" %s \"/>%s",
					"Remap", " <button class=\"bs\" onclick=\"to.value='';\"> Unmap </button>");
	strcat(templn, "</tr></table></form>");

	if(!is_ps3mapi_home) strcat(templn, HTML_RED_SEPARATOR); else strcat(templn, "<br>");
	concat(buffer, templn);
}

static bool add_sc_checkbox(int sc, const char *id, const char *label, char *buffer)
{
	bool disabled = is_syscall_disabled(sc);
	add_check_box(id, disabled, label, _BR_, (!disabled), buffer);
	return disabled;
}

static void ps3mapi_syscall(char *buffer, char *templn, const char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	u64 sc_null = peekq(SYSCALL_TABLE);

	if(strstr(param, ".ps3mapi?"))
	{
		for(u8 sc = 0; sc < CFW_SYSCALLS; sc++)
		{
			sprintf(templn, "sc%i=1", sc_disable[sc]);
			if(IS_MARKED(templn))
			{
				if(sc_disable[sc] == SC_PEEK_LV2) {peekq = lv2_peek_ps3mapi;}
				if(sc_disable[sc] == SC_POKE_LV2) {pokeq = lv2_poke_ps3mapi, lv2_poke_fan = (payload_ps3hen) ? lv2_poke_fan_hen : lv2_poke_ps3mapi;}
				if(sc_disable[sc] == SC_POKE_LV1) {poke_lv1 = lv1_poke_ps3mapi;}

				pokeq(SYSCALL_PTR(sc_disable[sc]), sc_null);
				system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, (u64)sc_disable[sc]);
			}
		}

#ifdef REMOVE_SYSCALLS
		if(IS_MARKED("sce=1"))  { restore_cfw_syscalls(); } else
		if(IS_MARKED("scd=1"))  { remove_cfw_syscalls(webman_config->keep_ccapi); }
#endif
	}

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s%s</b>"
						HTML_BLU_SEPARATOR
						"<table width=\"800\">", HOME_PS3MAPI, "CFW syscall");
	else
		sprintf(templn, "<table width=\"800\">"
						"<tr><td class=\"la\"><u>%s:</u><br><br></tr>", "CFW syscall");

	concat(buffer, templn);

	sprintf(templn, "<form id=\"syscall\" action=\"/syscall%s"
					"<br><tr><td width=\"260\" class=\"la\">",
					HTML_FORM_METHOD); concat(buffer, templn);

	u8 sc_count = 0;

	if(add_sc_checkbox(6, "sc6", "[6]LV2 Peek", buffer)) sc_count++;
	if(add_sc_checkbox(7, "sc7", "[7]LV2 Poke", buffer)) sc_count++;
	if(add_sc_checkbox(9, "sc9", "[9]LV1 Poke", buffer)) sc_count++;
	add_sc_checkbox(10, "sc10", "[10]LV1 Call", buffer);
	add_sc_checkbox(15, "sc15", "[15]LV2 Call", buffer);
	add_sc_checkbox(11, "sc11", "[11]LV1 Peek", buffer);

	concat(buffer, "<td  width=\"260\"  valign=\"top\" class=\"la\">");

	add_sc_checkbox(35, "sc35", "[35]Map Path", buffer);
	add_sc_checkbox(36, "sc36", "[36]Map Game", buffer);
	add_sc_checkbox(38, "sc38", "[38]New sk1e", buffer);
	add_sc_checkbox(1022, "sc1022", "[1022]PRX Loader", buffer);

	concat(buffer, "<td  width=\"260\"  valign=\"top\" class=\"la\">");

	add_sc_checkbox(200, "sc200", "[200]sys_dbg_read_process_memory", buffer);
	add_sc_checkbox(201, "sc201", "[201]sys_dbg_write_process_memory", buffer);
	add_sc_checkbox(202, "sc202", "[202]Free - Payloader3", buffer);
	add_sc_checkbox(203, "sc203", "[203]LV2 Peek CCAPI", buffer);
	add_sc_checkbox(204, "sc204", "[204]LV2 Poke CCAPI", buffer);

#ifdef REMOVE_SYSCALLS
	concat(buffer, "<br>");
	if(sc_count)  add_checkbox("sce\" onclick=\"b.value=(this.checked)?' Enable ':'Disable';", "Re-Enable Syscalls & Unlock syscall 8", _BR_, false, buffer);
	else	 add_checkbox_line("scd", "Disable Syscalls & Lock syscall 8", false, buffer);
#endif

	sprintf(templn, "</tr><tr><td class=\"ra\"><br><input class=\"bs\" id=\"%c\" type=\"submit\" value=\" %s \"/></tr></table></form><br>", 'b', "Disable");
	concat(buffer, templn);

	if(!is_ps3mapi_home && islike(param, "/syscall.ps3mapi")) {ps3mapi_syscall8(buffer, templn, param);}
}

static void ps3mapi_syscall8(char *buffer, char *templn, const char *param)
{
	bool is_ps3mapi_home = (*param == ' ');
	int ret_val = NONE;
	int disable_cobra = 0, disabled_cobra = 0;

	if(strstr(param, ".ps3mapi?"))
	{
		u8 mode = get_valuen(param, "mode=", 0, 5);

		#ifdef DISABLE_SIGNIN_DIALOG
		if(mode) enable_signin_dialog();
		#endif

		{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_DISABLE_COBRA, (mode == 5)); }

		if( mode <= 3 ) { system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, (u64)mode); }
		if( mode == 4 ) { system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, SC_COBRA_SYSCALL8); }
		if( mode == 5 )	{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, 3); }

		webman_config->sc8mode = (mode == 4) ? PS3MAPI_DISABLED : PS3MAPI_ENABLED; // fully disabled : Partially disabled
	}

	if(!is_ps3mapi_home && islike(param, "/syscall8.ps3mapi")) {ps3mapi_syscall(buffer, templn, param);}

	{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_DISABLE_COBRA, SYSCALL8_DISABLE_COBRA_CAPABILITY); disable_cobra = (int)p1;}
	if(disable_cobra == SYSCALL8_DISABLE_COBRA_OK)
	{
		{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_DISABLE_COBRA, SYSCALL8_DISABLE_COBRA_STATUS); disabled_cobra = (int)p1;}
		if(disabled_cobra) ret_val = 3;
	}

	sprintf(templn, "<b>%s%s</b>"
					HTML_BLU_SEPARATOR
					"<table width=\"800\">"
					"<form id=\"syscall8\" action=\"/syscall8%s"
					"<br><tr><td class=\"la\">",
					is_ps3mapi_home ? "" : HOME_PS3MAPI, "CFW syscall 8", HTML_FORM_METHOD); concat(buffer, templn);

	{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PCHECK_SYSCALL8); ret_val = (int)p1;}

	syscalls_removed = (ret_val != 0); peek_lv1 = (syscalls_removed) ? lv1_peek_ps3mapi : lv1_peek_cfw;

	#ifdef DISABLE_SIGNIN_DIALOG
	if(!syscalls_removed) disable_signin_dialog();
	#endif

	if(ret_val < 0)
	{
		add_radio_button("mode\" disabled", 0, "sc8_0", "Fully enabled", _BR_, false, buffer);
		add_radio_button("mode\" disabled", 1, "sc8_1", "Partially disabled : Keep only COBRA/MAMBA/PS3MAPI features", _BR_, false, buffer);
		add_radio_button("mode\" disabled", 2, "sc8_2", "Partially disabled : Keep only PS3MAPI features", _BR_, false, buffer);
		add_radio_button("mode\" disabled", 3, "sc8_3", "Fake disabled (can be re-enabled)", _BR_, false, buffer);
	}
	else
	{
		if(syscalls_removed && (ret_val == 0)) ret_val = 1; if(!c_firmware) ret_val = 4;

		add_radio_button("mode", 0, "sc8_0", "Fully enabled", _BR_, (ret_val == 0), buffer);
		add_radio_button("mode", 1, "sc8_1", "Partially disabled : Keep only COBRA/MAMBA/PS3MAPI features", _BR_, (ret_val == 1), buffer);
		add_radio_button("mode", 2, "sc8_2", "Partially disabled : Keep only PS3MAPI features", _BR_, (ret_val == 2), buffer);

		if(disable_cobra == SYSCALL8_DISABLE_COBRA_OK)
			add_radio_button("mode", 5, "sc8_5", "Disable COBRA/MAMBA/PS3MAPI features / keep lv1_peek (can be re-enabled)", _BR_, disabled_cobra, buffer);

		add_radio_button("mode", 3, "sc8_3", "Fake disabled (can be re-enabled)", _BR_, (ret_val == 3 && !disabled_cobra), buffer);
	}

	if(ret_val < 0 || ret_val == 3)
		add_radio_button("mode\" disabled=\"disabled", 4, "sc8_4", "Fully disabled (can't be re-enabled)", _BR_, (ret_val < 0), buffer);
	else
		add_radio_button("mode", 4, "sc8_4", "Fully disabled (can't be re-enabled)", _BR_, false, buffer);

	sprintf(templn, "</tr><tr><td class=\"ra\"><br><input class=\"bs\" id=\"%c\" type=\"submit\" value=\" %s \"/></tr></table></form><br>", 's', "Set");

	if(!is_ps3mapi_home) strcat(templn, HTML_RED_SEPARATOR);
	concat(buffer, templn);
}

static void ps3mapi_get_process_name_by_id(u32 pid, char *name, u16 size)
{
	_memset(name, size);
	system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID, (u64)pid, (u64)(u32)name);
}

static u8 add_proc_list(char *buffer, char *templn, u32 *proc_id, u8 src)
{
	u8 is_vsh = 0;
	u32 pid = *proc_id;

	concat(buffer, "<select name=\"proc\">");

	u32 pid_list[16];
	{system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (u64)(u32)pid_list); }
	if(pid == 0) pid = pid_list[0];
	for(int i = 0; i < 16; i++)
	{
		if(1 < pid_list[i])
		{
			ps3mapi_get_process_name_by_id(pid_list[i], templn, MAX_LINE_LEN);
			if(*templn) add_option_item(pid_list[i], templn, pid_list[i] == pid, buffer);
			if(pid_list[i] == pid) is_vsh = (strstr(templn, "_main_vsh.self") != NULL);
		}
	}

	if(src < 3)
	{
		if(!payload_ps3hen)
			add_option_item(LV1, "LV1 Memory", (pid == LV1), buffer);
			add_option_item(LV2, "LV2 Memory", (pid == LV2), buffer);
		if(src == 1)
		{
			add_option_item(FLASH, "Flash", (pid == FLASH), buffer);
			add_option_item(HDD0, drives[0], (pid == HDD0), buffer);
			add_option_item(USB0, drives[1], (pid == USB0), buffer);
			add_option_item(USB1, drives[2], (pid == USB0 + 1), buffer);
		}
	}
	concat(buffer, "</select>");
	concat(buffer, " <input class=\"bs\" type=\"submit\" value=\" Load \">");

	if(pid >= PID)
	{
		if(IS_INGAME)
		{
			sprintf(templn, HTML_BUTTON_FMT, HTML_BUTTON, "Exit", HTML_ONCLICK, "/xmb.ps3$exit");
			concat(buffer, templn);

			sprintf(templn, HTML_BUTTON_FMT2, HTML_BUTTON, "Reload", HTML_ONCLICK2, "/xmb.ps3$reloadgame", HTML_SEND_CMD);
			concat(buffer, templn);

			#ifdef ARTEMIS_PRX
			sprintf(templn, HTML_BUTTON_FMT, HTML_BUTTON, "Artemis", HTML_ONCLICK, "/artemis.ps3");
			concat(buffer, templn);
			#endif
		}

		sprintf(templn, HTML_BUTTON_FMT2, HTML_BUTTON, "Pause", HTML_ONCLICK2, "/xmb.ps3$rsx_pause", HTML_SEND_CMD);
		concat(buffer, templn);

		sprintf(templn, HTML_BUTTON_FMT2, HTML_BUTTON, "Continue", HTML_ONCLICK2, "/xmb.ps3$rsx_continue", HTML_SEND_CMD);
		concat(buffer, templn);

		char *link = templn + 0x200;
		sprintf(link, "%s?proc=0x%x", (src == 3) ? "/getmem.ps3mapi" : "/gameplugin.ps3mapi", pid);
		sprintf(templn, HTML_BUTTON_FMT, HTML_BUTTON, (src == 3) ? "Process" : "Plugins", HTML_ONCLICK, link);
		concat(buffer, templn);
	}
	concat(buffer, "<br><br>");

	add_game_info(buffer, templn, src);

	*proc_id = pid;

	return is_vsh;
}

static u32 ps3mapi_find_offset(u32 pid, u32 address, u32 stop, u8 step, const char *sfind, u8 len, char *mask, u32 fallback)
{
	char label[20], buffer[0x200], *mem;

	int retval = NONE;
	found_offset = fallback;

	u32 chunk_size = _64KB_; sys_addr_t sysmem = sys_mem_allocate(chunk_size);
	if(sysmem)
	{
		mem = (char*)sysmem;
	}
	else // fallback to small buffer
	{
		mem = buffer, chunk_size = sizeof(buffer);
	}

	u64 faster = faster_find(sfind, len, mask);
	const u32 m = chunk_size - len, gap = (len + 0x10) - (len % 0x10);
	for(; address < stop; address += chunk_size - gap)
	{
		retval = ps3mapi_get_memory(pid, address, mem, chunk_size);
		if(retval < 0) break;

		sprintf(label, "0x%x", address); show_progress(label, OV_FIND);

		for(u32 offset = 0; offset < m; offset += step)
		{
			if(faster && (*(u64*)(mem + offset) != faster)) continue;

			if( !bcompare(mem + offset, sfind, len, mask) )
			{
				found_offset = (address + offset);
				address = stop;
				break;
			}
		}
	}
	if(chunk_size == _64KB_)
		sys_memory_free(sysmem);
	disable_progress();
	return found_offset;
}

static int ps3mapi_patch_process(u32 pid, u32 address, const char *new_value, int size, u8 oper)
{
	if(pid == LV1)
	{
		poke_chunk_lv1(address, size, (u8*)new_value, oper);
	}
	else if(pid == LV2)
	{
		poke_chunk_lv2(address, size, (u8*)new_value, oper);
	}
	else
	{
		#ifndef LITE_EDITION
		if(oper)
		{
			int len = (size & 0xFFFF8) + 8; char old_value[len];
			system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MEM, (u64)pid, (u64)address, (u64)(u32)old_value, (u64)len);
			u64 *nv = (u64*)new_value, *ov = (u64*)old_value;
			for(u8 i = 0; i < (len/8); i++, ov++, nv++) *nv = update_value(*ov, *nv, oper);
		}
		#endif
		system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PROC_MEM, (u64)pid, (u64)address, (u64)(u32)new_value, (u64)size);
		return (int)p1;
	}
	return 0;
}

#define BINDATA_SIZE	0x100
#define HEXDATA_SIZE	0x200
#define BINDATA_LEN		"256"
#define HEXDATA_LEN		"512"

static void ps3mapi_getmem(char *buffer, char *templn, const char *param)
{
	u32 pid = get_valuen32(param, "proc=");
	u32 address = 0;
	int length = BINDATA_SIZE;
	found_offset = 0;
	int hilite = 0;

	bool is_read_only = (pid >= FLASH) && (pid < PID);
	bool is_ps3mapi_home = (*param == ' ');
	bool not_found = false;

	const char *find = "";
	const char *dump_file = (char*)"/dev_hdd0/mem_dump.bin";
	const char *dump_size = (char*)"&dump=400000";

	if(strstr(param, ".ps3mapi?"))
	{
		char addr_tmp[0x60];
		if(get_param("addr=", addr_tmp, param, 0x10))
		{
			address = convertH(addr_tmp);

			length = (int)get_valuen32(param, "len=");
			if(length == 0) length = BINDATA_SIZE;
			length = RANGE(length, 1, BINDATA_SIZE);
		}

		if(!pid) pid = get_current_pid();

		if(pid >= PID) address = MAX(address, 0x10000);

		if(get_param("find=", addr_tmp, param, 0x60))
		{
			find = strstr(param, "find=") + 5;
			char sfind[0x60], *mask = addr_tmp;
			u8 len = snprintf(sfind, sizeof(sfind), "%s", addr_tmp);

			// search hex: 0xAABBCC112233
			if(isHEX(addr_tmp))
			{
				len = Hex2Bin(addr_tmp, sfind);
				for(u8 i = 0, n = 0; i < len; i++, n+=2) mask[i] = addr_tmp[n]; sfind[len] = mask[len] = 0;
			}
			else if(strstr(param, "&exact"))
				_memset(mask, len);

			if(address == 0) address = 4;

			u32 addr = address;
			u32 stop = 0; const char *pos = strstr(param, "&stop=");
			if(pos) stop = convertH(pos + 6); if(stop < address) stop = (address + 0x1000000ULL);
			u8  step = get_valuen(param, "step=", 0, 0xE0); if(step < 1) step = 4;
			u8  rep  = get_valuen(param, "rep=", 1, 0xFF);

			address += step;

			while(rep--)
			{
				address = ps3mapi_find_offset(pid, address, stop, step, sfind, len, mask, addr);
				if(rep)
				{
					if(address == addr) break;
					address += step;
				}
			}
			if(address == addr)
				not_found = true;
			else
				hilite = len;

		}

		if(pid == FLASH)
		{
			#ifdef SPOOF_CONSOLEID
			if(!address)
			{
				get_eid0_idps();
				dump_size = IS_NAND ? (char*)"&dump=10000000" : (char*)"&dump=1000000";
			}
			#else
			if(!address)
				dump_size = (char*)"&dump=1000000";
			#endif
			dump_file = (char*)"/dev_hdd0/dump_flash.bin";
		}

		if(!not_found) // ignore dump / patch_process if find returned not found
		{
			int offset = (int)get_valuen64(param, "&offset="); address += offset;

			if(get_param("dump=", addr_tmp, param, 16))
			{
				u32 size = convertH(addr_tmp);
				if(size <= 256) size *= _1MB_;
				if(size >= _64KB_) ps3mapi_dump_process(dump_file, pid, address, size);
			}

			char *val_param = strstr(param, "&val");
			if(val_param)
			{
				char value[BINDATA_SIZE + 1];
				char val_tmp[HEXDATA_SIZE + 1];
				char *new_value = val_tmp;

				u8 oper = get_operator(val_param, false); // val=, val|=, val&=, val^=, val+=, val-=
				if(!oper && get_param("oper=", addr_tmp, param, 4)) oper = (u8)val(addr_tmp);

				ps3mapi_get_memory(pid, address, value, BINDATA_SIZE);

				hilite = length = get_param("=", new_value, val_param, HEXDATA_SIZE);
				if(isHEX(val_tmp))
					{hilite = length = Hex2Bin(val_tmp, value); new_value = (char*)value;}

				if(length) {ps3mapi_patch_process(pid, address, new_value, length, oper);}
				length = BINDATA_SIZE;
			}
		}
	}

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s%s</b>"
						HTML_BLU_SEPARATOR,
						HOME_PS3MAPI, "Get process memory");
	else
		sprintf(templn, "<b>%s</b>"
						HTML_BLU_SEPARATOR,
						"Processes Commands");

	concat(buffer, templn);

	sprintf(templn, HTML_FORM_METHOD_FMT("/getmem")
					"<b><u>%s:</u></b>  ", HTML_FORM_METHOD, "Process"); concat(buffer, templn); _memset(templn, MAX_LINE_LEN);

	add_proc_list(buffer, templn, &pid, 1);

	sprintf(templn, "<b><u>%s:</u></b> " HTML_INPUT("addr", "%X", "16", "18")
					" <b><u>%s:</u></b> " HTML_NUMBER("len", "%i", "1", BINDATA_LEN)
					" <input class=\"bs\" type=\"submit\" value=\" %s \"/></form>", "Address", address, "Length", length, "Get");
	concat(buffer, templn);

	if((pid != 0) && (length > 0))
	{
		sprintf(templn, "<br><b><u>%s:</u></b>", "Output");
		concat(buffer, templn);

		sprintf(templn, " <a id=\"pblk\" href=\"/getmem.ps3mapi?proc=0x%x&addr=%x\">&lt;&lt;</a> <a id=\"back\" href=\"/getmem.ps3mapi?proc=0x%x&addr=%x\">&lt;Back</a>", pid, address - 0x2000, pid, address - BINDATA_SIZE); buffer += concat(buffer, templn);
		sprintf(templn, " <a id=\"next\" href=\"/getmem.ps3mapi?proc=0x%x&addr=%x\">Next></a> <a id=\"nblk\" href=\"/getmem.ps3mapi?proc=0x%x&addr=%x\">>></a>", pid, address + BINDATA_SIZE, pid, address + 0x2000); buffer += concat(buffer, templn);

		if(file_exists(dump_file)) {add_breadcrumb_trail2(buffer, " [", dump_file); concat(buffer, " ]");}

		if(!strstr(param, "dump=")) sprintf(templn, " [<a href=\"%s%s\">%s</a>]", param, dump_size, "Dump Process");
		concat(buffer, templn);

		char *pos = strstr(param, "&addr="); if(pos) *pos = 0;
		sprintf(templn, " [<a href=\"javascript:void(location.href='%s&addr=%x&find='+prompt('%s','%s').toString());\">%s</a>] %s%s%s", param, address, "Find", find, "Find", "<font color=#ff0>", not_found ? "Not found!" : "", "</font><hr>");
		concat(buffer, templn); if(pos) *pos = '&';
		char buffer_tmp[length + 1];
		_memset(buffer_tmp, sizeof(buffer_tmp));
		int retval = NONE;
		retval = ps3mapi_get_memory(pid, address, buffer_tmp, length);
		if(0 <= retval)
		{
			// show hex dump
			u8 byte = 0, p = 0;
			u16 num_bytes = MIN(0x200, ((u16)((length + 15) / 0x10) * 0x10));
			for(u16 i = 0, n = 0; i < num_bytes; i++)
			{
				if(!p)
				{
					sprintf(templn, "%08X  ", (int)((address & 0xFFFFFFFFULL) + i));
					buffer += concat(buffer, templn);
				}

				if(i >= length) buffer += concat(buffer, "&nbsp;&nbsp; ");
				else
				{
					if(hilite && (p == 0))
						 buffer += concat(buffer, "<font color=#ff0>");

					byte = (u8)buffer_tmp[i];

					sprintf(templn, "%02X ", byte); buffer += concat(buffer, templn);

					if(hilite && ((hilite == (p + 1)) || (p == 0xF)))
						 buffer += concat(buffer, "</font>");
				}

				if(p == 0xF)
				{
					buffer += concat(buffer, " ");
					if(hilite)
						 buffer += concat(buffer, "<font color=#ff0>");
					for(u8 c = 0; c < 0x10; c++, n++)
					{
						if(n >= length) break;
						byte = (u8)buffer_tmp[n];
						if(byte<32 || byte>=127) byte='.';

						if(byte==0x3C)
							buffer += concat(buffer, "&lt;");
						else if(byte==0x3E)
							buffer += concat(buffer, ">");
						else
							{sprintf(templn,"%c", byte); buffer += concat(buffer, templn);}
						if(hilite)
						{
							hilite--; if(!hilite) buffer += concat(buffer, "</font>");
						}
					}
					if(hilite) buffer += concat(buffer, "</font>");
					buffer += concat(buffer, "<br>");
				}

				p++; if(p >= 0x10) p=0;
			}
			//

			// add navigation with left/right keys
			add_html('k', 0, buffer, templn);
			concat(buffer,  "<textarea id=\"output\" style=\"display:none\">");

			for(int i = 0; i < length; i++)
			{
				sprintf(templn, "%02X", (u8)buffer_tmp[i]);
				concat(buffer, templn);
			}
		}
		else {sprintf(templn, "%s: %i", "Error", retval); concat(buffer, templn);}
		concat(buffer, "</textarea>");
	}

	concat(buffer, "<br>");

#ifdef DEBUG_MEM
	concat(buffer, "Dump: [<a href=\"/dump.ps3?mem\">Full Memory</a>] [<a href=\"/dump.ps3?flash\">Flash</a>] [<a href=\"/dump.ps3?rsx\">RSX</a>] [<a href=\"/dump.ps3?vsh\">VSH</a>] [<a href=\"/dump.ps3?lv1\">LV1</a>] [<a href=\"/dump.ps3?lv2\">LV2</a>]");
	sprintf(templn, " [<a href=\"/dump.ps3?%x\">LV1 Dump 0x%x</a>] [<a href=\"/peek.lv1?%x\">LV1 Peek</a>] [<a href=\"/peek.lv2?%x\">LV2 Peek</a>]", address, address, address, address); concat(buffer, templn + (address ? 0 : 41));
#endif
#ifdef ARTEMIS_PRX
	concat(buffer, " [<a href=\"/artemis.ps3\">Artemis</a>]");
#endif

	concat(buffer, "<p>");

	if(is_read_only) return;

	if(!is_ps3mapi_home && islike(param, "/getmem.ps3mapi")) ps3mapi_setmem(buffer, templn, param);
}

static void ps3mapi_setmem(char *buffer, char *templn, const char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	u8 oper = 0; // replace value
	u32 pid = 0;
	u32 address = found_offset;
	int length = 0;
	char value[BINDATA_SIZE + 1]; _memset(value, BINDATA_SIZE + 1);
	char val_tmp[HEXDATA_SIZE + 1]; _memset(val_tmp, HEXDATA_SIZE + 1);

	if(strstr(param, ".ps3mapi?"))
	{
		char addr_tmp[17];
		if(get_param("addr=", addr_tmp, param, 16))
		{
			address = (u32)convertH(addr_tmp);

			char *val_param = strstr(param, "&val");
			if(val_param)
			{
				oper = get_operator(val_param, false); // val=, val|=, val&=, val^=, val+=, val-=
				if(!oper && get_param("oper=", addr_tmp, param, 4)) oper = (u8)val(addr_tmp);

				if(get_param("=", val_tmp, val_param, HEXDATA_SIZE))
				{
					if(isHEX(val_tmp))
						length = Hex2Bin(val_tmp, value);
					else
						length = sprintf(value, "%s", val_tmp);
				}
			}
		}
		else
			pid = get_current_pid();

		if(!pid) pid = get_valuen32(param, "proc=");
	}

	if(found_offset) address = found_offset; found_offset = 0;

	if(!is_ps3mapi_home && islike(param, "/setmem.ps3mapi")) ps3mapi_getmem(buffer, templn, param);

	if((pid >= FLASH) && (pid < PID)) return; // read-only

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s%s</b>"
						HTML_BLU_SEPARATOR,
						HOME_PS3MAPI, "Set process memory");
	else
		sprintf(templn, "<u>%s:</u>", "Set process memory");

	concat(buffer, templn);

	sprintf(templn, HTML_FORM_METHOD_FMT("/setmem"), HTML_FORM_METHOD); concat(buffer, templn);

	//add_proc_list(buffer, templn, &pid, 2);

	sprintf(templn, "<b><u>%s:</u></b> "  HTML_INPUT("addr", "%X", "16", "18"), "Address", address); concat(buffer, templn);

	add_html('o', oper, buffer, templn);

	sprintf(templn, "<br><table width=\"800\">"
					"<tr><td class=\"la\">"
					"<textarea accesskey=\"v\" id=\"val\" name=\"val\" cols=\"103\" rows=\"5\" maxlength=\"" HEXDATA_LEN "\">%s</textarea></tr>"
					"<tr><td class=\"ra\"><br>"
					"<input class=\"bs\" type=\"submit\" accesskey=\"s\" value=\" %s \"/>%s", val_tmp, "Set", "</tr></table></form>");
	concat(buffer, templn);

	if(pid && length && oper)
	{
		int retval = ps3mapi_patch_process(pid, address, value, length, oper);

		if(retval < 0)
			sprintf(templn, "<br><b><u>%s: %i</u></b>", "Error", retval);
		else
			sprintf(templn, "<br><b><u>%s!</u></b>", "Done");
		concat(buffer, templn);
	}

	if(length == 0) concat(buffer, "<script>val.value=output.value</script>");

	if(!is_ps3mapi_home) concat(buffer, "<br>" HTML_RED_SEPARATOR); else concat(buffer, "<br>");
}

static void ps3mapi_payload(char *buffer, char *templn, const char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	u32 pid = 0;

	char payload[STD_PATH_LEN]; strcpy(payload, "/dev_hdd0/payload.bin");

	if(strstr(param, ".ps3mapi?"))
	{
		if(get_param("val=", payload, param, STD_PATH_LEN))
		{
			pid = get_current_pid();
		}
		if(!pid) pid = get_valuen32(param, "proc=");
	}
	if(!pid)
		pid = get_current_pid();

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s%s</b>"
						HTML_BLU_SEPARATOR,
						HOME_PS3MAPI, "Set process payload");
	else
		sprintf(templn, "<u>%s:</u>", "Set process payload");
	
	concat(buffer, templn);

	sprintf(templn, HTML_FORM_METHOD_FMT("/payload"), HTML_FORM_METHOD); concat(buffer, templn);

	if(!is_ps3mapi_home)
		add_proc_list(buffer, templn, &pid, 3);

	sprintf(templn, "Payload path:" 
					HTML_INPUT("val", "%s", "260", "80")
					" <input class=\"bs\" type=\"submit\" accesskey=\"s\" value=\" %s \"/>%s", payload, "Load", "</tr></table></form>");
	concat(buffer, templn);

	if(strstr(param, ".ps3mapi?"))
	{
		if(!pid)
			sprintf(templn, "<br><b>%s %s %s</b> (pid=0x%X)", STR_ERROR, "Process", "Not found!", pid);
		else if(file_exists(payload))
		{
			char error_msg[64];
			uint64_t pageTable[2];
			uint64_t executableMemoryAddress = StartGamePayload(pid, payload, 0x7D0, 0x4000, pageTable, error_msg);

			sprintf(templn, "<br><b>%s %s %s</b> (pid=0x%X)", payload, executableMemoryAddress ? STR_LOADED : STR_ERROR,
															  error_msg, pid);
		}
		else
			sprintf(templn, "<br><b>%s %s %s</b> (pid=0x%X)", STR_ERROR, payload, "Not found!", pid);
		
		concat(buffer, templn);
	}

	if(!is_ps3mapi_home) concat(buffer, "<br>" HTML_RED_SEPARATOR); else concat(buffer, "<br>");
}

static void ps3mapi_setidps(char *buffer, char *templn, const char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	//{system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_IDPS, (u64)IDPS);}
	//{system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PSID, (u64)PSID);}

	u64 _new_IDPS[2] = { IDPS[0], IDPS[1]};
	u64 _new_PSID[2] = { PSID[0], PSID[1]};

	if(islike(param, "/setidps.ps3mapi") && param[16] == '?')
	{
		char tmp_value[17];
		if(get_param("idps1=", tmp_value, param, 16))
		{
			_new_IDPS[0] = convertH(tmp_value);

			if(get_param("idps2=", tmp_value, param, 16))
			{
				_new_IDPS[1] = convertH(tmp_value);

				{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_IDPS, (u64)_new_IDPS[0], (u64)_new_IDPS[1]);}
			}
		}

		if(get_param("psid1=", tmp_value, param, 16))
		{
			_new_PSID[0] = convertH(tmp_value);

			if(get_param("psid2=", tmp_value, param, 16))
			{
				_new_PSID[1] = convertH(tmp_value);

				{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PSID, (u64)_new_PSID[0], (u64)_new_PSID[1]);}
			}
		}
	}

	get_idps_psid();

	sprintf(templn, "<b>%s%s</b>"
					HTML_BLU_SEPARATOR
					HTML_FORM_METHOD_FMT("/setidps")
					"<table id='ht' width=\"800\">"
					"<tr><td width=\"400\" class=\"la\">"
					"<br><b><u>%s:</u></b><br>"
					HTML_INPUT("idps1", "%016llX", "16", "18") HTML_INPUT("idps2", "%016llX", "16", "18")
					"<td class=\"la\">"
					"<br><b><u>%s:</u></b><br>"
					HTML_INPUT("psid1", "%016llX", "16", "18") HTML_INPUT("psid2", "%016llX", "16", "18") "</tr>"
					"<tr><td class=\"ra\"><br><button class=\"bs\">%s</button>",
					is_ps3mapi_home ? "" : HOME_PS3MAPI, "Set IDPS/PSID",
					HTML_FORM_METHOD, "IDPS", _new_IDPS[0], _new_IDPS[1], "PSID", _new_PSID[0], _new_PSID[1], "Set");

	concat(buffer, templn);

	concat(buffer,	"<style>.ht{-webkit-text-security:disc}</style>"
					"<script>var t='th';function h(){var e=document.getElementById('ht').getElementsByTagName('INPUT');t=t.split('').reverse().join('');for(var n=0;n<e.length;n++)e[n].className=t;}h();</script>"
					" <button onclick='h();return false;'>&#x1F453;</button>"
					"</tr>"
					"</table></form><br>");

	if(!is_ps3mapi_home) concat(buffer,	HTML_RED_SEPARATOR);
}

static void add_plugins_list(char *buffer, char *templn, u8 is_vsh)
{
	if(!strstr(buffer, "<datalist id=\"plugins\">"))
	{
		concat(buffer, "<div style=\"display:none\"><datalist id=\"plugins\">");
		int fd, cnt = 0;

		const char *vsh_modules[3] = {
									"/dev_flash/vsh/module",
									"/dev_flash/sys/internal",
									"/dev_flash/sys/external"
								};

		const char *paths[5] = {
									"/dev_hdd0/plugins",
									"/dev_hdd0/plugins/ps3xpad",
									"/dev_hdd0/plugins/ps3_menu",
									"/dev_hdd0/plugins/PS3Lock",
									WM_RES_PATH
								};

		#define PLUGINS_PATH	is_vsh ? vsh_modules[i] : (i < 3) ? drives[i] : paths[i - 3]

		const char *ext = ".sprx"; u8 count = is_vsh ? 3 : 8, ext_len = 5;

		if(is_vsh == 99) {count = 1, vsh_modules[0] = "/dev_hdd0/plugins/kernel", ext = ".bin", ext_len = 4;}

		for(u8 i = 0; i < count; i++)
			if(cellFsOpendir(PLUGINS_PATH, &fd) == CELL_FS_SUCCEEDED)
			{
				CellFsDirectoryEntry dir; u32 read_e;
				const char *entry_name = dir.entry_name.d_name;

				u16 plen = sprintf(templn, "<option>%s/", PLUGINS_PATH);

				while(working && (!cellFsGetDirectoryEntries(fd, &dir, sizeof(dir), &read_e) && read_e))
				{
					if(!extcmp(entry_name, ext, ext_len))
					{
						sprintf(templn + plen, "%s", entry_name); buffer += concat(buffer, templn); if(++cnt > 450) break;
					}
				}
				cellFsClosedir(fd);
			}

		concat(buffer, "</datalist></div>");
	}
}

static void ps3mapi_vshplugin(char *buffer, char *templn, const char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	char tmp_name[32];
	char tmp_filename[STD_PATH_LEN];

	if(islike(param, "/vshplugin.ps3mapi") && param[18] == '?')
	{
		unsigned int uslot = 99;

		const char *pos = strstr(param + 18, "?s=");
		if(pos)
		{
			u8 boot_mode = get_valuen(pos, "?s=", 0, 4);
			sprintf(tmp_filename, "/dev_hdd0/boot_plugins.txt"); if(dex_mode) sprintf(tmp_filename + 22, "_dex.txt");
			switch (boot_mode)
			{
				case 1: sprintf(tmp_filename + 10, "mamba_plugins.txt"); break;
				case 2: sprintf(tmp_filename + 10, "prx_plugins.txt");   break;
				case 3: sprintf(tmp_filename + 10, "game/PRXLOADER/USRDIR/plugins.txt"); break;
				case 4: sprintf(tmp_filename + 22, "_nocobra.txt"); if(dex_mode) sprintf(tmp_filename + 30, "_dex.txt"); break;
			}

			sprintf(templn, "<p><a href=\"%s\" style=\"padding:8px;background:#900;border-radius:8px;\">%s</a><p>", tmp_filename, tmp_filename); concat(buffer, templn);

			create_file(tmp_filename);
			for (unsigned int slot = 1; slot < 7; slot++)
			{
				ps3mapi_get_vsh_plugin_info(slot, tmp_name, templn);

				if(*templn)
				{
					save_file(tmp_filename, templn, APPEND_TEXT);
				}
			}
		}
		else
		{
			uslot = get_valuen(param, "load_slot=", 0, 6);

			if(strstr(param, "unload_slot="))
			{
				ps3mapi_check_unload(uslot, tmp_name, tmp_filename);

				if ( uslot )
				{
					system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_UNLOAD_VSH_PLUGIN, (u64)uslot);
				}
			}
			else
			{
				char *prx_path = tmp_filename;
				if(get_param("prx=", prx_path, param, STD_PATH_LEN))
				{
					check_path_alias(prx_path);
					if (!uslot ) uslot = get_free_slot(); // find free slot if slot == 0
					if ( uslot ) {system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_LOAD_VSH_PLUGIN, (u64)uslot, (u64)(u32)prx_path, NULL, 0);}
					#ifdef FPS_OVERLAY
					if(strstr(prx_path, "/VshFpsCounter")) {overlay_enabled = 1, overlay_info = 0;}
					#endif
				}
			}
		}
	}

	sprintf(templn, "<b>%s%s</b>"
					HTML_BLU_SEPARATOR "<br>"
					"<table>"
					"<tr><td width=\"75\" class=\"la\">%s"
					"<td width=\"120\" class=\"la\">%s"
					"<td width=\"500\" class=\"la\">%s"
					"<td width=\"125\" class=\"ra\"> </tr>",
					is_ps3mapi_home ? "" : HOME_PS3MAPI, "VSH Plugins", "Slot", "Name", "File name");

	buffer += concat(buffer, templn);
	for (unsigned int slot = 0; slot < 7; slot++)
	{
		ps3mapi_get_vsh_plugin_info(slot, tmp_name, tmp_filename);

		if(*tmp_filename)
		{
			sprintf(templn, "<tr><td width=\"75\" class=\"la\">%i"
							"<td width=\"120\" class=\"la\">%s"
							"<td width=\"500\" class=\"la\">",
							slot, tmp_name); buffer += concat(buffer, templn);

			buffer += add_breadcrumb_trail(buffer, tmp_filename);

			sprintf(templn, "<td width=\"100\" class=\"ra\">"
							HTML_FORM_METHOD_FMT("/vshplugin")
							"<input name=\"unload_slot\" type=\"hidden\" value=\"%i\">",
							HTML_FORM_METHOD, slot);
		}
		else
 		{
			sprintf(templn, "<tr><td width=\"75\" class=\"la\">%i"
							"<td width=\"120\" class=\"la\">%s"
							HTML_FORM_METHOD_FMT("/vshplugin")
							"<td width=\"500\" class=\"la\">"
							HTML_INPUT("prx\" style=\"width:555px\" list=\"plugins", "", "128", "75")
							"<input name=\"load_slot\" type=\"hidden\" value=\"%i\">"
							"<td width=\"100\" class=\"ra\">",
							slot, "NULL", HTML_FORM_METHOD, slot);
		}
		buffer += concat(buffer, templn);
		if(*tmp_filename)
			sprintf(templn, "<input type=\"submit\" value=%s/></form></tr>", (slot) ? "\" Unload \"" : "\" Reserved \" disabled");
		else
			sprintf(templn, "<input type=\"submit\" value=%s/></form></tr>", (slot) ? "\" Load \"" : "\" Reserved \" disabled");
		buffer += concat(buffer, templn);
	}

	sprintf(templn, "<tr><td colspan=4><p>%s > "	HTML_BUTTON_FMT
													HTML_BUTTON_FMT
													HTML_BUTTON_FMT
													HTML_BUTTON_FMT
													HTML_BUTTON_FMT "</tr>", STR_SAVE,
		HTML_BUTTON, "boot_plugins.txt",				HTML_ONCLICK, "/vshplugin.ps3mapi?s=0",
		HTML_BUTTON, dex_mode ?
					"boot_plugins_nocobra_dex.txt" :
					"boot_plugins_nocobra.txt",		HTML_ONCLICK, "/vshplugin.ps3mapi?s=4",
		HTML_BUTTON, "mamba_plugins.txt",			HTML_ONCLICK, "/vshplugin.ps3mapi?s=1",
		HTML_BUTTON, "prx_plugins.txt",				HTML_ONCLICK, "/vshplugin.ps3mapi?s=2",
		HTML_BUTTON, "plugins.txt",					HTML_ONCLICK, "/vshplugin.ps3mapi?s=3"); concat(buffer, templn);

	add_plugins_list(buffer, templn, 0);

	sprintf(templn, "</table><br>");

	if(!is_ps3mapi_home) strcat(templn, HTML_RED_SEPARATOR);
	concat(buffer, templn);
}

static void ps3mapi_kernelplugin(char *buffer, char *templn, const char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	char tmp_name[48];
	char tmp_filename[STD_PATH_LEN];

	if(islike(param, "/kernelplugin.ps3mapi") && param[21] == '?')
	{
		unsigned int uslot = 0;

		const char *pos = strstr(param + 18, "?s=");
		if(pos)
		{
			u8 boot_mode = get_valuen(pos, "?s=", 0, 4);
			sprintf(tmp_name, "/dev_hdd0/boot_plugins_kernel.txt");
			if(boot_mode)
			{
				sprintf(tmp_name + 29, dex_mode ? "_nocobra_dex.txt" : "_nocobra.txt");
			}

			sprintf(templn, WMTMP "/kernel%i.txt", uslot);
			read_file(templn, tmp_filename, STD_PATH_LEN, 0);
			save_file(tmp_name, tmp_filename, SAVE_ALL);

			sprintf(templn, "<p><a href=\"%s\" style=\"padding:8px;background:#900;border-radius:8px;\">%s</a><p>", tmp_name, tmp_name); concat(buffer, templn);
		}
		else
		{
			uslot = get_valuen(param, "load_slot=", 0, 1);

			if(strstr(param, "unload_slot="))
			{
				if ( uslot )
					{system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_UNLOAD_PAYLOAD_DYNAMIC, (u64)residence); residence = 0;}
			}
			else
			{
				char *prx_path = tmp_filename;
				if(get_param("prx=", prx_path, param, STD_PATH_LEN))
				{
					check_path_alias(prx_path);
					size_t size = file_size(prx_path);

					sys_addr_t payload = sys_mem_allocate(_64KB_ + (int)((size - 1) / _64KB_));
					if (read_file(prx_path, (char*)payload, size, 0))
					{
						if (size < 4) { BEEP3 }
						else if (uslot)
						{
							BEEP2; system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_RUN_PAYLOAD_DYNAMIC, (u64)(u32)payload, size, (u64)&residence);
						}
						else
						{
							BEEP1; system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_RUN_PAYLOAD, (u64)(u32)payload,	size);
						}
					}

					if (payload)
						sys_memory_free(payload);

				}
				sprintf(tmp_name, WMTMP "/kernel%i.txt", uslot);
				save_file(tmp_name, prx_path, SAVE_ALL);
			}
		}
	}

	sprintf(templn, "<b>%s%s</b>"
					HTML_BLU_SEPARATOR "<br>"
					"<table>"
					"<tr><td width=\"75\" class=\"la\">%s"
					"<td width=\"120\" class=\"la\">%s"
					"<td width=\"500\" class=\"la\">%s"
					"<td width=\"125\" class=\"ra\"> </tr>",
					is_ps3mapi_home ? "" : HOME_PS3MAPI, "Kernel Plugins", "Slot", "Name", "File name");

	buffer += concat(buffer, templn);
	for (unsigned int slot = 0; slot < 2; slot++)
	{
		if(!slot)
		{
			if(!cobra_version)
				sprintf(templn, dex_mode ?  "boot_plugins_kernel_nocobra_dex.txt" :
											"boot_plugins_kernel_nocobra.txt");
			else
				sprintf(templn, "boot_plugins_kernel.txt");
			read_file(templn, tmp_filename, STD_PATH_LEN, 0);
			if(*tmp_filename == 0)
				sprintf(templn, WMTMP "/kernel%i.txt", slot);
		}
		else
			sprintf(templn, WMTMP "/kernel%i.txt", slot);
		read_file(templn, tmp_filename, STD_PATH_LEN, 0);

		strcpy(tmp_name, slot ? "DYNAMIC" : "FIXED");

		sprintf(templn, "<tr><td width=\"75\" class=\"la\">%i"
						"<td width=\"120\" class=\"la\">%s"
						HTML_FORM_METHOD_FMT("/kernelplugin")
						"<td width=\"500\" class=\"la\">"
						HTML_INPUT("prx\" style=\"width:555px\" list=\"plugins", "%s", "128", "75")
						"<input name=\"%s_slot\" type=\"hidden\" value=\"%i\">"
						"<td width=\"100\" class=\"ra\">",
						slot, tmp_name, HTML_FORM_METHOD, tmp_filename, residence ? "unload" : "load", slot);
		buffer += concat(buffer, templn);

		if(slot)
		{
			sprintf(templn, " &nbsp; " HTML_INPUT("addr", "%08x", "8", "8\" disabled=\"disabled") "<br>", (u32)residence);
			buffer += concat(buffer, templn);
		}

		if(residence && slot)
			sprintf(templn, "<input type=\"submit\" value=%s/></form></tr>", "\" Unload \"");
		else
			sprintf(templn, "<input type=\"submit\" value=%s/></form></tr>", "\" Load \"");
		buffer += concat(buffer, templn);
	}

	sprintf(templn, "<tr><td colspan=4><p>%s > "	HTML_BUTTON_FMT
													HTML_BUTTON_FMT "</tr>", STR_SAVE,
		HTML_BUTTON, "boot_plugins_kernel.txt",		HTML_ONCLICK, "/kernelplugin.ps3mapi?s=0",
		HTML_BUTTON, dex_mode ?
				"boot_plugins_kernel_nocobra_dex.txt" :
				"boot_plugins_kernel_nocobra.txt",  HTML_ONCLICK, "/kernelplugin.ps3mapi?s=1");
	concat(buffer, templn);

	add_plugins_list(buffer, templn, 99);

	sprintf(templn, "</table><br>");

	if(!is_ps3mapi_home) strcat(templn, HTML_RED_SEPARATOR);
	concat(buffer, templn);
}

static sys_prx_id_t load_start(const char *path)
{
	int modres, res;
	sys_prx_id_t id;
	id = sys_prx_load_module(path, 0, NULL);
	if (id < CELL_OK)
	{
		BEEP3;
		return id;
	}
	res = sys_prx_start_module(id, 0, NULL, &modres, 0, NULL);
	if (res < CELL_OK)
	{
		BEEP3;
		return res;
	}
	else
	{
		BEEP1;
		return id;
	}
}

static sys_prx_id_t stop_unload(sys_prx_id_t id)
{
	int modres, res;
	res = sys_prx_stop_module(id, 0, NULL, &modres, 0, NULL);
	if (res < CELL_OK)
	{
		BEEP3;
		return res;
	}
	res = sys_prx_unload_module(id, 0, NULL);
	if (res < CELL_OK)
	{
		BEEP3;
		return res;
	}
	else
	{
		BEEP1;
		return id;
	}
}

static void ps3mapi_gameplugin(char *buffer, char *templn, const char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	u32 pid = 0;

	if(islike(param, "/gameplugin.ps3mapi") && param[19] == '?')
	{
		pid = get_valuen32(param, "proc=");
		if(pid)
		{
			if(pid <= LV2) {ps3mapi_vshplugin(buffer, templn, param); return;}

			char *pos = strstr(param, "unload_slot=");
			if(pos)
			{
				unsigned int prx_id = get_valuen32(pos, "unload_slot=");
				unsigned int sys = get_valuen32(param, "sys=");
				if(sys >= 2)
				{
					wwwd_stop();
					sys_ppu_thread_exit(0);
				}
				else if(sys)
					stop_unload(prx_id); // <- unload system modules
				else
					{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_UNLOAD_PROC_MODULE, (u64)pid, (u64)prx_id); }
			}
			else
			{
				char prx_path[STD_PATH_LEN];
				if(get_param("prx=", prx_path, param, STD_PATH_LEN))
				{
					check_path_alias(prx_path);
					if(strstr(prx_path, "/dev_flash"))
						load_start(prx_path); // <- load system modules from flash to process
					else
						{system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LOAD_PROC_MODULE, (u64)pid, (u64)(u32)prx_path, NULL, 0); } // <- load custom modules to process
				}
			}
		}
	}

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s%s</b>"
						HTML_BLU_SEPARATOR,
						HOME_PS3MAPI, "Game Plugins");
	else
		sprintf(templn, "<b>%s</b>"
						HTML_BLU_SEPARATOR,
						"Game Plugins");

	concat(buffer, templn);

	sprintf(templn, HTML_FORM_METHOD_FMT("/gameplugin")
					"<b><u>%s:</u></b>  ", HTML_FORM_METHOD, "Process"); concat(buffer, templn);

	_memset(templn, MAX_LINE_LEN);

	u8 is_vsh = add_proc_list(buffer, templn, &pid, 3);

	sprintf(templn, "</form>");

	concat(buffer, templn);

	if(pid)
	{
		sprintf(templn,
					"<table>"
					 "<tr>"
					  "<td width=\"75\" class=\"la\">%s"
					  "<td width=\"300\" class=\"la\">%s"
					  "<td width=\"500\" class=\"la\">%s"
					  "<td width=\"125\" class=\"ra\"> "
					 "</tr>",
					"Slot", "Name", "File name"); buffer += concat(buffer, templn);

		char tmp_name[32];
		char tmp_filename[STD_PATH_LEN];

		#define MAX_SLOTS	61
		u32 mod_list[MAX_SLOTS]; _memset(mod_list, sizeof(mod_list));
		{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_MODULE_PID, (u64)pid, (u64)(u32)mod_list);}

		for(unsigned int slot = 0; slot < MAX_SLOTS; slot++)
		{
			_memset(tmp_name, sizeof(tmp_name));
			_memset(tmp_filename, sizeof(tmp_filename));
			if(1 < mod_list[slot])
			{
				{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_NAME, (u64)pid, (u64)mod_list[slot], (u64)(u32)tmp_name);}
				{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_FILENAME, (u64)pid, (u64)mod_list[slot], (u64)(u32)tmp_filename);}
			}
			if(*tmp_filename)
			{
				sprintf(templn,
						"<tr>"
						 "<td width=\"75\" class=\"la\">%i"
						 "<td width=\"300\" class=\"la\">%s"
						 "<td width=\"500\" class=\"la\">",
						slot, tmp_name); buffer += concat(buffer, templn);

				buffer += add_breadcrumb_trail(buffer, tmp_filename);

				sprintf(templn, ""
						 "<td width=\"100\" class=\"ra\">"
						  HTML_FORM_METHOD_FMT("/gameplugin")
						  "<input name=\"proc\" type=\"hidden\" value=\"0x%x\">"
						  "<input name=\"unload_slot\" type=\"hidden\" value=\"0x%x\">"
						  "<input name=\"sys\" type=\"hidden\" value=\"%u\">"
						  "<input type=\"submit\" value=\" Unload \" title=\"id=0x%x\">"
						  "</form>"
						 ""
						"</tr>", HTML_FORM_METHOD, pid, mod_list[slot], islike(tmp_filename, "/dev_flash") | IS(tmp_name, "WWWD")<<1, mod_list[slot]);
			}
			else
			{
				sprintf(tmp_name, "NULL");
				//sprintf(tmp_filename, "/dev_hdd0/tmp/my_plugin_%i.sprx", slot);
				sprintf(templn,
						"<tr>"
						 "<td width=\"75\" class=\"la\">%i"
						 "<td width=\"300\" class=\"la\">%s"
						 "<td width=\"100\" class=\"ra\">"
						  HTML_FORM_METHOD_FMT("/gameplugin")
						   "<td width=\"500\" class=\"la\">"
							 "<input name=\"proc\" type=\"hidden\" value=\"0x%x\">"
							 HTML_INPUT("prx\" list=\"plugins", "", "128", "75")
							 "<input name=\"load_slot\" type=\"hidden\" value=\"%i\">"
							 "<input type=\"submit\" value=\" Load \">"
						  "</form>"
						"</tr>",
						slot, tmp_name,
						HTML_FORM_METHOD, pid, slot);
			}
			buffer += concat(buffer, templn);
		}

		add_plugins_list(buffer, templn, is_vsh);
	}

	sprintf(templn, "</table><br>");

	if(!is_ps3mapi_home) strcat(templn, HTML_RED_SEPARATOR);
	concat(buffer, templn);
}

static void ps3mapi_home(char *buffer, char *templn)
{
	int syscall8_state = NONE;
	{system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PCHECK_SYSCALL8); syscall8_state = (int)p1;}
	int core_version = 0;
	if(syscall8_state>=0) {system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_VERSION); core_version = (int)(p1);}
	int versionfw = 0;
	if(syscall8_state>=0) {system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_FW_VERSION); versionfw = (int)(p1);}
	char fwtype[32]; _memset(fwtype, 32);
	if(syscall8_state>=0) {system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_FW_TYPE, (u64)(u32)fwtype);}

	if(!versionfw)
		syscall8_state = NONE;

	syscalls_removed = CFW_SYSCALLS_REMOVED(TOC);
	if(syscalls_removed)
	{
		sprintf(templn, "<h1>%s</h1>%s", STR_CFWSYSALRD, HTML_RED_SEPARATOR); concat(buffer, templn);
	}
	else if(!cobra_version)
	{
		sprintf(templn, "<h1>%s %s</h1>%s", "Cobra", STR_DISABLED, HTML_RED_SEPARATOR); concat(buffer, templn);
	}

	//---------------------------------------------
	//PS3 Commands---------------------------------
	//---------------------------------------------
	sprintf(templn, "<b>%s</b>"
					HTML_BLU_SEPARATOR
					"<table width=\"800\"><tr>", "PS3 Commands");
	concat(buffer, templn);

	//RingBuzzer
	ps3mapi_buzzer(buffer, templn, " ");

	//LedRed
	ps3mapi_led(buffer, templn, " ");

	//Notify
	ps3mapi_notify(buffer, templn, " ");

	if (syscall8_state >= 0 && syscall8_state < 3)
	{
		//---------------------------------------------
		//Map path
		//---------------------------------------------
		ps3mapi_mappath(buffer, templn, " ");

		//---------------------------------------------
		//Process Commands
		//---------------------------------------------
		//GetMem
		ps3mapi_getmem(buffer, templn, " ");

		//SetMem
		//ps3mapi_setmem(buffer, templn, " ");

		ps3mapi_payload(buffer, templn, " ");
	
		//---------------------------------------------
		//Game Plugin
		//---------------------------------------------
		ps3mapi_gameplugin(buffer, templn, " ");

		//---------------------------------------------
		//VSH Plugin
		//---------------------------------------------
		ps3mapi_vshplugin(buffer, templn, " ");

		//---------------------------------------------
		//Kernel Plugin
		//---------------------------------------------
		ps3mapi_kernelplugin(buffer, templn, " ");

		//---------------------------------------------
		//IDPS/PSID
		//---------------------------------------------
		if(core_version >= 0x0120)
		{
			ps3mapi_setidps(buffer, templn, " ");
		}

		//---------------------------------------------
		//CFW Syscall
		//---------------------------------------------
		ps3mapi_syscall(buffer, templn, " ");
		//CFW Syscall8
		ps3mapi_syscall8(buffer, templn, " ");

		sprintf(templn, HTML_RED_SEPARATOR
						"%s: %X %s | PS3MAPI: webUI v%X, Server v%X, Core v%X | By NzV, modified by OsirisX", STR_FIRMWARE, versionfw, fwtype, PS3MAPI_WEBUI_VERSION, PS3MAPI_SERVER_VERSION, core_version);
		concat(buffer, templn);
	}
	else
	{
		//CFW Syscall8
		ps3mapi_syscall8(buffer, templn, " ");

		sprintf(templn, "</table><br>" HTML_RED_SEPARATOR
						"[SYSCALL8 %sDISABLED] | PS3MAPI: webUI v%X, Server v%X | By NzV", (syscall8_state == 3) ? "PARTIALLY ":"", PS3MAPI_WEBUI_VERSION, PS3MAPI_SERVER_VERSION);
		concat(buffer, templn);
	}
}
#endif // #ifdef PS3MAPI

////////////////////////////////////////
///////////// PS3MAPI END //////////////
////////////////////////////////////////
