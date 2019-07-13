#ifdef PS3NET_SERVER

// share *.iso & *.iso.0 stored on local file system (hdd0, usb, ms, cf, sd). JB games, ntfs & net are not shared.

#define MAX_CLIENTS 3

#define CLIENT_BUFFER_SIZE     (0x4000)

static void handleclient_net(u64 arg);
static u8 net_working = 0;

typedef struct {
	int s;
	int fd;
	int fp[MAX_ISO_PARTS];
	u8  is_multipart;
	u8  part;
	u64 part_size;
	u64 file_size;
	int CD_SECTOR_SIZE_2352;
	char dirpath[STD_PATH_LEN];
} _client;

static _client clients[MAX_CLIENTS];

static void init_client(u8 index)
{
	if(clients[index].fd) cellFsClose(clients[index].fd);
	for(u8 i = 0; i < MAX_ISO_PARTS; i++) {if(clients[index].fp[i]) cellFsClose(clients[index].fp[i]); clients[index].fp[i] = NULL;}

	clients[index].fd = NULL;
	clients[index].is_multipart = 0;
	clients[index].part = 0;
	clients[index].part_size = 0;
	clients[index].file_size = 0;
	clients[index].CD_SECTOR_SIZE_2352 = 2352;
	clients[index].dirpath[0] = NULL;
}

static void translate_path(char *path, u16 fp_len)
{
	if(path[0] != '/')
	{
		path[0] = 0;
		return;
	}

	char tmppath[fp_len+1]; sprintf(tmppath, "%s", path);

	for(u8 i = 0; i < 16; i++)
	{
		if(i == 7) i = NTFS + 1; // skip range from /net0 to /ext

		sprintf(path, "%s%s", drives[i], tmppath);

		if(file_exists(path)) break;
	}

	return;
}

static int process_open_cmd(u8 index, netiso_open_cmd *cmd)
{
	int ret;

	init_client(index);

	u16 fp_len, root_len;

	root_len = 12;
	fp_len = (u16)(cmd->fp_len); if(root_len + fp_len > STD_PATH_LEN) return FAILED;

	/// get file path ///

	char filepath[STD_PATH_LEN];

	int s = clients[index].s;

	filepath[fp_len] = 0;
	ret = recv(s, (void *)filepath, fp_len, 0);
	if(ret != fp_len)
	{
		return FAILED;
	}

	/// translate path ///

	translate_path(filepath, fp_len);
	if(!filepath)
	{
		return FAILED;
	}

	/// init result ///

	netiso_open_result result;
	result.file_size = (s64)(NONE);
	result.mtime = (s64)(0);

	int fd = 0;

	struct CellFsStat st;

	if(cellFsStat(filepath, &st) == CELL_FS_SUCCEEDED)
	{
		/// open file ///
		if(cellFsOpen(filepath, CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
		{
			result.file_size = (s64)(st.st_size);
			result.mtime = (s64)(st.st_mtime);

			/// detect sector size ///

			if((result.file_size > _64KB_) && (result.file_size <= 0x35000000UL))
			{
				sys_addr_t sysmem = NULL; const u64 chunk_size = _64KB_;
				if(sys_memory_allocate(chunk_size, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK)
				{
					char *chunk = (char*)sysmem; u64 read_e;
					cellFsRead(fd, chunk, _64KB_, &read_e);
					clients[index].CD_SECTOR_SIZE_2352 = detect_cd_sector_size(chunk);
					sys_memory_free((sys_addr_t)sysmem);
				}
			}

			/// detect multi part ISO - open file parts ///

			if(!extcasecmp(filepath, ".iso.0", 6))
			{
				clients[index].fp[0] = fd;
				clients[index].is_multipart = 1;
				clients[index].part_size = (s64)(st.st_size);

				fp_len = strlen(filepath) - 1;

				for(u8 i = 1; i < MAX_ISO_PARTS; i++)
				{
					sprintf(filepath + fp_len, "%i", i);

					if(cellFsStat(filepath, &st) != CELL_FS_SUCCEEDED) break;

					result.file_size += (s64)(st.st_size);

					if(cellFsOpen(filepath, CELL_FS_O_RDONLY, &fd, NULL, 0) != CELL_FS_SUCCEEDED) break;

					clients[index].fp[i] = fd;
					clients[index].is_multipart = i + 1;
				}

				fd = clients[index].fp[0];
			}
		}
	}

	memset(filepath, 0, root_len + fp_len);

	/// send result ///

	clients[index].fd = fd;
	clients[index].file_size = result.file_size;

	ret = send(s, (void *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		return FAILED;
	}

	return CELL_OK;
}

static int process_read_file_critical(u8 index, netiso_read_file_critical_cmd *cmd)
{
	if(clients[index].fd == 0) return FAILED;

	/// file seek ///
	u64 bytes_read, offset = cmd->offset;

	if(clients[index].is_multipart)
	{
		clients[index].part = (offset / clients[index].part_size);

		if(cellFsLseek(clients[index].fp[clients[index].part], (offset % clients[index].part_size), SEEK_SET, &bytes_read) != CELL_FS_SUCCEEDED) return FAILED;
	}
	else
	{
		if(cellFsLseek(clients[index].fd, offset, SEEK_SET, &bytes_read) != CELL_FS_SUCCEEDED) return FAILED;
	}

	/// allocate buffer ///
	u32 remaining = cmd->num_bytes;

	u64 read_size = MIN(CLIENT_BUFFER_SIZE, remaining);

	char buffer[read_size];

	/// read sectors 2048 ///

	while (remaining > 0)
	{
		if(remaining < read_size)
		{
			read_size = remaining;
		}

		///////////////
		if(clients[index].is_multipart)
		{
			cellFsRead(clients[index].fp[clients[index].part], &buffer, read_size, &bytes_read);

			if(bytes_read < read_size && clients[index].part < (clients[index].is_multipart-1))
			{
				u64 bytes_read2 = 0;
				char *buffer2 = &buffer[bytes_read];
				cellFsRead(clients[index].fp[clients[index].part + 1], buffer2, read_size - bytes_read, &bytes_read2);
				bytes_read += bytes_read2;
			}
		}
		else
			cellFsRead(clients[index].fd, &buffer, read_size, &bytes_read);
		///////////////

		if(!working || (bytes_read <= 0))
		{
			return FAILED;
		}

		if(send(clients[index].s, buffer, bytes_read, 0) != (int)bytes_read)
		{
			return FAILED;
		}

		remaining -= bytes_read;
	}

	/// exit ///
	return CELL_OK;
}

static int process_read_cd_2048_critical_cmd(u8 index, netiso_read_cd_2048_critical_cmd *cmd)
{
	int s = clients[index].s, fd = clients[index].fd;

	if(fd == 0) return FAILED;

	/// get remaining ///

	u32 remaining = cmd->sector_count, sec_size = clients[index].CD_SECTOR_SIZE_2352;

	/// allocate buffer ///

	char buffer[sec_size];

	/// file seek ///
	u64 offset, bytes_read = 0;
	offset = (u32)(cmd->start_sector) * (sec_size);

	if(cellFsLseek(fd, offset + 24, SEEK_SET, &bytes_read) != CELL_FS_SUCCEEDED) return FAILED;

	/// read 2048 in sectors 2352 ///
	for( ; remaining > 0; remaining--)
	{
		///////////////
		if(!working) return FAILED;

		if(cellFsRead(fd, &buffer, sec_size, NULL) != CELL_FS_SUCCEEDED)
		{
			return FAILED;
		}

		if(send(s, buffer, CD_SECTOR_SIZE_2048, 0) != CD_SECTOR_SIZE_2048)
		{
			return FAILED;
		}
		///////////////
	}

	return CELL_OK;
}

static int process_read_file_cmd(u8 index, netiso_read_file_cmd *cmd)
{
	int s = clients[index].s, fd = clients[index].fd;

	if(fd == 0) return FAILED;

	netiso_read_file_result result;

	u64 offset, bytes_read = 0;
	u32 remaining;

	remaining = (s32)(cmd->num_bytes);

	/// allocate buffer ///

	sys_addr_t sysmem = 0; size_t buffer_size = 0;

	for(u8 n = MAX_PAGES; n > 0; n--)
		if(remaining >= ((n-1) * _64KB_) && sys_memory_allocate(n * _64KB_, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK) {buffer_size = n * _64KB_; break;}

	char *buffer = (char*)sysmem;

	if(buffer_size == 0) goto send_result_read_file;

	if(remaining > buffer_size) goto send_result_read_file;

	/// file seek ///

	offset = (s64)(cmd->offset);

	if(cellFsLseek(clients[index].fd, offset, SEEK_SET, &bytes_read) != CELL_FS_SUCCEEDED)
	{
		goto send_result_read_file;
	}

	/// read file ///

	cellFsRead(clients[index].fd, buffer, remaining, &bytes_read);

send_result_read_file:

	/// send result ///

	result.bytes_read = (s32)(bytes_read);

	if(send(s, (void *)&result, sizeof(result), 0) != 4)
	{
		if(sysmem) sys_memory_free(sysmem);
		return FAILED;
	}

	if(bytes_read > 0 && send(s, buffer, bytes_read, 0) != (int)bytes_read)
	{
		if(sysmem) sys_memory_free(sysmem);
		return FAILED;
	}

	/// free memory ///

	if(sysmem) sys_memory_free(sysmem);
	return CELL_OK;
}

static int process_stat_cmd(u8 index, netiso_stat_cmd *cmd)
{
	int ret, s = clients[index].s;

	u16 fp_len, root_len;

	root_len = 12;
	fp_len = (u16)(cmd->fp_len); if(root_len + fp_len > STD_PATH_LEN) return FAILED;

	/// get file path ///

	char filepath[STD_PATH_LEN];

	filepath[fp_len] = 0;
	ret = recv(s, (void *)filepath, fp_len, 0);
	if(ret != fp_len)
	{
		return FAILED;
	}

	/// translate path ///

	translate_path(filepath, fp_len);
	if(!filepath)
	{
		return FAILED;
	}

	/// file stat ///

	struct CellFsStat st;
	netiso_stat_result result;

	if (file_exists(filepath) == false && !strstr(filepath, "/is_ps3_compat1/") && !strstr(filepath, "/is_ps3_compat2/"))
	{
		result.file_size = (s64)(NONE);
	}
	else
	{
		if((st.st_mode & S_IFDIR) == S_IFDIR)
		{
			result.file_size = (s64)(0);
			result.is_directory = 1;
		}
		else
		{
			result.file_size = (s64)(st.st_size);
			result.is_directory = 0;

			/// calc size of multi-part ///

			if(!extcasecmp(filepath, ".iso.0", 6))
			{
				fp_len = strlen(filepath) - 1;
				for(u8 i = 1; i < MAX_ISO_PARTS; i++)
				{
					filepath[fp_len] = 0; sprintf(filepath, "%s%i", filepath, i);
					if(cellFsStat(filepath, &st) != CELL_FS_SUCCEEDED) break;
					result.file_size += (s64)(st.st_size);
				}
			}

		}

		result.mtime = (s64)(st.st_mtime);
		result.ctime = (s64)(st.st_ctime);
		result.atime = (s64)(st.st_atime);
	}

	memset(filepath, 0, root_len + fp_len);

	/// send result ///

	ret = send(s, (void *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		return FAILED;
	}

	return CELL_OK;
}

static int process_open_dir_cmd(u8 index, netiso_open_dir_cmd *cmd)
{
	u16 dp_len, root_len;

	root_len = 12;
	dp_len = (u16)(cmd->dp_len); if(root_len + dp_len > STD_PATH_LEN) return FAILED;

	/// get file path ///

	char dirpath[STD_PATH_LEN];

	dirpath[dp_len] = 0;

	int ret, s = clients[index].s;

	ret = recv(s, (void *)dirpath, dp_len, 0);
	if(ret != dp_len)
	{
		return FAILED;
	}

	sprintf(clients[index].dirpath, "%s", dirpath);

	/// translate path ///

	translate_path(dirpath, dp_len);
	if(!dirpath)
	{
		return FAILED;
	}

	/// check path ///

	netiso_open_dir_result result;
	result.open_result = (s32)(0);

	if(isDir(dirpath) == false)
	{
		clients[index].dirpath[0] = NULL;
		result.open_result = (s32)(NONE);
	}

	/// send result ///

	ret = send(s, (void *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		return FAILED;
	}

	return CELL_OK;
}

static int process_read_dir_cmd(u8 index, netiso_read_dir_entry_cmd *cmd)
{
	int s = clients[index].s;

	u64 read_e, max_entries = 0;
	u16 d_name_len, dirpath_len, count = 0, flags = 0, d; bool filter;

	/// allocate buffer ///

	sys_addr_t sysmem = 0;

	for(u64 n = MAX_PAGES; n > 0; n--)
		if(sys_memory_allocate(n * _64KB_, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK) {max_entries = (n * _64KB_) / sizeof(netiso_read_dir_result_data); break;}

	(void) cmd;
	netiso_read_dir_result result;
	netiso_read_dir_result_data *dir_entries; dir_entries = (netiso_read_dir_result_data *)sysmem;

	if(max_entries == 0) goto send_result_read_dir_cmd;

	/// get folder path ///

	dirpath_len = strlen(clients[index].dirpath);

	/// filter content ///

	filter = !(dirpath_len > 1 && clients[index].dirpath[dirpath_len - 1] == '/'); // unhide filtered files if path ends with /

	/// do not scan GAMES & GAMEZ ///
	if(filter && (IS(clients[index].dirpath, "/GAMES") || IS(clients[index].dirpath, "/GAMEZ"))) goto send_result_read_dir_cmd;

	struct CellFsStat st;
	CellFsDirent entry;

	int dir;
	char dirpath[STD_PATH_LEN];

	/// list folder in all devices ///

	for(u8 i = 0; i < 16; i++)
	{
		if(count >= max_entries || !working) break;

		if(i == 7) i = NTFS + 1; // skip range from /net0 to /ext

		sprintf(dirpath, "%s%s", drives[i], clients[index].dirpath);

		if(file_exists(dirpath) == false) continue;

		cellFsOpendir(dirpath, &dir); if(!dir) continue;

		dirpath_len = strlen(dirpath);

		while(working && (cellFsReaddir(dir, &entry, &read_e) == CELL_FS_SUCCEEDED) && (read_e > 0))
		{
			if(entry.d_name[0] == '.' && (entry.d_name[1] == '.' || entry.d_name[1] == 0)) continue;

			d_name_len = entry.d_namlen;
			if(d_name_len == 0) continue;

			if(dirpath_len + d_name_len < STD_PATH_LEN - 1)
			{
				sprintf(dirpath, "%s%s/%s", drives[i], clients[index].dirpath, entry.d_name);
				st.st_mode  = S_IFDIR;
				st.st_size  = 0;
				st.st_mtime = 0;
				st.st_atime = 0;
				st.st_ctime = 0;
				cellFsStat(dirpath, &st);

				if(!st.st_mtime) st.st_mtime = st.st_ctime;
				if(!st.st_mtime) st.st_mtime = st.st_atime;

				if((st.st_mode & S_IFDIR) == S_IFDIR)
				{
						/// avoid list duplicated folders (common only) ///
						for(d = 0; d < 11; d++) {if(IS(entry.d_name, paths[d])) break;}
						if(d < 11) {if(flags & (1<<d)) continue; flags |= (1<<d);}

						dir_entries[count].file_size = (0);
						dir_entries[count].is_directory = 1;
				}
				else
				{
						dir_entries[count].file_size =  (s64)(st.st_size);
						dir_entries[count].is_directory = 0;
				}

				snprintf(dir_entries[count].name, 510, "%s", entry.d_name);
				dir_entries[count].mtime = (s64)(st.st_mtime);

				count++;
				if(count >= max_entries) break;
			}
		}

		if(dir) cellFsClosedir(dir);
	}

send_result_read_dir_cmd:

	/// send result ///

	result.dir_size = (s64)(count);
	if(send(s, (void *)&result, sizeof(result), 0) != sizeof(result))
	{
		if(sysmem) sys_memory_free(sysmem);
		return FAILED;
	}

	if(count > 0)
	{
		if(send(s, (void *)dir_entries, (int)(sizeof(netiso_read_dir_result_data) * count), 0) != (int)(sizeof(netiso_read_dir_result_data) * count))
		{
			if(sysmem) sys_memory_free(sysmem);
			return FAILED;
		}
	}

	/// free memory ///

	if(sysmem) sys_memory_free(sysmem);
	return CELL_OK;
}

static void handleclient_net(u64 arg)
{
	u8 index = (u32)arg;

	netiso_cmd cmd;
	int ret;
/*
	sys_net_sockinfo_t conn_info;
	sys_net_get_sockinfo(clients[index].s, &conn_info, 1);

	char ip_address[16];
	sprintf(ip_address, "%s", inet_ntoa(conn_info.remote_adr));

	if(bind_check && webman_config->bind && ((conn_info.local_adr.s_addr!=conn_info.remote_adr.s_addr)  && strncmp(ip_address, webman_config->allow_ip, strlen(webman_config->allow_ip))!=0))
	{
		sclose(&clients[index].s);
		sys_ppu_thread_exit(0);
	}
*/

	while(working)
	{
		if(recv(clients[index].s, (void *)&cmd, sizeof(cmd), 0) > 0)
		{
			switch (cmd.opcode)
			{
				case NETISO_CMD_READ_FILE_CRITICAL:
					ret = process_read_file_critical(index, (netiso_read_file_critical_cmd *)&cmd); // BD & DVD ISO
				break;

				case NETISO_CMD_READ_CD_2048_CRITICAL:
					ret = process_read_cd_2048_critical_cmd(index, (netiso_read_cd_2048_critical_cmd *)&cmd); // CD ISO
				break;

				case NETISO_CMD_READ_FILE:
					ret = process_read_file_cmd(index, (netiso_read_file_cmd *)&cmd);
				break;

				case NETISO_CMD_STAT_FILE:
					ret = process_stat_cmd(index, (netiso_stat_cmd *)&cmd);
				break;

				case NETISO_CMD_OPEN_FILE:
					ret = process_open_cmd(index, (netiso_open_cmd *)&cmd);
				break;

				case NETISO_CMD_OPEN_DIR:
					ret = process_open_dir_cmd(index, (netiso_open_dir_cmd *)&cmd);
				break;

				case NETISO_CMD_READ_DIR:
					ret = process_read_dir_cmd(index, (netiso_read_dir_entry_cmd *)&cmd);
				break;

				default:
					ret = FAILED; // Unknown command received
			}

			if(ret != 0) break;
		}
		else
		{
			break;
		}
	}

	sclose(&clients[index].s);

	clients[index].s = NULL;
	init_client(index);

	sys_ppu_thread_exit(0);
}

static void netsvrd_thread(__attribute__((unused)) u64 arg)
{
	int list_s = NONE;
	net_working = 1;

relisten:
	if(working && net_working) list_s = slisten(webman_config->netsrvp, NET_BACKLOG);
	else goto end;

	if(list_s < 0)
	{
		sys_ppu_thread_sleep(1);
		if(working) goto relisten;
		else goto end;
	}

	active_socket[3] = list_s;

	//if(list_s >= 0)
	{
		while(working)
		{
			sys_ppu_thread_usleep(1668);
			int conn_s_net;
			if(!working || !net_working) break;

			if((conn_s_net = accept(list_s, NULL, NULL)) >= 0)
			{
				// get client slot
				int index = NONE;
				for(u8 i = 0; i < MAX_CLIENTS; i++) if(!clients[i].s) {index = i; break;}
				if(index < 0) {sclose(&conn_s_net); continue;}

				// initizalize client
				init_client(index);
				clients[index].s = conn_s_net;

				// handle client
				sys_ppu_thread_t t_id;
				if(working) sys_ppu_thread_create(&t_id, handleclient_net, (u64)index, THREAD_PRIO_NET, THREAD_STACK_SIZE_NET_CLIENT, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_NETSVRD);
				else {sclose(&conn_s_net); break;}
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

#endif