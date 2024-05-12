// -----------------------
// mount extFAT / NTFS ISO
// -----------------------

char *ntfs_ext = strstr(_path, ".ntfs[");
if(ntfs_ext)
{
	set_mount_type(ntfs_ext);

#ifdef USE_INTERNAL_NTFS_PLUGIN
	// ------------------------------------------------------------------------------------------------------------
	// launch ntfs psx & isos tagged [raw] with external rawseciso sprx (if available) (due support for multi PSX)
	// ------------------------------------------------------------------------------------------------------------
	if(islike(ntfs_ext, ".ntfs[PSXISO]") || (!strstr(_path, "[raw]")))
#endif
	{
		if(multiCD) check_multipsx = !isDir("/dev_usb000"); // check eject/insert USB000 in mount_on_insert_usb()

		const char *rawseciso_sprx[3] = { WM_RES_PATH "/raw_iso.sprx",
										  VSH_MODULE_DIR "raw_iso.sprx",
										  WMTMP "/res/sman.ntf" };

		u8 n;
		for(n = 0; n < 3; n++)
			if(file_exists(rawseciso_sprx[n])) break;

		if(n < 3)
		{
			cellFsChmod(_path, MODE);

			sys_addr_t addr = sys_mem_allocate(_64KB_);
			if(addr)
			{
				char *rawseciso_data = (char *)addr;
				u64 msiz = read_file(_path, rawseciso_data, _64KB_, 0);
				if(msiz > sizeof(rawseciso_args))
				{
					ret = (cobra_load_vsh_plugin(0, (char*)rawseciso_sprx[n], rawseciso_data, msiz) == CELL_OK);
					sys_ppu_thread_sleep(1);
				}
				else
					ret = false;

				sys_memory_free(addr);

#ifdef USE_INTERNAL_NTFS_PLUGIN
				if(ret) goto mounted_ntfs;
#endif
			}
		}
	}

#ifdef USE_INTERNAL_NTFS_PLUGIN
	sys_addr_t addr  = sys_mem_allocate(_64KB_);
	if(addr)
	{
		char *rawseciso_data = (char*)addr;
		if(read_file(_path, rawseciso_data, _64KB_, 0) > sizeof(rawseciso_args))
		{
			sys_ppu_thread_create(&thread_id_ntfs, rawseciso_thread, (u64)addr, THREAD_PRIO, THREAD_STACK_SIZE_NTFS_ISO, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_NTFS);
			sys_ppu_thread_sleep(1);
		}
		else
			ret = false;

		sys_memory_free(addr);
	}

mounted_ntfs:
#endif //#ifdef USE_INTERNAL_NTFS_PLUGIN

	if(ret)
	{
		wait_for("/dev_bdvd", 3);

		if(IS(ntfs_ext, ".ntfs[PS3ISO]"))
		{
			get_name(templn, _path, GET_WMTMP);
			cache_icon0_and_param_sfo(templn);
#ifdef FIX_GAME
			fix_game(_path, title_id, webman_config->fixgame);
#endif
		}

#ifdef PKG_HANDLER
		if(!(webman_config->auto_install_pkg) && IS(ntfs_ext, ".ntfs[BDFILE]") && islike(ntfs_ext - 4, ".pkg"))
		{
			installPKG_all("/dev_bdvd", false);
			goto exit_mount;
		}
#endif
		// cache PS2ISO or PSPISO to HDD0
		bool is_ps2 = IS(ntfs_ext, ".ntfs[PS2ISO]");
		bool is_psp = IS(ntfs_ext, ".ntfs[PSPISO]");

		if(is_psp || is_ps2)
		{
			int fd;

			if(cellFsOpendir("/dev_bdvd", &fd) == CELL_FS_SUCCEEDED)
			{
				CellFsDirent entry; u64 read_e;

				while((cellFsReaddir(fd, &entry, &read_e) == CELL_FS_SUCCEEDED) && (read_e > 0))
				{
					if(entry.d_name[0] != '.') break;
				}
				cellFsClosedir(fd);

				if(entry.d_name[0] == '\0') goto exit_mount;

				sprintf(_path, "/dev_bdvd/%s", entry.d_name);
				if(not_exists(_path)) goto exit_mount;

				if(is_psp)
				{
					strcpy(templn, _path);
					ret = (cobra_set_psp_umd(_path, templn, (char*)"/dev_hdd0/tmp/wm_icons/psp_icon.png") == CELL_FS_SUCCEEDED);
				}
				else //if(is_ps2)
					goto copy_ps2iso_to_hdd0;
			}
		}
	}
	goto exit_mount;
}
