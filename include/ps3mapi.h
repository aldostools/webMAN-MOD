///////////// PS3MAPI BEGIN //////////////

#define SYSCALL8_OPCODE_PS3MAPI						0x7777

#define PS3MAPI_SERVER_VERSION						0x0120
#define PS3MAPI_SERVER_MINVERSION					0x0120

#define PS3MAPI_WEBUI_VERSION						0x0121
#define PS3MAPI_WEBUI_MINVERSION					0x0120

#define PS3MAPI_CORE_MINVERSION						0x0111

#define PS3MAPI_OPCODE_GET_CORE_VERSION				0x0011
#define PS3MAPI_OPCODE_GET_CORE_MINVERSION			0x0012
#define PS3MAPI_OPCODE_GET_FW_TYPE					0x0013
#define PS3MAPI_OPCODE_GET_FW_VERSION				0x0014
#define PS3MAPI_OPCODE_GET_ALL_PROC_PID				0x0021
#define PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID			0x0022
#define PS3MAPI_OPCODE_GET_PROC_BY_PID				0x0023
#define PS3MAPI_OPCODE_GET_CURRENT_PROC				0x0024
#define PS3MAPI_OPCODE_GET_CURRENT_PROC_CRIT		0x0025
#define PS3MAPI_OPCODE_GET_PROC_MEM					0x0031
#define PS3MAPI_OPCODE_SET_PROC_MEM					0x0032
#define PS3MAPI_OPCODE_GET_ALL_PROC_MODULE_PID		0x0041
#define PS3MAPI_OPCODE_GET_PROC_MODULE_NAME			0x0042
#define PS3MAPI_OPCODE_GET_PROC_MODULE_FILENAME		0x0043
#define PS3MAPI_OPCODE_LOAD_PROC_MODULE				0x0044
#define PS3MAPI_OPCODE_UNLOAD_PROC_MODULE			0x0045
#define PS3MAPI_OPCODE_UNLOAD_VSH_PLUGIN			0x0046
#define PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO			0x0047
#define PS3MAPI_OPCODE_GET_IDPS 					0x0081
#define PS3MAPI_OPCODE_SET_IDPS 					0x0082
#define PS3MAPI_OPCODE_GET_PSID 					0x0083
#define PS3MAPI_OPCODE_SET_PSID						0x0084
#define PS3MAPI_OPCODE_CHECK_SYSCALL				0x0091
#define PS3MAPI_OPCODE_DISABLE_SYSCALL				0x0092
#define PS3MAPI_OPCODE_PDISABLE_SYSCALL8 			0x0093
#define PS3MAPI_OPCODE_PCHECK_SYSCALL8 				0x0094
#define PS3MAPI_OPCODE_RENABLE_SYSCALLS				0x0095
#define PS3MAPI_OPCODE_REMOVE_HOOK					0x0101

#define PS3MAPI_OPCODE_SUPPORT_SC8_PEEK_POKE		0x1000
//#define PS3MAPI_OPCODE_LV2_PEEK					0x1006
//#define PS3MAPI_OPCODE_LV2_POKE					0x1007
//#define PS3MAPI_OPCODE_LV1_PEEK					0x1008
//#define PS3MAPI_OPCODE_LV1_POKE					0x1009

#define PS3MAPI_OPCODE_SET_ACCESS_KEY				0x2000
#define PS3MAPI_OPCODE_REQUEST_ACCESS				0x2001

#define PS3MAPI_OPCODE_PHOTO_GUI					0x2222

#define PS3MAPI_FIND_FREE_SLOT						NULL

#define unload_vsh_plugin(a) get_vsh_plugin_slot_by_name(a, true)
#define get_free_slot(a)     get_vsh_plugin_slot_by_name(PS3MAPI_FIND_FREE_SLOT, false)

///////////// PS3MAPI END //////////////

#if defined(REMOVE_SYSCALLS) || defined(PS3MAPI)
#define CFW_SYSCALLS 16
static u16 sc_disable[CFW_SYSCALLS] = {200, 201, 202, 203, 204, 1022, 6, 7, 10, 11, 15, 20, 35, 36, 38, 9};
#else
#define CFW_SYSCALLS 17
static u16 sc_disable[CFW_SYSCALLS] = {200, 201, 202, 203, 204, 1022, 6, 7, 10, 11, 15, 20, 35, 36, 38, 8, 9};
#endif

#ifdef PS3MAPI

static void ps3mapi_buzzer(char *buffer, char *templn, char *param);
static void ps3mapi_led(char *buffer, char *templn, char *param);
static void ps3mapi_notify(char *buffer, char *templn, char *param);
static void ps3mapi_syscall(char *buffer, char *templn, char *param);
static void ps3mapi_syscall8(char *buffer, char *templn, char *param);
static void ps3mapi_setidps(char *buffer, char *templn, char *param);
static void ps3mapi_getmem(char *buffer, char *templn, char *param);
static void ps3mapi_setmem(char *buffer, char *templn, char *param);
static void ps3mapi_vshplugin(char *buffer, char *templn, char *param);
static void ps3mapi_gameplugin(char *buffer, char *templn, char *param);
static unsigned int get_vsh_plugin_slot_by_name(const char *name, bool unload);

static u8 ps3mapi_working = 0;

static int is_syscall_disabled(u32 sc)
{
	int ret_val = NONE;
	{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_CHECK_SYSCALL, sc); ret_val = (int)p1;}
	if(ret_val<0) {u64 sc_null = peekq(SYSCALL_TABLE); ret_val = (peekq(SYSCALL_PTR(sc)) == sc_null);}

	return ret_val;
}

static void ps3mapi_home(char *buffer, char *templn)
{
	int syscall8_state = NONE;
	{system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PCHECK_SYSCALL8); syscall8_state = (int)p1;}
	int core_version = 0;
	if(syscall8_state>=0) {system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_VERSION); core_version = (int)(p1);}
	int versionfw = 0;
	if(syscall8_state>=0) {system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_FW_VERSION); versionfw = (int)(p1);}
	char fwtype[32]; memset(fwtype, 0, 32);
	if(syscall8_state>=0) {system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_FW_TYPE, (u64)(u32)fwtype);}

	if(!versionfw)
		syscall8_state = NONE;

	//---------------------------------------------
	//PS3 Commands---------------------------------
	//---------------------------------------------
	sprintf(templn, "<b>%s</b>"
					HTML_BLU_SEPARATOR
					"<table width=\"800\"><tr>", "PS3 Commands");
	concat(buffer, templn);

	//RingBuzzer
	ps3mapi_buzzer(buffer, templn, (char*)" ");

	//LedRed
	ps3mapi_led(buffer, templn, (char*)" ");

	sprintf(templn, "%s", "</td>");
	concat(buffer, templn);

	//Notify
	ps3mapi_notify(buffer, templn, (char*)" ");

	if (syscall8_state >= 0 && syscall8_state < 3)
	{
		ps3mapi_syscall(buffer, templn, (char*)" ");
	}
	if (syscall8_state >= 0)
	{
		//Syscall8
		ps3mapi_syscall8(buffer, templn, (char*)" ");
	}
	if (syscall8_state >= 0 && syscall8_state < 3)
	{
		//IDPS/PSID
		if(core_version >= 0x0120)
		{
			ps3mapi_setidps(buffer, templn, (char*)" ");
		}
		else
		{
			sprintf(templn, "%s", "</table><br>");
			concat(buffer, templn);
		}
		//---------------------------------------------
		//Process Commands-----------------------------
		//---------------------------------------------
		//GetMem
		ps3mapi_getmem(buffer, templn, (char*)" ");

		//SetMem
		ps3mapi_setmem(buffer, templn, (char*)" ");

		//---------------------------------------------
		//VSH Plugin-----------------------------------
		//---------------------------------------------
		ps3mapi_vshplugin(buffer, templn, (char*)" ");

		//---------------------------------------------
		//Game Plugin-----------------------------------
		//---------------------------------------------
		ps3mapi_gameplugin(buffer, templn, (char*)" ");

		sprintf(templn, HTML_RED_SEPARATOR
						"Firmware: %X %s | PS3MAPI: webUI v%X, Server v%X, Core v%X | By NzV, modified by OsirisX", versionfw, fwtype, PS3MAPI_WEBUI_VERSION, PS3MAPI_SERVER_VERSION, core_version);
		concat(buffer, templn);
	}
	else
	{
		sprintf(templn, "</table><br>" HTML_RED_SEPARATOR
						"[SYSCALL8 %sDISABLED] | PS3MAPI: webUI v%X, Server v%X | By NzV", (syscall8_state == 3) ? "PARTIALLY ":"", PS3MAPI_WEBUI_VERSION, PS3MAPI_SERVER_VERSION);
		concat(buffer, templn);
	}
}

static void ps3mapi_buzzer(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	if(islike(param, "/buzzer.ps3mapi") && param[15] == '?')
	{
		u8 value = get_valuen(param, "mode=", 1, 3);

		if(value == 1) { BEEP1 }
		if(value == 2) { BEEP2 }
		if(value == 3) { BEEP3 }

	}

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s --> %s --> %s</b>"
						HTML_BLU_SEPARATOR,
						"<a href='/home.ps3mapi'>PS3MAPI</a>", "PS3 Commands", "Buzzer");
	else
		sprintf(templn, "<td width=\"260\" class=\"la\"><u>%s:</u><br>", "Buzzer");
	concat(buffer, templn);

	sprintf(templn, "<form id=\"buzzer\" action=\"/buzzer%s<br>"
					"<b>%s:</b>  <select name=\"mode\">", HTML_FORM_METHOD, "Mode");
	concat(buffer, templn);
	add_option_item(1, "Simple", IS_MARKED("mode=1"), buffer);
	add_option_item(2, "Double", IS_MARKED("mode=2"), buffer);
	add_option_item(3, "Triple", IS_MARKED("mode=3"), buffer);
	sprintf(templn, "</select>   <input type=\"submit\" value=\" %s \"/></td></form><br>", "Ring");

	if(!is_ps3mapi_home) strcat(templn, HTML_RED_SEPARATOR); else strcat(templn, "</td>");
	concat(buffer, templn);
}

static void ps3mapi_led(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	if(islike(param, "/led.ps3mapi") && param[12] == '?')
	{
		int color = get_valuen(param, "color=", 0, 2);

		if(color == 0) color = RED;	  else
		if(color == 1) color = GREEN; else
		if(color == 2) color = RED+GREEN; //YELLOW

		int mode = get_valuen(param, "mode=", 0, 3);

		if(mode == 1) mode = ON;		 else
		if(mode == 2) mode = BLINK_FAST; else
		if(mode == 3) mode = BLINK_SLOW;

		if(color & RED)   { system_call_2(SC_SYS_CONTROL_LED, RED  , mode); }
		if(color & GREEN) { system_call_2(SC_SYS_CONTROL_LED, GREEN, mode); }
	}

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s --> %s --> %s</b>"
						HTML_BLU_SEPARATOR,
						"<a href='/home.ps3mapi'>PS3MAPI</a>", "PS3 Commands", "Led");
	else
		sprintf(templn, "<td width=\"260\" class=\"la\"><u>%s:</u><br>", "Led");

	concat(buffer, templn);

	sprintf(templn, "<form id=\"led\" action=\"/led%s<br>"
					"<b>%s:</b>  <select name=\"color\">", HTML_FORM_METHOD,  "Color"); concat(buffer, templn);

	add_option_item(0, "Red",                IS_MARKED("color=0"), buffer);
	add_option_item(1, "Green",              IS_MARKED("color=1"), buffer);
	add_option_item(2, "Yellow (Red+Green)", IS_MARKED("color=2"), buffer);

	sprintf(templn, "</select>   <b>%s:</b>  <select name=\"mode\">", "Mode"); concat(buffer, templn);

	add_option_item(0, "Off",        IS_MARKED("mode=0"), buffer);
	add_option_item(1, "On",         IS_MARKED("mode=1"), buffer);
	add_option_item(2, "Blink fast", IS_MARKED("mode=2"), buffer);
	add_option_item(3, "Blink slow", IS_MARKED("mode=3"), buffer);

	sprintf(templn, "</select>   <input type=\"submit\" value=\" %s \"/></form><br>", "Set");
	if(!is_ps3mapi_home) strcat(templn, HTML_RED_SEPARATOR); else strcat(templn, "</table></td>");
	concat(buffer, templn);
}

static void ps3mapi_notify(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	char msg[200]; strcpy(msg, "Hello :)");
	if(get_param("?msg=", msg, param, 199))
	{
		show_msg(msg);
	}

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s --> %s --> %s</b>"
						HTML_BLU_SEPARATOR,
						"<a href='/home.ps3mapi'>PS3MAPI</a>", "PS3 Commands", "Notify");
	else
		sprintf(templn, "<tr><td class=\"la\"><br><u>%s:</u><br><br>", "Notify");

	concat(buffer, templn);

	sprintf(templn, "<form action=\"/notify" HTML_FORM_METHOD_FMT
					"<table width=\"800\"><tr><td class=\"la\">"
					"<textarea name=\"msg\" cols=\"111\" rows=\"2\" maxlength=\"199\">%s</textarea></td></tr>"
					"<tr><td class=\"ra\"><br><input class=\"bs\" type=\"submit\" value=\" %s \"/></td></tr></table></form>",
					HTML_FORM_METHOD, msg, "Send");

	if(!is_ps3mapi_home) strcat(templn, HTML_RED_SEPARATOR); else strcat(templn, "</td>");
	concat(buffer, templn);
}

static void ps3mapi_syscall(char *buffer, char *templn, char *param)
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
		sprintf(templn, "<b>%s --> %s --> %s</b>"
						HTML_BLU_SEPARATOR
						"<table width=\"800\">", "<a href='/home.ps3mapi'>PS3MAPI</a>", "PS3 Commands", "CFW syscall");
	else
		sprintf(templn, "<table width=\"800\">"
						"<tr><td class=\"la\"><u>%s:</u><br><br></td></tr>", "CFW syscall");

	concat(buffer, templn);

	sprintf(templn, "<form id=\"syscall\" action=\"/syscall%s"
					"<br><tr><td width=\"260\" class=\"la\">",
					HTML_FORM_METHOD); concat(buffer, templn);

	bool disabled; u8 sc_count = 0;

	#define checked		(!disabled)

	disabled = is_syscall_disabled(6);
	add_check_box("sc6", disabled, "[6]LV2 Peek", _BR_, checked, buffer); if(disabled) sc_count++;

	disabled = is_syscall_disabled(7);
	add_check_box("sc7", disabled, "[7]LV2 Poke", _BR_, checked, buffer); if(disabled) sc_count++;

	disabled = is_syscall_disabled(9);
	add_check_box("sc9", disabled, "[9]LV1 Poke", _BR_, checked, buffer); if(disabled) sc_count++;

	disabled = is_syscall_disabled(10);
	add_check_box("sc10", disabled, "[10]LV1 Call", _BR_, checked, buffer);

	disabled = is_syscall_disabled(15);
	add_check_box("sc15", disabled, "[15]LV2 Call", _BR_, checked, buffer);

	disabled = is_syscall_disabled(11);
	add_check_box("sc11", disabled, "[11]LV1 Peek", _BR_, checked, buffer);

	concat(buffer, "</td><td  width=\"260\"  valign=\"top\" class=\"la\">");

	disabled = is_syscall_disabled(35);
	add_check_box("sc35", disabled, "[35]Map Path", _BR_, checked, buffer);

	disabled = is_syscall_disabled(36);
	add_check_box("sc36", disabled, "[36]Map Game", _BR_, checked, buffer);

	disabled = is_syscall_disabled(38);
	add_check_box("sc38", disabled, "[38]New sk1e", _BR_, checked, buffer);

	disabled = is_syscall_disabled(1022);
	add_check_box("sc1022", disabled, "[1022]PRX Loader", _BR_, checked, buffer);

	concat(buffer, "</td><td  width=\"260\"  valign=\"top\" class=\"la\">");

	disabled = is_syscall_disabled(200);
	add_check_box("sc200", disabled, "[200]sys_dbg_read_process_memory", _BR_, checked, buffer);

	disabled = is_syscall_disabled(201);
	add_check_box("sc201", disabled, "[201]sys_dbg_write_process_memory", _BR_, checked, buffer);

	disabled = is_syscall_disabled(202);
	add_check_box("sc202", disabled, "[202]Free - Payloader3", _BR_, checked, buffer);

	disabled = is_syscall_disabled(203);
	add_check_box("sc203", disabled, "[203]LV2 Peek CCAPI", _BR_, checked, buffer);

	disabled = is_syscall_disabled(204);
	add_check_box("sc204", disabled, "[204]LV2 Poke CCAPI", _BR_, checked, buffer);

	#undef checked

#ifdef REMOVE_SYSCALLS
	concat(buffer, "<br>");
	if(sc_count) add_check_box("sce\" onclick=\"b.value=(this.checked)?' Enable ':'Disable';", false, "Re-Enable Syscalls & Unlock syscall 8", _BR_, false, buffer);
	else		 add_check_box("scd", false, "Disable Syscalls & Lock syscall 8"  , _BR_, false, buffer);
#endif

	sprintf(templn, "</td></tr><tr><td class=\"ra\"><br><input class=\"bs\" id=\"b\" type=\"submit\" value=\" %s \"/></td></tr></form></table><br>", "Disable");
	concat(buffer, templn);

	if(!is_ps3mapi_home && islike(param, "/syscall.ps3mapi")) {ps3mapi_syscall8(buffer, templn, param);}
}

static void ps3mapi_syscall8(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (*param == ' ');
	int ret_val = NONE;
	int disable_cobra = 0, disabled_cobra = 0;

	if(strstr(param, ".ps3mapi?"))
	{
		u8 mode = get_valuen(param, "mode=", 0, 5);

		{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_DISABLE_COBRA, (mode == 5)); }

		if( mode <= 3 ) { system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, (u64)mode); }
		if( mode == 4 ) { system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, SC_COBRA_SYSCALL8); }
		if( mode == 5 )	{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, 3); }
	}

	if(!is_ps3mapi_home && islike(param, "/syscall8.ps3mapi")) {ps3mapi_syscall(buffer, templn, param);}

	{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_DISABLE_COBRA, SYSCALL8_DISABLE_COBRA_CAPABILITY); disable_cobra = (int)p1;}
	if(disable_cobra == SYSCALL8_DISABLE_COBRA_OK)
	{
		{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_DISABLE_COBRA, SYSCALL8_DISABLE_COBRA_STATUS); disabled_cobra = (int)p1;}
		if(disabled_cobra) ret_val = 3;
	}

	sprintf(templn, "<b>%s%s --> %s</b>"
					HTML_BLU_SEPARATOR
					"<table width=\"800\">"
					"<form id=\"syscall8\" action=\"/syscall8%s"
					"<br><tr><td class=\"la\">",
					is_ps3mapi_home ? "" : "PS3MAPI --> ", "PS3 Commands", "CFW syscall 8", HTML_FORM_METHOD); concat(buffer, templn);

	{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PCHECK_SYSCALL8); ret_val = (int)p1;}

	syscalls_removed = (ret_val != 0); peek_lv1 = (syscalls_removed) ? lv1_peek_ps3mapi : lv1_peek_cfw;

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

	sprintf(templn, "</td></tr><tr><td class=\"ra\"><br><input class=\"bs\" type=\"submit\" value=\" %s \"/></td></tr></form></table><br>", "Set");

	if(!is_ps3mapi_home) strcat(templn, HTML_RED_SEPARATOR);
	concat(buffer, templn);
}

static void add_proc_list(char *buffer, char *templn, u32 *proc_id)
{
	u32 pid = *proc_id;

	if(pid == 0)
	{
		concat(buffer, "<select name=\"proc\">");
		u32 pid_list[16];
		{system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (u64)(u32)pid_list); }
		for(int i = 0; i < 16; i++)
		{
			if(1 < pid_list[i])
			{
				memset(templn, 0, MAX_LINE_LEN);
				{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID, (u64)pid_list[i], (u64)(u32)templn); }
				if(*templn) add_option_item(pid_list[i], templn, (i == 0), buffer);
			}
		}
		concat(buffer, "</select> ");
	}
	else
	{
		memset(templn, 0, MAX_LINE_LEN);
		{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID, (u64)pid, (u64)(u32)templn); }
		concat(buffer, templn);
		sprintf(templn, "<input name=\"proc\" type=\"hidden\" value=\"%u\"><br><br>", pid);
		concat(buffer, templn);

		add_game_info(buffer, templn, false);

		*proc_id = pid;
	}
}

static void ps3mapi_getmem(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	u32 pid = 0;
	u64 address = 0;
	int length = 8;

	if(strstr(param, ".ps3mapi?"))
	{
		char addr_tmp[17];
		if(get_param("addr=", addr_tmp, param, 16))
		{
			address = convertH(addr_tmp);

			length = (int)get_valuen32(param, "len=");
			if(length == 0) {char *pos = strstr(param, "val="); if(pos) length = strlen(pos + 4) / 2;}
			length = RANGE(length, 1, 2048);

			pid = get_valuen32(param, "proc=");
		}
	}

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s --> %s --> %s</b>"
						HTML_BLU_SEPARATOR,
						"<a href='/home.ps3mapi'>PS3MAPI</a>", "Processes Commands", "Get process memory");
	else
		sprintf(templn, "<b>%s</b>"
						HTML_BLU_SEPARATOR,
						"Processes Commands");

	concat(buffer, templn);

	sprintf(templn, "<form action=\"/getmem" HTML_FORM_METHOD_FMT
					"<b><u>%s:</u></b>  ", HTML_FORM_METHOD, "Process"); concat(buffer, templn); memset(templn, 0, MAX_LINE_LEN);

	add_proc_list(buffer, templn, &pid);

	sprintf(templn, "<b><u>%s:</u></b> " HTML_INPUT("addr", "%llX", "16", "18")
					"   <b><u>%s:</u></b> <input name=\"len\" type=\"number\" value=\"%i\" min=\"1\" max=\"2048\">"
					"   <input class=\"bs\" type=\"submit\" value=\" %s \"/></form>", "Address", address, "Length", length, "Get");
	concat(buffer, templn);

	if(pid != 0 && length != 0)
	{
		sprintf(templn, "<br><b><u>%s:</u></b><br>", "Output");
		concat(buffer, templn);
		char buffer_tmp[length + 1];
		memset(buffer_tmp, 0, sizeof(buffer_tmp));
		int retval = NONE;
		{system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MEM, (u64)pid, (u64)address, (u64)(u32)buffer_tmp, (u64)length); retval = (int)p1;}
		if(0 <= retval)
		{
			// show hex dump
			u8 byte = 0, p = 0;
			u16 num_bytes = MIN(0x200, ((u16)((length+15)/0x10)*0x10));
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
					byte = (u8)buffer_tmp[i];

					sprintf(templn, "%02X ", byte); buffer += concat(buffer, templn);
				}

				if(p == 0x7) buffer += concat(buffer, " ");

				if(p == 0xF)
				{
					buffer += concat(buffer, " ");
					for(u8 c = 0; c < 0x10; c++, n++)
					{
						if(n >= length) break;
						byte = (u8)buffer_tmp[n];
						if(byte<32 || byte>=127) byte='.';

						if(byte==0x3C)
							buffer += concat(buffer, "&lt;");
						else if(byte==0x3E)
							buffer += concat(buffer, "&gt;");
						else
							{sprintf(templn,"%c", byte); buffer += concat(buffer, templn);}
					}
					buffer += concat(buffer, "<br>");
				}

				p++; if(p>=0x10) p=0;
			}
			//

			concat(buffer, "<br><textarea id=\"output\" name=\"output\" cols=\"111\" rows=\"10\" readonly=\"true\">");

			for(int i = 0; i < length; i++)
			{
				sprintf(templn, "%02X", (u8)buffer_tmp[i]);
				concat(buffer, templn);
			}
		}
		else {sprintf(templn, "%s: %i", "Error", retval); concat(buffer, templn);}
		concat(buffer, "</textarea><br>");
	}

	concat(buffer, "<br>");

#ifdef DEBUG_MEM
	concat(buffer, "Dump: [<a href=\"/dump.ps3?mem\">Full Memory</a>] [<a href=\"/dump.ps3?rsx\">RSX</a>] [<a href=\"/dump.ps3?lv1\">LV1</a>] [<a href=\"/dump.ps3?lv2\">LV2</a>]");
	sprintf(templn, " [<a href=\"/dump.ps3?%llx\">LV1 Dump 0x%llx</a>] [<a href=\"/peek.lv1?%llx\">LV1 Peek</a>] [<a href=\"/peek.lv2?%llx\">LV2 Peek</a>]", address, address, address, address); concat(buffer, templn);
#endif
	concat(buffer, "<p>");

	if(!is_ps3mapi_home && islike(param, "/getmem.ps3mapi")) ps3mapi_setmem(buffer, templn, param);
}

static void ps3mapi_setmem(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	u32 pid = 0;
	u64 address = 0;
	int length = 0;
	char value[130];
	char val_tmp[260];

	if(strstr(param, ".ps3mapi?"))
	{
		char addr_tmp[17];
		if(get_param("addr=", addr_tmp, param, 16))
		{
			address = convertH(addr_tmp);

			length = get_param("val=", val_tmp, param, 259) / 2;
			if(length)
			{
				Hex2Bin(val_tmp, value);

				pid = get_valuen32(param, "proc=");
			}
		}
	}

	if(!is_ps3mapi_home && islike(param, "/setmem.ps3mapi")) ps3mapi_getmem(buffer, templn, param);

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s --> %s --> %s</b>"
						HTML_BLU_SEPARATOR,
						"<a href='/home.ps3mapi'>PS3MAPI</a>", "Processes Commands", "Set process memory");
	else
		sprintf(templn, "<u>%s:</u>", "Set process memory");

	concat(buffer, templn);

	sprintf(templn, "<form action=\"/setmem" HTML_FORM_METHOD_FMT
					"<b><u>%s:</u></b>  ", HTML_FORM_METHOD, "Process"); concat(buffer, templn); memset(templn, 0, MAX_LINE_LEN);

	add_proc_list(buffer, templn, &pid);

	if(*val_tmp == 0) sprintf(val_tmp, "00");

	sprintf(templn, "<b><u>%s:</u></b> "  HTML_INPUT("addr", "%llX", "16", "18")
					"<br><br><b><u>%s:</u></b><br>"
					"<table width=\"800\">"
					"<tr><td class=\"la\">"
					"<textarea id=\"val\" name=\"val\" cols=\"111\" rows=\"3\" maxlength=\"199\">%s</textarea></td></tr>"
					"<tr><td class=\"ra\"><br>"
					"<input class=\"bs\" type=\"submit\" value=\" %s \"/></td></tr></table></form>", "Address", address, "Value", val_tmp, "Set");
	concat(buffer, templn);

	if(pid != 0 && length != 0)
	{
		int retval = NONE;
		{system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PROC_MEM, (u64)pid, (u64)address, (u64)(u32)value, (u64)length); retval = (int)p1;}
		if(0 <= retval) sprintf(templn, "<br><b><u>%s!</u></b>", "Done");
		else sprintf(templn, "<br><b><u>%s: %i</u></b>", "Error", retval);
		concat(buffer, templn);
	}

	if(length == 0) concat(buffer, "<script>val.value=output.value</script>");

	if(!is_ps3mapi_home) concat(buffer, "<br>" HTML_RED_SEPARATOR); else concat(buffer, "<br>");
}

static void ps3mapi_setidps(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	{system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_IDPS, (u64)IDPS);}
	{system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PSID, (u64)PSID);}
	u64 _new_IDPS[2] = { IDPS[0], IDPS[1]};
	u64 _new_PSID[2] = { PSID[0], PSID[1]};

	if(islike(param, "/setidps.ps3mapi") && param[16] == '?')
	{
		char idps1_tmp[17];
		if(get_param("idps1=", idps1_tmp, param, 16))
		{
			_new_IDPS[0] = convertH(idps1_tmp);

			char idps2_tmp[17];
			if(get_param("idps2=", idps2_tmp, param, 16))
			{
				_new_IDPS[1] = convertH(idps2_tmp);

				{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_IDPS, (u64)_new_IDPS[0], (u64)_new_IDPS[1]);}
			}
		}

		char psid1_tmp[17];
		if(get_param("psid1=", psid1_tmp, param, 16))
		{
			_new_PSID[0] = convertH(psid1_tmp);

			char psid2_tmp[17];
			if(get_param("psid2=", psid2_tmp, param, 16))
			{
				_new_PSID[1] = convertH(psid2_tmp);

				{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PSID, (u64)_new_PSID[0], (u64)_new_PSID[1]);}
			}
		}
	}

	sprintf(templn, "<b>%s%s --> %s</b>"
					HTML_BLU_SEPARATOR
					"<form action=\"/setidps" HTML_FORM_METHOD_FMT
					"<table width=\"800\">"
					"<tr><td width=\"400\" class=\"la\">"
					"<br><b><u>%s:</u></b><br>" HTML_INPUT("idps1", "%016llX", "16", "18") HTML_INPUT("idps2", "%016llX", "16", "18") "</td>"
					"<td class=\"la\">"
					"<br><b><u>%s:</u></b><br>" HTML_INPUT("psid1", "%016llX", "16", "18") HTML_INPUT("psid2", "%016llX", "16", "18") "</td></tr>"
					"<tr><td class=\"ra\"><br><input class=\"bs\" type=\"submit\" value=\" %s \"/></td></tr>"
					"</table></form><br>",
					is_ps3mapi_home ? "" : "PS3MAPI --> ", "PS3 Commands", "Set IDPS/PSID",
					HTML_FORM_METHOD, "IDPS", _new_IDPS[0], _new_IDPS[1], "PSID", _new_PSID[0], _new_PSID[1], "Set");

	if(!is_ps3mapi_home) strcat(templn, HTML_RED_SEPARATOR);
	concat(buffer, templn);
}

static void add_plugins_list(char *buffer, char *templn)
{
	if(!strstr(buffer, "<datalist id=\"plugins\">"))
	{
		concat(buffer, "<div style=\"display:none\"><datalist id=\"plugins\">");
		int fd, cnt = 0;

		const char *paths[6] = {
									"/dev_hdd0/plugins",
									"/dev_hdd0/plugins/ps3xpad",
									"/dev_hdd0/plugins/ps3_menu",
									"/dev_hdd0/plugins/PS3Lock",
									"/dev_hdd0/game/UPDWEBMOD/USRDIR",
									"/dev_hdd0/tmp/wm_res"
								};

		#define PLUGINS_PATH	(i < 3) ? drives[i] : paths[i - 3]

		for(u8 i = 0; i < 9; i++)
		if(cellFsOpendir(PLUGINS_PATH, &fd) == CELL_FS_SUCCEEDED)
		{
			CellFsDirectoryEntry dir; u32 read_e;
			char *entry_name = dir.entry_name.d_name;

			u16 plen = sprintf(templn, "<option>%s/", PLUGINS_PATH);

			while(working && (!cellFsGetDirectoryEntries(fd, &dir, sizeof(dir), &read_e) && read_e))
			{
				if(!extcmp(entry_name, ".sprx", 5))
				{
					sprintf(templn + plen, "%s</option>", entry_name); buffer += concat(buffer, templn); if(++cnt > 50) break;
				}
			}
			cellFsClosedir(fd);
		}

		concat(buffer, "</datalist></div>");
	}
}

static void ps3mapi_vshplugin(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	char tmp_name[30];
	char tmp_filename[STD_PATH_LEN];

	if(islike(param, "/vshplugin.ps3mapi") && param[18] == '?')
	{
		unsigned int uslot = 99;

		char *pos = strstr(param + 18, "?s=");
		if(pos)
		{
			u8 boot_mode = get_valuen(pos, "?s=", 0, 4);
			sprintf(tmp_filename, "/dev_hdd0/boot_plugins.txt"); if(IS_DEX) sprintf(tmp_filename + 22, "_dex.txt");
			switch (boot_mode)
			{
				case 1: sprintf(tmp_filename + 10, "mamba_plugins.txt"); break;
				case 2: sprintf(tmp_filename + 10, "prx_plugins.txt");   break;
				case 3: sprintf(tmp_filename + 10, "game/PRXLOADER/USRDIR/plugins.txt"); break;
				case 4: sprintf(tmp_filename + 10, "boot_plugins_nocobra.txt"); if(IS_DEX) sprintf(tmp_filename + 30, "_dex.txt"); break;
			}

			int fdw = 0;
			if(cellFsOpen(tmp_filename, CELL_FS_O_CREAT|CELL_FS_O_WRONLY|CELL_FS_O_TRUNC, &fdw, NULL, 0) == CELL_FS_SUCCEEDED)
			{
				sprintf(templn, "<p><a href=\"%s\" style=\"padding:8px;background:#900;border-radius:8px;\">%s</a><p>", tmp_filename, tmp_filename); concat(buffer, templn);

				for (unsigned int slot = 1; slot < 7; slot++)
				{
					memset(tmp_name, 0, sizeof(tmp_name));
					memset(tmp_filename, 0, sizeof(tmp_filename));
					{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO, (u64)slot, (u64)(u32)tmp_name, (u64)(u32)tmp_filename); }
					if(*tmp_filename)
					{
						size_t flen = sprintf(templn, "%s\n", tmp_filename);
						cellFsWrite(fdw, (void *)templn, flen, NULL);
					}
				}
				cellFsClose(fdw);
			}
		}
		else
		{
			uslot = get_valuen(param, "load_slot=", 0, 6);

			if(strstr(param, "unload_slot="))
			{
				if ( uslot ) {system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_UNLOAD_VSH_PLUGIN, (u64)uslot);}
			}
			else
			{
				char prx_path[STD_PATH_LEN];
				if(get_param("prx=", prx_path, param, STD_PATH_LEN))
				{
					if (!uslot ) uslot = get_free_slot(); // find free slot if slot == 0

					if ( uslot ) {{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_LOAD_VSH_PLUGIN, (u64)uslot, (u64)(u32)prx_path, NULL, 0);}}
				}
			}
		}
	}

	sprintf(templn, "<b>%s%s</b>"
					HTML_BLU_SEPARATOR "<br>"
					"<table>"
					"<tr><td width=\"75\" class=\"la\">%s</td>"
					"<td width=\"120\" class=\"la\">%s</td>"
					"<td width=\"500\" class=\"la\">%s</td>"
					"<td width=\"125\" class=\"ra\"> </td></tr>",
					is_ps3mapi_home ? "" : "PS3MAPI --> ", "VSH Plugins", "Slot", "Name", "File name");

	buffer += concat(buffer, templn);
	for (unsigned int slot = 0; slot < 7; slot++)
	{
		memset(tmp_name, 0, sizeof(tmp_name));
		memset(tmp_filename, 0, sizeof(tmp_filename));
		{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO, (u64)slot, (u64)(u32)tmp_name, (u64)(u32)tmp_filename); }
		if(*tmp_filename)
		{
			sprintf(templn, "<tr><td width=\"75\" class=\"la\">%i</td>"
							"<td width=\"120\" class=\"la\">%s</td>"
							"<td width=\"500\" class=\"la\">",
							slot, tmp_name); buffer += concat(buffer, templn);

			buffer += add_breadcrumb_trail(buffer, tmp_filename);

			sprintf(templn, "</td>"
							"<td width=\"100\" class=\"ra\">"
							"<form action=\"/vshplugin" HTML_FORM_METHOD_FMT
							"<input name=\"unload_slot\" type=\"hidden\" value=\"%i\"><input type=\"submit\" %s/></form></td></tr>",
							HTML_FORM_METHOD, slot, (slot) ? "value=\" Unload \"" : "value=\" Reserved \" disabled" );
		}
		else
 		{
			sprintf(templn, "<tr><td width=\"75\" class=\"la\">%i</td>"
							"<td width=\"120\" class=\"la\">%s</td>"
							"<form action=\"/vshplugin" HTML_FORM_METHOD_FMT
							"<td width=\"500\" class=\"la\">"
							HTML_INPUT("prx\" style=\"width:555px\" list=\"plugins", "", "128", "75") "<input name=\"load_slot\" type=\"hidden\" value=\"%i\"></td>"
							"<td width=\"100\" class=\"ra\"><input type=\"submit\" %s/></td></form></tr>",
							slot, "NULL",
							HTML_FORM_METHOD, slot, (slot) ? "value=\" Load \"" : "value=\" Reserved \" disabled" );
		}
			buffer += concat(buffer, templn);
	}

	sprintf(templn, "<tr><td colspan=4><p>%s > "	HTML_BUTTON_FMT
													HTML_BUTTON_FMT
													HTML_BUTTON_FMT
													HTML_BUTTON_FMT
													HTML_BUTTON_FMT "</td></tr>", STR_SAVE,
		HTML_BUTTON, dex_mode ?
					"boot_plugins_dex.txt" :
					"boot_plugins.txt",				HTML_ONCLICK, "/vshplugin.ps3mapi?s=0",
		HTML_BUTTON, dex_mode ? 
					"boot_plugins_nocobra_dex.txt" :
					"boot_plugins_nocobra.txt",		HTML_ONCLICK, "/vshplugin.ps3mapi?s=4",
		HTML_BUTTON, "mamba_plugins.txt",			HTML_ONCLICK, "/vshplugin.ps3mapi?s=1",
		HTML_BUTTON, "prx_plugins.txt",				HTML_ONCLICK, "/vshplugin.ps3mapi?s=2",
		HTML_BUTTON, "plugins.txt",					HTML_ONCLICK, "/vshplugin.ps3mapi?s=3"); concat(buffer, templn);

	add_plugins_list(buffer, templn);

	sprintf(templn, "%s", "</table><br>");

	if(!is_ps3mapi_home) strcat(templn, HTML_RED_SEPARATOR);
	concat(buffer, templn);
}

static void ps3mapi_gameplugin(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (*param == ' ');

	u32 pid = 0;

	if(islike(param, "/gameplugin.ps3mapi") && param[19] == '?')
	{
		pid = get_valuen32(param, "proc=");
		if(pid)
		{

			char *pos = strstr(param, "unload_slot=");
			if(pos)
			{
				unsigned int prx_id = get_valuen32(pos, "unload_slot=");
				{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_UNLOAD_PROC_MODULE, (u64)pid, (u64)prx_id); }
			}
			else
			{
				char prx_path[STD_PATH_LEN];
				if(get_param("prx=", prx_path, param, STD_PATH_LEN))
				{
					system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LOAD_PROC_MODULE, (u64)pid, (u64)(u32)prx_path, NULL, 0);
				}
			}
		}
	}

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s --> %s</b>"
						HTML_BLU_SEPARATOR,
						"<a href='/home.ps3mapi'>PS3MAPI</a>", "Game Plugins");
	else
		sprintf(templn, "<b>%s</b>"
						HTML_BLU_SEPARATOR,
						"Game Plugins");

	concat(buffer, templn);

	sprintf(templn, "<form action=\"/gameplugin" HTML_FORM_METHOD_FMT
					"<b><u>%s:</u></b>  ", HTML_FORM_METHOD, "Process"); concat(buffer, templn); memset(templn, 0, MAX_LINE_LEN);

	add_proc_list(buffer, templn, &pid);

	if(is_ps3mapi_home)
		sprintf(templn, "<input class=\"bs\" type=\"submit\" value=\" Set \" /></form>");
	else
		sprintf(templn, "</form>");

	concat(buffer, templn);

	if(pid)
	{
		sprintf(templn,
					"<table>"
					 "<tr>"
					  "<td width=\"75\" class=\"la\">%s</td>"
					  "<td width=\"300\" class=\"la\">%s</td>"
					  "<td width=\"500\" class=\"la\">%s</td>"
					  "<td width=\"125\" class=\"ra\"> </td>"
					 "</tr>",
					"Slot", "Name", "File name"); buffer += concat(buffer, templn);

		char tmp_name[30];
		char tmp_filename[STD_PATH_LEN];
		u32 mod_list[62];
		memset(mod_list, 0, sizeof(mod_list));
		{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_MODULE_PID, (u64)pid, (u64)(u32)mod_list);}

		for(unsigned int slot = 0; slot <= 60; slot++)
		{
			memset(tmp_name, 0, sizeof(tmp_name));
			memset(tmp_filename, 0, sizeof(tmp_filename));
			if(1 < mod_list[slot])
			{
				{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_NAME, (u64)pid, (u64)mod_list[slot], (u64)(u32)tmp_name);}
				{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_FILENAME, (u64)pid, (u64)mod_list[slot], (u64)(u32)tmp_filename);}
			}
			if(*tmp_filename)
			{
				sprintf(templn,
						"<tr>"
						 "<td width=\"75\" class=\"la\">%i</td>"
						 "<td width=\"300\" class=\"la\">%s</td>"
						 "<td width=\"500\" class=\"la\">", 
						slot, tmp_name); buffer += concat(buffer, templn);

				buffer += add_breadcrumb_trail(buffer, tmp_filename);

				sprintf(templn, "</td>"
						 "<td width=\"100\" class=\"ra\">"
						  "<form action=\"/gameplugin" HTML_FORM_METHOD_FMT
						  "<input name=\"proc\" type=\"hidden\" value=\"%u\">"
						  "<input name=\"unload_slot\" type=\"hidden\" value=\"%i\">"
						  "<input type=\"submit\" value=\" Unload \">"
						  "</form>"
						 "</td>"
						"</tr>", HTML_FORM_METHOD, pid, mod_list[slot]);
			}
			else
			{
				sprintf(tmp_name, "NULL");
				//sprintf(tmp_filename, "/dev_hdd0/tmp/my_plugin_%i.sprx", slot);
				sprintf(templn,
						"<tr>"
						 "<td width=\"75\" class=\"la\">%i</td>"
						 "<td width=\"300\" class=\"la\">%s</td>"
						 "<td width=\"100\" class=\"ra\">"
						  "<form action=\"/gameplugin" HTML_FORM_METHOD_FMT
						   "<td width=\"500\" class=\"la\">"
						     "<input name=\"proc\" type=\"hidden\" value=\"%u\">"
						     HTML_INPUT("prx\" list=\"plugins", "", "128", "75")
						     "<input name=\"load_slot\" type=\"hidden\" value=\"%i\">"
						     "<input type=\"submit\" value=\" Load \">"
						   "</td>"
						  "</form>"
						 "</td>"
						"</tr>",
						slot, tmp_name,
						HTML_FORM_METHOD, pid, slot);
			}
			buffer += concat(buffer, templn);
		}

		add_plugins_list(buffer, templn);
	}

	sprintf(templn, "%s", "</table><br>");

	if(!is_ps3mapi_home) strcat(templn, HTML_RED_SEPARATOR);
	concat(buffer, templn);
}


///////////// PS3MAPI BEGIN //////////////


#define THREAD_NAME_PS3MAPI						"ps3m_api_server"
#define THREAD02_NAME_PS3MAPI					"ps3m_api_client"

#define PS3MAPI_RECV_SIZE  2048

#define PS3MAPI_MAX_LEN    383

static u32 BUFFER_SIZE_PS3MAPI = (_64KB_);

static sys_ppu_thread_t thread_id_ps3mapi = SYS_PPU_THREAD_NONE;

static void handleclient_ps3mapi(u64 conn_s_ps3mapi_p)
{
	int conn_s_ps3mapi = (int)conn_s_ps3mapi_p; // main communications socket
	int data_s = NONE;							// data socket
	int pasv_s = NONE;

	int connactive = 1;							// whether the ps3mapi connection is active or not
	int dataactive = 0;							// prevent the data connection from being closed at the end of the loop

	char buffer[PS3MAPI_RECV_SIZE];
	char cmd[20], param1[PS3MAPI_MAX_LEN + 1], param2[PS3MAPI_MAX_LEN + 1];

	#define PS3MAPI_OK_150    "150 OK: Binary status okay; about to open data connection.\r\n"
	#define PS3MAPI_OK_200    "200 OK: The requested action has been successfully completed.\r\n"
	#define PS3MAPI_OK_220    "220 OK: PS3 Manager API Server v1.\r\n"
	#define PS3MAPI_OK_221    "221 OK: Service closing control connection.\r\n"
	#define PS3MAPI_OK_226    "226 OK: Closing data connection. Requested binary action successful.\r\n"
	#define PS3MAPI_OK_230    "230 OK: Connected to PS3 Manager API Server.\r\n"

	#define PS3MAPI_ERROR_425 "425 Error: Can't open data connection.\r\n"
	#define PS3MAPI_ERROR_451 "451 Error: Requested action aborted. Local error in processing.\r\n"
	#define PS3MAPI_ERROR_501 "501 Error: Syntax error in parameters or arguments.\r\n"
	#define PS3MAPI_ERROR_550 "550 Error: Requested action not taken.\r\n"
	#define PS3MAPI_ERROR_502 "502 Error: Command not implemented.\r\n"

	#define PS3MAPI_CONNECT_NOTIF 	 "PS3MAPI: Client connected [%s]\r\n"
	#define PS3MAPI_DISCONNECT_NOTIF "PS3MAPI: Client disconnected [%s]\r\n"

	sys_net_sockinfo_t conn_info;
	sys_net_get_sockinfo(conn_s_ps3mapi, &conn_info, 1);

	char ip_address[16];
	char pasv_output[56];

	setPluginActive();

	ssend(conn_s_ps3mapi, PS3MAPI_OK_220);

	u8 ip_len = sprintf(ip_address, "%s", inet_ntoa(conn_info.local_adr));
	for(u8 n = 0; n < ip_len; n++) if(ip_address[n] == '.') ip_address[n] = ',';

	ssend(conn_s_ps3mapi, PS3MAPI_OK_230);

	sprintf(buffer, PS3MAPI_CONNECT_NOTIF, inet_ntoa(conn_info.remote_adr)); show_msg(buffer);

	while(connactive == 1 && working)
	{
		if(working && (recv(conn_s_ps3mapi, buffer, PS3MAPI_RECV_SIZE, 0) > 0))
		{
			char *p = strstr(buffer, "\r\n");
			if(p) *p = NULL; else break;

			int split = ssplit(buffer, cmd, 19, param1, PS3MAPI_MAX_LEN);

			if(_IS(cmd, "DISCONNECT"))
			{
				ssend(conn_s_ps3mapi, PS3MAPI_OK_221);
				connactive = 0;
			}
			else if(_IS(cmd, "CORE") || _IS(cmd, "SERVER"))
			{
				if(split)
				{
					bool is_core = _IS(cmd, "CORE");
					split = ssplit(param1, cmd, 19, param2, PS3MAPI_MAX_LEN);
					if(_IS(cmd, "GETVERSION"))
					{
						int version = PS3MAPI_SERVER_VERSION;
						if(is_core) { system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_VERSION); version = (int)(p1); }
						sprintf(buffer, "200 %i\r\n", version);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(_IS(cmd, "GETMINVERSION"))
					{
						int version = PS3MAPI_SERVER_MINVERSION;
						if(is_core) { system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_MINVERSION); version = (int)(p1); }
						sprintf(buffer, "200 %i\r\n", version);
						ssend(conn_s_ps3mapi, buffer);
					}
					else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502);
				}
				else
				{
					ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
				}
			}
			else if(_IS(cmd, "PS3"))
			{
				if(split)
				{
					split = ssplit(param1, cmd, 19, param2, PS3MAPI_MAX_LEN); to_upper(cmd);

					if(_IS(cmd, "REBOOT") || _IS(cmd, "SOFTREBOOT") || _IS(cmd, "HARDREBOOT") || _IS(cmd, "SHUTDOWN"))
					{
						ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						setPluginExit();

						if(_IS(cmd, "REBOOT"))     {system_call_3(SC_SYS_POWER, SYS_REBOOT, NULL, 0); }
						if(_IS(cmd, "SOFTREBOOT")) {system_call_3(SC_SYS_POWER, SYS_SOFT_REBOOT, NULL, 0); }
						if(_IS(cmd, "HARDREBOOT")) {system_call_3(SC_SYS_POWER, SYS_HARD_REBOOT, NULL, 0); }
						if(_IS(cmd, "SHUTDOWN"))   {system_call_4(SC_SYS_POWER, SYS_SHUTDOWN, 0, 0, 0); }

						sys_ppu_thread_exit(0);
					}
					else if(_IS(cmd, "GETFWVERSION"))
					{
						int version = 0;
						{system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_FW_VERSION); version = (int)(p1); }
						sprintf(buffer, "200 %i\r\n", version);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(_IS(cmd, "GETFWTYPE"))
					{
						memset(param2, 0, sizeof(param2));
						{system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_FW_TYPE, (u64)(u32)param2); }
						sprintf(buffer, "200 %s\r\n", param2);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(_IS(cmd, "NOTIFY"))
					{
						if(split)
						{
							show_msg(param2);
							ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(islike(cmd, "BUZZER"))
					{
						if(cmd[6] == '1') { BEEP1 }
						if(cmd[6] == '2') { BEEP2 }
						if(cmd[6] == '3') { BEEP3 }

						ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
					}
					else if(_IS(cmd, "LED"))
					{
						if(split)
						{
							split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
							if(split)
							{
								u64 color = val(param1);
								u64 mode = val(param2);
								{system_call_2(SC_SYS_CONTROL_LED, color, mode); }
								ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(_IS(cmd, "GETTEMP"))
					{
						u8 cpu_temp = 0;
						u8 rsx_temp = 0;
						get_temperature(0, &cpu_temp);
						get_temperature(1, &rsx_temp);
						sprintf(buffer, "200 %i|%i\r\n", cpu_temp, rsx_temp);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(_IS(cmd, "DISABLESYSCALL"))
					{
						if(split)
						{
							int num = val(param2);
							{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, (u64)num); }
							ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(_IS(cmd, "CHECKSYSCALL"))
					{
						if(split)
						{
							int num = val(param2);
							int check = 0;
							{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_CHECK_SYSCALL, (u64)num); check = (int)(p1); }
							sprintf(buffer, "200 %i\r\n", check);
							ssend(conn_s_ps3mapi, buffer);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(_IS(cmd, "PDISABLESYSCALL8"))
					{
						if(split)
						{
							int mode = val(param2);
							{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, (u64)mode); }
							ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(_IS(cmd, "PCHECKSYSCALL8"))
					{
						int check = 0;
						{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PCHECK_SYSCALL8); check = (int)(p1); }
						sprintf(buffer, "200 %i\r\n", check);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(islike(cmd, "DELHISTORY"))
					{
						delete_history(_IS(cmd, "DELHISTORY+F"));
						ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
					}
					else if(_IS(cmd, "REMOVEHOOK"))
					{
						{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_REMOVE_HOOK); }
						ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
					}
					else if(_IS(cmd, "GETIDPS"))
					{
						u64 _new_idps[2] = {0, 0};
						{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_IDPS, (u64)(u32)_new_idps);}
						sprintf(buffer, "200 %016llX%016llX\r\n", _new_idps[0], _new_idps[1]);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(_IS(cmd, "SETIDPS"))
					{
						if(split)
						{
							split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
							if(split)
							{
								u64 part1 = convertH(param1);
								u64 part2 = convertH(param2);
								{ system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_IDPS, part1, part2);}
								ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(_IS(cmd, "GETPSID"))
					{
						u64 _new_psid[2] = {0, 0};
						{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PSID, (u64)(u32)_new_psid);}
						sprintf(buffer, "200 %016llX%016llX\r\n", _new_psid[0], _new_psid[1]);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(_IS(cmd, "SETPSID"))
					{
						if(split)
						{
							split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
							if(split)
							{
								u64 part1 = convertH(param1);
								u64 part2 = convertH(param2);
								{ system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PSID, (u64)part1, (u64)part2);}
								ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502);
				}
				else
				{
					ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
				}
			}
			else if(_IS(cmd, "PROCESS"))
			{
				if(split)
				{
					split = ssplit(param1, cmd, 19, param2, PS3MAPI_MAX_LEN);
					if(_IS(cmd, "GETNAME"))
					{
						if(split)
						{
							u32 pid = val(param2);
							memset(param2, 0, sizeof(param2));
							{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID, (u64)pid, (u64)(u32)param2); }
							sprintf(buffer, "200 %s\r\n", param2);
							ssend(conn_s_ps3mapi, buffer);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(_IS(cmd, "GETALLPID"))
					{
						u32 pid_list[16];
						memset(buffer, 0, sizeof(buffer));
						u32 buf_len = sprintf(buffer, "200 ");
						{system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (u64)(u32)pid_list); }
						for(int i = 0; i < 16; i++)
						{
							buf_len += sprintf(buffer + buf_len, "%i|", pid_list[i]);
						}
						buf_len += sprintf(buffer + buf_len, "\r\n");
						send(conn_s_ps3mapi, buffer, buf_len, 0);
					}
					else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502);
				}
				else
				{
					ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
				}
			}
			else if(_IS(cmd, "MEMORY"))
			{
				if(split)
				{
					split = ssplit(param1, cmd, 19, param2, PS3MAPI_MAX_LEN);
					if(_IS(cmd, "GET"))
					{
						if(data_s < 0 && pasv_s >= 0) data_s = accept(pasv_s, NULL, NULL);

						if(data_s > 0)
						{
							if(split)
							{
								split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
								if(split)
								{
									u32 attached_pid = val(param1);
									split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
									if(split)
									{
										u64 offset = convertH(param1);
										u32 size = val(param2);
										int rr = -4;
										sys_addr_t sysmem = NULL;
										if(sys_memory_allocate(BUFFER_SIZE_PS3MAPI, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK)
										{
											char *buffer2 = (char*)sysmem;
											ssend(conn_s_ps3mapi, PS3MAPI_OK_150);
											rr = 0;
											while(working)
											{
												if(size > BUFFER_SIZE_PS3MAPI)
												{
													u32 sizetoread = BUFFER_SIZE_PS3MAPI;
													u32 leftsize = size;
													if(size < BUFFER_SIZE_PS3MAPI) sizetoread = size;
													while(0 < leftsize)
													{
														system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MEM, (u64)attached_pid, offset, (u64)(u32)buffer2, (u64)sizetoread);
														if(send(data_s, buffer2, sizetoread, 0) < 0) { rr = -3; break; }
														offset += sizetoread;
														leftsize -= sizetoread;
														if(leftsize < BUFFER_SIZE_PS3MAPI) sizetoread = leftsize;
														if(sizetoread == 0) break;
													}
													break;
												}
												else
												{
													system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MEM, (u64)attached_pid, (u64)offset, (u64)(u32)buffer2, (u64)size);
													if(send(data_s, buffer2, size, 0) < 0) { rr = -3; break; }
													break;
												}
											}
											sys_memory_free(sysmem);
										}
										if(rr == 0) ssend(conn_s_ps3mapi, PS3MAPI_OK_226);
										else if(rr == -4) ssend(conn_s_ps3mapi, PS3MAPI_ERROR_550);
										else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_451);
									}
									else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
								}
								else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_425);
					}
					else if(_IS(cmd, "SET"))
					{
						if(data_s < 0 && pasv_s >= 0) data_s = accept(pasv_s, NULL, NULL);

						if(data_s > 0)
						{
							if(split)
							{
								split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
								if(split)
								{
									u32 attached_pid = val(param1);
									u64 offset = convertH(param2);
									int rr = NONE;
									sys_addr_t sysmem = NULL;
									if(sys_memory_allocate(BUFFER_SIZE_PS3MAPI, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK)
									{
										char *buffer2 = (char*)sysmem;
										u64 read_e = 0;
										ssend(conn_s_ps3mapi, PS3MAPI_OK_150);
										rr = 0;
										while(working)
										{
											if((read_e = (u64)recv(data_s, buffer2, BUFFER_SIZE_PS3MAPI, MSG_WAITALL)) > 0)
											{
												system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PROC_MEM, (u64)attached_pid, offset, (u64)(u32)buffer2, read_e);
												offset += read_e;
											}
											else
											{
												break;
											}
										}
										sys_memory_free(sysmem);
									}
									if(rr == 0) ssend(conn_s_ps3mapi, PS3MAPI_OK_226);
									else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_451);
								}
								else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_425);
					}
					else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502);
				}
				else
				{
					ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
				}
			}
			else if(_IS(cmd, "MODULE"))
			{
				if(split)
				{
					split = ssplit(param1, cmd, 19, param2, PS3MAPI_MAX_LEN);
					if(_IS(cmd, "GETNAME") || _IS(cmd, "GETFILENAME"))
					{
						if(split)
						{
							split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
							if(split)
							{
								u32 pid = val(param1);
								s32 prxid = val(param2);
								memset(param2, 0, sizeof(param2));

								if(_IS(cmd, "GETFILENAME"))
									{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_FILENAME, (u64)pid, (u64)prxid, (u64)(u32)param2); }
								else
									{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_NAME, (u64)pid, (u64)prxid, (u64)(u32)param2); }

								sprintf(buffer, "200 %s\r\n", param2);
								ssend(conn_s_ps3mapi, buffer);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(_IS(cmd, "GETALLPRXID"))
					{
						if(split)
						{
							s32 prxid_list[128];
							u32 pid = val(param2);
							memset(buffer, 0, sizeof(buffer));
							u32 buf_len = sprintf(buffer, "200 ");
							{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_MODULE_PID, (u64)pid, (u64)(u32)prxid_list); }
							for(u8 i = 0; i < 128; i++)
							{
								buf_len += sprintf(buffer + buf_len, "%i|", prxid_list[i]);
							}
							buf_len += sprintf(buffer + buf_len, "\r\n");
							send(conn_s_ps3mapi, buffer, buf_len, 0);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(_IS(cmd, "LOAD") || _IS(cmd, "UNLOAD"))
					{
						if(split)
						{
							split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
							if(split)
							{
								u32 pid = val(param1);

								if(_IS(cmd, "UNLOAD"))
								{
									s32 prx_id = val(param2);
									{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_UNLOAD_PROC_MODULE, (u64)pid, (u64)prx_id); }
								}
								else
									{system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LOAD_PROC_MODULE, (u64)pid, (u64)(u32)param2, NULL, 0); }

								ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(_IS(cmd, "LOADVSHPLUG"))
					{
						if(split)
						{
							split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
							if(split)
							{
								unsigned int slot = val(param1);
								if(!slot ) slot = get_free_slot();
								if( slot ) {{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_LOAD_VSH_PLUGIN, (u64)slot, (u64)(u32)param2, NULL, 0); }}
								ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
							}
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(_IS(cmd, "UNLOADVSHPLUGS"))
					{
						if(split)
						{
							unsigned int slot = val(param2);
							if( slot ) {{system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_UNLOAD_VSH_PLUGIN, (u64)slot); }}
							ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(_IS(cmd, "GETVSHPLUGINFO"))
					{
						if(split)
						{
							unsigned int slot = val(param2);
							memset(param1, 0, sizeof(param1));
							memset(param2, 0, sizeof(param2));
							{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO, (u64)slot, (u64)(u32)param1, (u64)(u32)param2); }
							sprintf(buffer, "200 %s|%s\r\n", param1, param2);
							ssend(conn_s_ps3mapi, buffer);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502);
				}
				else
				{
					ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
				}
			}
			else if(_IS(cmd, "TYPE"))
			{
				if(split)
				{
					ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
					dataactive = !IS(param1, "I");
				}
				else
				{
					dataactive = 1;
				}
			}
			else if(_IS(cmd, "PASV"))
			{
				CellRtcTick pTick; cellRtcGetCurrentTick(&pTick);
				u8 pasv_retry = 0; u32 pasv_port = (pTick.tick & 0xfeff00) >> 8;

				for(int p1x, p2x; pasv_retry < 250; pasv_retry++, pasv_port++)
				{
					if(data_s >= 0) sclose(&data_s);
					if(pasv_s >= 0) sclose(&pasv_s);

					p1x = ( (pasv_port & 0xff00) >> 8) | 0x80; // use ports 32768 -> 65528 (0x8000 -> 0xFFF8)
					p2x = ( (pasv_port & 0x00ff)     );

					pasv_s = slisten(getPort(p1x, p2x), 1);

					if(pasv_s >= 0)
					{
						sprintf(pasv_output, "227 Entering Passive Mode (%s,%i,%i)\r\n", ip_address, p1x, p2x);
						ssend(conn_s_ps3mapi, pasv_output);

						if((data_s = accept(pasv_s, NULL, NULL)) > 0)
						{
							dataactive = 1; break;
						}
					}
				}

				if(pasv_retry >= 250)
				{
					ssend(conn_s_ps3mapi, FTP_ERROR_451);	// Requested action aborted. Local error in processing.
					if(pasv_s >= 0) sclose(&pasv_s);
					pasv_s = NONE;
				}
			}
			else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502);

			if(dataactive) dataactive = 0;
			else
			{
				sclose(&data_s); data_s = NONE;
			}
		}
		else
		{
			connactive = 0;
			break;
		}

		sys_ppu_thread_usleep(1668);
	}

	sprintf(buffer, PS3MAPI_DISCONNECT_NOTIF, inet_ntoa(conn_info.remote_adr));
	show_msg(buffer);

	if(pasv_s >= 0) sclose(&pasv_s);
	sclose(&conn_s_ps3mapi);
	sclose(&data_s);

	setPluginInactive();
	sys_ppu_thread_exit(0);
}

static void ps3mapi_thread(__attribute__((unused)) u64 arg)
{
	int core_minversion = 0;
	{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_MINVERSION); core_minversion = (int)(p1); }
	if((core_minversion != 0) && (PS3MAPI_CORE_MINVERSION == core_minversion)) //Check if ps3mapi core has a compatible min_version.
	{
		int list_s = NONE;
		ps3mapi_working = 1;

	relisten:
		if(working) list_s = slisten(PS3MAPIPORT, PS3MAPI_BACKLOG);
		else goto end;

		if(list_s < 0)
		{
			sys_ppu_thread_sleep(1);
			if(working) goto relisten;
			else goto end;
		}

		active_socket[2] = list_s;

		//if(list_s >= 0)
		{
			while(working && ps3mapi_working)
			{
				sys_ppu_thread_usleep(100000);
				int conn_s_ps3mapi;
				if (!working || !ps3mapi_working) goto end;

				if(sys_admin && ((conn_s_ps3mapi = accept(list_s, NULL, NULL)) >= 0))
				{
					sys_ppu_thread_t t_id;
					if(working) sys_ppu_thread_create(&t_id, handleclient_ps3mapi, (u64)conn_s_ps3mapi, THREAD_PRIO, THREAD_STACK_SIZE_PS3MAPI_CLI, SYS_PPU_THREAD_CREATE_NORMAL, THREAD02_NAME_PS3MAPI);
					else {sclose(&conn_s_ps3mapi); break;}
				}
				else
				if((sys_net_errno == SYS_NET_EBADF) || (sys_net_errno == SYS_NET_ENETDOWN))
				{
					sclose(&list_s);
					if(working) goto relisten;
					else break;
				}
			}
		}
end:
		sclose(&list_s);
	}
	else show_msg((char *)"PS3MAPI Server not loaded!");

	sys_ppu_thread_exit(0);
}
#endif // #ifdef PS3MAPI

////////////////////////////////////////
///////////// PS3MAPI END //////////////
////////////////////////////////////////

#ifdef COBRA_ONLY
static unsigned int get_vsh_plugin_slot_by_name(const char *name, bool unload)
{
	char tmp_name[30];
	char tmp_filename[STD_PATH_LEN];

	bool find_free_slot = (!name || (*name == PS3MAPI_FIND_FREE_SLOT));

	unsigned int slot;
	for (slot = 1; slot < 7; slot++)
	{
		memset(tmp_name, 0, sizeof(tmp_name));
		memset(tmp_filename, 0, sizeof(tmp_filename));

		{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO, (u64)slot, (u64)(u32)tmp_name, (u64)(u32)tmp_filename); }

		if(find_free_slot) {if(*tmp_name) continue; break;}
		else
		if(IS(tmp_name, name) || strstr(tmp_filename, name)) {if(unload) cobra_unload_vsh_plugin(slot); break;}
	}
	return slot;
}

static void unload_vsh_gui(void)
{
	unload_vsh_plugin("VSH_MENU"); // unload vsh menu
	unload_vsh_plugin("sLaunch");  // unload sLaunch
}

static void start_vsh_gui(bool vsh_menu)
{
	unsigned int slot;
	slot = unload_vsh_plugin(vsh_menu ? "VSH_MENU" : "sLaunch");
	if(slot < 7) return; unload_vsh_gui();
	slot = get_free_slot(); char arg[2] = {1, 0};
	if(slot < 7) cobra_load_vsh_plugin(slot, vsh_menu ? WM_RES_PATH "/wm_vsh_menu.sprx" : WM_RES_PATH "/slaunch.sprx", (u8*)arg, 1);
}
#endif
///////////////////////////////
