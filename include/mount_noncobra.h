#ifndef COBRA_ONLY
install_mm_payload:

	if(c_firmware == 0.0f) {ret = false; goto exit_mount;}

	install_peek_poke();

	if(action) eject_insert(1, 1);

	pokeq(0x8000000000000000ULL + MAP_ADDR, 0x0000000000000000ULL);
	pokeq(0x8000000000000008ULL + MAP_ADDR, 0x0000000000000000ULL);

	if(base_addr == 0 || SYSCALL_TABLE == 0) {ret = false; goto exit_mount;}

	// restore syscall table
	{
		u64 sc_null = peekq(SYSCALL_TABLE);

		if(peekq(SYSCALL_PTR(79)) == sc_null)
		{
			for(u8 sc = 35; sc < 39; sc++)
				if(peekq(SYSCALL_PTR(sc)) != sc_null) pokeq(SYSCALL_PTR(sc), sc_null);
			//pokeq(SYSCALL_PTR(1023), sc_null);

			if(sc_600)
			{   // restore original values
				sc_600 |= 0x8000000000000000ULL;
				sc_604 |= 0x8000000000000000ULL;
				sc_142 |= 0x8000000000000000ULL;

				if(peekq(SYSCALL_PTR(600)) != sc_600) pokeq(SYSCALL_PTR(600), sc_600); // sys_storage_open 600
				if(peekq(SYSCALL_PTR(604)) != sc_604) pokeq(SYSCALL_PTR(604), sc_604); // sys_storage_send_device_cmd 604
				if(peekq(SYSCALL_PTR(142)) != sc_142) pokeq(SYSCALL_PTR(142), sc_142); // sys_timer_sleep 142
			}
		}
	}

	// disable mM path table
	pokeq(0x8000000000000000ULL + MAP_ADDR, 0x0000000000000000ULL);
	pokeq(0x8000000000000008ULL + MAP_ADDR, 0x0000000000000000ULL);

	// disable Iris path table
	pokeq(0x80000000007FD000ULL,			0x0000000000000000ULL);

	// restore hook used by all payloads)
	pokeq(open_hook + 0x00, 0xF821FF617C0802A6ULL);
	pokeq(open_hook + 0x08, 0xFB810080FBA10088ULL);
	pokeq(open_hook + 0x10, 0xFBE10098FB410070ULL);
	pokeq(open_hook + 0x18, 0xFB610078F80100B0ULL);
	pokeq(open_hook + 0x20, 0x7C9C23787C7D1B78ULL);

	// poke mM payload
	pokeq(base_addr + 0x00, 0x7C7D1B783B600001ULL);
	pokeq(base_addr + 0x08, 0x7B7BF806637B0000ULL | MAP_ADDR);
	pokeq(base_addr + 0x10, 0xEB5B00002C1A0000ULL);
	pokeq(base_addr + 0x18, 0x4D820020EBFB0008ULL);
	pokeq(base_addr + 0x20, 0xE8BA00002C050000ULL);
	pokeq(base_addr + 0x28, 0x418200CC7FA3EB78ULL);
	pokeq(base_addr + 0x30, 0xE89A001089640000ULL);
	pokeq(base_addr + 0x38, 0x892300005560063EULL);
	pokeq(base_addr + 0x40, 0x7F895800409E0040ULL);
	pokeq(base_addr + 0x48, 0x2F8000007CA903A6ULL);
	pokeq(base_addr + 0x50, 0x409E002448000030ULL);
	pokeq(base_addr + 0x58, 0x8964000089230000ULL);
	pokeq(base_addr + 0x60, 0x5560063E7F895800ULL);
	pokeq(base_addr + 0x68, 0x2F000000409E0018ULL);
	pokeq(base_addr + 0x70, 0x419A001438630001ULL);
	pokeq(base_addr + 0x78, 0x388400014200FFDCULL);
	pokeq(base_addr + 0x80, 0x4800000C3B5A0020ULL);
	pokeq(base_addr + 0x88, 0x4BFFFF98E89A0018ULL);
	pokeq(base_addr + 0x90, 0x7FE3FB7888040000ULL);
	pokeq(base_addr + 0x98, 0x2F80000098030000ULL);
	pokeq(base_addr + 0xA0, 0x419E00187C691B78ULL);
	pokeq(base_addr + 0xA8, 0x8C0400012F800000ULL);
	pokeq(base_addr + 0xB0, 0x9C090001409EFFF4ULL);
	pokeq(base_addr + 0xB8, 0xE8BA00087C632A14ULL);
	pokeq(base_addr + 0xC0, 0x7FA4EB78E8BA0000ULL);
	pokeq(base_addr + 0xC8, 0x7C842A1488040000ULL);
	pokeq(base_addr + 0xD0, 0x2F80000098030000ULL);
	pokeq(base_addr + 0xD8, 0x419E00187C691B78ULL);
	pokeq(base_addr + 0xE0, 0x8C0400012F800000ULL);
	pokeq(base_addr + 0xE8, 0x9C090001409EFFF4ULL);
	pokeq(base_addr + 0xF0, 0x7FFDFB787FA3EB78ULL);
	pokeq(base_addr + 0xF8, 0x4E8000204D4D504CULL); //blr + "MMPL"

	pokeq(MAP_BASE  + 0x00, 0x0000000000000000ULL);
	pokeq(MAP_BASE  + 0x08, 0x0000000000000000ULL);
	pokeq(MAP_BASE  + 0x10, 0x8000000000000000ULL);
	pokeq(MAP_BASE  + 0x18, 0x8000000000000000ULL);

	pokeq(0x8000000000000000ULL + MAP_ADDR, MAP_BASE);
	pokeq(0x8000000000000008ULL + MAP_ADDR, 0x80000000007FDBE0ULL);

	pokeq(open_hook + 0x20, (0x7C9C237848000001ULL | (base_addr-open_hook-0x24)));


	char path[STD_PATH_LEN];

	#ifdef EXT_GDATA

	//------------------
	// re-load last game
	//------------------

	if(action == MOUNT_EXT_GDATA) // extgd
	{
		// get last game path
		get_last_game(_path);
	}

	#endif //#ifdef EXT_GDATA

	sprintf(path, "%s", _path);

	if(!isDir(path)) *_path = *path = NULL;

	// -- get TitleID from PARAM.SFO
	#ifndef FIX_GAME
	{
		char filename[STD_PATH_LEN + 20];

		sprintf(filename, "%s/PS3_GAME/PARAM.SFO", _path);
		getTitleID(filename, title_id, GET_TITLE_ID_ONLY);
	}
	#else
		fix_game(_path, title_id, webman_config->fixgame);
	#endif //#ifndef FIX_GAME
	// ----

	//----------------------------------
	// map game to /dev_bdvd & /app_home
	//----------------------------------

	if(*path)
	{
		if(action)
		{
			add_to_map("/dev_bdvd", path);
			add_to_map("//dev_bdvd", path);

			char path2[strlen(_path) + 24];

			sprintf(path2, "%s/PS3_GAME", _path);
			add_to_map("/app_home/PS3_GAME", path2);

			sprintf(path2, "%s/PS3_GAME/USRDIR", _path);
			add_to_map("/app_home/USRDIR", path2);

			sprintf(path2, "%s/PS3_GAME/USRDIR/", _path);
			add_to_map("/app_home/", path2);
		}

		add_to_map("/app_home", path);
	}

	#ifdef EXT_GDATA

	//---------------------------------------------
	// auto-map /dev_hdd0/game to dev_usbxxx/GAMEI
	//---------------------------------------------

	if(action != MOUNT_NORMAL) ;

	else if(strstr(_path, "/GAME"))
	{
		char extgdfile[STD_PATH_LEN + 24], *extgdini = extgdfile;
		sprintf(extgdfile, "%s/PS3_GAME/PS3GAME.INI", _path);
		if(read_file(extgdfile, extgdini, 12, 0))
		{
			if((extgd == 0) &&  (extgdini[10] & (1<<1))) set_gamedata_status(1, false); else
			if((extgd == 1) && !(extgdini[10] & (1<<1))) set_gamedata_status(0, false);
		}
		else if(extgd) set_gamedata_status(0, false);
	}

	#endif

	//----------------------------
	// Patched explore_plugin.sprx
	//----------------------------
	{
		char expplg[64];
		char app_sys[40];

			sprintf(app_sys, "%s/sys", MM_ROOT_STD);
		if(!isDir(app_sys))
			sprintf(app_sys, "%s/sys", MM_ROOT_STL);
		if(!isDir(app_sys))
			sprintf(app_sys, "%s/sys", MM_ROOT_SSTL);

		if(c_firmware == 3.55f)
			sprintf(expplg, "%s/IEXP0_355.BIN", app_sys);
		else if(c_firmware == 4.21f)
			sprintf(expplg, "%s/IEXP0_420.BIN", app_sys);
		else if(c_firmware == 4.30f || c_firmware == 4.31f)
			sprintf(expplg, "%s/IEXP0_430.BIN", app_sys);
		else if(c_firmware == 4.40f || c_firmware == 4.41f)
			sprintf(expplg, "%s/IEXP0_440.BIN", app_sys);
		else if(c_firmware == 4.46f)
			sprintf(expplg, "%s/IEXP0_446.BIN", app_sys);
		else if(c_firmware >= 4.50f && c_firmware <= 4.55f)
			sprintf(expplg, "%s/IEXP0_450.BIN", app_sys);
		else if(c_firmware >= 4.60f && c_firmware <= 4.66f)
			sprintf(expplg, "%s/IEXP0_460.BIN", app_sys);
		else if(c_firmware >= 4.70f)
			sprintf(expplg, "%s/IEXP0_470.BIN", app_sys);
		else
			sprintf(expplg, "%s/none", app_sys);

		if(action && file_exists(expplg))
			add_to_map(VSH_MODULE_DIR "explore_plugin.sprx", expplg);

		//---------------
		// New libfs.sprx
		//---------------
		if(action && (c_firmware >= 4.20f))
		{
			if(file_exists(NEW_LIBFS_PATH))
				sprintf(expplg, "%s", NEW_LIBFS_PATH);
			else
				sprintf(expplg, "%s/ILFS0_000.BIN", app_sys);

			if(file_exists(expplg))
				add_to_map(ORG_LIBFS_PATH, expplg);
		}
	}

	//-----------------------------------------------//
	u64 map_data  = (MAP_BASE);
	u64 map_paths = (MAP_BASE) + (max_mapped + 1) * 0x20;

	for(u16 n = 0; n < 0x400; n += 8) pokeq(map_data + n, 0); // clear 8KB

	if(!max_mapped) {ret = false; goto exit_mount;}

	u16 src_len, dst_len;

	for(u8 n = 0; n < max_mapped; n++)
	{
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

	wait_for("/dev_bdvd", 2);

	//if(action) eject_insert(0, 1);
#endif
