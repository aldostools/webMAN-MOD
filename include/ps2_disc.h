#ifdef PS2_DISC

static void do_umount_ps2disc(bool mount);
static bool mount_ps2disc(char *path);

static void do_umount_ps2disc(bool mount)
{
	system_call_3(SC_FS_UMOUNT, (u64)(char*)"/dev_ps2disc", 0, 1);

	if(!mount) return;

	{system_call_8(SC_FS_MOUNT, (u64)(char*)"CELL_FS_IOS:BDVD_DRIVE", (u64)(char*)"CELL_FS_ISO9660", (u64)(char*)"/dev_ps2disc", 0, 1, 0, 0, 0);}
}

static bool mount_ps2disc(char *path)
{
	do_umount_ps2disc(true);
	do_umount_iso();

	if(!isDir(path)) return false;

 #ifdef COBRA_ONLY
	{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

	sys_map_path("/dev_ps2disc", path);

	{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }
 #else

	max_mapped = 0;
	add_to_map("/dev_ps2disc", path);

	u64 map_data  = (MAP_BASE);
	u64 map_paths = (MAP_BASE) + (max_mapped + 1) * 0x20;

	for(u16 n = 0; n < 0x400; n + =8) pokeq(map_data + n, 0);

	for(u8 n = 0; n < max_mapped; n++)
	{
		size_t src_len, dst_len;

		if(map_paths > 0x80000000007FE800ULL) break;
		pokeq(map_data + (n * 0x20) + 0x10, map_paths);
		src_len = string_to_lv2(file_to_map[n].src, map_paths);
		map_paths += src_len; //(src_len + 8) & 0x7f8;

		pokeq(map_data + (n * 0x20) + 0x18, map_paths);
		dst_len = string_to_lv2(file_to_map[n].dst, map_paths);
		map_paths += dst_len; //(dst_len + 8) & 0x7f8;

		pokeq(map_data + (n * 0x20) + 0x00, src_len);
		pokeq(map_data + (n * 0x20) + 0x08, dst_len);
	}

 #endif //#ifdef COBRA_ONLY

	wait_for("/dev_ps2disc", 2);

	return isDir("/dev_ps2disc");
}

#endif //#ifdef PS2_DISC
