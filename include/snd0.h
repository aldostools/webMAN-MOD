static sys_ppu_thread_t t_snd0_thread_id = SYS_PPU_THREAD_NONE;
static u8 prev_nosnd0 = 0;

static void snd0_thread(__attribute__((unused)) u64 arg)
{
	int fd;
	if(cellFsOpendir((char*)"/dev_hdd0/game", &fd) == CELL_FS_SUCCEEDED)
	{
		prev_nosnd0 = webman_config->nosnd0;

		CellFsDirectoryEntry entry; size_t read_e; char snd0_file[64];
		int mode = webman_config->nosnd0 ? NOSND : MODE; // toggle file access permissions

		while(working)
		{
			if(cellFsGetDirectoryEntries(fd, &entry, sizeof(entry), &read_e) || !read_e) break;
			if(entry.entry_name.d_namlen != TITLE_ID_LEN) continue;
			sprintf(snd0_file, "%s%s/SND0.AT3",  HDD0_GAME_DIR, entry.entry_name.d_name); cellFsChmod(snd0_file, mode);
			sprintf(snd0_file, "%s%s/ICON1.PAM", HDD0_GAME_DIR, entry.entry_name.d_name); cellFsChmod(snd0_file, mode);
		}
		cellFsClosedir(fd);
	}

	t_snd0_thread_id = SYS_PPU_THREAD_NONE;
	sys_ppu_thread_exit(0);
}

static void mute_snd0(bool scan_gamedir)
{
	cellFsChmod("/dev_bdvd/PS3_GAME/ICON1.PAM", webman_config->nosnd0 ? NOSND : MODE);

	if(!scan_gamedir) return;

	if((t_snd0_thread_id == SYS_PPU_THREAD_NONE) && !payload_ps3hen)
		sys_ppu_thread_create(&t_snd0_thread_id, snd0_thread, NULL, THREAD_PRIO, THREAD_STACK_SIZE_64KB, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_SND0);
}
