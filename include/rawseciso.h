#define CD_CACHE_SIZE           (48) // sectors 48*2448 = 115KB (up to 52 sectors fit in 128KB)

#define CD_SECTOR_SIZE_2048     2048

#define LAST_SECTOR             1
#define READ_SECTOR             0xFFFFFFFF

#ifdef RAWISO_PSX_MULTI
#define EMU_PSX_MULTI (EMU_PSX + 16)
#endif

enum STORAGE_COMMAND
{
	CMD_READ_ISO,
	CMD_READ_DISC,
	CMD_READ_CD_ISO_2352,
	CMD_FAKE_STORAGE_EVENT,
	CMD_GET_PSX_VIDEO_MODE
};

typedef struct
{
	u64 device;
	u32 emu_mode;
	u32 num_sections;
	u32 num_tracks;
	// sections after
	// sizes after
	// tracks after
} __attribute__((packed)) rawseciso_args; // 20 bytes

/*  new for PSX with multiple disc support (derivated from Estwald's PSX payload):

	in 0x80000000000000D0, u64 (8 bytes) countaining ISOs sector size (for alls)

	tracks index is from peekq(0x8000000000000050)

	max 8 disc, 4*u32 (16 bytes) containing

	0 -> u32 lv2 address for next entry (if 0 indicates no track info: you must generate it!)
	1 -> u32 to 0 (reserved)
	2 -> u32 lv2 address for track table info
	3 -> u32 with size of track table info

	table info: (from 0x80000000007E0000)
	u8 datas

	0, 1 -> contains an u16 with size - 2 of track info block
	2 -> to 1
	3 -> to 1
	// first track
	4 -> 0
	5 -> mode 0x14 for data, 0x10 for audio
	6 -> track number (from 1)
	7 -> 0
	8, 9, 10, 11 -> contain an u32 with the LBA

	....
	....

	// last track datas (skiped in rawseciso)

	0,
	0x14,
	0xAA,
	0,

	Ejection / insertion test (to change the disc):

	if psxseciso_args->device == 0
	test if virtual directory "/psx_cdrom0" is present

	virtual disc file names (use this name when psxseciso_args->discs_desc[x][0]==0):

	"/psx_d0"
	"/psx_d1"
	"/psx_d2"
	"/psx_d3"
	"/psx_d4"
	"/psx_d5"
	"/psx_d6"
	"/psx_d7"

	when psxseciso_args->discs_desc[x][0]!=0 it describe a list of sectors/numbers sectors arrays

	 discs_desc[x][0] = (parts << 16) | offset: parts number of entries, offset: in u32 units from the end of this packet
	 discs_desc[x][1] = toc_filesize (in LBA!. Used for files/sectors arrays isos to know the toc)

	sector array sample:

	u32 *datas = &psxseciso_args->discs_desc[8][0]; // datas starts here

	u32 *sectors_array1 = datas + offset;
	u32 *nsectors_array1 = datas + offset + parts;

	offset += parts * 2; // to the next disc sector array datas...

*/

static u32 CD_SECTOR_SIZE_2352 = 2352;

#ifdef USE_INTERNAL_PLUGIN

#ifdef RAWISO_PSX_MULTI
typedef struct
{
	u64 device;
	u32 emu_mode;         //   16 bits    16 bits    32 bits
	u32 discs_desc[8][2]; //    parts  |   offset | toc_filesize   -> if parts == 0  use files. Offset is in u32 units from the end of this packet. For 8 discs
	// sector array 1
	// numbers sector array 1
	//...
	// sector array 8
	// numbers sector array 8
} __attribute__((packed)) psxseciso_args;

static sys_ppu_thread_t thread_id_eject = SYS_PPU_THREAD_NONE;

static int discfd = NONE;
#endif

volatile u8 eject_running = 0;

u32 real_disctype;
ScsiTrackDescriptor tracks[64];
int emu_mode, num_tracks;
sys_event_port_t result_port;

static u8 rawseciso_loaded = 0;

static volatile u8 do_run = 0;

static int mode_file = 0, cd_sector_size_param = 0;

static u64 sec_size = 512ULL;

static sys_device_info_t disc_info;

static u64 usb_device = 0ULL;

static u8 ntfs_running = 0;

static sys_device_handle_t handle = SYS_DEVICE_HANDLE_NONE;
static sys_event_queue_t command_queue_ntfs = SYS_EVENT_QUEUE_NONE;

static u32 *sections, *sections_size;
static u32 num_sections;

static u64 discsize = 0;
static int is_cd2352 = 0;
static u8 *cd_cache = 0;

static int sys_storage_ext_mount_discfile_proxy(sys_event_port_t result_port, sys_event_queue_t command_queue_ntfs, int emu_type, u64 disc_size_bytes, u32 read_size, unsigned int trackscount, ScsiTrackDescriptor *tracks)
{
	system_call_8(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_MOUNT_DISCFILE_PROXY, result_port, command_queue_ntfs, emu_type, disc_size_bytes, read_size, trackscount, (u64)(u32)tracks);
	return (int)p1;
}

static void get_next_read(u64 discoffset, u64 bufsize, u64 *offset, u64 *readsize, int *idx, u64 sec_size)
{
	u64 last, sz, base = 0;
	*idx = NONE;
	*readsize = bufsize;
	*offset = 0;

	for(u32 i = 0; i < num_sections; i++)
	{
		if(i == 0 && mode_file > 1)
			sz = (((u64)sections_size[i]) * CD_SECTOR_SIZE_2048);
		else
			sz = (((u64)sections_size[i]) * sec_size);

		last = base + sz;

		if(discoffset >= base && discoffset < last)
		{
			u64 maxfileread = last-discoffset;

			if(bufsize > maxfileread)
				*readsize = maxfileread;
			else
				*readsize = bufsize;

			*idx = i;
			*offset = discoffset-base;
			return;
		}

		base += sz;
	}

	// We can be here on video blu-ray
	//DPRINTF("Offset or size out of range  %lx%08lx   %lx!!!!!!!!\n", discoffset>>32, discoffset, bufsize);
}

static u8 last_sect_buf[_4KB_] __attribute__((aligned(16)));
static u32 last_sect = READ_SECTOR;

static int process_read_iso_cmd_iso(u8 *buf, u64 offset, u64 size)
{
	u64 remaining;
	int retry;

	//DPRINTF("read iso: %p %lx %lx\n", buf, offset, size);
	remaining = size;

	while(remaining)
	{
		u64 pos, readsize;
		int idx;
		int ret;
		u32 sector;
		u32 r;

		if(!ntfs_running) return FAILED;

		get_next_read(offset, remaining, &pos, &readsize, &idx, sec_size);

		if(idx == NONE || sections[idx] == 3)
		{
			memset(buf, 0, readsize);
			buf += readsize;
			offset += readsize;
			remaining -= readsize;
			continue;
		}

		if(pos % sec_size)
		{
			u64 csize;

			sector = sections[idx] + (pos / sec_size);
			for(retry = 0; retry < 16; retry++)
			{
				r = 0;
				if(last_sect == sector)
				{
					ret = CELL_OK; r = LAST_SECTOR;
				}
				else
					ret = sys_storage_read(handle, 0, sector, 1, last_sect_buf, &r, 0);

				if(ret == CELL_OK && r == LAST_SECTOR)
					last_sect = sector;
				else
					last_sect = READ_SECTOR;

				if(last_sect == READ_SECTOR)
				{
#ifdef RAWISO_PSX_MULTI
					if(emu_mode == EMU_PSX_MULTI) return (int) 0x8001000A; // EBUSY
#endif
					if(ret == (int) 0x80010002 || ret == (int) 0x8001002D)
					{
						if(handle != SYS_DEVICE_HANDLE_NONE) sys_storage_close(handle); handle = SYS_DEVICE_HANDLE_NONE;

						while(ntfs_running)
						{
							if(sys_storage_get_device_info(usb_device, &disc_info) == 0)
							{
								ret = sys_storage_open(usb_device, 0, &handle, 0);
								if(ret == CELL_OK) break;

								handle = SYS_DEVICE_HANDLE_NONE; sys_ppu_thread_usleep(500000);
							}
							else sys_ppu_thread_usleep(7000000);
						}
						retry = -1; continue;
					}


					if(retry == 15 || !ntfs_running)
					{
						//DPRINTF("sys_storage_read failed: %x 1 -> %x\n", sector, ret);
						return FAILED;
					}
					else sys_ppu_thread_usleep(100000);
				}
				else break;
			}

			csize = sec_size - (pos % sec_size);

			if(csize > readsize) csize = readsize;

			memcpy(buf, last_sect_buf + (pos % sec_size), csize);
			buf += csize;
			offset += csize;
			pos += csize;
			remaining -= csize;
			readsize -= csize;
		}

		if(readsize)
		{
			u32 n = readsize / sec_size;

			if(n)
			{
				sector = sections[idx] + (pos / sec_size);
				for(retry = 0; retry < 16; retry++)
				{
					r = 0;
					ret = sys_storage_read(handle, 0, sector, n, buf, &r, 0);

					if(ret == CELL_OK && r == n)
						last_sect = sector + n - 1;
					else
						last_sect = READ_SECTOR;

					if(last_sect == READ_SECTOR)
					{
#ifdef RAWISO_PSX_MULTI
						if(emu_mode == EMU_PSX_MULTI) return (int) 0x8001000A; // EBUSY
#endif
						if(ret == (int) 0x80010002 || ret == (int) 0x8001002D)
						{
							if(handle != SYS_DEVICE_HANDLE_NONE) sys_storage_close(handle); handle = SYS_DEVICE_HANDLE_NONE;

							while(ntfs_running)
							{
								if(sys_storage_get_device_info(usb_device, &disc_info) == 0)
								{
									ret = sys_storage_open(usb_device, 0, &handle, 0);
									if(ret == CELL_OK) break;

									handle = SYS_DEVICE_HANDLE_NONE; sys_ppu_thread_usleep(500000);
								}
								else sys_ppu_thread_usleep(7000000);
							}
							retry = -1; continue;
						}

						if(retry == 15 || !ntfs_running)
						{
							//DPRINTF("sys_storage_read failed: %x %x -> %x\n", sector, n, ret);
							return FAILED;
						}
						else sys_ppu_thread_usleep(100000);
					}
					else break;
				}

				u64 s;

				s = n * sec_size;
				buf += s;
				offset += s;
				pos += s;
				remaining -= s;
				readsize -= s;
			}

			if(readsize)
			{
				sector = sections[idx] + pos / sec_size;
				for(retry = 0; retry < 16; retry++)
				{
					r = 0;
					if(last_sect == sector)
					{
						ret = CELL_OK; r = LAST_SECTOR;
					}
					else
						ret = sys_storage_read(handle, 0, sector, 1, last_sect_buf, &r, 0);

					if(ret == CELL_OK && r == LAST_SECTOR)
						last_sect = sector;
					else
						last_sect = READ_SECTOR;

					if(last_sect == READ_SECTOR)
					{
#ifdef RAWISO_PSX_MULTI
						if(emu_mode == EMU_PSX_MULTI) return (int) 0x8001000A; // EBUSY
#endif
						if(ret == (int) 0x80010002 || ret == (int) 0x8001002D)
						{
							if(handle != SYS_DEVICE_HANDLE_NONE) sys_storage_close(handle); handle = SYS_DEVICE_HANDLE_NONE;

							while(ntfs_running)
							{
								if(sys_storage_get_device_info(usb_device, &disc_info) == 0)
								{
									ret = sys_storage_open(usb_device, 0, &handle, 0);
									if(ret == CELL_OK) break;

									handle = SYS_DEVICE_HANDLE_NONE; sys_ppu_thread_usleep(500000);
								}
								else sys_ppu_thread_usleep(7000000);
							}
							retry = -1; continue;
						}

						if(retry == 15 || !ntfs_running)
						{
							//DPRINTF("sys_storage_read failed: %x 1 -> %x\n", sector, ret);
							return FAILED;
						}
						else sys_ppu_thread_usleep(100000);
					}
					else break;
				}

				memcpy(buf, last_sect_buf, readsize);
				buf += readsize;
				offset += readsize;
				remaining -= readsize;
			}
		}
	}

	return CELL_OK;
}

static int last_index = NONE;

static int process_read_file_cmd_iso(u8 *buf, u64 offset, u64 size)
{
	u64 remaining;

	char *path_name = (char *) sections;

	if(mode_file > 1)
	{
		last_index = NONE;
		path_name-= 0x200;
	}

	remaining = size;

	while(remaining)
	{
		u64 pos, readsize;
		int idx;

		if(!ntfs_running) return FAILED;

		get_next_read(offset, remaining, &pos, &readsize, &idx, CD_SECTOR_SIZE_2048);

#ifdef RAWISO_PSX_MULTI
		if(idx == NONE || path_name[0x200 * idx] == 0)
		{
#endif
			memset(buf, 0, readsize);
			buf += readsize;
			offset += readsize;
			remaining -= readsize;
#ifdef RAWISO_PSX_MULTI
			continue;
		}
		else
		{
			int ret;
			if(idx != last_index)
			{
				if(discfd != NONE) cellFsClose(discfd);

				while(ntfs_running)
				{
					ret = cellFsOpen(&path_name[0x200 * idx], CELL_FS_O_RDONLY, &discfd, NULL, 0);
					if(ret == CELL_OK) break;

					discfd = NONE;

					sys_ppu_thread_usleep(5000000);
				}

				last_index = idx;
			}

			u64 p = 0;
			ret = cellFsLseek(discfd, pos, SEEK_SET, &p);
			if(!ret) ret = cellFsRead(discfd, buf, readsize, &p);
			if(ret != CELL_OK)
			{
				last_index = NONE;

				continue;
			}
			else if(readsize != p)
			{
				if((offset + readsize) < discsize) return FAILED;
			}
			buf += readsize;
			offset += readsize;
			remaining -= readsize;
		}
#endif
	}

	return CELL_OK;
}

#ifdef RAWISO_PSX_MULTI
int psx_indx = 0;

u32 *psx_isos_desc;

char psx_filename[8][8]= {
	"/psx_d0",
	"/psx_d1",
	"/psx_d2",
	"/psx_d3",
	"/psx_d4",
	"/psx_d5",
	"/psx_d6",
	"/psx_d7"
};

static int process_read_psx_cmd_iso(u8 *buf, u64 offset, u64 size, u32 ssector)
{
	u64 remaining, sect_size;
	int ret, rel;

	u32 lba = offset + 150, lba2;

	sect_size = peekq(0x80000000000000D0ULL);
	offset *= sect_size;
	//size   *= sector;

	remaining = size;

	if(!psx_isos_desc[psx_indx * 2 + 0])
	{
		if(discfd == NONE)
		{
			while(ntfs_running)
			{
				ret = cellFsOpen(&psx_filename[psx_indx][0], CELL_FS_O_RDONLY, &discfd, NULL, 0);
				if(ret == CELL_OK) break;

				discfd = NONE;
				sys_ppu_thread_usleep(5000000);
			}
		}
	}

	while(remaining)
	{
		u64 pos, p, readsize = (u64) CD_SECTOR_SIZE_2352;

		p = 0;
		rel = 0;

		if(!ntfs_running) return FAILED;

		pos = offset;
		if(ssector == CD_SECTOR_SIZE_2048) { if(sect_size >= CD_SECTOR_SIZE_2352) pos += 24ULL; readsize = CD_SECTOR_SIZE_2048;}
		else
		{
			if(sect_size == CD_SECTOR_SIZE_2048)
			{
				rel = 24; readsize = CD_SECTOR_SIZE_2048;
				memset(&buf[0], 0x0, 24);
				memset(&buf[1], 0xFF, 10);
				buf[0x12] = buf[0x16]= 8;
				buf[0xf] = 2;

				buf[0xe] = lba % 75;

				lba2 = lba / 75;
				buf[0xd] = lba2 % 60;
				lba2 /= 60;
				buf[0xc] = lba2;
			}
		}

		if(!psx_isos_desc[psx_indx * 2 + 0])
		{
			ret = cellFsLseek(discfd, pos, SEEK_SET, &p);
			if(!ret) ret = cellFsRead(discfd, buf + rel, readsize, &p);
			if(ret != CELL_OK)
			{
				if(ret == (int) 0x8001002B) return (int) 0x8001000A; // EBUSY

				return CELL_OK;
			}
			else if(readsize != p)
			{
				if((offset + readsize) < discsize) return CELL_OK;
			}
		}
		else
		{
			ret = process_read_iso_cmd_iso(buf + rel, pos, readsize);
			if(ret) return ret;
		}


		buf += ssector;
		offset += sect_size;
		remaining --;
		lba++;
	}

	return CELL_OK;
}
#endif

static inline void my_memcpy(u8 *dst, u8 *src, int size)
{
	for(int i = 0; i < size; i++) dst[i] = src[i];
}

static u32 cached_cd_sector = 0x80000000;

static int process_read_cd_2048_cmd_iso(u8 *buf, u32 start_sector, u32 sector_count)
{
	u32 capacity = sector_count * CD_SECTOR_SIZE_2048;
	u32 fit = capacity / CD_SECTOR_SIZE_2352;
	u32 rem = (sector_count-fit);
	u32 i;
	u8 *in = buf;
	u8 *out = buf;

	if(fit)
	{
		process_read_iso_cmd_iso(buf, start_sector * CD_SECTOR_SIZE_2352, fit * CD_SECTOR_SIZE_2352);

		for(i = 0; i < fit; i++)
		{
			my_memcpy(out, in+24, CD_SECTOR_SIZE_2048);
			in += CD_SECTOR_SIZE_2352;
			out += CD_SECTOR_SIZE_2048;
			start_sector++;
		}
	}

	for(i = 0; i < rem; i++)
	{
		process_read_iso_cmd_iso(out, (start_sector * CD_SECTOR_SIZE_2352)+24, CD_SECTOR_SIZE_2048);
		out += CD_SECTOR_SIZE_2048;
		start_sector++;
	}

	return CELL_OK;
}

static int process_read_cd_2352_cmd_iso(u8 *buf, u32 sector, u32 remaining)
{
	u8 cache = 0;

	if(remaining <= CD_CACHE_SIZE)
	{
		int dif = (int)cached_cd_sector - sector;

		if(ABS(dif) < CD_CACHE_SIZE)
		{
			u8 *copy_ptr = NULL;
			u32 copy_offset = 0;
			u32 copy_size = 0;

			if(dif > 0)
			{
				if(dif < (int)remaining)
				{
					copy_ptr = cd_cache;
					copy_offset = dif;
					copy_size = remaining - dif;
				}
			}
			else
			{

				copy_ptr = cd_cache + ((-dif) * CD_SECTOR_SIZE_2352);
				copy_size = MIN((int)remaining, CD_CACHE_SIZE + dif);
			}

			if(copy_ptr)
			{
				memcpy(buf+(copy_offset * CD_SECTOR_SIZE_2352), copy_ptr, copy_size * CD_SECTOR_SIZE_2352);

				if(remaining == copy_size)
				{
					return CELL_OK;
				}

				remaining -= copy_size;

				if(dif <= 0)
				{
					u32 newsector = cached_cd_sector + CD_CACHE_SIZE;
					buf += ((newsector - sector) * CD_SECTOR_SIZE_2352);
					sector = newsector;
				}
			}
		}

		cache = 1;
	}

	if(!cache)
	{
		return process_read_iso_cmd_iso(buf, sector * CD_SECTOR_SIZE_2352, remaining * CD_SECTOR_SIZE_2352);
	}

	if(!cd_cache)
	{
		sys_addr_t addr;

		int ret = sys_memory_allocate(_128KB_, SYS_MEMORY_PAGE_SIZE_64K, &addr); // 128KB to cache 48 sectors of up to 2448 bytes [48*2448 = 117,504 bytes]
		if(ret != CELL_OK)
		{
			//DPRINTF("sys_memory_allocate failed: %x\n", ret);
			return ret;
		}

		cd_cache = (u8 *)addr;
	}

	if(process_read_iso_cmd_iso(cd_cache, sector * CD_SECTOR_SIZE_2352, CD_CACHE_SIZE * CD_SECTOR_SIZE_2352) != 0)
		return FAILED;

	memcpy(buf, cd_cache, remaining * CD_SECTOR_SIZE_2352);
	cached_cd_sector = sector;

	return CELL_OK;
}

#ifdef RAWISO_PSX_MULTI
static void get_psx_track_data(void)
{
	u32 track_data[4];
	u8 buff[CD_SECTOR_SIZE_2048];
	u64 lv2_addr = 0x8000000000000050ULL;
	lv2_addr+= 16ULL * psx_indx;

	my_memcpy((u8)(u32) track_data, lv2_addr, 16ULL);

	int k = 4;
	num_tracks = 0;

	if(!track_data[0])
	{
		tracks[num_tracks].adr_control = 0x14;
		tracks[num_tracks].track_number = 1;
		tracks[num_tracks].track_start_addr = 0;
		num_tracks++;

	}
	else
	{
		lv2_addr = 0x8000000000000000ULL + (u64) track_data[2];
		my_memcpy((u8)(u32) buff, lv2_addr, track_data[3]);

		while(k < (int) track_data[3])
		{
			tracks[num_tracks].adr_control = (buff[k + 1] != 0x14) ? 0x10 : 0x14;
			tracks[num_tracks].track_number = num_tracks+1;
			tracks[num_tracks].track_start_addr = ((u32) buff[k + 4] << 24) | ((u32) buff[k + 5] << 16) |
												  ((u32) buff[k + 6] << 8)  | ((u32) buff[k + 7]);
			num_tracks++; if(num_tracks >= 64) break;
			k+= 8;
		}

		if(num_tracks > 1) num_tracks--;
	}

	discsize = ((u64) psx_isos_desc[psx_indx * 2 + 1]) * (u64)CD_SECTOR_SIZE_2352;

	num_sections = (psx_isos_desc[psx_indx * 2 + 0] >> 16) & 0xFFFF;
	sections = &psx_isos_desc[16] + (psx_isos_desc[psx_indx * 2 + 0] & 0xFFFF);
	sections_size = sections + num_sections;
}

static int ejected_disc(void)
{
	static int ejected = 0;
	static int counter = 0;
	int fd = NONE;

	if(usb_device != 0)
	{
		int r = sys_storage_get_device_info(usb_device, &disc_info);
		if(r == 0)
		{
			counter++;

			if(ejected && counter > 100)
			{
				if(handle != SYS_DEVICE_HANDLE_NONE) sys_storage_close(handle); handle = SYS_DEVICE_HANDLE_NONE;

				if(sys_storage_open(usb_device, 0, &handle, 0) == 0)
				{
					led(GREEN, ON);
					ejected = 0;
					counter = 0;
					return 1;

				}
				else
				{
					handle = SYS_DEVICE_HANDLE_NONE;
					return FAILED;
				}
			}
			else
				return 2;
		}
		else if(r == (int) 0x80010002)
		{
			led(GREEN, BLINK_FAST);

			if(!ejected)
			{
				ejected = 1;
				counter = 0;
				return CELL_OK;
			}
			else
				return FAILED;
		}
		else
		{
			ejected = 1;
			return -128;
		}
	}


	if(cellFsOpendir("/psx_cdrom0", &fd) == CELL_FS_SUCCEEDED)
	{
		cellFsClosedir(fd);
		if(ejected)
		{
			led(GREEN, ON);
			ejected = 0;
			return 1;
		}
		else
			return 2;

	}
	else
	{
		led(GREEN, BLINK_FAST);

		if(!ejected)
		{
			ejected = 1;
			return CELL_OK;
		}
		else
			return FAILED;
	}

	return -2;
}

static void eject_thread(u64 arg)
{
	eject_running = 1;

	while(eject_running)
	{
		if(emu_mode == EMU_PSX_MULTI)
		{
			int ret;
			ret = ejected_disc();

			if(ret == CELL_OK)
			{
				sys_storage_ext_get_disc_type(&real_disctype, NULL, NULL);
				fake_eject_event(BDVD_DRIVE);
				sys_storage_ext_umount_discfile();

				if(real_disctype != DISC_TYPE_NONE)
				{
					fake_insert_event(BDVD_DRIVE, real_disctype);
				}

				if(discfd != NONE) cellFsClose(discfd); discfd = NONE;

				if(handle != SYS_DEVICE_HANDLE_NONE) sys_storage_close(handle); handle = SYS_DEVICE_HANDLE_NONE;

				if(command_queue_ntfs != SYS_EVENT_QUEUE_NONE)
				{
					eject_running = 2;
					sys_event_queue_t command_queue2 = command_queue_ntfs;
					command_queue_ntfs = SYS_EVENT_QUEUE_NONE;

					if(sys_event_queue_destroy(command_queue2, SYS_EVENT_QUEUE_DESTROY_FORCE) != 0)
					{
						//DPRINTF("Failed in destroying command_queue_ntfs\n");
					}
				}

				while(do_run) sys_ppu_thread_usleep(100000);

				psx_indx = (psx_indx + 1) & 7;

			}
			else if(ret == 1)
			{
				get_psx_track_data();

				sys_storage_ext_get_disc_type(&real_disctype, NULL, NULL);

				if(real_disctype != DISC_TYPE_NONE)
				{
					fake_eject_event(BDVD_DRIVE);
				}

				sys_event_queue_attribute_t queue_attr;
				sys_event_queue_attribute_initialize(queue_attr);
				ret = sys_event_queue_create(&command_queue_ntfs, &queue_attr, 0, 1);
				if(ret == CELL_OK) {eject_running = 1; ret = sys_storage_ext_mount_discfile_proxy(result_port, command_queue_ntfs, emu_mode & 0xF, discsize, _256KB_, (num_tracks | cd_sector_size_param), tracks);}

				if(ret != CELL_OK) psx_indx = (psx_indx - 1) & 7;
				else {fake_insert_event(BDVD_DRIVE, real_disctype);}
			}
		}

		sys_ppu_thread_usleep(70000);
	}

	//DPRINTF("Exiting eject thread!\n");
	sys_ppu_thread_exit(0);
}
#endif //#ifdef RAWISO_PSX_MULTI

static sys_ppu_thread_t thread_id_ntfs = SYS_PPU_THREAD_NONE;

static void rawseciso_thread(u64 arg)
{
	sys_addr_t sysmem = 0;
	if(sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) != CELL_OK) {sys_ppu_thread_exit(0); return;}

	u64 *argp = (u64*)(u32)arg;
	u64 *addr = (u64*)sysmem;

	for(u16 i = 0; i < 0x2000; i++) addr[i] = argp[i]; // copy arguments 64KB

	rawseciso_args *args;

	sys_event_queue_attribute_t queue_attr;

	int ret = 0; cd_sector_size_param = 0;

	args = (rawseciso_args *)(u32)addr; if(!args) {sys_memory_free(sysmem); sys_ppu_thread_exit(ret);}

	//DPRINTF("Hello VSH\n");

	num_sections = 0;
	CD_SECTOR_SIZE_2352 = 2352;

	emu_mode = args->emu_mode & 0x3FF;

#ifdef RAWISO_PSX_MULTI
	psxseciso_args *psx_args;
	psx_args = (psxseciso_args *)(u32)arg;

	if(emu_mode == EMU_PSX_MULTI)
		mode_file = 0;
	else
#endif
	{
		num_sections = args->num_sections;
		sections = (u32 *)(args+1);

		mode_file = (args->emu_mode & 3072) >> 10;

		if(mode_file == 0)
			sections_size = sections + num_sections;
		else if(mode_file == 1)
			sections_size = sections + num_sections * 0x200/4;
		else
		{
			sections += 0x200/4;
			sections_size = sections + num_sections;
		}

		discsize = 0;

		for(u32 i = 1 * (mode_file > 1); i < num_sections; i++)
		{
			discsize += sections_size[i];
		}
	}

	if(mode_file != 1)
	{
		sec_size = 512ULL;
		if(args->device != 0)
		{
			for(int retry = 0; retry < 16; retry++)
			{
				if(sys_storage_get_device_info(args->device, &disc_info) == 0)
				{
					sec_size = (u32) disc_info.sector_size;
					break;
				}

				sys_ppu_thread_usleep(500000);
			}
		}

	}
	else
		sec_size = CD_SECTOR_SIZE_2048;

#ifdef RAWISO_PSX_MULTI
	if(emu_mode == EMU_PSX_MULTI)
	{
		psx_isos_desc = &psx_args->discs_desc[0][0];

		is_cd2352 = 2;
	}
	else
#endif
	if(emu_mode == EMU_PSX)
	{   // old psx support
		num_tracks = args->num_tracks;

		if(num_tracks > 0xFF)
		{
			CD_SECTOR_SIZE_2352 = (num_tracks & 0xFFF00)>>4;
			if(CD_SECTOR_SIZE_2352 > 2448) CD_SECTOR_SIZE_2352 = (num_tracks & 0xFFF00)>>8;
		}

		num_tracks &= 0xFF;

		if(num_tracks)
			memcpy((void *) tracks, (void *) ((ScsiTrackDescriptor *)(sections_size + num_sections)), num_tracks * sizeof(ScsiTrackDescriptor));
		else
		{
			tracks[num_tracks].adr_control = 0x14;
			tracks[num_tracks].track_number = 1;
			tracks[num_tracks].track_start_addr = 0;
			num_tracks++;
		}

		is_cd2352 = 1;

		discsize *= sec_size;
		CD_SECTOR_SIZE_2352 = default_cd_sector_size(discsize);

		sys_addr_t addr;

		if(sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &addr) == CELL_OK)
		{
			cd_cache = (u8 *)addr;

			if(process_read_iso_cmd_iso(cd_cache, 0, 0xA000) == CELL_OK)
			{
				CD_SECTOR_SIZE_2352 = detect_cd_sector_size((char*)cd_cache);
			}

			sys_memory_free(addr); cd_cache = 0;
		}

		if(discsize % CD_SECTOR_SIZE_2352)
		{
			discsize -= (discsize % CD_SECTOR_SIZE_2352);
		}

		//if(CD_SECTOR_SIZE_2352 != 2352 && CD_SECTOR_SIZE_2352 != 2048 && CD_SECTOR_SIZE_2352 != 2336 && CD_SECTOR_SIZE_2352 != 2448) CD_SECTOR_SIZE_2352 = 2352;
		if(CD_SECTOR_SIZE_2352 & 0xf) cd_sector_size_param = CD_SECTOR_SIZE_2352<<8;
		else if(CD_SECTOR_SIZE_2352 != 2352) cd_sector_size_param = CD_SECTOR_SIZE_2352<<4;
	}
	else
	{
		num_tracks = 0;
		discsize = discsize * sec_size + ((mode_file > 1) ? (u64) (CD_SECTOR_SIZE_2048 * sections[0] ) : 0ULL) ;
		is_cd2352 = 0;
	}


	//DPRINTF("discsize = %lx%08lx\n", discsize>>32, discsize);

	if(mode_file != 1)
	{
		usb_device = args->device;

		if(usb_device != 0)
		{
			ret = sys_storage_open(usb_device, 0, &handle, 0);
			if(ret != CELL_OK)
			{
				//DPRINTF("sys_storage_open failed: %x\n", ret);
				goto exit_rawseciso;
			}
		}
	}

	ret = sys_event_port_create(&result_port, 1, SYS_EVENT_PORT_NO_NAME);
	if(ret != CELL_OK)
	{
		//DPRINTF("sys_event_port_create failed: %x\n", ret);
		goto exit_rawseciso;
	}

	sys_event_queue_attribute_initialize(queue_attr);
	ret = sys_event_queue_create(&command_queue_ntfs, &queue_attr, 0, 1);
	if(ret != CELL_OK)
	{
		//DPRINTF("sys_event_queue_create failed: %x\n", ret);
		goto exit_rawseciso;
	}

	sys_storage_ext_get_disc_type(&real_disctype, NULL, NULL);

	if(real_disctype != DISC_TYPE_NONE)
	{
		fake_eject_event(BDVD_DRIVE);
	}

#ifdef RAWISO_PSX_MULTI
	if(emu_mode == EMU_PSX_MULTI)
	{
		get_psx_track_data();

		sys_ppu_thread_create(&thread_id_eject, eject_thread, 0, 3000, THREAD_STACK_SIZE_8KB, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_PSX_EJECT);
	}
#endif

	ret = sys_storage_ext_mount_discfile_proxy(result_port, command_queue_ntfs, emu_mode & 0xF, discsize, _256KB_, (num_tracks | cd_sector_size_param), tracks);
	//DPRINTF("mount = %x\n", ret);

	fake_insert_event(BDVD_DRIVE, real_disctype);

	if(ret != CELL_OK)
	{
		// Queue destroyed in stop thread sys_event_queue_destroy(command_queue_ntfs);
		goto exit_rawseciso;
	}

	rawseciso_loaded = ntfs_running = 1;

	while(ntfs_running)
	{
		sys_event_t event;

		do_run = 0;

		if(command_queue_ntfs == SYS_EVENT_QUEUE_NONE)
		{
			if(!ntfs_running) break;

			sys_ppu_thread_usleep(100000);

			continue;
		}

		ret = sys_event_queue_receive(command_queue_ntfs, &event, 0);
		if(ret != CELL_OK)
		{
			if(command_queue_ntfs == SYS_EVENT_QUEUE_NONE || eject_running == 2)
			{
				if(!ntfs_running) break;

				sys_ppu_thread_usleep(100000);

				continue;
			}

			if(ret != (int) 0x80010013) vsh_shutdown();
			//DPRINTF("sys_event_queue_receive failed: %x\n", ret);
			break;
		}

		if(!ntfs_running) break;

		do_run = 1;

		void *buf = (void *)(u32)(event.data3>>32ULL);
		u64 offset = event.data2;
		u32 size = event.data3 & 0xFFFFFFFF;

		switch (event.data1)
		{
			case CMD_READ_ISO:
			{
				if(is_cd2352)
				{
#ifdef RAWISO_PSX_MULTI
					if(is_cd2352 == 1)
						ret = process_read_cd_2048_cmd_iso(buf, offset / CD_SECTOR_SIZE_2048, size / CD_SECTOR_SIZE_2048);
					else
						ret = process_read_psx_cmd_iso(buf, offset / CD_SECTOR_SIZE_2048, size / CD_SECTOR_SIZE_2048, CD_SECTOR_SIZE_2048);
#else
						ret = process_read_cd_2048_cmd_iso(buf, offset / CD_SECTOR_SIZE_2048, size / CD_SECTOR_SIZE_2048);
#endif
				}
				else
				{
					if(mode_file == 0)
						ret = process_read_iso_cmd_iso(buf, offset, size);
					else if(mode_file == 1)
						ret = process_read_file_cmd_iso(buf, offset, size);
					else
					{
						if(offset < (((u64)sections_size[0]) * CD_SECTOR_SIZE_2048))
						{
							u32 rd = sections_size[0] * CD_SECTOR_SIZE_2048;

							if(rd > size) rd = size;
							ret = process_read_file_cmd_iso(buf, offset, rd);
							if(ret == CELL_OK)
							{
								size-= rd;
								offset+= rd;
								buf = ((char *) buf) + rd;

								if(size != 0)
									ret = process_read_iso_cmd_iso(buf, offset, size);
							}
						}
						else
							ret = process_read_iso_cmd_iso(buf, offset, size);

#ifdef RAWISO_PSX_MULTI
						if(discfd != NONE) cellFsClose(discfd); discfd = NONE;
#endif
					}
				}
			}
			break;

			case CMD_READ_CD_ISO_2352:
			{
#ifdef RAWISO_PSX_MULTI
				if(is_cd2352 == 1)
					ret = process_read_cd_2352_cmd_iso(buf, offset / CD_SECTOR_SIZE_2352, size / CD_SECTOR_SIZE_2352);
				else
					ret = process_read_psx_cmd_iso(buf, offset / CD_SECTOR_SIZE_2352, size / CD_SECTOR_SIZE_2352, CD_SECTOR_SIZE_2352);
#else
					ret = process_read_cd_2352_cmd_iso(buf, offset / CD_SECTOR_SIZE_2352, size / CD_SECTOR_SIZE_2352);
#endif
			}
			break;
		}

		while(ntfs_running)
		{
			ret = sys_event_port_send(result_port, ret, 0, 0);
			if(ret == CELL_OK) break;

			if(ret == (int) 0x8001000A)
			{   // EBUSY
				sys_ppu_thread_usleep(100000);
				continue;
			}

			break;
		}

		//DPRINTF("sys_event_port_send failed: %x\n", ret);
		if(ret != CELL_OK) break;
	}

	ret = CELL_OK;

exit_rawseciso:

	do_run = eject_running = 0;

	if(args) sys_memory_free((sys_addr_t)args);

	if(command_queue_ntfs != SYS_EVENT_QUEUE_NONE)
	{
		sys_event_queue_destroy(command_queue_ntfs, SYS_EVENT_QUEUE_DESTROY_FORCE);
	}

	sys_storage_ext_get_disc_type(&real_disctype, NULL, NULL);
	fake_eject_event(BDVD_DRIVE);
	sys_storage_ext_umount_discfile();

	if(real_disctype != DISC_TYPE_NONE)
	{
		fake_insert_event(BDVD_DRIVE, real_disctype);
	}

	if(cd_cache)
	{
		sys_memory_free((sys_addr_t)cd_cache);
	}

	if(handle != SYS_DEVICE_HANDLE_NONE) sys_storage_close(handle);

#ifdef RAWISO_PSX_MULTI
	if(discfd != NONE) cellFsClose(discfd);
#endif

	sys_event_port_disconnect(result_port);

	if(sys_event_port_destroy(result_port) != 0)
	{
		//DPRINTF("Error destroyng result_port\n");
	}

	// queue destroyed in stop thread
	rawseciso_loaded = 0;

	//DPRINTF("Exiting main thread!\n");
	sys_ppu_thread_exit(ret);
}

static void rawseciso_stop_thread(u64 arg)
{
	u64 exit_code;

	do_run = ntfs_running = 0;

	if(command_queue_ntfs != SYS_EVENT_QUEUE_NONE)
	{
		if(sys_event_queue_destroy(command_queue_ntfs, SYS_EVENT_QUEUE_DESTROY_FORCE) != 0)
		{
			//DPRINTF("Failed in destroying command_queue_ntfs\n");
		}
	}

	if(thread_id_ntfs != SYS_PPU_THREAD_NONE)
	{
		sys_ppu_thread_join(thread_id_ntfs, &exit_code);
	}

#ifdef RAWISO_PSX_MULTI
	eject_running = 0;

	if(thread_id_eject != SYS_PPU_THREAD_NONE)
	{
		sys_ppu_thread_join(thread_id_eject, &exit_code);
	}
#endif

	sys_ppu_thread_exit(0);
}

#endif //#ifdef USE_INTERNAL_PLUGIN
