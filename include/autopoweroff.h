static u16 plugin_active = 0;

#ifdef AUTO_POWER_OFF

#define AUTO_POWER_OFF_BACKUP_FILE   WMTMP "/auto_power_off"

static int AutoPowerOffGame = -1;
static int AutoPowerOffVideo = -1;

static void setAutoPowerOff(bool disable)
{
	if((c_firmware < 4.46f) || webman_config->auto_power_off) return;

	if(AutoPowerOffGame < 0)
	{
		xsetting_D0261D72()->loadRegistryIntValue(0x33, &AutoPowerOffGame);
		xsetting_D0261D72()->loadRegistryIntValue(0x32, &AutoPowerOffVideo);
	}

	xsetting_D0261D72()->saveRegistryIntValue(0x33, disable ? 0 : AutoPowerOffGame);
	xsetting_D0261D72()->saveRegistryIntValue(0x32, disable ? 0 : AutoPowerOffVideo);

	if(disable)
	{
		int fd;
		if(cellFsOpen(AUTO_POWER_OFF_BACKUP_FILE, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
		{
			cellFsWrite(fd, (void *)&AutoPowerOffGame,  sizeof(int), NULL);
			cellFsWrite(fd, (void *)&AutoPowerOffVideo, sizeof(int), NULL);
			cellFsClose(fd);
		}
	}
	else
		cellFsUnlink(AUTO_POWER_OFF_BACKUP_FILE);
}

static void restoreAutoPowerOff(void)
{
	int fd;
	if(cellFsOpen(AUTO_POWER_OFF_BACKUP_FILE, CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		cellFsRead(fd, &AutoPowerOffGame,  sizeof(int), NULL);
		cellFsRead(fd, &AutoPowerOffVideo, sizeof(int), NULL);
		cellFsClose(fd);

		setAutoPowerOff(false);
	}
}

static void setPluginActive(void)
{
	if(plugin_active == 0) setAutoPowerOff(true);
	plugin_active++;
}

static void setPluginInactive(void)
{
	if(plugin_active > 0)
	{
		plugin_active--;
		if(plugin_active == 0) setAutoPowerOff(false);
	}
}

#else
 #define setPluginActive()
 #define setPluginInactive()
#endif

static void setPluginExit(void)
{
	working = plugin_active = 0;

	#ifdef AUTO_POWER_OFF
	setAutoPowerOff(false);
	#endif

	del_turnoff(0);
}
