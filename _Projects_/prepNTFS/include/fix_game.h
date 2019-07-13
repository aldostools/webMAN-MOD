#define READ_SFO_HEADER(ret) \
	if(!(mem[1] == 'P' && mem[2] == 'S' && mem[3] == 'F')) return ret; \
	u16 pos, str, dat, indx=0; \
	str=(mem[0x8] + (mem[0x9]<<8)); \
	dat=pos=(mem[0xc] + (mem[0xd]<<8));

#define FOR_EACH_SFO_FIELD() \
	while(str < 4090) \
	{ \
		if((mem[str] == 0) || (str>=dat)) break;

#define READ_NEXT_SFO_FIELD() \
		while(mem[str]) str++; str++; \
		pos += (mem[0x1c+indx] + (mem[0x1d+indx]<<8)); \
		indx += 0x10; \
	}

static void get_titleid(char *filename, char *titleID)
{
	char paramsfo[_4KB_];
	unsigned char *mem = (u8*)paramsfo;

	int fd = ps3ntfs_open(filename, O_RDONLY, 0766);
	if(fd >= 0)
	{
		uint64_t size = _4KB_;
		ps3ntfs_read(fd, (void *) paramsfo, size);
		ps3ntfs_close(fd);
	}
	else return;

	READ_SFO_HEADER();

	memset(titleID, 0, 16);

	FOR_EACH_SFO_FIELD()
	{
		if(!memcmp((char *) &mem[str], "TITLE_ID", 8))
		{
			strncpy(titleID, (char *) &mem[pos], 9); break;
		}

		READ_NEXT_SFO_FIELD()
	}
}

static bool need_fix(char *filename)
{
	if(c_firmware >= FW_VERSION) return false;

	char paramsfo[_4KB_];
	unsigned char *mem = (u8*)paramsfo;

	int fd = ps3ntfs_open(filename, O_RDONLY, 0766);
	if(fd >= 0)
	{
		uint64_t size = _4KB_;
		ps3ntfs_read(fd, (void *) paramsfo, size);
		ps3ntfs_close(fd);
	}
	else return true;

	READ_SFO_HEADER(true);

	FOR_EACH_SFO_FIELD()
	{
		if(!memcmp((char *) &mem[str], "PS3_SYSTEM_VER", 14))
		{
			char version[8];
			strncpy(version, (char *) &mem[pos], 7);
			int fw_ver=10000*((version[1] & 0xFF)-'0') + 1000*((version[3] & 0xFF)-'0') + 100*((version[4] & 0xFF)-'0');
			if((c_firmware >= 4.20f && c_firmware < FW_VERSION) && (fw_ver>(int)(c_firmware*10000.0f))) return true;
			break;
		}

		READ_NEXT_SFO_FIELD()
	}

	return false;
}

static bool fix_param_sfo(unsigned char *mem, char *titleID)
{
	if(c_firmware >= FW_VERSION) return false;

	READ_SFO_HEADER(false)

	memset(titleID, 0, 10);

	u8 fcount = 0;
	bool ret = false;

	FOR_EACH_SFO_FIELD()
	{
		if(!memcmp((char *) &mem[str], "TITLE_ID", 8))
		{
			strncpy(titleID, (char *) &mem[pos], 9);
			fcount++; if(fcount>=2) break;
		}
		else
		if(!memcmp((char *) &mem[str], "PS3_SYSTEM_VER", 14))
		{
			char version[8];
			strncpy(version, (char *) &mem[pos], 7);
			int fw_ver = 10000*((version[1] & 0xFF) - '0') + 1000*((version[3] & 0xFF) - '0') + 100*((version[4] & 0xFF) - '0');
			if((c_firmware >= 4.20f && c_firmware < FW_VERSION) && (fw_ver>(int)(c_firmware * 10000.0f)))
			{
				mem[pos + 1] = '4'; mem[pos + 3] = '2'; mem[pos + 4] = '0'; ret = true;
			}
			fcount++; if(fcount>=2) break;
		}

		READ_NEXT_SFO_FIELD()
	}

	return ret;
}

uint64_t getlba(char *s1, u16 n1, const char *s2, u16 n2, u16 start)
{
	u16 c=0; u32 lba = 0;

	for(u16 n = start + 0x1F; n < n1 - n2; n++)
	{
		c = 0; while((u8)s1[n+c] == (u8)s2[c] && c<n2) c++;
		if(c == n2)
		{
			while(n > 0x1D && (u8)s1[n--] != 0x01); n -= 0x1C;
			lba = ((u8)s1[n + 0]&0xFF) + ((u8)s1[n + 1]&0xFF) * 0x100UL + ((u8)s1[n + 2]&0xFF) * 0x10000UL + ((u8)s1[n + 3]&0xFF) * 0x1000000UL;
			start = n + 0x1C + n2; return lba;
		}
	}
	return 0;
}

void fix_iso(char *iso_file, uint64_t maxbytes)
{
	uint64_t size = get_filesize(iso_file); if(size < 0x800) return;

	int fd = ps3ntfs_open(iso_file, O_RDWR, 0766);
	if(fd >= 0)
	{
		uint64_t chunk_size = _4KB_; char chunk[chunk_size], ps3_sys_version[8], titleID[10];
		uint64_t lba = 0, pos = 0xA000ULL;

		bool fix_sfo = true, fix_eboot = true, fix_ver = false;

		if(maxbytes > 0 && size > maxbytes) size = maxbytes;
		if(size > pos) size -= pos; else size = 0;

		while(size > 0ULL)
		{
			if(fix_sfo)
			{
				if(ps3ntfs_seek64(fd, pos, SEEK_SET) != pos) break;
				if(ps3ntfs_read(fd, (void *) chunk, chunk_size) != chunk_size) break;

				lba = getlba(chunk, chunk_size, "PARAM.SFO;1", 11, 0);

				if(lba)
				{
					lba *= 0x800ULL; fix_sfo = false;

					if(ps3ntfs_seek64(fd, lba, SEEK_SET) != lba) break;
					if(ps3ntfs_read(fd, (void *) chunk, chunk_size) != chunk_size) break;

					fix_ver = fix_param_sfo((unsigned char *)chunk, titleID);

					if(fix_ver)
					{
						if(ps3ntfs_seek64(fd, lba, SEEK_SET) != lba) break;
						ps3ntfs_write(fd, (void *) chunk, (int) chunk_size);
					}
					else goto exit_fix; //do not fix if sfo version is ok

					if(size > lba) size = lba;

					lba = getlba(chunk, chunk_size, "PS3_DISC.SFB;1", 14, 0); lba *= 0x800ULL; chunk_size = 0x800; //1 sector
					if(lba > 0 && size > lba) size = lba;
				}
			}

			u16 start, offset;

			for(u8 t = (fix_eboot ? 0 : 1); t < 5; t++)
			{
				if(ps3ntfs_seek64(fd, pos, SEEK_SET) != pos) break;
				if(ps3ntfs_read(fd, (void *) chunk, chunk_size) != chunk_size) break;

				start = 0;

				while(true)
				{
					if(t == 0) lba = getlba(chunk, chunk_size, "EBOOT.BIN;1", 11, start);
					if(t == 1) lba = getlba(chunk, chunk_size, ".SELF;1", 7, start);
					if(t == 2) lba = getlba(chunk, chunk_size, ".self;1", 7, start);
					if(t == 3) lba = getlba(chunk, chunk_size, ".SPRX;1", 7, start);
					if(t == 4) lba = getlba(chunk, chunk_size, ".sprx;1", 7, start);

					if(lba)
					{
						if(t == 0) fix_eboot = false;

						lba *= 0x800ULL;

						if(ps3ntfs_seek64(fd, lba, SEEK_SET) != lba) break;
						if(ps3ntfs_read(fd, (void *) chunk, chunk_size) != chunk_size) break;

						offset = (chunk[0xC]<<24) + (chunk[0xD]<<16) + (chunk[0xE]<<8) + chunk[0xF]; offset -= 0x78;
						if(offset < 0x90 || offset > 0x800 || (chunk[offset] | chunk[offset+1] | chunk[offset+2] | chunk[offset+3] | chunk[offset+4] | chunk[offset+5])) offset=(t > 2) ? 0x258 : 0x428;

						if((t>2) && (offset == 0x258) && (chunk[offset] | chunk[offset+1] | chunk[offset+2] | chunk[offset+3] | chunk[offset+4] | chunk[offset+5])) offset = 0x278;

						for(u8 i = 0; i < 8; i++) ps3_sys_version[i] = chunk[offset + i];

						if((ps3_sys_version[0] + ps3_sys_version[1] + ps3_sys_version[2] + ps3_sys_version[3] + ps3_sys_version[4] + ps3_sys_version[5]) == 0 && (ps3_sys_version[6] & 0xFF)>0xA4)
						{
							ps3_sys_version[6] = 0XA4; ps3_sys_version[7] = 0X10;

							if(ps3ntfs_seek64(fd, (lba+offset), SEEK_SET) != (lba+offset)) break;
							ps3ntfs_write(fd, (void *) ps3_sys_version, (int) 8);
						}
						else goto exit_fix;

						if(t == 0) break;

					} else break;
				}
			}

			if(chunk_size <= 0) break;

			pos += chunk_size;
			size -= chunk_size;
			if(chunk_size > size) chunk_size = (int) size;
		}
exit_fix:
		ps3ntfs_close(fd);
	}
}
