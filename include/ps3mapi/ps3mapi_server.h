///////////// SUMMARY OF PS3MAPI SERVER COMMANDS //////////////
// /<web command>
// GET /<web command>
// DISCONNECT
// TYPE <A/I>
// PASV
// CORE GETVERSION
// SERVER GETVERSION
// CORE GETMINVERSION
// SERVER GETMINVERSION
// PS3 REBOOT
// PS3 SOFTREBOOT
// PS3 HARDREBOOT
// PS3 SHUTDOWN
// PS3 GETFWVERSION
// PS3 GETFWMINVERSION
// PS3 GETFWTYPE
// PS3 GETSYSINFO <id>
// PS3 GETBDINFO <dump-file>
// PS3 GETNOBDINFO
// PS3 GETCLOCKINFO
// PS3 NOTIFY <msg>&icon=<0-50>&snd=<0-9>
// PS3 BUZZER<0-9>
// PS3 LED <color> <mode>
// PS3 GETTEMP
// PS3 DISABLESYSCALL <sc-num>
// PS3 CHECKSYSCALL <sc-num>
// PS3 PDISABLESYSCALL8 <mode>
// PS3 PCHECKSYSCALL8
// PS3 DELHISTORY
// PS3 DELHISTORY+F
// PS3 REMOVEHOOK
// PS3 GETRSXCLOCK
// PS3 SETGPUCLOCK <mhz>
// PS3 SETVRAMCLOCK <mhz>
// PS3 GETIDPS
// PS3 SETIDPS <part1> <part2>
// PS3 GETPSID
// PS3 SETPSID <part1> <part2>
// PROCESS GETNAME <pid>
// PROCESS GETALLPID
// PROCESS GETCURRENTPID
// MEMORY GET <pid> <offset> <size> <= Memory data is sent through data port
// MEMORY SET <pid> <offset>        <= New data is received through data port
// MEMORY PAYLOAD <pid> <payload-path>
// MEMORY PAYLOAD <pid> unload
// MEMORY PAGEALLOCATE <pid> <size> <page_size> <flags> <is_executable>
// MEMORY PAGEFREE <pid> <flags> <page_table_0> <page_table_1>
// MODULE GETNAME <pid> <prxid>
// MODULE GETFILENAME <pid> <prxid>
// MODULE UNLOAD <pid> <prx-id>
// MODULE LOAD <pid> <prx_path>
// MODULE LOADVSHPLUGS <slot> <prx-path>
// MODULE UNLOADVSHPLUGS <slot>
// MODULE GETVSHPLUGINFO <slot>
// MODULE GETSEGMENTS <pid> <prx-id>
// THREAD CREATE <pid> <page_table_0> <toc> <arg> <prio> <stack_size> <name>
// REGISTRY GET <regkey>
// REGISTRY SET <regkey> <value>
// PEEKLV1 <address>
// PEEKLV2 <address>
// POKELV1 <address> <value>
// POKELV2 <address> <value>
// SYSCALL <syscall-number>|0x<hex-value>|<decimal-value>|<string-value>
// FILE EXISTS <file-path>
// FILE ISDIR <file-path>
// FILE SIZE <file-path>
// FILE MD5 <file-path>
///////////// PS3MAPI BEGIN //////////////

#define THREAD_NAME_PS3MAPI				"ps3m_api_server"
#define THREAD02_NAME_PS3MAPI			"ps3m_api_client"

#define PS3MAPI_RECV_SIZE  2048

#define PS3MAPI_CMD_LEN	19
#define PS3MAPI_MAX_LEN	383

#ifdef PS3MAPI
static u32 BUFFER_SIZE_PS3MAPI = (_64KB_);

static sys_ppu_thread_t thread_id_ps3mapi = SYS_PPU_THREAD_NONE;

#define RESPONSE_KEY	"{\"response\": "

static int ps3mapi_response_status(int conn_s_ps3mapi, char *buffer, const char *status)
{
	int code = (int)val(status);

	if(conn_s_ps3mapi)
	{
		// send response status to PS3MAPI client
		ssend(conn_s_ps3mapi, status);
	}
	else
	{
		// return JSON with status code & message
		const char *msg = (code == 200) ? "OK\r\n" : status + 4;
		sprintf(buffer, "{\"code\": %i, \"status\": \"%s", code, msg);

		// replace last CRLF with "}
		replace_char(buffer, '\r', '"');
		replace_char(buffer, '\n', '}');
	}
	return code;
}

static int ps3mapi_response_str(int conn_s_ps3mapi, char *buffer, const char *value, bool quoted)
{
	int len;
	if(conn_s_ps3mapi)
	{
		// send response value to PS3MAPI client
		len = sprintf(buffer, "200 %s\r\n", value);
		ssend(conn_s_ps3mapi, buffer);
	}
	else
	{
		// return JSON with response key
		len = sprintf(buffer, quoted ? "%s\"%s\"}" : "%s%s}", RESPONSE_KEY, value);
	}

	return len;
}

static int ps3mapi_response_int(int conn_s_ps3mapi, char *buffer, int value, bool numeric)
{
	bool is_num = (numeric || !value || conn_s_ps3mapi);
	char *str_value = buffer + 0x20;

	//convert integer to string or hex string
	sprintf(str_value, is_num ? "%i" : "0x%X", value);
	return ps3mapi_response_str(conn_s_ps3mapi, buffer, str_value, !is_num);
}

static int ps3mapi_response_long(int conn_s_ps3mapi, char *buffer, u64 value, bool numeric)
{
	bool is_num = (numeric || !value || conn_s_ps3mapi);
	char *str_value = buffer + 0x20;

	//convert long integer to string or hex string
	sprintf(str_value, is_num ? "%llu" : "0x%016llX", value);
	return ps3mapi_response_str(conn_s_ps3mapi, buffer, str_value, !is_num);
}

static int ps3mapi_response_values(int conn_s_ps3mapi, char *buffer, char *values)
{
	if(conn_s_ps3mapi)
	{
		// send values separated by pipes |
		sprintf(buffer, "200 %s\r\n", values);
		ssend(conn_s_ps3mapi, buffer);
	}
	else
	{
		// return a JSON array
		replace_char(values, '|', ',');
		
		sprintf(buffer, "%s[%s]}", RESPONSE_KEY, values);
	}
	return 200;
}

static int ps3mapi_command(int conn_s_ps3mapi, int data_s, int pasv_s, char *buffer)
{
	char cmd[20], param1[PS3MAPI_MAX_LEN + 1], param2[PS3MAPI_MAX_LEN + 1];

	#define PS3MAPI_OK_150	"150 OK: Binary status okay; about to open data connection.\r\n"
	#define PS3MAPI_OK_200	"200 OK: The requested action has been successfully completed.\r\n"
	#define PS3MAPI_OK_226	"226 OK: Closing data connection. Requested binary action successful.\r\n"

	#define PS3MAPI_ERROR_425 "425 Error: Can't open data connection.\r\n"
	#define PS3MAPI_ERROR_451 "451 Error: Requested action aborted. Local error in processing.\r\n"
	#define PS3MAPI_ERROR_501 "501 Error: Syntax error in parameters or arguments.\r\n"
	#define PS3MAPI_ERROR_550 "550 Error: Requested action not taken.\r\n"
	#define PS3MAPI_ERROR_502 "502 Error: Command not implemented.\r\n"

	int split = ssplit(buffer, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);


	if(_IS(cmd, "CORE") || _IS(cmd, "SERVER"))
	{
		if(split)
		{
			bool is_core = _IS(cmd, "CORE");
			split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);
			if(_IS(cmd, "GETVERSION"))	// CORE GETVERSION
			{							// SERVER GETVERSION
				int version = PS3MAPI_SERVER_VERSION;
				if(is_core) { system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_VERSION); version = (int)(p1); }
				split = ps3mapi_response_int(conn_s_ps3mapi, buffer, version, false);
			}
			else if(_IS(cmd, "GETMINVERSION"))	// CORE GETMINVERSION
			{									// SERVER GETMINVERSION
				int version = PS3MAPI_SERVER_MINVERSION;
				if(is_core) { system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_MINVERSION); version = (int)(p1); }
				split = ps3mapi_response_int(conn_s_ps3mapi, buffer, version, false);
			}
			else split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_502);
		}
		else split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_501);
	}
	else if(_IS(cmd, "PS3"))
	{
		if(split)
		{
			split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN); to_upper(cmd);

			if(_IS(cmd, "REBOOT") || _IS(cmd, "SOFTREBOOT") || _IS(cmd, "HARDREBOOT") || _IS(cmd, "SHUTDOWN"))
			{
				split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);
				setPluginExit();

				if(_IS(cmd, "REBOOT"))     { system_call_3(SC_SYS_POWER, SYS_REBOOT, NULL, 0); }
				if(_IS(cmd, "SOFTREBOOT")) { system_call_3(SC_SYS_POWER, SYS_SOFT_REBOOT, NULL, 0); }
				if(_IS(cmd, "HARDREBOOT")) { system_call_3(SC_SYS_POWER, SYS_HARD_REBOOT, NULL, 0); }
				if(_IS(cmd, "SHUTDOWN"))   { system_call_4(SC_SYS_POWER, SYS_SHUTDOWN, 0, 0, 0); }

				sys_ppu_thread_exit(0);
			}
			else if(_IS(cmd, "GETFWVERSION"))	// PS3 GETFWVERSION
			{
				int version = 0;
				{system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_FW_VERSION); version = (int)(p1); }
				split = ps3mapi_response_int(conn_s_ps3mapi, buffer, version, false);
			}
			else if(_IS(cmd, "GETFWMINVERSION"))	// PS3 GETFWMINVERSION
			{
				u8 data[0x20];
				_memset(data, sizeof(data));

				int ret = GetApplicableVersion(data);
				if(ret)
					split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_451);
				else
				{
					sprintf(param1, "%x.%02x", data[1], data[3]);
					split = ps3mapi_response_str(conn_s_ps3mapi, buffer, param1, true);
				}
			}
			else if(_IS(cmd, "GETFWTYPE"))	// PS3 GETFWTYPE
			{
				_memset(param2, sizeof(param2));
				{system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_FW_TYPE, (u64)(u32)param2); }
				split = ps3mapi_response_str(conn_s_ps3mapi, buffer, param2, true);
			}
			else if(_IS(cmd, "GETSYSINFO"))	// PS3 GETSYSINFO <id>
			{
				get_sys_info(param2, val(param2), true);
				split = ps3mapi_response_str(conn_s_ps3mapi, buffer, param2, true);
			}
			#ifdef BDINFO
			else if(_IS(cmd, "GETBDINFO"))	// PS3 GETBDINFO <dump-file>
			{
				char *data = param1, *outpath = param2;

				get_bdvd_info(outpath, data);

				split = ps3mapi_response_str(conn_s_ps3mapi, buffer, data, true);
			}
			#endif
			#ifdef NOBD_PATCH
			else if(_IS(cmd, "GETNOBDINFO"))	// PS3 GETNOBDINFO
			{
				int check = ALLOW_NOBD ? isNOBD : -1;
				split = ps3mapi_response_int(conn_s_ps3mapi, buffer, check, true);
			}
			#endif
			else if(_IS(cmd, "GETHDDINFO"))	// PS3 GETHDDINFO <dev_path>
			{
				char *full_path = split ? param2 : drives[0];

				u32 minfree = 0, optim = 0;
				u64 sb_addr = _islike(full_path, drives[0]) ? get_ufs_sb_addr() : 0;

				if(sb_addr)
				{
					minfree = lv2_peek_32(sb_addr + 0x3C);
					optim   = lv2_peek_32(sb_addr + 0x80);
				}

				u64 freeSize = 0, devSize = 0;
				#ifdef USE_NTFS
				if(is_ntfs_path(full_path))
					get_ntfs_disk_space(full_path + 5, &freeSize, &devSize);
				else
				#endif
				if(isDir(full_path)) {system_call_3(SC_FS_DISK_FREE, (u64)(u32)(full_path), (u64)(u32)&devSize, (u64)(u32)&freeSize);}

				sprintf(param2, "%llu|%llu|%u|%u", (long long unsigned int)freeSize, (long long unsigned int)devSize, (unsigned int)minfree, (unsigned int)optim);
				split = ps3mapi_response_values(conn_s_ps3mapi, buffer, param2);
			}
			else if(_IS(cmd, "GETCLOCKINFO"))	// PS3 GETCLOCKINFO
			{
				CellRtcDateTime pTime;
				cellRtcGetCurrentClockLocalTime(&pTime);

				sprintf(param2, "%04i-%02i-%02i %02i:%02i:%02i", pTime.year, pTime.month, pTime.day, pTime.hour, pTime.minute, pTime.second);
				split = ps3mapi_response_str(conn_s_ps3mapi, buffer, param2, true);
			}
			else if(_IS(cmd, "GETFANSPEED"))	// PS3 GETFANSPEED
			{
				u8 st, mode, unknown;
				u8 fan_speed8 = 0;
				sys_sm_get_fan_policy(0, &st, &mode, &fan_speed8, &unknown);
				sprintf(param2, "%i|0x%x|%i", (int)((int)fan_speed8 * 100) / 255, fan_speed8, mode);
				split = ps3mapi_response_values(conn_s_ps3mapi, buffer, param2);
			}
			else if(_IS(cmd, "NOTIFY"))	// PS3 NOTIFY <msg>&icon=<0-50>&snd=<0-9>
			{
				if(split)
				{
					show_msg(param2);
					split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);
				}
			}
			else if(islike(cmd, "BUZZER"))	// PS3 BUZZER<0-9>
			{
				play_sound_id(cmd[6]);

				split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200); split = -200;
			}
			else if(_IS(cmd, "LED"))	// PS3 LED <color> <mode>
			{
				if(split)
				{
					split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
					if(split)
					{
						u64 color = val(cmd);
						u64 mode = val(param1);
						{system_call_2(SC_SYS_CONTROL_LED, color, mode); }
						split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);
					}
				}
			}
			else if(_IS(cmd, "GETTEMP"))	// PS3 GETTEMP
			{
				u8 cpu_temp = 0;
				u8 rsx_temp = 0;
				get_temperature(0, &cpu_temp);
				get_temperature(1, &rsx_temp);
				sprintf(param2, "%i|%i", cpu_temp, rsx_temp);
				split = ps3mapi_response_values(conn_s_ps3mapi, buffer, param2);
			}
			else if(_IS(cmd, "DISABLESYSCALL"))	// PS3 DISABLESYSCALL <sc-num>
			{
				if(split)
				{
					int num = val(param2);
					{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, (u64)num); }
					split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);
				}
			}
			else if(_IS(cmd, "CHECKSYSCALL"))	// PS3 CHECKSYSCALL <sc-num>
			{
				if(split)
				{
					int num = val(param2);
					int check = 0;
					{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_CHECK_SYSCALL, (u64)num); check = (int)(p1); }
					split = ps3mapi_response_int(conn_s_ps3mapi, buffer, check, true);
				}
			}
			else if(_IS(cmd, "PDISABLESYSCALL8"))	// PS3 PDISABLESYSCALL8 <mode>
			{
				if(split)
				{
					int mode = val(param2);
					{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, (u64)mode); }
					split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);
				}
			}
			else if(_IS(cmd, "PCHECKSYSCALL8"))	// PS3 PCHECKSYSCALL8
			{
				int check = 0;
				{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PCHECK_SYSCALL8); check = (int)(p1); }
				split = ps3mapi_response_int(conn_s_ps3mapi, buffer, check, true);
			}
			else if(islike(cmd, "DELHISTORY"))	// PS3 DELHISTORY
			{									// PS3 DELHISTORY+F
				delete_history(LCASE(cmd[11]) == 'f');
				split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200); split = -200;
			}
			else if(_IS(cmd, "REMOVEHOOK"))	// PS3 REMOVEHOOK
			{
				{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_REMOVE_HOOK); }
				split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);  split = -200;
			}
			else if(_IS(cmd, "GETIDPS"))	// PS3 GETIDPS
			{
				u64 _new_idps[2] = {0, 0};
				{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_IDPS, (u64)(u32)_new_idps);}
				sprintf(param2, "%016llX%016llX", _new_idps[0], _new_idps[1]);
				split = ps3mapi_response_str(conn_s_ps3mapi, buffer, param2, true);
			}
			#ifdef OVERCLOCKING
			else if(_IS(cmd, "GETRSXCLOCK"))	// PS3 GETRSXCLOCK
			{
				sprintf(param2, "%i|%i", get_rsxclock(GPU_CORE_CLOCK), get_rsxclock(GPU_VRAM_CLOCK));
				split = ps3mapi_response_values(conn_s_ps3mapi, buffer, param2);
			}
			else if(_IS(cmd, "SETGPUCLOCK"))	// PS3 SETGPUCLOCK <mhz>
			{
				if(split)
				{
					u16 mhz = (u16)val(param2); overclock(mhz, true);

					split = ps3mapi_response_int(conn_s_ps3mapi, buffer, get_rsxclock(GPU_CORE_CLOCK), true);
				}
			}
			else if(_IS(cmd, "SETVRAMCLOCK"))	// PS3 SETVRAMCLOCK <mhz>
			{
				if(split)
				{
					u16 mhz = (u16)val(param2); overclock(mhz, false);

					split = ps3mapi_response_int(conn_s_ps3mapi, buffer, get_rsxclock(GPU_VRAM_CLOCK), true);
				}
			}
			#endif
			else if(_IS(cmd, "SETIDPS"))	// PS3 SETIDPS <part1> <part2>
			{
				if(split)
				{
					split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
					if(split)
					{
						u64 part1 = convertH(cmd);
						u64 part2 = convertH(param1);
						{ system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_IDPS, part1, part2);}
						split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);
					}
				}
			}
			else if(_IS(cmd, "GETPSID"))	// PS3 GETPSID
			{
				u64 _new_psid[2] = {0, 0};
				{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PSID, (u64)(u32)_new_psid);}
				sprintf(param2, "%016llX%016llX", _new_psid[0], _new_psid[1]);
				split = ps3mapi_response_str(conn_s_ps3mapi, buffer, param2, true);
			}
			else if(_IS(cmd, "SETPSID"))	// PS3 SETPSID <part1> <part2>
			{
				if(split)
				{
					split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
					if(split)
					{
						u64 part1 = convertH(cmd);
						u64 part2 = convertH(param1);
						{ system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PSID, (u64)part1, (u64)part2);}
						split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);
					}
				}
			}
			else {split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_502);}
		}

		if(!split) split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_501);
	}
	else if(_IS(cmd, "PROCESS"))
	{
		if(split)
		{
			split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);
			if(_IS(cmd, "GETNAME"))	// PROCESS GETNAME <pid>
			{
				if(split)
				{
					u32 pid = val(param2);
					ps3mapi_get_process_name_by_id(pid, param2, sizeof(param2));
					ps3mapi_response_str(conn_s_ps3mapi, buffer, param2, true);
				}
			}
			else if(_IS(cmd, "GETALLPID")) // PROCESS GETALLPID
			{
				u32 pid_list[MAX_PID];
				u32 buf_len = 0;
				{system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (u64)(u32)pid_list); }
				for(int i = 0; i < MAX_PID; i++)
				{
					buf_len += sprintf(param2 + buf_len, (!pid_list[i] || conn_s_ps3mapi) ? "%i|" : "0x%x", pid_list[i]);
				}

				split = ps3mapi_response_values(conn_s_ps3mapi, buffer, param2);
			}
			else if(_IS(cmd, "GETCURRENTPID")) // PROCESS GETCURRENTPID
			{
				split = ps3mapi_response_long(conn_s_ps3mapi, buffer, (u64)get_current_pid(), false);
			}
			else {split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_502);}
		}

		if(!split) split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_501);
	}
	else if(_IS(cmd, "MEMORY"))
	{
		if(split)
		{
			split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);

			if(_IS(cmd, "GET"))	// MEMORY GET <pid> <offset> <size>
			{
				if(data_s < 0 && pasv_s >= 0) data_s = accept(pasv_s, NULL, NULL);

				if(data_s >= 0)
				{
					if(split)
					{
						split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
						if(split)
						{
							u32 attached_pid = val(cmd);
							split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);

							if(split)
							{
								u32 offset = (u32)convertH(cmd);
								u32 size = val(param2);

								int rr = -4;
								sys_addr_t sysmem = sys_mem_allocate(BUFFER_SIZE_PS3MAPI);
								if(sysmem)
								{
									char *buffer2 = (char *)sysmem;
									if(conn_s_ps3mapi)
										split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_150);

									rr = 0;

									if((size > BUFFER_SIZE_PS3MAPI) && conn_s_ps3mapi)
									{
										u32 sizetoread = BUFFER_SIZE_PS3MAPI;
										u32 leftsize = size;

										while(leftsize)
										{
											if(!working) break;
											if(sizetoread > leftsize) sizetoread = leftsize;
											ps3mapi_get_memory(attached_pid, offset, buffer2, sizetoread);
											if(send(data_s, buffer2, sizetoread, 0) < 0) { rr = -3; break; }
											offset   += sizetoread;
											leftsize -= sizetoread;
											if(sizetoread == 0) break;
										}
									}
									else
									{
										ps3mapi_get_memory(attached_pid, offset, buffer2, size);

										if(conn_s_ps3mapi)
										{
											if(send(data_s, buffer2, size, 0) < 0) rr = -3;
										}
										else
										{
											// convert buffer2 to hex data (maximum data size 16KB binary => 32KB hex)
											char *data = buffer2 + 0x5500; u32 i;
											for(i = 0; i < (2 * size) && i < (0xAA00 - 22); buffer2++)
											{
												data[i++] = h2a((*buffer2 & 0xF0)>>8);
												data[i++] = h2a((*buffer2 & 0x0F));
											}
											data[i] = 0;
	
											split = ps3mapi_response_str(conn_s_ps3mapi, buffer, data, true);
										}
									}

									sys_memory_free(sysmem);
								}

								if(!conn_s_ps3mapi) ;
								else if(rr ==  0) split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_226);
								else if(rr == -4) split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_550);
								else split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_451);
							}
						}
					}
				}
				else {split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_425);}
			}
			else if(_IS(cmd, "SET"))	// MEMORY SET <pid> <offset>
			{
				if(data_s < 0 && pasv_s >= 0) data_s = accept(pasv_s, NULL, NULL);

 				if(data_s >= 0)
				{
					if(split)
					{
						split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
						if(split)
						{
							u32 attached_pid = val(cmd);
							u32 offset = (u32)convertH(param1);

							int rr = NONE;
							int read_e = 0;

							if(conn_s_ps3mapi)
							{
								sys_addr_t sysmem = sys_mem_allocate(BUFFER_SIZE_PS3MAPI);
								if(sysmem)
								{
									char *buffer2 = (char*)sysmem;
									split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_150);

									rr = 0;
									while(working)
									{
										if((read_e = (u32)recv(data_s, buffer2, BUFFER_SIZE_PS3MAPI, MSG_WAITALL)) > 0)
										{
											ps3mapi_patch_process(attached_pid, offset, buffer2, read_e, 0);
											offset += read_e;
										}
										else
										{
											break;
										}
									}
									sys_memory_free(sysmem);
								}
							}
							else
							{
								char *data = strchr(param1, ' ');
								if(data)
								{
									++data;
									char *buffer1 = data;
									char *buffer2 = data;
									for(read_e = 0; *buffer2 && (read_e < PS3MAPI_MAX_LEN - 0x20); read_e++)
									{
										*buffer1 = (buffer2[0] << 8) | buffer2[1]; // hex to bin
										buffer1++, buffer2 += 2;
									}

									if(read_e)
										rr = ps3mapi_patch_process(attached_pid, offset, data, read_e, 0);
								}
							}
							if(rr == 0) split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_226);
							else split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_451);
						}
					}
				}
				else {split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_425);}
			}
			else if(_IS(cmd, "PAYLOAD"))	// MEMORY PAYLOAD <pid> <payload-path>
			{								// MEMORY PAYLOAD <pid> unload
				if(data_s < 0 && pasv_s >= 0) data_s = accept(pasv_s, NULL, NULL);

 				if(data_s >= 0)
				{
					if(split)
					{
						split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
						if(split)
						{
							if(_IS(param1, "unload"))
							{
								u32 attached_pid = val(cmd);
								if(attached_pid && pageTable[0] && pageTable[1])
								{
									ps3mapi_process_page_free(attached_pid, 0x2F, pageTable);
									pageTable[0] = pageTable[1] = 0;
									split = ps3mapi_response_str(conn_s_ps3mapi, buffer, "Payload unloaded", true);
								}
								else
									split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_451);
							}
							else if(file_exists(param1))
							{
								u32 attached_pid = val(cmd);
								char *error_msg = param2;
								uint64_t executableMemoryAddress = StartGamePayload(attached_pid, param1, 0x7D0, 0x4000, pageTable, error_msg);
								if(conn_s_ps3mapi)
									sprintf(param1, "%llu|%s\r\n", executableMemoryAddress, error_msg);
								else
									sprintf(param1, "[\"0x%llX\",\"%s\"]", executableMemoryAddress, error_msg);

								split = ps3mapi_response_str(conn_s_ps3mapi, buffer, param1, false);
							}
							else
								split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_451);
						}
					}
				}
				else {split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_425); split = 425;}
			}
			else if (_IS(cmd, "PAGEALLOCATE"))	// MEMORY PAGEALLOCATE <pid> <size> <page_size> <flags> <is_executable>
			{
				if (split)
				{
					split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
					if (split)
					{
						u32 pid = val(cmd);
						split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);
						if (split)
						{
							u32 size = (u32)val(cmd);
							split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
							if (split)
							{
								u64 page_size = convertH(cmd);
								split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);
								if (split)
								{
									u32 flags = (u32)val(cmd);
									u32 is_executable = (u32)val(param2);

									u64 page_table[2] = { 0, 0 };
									{system_call_8(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PROC_PAGE_ALLOCATE, (u64)pid, (u64)size, (u64)page_size, (u64)flags, (u64)is_executable, (u64)(u32)page_table); }

									if(conn_s_ps3mapi)
										sprintf(param2, "%016llX|%016llX", page_table[0], page_table[1]);
									else
										sprintf(param2, "[\"%016llX\",\"0x%016llX\"]", page_table[0], page_table[1]);

									split = ps3mapi_response_str(conn_s_ps3mapi, buffer, param2, false);
								}
							}
						}
					}
				}
			}
			else if (_IS(cmd, "PAGEFREE"))	// MEMORY PAGEFREE <pid> <flags> <page_table_0> <page_table_1>
			{
				if (split)
				{
					split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
					if (split)
					{
						u32 pid = val(cmd);
						split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);
						if (split)
						{
							u32 flags = (u32)val(cmd);
							split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
							if (split)
							{
								u64 page_table_0 = convertH(cmd);
								u64 page_table_1 = convertH(param1);

								u64 _page_table[2];
								_page_table[0] = page_table_0;
								_page_table[1] = page_table_1;
								{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PROC_PAGE_FREE, (u64)pid, (u64)flags, (u64)(u32)_page_table); }

								split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);
							}
						}
					}
				}
			}
			else {split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_502);}
		}

		if(!split) split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_501);
	}
	else if(_IS(cmd, "MODULE"))
	{
		if(split)
		{
			split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);
			if(_IS(cmd, "GETNAME") || _IS(cmd, "GETFILENAME"))	// MODULE GETNAME <pid> <prxid>
			{													// MODULE GETFILENAME <pid> <prxid>
				if(split)
				{
					split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
					if(split)
					{
						u32 pid = val(cmd);
						s32 prxid = val(param1);
						_memset(param2, sizeof(param2));

						if(_IS(cmd, "GETFILENAME"))
							{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_FILENAME, (u64)pid, (u64)prxid, (u64)(u32)param2); }
						else
							{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_NAME, (u64)pid, (u64)prxid, (u64)(u32)param2); }

						split = ps3mapi_response_str(conn_s_ps3mapi, buffer, param2, true);
					}
				}
			}
			else if(_IS(cmd, "GETALLPRXID"))	// MODULE GETALLPRXID <pid>
			{
				if(split)
				{
					s32 prxid_list[128];
					u32 pid = val(param2);
					u32 buf_len = 0;
					{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_MODULE_PID, (u64)pid, (u64)(u32)prxid_list); }
					for(u8 i = 0; i < 128; i++)
					{
						buf_len += sprintf(param2 + buf_len, (!prxid_list[i] || conn_s_ps3mapi) ? "%i|" : "0x%x", prxid_list[i]);
					}

					split = ps3mapi_response_values(conn_s_ps3mapi, buffer, param2);
				}
			}
			else if(_IS(cmd, "LOAD") || _IS(cmd, "UNLOAD"))	// MODULE UNLOAD <pid> <prx-id>
			{												// MODULE LOAD <pid> <prx_path>
				if(split)
				{
					split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
					if(split)
					{
						u32 pid = val(cmd);

						if(_IS(cmd, "UNLOAD"))
						{
							s32 prx_id = val(param1); int ret;
							{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_UNLOAD_PROC_MODULE, (u64)pid, (u64)prx_id); ret = p1;}
							if(ret) stop_unload(prx_id); // <- unload system modules
						}
						else
						{
							char *prx_path = param1;
							check_path_alias(prx_path);
							if(strstr(prx_path, "/dev_flash"))
								load_start(prx_path); // <- load system modules from flash to process
							else
								{system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LOAD_PROC_MODULE, (u64)pid, (u64)(u32)prx_path, NULL, 0); } // <- load custom modules to process
						}

						split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);
					}
				}
			}
			else if(_IS(cmd, "LOADVSHPLUG"))	// MODULE LOADVSHPLUGS <slot> <prx-path>
			{
				if(split)
				{
					split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
					if(split)
					{
						char *prx_path = param1;
						check_path_alias(prx_path);
						unsigned int slot = val(cmd);
						if(!slot ) slot = get_free_slot();
						if( slot ) cobra_load_vsh_plugin(slot, prx_path, NULL, 0);
						split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);
					}
				}
			}
			else if(_IS(cmd, "UNLOADVSHPLUGS"))	// MODULE UNLOADVSHPLUGS <slot>
			{
				if(split)
				{
					unsigned int slot = val(param2);
					ps3mapi_check_unload(slot, param1, param2);
					if( slot ) cobra_unload_vsh_plugin(slot);
					split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);
				}
			}
			else if(_IS(cmd, "GETVSHPLUGINFO"))	// MODULE GETVSHPLUGINFO <slot>
			{
				if(split)
				{
					unsigned int slot = val(param2);
					ps3mapi_get_vsh_plugin_info(slot, param1, param2);
					if(conn_s_ps3mapi)
						sprintf(buffer, "%s|%s\r\n", param1, param2);
					else
						sprintf(buffer, "[\"%s\",\"%s\"]", param1, param2);

					strcopy(param2, buffer);
					split = ps3mapi_response_str(conn_s_ps3mapi, buffer, param2, false);
				}
			}
			else if(_IS(cmd, "GETSEGMENTS"))	// MODULE GETSEGMENTS <pid> <prx-id>
			{
				if (split)
				{
					split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
					if (split)
					{
						sys_prx_module_info_t moduleInfo;
						sys_prx_segment_info_t moduleSegments[10];
						char filename[SYS_PRX_MODULE_FILENAME_SIZE];
						_memset(filename, sizeof(filename));
						_memset(moduleSegments, sizeof(moduleSegments));

						moduleInfo.size = sizeof(moduleInfo);
						moduleInfo.segments = moduleSegments;
						moduleInfo.segments_num = sizeof(moduleSegments) / sizeof(sys_prx_segment_info_t);
						moduleInfo.filename = filename;
						moduleInfo.filename_size = sizeof(filename);
						_memset(moduleInfo.segments, sizeof(moduleInfo.segments));
						_memset(moduleInfo.filename, sizeof(moduleInfo.filename));

						u32 pid = val(cmd);
						s32 prx_id = val(param1);

						{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_SEGMENTS, (u64)pid, (u64)prx_id, (u64)(u32)&moduleInfo); }

						u32 buf_len = sprintf(param2, conn_s_ps3mapi ? "%s|%s|%i|%i|%i|%i|" : "\"%s\"|\"%s\"|%i|%i|%i|%i|",
							moduleInfo.name, moduleInfo.filename, moduleInfo.filename_size,
							moduleInfo.modattribute, moduleInfo.start_entry, moduleInfo.stop_entry);
						for(u8 s = 0; s < 4; s++)
							buf_len += sprintf(param2 + buf_len, conn_s_ps3mapi ? "seg%i|%016llX|%016llX|%016llX|%016llX|%016llX" :
																	"\"seg%i\"|0x%016llX|0x%016llX|0x%016llX|0x%016llX|0x%016llX", s,
								moduleInfo.segments[s].base,  moduleInfo.segments[s].filesz, moduleInfo.segments[s].memsz,
								moduleInfo.segments[s].index, moduleInfo.segments[s].type);

						split = ps3mapi_response_values(conn_s_ps3mapi, buffer, param2);
					}
				}
			}
			else {split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_502);}
		}

		if(!split) split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_501);
	}
	else if (_IS(cmd, "THREAD"))
	{
		if (split)
		{
			split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);
			if (_IS(cmd, "CREATE"))	// THREAD CREATE <pid> <page_table_0> <toc> <arg> <prio> <stack_size> <name>
			{
				if (split)
				{
					split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
					if (split)
					{
						u32 pid = val(cmd);
						split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);
						if (split)
						{
							u64 page_table_0 = convertH(cmd);
							split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
							if (split)
							{
								u64 toc_memory_address = convertH(cmd);
								split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);
								if (split)
								{
									u64 thread_arg = convertH(cmd);
									split = ssplit(param2, cmd, PS3MAPI_CMD_LEN, param1, PS3MAPI_MAX_LEN);
									if (split)
									{
										u32 thread_prio = val(cmd);
										split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);
										if (split)
										{
											u32 thread_stack_size = (u32)val(cmd);
											char* thread_name = param2;

											u64 threadOpd[2];
											threadOpd[0] = page_table_0; // assuming main() is the first function in page_table_0
											threadOpd[1] = toc_memory_address;

											typedef struct
											{
												void* unk_0; // ptr to some funcs
												u64 unk_8;
												u32 unk_10;
												u32 unk_14;
												void* unk_18;
												void* unk_20; // same as unk_18? :S
												u64 unk_28[3];
												void* unk_40; // same as unk_0?
												// ...
											} thread_t;

											thread_t thrd;
											{system_call_8(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PROC_CREATE_THREAD, (u64)pid, (u64)(u32)&thrd, (u64)(u32)threadOpd, (u64)thread_arg, (u64)thread_prio, (u64)thread_stack_size, (u64)(u32)thread_name); }

											split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);
										}
									}
								}
							}
						}
					}
				}
			}
			else split = 0;
		}

		if(!split) split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_501);
	}
	else if(_IS(cmd, "FILE"))
	{
		if(split)
		{
			split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);
			if(split)
			{
				char *filename = param2;
				check_path_alias(filename);

				if(_IS(cmd, "EXISTS")) // FILE EXISTS <file-path>
				{
					int check = file_exists(filename);
					split = ps3mapi_response_int(conn_s_ps3mapi, buffer, check, true);
				}
				else if(_IS(cmd, "ISDIR")) // FILE ISDIR <file-path>
				{
					int check = isDir(filename);
					split = ps3mapi_response_int(conn_s_ps3mapi, buffer, check, true);
				}
				else if(_IS(cmd, "SIZE")) // FILE SIZE <file-path>
				{
					u64 sz = file_size(filename);
					split = ps3mapi_response_long(conn_s_ps3mapi, buffer, sz, true);
				}
				#ifdef CALC_MD5
				else if(_IS(cmd, "MD5")) // FILE MD5 <file-path>
				{
					u64 sz = file_size(filename);

					char md5[33];
					calc_md5(filename, md5);

					if(conn_s_ps3mapi)
						sprintf(param1, "%llu|%s\r\n", sz, md5);
					else
						sprintf(param1, "{\"size\":%llu,\"md5\":\"%s\"}", sz, md5);

					split = ps3mapi_response_str(conn_s_ps3mapi, buffer, param1, false);
				}
				#endif
			}
		}
	}
	#ifdef DEBUG_XREGISTRY
	else if(_IS(cmd, "REGISTRY"))
	{
		if(split)
		{
			split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);
			if(split)
			{
				if(_IS(cmd, "GET")) // REGISTRY GET <regkey>
				{
					_memset(param1, PS3MAPI_MAX_LEN);
					int value = get_xreg_value(param2, 0, param1, true);
					if(*param1)
						split = ps3mapi_response_str(conn_s_ps3mapi, buffer, param1, true);
					else
						split = ps3mapi_response_int(conn_s_ps3mapi, buffer, value, false);
				}
				else if(_IS(cmd, "SET")) // REGISTRY SET <regkey> <value>
				{
					split = ssplit(param2, param1, PS3MAPI_MAX_LEN, buffer, PS3MAPI_MAX_LEN);
					if(split)
					{
						int value = val(buffer);
						get_xreg_value(param1, value, buffer, false);
						split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);
					}
				}
			}
		}

		if(!split) split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_501);
	}
	#endif // #ifdef DEBUG_XREGISTRY

	//// Code: @PHTNCx64 2021/09/21
	else if(_IS(cmd, "PEEKLV2") || (_IS(cmd, "PEEKLV1")))	// PEEKLV1 <address>
	{														// PEEKLV2 <address>
		if(split)
		{
			bool isLV2Peek = (cmd[6] == '2');

			system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, isLV2Peek ? PS3MAPI_OPCODE_LV2_PEEK : PS3MAPI_OPCODE_LV1_PEEK, val(param1));
			split = ps3mapi_response_long(conn_s_ps3mapi, buffer, p1, false);
		}
		else split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_501);
	}
	else if(_IS(cmd, "POKELV2") || (_IS(cmd, "POKELV1")))	// POKELV1 <address> <value>
	{														// POKELV2 <address> <value>
		if(split)
		{
			bool isLV2Peek = (cmd[6] == '2');

			split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);
			if(split)
			{
				system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, isLV2Peek ? PS3MAPI_OPCODE_LV2_POKE : PS3MAPI_OPCODE_LV1_POKE, val(cmd), val(param2));
				split = ps3mapi_response_long(conn_s_ps3mapi, buffer, p1, false);
			}
		}

		if(!split) split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_501);
	}
	else if(_IS(cmd, "SYSCALL"))	// SYSCALL <syscall-number>|0x<hex-value>|<decimal-value>|<string-value>
	{
		if(split)
		{
			split = ssplit(param1, cmd, PS3MAPI_CMD_LEN, param2, PS3MAPI_MAX_LEN);

			u64 sp[9]; u8 n;
			u16 sc = parse_syscall(param2, sp, &n);
			u64 ret = call_syscall(sc, sp, n);

			split = ps3mapi_response_long(conn_s_ps3mapi, buffer, ret, false);
		}
		else split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_501);
	}
	else if(!conn_s_ps3mapi && (*buffer != '{'))
	{
		split = ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_502);
	}

	return split;
}

static void handleclient_ps3mapi(u64 conn_s_ps3mapi_p)
{
	int conn_s_ps3mapi = (int)conn_s_ps3mapi_p; // main communications socket
	int data_s = NONE;							// data socket
	int pasv_s = NONE;

	int connactive = 1;							// whether the ps3mapi connection is active or not
	int dataactive = 0;							// prevent the data connection from being closed at the end of the loop

	char buffer[PS3MAPI_RECV_SIZE + 1];

	sys_net_sockinfo_t conn_info;
	sys_net_get_sockinfo(conn_s_ps3mapi, &conn_info, 1);

	char ip_address[16];
	char pasv_output[56];

	setPluginActive();

	#define PS3MAPI_OK_220		"220 OK: PS3 Manager API Server v1.\r\n"
	#define PS3MAPI_OK_221		"221 OK: Service closing control connection.\r\n"
	#define PS3MAPI_OK_230		"230 OK: Connected to PS3 Manager API Server.\r\n"
	
	#define PS3MAPI_ERROR_421	"421 Could not create socket\r\n"

	#define PS3MAPI_CONNECT_NOTIF 	 "PS3MAPI: Client connected [%s]\r\n"
	#define PS3MAPI_DISCONNECT_NOTIF "PS3MAPI: Client disconnected [%s]\r\n"

	ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_220);

	u8 ip_len = strcopy(ip_address, inet_ntoa(conn_info.local_adr));
	for(u8 n = 0; n < ip_len; n++) if(ip_address[n] == '.') ip_address[n] = ',';

	ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_230);

	sprintf(buffer, PS3MAPI_CONNECT_NOTIF, inet_ntoa(conn_info.remote_adr)); show_msg_with_icon(ICON_NETWORK, buffer);

	while(connactive == 1 && working)
	{
		_memset(buffer, sizeof(buffer));
		if(working && (recv(conn_s_ps3mapi, buffer, PS3MAPI_RECV_SIZE, 0) > 0))
		{
			if(!get_flag(buffer, "\r\n")) break;

			#ifdef WM_REQUEST
			if(islike(buffer, "GET")) // e.g.  GET /install.ps3<pkg-path>
				strcopy(buffer, buffer + 4);
			if(*buffer == '/') 
			{
				handle_file_request(buffer); // e.g. /install.ps3<pkg-path>

				ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);
				continue;
			}
			#endif

			////
			if(_IS(buffer, "DISCONNECT"))	// DISCONNECT
			{
				ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_221);
				connactive = 0;
			}
			else if(_islike(buffer, "TYPE"))	// TYPE <A/I>
			{
				if(buffer[4])
				{
					dataactive = (buffer[5] != 'I');
					ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_OK_200);
				}
				else
				{
					dataactive = 1;
				}
			}
			else if(_islike(buffer, "PASV"))
			{
				CellRtcTick pTick; cellRtcGetCurrentTick(&pTick);
				u8 pasv_retry = 0; u32 pasv_port = (pTick.tick & 0xfeff00) >> 8;

				for(int p1x, p2x; pasv_retry < 250; pasv_retry++, pasv_port++)
				{
					if(data_s >= 0) sclose(&data_s);
					if(pasv_s >= 0) sclose(&pasv_s);

					bool kernel_full = wait_for_new_socket();

					if(kernel_full)
					{
						// Kernel space is full, tell client to retry later
						ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_421);
						break; // Abort PASV command
					}

					p1x = ( (pasv_port & 0xff00) >> 8) | 0x80; // use ports 32768 -> 65528 (0x8000 -> 0xFFF8)
					p2x = ( (pasv_port & 0x00ff)	 );

					pasv_s = slisten(getPort(p1x, p2x), 1);

					if(pasv_s >= 0)
					{
						sprintf(pasv_output, "227 Entering Passive Mode (%s,%i,%i)\r\n", ip_address, p1x, p2x);
						ps3mapi_response_status(conn_s_ps3mapi, buffer, pasv_output);

						if((data_s = accept(pasv_s, NULL, NULL)) >= 0)
						{
							dataactive = 1; break;
						}
					}
				}

				if(pasv_retry >= 250)
				{
					ps3mapi_response_status(conn_s_ps3mapi, buffer, FTP_ERROR_451);	// Requested action aborted. Local error in processing.
					if(pasv_s >= 0) sclose(&pasv_s);
					pasv_s = NONE;
				}
			}
			else if(ps3mapi_command(conn_s_ps3mapi, data_s, pasv_s, buffer) == 0)
				ps3mapi_response_status(conn_s_ps3mapi, buffer, PS3MAPI_ERROR_502);

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
	show_msg_with_icon(ICON_NETWORK, buffer);

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
		if(!working) goto end;

		list_s = slisten(PS3MAPIPORT, PS3MAPI_BACKLOG);

		if(list_s < 0)
		{
			sys_ppu_thread_sleep(1);
			goto relisten;
		}

		active_socket[2] = list_s;

		//if(list_s >= 0)
		{
			while(ps3mapi_working)
			{
				sys_ppu_thread_usleep(100000);
				if (!working || !ps3mapi_working) goto end;

				int conn_s_ps3mapi;
				if(sys_admin && ((conn_s_ps3mapi = accept(list_s, NULL, NULL)) >= 0))
				{
					if(!working) {sclose(&conn_s_ps3mapi); break;}

					sys_ppu_thread_t t_id;
					sys_ppu_thread_create(&t_id, handleclient_ps3mapi, (u64)conn_s_ps3mapi, THREAD_PRIO, THREAD_STACK_SIZE_PS3MAPI_CLI, SYS_PPU_THREAD_CREATE_NORMAL, THREAD02_NAME_PS3MAPI);
				}
				else if((sys_net_errno == SYS_NET_EBADF) || (sys_net_errno == SYS_NET_ENETDOWN))
				{
					sclose(&list_s);
					goto relisten;
				}
			}
		}
end:
		sclose(&list_s);
	}
	else show_msg_with_icon(ICON_EXCLAMATION, (char *)"PS3MAPI Server not loaded!");

	//thread_id_ps3mapi = SYS_PPU_THREAD_NONE;
	sys_ppu_thread_exit(0);
}

#endif // #ifdef PS3MAPI

