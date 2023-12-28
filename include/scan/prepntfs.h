#ifdef USE_NTFS
#define MAX_SECTIONS	(int)((_64KB_-sizeof(rawseciso_args))/8)

static u64 device_id;
static u8  ntfs_m;

static char *ntfs_subdir = NULL;

static u16 ntfs_count = 0;
static u8 prepntfs_working = false;

static u8  *plugin_args = NULL;
static u32 *sectionsP = NULL;
static u32 *sections_sizeP = NULL;

static sys_addr_t sysmem_p = NULL;

// extract ICON0.PNG & PARAM.SFO from ISO
static u64 extract_iso_file(const char *iso_path, const char *file, u8 len, const char *out_path, const char *sector)
{
	if(!sector) return 0;
	u16 start = 0x40; u32 size; fixed_count = 0;
	u64 lba = getLBA(sector, 0x800, file, len, &start, &size);
	if(!lba && (len == 11)) lba = getLBA(sector, 0x800, file, len - 2, &start, &size); // search without ;1
	if(lba)
	{
		if(!size || (size > _62KB_)) return 0; // is file larger than buffer size?
		lba *= 0x800ULL; if(lba > (file_size(iso_path) - size)) return 0; // is offset larger than iso?
		if(!out_path) return lba; // return offset

		char *buffer = (char*)(sector + 0x800); // (0-62KB)
		read_file(iso_path, buffer, size, lba);
		save_file(out_path, buffer, size);
	}
	return lba; // return offset
}

static void create_ntfs_file(char *iso_path, char *filename, size_t plen)
{
	if(!plugin_args || !sectionsP || !sections_sizeP) return;

	const int max_sections = MAX_SECTIONS;

	int parts = ps3ntfs_file_to_sectors(iso_path, sectionsP, sections_sizeP, max_sections, 1);

	if(parts <= 0) return;

	unsigned int num_tracks;
	int emu_mode = EMU_PS3;
	TrackDef tracks[MAX_TRACKS];
	ScsiTrackDescriptor *scsi_tracks;

	u8 extlen = 4, sfo_found = 0;
	char tmp_path[MAX_PATH_LEN];

	rawseciso_args *p_args;

	// get multi-part file sectors
	if(is_iso_0(filename))
	{
		size_t nlen = sprintf(tmp_path, "%s", iso_path);
		extlen = 6, --nlen; int nparts;

		for(u8 o = 1; o < 64; o++)
		{
			if(parts >= max_sections) return;

			sprintf(tmp_path + nlen, "%i", o);
			if(not_exists(tmp_path)) break;

			nparts = ps3ntfs_file_to_sectors(tmp_path, sectionsP + (parts * sizeof(u32)), sections_sizeP + (parts * sizeof(u32)), max_sections - parts, 1);
			if(nparts <= 0) return;

			parts += nparts;
		}
	}

	if(parts >= max_sections)
	{
		return;
	}
	else if(parts > 0)
	{
		num_tracks = 1;
		int cd_sector_size_param = 0;

			 if(ntfs_m == id_PS3ISO) emu_mode = EMU_PS3;
		else if(ntfs_m == id_BDISO ) emu_mode = EMU_BD;
		else if(ntfs_m == id_DVDISO) emu_mode = EMU_DVD;
		else if(ntfs_m == id_PSXISO)
		{
			emu_mode = EMU_PSX;

			struct stat bufn;
			if(ps3ntfs_stat(iso_path, &bufn) < 0) return;

			// detect CD sector size
			char *buffer = (char*)sysmem_p;
			read_file(iso_path, buffer + _32KB_, _8KB_, 0);
			int cd_sector_size = detect_cd_sector_size(buffer);

			// set CD sector size parameter for non 2352 sectors
			if(cd_sector_size & 0xf) cd_sector_size_param = cd_sector_size<<8;
			else if(cd_sector_size != 2352) cd_sector_size_param = cd_sector_size<<4;

			// read LBA for tracks from cue file
			if(change_ext(iso_path, 4, cue_ext))
			{
				char *cue_file = iso_path;
				char *cue_buf = (char*)sysmem_p;
				int cue_size = read_file(cue_file, cue_buf, _8KB_, 0);

				num_tracks = parse_cue(cue_file, cue_buf, cue_size, tracks);
			}
		}

		p_args = (rawseciso_args *)plugin_args; _memset(p_args, sizeof(rawseciso_args));
		p_args->device = device_id;
		p_args->emu_mode = emu_mode;
		p_args->num_sections = parts;

		u32 array_len = parts * sizeof(u32);

		memcpy64(plugin_args + sizeof(rawseciso_args) + array_len, sections_sizeP, array_len);

		int max = max_sections - ((num_tracks * sizeof(ScsiTrackDescriptor)) / 8);

		if(parts >= max)
		{
			return;
		}

		p_args->num_tracks = num_tracks | cd_sector_size_param;

		scsi_tracks = (ScsiTrackDescriptor *)(plugin_args + sizeof(rawseciso_args) + (2 * array_len));

		scsi_tracks[0].adr_control = 0x14;
		scsi_tracks[0].track_number = 1;
		scsi_tracks[0].track_start_addr = 0;

		for(unsigned int t = 1; t < num_tracks; t++)
		{
			scsi_tracks[t].adr_control = 0x10;
			scsi_tracks[t].track_number = t + 1;
			scsi_tracks[t].track_start_addr = tracks[t].lba;
		}

		int slen = strlen(filename) - extlen;
		filename[slen] = '\0'; // truncate file extension

		snprintf(tmp_path, sizeof(tmp_path), "%s/%s%s.SFO", WMTMP, filename, SUFIX2(profile));
		sfo_found = not_exists(tmp_path); if(!sfo_found) {filename[slen] = '.', slen += extlen;} // create file with .iso extension

		// add sub-directory to file name
		if(ntfs_subdir && strncmp(ntfs_subdir, filename, slen))
		{
			sprintf(tmp_path, "[%s] %s", ntfs_subdir, filename);
			strcpy(filename, tmp_path);
		}

		// create .ntfs[] file
		snprintf(tmp_path, sizeof(tmp_path), "%s/%s%s.ntfs[%s]", WMTMP, filename, SUFIX2(profile), paths[ntfs_m]);

		save_file(tmp_path, (char*)plugin_args, (sizeof(rawseciso_args) + (2 * array_len) + (num_tracks * sizeof(ScsiTrackDescriptor)))); ntfs_count++;

		// extract PNG and SFO from ISO
		if(ntfs_m == id_PS3ISO)
		{
			snprintf(tmp_path, sizeof(tmp_path), "%s/%s%s.SFO", WMTMP, filename, SUFIX2(profile));
			if(not_exists(tmp_path) && (file_size(iso_path) > _128KB_))
			{
				char *sector = (char*)sysmem_p;

				// Read root directory of ISO
				read_file(iso_path, sector, 0x800, 0x10000); // get root sector 20
				u64 offset = extract_iso_file(iso_path, "PS3_GAME", 8, NULL, sector); // find offset
				if(offset)
				{
					// extract PARAM.SFO from ISO
					read_file(iso_path, sector, 0x800, offset); // get PS3_GAME sector
					extract_iso_file(iso_path, "PARAM.SFO;1", 11, tmp_path, sector);

					// extract ICON0.PNG from ISO
					snprintf(tmp_path, sizeof(tmp_path), "%s/%s%s.PNG", WMTMP, filename, SUFIX2(profile));
					extract_iso_file(iso_path, "ICON0.PNG;1", 11, tmp_path, sector);

					if(file_exists(tmp_path)) return;
				}
			}
		}

		// copy .PNG and .SFO from NTFS drive
		int tlen = sprintf(tmp_path, "%s/%s", WMTMP, filename);

		// copy external .SFO to WMTMP if exists
		if((ntfs_m == id_PS3ISO) && !sfo_found)
		{
			char *sfo_path = iso_path;
			strcpy(sfo_path + plen, ".SFO");
			if(not_exists(sfo_path))
				strcpy(sfo_path + plen - extlen, ".SFO");
			strcpy(tmp_path + tlen, ".SFO");
			force_copy(sfo_path, tmp_path);
		}

		// copy external .key to WMTMP
		if(ntfs_m == id_PS3ISO)
		{
			char *key_path = iso_path;
			strcpy(key_path + plen, ".key");
			strcpy(tmp_path + tlen, ".key");
			force_copy(key_path, tmp_path);
		}

		// copy external image to WMTMP if exists
		char *img_path = iso_path;
		if(!get_image_file(img_path, plen)) // image not found in NTFS
		if(!get_image_file(img_path, plen - extlen)) return; // check without .iso extension
		if( get_image_file(tmp_path, tlen)) return; // found image in WMTMP

		// copy external image to WMTMP
		force_copy(img_path, tmp_path);
	}
}

static void scan_path_ntfs(const char *path, bool chk_dirs)
{
	DIR_ITER *pdir = NULL;

	typedef struct
	{
		char path[STD_PATH_LEN];
	} t_line_entries;

	pdir = ps3ntfs_opendir(path);
	if(pdir)
	{
		sys_addr_t sysmem = NULL;
		if(chk_dirs) sysmem = sys_mem_allocate(_64KB_);

		t_line_entries *dir_entries = (t_line_entries *)sysmem;
		u16 max_entries = sysmem ? (_64KB_ / sizeof(t_line_entries)) : 0;

		int idx = 0;

		struct stat st;
		CellFsDirent dir;
		char dir_entry[STD_PATH_LEN];
		size_t plen = snprintf(dir_entry, STD_PATH_LEN, "%s/", path);

		while(ps3ntfs_dirnext(pdir, dir.d_name, &st) == 0)
		{
			if(dir.d_name[0] == '.') continue;
			size_t flen = snprintf(dir_entry + plen, STD_PATH_LEN - plen, "%s", dir.d_name);

			if(st.st_mode & S_IFDIR)
			{
				if(idx < max_entries) {sprintf(dir_entries[idx++].path, "%s", dir.d_name);}
			}
			else if(is_iso_file(dir.d_name, flen, ntfs_m, 0))
			{
				create_ntfs_file(dir_entry, dir.d_name, plen + flen);
			}
		}
		ps3ntfs_dirclose(pdir);

		for(int i = 0; i < idx; i++)
		{
			snprintf(dir_entry + plen, STD_PATH_LEN - plen, "%s", dir_entries[i].path);

			ntfs_subdir = dir_entries[i].path;
			for(int c = 0; ntfs_subdir[c]; c++)
				if(ntfs_subdir[c] == '[') ntfs_subdir[c] = '('; else
				if(ntfs_subdir[c] == ']') ntfs_subdir[c] = ')';

			scan_path_ntfs(dir_entry, false);
		}

		if(sysmem) sys_memory_free(sysmem);
	}
}

static int prepNTFS(u8 clear)
{
	if(prepntfs_working || skip_prepntfs) {skip_prepntfs = false; return 0;}
	prepntfs_working = true;

	const char *prefix[2] = {"/", "/PS3/"};

	cellFsMkdir(TMP_DIR, DMODE);
	cellFsMkdir(WMTMP, DMODE);
	cellFsChmod(WMTMP, DMODE);
	cellFsUnlink(WMTMP "/games.html");
	char path[STD_PATH_LEN];

	// remove ntfs files from WMTMP
	const char *cache_ext[4] = { ".ntfs[PS3ISO]", ".ntfs[PSXISO]", ".ntfs[DVDISO]", ".ntfs[BDISO]" };
	for(u8 n = 0; n < 4; n++)
		scan(WMTMP, false, cache_ext[n], SCAN_DELETE, NULL);

	sys_addr_t sysmem = NULL;

	// allocate memory
	if(mountCount <= 0) mount_all_ntfs_volumes(); //check_ntfs_volumes();
	if(mountCount <= 0) {mountCount = NTFS_UNMOUNTED; goto exit_prepntfs;}

	sysmem = sys_mem_allocate(_64KB_);
	if(!sysmem) goto exit_prepntfs;

	sysmem_p = sys_mem_allocate(_64KB_);
	if(!sysmem_p) goto exit_prepntfs;

	plugin_args    = (u8 *)(sysmem);
	sectionsP      = (u32*)(sysmem + sizeof(rawseciso_args));
	sections_sizeP = (u32*)(sysmem + sizeof(rawseciso_args) + _32KB_);

	// scan
	ntfs_count = 0;

	for(u8 i = 0; i < mountCount; i++)
	{
		device_id = USB_MASS_STORAGE((mounts[i].interface->ioType & 0x0F));

		for(u8 n = 0; n < 2; n++)
		{
			for(u8 profile = 0; profile < 5; profile++)
			{
				for(u8 m = id_PS3ISO; m <= id_PSXISO; m++)
				{
					if(m == id_PS2ISO) continue;

					ntfs_m = m;
					ntfs_subdir = NULL;

					snprintf(path, sizeof(path), "%s:%s%s%s", mounts[i].name, prefix[n], paths[m], SUFIX(profile));
					scan_path_ntfs(path, true);
				}
			}
		}
	}

exit_prepntfs:
	if(sysmem_p) sys_memory_free(sysmem_p); sysmem_p = NULL;
	if(sysmem) sys_memory_free(sysmem); sysmem = NULL;

	plugin_args = NULL;
	sectionsP = NULL;
	sections_sizeP = NULL;

	prepntfs_working = false;
	return ntfs_count;
}
#endif // #ifdef USE_NTFS
