static bool g_free = false;
static u32 g_chunk_size = 0;
static sys_addr_t g_sysmem = NULL;

static u32 copied_count = 0;
static u16 copy_in_progress = 0;

static bool dont_copy_same_size = true; // skip copy the file if it already exists in the destination folder with the same file size
static bool allow_sc36 = true; // used to skip decrypt dev_bdvd files in file_copy function if it's called from folder_copy

#define COPY_WHOLE_FILE		0

//////////////////////////////////////////////////////////////

static bool is_ext(const char *path, const char *ext)
{
	return !extcasecmp(path, ext, 4);
}
#ifndef LITE_EDITION
static void check_path_tags(char *param)
{
	if(!param) return;

	char *param2 = strchr(param, '$'); if(!param2) return;

	char *tag = strstr(param2, "$USERID$");
	if(tag)
	{
		char uid[10];
		sprintf(uid, "%08i", xusers()->GetCurrentUserNumber());
		memcpy(tag, uid, 8);
	}

	tag = strstr(param2, "$RND$");
	bool fixed = !tag; if(fixed) tag = strstr(param2, "$000$");
	if(tag)
	{
		CellRtcTick nTick; cellRtcGetCurrentTick(&nTick);
		u8 n = 5, rnd = nTick.tick % 0x100;
		do
		{
			sprintf(tag, fixed ? "%03i%s" : "%i%s", rnd, tag + n);
			n = (fixed || rnd > 99) ? 3 : (rnd >  9) ? 2 : 1; rnd /= 2;
		}
		while(rnd && not_exists(param));
	}

	tag = strstr(param2, "$NEW$");
	if(tag)
	{
		for(u16 n = 5, cnt = 1; cnt < 1000; ++cnt)
		{
			sprintf(tag, "%03i%s", cnt, tag + n);
			if(not_exists(param)) break; n = 3;
		}
	}
}
#else
#define check_path_tags(a)
#endif

static void filepath_check(char *file)
{
	check_path_tags(file);

	if((file[5] == 'u' && islike(file, "/dev_usb"))
	#ifdef USE_NTFS
	|| (is_ntfs_path(file))
	#endif
	)
	{
		// remove invalid chars
		u16 n = 11;
		for(u16 c = 11; file[c]; c++)
		{
			if(file[c] == '\\') file[c] = '/';
			if(!strchr("\"<|>:*?", file[c])) file[n++] = file[c];
		}
		file[n] = 0;
	}
	#ifdef USE_NTFS
	if(is_ntfs_path(file)) {ntfs_path(file); if(mountCount <= NTFS_UNMOUNTED) check_ntfs_volumes();}
	#endif
}

#if defined(FIX_GAME) || defined(COPY_PS3)
static void get_copy_stats(char *msg, const char *label)
{
	u64 cur_size = file_size(current_file);
	//if(cur_size && current_size > 0x40000000)
	//	sprintf(msg, "%s %s\n(%'llu / %'llu %s, %i %s)", label, current_file, cur_size >> 30, current_size >> 30, STR_GIGABYTE, copied_count + 1, STR_FILES);
	//else
	if(cur_size && current_size)
		sprintf(msg, "%s %s\n(%'llu / %'llu %s, %i %s)", label, current_file, cur_size >> 20, current_size >> 20, STR_MEGABYTE, copied_count + 1, STR_FILES);
	else
		sprintf(msg, "%s %s (%i %s)", label, current_file, copied_count + 1, STR_FILES);
}
#endif

int64_t file_copy(const char *file1, char *file2);
int64_t file_copy(const char *file1, char *file2)
{
	struct CellFsStat buf;
	int fd1, fd2;
	int64_t ret = FAILED;
	copy_aborted = false;

	#ifdef USE_NTFS
	if(is_ntfs_path(file2)) {if(mountCount <= NTFS_UNMOUNTED) check_ntfs_volumes();}
	#endif

	filepath_check(file2);

	if(IS(file1, file2)) return FAILED;

	bool is_ntfs1 = false, is_ntfs2 = false;

	#ifdef COPY_PS3
	if(!ftp_state) strcpy(current_file, file2);
	#endif

	#ifdef USE_NTFS
	if((is_ntfs1 = is_ntfs_path(file1)))
	{
		struct stat bufn;
		if(ps3ntfs_stat(ntfs_path(file1), &bufn) >= 0) buf.st_size = bufn.st_size; else return FAILED;
	}
	else
	#endif
	if(cellFsStat(file1, &buf) != CELL_FS_SUCCEEDED)
	{
		#ifdef COBRA_NON_LITE
		if(islike(file1, "/net"))
		{
			++copy_in_progress, ++net_copy_in_progress;

			int ns = connect_to_remote_server((file1[4] & 0x0F));
			copy_net_file(file2, file1 + 5, ns);
			if(ns >= 0) sclose(&ns);

			--copy_in_progress, --net_copy_in_progress, copied_count++;

			if(file_exists(file2)) return 0;
		}
		#endif
		return FAILED;
	}

	current_size = buf.st_size;

	char *file1_666 = NULL;
	bool check_666 = false;

	show_progress(file1, OV_COPY);

	#ifdef UNLOCK_SAVEDATA
	if(webman_config->unlock_savedata && (current_size < _4KB_))
	{
		u16 size = (u16)current_size;
		unsigned char data[_4KB_]; *data = NULL;
		if(unlock_param_sfo(file1, data, size))
		{
			save_file(file2, (void*)data, size);
			return size;
		}
	}
	#endif

	if(islike(file2, INT_HDD_ROOT_PATH))
	{
		if(islike(file1, INT_HDD_ROOT_PATH))
		{
			cellFsUnlink(file2); copied_count++;
			return sysLv2FsLink(file1, file2);
		}
		else
			check_666 = islike(file1, "/dev_usb");

		if(check_666)
		{
			file1_666 = (char*)file1 + strlen(file1) - 6;
			check_666 = islike(file1_666, ".666");
			if(check_666)
			{
				if(!islike(file1_666, ".66600")) return 0; // ignore .666xx
				u16 flen2 = strlen(file2) - 6;
				if(islike(file2 + flen2, ".66600")) file2[flen2] = '\0'; // remove .66600
			}
		}

		if(current_size > get_free_space(drives[0])) return FAILED;
	}

	if(allow_sc36 && islike(file1, "/dev_bdvd"))
		sysLv2FsBdDecrypt(); // decrypt dev_bdvd files

	#ifdef USE_NTFS
	if(is_ntfs1)
	{
		fd1 = ps3ntfs_open(ntfs_path(file1), O_RDONLY, 0);
		if(fd1 < 0) is_ntfs1 = false;
	}
	else
	#endif
	// skip if file already exists with same size
	if(dont_copy_same_size)
	{
		if(file_ssize(file2) == (s64)current_size)
		{
			copied_count++;
			return current_size;
		}
	}

	u64 pos;
	u8 merge_part = 0;
	u32 chunk_size; sys_addr_t sysmem = NULL;

	if(g_free) {chunk_size = g_chunk_size, sysmem = g_sysmem, g_free = false;} else
	{
		chunk_size = is_ntfs1 || (current_size <= _64KB_) ? _64KB_ : _128KB_;
		sysmem = sys_mem_allocate(chunk_size);
		if(!sysmem) {chunk_size = _64KB_; sys_mem_allocate(chunk_size);}
	}

	if(sysmem)
	{
		++copy_in_progress;
merge_next:
		if(is_ntfs1 || (cellFsOpen(file1, CELL_FS_O_RDONLY, &fd1, NULL, 0) == CELL_FS_SUCCEEDED))
		{
			u64 size = current_size, part_size = current_size; u8 part = 0;

			if((part_size > 0xFFFFFFFFULL) && islike(file2, "/dev_usb"))
			{
				if(is_ext(file2, ".iso")) strcat(file2, ".0"); else strcat(file2, ".66600");
				part++; part_size = 0xFFFF0000ULL; //4Gb - 64kb
			}

			u64 read = 0, written = 0;
			char *chunk = (char*)sysmem;
			u16 flen = strlen(file2);
next_part:
			#ifdef USE_NTFS
			if((is_ntfs2 = is_ntfs_path(file2)))
			{
				fd2 = ps3ntfs_open(ntfs_path(file2), O_CREAT | O_WRONLY | O_TRUNC, MODE);
				if(fd2 < 0) is_ntfs2 = false;
			}
			#endif
			// copy_file
			if(is_ntfs2 || merge_part || (cellFsOpen(file2, CELL_FS_O_CREAT | CELL_FS_O_WRONLY | CELL_FS_O_TRUNC , &fd2, 0, 0) == CELL_FS_SUCCEEDED))
			{
				pos = 0;
				while(size > 0)
				{
					if(copy_aborted) break;

					#ifdef USE_NTFS
					if(is_ntfs1)
					{
						ps3ntfs_seek64(fd1, pos, SEEK_SET);
						read = ps3ntfs_read(fd1, (void *)chunk, chunk_size);
					}
					else
					#endif
					{
						cellFsReadWithOffset(fd1, pos, chunk, chunk_size, &read);
					}

					if(!read) break;

					#ifdef USE_NTFS
					if(is_ntfs2)
					{
						written = ps3ntfs_write(fd2, chunk, read);
					}
					else
					#endif
					cellFsWrite(fd2, chunk, read, &written);

					if(!written) break;

					pos  += written;
					size -= written;

					if(chunk_size > size) chunk_size = (int) size;

					part_size -= written;
					if(part_size == 0) break;

					sys_ppu_thread_usleep(1000);
				}

				copied_size += pos;

				if(check_666 && !copy_aborted && (merge_part < 99))
				{
					sprintf(file1_666, ".666%02i", ++merge_part);

					current_size = file_size(file1);
					if(current_size)
					{
						cellFsClose(fd1);
						goto merge_next;
					}
				}

				#ifdef USE_NTFS
				if(is_ntfs2) ps3ntfs_close(fd2);
				else
				#endif
				cellFsClose(fd2);

				if(copy_aborted)
				{
					#ifdef USE_NTFS
					if(is_ntfs2) ps3ntfs_unlink(ntfs_path(file2));
					else
					#endif
					cellFsUnlink(file2); //remove incomplete file
				}
				else if((part > 0) && (size > 0))
				{
					if(file2[flen - 2] == '.')
						sprintf(file2 + flen - 2, ".%i", part);
					else
						sprintf(file2 + flen - 2, "%02i", part);

					part++; part_size = 0xFFFF0000ULL;
					goto next_part;
				}
				else
					{cellFsChmod(file2, MODE); copied_count++;}

				ret = size;
			}

			#ifdef USE_NTFS
			if(is_ntfs1) ps3ntfs_close(fd1);
			else
			#endif
			cellFsClose(fd1);
		}
		--copy_in_progress;
	}

	if(g_sysmem && !g_free)
		g_free = true;
	else if(sysmem)
		sys_memory_free(sysmem);

	return ret;
}

static void force_copy(const char *file1, char *file2)
{
	if(!file1 || !file2 || not_exists(file1)) return;
	dont_copy_same_size = false; // force copy file with the same size than existing file
	file_copy(file1, file2);
	dont_copy_same_size = true;  // restore default mode (assume file is already copied if existing file has same size)
	disable_progress();
}

#ifdef COPY_PS3
static void mkdir_tree(const char *full_path);
static void normalize_path(char *path, bool slash);

static int recursive_folder_copy(const char *path1, char *path2)
{
	filepath_check(path2);

	int fd; bool is_ntfs = false;

	copy_aborted = false;

	#ifdef USE_NTFS
	struct stat bufn;
	DIR_ITER *pdir = NULL;

	if(is_ntfs_path(path1))
	{
		pdir = ps3ntfs_opendir(ntfs_path(path1));
		if(pdir) is_ntfs = true;
	}
	else
	#endif
	{
		cellFsChmod(path1, DMODE);
	}

	bool is_root = IS(path1, "/");

	if(is_ntfs || cellFsOpendir(path1, &fd) == CELL_FS_SUCCEEDED)
	{
		if(islike(path1, "/dev_bdvd"))
			{allow_sc36 = false; sysLv2FsBdDecrypt();} // decrypt dev_bdvd files

		normalize_path(path2, true);
		mkdir_tree(path2);

		CellFsDirent dir; u64 read_e;
		CellFsDirectoryEntry entry; u32 read_f;
		char *entry_name = (is_root) ? dir.d_name : entry.entry_name.d_name;

		char source[STD_PATH_LEN];
		char target[STD_PATH_LEN];

		u16 plen1 = strcopy(source, path1);
		u16 plen2 = strcopy(target, path2);

		while(working)
		{
			#ifdef USE_NTFS
			if(is_ntfs)
			{
				if(ps3ntfs_dirnext(pdir, entry_name, &bufn)) break;
				if(entry_name[0]=='$' && path1[12] == 0) continue;
			}
			else
			#endif
			if(is_root && ((cellFsReaddir(fd, &dir, &read_e) != CELL_FS_SUCCEEDED) || (read_e == 0))) break;
			else
			if(cellFsGetDirectoryEntries(fd, &entry, sizeof(entry), &read_f) || !read_f) break;

			if(copy_aborted) break;
			if(entry_name[0] == '.' && (entry_name[1] == '.' || entry_name[1] == '\0')) continue;

			sprintf(source + plen1, "/%s", entry_name);
			sprintf(target + plen2, "/%s", entry_name);

			if(isDir(source))
			{
				if(IS(source, "/dev_bdvd/PS3_UPDATE")) {cellFsMkdir(target, DMODE); continue;} // just create /PS3_UPDATE without its content
				recursive_folder_copy(source, target);
			}
			else
				file_copy(source, target);
		}

		#ifdef USE_NTFS
		if(is_ntfs) ps3ntfs_dirclose(pdir);
		else
		#endif
		cellFsClosedir(fd); allow_sc36 = true;

		if(copy_aborted) return FAILED;
	}
	return CELL_FS_SUCCEEDED;
}

static int folder_copy(const char *path1, char *path2)
{
	if(!g_sysmem) {g_chunk_size = _256KB_; g_sysmem = sys_mem_allocate(g_chunk_size);}
	if(!g_sysmem) {g_chunk_size = _128KB_; g_sysmem = sys_mem_allocate(g_chunk_size);}
	if(!g_sysmem) {g_chunk_size =  _64KB_; g_sysmem = sys_mem_allocate(g_chunk_size);}
	if( g_sysmem) g_free = true;

	if(!copy_in_progress) copied_size = copied_count = 0;

	++copy_in_progress;

	int ret = recursive_folder_copy(path1, path2);

	--copy_in_progress;

	if(g_sysmem && g_free) {sys_memory_free(g_sysmem); g_sysmem = NULL, g_free = false;}
	return ret;
}

#ifdef WM_REQUEST
static void force_folder_copy(char *file1, char *file2)
{
	dont_copy_same_size = false; // force copy file with the same size than existing file
	folder_copy(file1, file2);
	dont_copy_same_size = true;  // restore default mode (assume file is already copied if existing file has same size)
}
#endif

#endif // #ifdef COPY_PS3

//////////////////////////////////////////////////////////////

/*
static int file_concat(const char *file1, char *file2)
{
	struct CellFsStat buf;
	int fd1, fd2;
	int ret = FAILED;

	filepath_check(file2);

	if(islike(file1, "/dvd_bdvd"))
		sysLv2FsBdDecrypt(); // decrypt dev_bdvd files

	if(cellFsStat(file1, &buf) != CELL_FS_SUCCEEDED) return ret;

	if(cellFsOpen(file1, CELL_FS_O_RDONLY, &fd1, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		u64 size = buf.st_size;

		u64 chunk_size = _64KB_; sys_addr_t sysmem = sys_mem_allocate(_64KB_);

		if(sysmem)
		{
			// append
			if(cellFsOpen(file2, CELL_FS_O_RDWR | CELL_FS_O_CREAT | CELL_FS_O_APPEND, &fd2, 0, 0) == CELL_FS_SUCCEEDED)
			{
				char *chunk = (char*)sysmem;
				u64 read = 0, written = 0, pos=0;
				copy_aborted = false;

				while(size > 0)
				{
					if(copy_aborted) break;

					cellFsLseek(fd1, pos, CELL_FS_SEEK_SET, &read);
					cellFsRead(fd1, chunk, chunk_size, &read);

					cellFsWrite(fd2, chunk, read, &written);
					if(!written) {break;}

					pos+=written;
					size-=written;
					if(chunk_size>size) chunk_size=(int) size;
				}
				cellFsClose(fd2);

				if(copy_aborted)
					cellFsUnlink(file2); //remove incomplete file
				else
					{cellFsChmod(file2, MODE); copied_count++;}

				ret=size;
			}
			sys_memory_free(sysmem);
		}
		cellFsClose(fd1);
	}

	return ret;
}
*/

#ifdef BACKUP_ACT_DAT
static void backup_act_dat(void)
{
	if(!payload_ps3hen) return;

	int fd;

	// backup / restore act.bak -> act.dat
	if(cellFsOpendir(HDD0_HOME_DIR, &fd) == CELL_FS_SUCCEEDED)
	{
		char path1[48], path2[48];
		CellFsDirectoryEntry dir; u32 read_e, count = 0;
		char *entry_name = dir.entry_name.d_name;

		while(working && (!cellFsGetDirectoryEntries(fd, &dir, sizeof(dir), &read_e) && read_e))
		{
			if(++count > 2000) break; // fail safe break loop

			sprintf(path1, "%s/%.8s/exdata/act.bak", HDD0_HOME_DIR, entry_name);
			sprintf(path2, "%s/%.8s/exdata/act.dat", HDD0_HOME_DIR, entry_name);

			if(file_exists(path1))
			{
				if(not_exists(path2)) sysLv2FsLink(path1, path2); // restore .bak -> .dat
			}
			else if(file_exists(path2))
			{
				sysLv2FsLink(path2, path1); // backup .dat -> .bak (if .bak doesn't exist)
			}
		}
		cellFsClosedir(fd);
	}
}
#else
#define backup_act_dat()
#endif

#ifdef MOUNT_ROMS
static char *get_filename(const char *path);
static int patch_file(const char *file, const char *data, u64 offset, int size);

static void copy_rom_media(const char *src_path)
{
	char dst_path[64];
	const char *PS3_GAME[2] = { "/PS3_GAME/", "/" };
	if(file_exists(WM_ICONS_PATH "/ICON0.PNG"))
	{
		for(u8 p = 0; p < 2; p++)
		{
			// copy rom icon to ICON0.PNG
			concat3(dst_path, PKGLAUNCH_DIR, PS3_GAME[p], "ICON0.PNG");
			cellFsUnlink(dst_path); sysLv2FsLink(WM_ICONS_PATH "/ICON0.PNG", dst_path);
		}
	}

	// get rom name & file extension
	char *name = get_filename(src_path);
	if(!name) return;

	// get rom alias. use file name as default title
	char path[MAX_LINE_LEN]; strcpy(path, src_path);
	char *title = get_filename(path); *title = 0; char *ex = strrchr(++title, '.');
	#ifndef ENGLISH_ONLY
	rom_alias(title, title, path); close_language(); 
	#endif
	if(ex && *ex == '.') *ex = 0;

	// patch title name in PARAM.SFO of PKGLAUNCH
	concat2(dst_path, PKGLAUNCH_DIR, "/PS3_GAME/PARAM.SFO");
	patch_file(dst_path, title, 0x378, 0x80);

	char *ext  = strrchr(++name, '.');
	if(ext)
	{
		for(u8 p = 0; p < 2; p++)
		{
			// copy rom icon to ICON0.PNG
			concat3(dst_path, PKGLAUNCH_DIR, PS3_GAME[p], "ICON0.PNG");
			{strcpy(ext, ".png"); if(file_exists(src_path)) {file_copy(src_path, dst_path);} else
			{strcpy(ext, ".PNG"); if(file_exists(src_path)) {file_copy(src_path, dst_path);} else
			{
				concat_path(path, WMTMP, name);
				char *ext2 = strrchr(path, '.');
				{strcpy(ext2, ".png"); if(file_exists(path)) {file_copy(path, dst_path);} else
				{strcpy(ext2, ".PNG"); if(file_exists(path)) {file_copy(path, dst_path);} else
															 {file_copy(PKGLAUNCH_ICON, dst_path);}}}
			}}}

			strcpy(path, src_path); char *path_ = get_filename(path) + 1;

			const char *media[6] = {"ICON0.PNG", "PIC0.PNG", "PIC1.PNG", "PIC2.PNG", "SND0.AT3", "ICON1.PAM"};
			for(u8 i = 0; i < 6; i++)
			{
				concat3(dst_path, PKGLAUNCH_DIR, PS3_GAME[p], media[i]); cellFsUnlink(dst_path);
				strcpy(ext + 1, media[i]);
				if(file_exists(src_path))
					file_copy(src_path, dst_path);
				else
				{
					strcpy(path_, media[i]);
					if(file_exists(path))
						file_copy(path, dst_path);
				}
			}
		}
		*ext = NULL;
	}
}
#endif // #ifdef MOUNT_ROMS
