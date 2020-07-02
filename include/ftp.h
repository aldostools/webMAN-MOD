#define FTP_OK_150			"150 OK\r\n"						// File status okay; about to open data connection.
#define FTP_OK_200			"200 OK\r\n"						// The requested action has been successfully completed.
#define FTP_OK_TYPE_200		"200 TYPE OK\r\n"					// The requested action has been successfully completed.
#define FTP_OK_221			"221 BYE\r\n"						// Service closing control connection.
#define FTP_OK_226			"226 OK\r\n"						// Closing data connection. Requested file action successful (for example, file transfer or file abort).
#define FTP_OK_ABOR_226		"226 ABOR OK\r\n"					// Closing data connection. Requested file action successful
#define FTP_OK_230			"230 OK\r\n"						// User logged in, proceed. Logged out if appropriate.
#define FTP_OK_USER_230		"230 Already in\r\n"				// User logged in, proceed.
#define FTP_OK_250			"250 OK\r\n"						// Requested file action okay, completed.
#define FTP_OK_331			"331 OK\r\n"						// User name okay, need password.
#define FTP_OK_REST_350		"350 REST command successful\r\n"	// Requested file action pending further information.
#define FTP_OK_RNFR_350		"350 RNFR OK\r\n"					// Requested file action pending further information.

#define FTP_ERROR_425		"425 Error in data connection\r\n"	// Can't open data connection.
#define FTP_ERROR_430		"430 Invalid login\r\n"				// Invalid username or password.
#define FTP_ERROR_450		"450 Can't access file\r\n"			// Can't access file.
#define FTP_ERROR_451		"451 Action aborted\r\n"			// Requested action aborted. Local error in processing.
#define FTP_ERROR_500		"500 Syntax error\r\n"				// Syntax error, command unrecognized and the requested	action did not take place.
#define FTP_ERROR_501		"501 Error in arguments\r\n"		// Syntax error in parameters or arguments.
#define FTP_ERROR_REST_501	"501 No restart point\r\n"			// Syntax error in parameters or arguments.
#define FTP_ERROR_502		"502 Not implemented\r\n"			// Command not implemented.
#define FTP_ERROR_530		"530 Not logged in\r\n"				// Not logged in.
#define FTP_ERROR_550		"550 File unavailable\r\n"			// Requested action not taken. File unavailable (e.g., file not found, no access).
#define FTP_ERROR_RNFR_550	"550 RNFR Error\r\n"				// Requested action not taken. File unavailable.

static u8 ftp_active = 0;
static u8 ftp_working = 0;
static u8 ftp_session = 1;

#define FTP_RECV_SIZE  (STD_PATH_LEN + 20)

#define FTP_FILE_UNAVAILABLE    -4
#define FTP_OUT_OF_MEMORY       -6
#define FTP_DEVICE_IS_FULL      -8

static u8 absPath(char* absPath_s, const char* path, const char* cwd)
{
	if(*path == '/') strcpy(absPath_s, path);
	else
	{
		u16 len = sprintf(absPath_s, "%s", cwd);

		if(cwd[len - 1] != '/') strcat(absPath_s + len, "/");

		strcat(absPath_s + len, path);
	}

	filepath_check(absPath_s);

	if(islike(absPath_s, "/dev_blind")) {mount_device("/dev_blind", NULL, NULL); return 1;}
	if(islike(absPath_s, "/dev_hdd1") )  mount_device("/dev_hdd1",  NULL, NULL);

	return 0;
}

static int ssplit(const char* str, char* left, int lmaxlen, char* right, int rmaxlen)
{
	int ios = strcspn(str, " ");
	int ret = (ios < (int)strlen(str) - 1);
	int lsize = MIN(ios, lmaxlen);

	strncpy(left, str, lsize);
	left[lsize] = '\0';

	if(ret)
	{
		strncpy(right, str + ios + 1, rmaxlen);
		right[rmaxlen] = '\0';
	}
	else
	{
		right[0] = '\0';
	}

	return ret;
}


static void send_reply(int conn_s_ftp, int err)
{
	if(err == CELL_FS_OK)
	{
		ssend(conn_s_ftp, FTP_OK_226);		// Closing data connection. Requested file action successful (for example, file transfer or file abort).
	}
	else if(err == FTP_FILE_UNAVAILABLE)
	{
		ssend(conn_s_ftp, FTP_ERROR_550);	// Requested action not taken. File unavailable (e.g., file not found, no access).
	}
	else if(err == FTP_DEVICE_IS_FULL)
	{
		ssend(conn_s_ftp, "451 ERR, Device is full\r\n");	// Closing data connection. Requested file action successful (for example, file transfer or file abort).
	}
	else if(err == FTP_OUT_OF_MEMORY)
	{
		ssend(conn_s_ftp, "451 ERR, Out of memory\r\n");	// Requested action aborted. Local error in processing.
	}
	else
	{
		ssend(conn_s_ftp, FTP_ERROR_451);	// Requested action aborted. Local error in processing.
	}
}

static sys_addr_t allocate_ftp_buffer(sys_addr_t sysmem)
{
	if(!sysmem && (ftp_active > 1) && IS_ON_XMB)
	{
		sys_memory_container_t vsh_mc = get_vsh_memory_container();
		if(vsh_mc)	sys_memory_allocate_from_container(BUFFER_SIZE_FTP, vsh_mc, SYS_MEMORY_PAGE_SIZE_64K, &sysmem);
	}

	if(sysmem || (!sysmem && sys_memory_allocate(BUFFER_SIZE_FTP, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK)) return sysmem;

	return NULL;
}

static void handleclient_ftp(u64 conn_s_ftp_p)
{
	int conn_s_ftp = (int)conn_s_ftp_p; // main communications socket

	sys_net_sockinfo_t conn_info;
	sys_net_get_sockinfo(conn_s_ftp, &conn_info, 1);

	char remote_ip[16];
	sprintf(remote_ip, "%s", inet_ntoa(conn_info.remote_adr));

	if(webman_config->bind && ((conn_info.local_adr.s_addr != conn_info.remote_adr.s_addr) && strncmp(remote_ip, webman_config->allow_ip, strlen(webman_config->allow_ip)) != 0))
	{
		ssend(conn_s_ftp, "451 Access Denied. Use SETUP to allow remote connections.\r\n");
		sclose(&conn_s_ftp);
		sys_ppu_thread_exit(0);
	}

#ifdef USE_NTFS
	if(!ftp_active && (mountCount == NTFS_UNMOUNTED) && !refreshing_xml && root_check) check_ntfs_volumes();
#endif

	setPluginActive();

	char ip_address[16];
	sprintf(ip_address, "%s", inet_ntoa(conn_info.local_adr));
	for(u8 n = 0; ip_address[n]; n++) if(ip_address[n] == '.') ip_address[n] = ',';

	int data_s = NONE;			// data socket
	int pasv_s = NONE;			// passive data socket

	u8 connactive = 1;			// whether the ftp connection is active or not
	u8 dataactive = 0;			// prevent the data connection from being closed at the end of the loop
	u8 loggedin = 0;			// whether the user is logged in or not

	char cwd[STD_PATH_LEN];	// Current Working Directory
	u64 rest = 0;			// for resuming file transfers

	CellRtcDateTime rDate;
	char cmd[16], param[STD_PATH_LEN], filename[STD_PATH_LEN], source[STD_PATH_LEN]; // used as source parameter in RNFR and COPY commands
	char *cpursx = filename, *tempcwd = filename, *d_path = param, *pasv_output = param;
	struct CellFsStat buf;
	int fd, pos;

	bool is_ntfs = false, do_sc36 = true;

	char buffer[FTP_RECV_SIZE];

#ifdef USE_NTFS
	struct stat bufn;

	sprintf(buffer, "%i " WM_APPNAME "ftpd " WM_VERSION " [NTFS:%i]\r\n", 220, MAX(mountCount, 0)); ssend(conn_s_ftp, buffer);
#else
	sprintf(buffer, "%i " WM_APPNAME "ftpd " WM_VERSION "\r\n", 220); ssend(conn_s_ftp, buffer);
#endif

	strcpy(cwd, "/");

	struct timeval tv;
	tv.tv_usec = 0;
	tv.tv_sec = 20;

	if(webman_config->ftp_timeout)
	{
		tv.tv_sec = (webman_config->ftp_timeout * 60);
	}
	setsockopt(conn_s_ftp, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	sys_addr_t sysmem = NULL;

	ftp_active++;

	while(connactive && working && ftp_session)
	{
		int rlen = (int)recv(conn_s_ftp, buffer, FTP_RECV_SIZE, 0);
		if(rlen > 0)
		{
			buffer[rlen] = NULL;

			char *p = strstr(buffer, "\r\n");
			if(p) *p = NULL; else break;

			is_ntfs = false;

			int split = ssplit(buffer, cmd, 15, param, STD_PATH_LEN - 1);

			if(!working || _IS(cmd, "QUIT") || _IS(cmd, "BYE"))
			{
				if(working) ssend(conn_s_ftp, FTP_OK_221); // Service closing control connection.
				connactive = 0;
				break;
			}
			else
			if(loggedin)
			{
				if(_IS(cmd, "RETR"))
				{
					if(data_s < 0 && pasv_s >= 0) data_s = accept(pasv_s, NULL, NULL);

					if(data_s >= 0)
					{
						if(split)
						{
							absPath(filename, param, cwd);

							int err = FTP_FILE_UNAVAILABLE;

							if(do_sc36 && islike(filename, "/dvd_bdvd"))
								{do_sc36 = false; system_call_1(36, (u64) "/dev_bdvd");} // decrypt dev_bdvd files

							sysmem = allocate_ftp_buffer(sysmem);

							if(sysmem)
							{
#ifdef COPY_PS3
								if(!copy_in_progress) {ftp_state = 1; strcpy(current_file, filename);}
#endif
								char *buffer2 = (char*)sysmem;
#ifdef USE_NTFS
								if(is_ntfs_path(filename))
								{
									fd = ps3ntfs_open(filename + 5, O_RDONLY, 0);

									if(fd > 0)
									{
										ps3ntfs_seek64(fd, rest, SEEK_SET);

										rest = 0, err = FAILED;
										ftp_ntfs_transfer_in_progress++;

										ssend(conn_s_ftp, FTP_OK_150);

										int read_e = 0;
										while(working)
										{
											read_e = ps3ntfs_read(fd, (void *)buffer2, BUFFER_SIZE_FTP);
											if(read_e > 0)
											{
												if(send(data_s, buffer2, (size_t)read_e, 0) < 0) break; // FAILED
											}
											else if(read_e < 0)
												break; // FAILED
											else
												{err = CELL_FS_OK; break;}
										}

										ps3ntfs_close(fd); ftp_ntfs_transfer_in_progress--;
									}
								}
								else
#endif
								if(cellFsOpen(filename, CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
								{
									u64 read_e, pos;
									if(rest) cellFsLseek(fd, rest, CELL_FS_SEEK_SET, &pos);

									//int optval = BUFFER_SIZE_FTP;
									//setsockopt(data_s, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval));

									rest = 0, err = FAILED;

									ssend(conn_s_ftp, FTP_OK_150); // File status okay; about to open data connection.

									while(working)
									{
										if(cellFsRead(fd, (void *)buffer2, BUFFER_SIZE_FTP, &read_e) == CELL_FS_SUCCEEDED)
										{
											if(read_e)
											{
												if(send(data_s, buffer2, (size_t)read_e, 0) < 0) break; // FAILED
											}
											else
												{err = CELL_FS_OK; break;}
										}
										else
											break; // FAILED
									}
									cellFsClose(fd);
								}
								ftp_state = 0;
							}
							else
								err = FTP_OUT_OF_MEMORY;

							send_reply(conn_s_ftp, err);
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_501);			// Syntax error in parameters or arguments.
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_425);				// Can't open data connection.
					}
				}
				else
				if(_IS(cmd, "STOR") || _IS(cmd, "APPE"))
				{
					if(data_s < 0 && pasv_s >= 0) data_s = accept(pasv_s, NULL, NULL);

					if(data_s >= 0)
					{
						if(split)
						{
							u8 is_dev_blind = absPath(filename, param, cwd);

							int err = FAILED, is_append = _IS(cmd, "APPE");

							sysmem = allocate_ftp_buffer(sysmem);

							if(sysmem)
							{
								char *buffer2 = (char*)sysmem; int read_e = 0;

								setsockopt(data_s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
								#ifdef COPY_PS3
								if(!copy_in_progress) {ftp_state = 2; strcpy(current_file, filename);}
								#endif
								#ifdef USE_NTFS
								if(is_ntfs_path(filename))
								{
									fd = ps3ntfs_open(filename + 5, O_CREAT | O_WRONLY | ((rest | is_append) ? O_APPEND : O_TRUNC), MODE);

									if(fd > 0)
									{
										ps3ntfs_seek64(fd, rest, SEEK_SET);

										rest = 0;
										ftp_ntfs_transfer_in_progress++;

										ssend(conn_s_ftp, FTP_OK_150);

										while(working)
										{
											read_e = (int)recv(data_s, buffer2, BUFFER_SIZE_FTP, MSG_WAITALL);
											if(read_e > 0)
											{
												if(ps3ntfs_write(fd, buffer2, read_e) != (int)read_e) break; // FAILED
											}
											else if(read_e < 0)
												break; // FAILED
											else
												{err = CELL_FS_OK; break;}
										}

										ps3ntfs_close(fd); ftp_ntfs_transfer_in_progress--;
										if(!working || err != CELL_FS_OK) ps3ntfs_unlink(filename + 5);
									}
								}
								else
								#endif
								{
									if(is_dev_blind && file_exists(filename))
									{
										u16 len = sprintf(source, "%s", filename);
										sprintf(filename + len, ".~");
										if(file_exists(filename)) {filename[len] = *source = NULL;} // overwrite if the temp file exists
									}
									else
										*source = NULL;

									if(cellFsOpen(filename, CELL_FS_O_CREAT | CELL_FS_O_WRONLY | ((rest | is_append) ? CELL_FS_O_APPEND : CELL_FS_O_TRUNC), &fd, NULL, 0) == CELL_FS_SUCCEEDED)
									{
										u64 pos = 0;

										if(rest) cellFsLseek(fd, rest, CELL_FS_SEEK_SET, &pos);

										rest = 0;

										ssend(conn_s_ftp, FTP_OK_150); // File status okay; about to open data connection.

										//int optval = BUFFER_SIZE_FTP;
										//setsockopt(data_s, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));

										while(working)
										{
											read_e = (int)recv(data_s, buffer2, BUFFER_SIZE_FTP, MSG_WAITALL);
											if(read_e > 0)
											{
												if(cellFsWrite(fd, buffer2, read_e, NULL) != CELL_FS_SUCCEEDED) break; // FAILED
											}
											else if(read_e < 0)
												break; // FAILED
											else
												{err = CELL_FS_OK; break;}
										}

										cellFsClose(fd);
										if(!working || err != CELL_FS_OK)
										{
											for(u8 n = 0; n < 17; n++)
											{
												if(islike(filename, drives[n]) && (get_free_space(drives[n]) < BUFFER_SIZE_FTP))
													{ err = FTP_DEVICE_IS_FULL; break; }
											}

											cellFsUnlink(filename);
										}
									}
								}
								ftp_state = 0;
							}
							else
								err = FTP_OUT_OF_MEMORY;

							if(err == CELL_FS_OK)
							{
								ssend(conn_s_ftp, FTP_OK_226);		// Closing data connection. Requested file action successful (for example, file transfer or file abort).
								cellFsChmod(filename, is_dev_blind ? 0644 : MODE);

								if(*source == '/') {cellFsUnlink(source); cellFsRename(filename, source);} // replace original file
								*source = NULL;
							}
							else
							{
								send_reply(conn_s_ftp, err);
							}
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_501);		// Syntax error in parameters or arguments.
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_425);			// Can't open data connection.
					}
				}
				else
				if(_IS(cmd, "REST"))
				{
					if(split)
					{
						ssend(conn_s_ftp, FTP_OK_REST_350); // Requested file action pending further information
						rest = (u64)val(param);
						dataactive = 1;
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_REST_501); // Syntax error in parameters or arguments.
					}
				}
				else
				if(_IS(cmd, "SIZE"))
				{
					if(split)
					{
						absPath(filename, param, cwd);
						#ifdef USE_NTFS
						if(is_ntfs_path(filename))
						{
							filename[10] = ':';
							if(ps3ntfs_stat(filename + 5, &bufn) >= 0) {is_ntfs = true; buf.st_size = bufn.st_size;}
						}
						#endif
						if(is_ntfs || cellFsStat(filename, &buf) == CELL_FS_SUCCEEDED)
						{
							sprintf(buffer, "213 %llu\r\n", (unsigned long long)buf.st_size);
							ssend(conn_s_ftp, buffer);
							dataactive = 1;
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_550); // Requested action not taken. File unavailable (e.g., file not found, no access).
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501); // Syntax error in parameters or arguments.
					}
				}
				else
				if(_IS(cmd, "DELE"))
				{
					if(split)
					{
						absPath(filename, param, cwd);
#ifdef USE_NTFS
						if(is_ntfs_path(filename))
						{
							filename[10] = ':';
							if(ps3ntfs_unlink(filename + 5) >= 0) is_ntfs = true;
						}
#endif
						if(is_ntfs || cellFsUnlink(filename) == CELL_FS_SUCCEEDED)
						{
							ssend(conn_s_ftp, FTP_OK_250); // Requested file action okay, completed.
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_550); // Requested action not taken. File unavailable (e.g., file not found, no access).
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501); // Syntax error in parameters or arguments.
					}
				}
				else
				if(_IS(cmd, "RNFR"))
				{
					if(split)
					{
						absPath(source, param, cwd);

						if(file_exists(source))
						{
							ssend(conn_s_ftp, FTP_OK_RNFR_350);		// Requested file action pending further information
						}
						else
						{
							*source = NULL;
							ssend(conn_s_ftp, FTP_ERROR_RNFR_550);	// Requested action not taken. File unavailable
						}
					}
					else
					{
						*source = NULL;
						ssend(conn_s_ftp, FTP_ERROR_501);			// Syntax error in parameters or arguments.
					}
				}
				else
				if(_IS(cmd, "RNTO"))
				{
					if(split && (*source == '/'))
					{
						absPath(filename, param, cwd);
#ifdef USE_NTFS
						if(is_ntfs_path(source) && is_ntfs_path(filename))
						{
							source[10] = filename[10] = ':';
							if(ps3ntfs_rename(source + 5, filename + 5) >= 0) is_ntfs = true;
						}
#endif
						if(is_ntfs || (cellFsRename(source, filename) == CELL_FS_SUCCEEDED))
						{
							ssend(conn_s_ftp, FTP_OK_250); // Requested file action okay, completed.
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_550); // Requested action not taken. File unavailable (e.g., file not found, no access).
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501); // Syntax error in parameters or arguments.
					}
					*source = NULL;
				}
				else
				if(_IS(cmd, "MDTM"))
				{
					if(split)
					{
						absPath(filename, param, cwd);
#ifdef USE_NTFS
						if(is_ntfs_path(filename))
						{
							filename[10] = ':';
							if(ps3ntfs_stat(filename + 5, &bufn) >= 0) {is_ntfs = true; buf.st_mtime = bufn.st_mtime;}
						}
#endif
						if(is_ntfs || cellFsStat(filename, &buf) == CELL_FS_SUCCEEDED)
						{
							cellRtcSetTime_t(&rDate, buf.st_mtime);
							sprintf(buffer, "213 %04i%02i%02i%02i%02i%02i\r\n", rDate.year, rDate.month, rDate.day, rDate.hour, rDate.minute, rDate.second);
							ssend(conn_s_ftp, buffer);
							dataactive = 1;
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_550);	// Requested action not taken. File unavailable (e.g., file not found, no access).
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501);		// Syntax error in parameters or arguments.
					}
				}
				else
				if(_IS(cmd, "PORT"))
				{
					rest = 0;

					if(split)
					{
						char data[6][4];
						u8 i = 0;

						for(u8 j = 0, k = 0; ; j++)
						{
							if(ISDIGIT(param[j])) data[i][k++] = param[j];
							else {data[i++][k] = 0, k = 0;}
							if((i >= 6) || (k >= 4) || !param[j]) break;
						}

						if(i == 6)
						{
							char ipaddr[16];
							sprintf(ipaddr, "%s.%s.%s.%s", data[0], data[1], data[2], data[3]);

							data_s = connect_to_server(ipaddr, getPort(val(data[4]), val(data[5])));

							if(data_s >= 0)
							{
								ssend(conn_s_ftp, FTP_OK_200);		// The requested action has been successfully completed.
								dataactive = 1;
							}
							else
							{
								ssend(conn_s_ftp, FTP_ERROR_451);	// Requested action aborted. Local error in processing.
							}
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_501);		// Syntax error in parameters or arguments.
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501);			// Syntax error in parameters or arguments.
					}
				}
				else
				if(_IS(cmd, "PASV"))
				{
					rest = 0;
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
							ssend(conn_s_ftp, pasv_output);

							if((data_s = accept(pasv_s, NULL, NULL)) > 0)
							{
								setsockopt(pasv_s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

								dataactive = 1; break;
							}
						}
					}

					if(pasv_retry >= 250)
					{
						ssend(conn_s_ftp, FTP_ERROR_451);	// Requested action aborted. Local error in processing.
						if(pasv_s >= 0) sclose(&pasv_s);
						pasv_s = NONE;
					}
				}
				else
				if(_IS(cmd, "MLSD") || _IS(cmd, "LIST") || _IS(cmd, "MLST") || _IS(cmd, "NLST"))
				{
					bool nolist  = _IS(cmd, "NLST");
					bool is_MLSD = _IS(cmd, "MLSD");
					bool is_MLST = _IS(cmd, "MLST");
					bool is_MLSx = is_MLSD || is_MLST;

					if(_IS(param, "-l") || _IS(param, "-a") || _IS(param, "-la") || _IS(param, "-al")) {*param = NULL, nolist = false;}

					if((data_s < 0) && (pasv_s >= 0) && !is_MLST) data_s = accept(pasv_s, NULL, NULL);

					if(!(is_MLST || nolist) && sysmem) {sys_memory_free(sysmem); sysmem = NULL;}

					if(data_s >= 0)
					{
						// --- get d_path & wildcard ---
						char *pw, *ps, wcard[STD_PATH_LEN]; *wcard = NULL;

						pw = strchr(param, '*'); if(pw) {ps = strrchr(param, '/'); if((ps > param) && (ps < pw)) pw = ps; while(*pw == '*' || *pw == '/') *pw++ = 0; strcpy(wcard, pw); pw = strstr(wcard, "*"); if(pw) *pw = 0; if(!*wcard && !ps) strcpy(wcard, param);}

						if(*param == NULL) split = 0;

						if(split)
						{
							strcpy(tempcwd, param);
							absPath(d_path, tempcwd, cwd);

							if(!isDir(d_path) && (*wcard == NULL)) {strcpy(wcard, tempcwd); split = 0, *param = NULL;}
						}

						if(!split || !isDir(d_path)) strcpy(d_path, cwd);

						mode_t mode = NULL; char dirtype[2]; dirtype[1] = NULL;

						size_t d_path_len = sprintf(filename, "%s/", d_path);
						bool is_root = (d_path_len < 6); if(is_root) d_path_len = sprintf(filename, "/");

#ifdef USE_NTFS
						DIR_ITER *pdir = NULL;

						if(is_root) check_ntfs_volumes();

						if(is_ntfs_path(d_path))
						{
							cellRtcSetTime_t(&rDate, 0);
							pdir = ps3ntfs_opendir(d_path); // /dev_ntfs1v -> ntfs1:
							if(pdir) is_ntfs = true;
						}
#endif
						if(is_ntfs || cellFsOpendir(d_path, &fd) == CELL_FS_SUCCEEDED)
						{
							ssend(conn_s_ftp, FTP_OK_150); // File status okay; about to open data connection.

							CellFsDirectoryEntry entry; u32 read_f;
							CellFsDirent entry_s; u64 read_e; // list root folder using the slower readdir
							char *entry_name = (is_root) ? entry_s.d_name : entry.entry_name.d_name;
							u16 slen;

							while(working)
							{
#ifdef USE_NTFS
								if(is_ntfs) {if(ps3ntfs_dirnext(pdir, entry_name, &bufn) != CELL_OK) break; entry.attribute.st_mode = bufn.st_mode, entry.attribute.st_size = bufn.st_size, entry.attribute.st_mtime = bufn.st_mtime;}
								else
#endif
								if(is_root) {if((cellFsReaddir(fd, &entry_s, &read_e) != CELL_FS_SUCCEEDED) || (read_e == 0)) break;}
								else
								if(cellFsGetDirectoryEntries(fd, &entry, sizeof(entry), &read_f) || !read_f) break;

								if(*wcard && strcasestr(entry_name, wcard) == NULL) continue;

								if((entry_name[0]=='$' && d_path[12] == 0) || (*wcard && strcasestr(entry_name, wcard) == NULL)) continue;
#ifdef USE_NTFS
								// use host_root to expand all /dev_ntfs entries in root
								bool is_host = is_root && ((mountCount > 0) && IS(entry_name, "host_root") && mounts);

								u8 ntmp = 1;
								if(is_host) ntmp = mountCount + 1;

								for(u8 u = 0; u < ntmp; u++)
								{
									if(u) sprintf(entry_name, "dev_%s:", mounts[u-1].name);
#endif
									if(nolist)
										slen = sprintf(buffer, "%s\015\012", entry_name);
									else
									{
										if(is_root && IS(entry_name, "host_root")) continue;

										sprintf(filename + d_path_len, "%s", entry_name);

										if(is_root)
										{
											cellFsStat(filename, &buf);
											entry.attribute.st_mode  = buf.st_mode;
											entry.attribute.st_size  = get_free_space(filename); // buf.st_size;
											entry.attribute.st_mtime = buf.st_mtime;
										}

										cellRtcSetTime_t(&rDate, entry.attribute.st_mtime);

										mode = entry.attribute.st_mode;

										if(is_MLSx)
										{
											if(IS(entry_name, "."))		*dirtype =  'c'; else
											if(IS(entry_name, ".."))	*dirtype =  'p'; else
																		*dirtype = '\0';

											slen = sprintf(buffer, "%stype=%s%s;siz%s=%llu;modify=%04i%02i%02i%02i%02i%02i;UNIX.mode=0%i%i%i;UNIX.uid=root;UNIX.gid=root; %s\r\n",
													is_MLSD ? "" : " ",
													dirtype,
													( (mode & S_IFDIR) != 0) ? "dir" : "file",
													( (mode & S_IFDIR) != 0) ? "d" : "e", (unsigned long long)entry.attribute.st_size, rDate.year, rDate.month, rDate.day, rDate.hour, rDate.minute, rDate.second,
													(((mode & S_IRUSR) != 0) * 4 + ((mode & S_IWUSR) != 0) * 2 + ((mode & S_IXUSR) != 0)),
													(((mode & S_IRGRP) != 0) * 4 + ((mode & S_IWGRP) != 0) * 2 + ((mode & S_IXGRP) != 0)),
													(((mode & S_IROTH) != 0) * 4 + ((mode & S_IWOTH) != 0) * 2 + ((mode & S_IXOTH) != 0)),
													entry_name);
										}
										else
											slen = sprintf(buffer, "%s%s%s%s%s%s%s%s%s%s 1 root  root  %13llu %s %02i %02i:%02i %s\r\n",
													(mode & S_IFDIR) ? "d" : "-",
													(mode & S_IRUSR) ? "r" : "-",
													(mode & S_IWUSR) ? "w" : "-",
													(mode & S_IXUSR) ? "x" : "-",
													(mode & S_IRGRP) ? "r" : "-",
													(mode & S_IWGRP) ? "w" : "-",
													(mode & S_IXGRP) ? "x" : "-",
													(mode & S_IROTH) ? "r" : "-",
													(mode & S_IWOTH) ? "w" : "-",
													(mode & S_IXOTH) ? "x" : "-",
													(unsigned long long)entry.attribute.st_size, smonth[rDate.month - 1], rDate.day,
													rDate.hour, rDate.minute, entry_name);
									}
									if(send(data_s, buffer, slen, 0) < 0) break;
#ifdef USE_NTFS
								}
#endif
							}

#ifdef USE_NTFS
							if(is_ntfs)
								ps3ntfs_dirclose(pdir);
							else
#endif
								cellFsClosedir(fd);

							get_cpursx(cpursx); cpursx[7] = cpursx[20] = ' ';

							if(is_root)
							{
								sprintf(buffer, "226 [/] [%s]\r\n", cpursx);
								ssend(conn_s_ftp, buffer);
							}
							else
							{
								u64 mb_free;
								char *slash = strchr(d_path + 1, '/');
								if(slash) *slash = '\0';
								mb_free = (get_free_space(d_path)>>20);

								sprintf(buffer, "226 [%s] [ %llu %s %s]\r\n", d_path, mb_free, STR_MBFREE, cpursx);
								ssend(conn_s_ftp, buffer);
							}
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_550);	// Requested action not taken. File unavailable (e.g., file not found, no access).
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_425);		// Can't open data connection.
					}
				}
				else
				if(_IS(cmd, "CWD") || _IS(cmd, "XCWD"))
				{
					if(sysmem) {sys_memory_free(sysmem); sysmem = NULL;} // release allocated buffer on directory change

					if(split)
					{
						if(IS(param, "..")) goto cdup;
						absPath(tempcwd, param, cwd);
					}
					else
						sprintf(tempcwd, "%s", cwd);
#ifdef USE_NTFS
					if(is_ntfs_path(tempcwd))
					{
						tempcwd[10] = ':';
						if(strlen(tempcwd) < 13 || (ps3ntfs_stat(tempcwd + 5, &bufn) >= 0 && (bufn.st_mode & S_IFDIR))) is_ntfs = true;
					}
#endif
					if(is_ntfs || isDir(tempcwd))
					{
						sprintf(cwd, "%s", tempcwd);
						ssend(conn_s_ftp, FTP_OK_250); // Requested file action okay, completed.

						dataactive = 1;
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_550); // Requested action not taken. File unavailable (e.g., file not found, no access).
					}
				}
				else
				if(_IS(cmd, "CDUP") || _IS(cmd, "XCUP"))
				{
					if(sysmem) {sys_memory_free(sysmem); sysmem = NULL;} // release allocated buffer on directory change

					cdup:
					pos = strlen(cwd) - 2;

					for(int i = pos; i > 0; i--)
					{
						if(i < pos && cwd[i] == '/')
						{
							break;
						}
						else
						{
							cwd[i] = '\0';
						}
					}
					ssend(conn_s_ftp, FTP_OK_250); // Requested file action okay, completed.
				}
				else
				if(_IS(cmd, "PWD") || _IS(cmd, "XPWD"))
				{
					sprintf(buffer, "257 \"%s\"\r\n", cwd);
					ssend(conn_s_ftp, buffer);
				}
				else
				if(_IS(cmd, "MKD") || _IS(cmd, "XMKD"))
				{
					if(split)
					{
						absPath(filename, param, cwd);
#ifdef USE_NTFS
						if(is_ntfs_path(filename))
						{
							filename[10] = ':';
							if(ps3ntfs_mkdir(filename + 5, DMODE) >= CELL_OK) is_ntfs = true;
						}
#endif
						if(is_ntfs || cellFsMkdir(filename, DMODE) == CELL_FS_SUCCEEDED)
						{
							sprintf(buffer, "257 \"%s\" OK\r\n", param);
							ssend(conn_s_ftp, buffer);
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_550); // Requested action not taken. File unavailable (e.g., file not found, no access).
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501); // Syntax error in parameters or arguments.
					}
				}
				else
				if(_IS(cmd, "RMD") || _IS(cmd, "XRMD"))
				{
					if(split)
					{
						absPath(filename, param, cwd);

#ifndef LITE_EDITION
						if(del(filename, true) == CELL_FS_SUCCEEDED)
#else
						if(cellFsRmdir(filename) == CELL_FS_SUCCEEDED)
#endif
						{
							ssend(conn_s_ftp, FTP_OK_250); // Requested file action okay, completed.
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_550); // Requested action not taken. File unavailable (e.g., file not found, no access).
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501); // Syntax error in parameters or arguments.
					}
				}
				else
				if(_IS(cmd, "NOOP"))
				{
					ssend(conn_s_ftp, "200 NOOP\r\n");
					dataactive = 1;
				}
				else
				if(_IS(cmd, "USER") || _IS(cmd, "PASS"))
				{
					ssend(conn_s_ftp, FTP_OK_USER_230); // User logged in, proceed.
				}
				else
				if(_IS(cmd, "SYST"))
				{
					ssend(conn_s_ftp, "215 UNIX Type: L8\r\n");
				}
				else
				if(_IS(cmd, "TYPE"))
				{
					ssend(conn_s_ftp, FTP_OK_TYPE_200); // The requested action has been successfully completed.
					dataactive = 1;
				}
				else
				if(_IS(cmd, "ABOR"))
				{
					sclose(&data_s);
					ssend(conn_s_ftp, FTP_OK_ABOR_226); // Closing data connection. Requested file action successful
				}
				else
				if(_IS(cmd, "SITE"))
				{
					if(split)
					{
						split = ssplit(param, cmd, 10, filename, STD_PATH_LEN - 1);

						if(_IS(cmd, "HELP"))
						{
							ssend(conn_s_ftp, "214-CMDs:\r\n"
#ifndef LITE_EDITION
											  " SITE FLASH\r\n"
 #ifdef USE_NTFS
											  " SITE NTFS\r\n"
 #endif
 #ifdef PKG_HANDLER
											  " SITE INSTALL <file>\r\n"
 #endif
 #ifdef EXT_GDATA
											  " SITE EXTGD <ON/OFF>\r\n"
 #endif
											  " SITE MAPTO <path>\r\n"
 #ifdef FIX_GAME
											  " SITE FIX <path>\r\n"
 #endif
											  " SITE UMOUNT\r\n"
											  " SITE COPY <file>\r\n"
											  " SITE PASTE <file>\r\n"
											  " SITE CHMOD 777 <file>\r\n"
#endif
											  " SITE SHUTDOWN\r\n"
											  " SITE RESTART\r\n"
											  " SITE STOP\r\n"
											  " SITE TIMEOUT <secs>\r\n"
											  "214 End\r\n");
						}
						else
						if(_IS(cmd, "RESTART") || _IS(cmd, "REBOOT") || _IS(cmd, "SHUTDOWN"))
						{
							ssend(conn_s_ftp, FTP_OK_221); // Service closing control connection.
							if(sysmem) sys_memory_free(sysmem);
							working = 0;
							if(_IS(cmd, "REBOOT")) save_file(WMNOSCAN, NULL, SAVE_ALL);
							if(_IS(cmd, "SHUTDOWN")) {del_turnoff(1); vsh_shutdown();} else {del_turnoff(2); vsh_reboot();}
							sys_ppu_thread_exit(0);
						}
#ifdef USE_NTFS
						else
						if(_IS(cmd, "NTFS"))
						{
							mount_all_ntfs_volumes();
							sprintf(buffer, "221 OK [NTFS VOLUMES: %i]\r\n", mountCount);

							ssend(conn_s_ftp, buffer);
						}
#endif
						else
						if(_IS(cmd, "STOP"))
						{
							if(working) ssend(conn_s_ftp, FTP_OK_221); // Service closing control connection.
							ftp_working = connactive = 0;
							break;
						}
						else
						if(_IS(cmd, "TIMEOUT"))
						{
							if(working) ssend(conn_s_ftp, FTP_OK_221); // Service closing control connection.

							tv.tv_sec = val(filename);
							setsockopt(conn_s_ftp, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
							break;
						}
						else
						if(_IS(cmd, "FLASH"))
						{
							ssend(conn_s_ftp, FTP_OK_250); // Requested file action okay, completed.

							bool rw_flash = isDir("/dev_blind"); char *status = to_upper(filename);

							if(*status == NULL) ; else
							if(_IS(status, "ON" )) {if( rw_flash) continue;} else
							if(_IS(status, "OFF")) {if(!rw_flash) continue;}

							if(rw_flash)
								disable_dev_blind();
							else
								enable_dev_blind(NO_MSG);
						}
#ifndef LITE_EDITION
 #ifdef PKG_HANDLER
						else
						if(_IS(cmd, "INSTALL"))
						{
							absPath(param, filename, cwd); char *msg = filename;

							if(installPKG(param, msg) == CELL_OK)
								ssend(conn_s_ftp, FTP_OK_250); // Requested file action okay, completed.
							else
								ssend(conn_s_ftp, FTP_ERROR_451); // Requested action aborted. Local error in processing.

							show_msg(msg);
						}
 #endif
 #ifdef EXT_GDATA
						else
						if(_IS(cmd, "EXTGD"))
						{
							ssend(conn_s_ftp, FTP_OK_250); // Requested file action okay, completed.

							char *status = to_upper(filename);

							if(*status == NULL)		set_gamedata_status(extgd^1, true); else
							if(_IS(status, "ON" ))	set_gamedata_status(0, true);		else
							if(_IS(status, "OFF"))	set_gamedata_status(1, true);

						}
 #endif
						else
						if(_IS(cmd, "UMOUNT"))
						{
							ssend(conn_s_ftp, FTP_OK_250); // Requested file action okay, completed.
							do_umount(true);
						}
 #ifdef COBRA_ONLY
						else
						if(_IS(cmd, "MAPTO"))
						{
							ssend(conn_s_ftp, FTP_OK_250); // Requested file action okay, completed.

							char *src_path = filename;

							if(isDir(src_path))
							{
								// map current directory to path
								sys_map_path(src_path, (IS(cwd, "/") ? NULL : cwd) ); // unmap if cwd is the root
							}
							else
							{
								mount_game(cwd, 0);
							}
						}
 #endif //#ifdef COBRA_ONLY
 #ifdef FIX_GAME
						else
						if(_IS(cmd, "FIX"))
						{
							if(fix_in_progress)
							{
								ssend(conn_s_ftp, FTP_ERROR_451);	// Requested action aborted. Local error in processing.
							}
							else
							{
								ssend(conn_s_ftp, FTP_OK_250);		// Requested file action okay, completed.
								absPath(param, filename, cwd);

								fix_in_progress = true, fix_aborted = false;

  #ifdef COBRA_ONLY
								if(strcasestr(filename, ".iso"))
									fix_iso(param, 0x100000UL, false);
								else
  #endif //#ifdef COBRA_ONLY
									fix_game(param, filename, FIX_GAME_FORCED);

								fix_in_progress = false;
							}
						}
 #endif //#ifdef FIX_GAME
						else
						if(_IS(cmd, "CHMOD"))
						{
							split = ssplit(param, cmd, 10, filename, STD_PATH_LEN - 1);

							strcpy(param, filename); absPath(filename, param, cwd);

							ssend(conn_s_ftp, FTP_OK_250); // Requested file action okay, completed.
							int attributes = val(cmd);
							if(attributes == 0)
								cellFsChmod(filename, MODE);
							else
								cellFsChmod(filename, attributes);
						}
 #ifdef COPY_PS3
						else
						if(_IS(cmd, "COPY"))
						{
							sprintf(buffer, "%s %s", STR_COPYING, filename);
							show_msg(buffer);

							absPath(source, filename, cwd);
							ssend(conn_s_ftp, FTP_OK_200); // The requested action has been successfully completed.
						}
						else
						if(_IS(cmd, "PASTE"))
						{
							absPath(param, filename, cwd);
							if((!copy_in_progress) && (*source) && (!IS(source, param)) && file_exists(source))
							{
								copy_in_progress = true; copied_count = 0;
								ssend(conn_s_ftp, FTP_OK_250); // Requested file action okay, completed.

								sprintf(buffer, "%s %s\n%s %s", STR_COPYING, source, STR_CPYDEST, param);
								show_msg(buffer);

								if(isDir(source))
									folder_copy(source, param);
								else
									file_copy(source, param, COPY_WHOLE_FILE);

								show_msg((char*)STR_CPYFINISH);
								copy_in_progress = false;
							}
							else
							{
								ssend(conn_s_ftp, FTP_ERROR_500);
							}
						}
 #endif
 #ifdef WM_REQUEST
						else
						if(*param == '/')
						{
							u16 size = sprintf(buffer, "GET %s", param);
							save_file(WMREQUEST_FILE, buffer, size);

							do_custom_combo(WMREQUEST_FILE);

							ssend(conn_s_ftp, FTP_OK_200); // The requested action has been successfully completed.
						}
 #endif
#endif //#ifndef LITE_EDITION
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_500);
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501); // Syntax error in parameters or arguments.
					}
				}
				else
				if(_IS(cmd, "FEAT"))
				{
					ssend(conn_s_ftp,	"211-Ext:\r\n"
										" REST STREAM\r\n"
										" PASV\r\n"
										" PORT\r\n"
										" CDUP\r\n"
										" ABOR\r\n"
										" PWD\r\n"
										" TYPE\r\n"
										" SIZE\r\n"
										" SITE\r\n"
										" APPE\r\n"
										" LIST\r\n"
										" MLSD\r\n"
										" MDTM\r\n"
										" MLST type*;size*;modify*;UNIX.mode*;UNIX.uid*;UNIX.gid*;\r\n"
										"211 End\r\n");
				}
				else
				/*if(  _IS(cmd, "AUTH") || _IS(cmd, "ADAT")
					|| _IS(cmd, "CCC")  || _IS(cmd, "CLNT")
					|| _IS(cmd, "CONF") || _IS(cmd, "ENC" )
					|| _IS(cmd, "EPRT") || _IS(cmd, "EPSV")
					|| _IS(cmd, "LANG") || _IS(cmd, "LPRT")
					|| _IS(cmd, "LPSV") || _IS(cmd, "MIC" )
					|| _IS(cmd, "OPTS")
					|| _IS(cmd, "PBSZ") || _IS(cmd, "PROT")
					|| _IS(cmd, "SMNT") || _IS(cmd, "STOU")
					|| _IS(cmd, "XRCP") || _IS(cmd, "XSEN")
					|| _IS(cmd, "XSEM") || _IS(cmd, "XRSQ")
					// RFC 5797 mandatory
					|| _IS(cmd, "ACCT") || _IS(cmd, "ALLO")
					|| _IS(cmd, "MODE") || _IS(cmd, "REIN")
					|| _IS(cmd, "STAT") || _IS(cmd, "STRU") )
				{
					ssend(conn_s_ftp, FTP_ERROR_502);	// Command not implemented.
				}
				else*/
				{
					ssend(conn_s_ftp, FTP_ERROR_500);	// Syntax error, command unrecognized and the requested	action did not take place.
				}

				if(dataactive) dataactive = 0;
				else
				{
					sclose(&data_s); data_s = NONE;
					rest = 0;
				}
			}
			else
			{
				// commands available when not logged in
				if(_IS(cmd, "USER"))
				{
					ssend(conn_s_ftp, FTP_OK_331); // User name okay, need password.
				}
				else
				if(_IS(cmd, "PASS"))
				{
					if((webman_config->ftp_password[0] == NULL) || IS(webman_config->ftp_password, param))
					{
						ssend(conn_s_ftp, FTP_OK_230);		// User logged in, proceed. Logged out if appropriate.
						loggedin = 1;
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_430);	// Invalid username or password
					}
				}
				else
				{
					ssend(conn_s_ftp, FTP_ERROR_530); // Not logged in.
				}
			}
		}
		else
		{
			connactive = 0;
			break;
		}

		//sys_ppu_thread_usleep(2000);
	}

	if(sysmem) sys_memory_free(sysmem);

	if(pasv_s >= 0) sclose(&pasv_s);
	sclose(&conn_s_ftp);
	sclose(&data_s);

	ftp_active--;

	setPluginInactive();

	sys_ppu_thread_exit(0);
}

static void close_ftp_sessions_idle(void)
{
	ftp_session = 0; // close all open ftp sessions idle
	for(u8 retry = 0; ftp_active && (retry < 5); retry++) sys_ppu_thread_sleep(1);
	ftp_session = 1; // allow new ftp sessions
}

static void ftpd_thread(__attribute__((unused)) u64 arg)
{
	int list_s = NONE;
	ftp_active = 0;
	ftp_working = 1;

relisten:
	if(working && ftp_working) list_s = slisten(webman_config->ftp_port, FTP_BACKLOG);
	else goto end;

	if(list_s < 0)
	{
		sys_ppu_thread_sleep(1);
		if(working) goto relisten;
		else goto end;
	}

	active_socket[0] = list_s;

	//if(list_s >= 0)
	{
		while(working)
		{
			sys_ppu_thread_usleep(50000);
			int conn_s_ftp;
			if(!working || !ftp_working) break;
			if(ftp_active > MAX_FTP_THREADS) continue;

			if(sys_admin && ((conn_s_ftp = accept(list_s, NULL, NULL)) >= 0))
			{
				sys_ppu_thread_t t_id;
				if(working) sys_ppu_thread_create(&t_id, handleclient_ftp, (u64)conn_s_ftp, THREAD_PRIO_FTP, THREAD_STACK_SIZE_FTP_CLIENT, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_FTPD);
				else {sclose(&conn_s_ftp); break;}
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
	sys_ppu_thread_exit(0);
}
