#if defined(EXT_GDATA) || defined(COPY_PS3)
static u8 get_default_usb_drive(const char *folder)
{
	char usb_path[32]; u8 n;
	for(n = 1; n < 7; n++) {sprintf(usb_path, "%s%s", drives[n], folder ? folder : ""); if(isDir(usb_path)) break;}

	if((n < 7) || folder) return n;

	return 1; // 1 = usb000
}
#endif

#ifdef EXT_GDATA

#define MOUNT_EXT_GDATA		2

static u8 extgd = 0;       //external gameDATA

static int set_gamedata_status(u8 status, bool do_mount)
{
	char msg[100];
	char gamei_path[STD_PATH_LEN]; u8 n = 0;

	{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

#ifndef COBRA_ONLY
	sprintf(gamei_path, "/%s", "/dev_hdd0/game");
	if(do_mount) max_mapped = 0;
#endif

	if(status)
	{
		if(status == 2)
		{
			sprintf(gamei_path, "/dev_bdvd/GAMEI"); // auto-enable external gameDATA (if GAMEI exists on /bdvd)
			status = 1;
		}
		else
		{
			n = get_default_usb_drive("/GAMEI"); // find first USB HDD with /GAMEI

			if(n >= 7)
			{
				n = get_default_usb_drive(0); // find first USB HDD, then create /GAMEI folder
			}

			sprintf(gamei_path, "%s/GAMEI", drives[n]);
			cellFsMkdir(gamei_path, DMODE); if(!isDir(gamei_path)) n = 99;
		}

		if(n < 7)
		{
#ifdef COBRA_ONLY
			sys_map_path("/dev_hdd0/game", gamei_path);
			if(isDir(MM_ROOT_STD)) sys_map_path(MM_ROOT_STD, "/" MM_ROOT_STD);
#else
			if(isDir(MM_ROOT_STD)) add_to_map(MM_ROOT_STD, MM_ROOT_STD);
			add_to_map("/dev_hdd0/game", gamei_path);
#endif
			sprintf(msg, "gameDATA %s (%s)", STR_ENABLED, drives[n]);
		}
		else
		{
			extgd = 0;
#ifdef COBRA_ONLY
			sys_map_path("/dev_hdd0/game", NULL);
			{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }
#endif
			sprintf(msg, "gameDATA %s (no usb)", STR_ERROR);
			show_msg(msg);
			return FAILED;
		}
	}
	else
	{
		sprintf(msg, "gameDATA %s", STR_DISABLED);

#ifdef COBRA_ONLY
		sys_map_path("/dev_hdd0/game", NULL);
#else
		add_to_map("/dev_hdd0/game", gamei_path);
#endif
	}

	extgd = status;

	{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }

	if(do_mount)
	{
		show_msg(msg);
#ifndef COBRA_ONLY
		mount_game(gamei_path, MOUNT_EXT_GDATA);
#endif
	}

	return CELL_OK;
}

#endif //#ifdef EXT_GDATA
