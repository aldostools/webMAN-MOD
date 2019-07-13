#ifdef USE_NTFS
#define MAX_SECTIONS	(int)((_64KB_-sizeof(rawseciso_args))/8)

//static char paths [13][12] = {"GAMES", "GAMEZ", "PS3ISO", "BDISO", "DVDISO", "PS2ISO", "PSXISO", "PSXGAMES", "PSPISO", "ISO", "video", "GAMEI", "ROMS"};

enum ntfs_folders
{
	mPS3 = 2,
	mBLU = 3,
	mDVD = 4,
	mPS2 = 5,
	mPSX = 6,
	mMAX = 7,
};

static u8 prepntfs_working = false;

static int prepNTFS(u8 clear)
{
	if(prepntfs_working || skip_prepntfs) {skip_prepntfs = false; return 0;}
	prepntfs_working = true;

	int i, parts, dlen, count = 0;

	unsigned int num_tracks;
	int emu_mode = 0;
	TrackDef tracks[32];
	ScsiTrackDescriptor *scsi_tracks;

	rawseciso_args *p_args;

	CellFsDirent dir;

	DIR_ITER *pdir = NULL, *psubdir= NULL;
	struct stat st;
	bool has_dirs, is_iso = false;

	char prefix[2][8]={"/", "/PS3/"};

	u8  *plugin_args = NULL;
	u32 *sectionsP = NULL;
	u32 *sections_sizeP = NULL;

	cellFsMkdir(WMTMP, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
	cellFsChmod(WMTMP, S_IFDIR | 0777);
	cellFsUnlink((char*)WMTMP "/games.html");
	int fd = NONE;
	char path[STD_PATH_LEN], subpath[STD_PATH_LEN], sufix[8], filename[STD_PATH_LEN]; char *path0 = filename;

	check_ntfs_volumes();

	if(cellFsOpendir(WMTMP, &fd) == CELL_FS_SUCCEEDED)
	{
		dlen = sprintf(path, "%s/", WMTMP);
		char *ext, *path_file = path + dlen;
		
		CellFsDirectoryEntry dir; u32 read_f;
		char *entry_name = dir.entry_name.d_name;
		
		while(!cellFsGetDirectoryEntries(fd, &dir, sizeof(dir), &read_f) && read_f)
		{
			ext = strstr(entry_name, ".ntfs[");
			if(ext && (clear || (mountCount <= 0) || (!IS(ext, ".ntfs[BDFILE]") && !IS(ext, ".ntfs[PS2ISO]") && !IS(ext, ".ntfs[PSPISO]")))) {sprintf(path_file, "%s", entry_name); cellFsUnlink(path);}
		}
		cellFsClosedir(fd);
	}

	sys_addr_t addr = NULL;
	sys_addr_t sysmem = NULL;

	if(mountCount <= 0) {mountCount = NTFS_UNMOUNTED; goto exit_prepntfs;}
	{
		sys_memory_container_t vsh_mc = get_vsh_memory_container();
		if(vsh_mc) sys_memory_allocate_from_container(_64KB_, vsh_mc, SYS_MEMORY_PAGE_SIZE_64K, &addr);
		if(!addr && sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &addr) != CELL_OK) goto exit_prepntfs;
	}

	plugin_args    = (u8 *)(addr);
	sectionsP      = (u32*)(addr + sizeof(rawseciso_args));
	sections_sizeP = (u32*)(addr + sizeof(rawseciso_args) + _32KB_);

	size_t plen, nlen, extlen;

	for(i = 0; i < mountCount; i++)
	{
		for(u8 n = 0; n < 2; n++)
		{
			for(u8 profile = 0; profile < 5; profile++)
			{
				sprintf(sufix, "%s", SUFIX(profile));

				for(u8 m = mPS3; m < mMAX; m++)
				{
					has_dirs = false; if(m == mPS2) continue;

					snprintf(path, sizeof(path), "%s:%s%s", mounts[i].name, prefix[n], paths[m]);
					pdir = ps3ntfs_diropen(path);
					if(pdir!=NULL)
					{
						while(ps3ntfs_dirnext(pdir, dir.d_name, &st) == 0)
						{
							if(dir.d_name[0] == '.') continue;

////////////////////////////////////////////////////////
							//--- is SUBFOLDER?
							if(st.st_mode & S_IFDIR)
							{
								sprintf(subpath, "%s:%s%s%s/%s", mounts[i].name, prefix[n], paths[m], sufix, dir.d_name);
								psubdir = ps3ntfs_diropen(subpath);
								if(psubdir==NULL) continue;
								sprintf(subpath, "%s", dir.d_name); has_dirs = true;
next_ntfs_entry:
								if(ps3ntfs_dirnext(psubdir, dir.d_name, &st) < 0) {has_dirs = false; continue;}
								if(dir.d_name[0]=='.') goto next_ntfs_entry;

								dlen = sprintf(path, "%s", dir.d_name);

								is_iso = is_iso_file(dir.d_name, dlen, m, 0);

								if(is_iso)
								{
									dlen -= _IS(path + dlen - 6, ".iso.0") ? 6 : 4;

									if((dlen < 0) || strncmp(subpath, path, dlen))
										sprintf(filename, "[%s] %s", subpath, path);
									else
										sprintf(filename, "%s", path);

									sprintf(dir.d_name, "%s/%s", subpath, path);
								}
							}
							else
							{
								dlen = sprintf(filename, "%s", dir.d_name);

								is_iso = is_iso_file(dir.d_name, dlen, m, 0);
							}
////////////////////////////////////////////////////////////

							if(is_iso)
							{
								plen = snprintf(path, sizeof(path), "%s:%s%s%s/%s", mounts[i].name, prefix[n], paths[m], sufix, dir.d_name);

								parts = ps3ntfs_file_to_sectors(path, sectionsP, sections_sizeP, MAX_SECTIONS, 1);

								// get multi-part file sectors
								if(!extcasecmp(dir.d_name, ".iso.0", 6))
								{
									char iso_name[MAX_PATH_LEN], iso_path[MAX_PATH_LEN];

									size_t nlen = sprintf(iso_name, "%s", path);
									iso_name[nlen - 1] = '\0'; extlen = 6;

									for(u8 o = 1; o < 64; o++)
									{
										if(parts >= MAX_SECTIONS) break;

										sprintf(iso_path, "%s%i", iso_name, o);
										if(file_exists(iso_path) == false) break;

										parts += ps3ntfs_file_to_sectors(iso_path, sectionsP + (parts * sizeof(u32)), sections_sizeP + (parts * sizeof(u32)), MAX_SECTIONS - parts, 1);
									}
								}
								else
									extlen = 4;

								if(parts >= MAX_SECTIONS)
								{
									continue;
								}
								else if(parts > 0)
								{
									num_tracks = 1;
									struct stat bufn; int cd_sector_size = 0, cd_sector_size_param = 0;

										 if(m == mPS3) emu_mode = EMU_PS3;
									else if(m == mBLU) emu_mode = EMU_BD;
									else if(m == mDVD) emu_mode = EMU_DVD;
									else if(m == mPSX)
									{
										emu_mode = EMU_PSX;

										if(ps3ntfs_stat(path, &bufn) < 0) continue;
										cd_sector_size = default_cd_sector_size(bufn.st_size);

										// detect CD sector size
										fd = ps3ntfs_open(path, O_RDONLY, 0);
										if(fd >= 0)
										{
											if(sysmem || sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK)
											{
												char *buffer = (char*)sysmem;
												ps3ntfs_seek64(fd, 0, SEEK_SET);
												ps3ntfs_read(fd, (void *)buffer, _48KB_);
												cd_sector_size = detect_cd_sector_size(buffer);
											}
											ps3ntfs_close(fd);
										}

										if(cd_sector_size & 0xf) cd_sector_size_param = cd_sector_size<<8;
										else if(cd_sector_size != 2352) cd_sector_size_param = cd_sector_size<<4;

										strcpy(path + plen - 3, "CUE");

										fd = ps3ntfs_open(path, O_RDONLY, 0);
										if(fd < 0)
										{
											strcpy(path + plen - 3, "cue");
											fd = ps3ntfs_open(path, O_RDONLY, 0);
										}

										if(fd >= 0)
										{
											if(sysmem || sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK)
											{
												char *cue_buf = (char*)sysmem;
												int cue_size = ps3ntfs_read(fd, (void *)cue_buf, _4KB_);

												char *templn = path;
												num_tracks = parse_cue(templn, cue_buf, cue_size, tracks);
											}
											ps3ntfs_close(fd);
										}
									}

									p_args = (rawseciso_args *)plugin_args; memset(p_args, 0x0, sizeof(rawseciso_args));
									p_args->device = USB_MASS_STORAGE((mounts[i].interface->ioType & 0x0F));
									p_args->emu_mode = emu_mode;
									p_args->num_sections = parts;

									memcpy(plugin_args + sizeof(rawseciso_args) + (parts * sizeof(u32)), sections_sizeP, parts * sizeof(u32));

									if(emu_mode == EMU_PSX)
									{
										int max = MAX_SECTIONS - ((num_tracks * sizeof(ScsiTrackDescriptor)) / 8);

										if(parts >= max)
										{
											continue;
										}

										p_args->num_tracks = num_tracks | cd_sector_size_param;

										scsi_tracks = (ScsiTrackDescriptor *)(plugin_args + sizeof(rawseciso_args) + (2 * (parts * sizeof(u32))));

										if(num_tracks <= 1)
										{
											scsi_tracks[0].adr_control = 0x14;
											scsi_tracks[0].track_number = 1;
											scsi_tracks[0].track_start_addr = 0;
										}
										else
										{
											for(unsigned int t = 0; t < num_tracks; t++)
											{
												scsi_tracks[t].adr_control = (tracks[t].is_audio) ? 0x10 : 0x14;
												scsi_tracks[t].track_number = t + 1;
												scsi_tracks[t].track_start_addr = tracks[t].lba;
											}
										}
									}

									filename[strlen(filename) - extlen] = NULL;
									snprintf(path, sizeof(path), "%s/%s%s.ntfs[%s]", WMTMP, filename, SUFIX2(profile), paths[m]);

									save_file(path, (char*)plugin_args, (sizeof(rawseciso_args) + (2 * (parts * sizeof(u32))) + (num_tracks * sizeof(ScsiTrackDescriptor)))); count++;

									nlen = snprintf(path0, sizeof(path0), "%s:%s%s%s/%s", mounts[i].name, prefix[n], paths[m], sufix, dir.d_name);
									if(!get_image_file(path0, nlen - extlen)) goto next_entry; // not found image in NTFS

									plen = snprintf(path, sizeof(path), "%s/%s", WMTMP, dir.d_name);
									if( get_image_file(path, plen - extlen) ) goto next_entry; // found image in WMTMP

									// copy external image
									strcpy(path + plen - 3, path0 + nlen - 3);
									_file_copy(path0, path);
/*
for_sfo:
									if(m == mPS3) // mount PS3ISO
									{
										strcpy(path + plen - 3, "SFO");
										if(file_exists(path) == false)
										{
											if(isDir("/dev_bdvd")) do_umount(false);

											sys_ppu_thread_create(&thread_id_ntfs, rawseciso_thread, (u64)plugin_args, THREAD_PRIO, THREAD_STACK_SIZE_NTFS_ISO, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_NTFS);

											wait_for("/dev_bdvd/PS3_GAME/PARAM.SFO", 2);

											if(file_exists("/dev_bdvd/PS3_GAME/PARAM.SFO"))
											{
												file_copy("/dev_bdvd/PS3_GAME/PARAM.SFO", path, COPY_WHOLE_FILE);

												strcpy(path + plen - 3, "PNG");
												if(file_exists(path) == false)
													file_copy("/dev_bdvd/PS3_GAME/ICON0.PNG", path, COPY_WHOLE_FILE);
											}

											sys_ppu_thread_t t;
											sys_ppu_thread_create(&t, rawseciso_stop_thread, 0, 0, THREAD_STACK_SIZE_STOP_THREAD, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
											while(rawseciso_loaded) {sys_ppu_thread_usleep(50000);}
										}
									}
*/
								}
							}
//////////////////////////////////////////////////////////////
next_entry:
							if(has_dirs) goto next_ntfs_entry;
//////////////////////////////////////////////////////////////
						}
						ps3ntfs_dirclose(pdir);
					}
				}
			}
		}
	}

exit_prepntfs:
	if(sysmem) sys_memory_free(sysmem);
	if(addr) sys_memory_free(addr);

	prepntfs_working = false;
	return count;
}
#endif
