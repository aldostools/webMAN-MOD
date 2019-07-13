#ifdef NOSINGSTAR

#define XIL2_DIR	"/dev_hdd0/tmp/explore/xil2/game"

static void no_singstar_icon(void)
{
#ifndef USE_OLD_METHOD_FOR_NO_SINGSTAR
	del(XIL2_DIR, RECURSIVE_DELETE);
	if(webman_config->noss)
	{
		save_file(XIL2_DIR, NULL, 0); // DeViL303's new method for no singstar:
		cellFsChmod(XIL2_DIR, 0444); // make fake game dir read only
	}
#else
	int fd;

	if(cellFsOpendir(XIL2_DIR, &fd) == CELL_FS_SUCCEEDED)
	{
		CellFsDirectoryEntry dir; u32 read_e;
		char *country_id = dir.entry_name.d_name;

		while(working && (!cellFsGetDirectoryEntries(fd, &dir, sizeof(dir), &read_e) && read_e))
		{
			if(country_id[0] == '.') continue;
			if(country_id[2] == '\0' && country_id[1] != '\0')
			{
				char country_path[48];

				if(webman_config->noss)
				{
					int fd2;

					sprintf(country_path, "%s/%s/c", XIL2_DIR, country_id);

					if(cellFsOpendir(country_path, &fd2) == CELL_FS_SUCCEEDED)
					{
						char xmlpath[96]; to_upper(country_id);

						CellFsDirectoryEntry dir2;
						char *entry_name = dir2.entry_name.d_name;

						while(working && (!cellFsGetDirectoryEntries(fd2, &dir2, sizeof(dir2), &read_e) && read_e))
						{
							if(entry_name[0] == '.') continue;

							sprintf(xmlpath, "%s/%s/NSXWSV-PN.P3.%s-XMB_COLUMN_GAME.xml", country_path, entry_name, country_id);

							if(file_exists(xmlpath))
							{
								save_file(xmlpath, NULL, 0);
								cellFsChmod(xmlpath, 0444); // make blank xml read only
							}
						}
						cellFsClosedir(fd2);
					}
				}
				else
				{
					sprintf(country_path, "%s/%s", XIL2_DIR, country_id);
					del(country_path, RECURSIVE_DELETE); // allow restore singstar icon
				}
			}
		}
		cellFsClosedir(fd);
	}
#endif
}
#endif
