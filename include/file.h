#define SC_FS_LINK						(810)

#define SC_STORAGE_OPEN 				(600)
#define SC_STORAGE_CLOSE 				(601)
#define SC_STORAGE_INSERT_EJECT			(616)

static bool copy_in_progress = false;

static u32 copied_count = 0;

static int isDir(const char* path);

static int savefile(char *file, char *mem, u64 size);
static int filecopy(const char *file1, char *file2, uint64_t maxbytes);
static int folder_copy(char *path1, char *path2);

#ifndef LITE_EDITION
static int del(const char *path, bool recursive);
#endif

static void enable_dev_blind(char *msg);

static void delete_history(bool delete_folders);

#ifdef COPY_PS3
static void  import_edats(char *path1, char *path2);
#endif

#define COPY_WHOLE_FILE		0
/*
static void add_log(const char *fmt, int value1, int value2)
{
	char buffer[2048];

	sprintf(buffer, fmt, value1, value2);

	//console_write(buffer);
	int fd;

	if(cellFsOpen("/dev_hdd0/webMAN.log", CELL_FS_O_RDWR|CELL_FS_O_CREAT|CELL_FS_O_APPEND, &fd, NULL, 0) == CELL_OK)
	{
		uint64_t nrw; int size = strlen(buffer);
		cellFsWrite(fd, buffer, size, &nrw);
		cellFsClose(fd);
	}
}
*/

static int sysLv2FsLink(const char *oldpath,const char *newpath)
{
	system_call_2(SC_FS_LINK, (u64)(u32)oldpath, (u64)(u32)newpath);
	return_to_user_prog(int);
}

static int isDir(const char* path)
{
	struct CellFsStat s;
	if(cellFsStat(path, &s) == CELL_FS_SUCCEEDED)
		return ((s.st_mode & CELL_FS_S_IFDIR) != 0);
	else
		return 0;
}

static bool file_exists(const char* path)
{
	struct CellFsStat s;
	return (cellFsStat(path, &s) == CELL_FS_SUCCEEDED);
}

static int savefile(char *file, char *mem, u64 size)
{
	u64 written; int fd=0; u32 flags = CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY;
	cellFsChmod(file, MODE);

	if(!extcmp(file, "/PARAM.SFO", 10)) flags = CELL_FS_O_CREAT | CELL_FS_O_WRONLY;

	if(cellFsOpen(file, flags, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		if(size) cellFsWrite(fd, (void *)mem, size, &written);
		cellFsClose(fd);
		cellFsChmod(file, MODE);
		return CELL_FS_SUCCEEDED;
	}

	return FAILED;
}
/*
static int appendfile(char *file, char *mem, u64 size)
{
	u64 written; int fd=0;
	if(cellFsOpen(file, CELL_FS_O_CREAT | CELL_FS_O_APPEND | CELL_FS_O_WRONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		if(size) cellFsWrite(fd, (void *)mem, size, &written);
		cellFsClose(fd);
		return CELL_FS_SUCCEEDED;
	}
	else
		return FAILED;
}

static int concat(char *file1, char *file2)
{
	struct CellFsStat buf;
	int fd1, fd2;
	int ret=FAILED;

	if(islike(file1, "/dvd_bdvd"))
		{system_call_1(36, (uint64_t) "/dev_bdvd");} // decrypt dev_bdvd files

	if(cellFsOpen((char*)file1, CELL_FS_O_RDONLY, &fd1, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		uint64_t size = buf.st_size;

		sys_addr_t buf1 = 0; uint64_t chunk_size = _64KB_;

		if(sys_memory_allocate(chunk_size, SYS_MEMORY_PAGE_SIZE_64K, &buf1)==0)
		{
			// append
			if(cellFsOpen(file2, CELL_FS_O_CREAT | CELL_FS_O_RDWR | CELL_FS_O_APPEND, &fd2, 0, 0) == CELL_FS_SUCCEEDED)
			{
				char *chunk=(char*)buf1;
				uint64_t msiz1 = 0, msiz2 = 0, pos=0;
				copy_aborted = false;

				while(size>0ULL)
        	    {
					if(copy_aborted) break;

					cellFsLseek(fd1, pos, CELL_FS_SEEK_SET, &msiz1);
					cellFsRead(fd1, chunk, chunk_size, &msiz1);

					cellFsWrite(fd2, chunk, msiz1, &msiz2);
					if(!msiz2) {break;}

					pos+=msiz2;
					size-=msiz2;
					if(chunk_size>size) chunk_size=(int) size;
				}
				cellFsClose(fd2);

				if(copy_aborted)
					cellFsUnlink(file2); //remove incomplete file
				else
					{cellFsChmod(file2, MODE); copied_count++;}

				ret=size;
			}
			sys_memory_free(buf1);
		}
		cellFsClose(fd1);
	}

	return ret;
}
*/
static int filecopy(const char *file1, char *file2, uint64_t maxbytes)
{
	struct CellFsStat buf;
	int fd1, fd2;
	int ret = FAILED;
	copy_aborted = false;

	if(strcmp(file1, file2) == 0) return FAILED;

#ifdef COPY_PS3
	sprintf(current_file, "%s", file2);
#endif

	if(cellFsStat(file1, &buf) != CELL_FS_SUCCEEDED)
	{
#ifndef LITE_EDITION
#ifdef COBRA_ONLY
		if(islike(file1, "/net"))
		{
			int ns = connect_to_remote_server((file1[4] & 0xFF) - '0');
			copy_net_file(file2, file1 + 5, ns, maxbytes);
			if(ns>=0) {shutdown(ns, SHUT_RDWR); socketclose(ns);}

			if(file_exists(file2)) return 0;
		}
#endif
#endif
		return FAILED;
	}

	if(islike(file1, "/dev_hdd0/") && islike(file2, "/dev_hdd0/"))
	{
		cellFsUnlink(file2); copied_count++;
		return sysLv2FsLink(file1, file2);
	}

	uint32_t blockSize;
	uint64_t freeSize;
	cellFsGetFreeSize((char*)"/dev_hdd0", &blockSize, &freeSize);

	if(buf.st_size > ((u64)blockSize*freeSize)) return FAILED;

	if(islike(file1, "/dvd_bdvd"))
		{system_call_1(36, (uint64_t) "/dev_bdvd");} // decrypt dev_bdvd files

	if(cellFsOpen((char*)file1, CELL_FS_O_RDONLY, &fd1, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		sys_addr_t buf1 = 0; uint64_t chunk_size = _64KB_;

		if(sys_memory_allocate(chunk_size, SYS_MEMORY_PAGE_SIZE_64K, &buf1)==0)
		{
			uint64_t size = buf.st_size, part_size = buf.st_size; u8 part = 0;
			if(maxbytes > 0 && size > maxbytes) size = maxbytes;

			if(islike(file2, "/dev_usb"))
			{
				if(!extcasecmp(file2, ".iso", 4)) strcat(file2, ".0"); else strcat(file2, ".66600");
				part++; part_size = 0xFFFF0000ULL; //4Gb - 64kb
			}

			uint64_t msiz1 = 0, msiz2 = 0, pos=0;
			char *chunk=(char*)buf1;
			u16 flen = strlen(file2);
next_part:
			// copy_file
			if(cellFsOpen(file2, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fd2, 0, 0) == CELL_FS_SUCCEEDED)
			{
				while(size > 0ULL)
        	    {
					if(copy_aborted) break;

					cellFsLseek(fd1, pos, CELL_FS_SEEK_SET, &msiz1);
					cellFsRead(fd1, chunk, chunk_size, &msiz1);
					if(!msiz1) break;

					cellFsWrite(fd2, chunk, msiz1, &msiz2);
					if(!msiz2) break;

					pos+=msiz2;
					size-=msiz2;
					if(chunk_size>size) chunk_size=(int) size;

					part_size-=msiz2;
					if(part_size == 0) break;

					sys_timer_usleep(1000);
				}
				cellFsClose(fd2);

				if(copy_aborted)
					cellFsUnlink(file2); //remove incomplete file
				else if((part>0) && (size > 0ULL))
				{
					if(part < 10)
						file2[flen-1] = '0' + part;
					else if(file2[flen-2] == '.')
					{
						file2[flen-1] = '0' + (u8)(part / 10);
						file2[flen  ] = '0' + (u8)(part % 10);
						file2[flen+1] = 0;
					}
					else
					{
						file2[flen-2] = '0' + (u8)(part / 10);
						file2[flen-1] = '0' + (u8)(part % 10);
					}
					part++; part_size = 0xFFFF0000ULL;
					goto next_part;
				}
				else
					{cellFsChmod(file2, MODE); copied_count++;}

				ret=size;
			}
			sys_memory_free(buf1);
		}
		cellFsClose(fd1);
	}

	return ret;
}

static int folder_copy(char *path1, char *path2)
{
	copy_aborted = false;

	cellFsChmod(path1, DMODE);
	cellFsMkdir((char*)path2, DMODE);

	int fd;

	if(cellFsOpendir(path1, &fd) == CELL_FS_SUCCEEDED)
	{
		CellFsDirent dir;
		char source[MAX_PATH_LEN];
		char target[MAX_PATH_LEN];
		u64 read;

		read = sizeof(CellFsDirent);
		while(!cellFsReaddir(fd, &dir, &read))
		{
			if(!read || copy_aborted) break;
			if(dir.d_name[0]=='.' && (dir.d_name[1]=='.' || dir.d_name[1]==0)) continue;

			sprintf(source, "%s/%s", path1, dir.d_name);
			sprintf(target, "%s/%s", path2, dir.d_name);

			if(isDir(source))
			{
				if(!strcmp(source, "/dev_bdvd/PS3_UPDATE")) continue;
				folder_copy(source, target);
			}
			else
				filecopy(source, target, COPY_WHOLE_FILE);
		}
		cellFsClosedir(fd);

		if(copy_aborted) return FAILED;
	}
	else
		return FAILED;

	return CELL_FS_SUCCEEDED;
}

#ifndef LITE_EDITION
static int del(const char *path, bool recursive)
{
	if(!isDir(path)) {return cellFsUnlink(path);}
	if(strlen(path) < 11 || islike(path, "/dev_bdvd") || islike(path, "/dev_flash") || islike(path, "/dev_blind")) return FAILED;

	int fd;
	u64 read;
	CellFsDirent dir;
	char entry[MAX_PATH_LEN];

	copy_aborted = false;

	if(cellFsOpendir(path, &fd) == CELL_FS_SUCCEEDED)
	{
		read = sizeof(CellFsDirent);
		while(!cellFsReaddir(fd, &dir, &read))
		{
			if(!read || copy_aborted) break;
			if(dir.d_name[0]=='.' && (dir.d_name[1]=='.' || dir.d_name[1]==0)) continue;

			sprintf(entry, "%s/%s", path, dir.d_name);

			if(isDir(entry))
				{if(recursive) del(entry, recursive);}
			else
				cellFsUnlink(entry);
		}
		cellFsClosedir(fd);

		if(copy_aborted) return FAILED;
	}
	else
		return FAILED;

	if(recursive) cellFsRmdir(path);

	return CELL_FS_SUCCEEDED;
}
#endif

static void waitfor(char *path, uint8_t timeout)
{
	struct CellFsStat s;
	for(uint8_t n=0; n < (timeout*2); n++)
	{
		if(path[0]!=NULL && cellFsStat(path, &s) == CELL_FS_SUCCEEDED) break;
		sys_timer_usleep(500000); if(!working) break;
	}
}

static void enable_dev_blind(char *msg)
{
	if(!isDir("/dev_blind"))
		{system_call_8(SC_FS_MOUNT, (u64)(char*)"CELL_FS_IOS:BUILTIN_FLSH1", (u64)(char*)"CELL_FS_FAT", (u64)(char*)"/dev_blind", 0, 0, 0, 0, 0);}

	if(!msg) return;

	show_msg((char*) msg);
	sys_timer_sleep(2);
}

static void delete_history(bool delete_folders)
{
	int fd; char path[128];

	if(cellFsOpendir("/dev_hdd0/home", &fd) == CELL_FS_SUCCEEDED)
	{
		CellFsDirent dir; u64 read = sizeof(CellFsDirent);

		while(!cellFsReaddir(fd, &dir, &read))
		{
			if(!read) break;
			sprintf(path, "%s/%s/etc/boot_history.dat", "/dev_hdd0/home", dir.d_name);
			cellFsUnlink(path);
			sprintf(path, "%s/%s/etc/community/CI.TMP", "/dev_hdd0/home", dir.d_name);
			cellFsUnlink(path);
			sprintf(path, "%s/%s/community/MI.TMP", "/dev_hdd0/home", dir.d_name);
			cellFsUnlink(path);
			sprintf(path, "%s/%s/community/PTL.TMP", "/dev_hdd0/home", dir.d_name);
			cellFsUnlink(path);
		}
		cellFsClosedir(fd);
	}

	cellFsUnlink("/dev_hdd0/vsh/pushlist/game.dat");
	cellFsUnlink("/dev_hdd0/vsh/pushlist/patch.dat");

	if(!delete_folders) return;

	for(u8 p=0; p<10; p++)
	{
		sprintf(path, "%s/%s", drives[0], paths[p]); cellFsRmdir(path);
		strcat(path, " [auto]"); cellFsRmdir(path);
	}
	cellFsRmdir("/dev_hdd0/PKG");
}

#ifdef COPY_PS3
static void import_edats(char *path1, char *path2)
{
	int fd; bool from_usb;
	u64 read;
	CellFsDirent dir;
	struct CellFsStat buf;

	char source[MAX_PATH_LEN];
	char target[MAX_PATH_LEN];

	cellFsMkdir((char*)path2, DMODE);
	if(cellFsStat(path2, &buf) != CELL_FS_SUCCEEDED) return;

	copy_aborted = false;
	from_usb = islike(path1, "/dev_usb");

	if(cellFsOpendir(path1, &fd) == CELL_FS_SUCCEEDED)
	{
		read = sizeof(CellFsDirent);
		while(!cellFsReaddir(fd, &dir, &read))
		{
			if(!read || copy_aborted) break;
			if(strstr(dir.d_name, ".edat")==NULL || !extcmp(dir.d_name, ".bak", 4)) continue;

			sprintf(source, "%s/%s", path1, dir.d_name);
			sprintf(target, "%s/%s", path2, dir.d_name);

			if(cellFsStat(target, &buf) != CELL_FS_SUCCEEDED)
				filecopy(source, target, COPY_WHOLE_FILE);
			if(from_usb && cellFsStat(target, &buf) == CELL_FS_SUCCEEDED)
                {sprintf(target, "%s.bak", source); cellFsRename(source, target);}
		}
		cellFsClosedir(fd);
	}
	else
		return;

	return;
}
#endif
