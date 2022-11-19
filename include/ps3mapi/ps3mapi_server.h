///////////// PS3MAPI BEGIN //////////////

#define THREAD_NAME_PS3MAPI				"ps3m_api_server"
#define THREAD02_NAME_PS3MAPI			"ps3m_api_client"

#define PS3MAPI_RECV_SIZE  2048

#define PS3MAPI_MAX_LEN	383

#ifdef PS3MAPI
static u32 BUFFER_SIZE_PS3MAPI = (_64KB_);

static sys_ppu_thread_t thread_id_ps3mapi = SYS_PPU_THREAD_NONE;

static void handleclient_ps3mapi(u64 conn_s_ps3mapi_p)
{
	int conn_s_ps3mapi = (int)conn_s_ps3mapi_p; // main communications socket
	int data_s = NONE;							// data socket
	int pasv_s = NONE;

	int connactive = 1;							// whether the ps3mapi connection is active or not
	int dataactive = 0;							// prevent the data connection from being closed at the end of the loop

	char buffer[PS3MAPI_RECV_SIZE + 1];
	char cmd[20], param1[PS3MAPI_MAX_LEN + 1], param2[PS3MAPI_MAX_LEN + 1];

	#define PS3MAPI_OK_150	"150 OK: Binary status okay; about to open data connection.\r\n"
	#define PS3MAPI_OK_200	"200 OK: The requested action has been successfully completed.\r\n"
	#define PS3MAPI_OK_220	"220 OK: PS3 Manager API Server v1.\r\n"
	#define PS3MAPI_OK_221	"221 OK: Service closing control connection.\r\n"
	#define PS3MAPI_OK_226	"226 OK: Closing data connection. Requested binary action successful.\r\n"
	#define PS3MAPI_OK_230	"230 OK: Connected to PS3 Manager API Server.\r\n"

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

	sprintf(buffer, PS3MAPI_CONNECT_NOTIF, inet_ntoa(conn_info.remote_adr)); show_msg_with_icon(ICON_NETWORK, buffer);

	while(connactive == 1 && working)
	{
		_memset(buffer, sizeof(buffer));
		if(working && (recv(conn_s_ps3mapi, buffer, PS3MAPI_RECV_SIZE, 0) > 0))
		{
			if(!get_flag(buffer, "\r\n")) break;

			#ifdef WM_REQUEST
			if((*buffer == '/') || islike(buffer, "GET"))
			{
				save_file(WM_REQUEST_FILE, buffer, SAVE_ALL); // e.g.  GET /install.ps3<pkg-path>

				do_custom_combo(WM_REQUEST_FILE);

				ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
				continue;
			}
			#endif
			int split = ssplit(buffer, cmd, 19, param1, PS3MAPI_MAX_LEN);

			if(_IS(cmd, "DISCONNECT"))	// DISCONNECT
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
					if(_IS(cmd, "GETVERSION"))	// CORE GETVERSION
					{							// SERVER GETVERSION
						int version = PS3MAPI_SERVER_VERSION;
						if(is_core) { system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_VERSION); version = (int)(p1); }
						sprintf(buffer, "200 %i\r\n", version);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(_IS(cmd, "GETMINVERSION"))	// CORE GETMINVERSION
					{									// SERVER GETMINVERSION
						int version = PS3MAPI_SERVER_MINVERSION;
						if(is_core) { system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_MINVERSION); version = (int)(p1); }
						sprintf(buffer, "200 %i\r\n", version);
						ssend(conn_s_ps3mapi, buffer);
					}
					else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502);
				}
				else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
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
						split = sprintf(buffer, "200 %i\r\n", version);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(_IS(cmd, "GETFWTYPE"))	// PS3 GETFWTYPE
					{
						_memset(param2, sizeof(param2));
						{system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_FW_TYPE, (u64)(u32)param2); }
						split = sprintf(buffer, "200 %s\r\n", param2);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(_IS(cmd, "GETSYSINFO"))	// PS3 GETSYSINFO <id>
					{
						get_sys_info(param2, val(param2), true);
						split = sprintf(buffer, "200 %s\r\n", param2);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(_IS(cmd, "NOTIFY"))	// PS3 NOTIFY <msg>&icon=<0-50>&snd=<0-9>
					{
						if(split)
						{
							show_msg(param2);
							ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						}
					}
					else if(islike(cmd, "BUZZER"))	// PS3 BUZZER<0-9>
					{
						play_sound_id(cmd[6]);

						ssend(conn_s_ps3mapi, PS3MAPI_OK_200); split = -200;
					}
					else if(_IS(cmd, "LED"))	// PS3 LED <color> <mode>
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
						}
					}
					else if(_IS(cmd, "GETTEMP"))	// PS3 GETTEMP
					{
						u8 cpu_temp = 0;
						u8 rsx_temp = 0;
						get_temperature(0, &cpu_temp);
						get_temperature(1, &rsx_temp);
						split = sprintf(buffer, "200 %i|%i\r\n", cpu_temp, rsx_temp);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(_IS(cmd, "DISABLESYSCALL"))	// PS3 DISABLESYSCALL <sc-num>
					{
						if(split)
						{
							int num = val(param2);
							{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, (u64)num); }
							ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						}
					}
					else if(_IS(cmd, "CHECKSYSCALL"))	// PS3 CHECKSYSCALL <sc-num>
					{
						if(split)
						{
							int num = val(param2);
							int check = 0;
							{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_CHECK_SYSCALL, (u64)num); check = (int)(p1); }
							sprintf(buffer, "200 %i\r\n", check);
							ssend(conn_s_ps3mapi, buffer);
						}
					}
					else if(_IS(cmd, "PDISABLESYSCALL8"))	// PS3 PDISABLESYSCALL8 <mode>
					{
						if(split)
						{
							int mode = val(param2);
							{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, (u64)mode); }
							ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						}
					}
					else if(_IS(cmd, "PCHECKSYSCALL8"))	// PS3 PCHECKSYSCALL8
					{
						int check = 0;
						{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PCHECK_SYSCALL8); check = (int)(p1); }
						split = sprintf(buffer, "200 %i\r\n", check);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(islike(cmd, "DELHISTORY"))	// PS3 DELHISTORY
					{									// PS3 DELHISTORY+F
						delete_history(_IS(cmd, "DELHISTORY+F"));
						ssend(conn_s_ps3mapi, PS3MAPI_OK_200); split = -200;
					}
					else if(_IS(cmd, "REMOVEHOOK"))	// PS3 REMOVEHOOK
					{
						{ system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_REMOVE_HOOK); }
						ssend(conn_s_ps3mapi, PS3MAPI_OK_200);  split = -200;
					}
					else if(_IS(cmd, "GETIDPS"))	// PS3 GETIDPS
					{
						u64 _new_idps[2] = {0, 0};
						{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_IDPS, (u64)(u32)_new_idps);}
						split = sprintf(buffer, "200 %016llX%016llX\r\n", _new_idps[0], _new_idps[1]);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(_IS(cmd, "SETIDPS"))	// PS3 SETIDPS <part1> <part2>
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
						}
					}
					else if(_IS(cmd, "GETPSID"))	// PS3 GETPSID
					{
						u64 _new_psid[2] = {0, 0};
						{ system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PSID, (u64)(u32)_new_psid);}
						split = sprintf(buffer, "200 %016llX%016llX\r\n", _new_psid[0], _new_psid[1]);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(_IS(cmd, "SETPSID"))	// PS3 SETPSID <part1> <part2>
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
						}
					}
					else {ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502); split = 502;}
				}

				if(!split) ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
			}
			else if(_IS(cmd, "PROCESS"))
			{
				if(split)
				{
					split = ssplit(param1, cmd, 19, param2, PS3MAPI_MAX_LEN);
					if(_IS(cmd, "GETNAME"))	// PROCESS GETNAME <pid>
					{
						if(split)
						{
							u32 pid = val(param2);
							ps3mapi_get_process_name_by_id(pid, param2, sizeof(param2));
							sprintf(buffer, "200 %s\r\n", param2);
							ssend(conn_s_ps3mapi, buffer);
						}
					}
					else if(_IS(cmd, "GETALLPID")) // PROCESS GETALLPID
					{
						u32 pid_list[16];
						u32 buf_len = sprintf(buffer, "200 ");
						{system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (u64)(u32)pid_list); }
						for(int i = 0; i < 16; i++)
						{
							buf_len += sprintf(buffer + buf_len, "%i|", pid_list[i]);
						}
						buf_len += sprintf(buffer + buf_len, "\r\n");
						send(conn_s_ps3mapi, buffer, buf_len, 0); split = 200;
					}
					else if(_IS(cmd, "GETCURRENTPID")) // PROCESS GETCURRENTPID
					{
						u32 buf_len = sprintf(buffer, "200 %u\r\n", get_current_pid());
						send(conn_s_ps3mapi, buffer, buf_len, 0); split = 200;
					}
					else {ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502); split = 502;}
				}

				if(!split) ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
			}
			else if(_IS(cmd, "MEMORY"))
			{
				if(split)
				{
					split = ssplit(param1, cmd, 19, param2, PS3MAPI_MAX_LEN);
					if(_IS(cmd, "GET"))	// MEMORY GET <pid> <offset> <size>
					{
						if(data_s < 0 && pasv_s >= 0) data_s = accept(pasv_s, NULL, NULL);

						if(data_s >= 0)
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
										int rr = -4;
										sys_addr_t sysmem = sys_mem_allocate(BUFFER_SIZE_PS3MAPI);
										if(sysmem)
										{
											char *buffer2 = (char *)sysmem;
											ssend(conn_s_ps3mapi, PS3MAPI_OK_150);

											u32 offset = (u32)convertH(param1);
											u32 size = val(param2);
											rr = 0;

											if(size > BUFFER_SIZE_PS3MAPI)
											{
												u32 sizetoread = BUFFER_SIZE_PS3MAPI;
												u32 leftsize = size;

												while(leftsize)
												{
													if(!working) break;
													ps3mapi_get_memory(attached_pid, offset, buffer2, sizetoread);
													if(send(data_s, buffer2, sizetoread, 0) < 0) { rr = -3; break; }
													offset   += sizetoread;
													leftsize -= sizetoread;
													if(leftsize < BUFFER_SIZE_PS3MAPI) sizetoread = leftsize;
													if(sizetoread == 0) break;
												}
											}
											else
											{
												ps3mapi_get_memory(attached_pid, offset, buffer2, size);
												if(send(data_s, buffer2, size, 0) < 0) rr = -3;
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
					else if(_IS(cmd, "SET"))	// MEMORY SET <pid> <offset>
					{
						if(data_s < 0 && pasv_s >= 0) data_s = accept(pasv_s, NULL, NULL);

						if(data_s >= 0)
						{
							if(split)
							{
								split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
								if(split)
								{
									int rr = NONE;
									sys_addr_t sysmem = sys_mem_allocate(BUFFER_SIZE_PS3MAPI);
									if(sysmem)
									{
										char *buffer2 = (char*)sysmem;
										int read_e = 0;
										ssend(conn_s_ps3mapi, PS3MAPI_OK_150);

										rr = 0;
										u32 attached_pid = val(param1);
										u32 offset = (u32)convertH(param2);

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
									if(rr == 0) ssend(conn_s_ps3mapi, PS3MAPI_OK_226);
									else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_451);
								}
							}
						}
						else {ssend(conn_s_ps3mapi, PS3MAPI_ERROR_425); split = 425;}
					}
					else if (_IS(cmd, "PAGEALLOCATE"))	// MEMORY PAGEALLOCATE <pid> <size> <page_size> <flags> <is_executable>
					{
						if (split)
						{
							split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
							if (split)
							{
								u32 pid = val(param1);
								split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
								if (split)
								{
									u32 size = (u32)val(param1);
									split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
									if (split)
									{
										u64 page_size = convertH(param1);
										split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
										if (split)
										{
											u32 flags = (u32)val(param1);
											u32 is_executable = (u32)val(param2);

											u64 page_table[2] = { 0, 0 };
											{system_call_8(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PROC_PAGE_ALLOCATE, (u64)pid, (u64)size, (u64)page_size, (u64)flags, (u64)is_executable, (u64)(u32)page_table); }

											split = sprintf(buffer, "200 %016llX|%016llX\r\n", page_table[0], page_table[1]);
											ssend(conn_s_ps3mapi, buffer);
										}
										else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
									}
									else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
								}
								else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if (_IS(cmd, "PAGEFREE"))	// MEMORY PAGEFREE <pid> <flags> <page_table_0> <page_table_1>
					{
						if (split)
						{
							split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
							if (split)
							{
								u32 pid = val(param1);
								split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
								if (split)
								{
									u32 flags = (u32)val(param1);
									split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
									if (split)
									{
										u64 page_table_0 = convertH(param1);
										u64 page_table_1 = convertH(param2);

										u64 _page_table[2];
										_page_table[0] = page_table_0;
										_page_table[1] = page_table_1;
										{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PROC_PAGE_FREE, (u64)pid, (u64)flags, (u64)(u32)_page_table); }

										ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
									}
									else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
								}
								else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else {ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502); split = 502;}
				}

				if(!split) ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
			}
			else if(_IS(cmd, "MODULE"))
			{
				if(split)
				{
					split = ssplit(param1, cmd, 19, param2, PS3MAPI_MAX_LEN);
					if(_IS(cmd, "GETNAME") || _IS(cmd, "GETFILENAME"))	// MODULE GETNAME <pid> <prxid>
					{													// MODULE GETFILENAME <pid> <prxid>
						if(split)
						{
							split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
							if(split)
							{
								u32 pid = val(param1);
								s32 prxid = val(param2);
								_memset(param2, sizeof(param2));

								if(_IS(cmd, "GETFILENAME"))
									{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_FILENAME, (u64)pid, (u64)prxid, (u64)(u32)param2); }
								else
									{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_NAME, (u64)pid, (u64)prxid, (u64)(u32)param2); }

								sprintf(buffer, "200 %s\r\n", param2);
								ssend(conn_s_ps3mapi, buffer);
							}
						}
					}
					else if(_IS(cmd, "GETALLPRXID"))	// MODULE GETALLPRXID <pid>
					{
						if(split)
						{
							s32 prxid_list[128];
							u32 pid = val(param2);
							_memset(buffer, sizeof(buffer));
							u32 buf_len = sprintf(buffer, "200 ");
							{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_MODULE_PID, (u64)pid, (u64)(u32)prxid_list); }
							for(u8 i = 0; i < 128; i++)
							{
								buf_len += sprintf(buffer + buf_len, "%i|", prxid_list[i]);
							}
							buf_len += sprintf(buffer + buf_len, "\r\n");
							send(conn_s_ps3mapi, buffer, buf_len, 0);
						}
					}
					else if(_IS(cmd, "LOAD") || _IS(cmd, "UNLOAD"))	// MODULE UNLOAD <pid> <prx-id>
					{												// MODULE LOAD <pid> <prx_path>
						if(split)
						{
							split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
							if(split)
							{
								u32 pid = val(param1);

								if(_IS(cmd, "UNLOAD"))
								{
									s32 prx_id = val(param2); int ret;
									{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_UNLOAD_PROC_MODULE, (u64)pid, (u64)prx_id); ret = p1;}
									if(ret) stop_unload(prx_id); // <- unload system modules
								}
								else
								{
									char *prx_path = param2;
									check_path_alias(prx_path);
									if(strstr(prx_path, "/dev_flash"))
										load_start(prx_path); // <- load system modules from flash to process
									else
										{system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LOAD_PROC_MODULE, (u64)pid, (u64)(u32)prx_path, NULL, 0); } // <- load custom modules to process
								}

								ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
							}
						}
					}
					else if(_IS(cmd, "LOADVSHPLUG"))	// MODULE LOADVSHPLUGS <slot> <prx-path>
					{
						if(split)
						{
							split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
							if(split)
							{
								char *prx_path = param2;
								check_path_alias(prx_path);
								unsigned int slot = val(param1);
								if(!slot ) slot = get_free_slot();
								if( slot ) cobra_load_vsh_plugin(slot, prx_path, NULL, 0);
								ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
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
							ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						}
					}
					else if(_IS(cmd, "GETVSHPLUGINFO"))	// MODULE GETVSHPLUGINFO <slot>
					{
						if(split)
						{
							unsigned int slot = val(param2);
							ps3mapi_get_vsh_plugin_info(slot, param1, param2);
							sprintf(buffer, "200 %s|%s\r\n", param1, param2);
							ssend(conn_s_ps3mapi, buffer);
						}
					}
					else if(_IS(cmd, "GETSEGMENTS"))	// MODULE GETSEGMENTS <pid> <prx-id>
					{
						if (split)
						{
							split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
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

								u32 pid = val(param1);
								s32 prx_id = val(param2);

								_memset(buffer, sizeof(buffer));
								u32 buf_len = sprintf(buffer, "200 ");
								{system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_SEGMENTS, (u64)pid, (u64)prx_id, (u64)(u32)&moduleInfo); }

								buf_len += sprintf(buffer + buf_len, "%s|", moduleInfo.name);
								buf_len += sprintf(buffer + buf_len, "%s|", moduleInfo.filename);
								buf_len += sprintf(buffer + buf_len, "%i|", moduleInfo.filename_size);
								buf_len += sprintf(buffer + buf_len, "%i|", moduleInfo.modattribute);
								buf_len += sprintf(buffer + buf_len, "%i|", moduleInfo.start_entry);
								buf_len += sprintf(buffer + buf_len, "%i|", moduleInfo.stop_entry);

								buf_len += sprintf(buffer + buf_len, "seg0|%016llX|%016llX|%016llX|%016llX|%016llX", 
									moduleInfo.segments[0].base, moduleInfo.segments[0].filesz, moduleInfo.segments[0].memsz, 
									moduleInfo.segments[0].index, moduleInfo.segments[0].type);
								buf_len += sprintf(buffer + buf_len, "seg1|%016llX|%016llX|%016llX|%016llX|%016llX",
									moduleInfo.segments[1].base, moduleInfo.segments[1].filesz, moduleInfo.segments[0].memsz,
									moduleInfo.segments[1].index, moduleInfo.segments[1].type);
								buf_len += sprintf(buffer + buf_len, "seg2|%016llX|%016llX|%016llX|%016llX|%016llX",
									moduleInfo.segments[2].base, moduleInfo.segments[2].filesz, moduleInfo.segments[0].memsz,
									moduleInfo.segments[2].index, moduleInfo.segments[2].type);
								buf_len += sprintf(buffer + buf_len, "seg3|%016llX|%016llX|%016llX|%016llX|%016llX",
									moduleInfo.segments[3].base, moduleInfo.segments[3].filesz, moduleInfo.segments[0].memsz,
									moduleInfo.segments[3].index, moduleInfo.segments[3].type);

								buf_len += sprintf(buffer + buf_len, "\r\n");
								send(conn_s_ps3mapi, buffer, buf_len, 0);
							}
						}
					}
					else {ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502); split = 502;}
				}

				if(!split) ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
			}
			else if (_IS(cmd, "THREAD"))
			{
				if (split)
				{
					split = ssplit(param1, cmd, 19, param2, PS3MAPI_MAX_LEN);
					if (_IS(cmd, "CREATE"))	// THREAD CREATE <pid> <page_table_0> <toc> <arg> <prio> <stack_size> <name>
					{
						if (split)
						{
							split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
							if (split)
							{
								u32 pid = val(param1);
								split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
								if (split)
								{
									u64 page_table_0 = convertH(param1);
									split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
									if (split)
									{
										u64 toc_memory_address = convertH(param1);
										split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
										if (split)
										{
											u64 thread_arg = convertH(param1);
											split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
											if (split)
											{
												u32 thread_prio = val(param1);
												split = ssplit(param2, param1, PS3MAPI_MAX_LEN, param2, PS3MAPI_MAX_LEN);
												if (split)
												{
													u32 thread_stack_size = (u32)val(param1);
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

													ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
												}
												else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
											}											
											else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
										}
										else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
									}
									else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
								}
								else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
				}
				else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
			}
			#ifdef DEBUG_XREGISTRY
			else if(_IS(cmd, "REGISTRY"))
			{
				if(split)
				{
					split = ssplit(param1, cmd, 19, param2, PS3MAPI_MAX_LEN);
					if(split)
					{
						if(_IS(cmd, "GET")) // REGISTRY GET <regkey>
						{
							_memset(param1, PS3MAPI_MAX_LEN);
							int value = get_xreg_value(param2, 0, param1, true);
							if(*param1)
								sprintf(buffer, "200 %s\r\n", param1);
							else
								sprintf(buffer, "200 %i\r\n", value);
							ssend(conn_s_ps3mapi, buffer);
						}
						else if(_IS(cmd, "SET")) // REGISTRY SET <regkey> <value>
						{
							split = ssplit(param2, param1, PS3MAPI_MAX_LEN, buffer, PS3MAPI_MAX_LEN);
							if(split)
							{
								int value = val(buffer);
								get_xreg_value(param1, value, buffer, false);
								ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
							}
						}
					}
				}

				if(!split) ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
			}
			#endif // #ifdef DEBUG_XREGISTRY
			else if(_IS(cmd, "TYPE"))	// TYPE <A/I>
			{
				if(split)
				{
					dataactive = !IS(param1, "I");
					ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
				}
				else
				{
					dataactive = 1;
				}
			}
			//// Code: @PHTNCx64 2021/09/21
			else if(_IS(cmd, "PEEKLV2") || (_IS(cmd, "PEEKLV1")))	// PEEKLV1 <address>
			{														// PEEKLV2 <address>
				if(split)
				{
					bool isLV2Peek = _IS(cmd, "PEEKLV2");

					system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, isLV2Peek ? PS3MAPI_OPCODE_LV2_PEEK : PS3MAPI_OPCODE_LV1_PEEK, val(param1));
					sprintf(buffer, "200 %llu\r\n", p1);
					ssend(conn_s_ps3mapi, buffer);
				}
				else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
			}
			else if(_IS(cmd, "POKELV2") || (_IS(cmd, "POKELV1")))	// POKELV1 <address> <value>
			{														// POKELV2 <address> <value>
				if(split)
				{
					bool isLV2Peek = _IS(cmd, "POKELV2");

					split = ssplit(param1, cmd, 19, param2, PS3MAPI_MAX_LEN);
					if(split)
					{
						system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, isLV2Peek ? PS3MAPI_OPCODE_LV2_POKE : PS3MAPI_OPCODE_LV1_POKE, val(cmd), val(param2));
						sprintf(buffer, "200 %llu\r\n", p1);
						ssend(conn_s_ps3mapi, buffer);
					}
					else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
				}
				else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
			}
			else if(_IS(cmd, "SYSCALL"))	// SYSCALL <syscall-number>|0x<hex-value>|<decimal-value>|<string-value>
			{
				if(split)
				{
					split = ssplit(param1, cmd, 19, param2, PS3MAPI_MAX_LEN);

					u64 sp[9]; u8 n;
					u16 sc = parse_syscall(param2, sp, &n);
					u64 ret = call_syscall(sc, sp, n);

					sprintf(buffer, "200 %llu\r\n", ret);
					ssend(conn_s_ps3mapi, buffer);
				}
				else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
			}
			////
			else if(_IS(cmd, "PASV"))
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
						ssend(conn_s_ps3mapi, "421 Could not create socket\r\n");
						break; // Abort PASV command
					}

					p1x = ( (pasv_port & 0xff00) >> 8) | 0x80; // use ports 32768 -> 65528 (0x8000 -> 0xFFF8)
					p2x = ( (pasv_port & 0x00ff)	 );

					pasv_s = slisten(getPort(p1x, p2x), 1);

					if(pasv_s >= 0)
					{
						sprintf(pasv_output, "227 Entering Passive Mode (%s,%i,%i)\r\n", ip_address, p1x, p2x);
						ssend(conn_s_ps3mapi, pasv_output);

						if((data_s = accept(pasv_s, NULL, NULL)) >= 0)
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

