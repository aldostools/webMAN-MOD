#ifdef NET_SUPPORT

#ifdef NET3NET4
	const u8 netsrvs = 5;
#else
	const u8 netsrvs = 3;
#endif

typedef struct
{
	char server[0x40];
	char path[0x420];
	u32 emu_mode;
	u32 num_tracks;
	u16 port;
	u8 pad[6];
	ScsiTrackDescriptor tracks[32];
} __attribute__((packed)) _netiso_args;

_netiso_args netiso_args;

static int g_socket = NONE;

#define MAX_RETRIES    3

#define TEMP_NET_PSXCUE  WMTMP "/~netpsx.cue"

static int netiso_svrid = NONE;

static int read_remote_file(int s, void *buf, u64 offset, u32 size, int *abort_connection)
{
	*abort_connection = 1;

	netiso_read_file_cmd cmd;
	netiso_read_file_result res;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = (NETISO_CMD_READ_FILE);
	cmd.offset = (offset);
	cmd.num_bytes = (size);

	if(send(s, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		//DPRINTF("send failed (read_remote_file) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	if(recv(s, &res, sizeof(res), MSG_WAITALL) != sizeof(res))
	{
		//DPRINTF("recv failed (read_remote_file) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	*abort_connection = 0;

	int bytes_read = (res.bytes_read);
	if(bytes_read <= 0)
		return bytes_read;

	if(recv(s, buf, bytes_read, MSG_WAITALL) != bytes_read)
	{
		//DPRINTF("recv failed (read_remote_file) (errno=%d)!\n", get_network_error());
		*abort_connection = 1;
		return FAILED;
	}

	return bytes_read;
}

static s64 open_remote_file(int s, const char *path, int *abort_connection)
{
	*abort_connection = 1;

	s32 net_enabled = 0;
	xsetting_F48C0548()->GetSettingNet_enable(&net_enabled);

	if(!net_enabled) return FAILED;

	netiso_open_cmd cmd;
	netiso_open_result res;
	int len, emu_mode = *abort_connection;

	len = strlen(path);

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = BE16(NETISO_CMD_OPEN_FILE);
	cmd.fp_len = BE16(len);

	if(send(s, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		//DPRINTF("send failed (open_remote_file) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	if(send(s, path, len, 0) != len)
	{
		//DPRINTF("send failed (open_remote_file) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	if(recv(s, &res, sizeof(res), MSG_WAITALL) != sizeof(res))
	{
		//DPRINTF("recv failed (open_remote_file) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	if(res.file_size == NONE)
	{
		//DPRINTF("Remote file %s doesn't exist!\n", path);
		return FAILED;
	}

	// detect CD sector size
	if((emu_mode == EMU_PSX) && (res.file_size >= _64KB_) && (res.file_size <= 0x35000000UL))
	{
		CD_SECTOR_SIZE_2352 = default_cd_sector_size(res.file_size);

		sys_addr_t sysmem = NULL; u64 chunk_size = _64KB_;
		if(sys_memory_allocate(chunk_size, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK)
		{
			char *chunk = (char*)sysmem;

			int bytes_read = 0;

			bytes_read = read_remote_file(s, (char*)chunk, 0, chunk_size, abort_connection);
			if(bytes_read)
			{
				CD_SECTOR_SIZE_2352 = detect_cd_sector_size(chunk);
			}

			sys_memory_free((sys_addr_t)sysmem);
		}
	}

	*abort_connection = 0;

	return (res.file_size);
}

static int remote_stat(int s, const char *path, int *is_directory, s64 *file_size, u64 *mtime, u64 *ctime, u64 *atime, int *abort_connection)
{
	*abort_connection = 1;

	s32 net_enabled = 0;
	xsetting_F48C0548()->GetSettingNet_enable(&net_enabled);

	if(!net_enabled) return FAILED;

	netiso_stat_cmd cmd;
	netiso_stat_result res;
	int len;

	len = strlen(path);
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = (NETISO_CMD_STAT_FILE);
	cmd.fp_len = (len);

	if(send(s, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		//DPRINTF("send failed (remote_stat) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	if(send(s, path, len, 0) != len)
	{
		//DPRINTF("send failed (remote_stat) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	if(recv(s, &res, sizeof(res), MSG_WAITALL) != sizeof(res))
	{
		//DPRINTF("recv failed (remote_stat) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	*abort_connection = 0;

	*file_size = (res.file_size);
	if(*file_size == NONE)
		return FAILED;

	*is_directory = res.is_directory;
	*mtime = (res.mtime);
	*ctime = (res.ctime);
	*atime = (res.atime);

	return CELL_OK;
}

#ifdef USE_INTERNAL_PLUGIN
static int read_remote_file_critical(u64 offset, void *buf, u32 size)
{
	netiso_read_file_critical_cmd cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = NETISO_CMD_READ_FILE_CRITICAL;
	cmd.num_bytes = size;
	cmd.offset = offset;

	if(send(g_socket, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		//DPRINTF("send failed (read file) (errno=%d)!\n", sys_net_errno);
		return FAILED;
	}

	if(recv(g_socket, buf, size, MSG_WAITALL) != (int)size)
	{
		//DPRINTF("recv failed (recv file)  (errno=%d)!\n", sys_net_errno);
		return FAILED;
	}

	return CELL_OK;
}

static int process_read_cd_2048_cmd(u8 *buf, u32 start_sector, u32 sector_count)
{
	netiso_read_cd_2048_critical_cmd cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = NETISO_CMD_READ_CD_2048_CRITICAL;
	cmd.start_sector = start_sector;
	cmd.sector_count = sector_count;

	if(send(g_socket, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		//DPRINTF("send failed (read 2048) (errno=%d)!\n", sys_net_errno);
		return FAILED;
	}

	if(recv(g_socket, buf, sector_count * CD_SECTOR_SIZE_2048, MSG_WAITALL) != (int)(sector_count * CD_SECTOR_SIZE_2048))
	{
		//DPRINTF("recv failed (read 2048)  (errno=%d)!\n", sys_net_errno);
		return FAILED;
	}

	return CELL_OK;
}

static int process_read_iso_cmd(u8 *buf, u64 offset, u32 size)
{
	u64 read_end;

	//DPRINTF("read iso: %p %lx %x\n", buf, offset, size);
	read_end = offset + size;

	if(read_end >= discsize)
	{
		//DPRINTF("Read beyond limits: %llx %x (discsize=%llx)!\n", offset, size, discsize);

		if(offset >= discsize)
		{
			memset(buf, 0, size);
			return CELL_OK;
		}

		memset(buf + (discsize - offset), 0, read_end - discsize);
		size = discsize - offset;
	}

	return read_remote_file_critical(offset, buf, size);
}

static int process_read_cd_2352_cmd(u8 *buf, u32 sector, u32 remaining)
{
	int cache = 0;

	if(remaining <= CD_CACHE_SIZE)
	{
		int dif = (int)cached_cd_sector - sector;

		if(ABS(dif) < CD_CACHE_SIZE)
		{
			u8 *copy_ptr = NULL;
			u32 copy_offset = 0;
			u32 copy_size = 0;

			if(dif > 0)
			{
				if(dif < (int)remaining)
				{
					copy_ptr = cd_cache;
					copy_offset = dif;
					copy_size = remaining - dif;
				}
			}
			else
			{
				copy_ptr = cd_cache + ((-dif) * CD_SECTOR_SIZE_2352);
				copy_size = MIN((int)remaining, CD_CACHE_SIZE + dif);
			}

			if(copy_ptr)
			{
				memcpy(buf + (copy_offset * CD_SECTOR_SIZE_2352), copy_ptr, copy_size * CD_SECTOR_SIZE_2352);

				if(remaining == copy_size)
				{
					return CELL_OK;
				}

				remaining -= copy_size;

				if(dif <= 0)
				{
					u32 newsector = cached_cd_sector + CD_CACHE_SIZE;
					buf += ((newsector-sector) * CD_SECTOR_SIZE_2352);
					sector = newsector;
				}
			}
		}

		cache = 1;
	}

	if(!cache)
	{
		return process_read_iso_cmd(buf, sector * CD_SECTOR_SIZE_2352, remaining * CD_SECTOR_SIZE_2352);
	}

	if(!cd_cache)
	{
		sys_addr_t addr = NULL;

		int ret = sys_memory_allocate(_128KB_, SYS_MEMORY_PAGE_SIZE_64K, &addr);
		if(ret != CELL_OK)
		{
			//DPRINTF("sys_memory_allocate failed: %x\n", ret);
			return ret;
		}

		cd_cache = (u8 *)addr;
	}

	if(process_read_iso_cmd(cd_cache, sector * CD_SECTOR_SIZE_2352, CD_CACHE_SIZE * CD_SECTOR_SIZE_2352) != 0)
		return FAILED;

	memcpy(buf, cd_cache, remaining * CD_SECTOR_SIZE_2352);
	cached_cd_sector = sector;

	return CELL_OK;
}

static u8 netiso_loaded = 0;
static sys_event_queue_t command_queue_net = NONE;
static sys_ppu_thread_t thread_id_net = SYS_PPU_THREAD_NONE;

static void netiso_thread(__attribute__((unused)) u64 arg)
{
	unsigned int real_disctype;
	ScsiTrackDescriptor *tracks;
	int emu_mode, num_tracks;
	sys_event_port_t result_port = (sys_event_port_t)(NONE);

	emu_mode = netiso_args.emu_mode & 0xF;
	CD_SECTOR_SIZE_2352 = 2352;

	//DPRINTF("Hello VSH\n");

	g_socket = connect_to_server(netiso_args.server, netiso_args.port);
	if(g_socket < 0 && !IS(webman_config->allow_ip, netiso_args.server))
	{
		// retry using ip of the remote connection
		g_socket = connect_to_server(webman_config->allow_ip, netiso_args.port);
	}

	if(g_socket < 0)
	{
		goto exit_netiso;
	}

	int ret = emu_mode;

	s64 size = open_remote_file(g_socket, netiso_args.path, &ret);
	if(size < 0)
	{
		goto exit_netiso;
	}

	discsize = (u64)size;

	ret = sys_event_port_create(&result_port, 1, SYS_EVENT_PORT_NO_NAME);
	if(ret != CELL_OK)
	{
		//DPRINTF("sys_event_port_create failed: %x\n", ret);
		goto exit_netiso;
	}

	sys_event_queue_attribute_t queue_attr;
	sys_event_queue_attribute_initialize(queue_attr);
	ret = sys_event_queue_create(&command_queue_net, &queue_attr, 0, 1);
	if(ret != CELL_OK)
	{
		//DPRINTF("sys_event_queue_create failed: %x\n", ret);
		goto exit_netiso;
	}

	unsigned int cd_sector_size_param = 0;

	if(emu_mode == EMU_PSX)
	{
		num_tracks = netiso_args.num_tracks;
		tracks = netiso_args.tracks;

		is_cd2352 = 1;

		if(discsize % CD_SECTOR_SIZE_2352)
		{
			discsize -= (discsize % CD_SECTOR_SIZE_2352);
		}

		if(CD_SECTOR_SIZE_2352 & 0xf) cd_sector_size_param = CD_SECTOR_SIZE_2352<<8;
		else if(CD_SECTOR_SIZE_2352 != 2352) cd_sector_size_param = CD_SECTOR_SIZE_2352<<4;
	}
	else
	{
		num_tracks = 0;
		tracks = NULL;
		is_cd2352 = 0;
	}

	sys_storage_ext_get_disc_type(&real_disctype, NULL, NULL);

	if(real_disctype != DISC_TYPE_NONE)
	{
		fake_eject_event(BDVD_DRIVE);
	}

	ret = sys_storage_ext_mount_discfile_proxy(result_port, command_queue_net, emu_mode, discsize, _256KB_, (num_tracks | cd_sector_size_param), tracks);
	//DPRINTF("mount = %x\n", ret);

	fake_insert_event(BDVD_DRIVE, real_disctype);

	if(ret != CELL_OK)
	{
		sys_event_port_destroy(result_port);
		goto exit_netiso;
	}

	netiso_loaded = 1;

	while(netiso_loaded)
	{
		sys_event_t event;

		ret = sys_event_queue_receive(command_queue_net, &event, 0);
		if(ret != CELL_OK)
		{
			//DPRINTF("sys_event_queue_receive failed: %x\n", ret);
			break;
		}

		if(!netiso_loaded) break;

		void *buf = (void *)(u32)(event.data3>>32ULL);
		u64 offset = event.data2;
		u32 size = event.data3&0xFFFFFFFF;

		switch(event.data1)
		{
			case CMD_READ_ISO:
			{
				if(is_cd2352)
				{
					ret = process_read_cd_2048_cmd(buf, offset / CD_SECTOR_SIZE_2048, size / CD_SECTOR_SIZE_2048);
				}
				else
				{
					ret = process_read_iso_cmd(buf, offset, size);
				}
			}
			break;

			case CMD_READ_CD_ISO_2352:
			{
				ret = process_read_cd_2352_cmd(buf, offset / CD_SECTOR_SIZE_2352, size / CD_SECTOR_SIZE_2352);
			}
			break;
		}

		while(netiso_loaded)
		{
			ret = sys_event_port_send(result_port, ret, 0, 0);
			if(ret == CELL_OK) break;

			if(ret == (int) 0x8001000A)
			{   // EBUSY
				sys_ppu_thread_usleep(100000);
				continue;
			}

			break;
		}

		//DPRINTF("sys_event_port_send failed: %x\n", ret);
		if(ret != 0) break;
	}

exit_netiso:

	//if(args) sys_memory_free((sys_addr_t)args);

	if(command_queue_net != SYS_EVENT_QUEUE_NONE)
	{
		sys_event_queue_destroy(command_queue_net, SYS_EVENT_QUEUE_DESTROY_FORCE);
	}

	sys_storage_ext_get_disc_type(&real_disctype, NULL, NULL);
	fake_eject_event(BDVD_DRIVE);
	sys_storage_ext_umount_discfile();

	if(real_disctype != DISC_TYPE_NONE)
	{
		fake_insert_event(BDVD_DRIVE, real_disctype);
	}

	if(cd_cache)
	{
		sys_memory_free((sys_addr_t)cd_cache);
	}

	if(g_socket >= 0)
	{
		sclose(&g_socket);
	}

	sys_event_port_disconnect(result_port);

	if(sys_event_port_destroy(result_port) != CELL_OK)
	{
		//DPRINTF("Error destroyng result_port\n");
	}

	//DPRINTF("Exiting main thread!\n");
	netiso_loaded = 0;
	netiso_svrid = NONE;

	sys_ppu_thread_exit(0);
}

static void netiso_stop_thread(__attribute__((unused)) u64 arg)
{
	u64 exit_code;
	netiso_loaded = 0;
	netiso_svrid = NONE;

	if(g_socket >= 0)
	{
		sclose(&g_socket);
	}

	if(command_queue_net != SYS_EVENT_QUEUE_NONE)
	{
		if(sys_event_queue_destroy(command_queue_net, SYS_EVENT_QUEUE_DESTROY_FORCE) != 0)
		{
			//DPRINTF("Failed in destroying command_queue_net\n");
		}
	}

	if(thread_id_net != SYS_PPU_THREAD_NONE)
	{
		sys_ppu_thread_join(thread_id_net, &exit_code);
	}

	sys_ppu_thread_exit(0);
}
#endif // #ifdef USE_INTERNAL_PLUGIN

static bool is_netsrv_enabled(u8 server_id)
{
	server_id &= 0x0F; // change '0'-'4' to  0..4

	if(server_id > 4) return false;

	s32 net_enabled = 0;
	xsetting_F48C0548()->GetSettingNet_enable(&net_enabled);

	if(!net_enabled) return false;

	return( (webman_config->netd[server_id] == 1) && // is enabled
			(webman_config->neth[server_id][0] != NULL) && // has host
			(webman_config->netp[server_id] > 0) && // has port
			!islike(webman_config->neth[server_id], "127.") && !islike(webman_config->neth[server_id], "localhost") // not a loopback
		);
}

static int connect_to_remote_server(u8 server_id)
{
	int ns = FAILED;

	server_id &= 0x0F; // change '0'-'4' to  0..4

	if( is_netsrv_enabled(server_id) )
	{
		// check duplicated connections
		for(u8 n = 0; n < server_id; n++)
			if((webman_config->netd[n] == 1) && IS(webman_config->neth[n], webman_config->neth[server_id]) && webman_config->netp[n] == webman_config->netp[server_id]) return FAILED;

		u8 retries = 0, rcv_timeout = 30, max_tries = MAX_RETRIES;

		if(refreshing_xml)
		{
			rcv_timeout = 5, max_tries = 1;
		}

	reconnect:

		ns = connect_to_server_ex(webman_config->neth[server_id], webman_config->netp[server_id], rcv_timeout);

		if(ns < 0)
		{
			if(retries < max_tries)
			{
				retries++;
				sys_ppu_thread_sleep(1);
				goto reconnect;
			}

			if(refreshing_xml)
				webman_config->netd[server_id] = 0; // disable connection to offline server

			if(server_id > 0 || !webman_config->netd[0] || islike(webman_config->allow_ip, "127.") || IS(webman_config->allow_ip, "localhost")) return ns;

			for(u8 n = 1; n < netsrvs; n++)
				if(IS(webman_config->neth[n], webman_config->allow_ip)) return ns;

			// retry using IP of client (/net0 only) - update IP in neth[0] if connection is successful
			ns = connect_to_server_ex(webman_config->allow_ip, webman_config->netp[0], rcv_timeout);
			if(ns >= 0) strcpy(webman_config->neth[0], webman_config->allow_ip);
		}
	}
	return ns;
}

static int open_remote_dir(int s, const char *path, int *abort_connection)
{
	*abort_connection = 1;

	s32 net_enabled = 0;
	xsetting_F48C0548()->GetSettingNet_enable(&net_enabled);

	if(!net_enabled) 
	{
		return FAILED;
	}

	netiso_open_dir_cmd cmd;
	netiso_open_dir_result res;
	int len;

	len = strlen(path);
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = (NETISO_CMD_OPEN_DIR);
	cmd.dp_len = (len);

	if(send(s, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		//DPRINTF("send failed (open_remote_dir) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	if(send(s, path, len, 0) != len)
	{
		//DPRINTF("send failed (open_remote_dir) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	if(recv(s, &res, sizeof(res), MSG_WAITALL) != sizeof(res))
	{
		//DPRINTF("recv failed (open_remote_dir) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	*abort_connection = 0;

	return (res.open_result);
}

static int read_remote_dir(int s, sys_addr_t *data /*netiso_read_dir_result_data **data*/, int *abort_connection)
{
	*abort_connection = 1;

	netiso_read_dir_entry_cmd cmd;
	netiso_read_dir_result res;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = (NETISO_CMD_READ_DIR);

	//MM_LOG("Sending request...(%i) ", s);
	if(send(s, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
	//MM_LOG("FAILED!\n");
		return FAILED;
	}
	//MM_LOG("Receiving response...");
	if(recv(s, &res, sizeof(res), MSG_WAITALL) != sizeof(res))
	{
	//MM_LOG("FAILED!\n");
		return FAILED;
	}

	//MM_LOG("OK (%i entries)\n", res.dir_size );
	if(res.dir_size > 0)
	{
		int len;
		sys_addr_t data1 = NULL;
		for(int retry = 25; retry > 0; retry--)
		{
			if(res.dir_size > (retry * 123)) res.dir_size = retry * 123;
			len = (sizeof(netiso_read_dir_result_data)*res.dir_size);

			int len2 = ((len + _64KB_) / _64KB_) * _64KB_;

			if(sys_memory_allocate(len2, SYS_MEMORY_PAGE_SIZE_64K, &data1) == CELL_OK)
			{
				u8 *data2 = (u8*)data1; *data = data1;

				if(recv(s, data2, len, MSG_WAITALL) != len)
				{
					sys_memory_free(data1);
					*data = NULL;
					return FAILED;
				}
				break;
			}
			else
				*data = NULL;
		}
	}
	else
		*data = NULL;

	*abort_connection = 0;

	return (res.dir_size);
}

static int copy_net_file(const char *local_file, const char *remote_file, int ns, u64 maxbytes)
{
	copy_aborted = false;

	if(ns < 0) return FAILED;

	if(file_exists(local_file)) return CELL_OK; // local file already exists

	s64 file_size = 0; int abort_connection = 0;

	//
	int is_directory = 0; u64 mtime, ctime, atime;
	if(remote_stat(ns, remote_file, &is_directory, &file_size, &mtime, &ctime, &atime, &abort_connection) != CELL_OK || file_size <= 0) return FAILED;
	//

	int ret = FAILED;

	file_size = open_remote_file(ns, remote_file, &abort_connection);

	if(file_size > 0)
	{
		sys_addr_t sysmem = NULL; u64 chunk_size = _64KB_;

		if(sys_memory_allocate(chunk_size, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK)
		{
			char *chunk = (char*)sysmem; int fdw;

			if(cellFsOpen(local_file, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fdw, NULL, 0) == CELL_FS_SUCCEEDED)
			{
				if(maxbytes > 0UL && (u64)file_size > maxbytes) file_size = maxbytes;

				if(chunk_size > (u64)file_size) chunk_size = (u64)file_size;

				int bytes_read, boff = 0;
				while(boff < file_size)
				{
					if(copy_aborted) break;

					bytes_read = read_remote_file(ns, (char*)chunk, boff, chunk_size, &abort_connection);
					if(bytes_read)
						cellFsWrite(fdw, (char*)chunk, bytes_read, NULL);

					boff += bytes_read;
					if(((u64)bytes_read < chunk_size) || abort_connection) break;
				}
				cellFsClose(fdw);
				cellFsChmod(local_file, MODE);

				ret = CELL_OK;
			}
			sys_memory_free(sysmem);
		}
	}

	open_remote_file(ns, "/CLOSEFILE", &abort_connection);

	return ret;
}
#endif //#ifdef NET_SUPPORT
