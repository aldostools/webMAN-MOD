#include "fake_iso_data.h"

static char * get_extension(char *path)
{
	int n = strlen(path);
	int m = n;

	while(m > 1 && path[m] != '.' && path[m] != '/') m--;

	if(!strcmp(&path[m], ".0"))
		while(m > 1 && path[m] != '.' && path[m] != '/') m--;

	if(path[m] == '.') return &path[m];

	return &path[n];
}

static char * get_filename(char *path)
{
	int n = strlen(path);
	int m = n;

	while(m > 0 && path[m] != '/') m--;

	if(path[m] == '/') m++;

	return &path[m];
}

static void set733(unsigned char *p,int n)
{
	*p++=(n & 0xff);*p++=((n>>8) & 0xff);*p++=((n>>16) & 0xff);*p++=((n>>24) & 0xff);
	*p++=((n>>24) & 0xff);*p++=((n>>16) & 0xff);*p++=((n>>8) & 0xff);*p++=(n & 0xff);
}

static void set723(unsigned char *p,int n)
{
	*p++=(n & 0xff);*p++=((n>>8) & 0xff);*p++=((n>>8) & 0xff);*p++=(n & 0xff);
}

static void UTF8_to_UTF16(u8 *stb, u16 *stw)
{
	int n, m;
	u32 UTF32;
	while(*stb)
	{
		if(*stb & 128)
		{
			m = 1;

			if((*stb & 0xf8) == 0xf0)
			{
				// 4 bytes
				UTF32 = (u32) (*(stb++) & 3);
				m = 3;
			}
			else if((*stb & 0xE0) == 0xE0)
			{
				// 3 bytes
				UTF32 = (u32) (*(stb++) & 0xf);
				m = 2;
			}
			else if((*stb & 0xE0) == 0xC0)
			{
				// 2 bytes
				UTF32 = (u32) (*(stb++) & 0x1f);
				m = 1;
			}
			else {stb++; continue;} // error!

			for(n = 0; n < m; n++)
			{
				if(!*stb) break; // error!

				if((*stb & 0xc0) != 0x80) break; // error!
				UTF32 = (UTF32 <<6) |((u32) (*(stb++) & 63));
			}

			if((n != m) && !*stb) break;

		} else UTF32 = (u32) *(stb++);

		if(UTF32<65536)
			*stw++= (u16) UTF32;
		else
		{
			//110110ww wwzzzzyy 110111yy yyxxxxxx
			*stw++= (((u16) (UTF32>>10)) & 0x3ff) | 0xD800;
			*stw++= (((u16) (UTF32)) & 0x3ff) | 0xDC00;
		}
	}

	*stw++ = 0;
}


static void *create_directory_record(struct iso_directory_record *idr, char *name, u16 name_len, u64 size, u8 flags, u32 last_lba)
{
	memcpy(idr->name, name, name_len);

	idr->name_len[0] = name_len;
	idr->length[0] = (name_len + sizeof(struct iso_directory_record) + 1) & ~1;
	idr->ext_attr_length[0] = 0;

	set733(idr->extent, last_lba);
	set733(idr->size, size);

	idr->date[0] = 0x71; idr->date[1] = 0x0B;
	idr->date[2] = 0x0A; idr->date[3] = 0x0D;
	idr->date[4] = 0x38; idr->date[5] = 0x00;
	idr->date[6] = 0x04;

	idr->flags[0] = flags;
	idr->file_unit_size[0] = 0;
	idr->interleave[0] = 0;

	set723(idr->volume_sequence_number, 1);

	return (struct iso_directory_record *) (((char *) idr) + idr->length[0]);
}

static u8 *create_fake_file_iso_mem(char *filename, u64 size)
{
	u8 *mem = malloc(build_iso_size);
	if(!mem) return NULL;
	u16 *string = (u16 *) malloc(256);
	if(!string) {free(mem); return NULL;}

	char name[65];
	u16 len_string = snprintf(name, 64, "%s", filename);
	name[64] = 0;

	if(len_string > 64)
	{
		char *ext = get_extension(filename);
		// break the string
		int pos = 63 - strlen(ext);
		while(pos > 0 && (name[pos] & 192) == 128) pos--; // skip UTF extra codes
		strcpy(&name[pos], ext);
	}

	UTF8_to_UTF16((u8 *) name, string);

	for(len_string = 0; len_string < 0x200; len_string++) if(string[len_string] == 0) break; // get string length

	memset(mem, 0, build_iso_size);
	memcpy(mem + 0x8000, build_iso_data, sizeof(build_iso_data));

	struct iso_primary_descriptor *ipd = (struct iso_primary_descriptor *) &mem[0x8000];
	struct iso_primary_descriptor *ipd2 = (struct iso_primary_descriptor *) &mem[0x8800];
	struct iso_directory_record * idr = (struct iso_directory_record *) &mem[0xB840];
	struct iso_directory_record * idr2 = (struct iso_directory_record *) &mem[0xC044];

	u32 last_lba = isonum_733 (ipd->volume_space_size);

	u64 size0 = size;

	while(size > 0)
	{
		u8 flags = 0;

		if(size > 0xFFFFF800ULL) {flags = 0x80; size0 = 0xFFFFF800ULL;} else size0 = size;

		idr = create_directory_record(idr, name, strlen(name), size0, flags, last_lba);

		idr2 = create_directory_record(idr2, (char*)string, len_string * 2, size0, flags, last_lba);

		/////////////////////
		last_lba += (size0 + 0x7ffULL)/ 0x800ULL;
		size -= size0;
	}

	last_lba += (size + SECTOR_FILL) / SECTOR_SIZE;
	set733(ipd->volume_space_size, last_lba);
	set733(ipd2->volume_space_size, last_lba);

	free(string);
	return mem;
}

static int create_fake_file_iso(char *path, char *filename, u64 size)
{
	u8 *mem = create_fake_file_iso_mem(filename, size);
	if(!mem) return FAILED;

	int ret = SUCCESS;

	FILE *fp2 = fopen(path, "wb");

	if(fp2)
	{
		fwrite((void *) mem, 1, build_iso_size, fp2);
		fclose(fp2);
	}
	else ret = FAILED;

	free(mem);

	return ret;
}

static int build_fake_iso(char *iso_path, char *src_path, uint64_t device_id, char *file_ext, u64 size)
{
	int type = EMU_BD;

	bool is_exfat = (*src_path >= '0' && *src_path <= '9');

	if(is_exfat) ; else
	if(not_exists(src_path)) return FAILED;

	int iso_path_len = strlen(iso_path) - 4; if(iso_path_len < 0) return FAILED;

	uint8_t *plugin_args = malloc(PLUGIN_ARGS_SIZE);

	if(plugin_args)
	{
		if(size == 0)
			size = get_filesize(src_path);

		char filename[MAXPATHLEN]; //name only
		sprintf(filename, "%s", get_filename(src_path));
		create_fake_file_iso(iso_path, filename, size);

		if(not_exists(iso_path)) {free(plugin_args); return FAILED;}

		int r = FAILED;

		memset(plugin_args, 0, PLUGIN_ARGS_SIZE);

		uint32_t *sections      = (uint32_t *)(plugin_args + sizeof(rawseciso_args));
		uint32_t *sections_size = (uint32_t *)(plugin_args + sizeof(rawseciso_args) + (MAX_SECTIONS * sizeof(uint32_t)));

		// create file section
		strncpy((char *) sections, iso_path, 0x1ff);
		((char *) sections)[0x1ff] = 0;

		size = get_filesize(iso_path);
		if (size == 0) goto skip_load_ntfs;
		sections_size[0] = size / 2048ULL;
		sections[0x200/4] = 0;
		int parts = 1;

		if(parts < MAX_SECTIONS)
		{
			if(is_exfat)
				parts += fflib_file_to_sectors  (src_path, sections + parts + 0x200/4, sections_size + parts, MAX_SECTIONS - parts - 0x200/4, 1);
			else
				parts += ps3ntfs_file_to_sectors(src_path, sections + parts + 0x200/4, sections_size + parts, MAX_SECTIONS - parts - 0x200/4, 1);
		}

		if (parts > 0 && parts < (MAX_SECTIONS - 0x200/4))
		{
			rawseciso_args *p_args;
			p_args = (rawseciso_args *)plugin_args;
			p_args->device = device_id;
			p_args->emu_mode = type | 0x800;
			p_args->num_sections = parts;
			p_args->num_tracks = 0;

			//memcpy(plugin_args + sizeof(rawseciso_args), sections, parts * sizeof(uint32_t) + 0x200);
			memcpy(plugin_args + sizeof(rawseciso_args) + ((parts * sizeof(uint32_t)) + 0x200), sections_size, parts * sizeof(uint32_t));

			// save sectors file
			iso_path[iso_path_len] = 0; strcat(iso_path, file_ext);

			int fd = ps3ntfs_open(iso_path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
			if(fd >= 0)
			{
				size = sizeof(rawseciso_args) + ((2 * parts * sizeof(uint32_t)) + 0x200);
				if(ps3ntfs_write(fd, (void *) plugin_args, size) == size) r = SUCCESS;
				ps3ntfs_close(fd);
			}
		}

	skip_load_ntfs:
		if(plugin_args) free(plugin_args); plugin_args = NULL;

		if(r == 0) return SUCCESS;
	}

	return FAILED;
}

static void make_fake_iso(uint8_t m, char *ext, char *iso_name, char *src_path, uint64_t device_id, uint64_t file_size)
{
	//if(m >= 4) // 4="VIDEO", 5="MOVIES", 6="PKG", 7="Packages", 8="packages", 9="BDFILE", 10="PS2ISO", 11="PSPISO", 12="MUSIC", 13="THEME", 14="UPDATE", 15="ROMS"
	{
		if((m == VIDEO || m == MOVIES) && !strcasestr(".mp4|.mkv|.avi|.wmv|.flv|.mpg|mpeg|.mov|m2ts|.vob|.asf|divx|xvid|.pam|.bik|bink|.vp6|.mth|.3gp|rmvb|.ogm|.ogv|.m2t|.mts|.tsv|.tsa|.tts|.vp3|.vp5|.vp8|.264|.m1v|.m2v|.m4b|.m4p|.m4r|.m4v|mp4v|.mpe|bdmv|.dvb|webm|.nsv", ext)) return;
		if((m == MUSIC) && !strcasestr(".mp3|.mp2|.wma|.wav|.aac|.ac3|.ogg", ext)) return;
		if((m == PKGFILE) && !strcasestr(".pkg|.pup|.zip", ext)) return;
		if((m == BDFILE) && ((iso_name[0] == '.') || (strstr(iso_name, ".") == NULL))) return;
		if((m == PS2ISO) && !strcasestr(".iso|.enc", ext)) return;
		if((m == PSPISO) && !strcasestr(".iso", ext)) return;
		if((m == THEME) && !strcasestr(".p3t|.edat", ext)) return;
		if((m == ROMS) && (	(iso_name[0] == '.') || (strstr(iso_name, ".") == NULL) ||
							(strcasestr(".png|.jpg|.bmp|.cue|.ini|.cfg|.txt|.log|.htm", ext) != NULL) ||
							(strcasestr(".mp4|.mkv|.avi|.wmv|.flv|.mpg|mpeg", ext) != NULL) ||
							(strcasestr(".mp3|.mp2|.wma|.wav|.aac|.ac3|.ogg", ext) != NULL)
							)) return;

		sprintf(path, "/dev_hdd0/tmp/wmtmp/[%s] %s.iso", c_path[m], iso_name);
		if(file_exists(path)) return;

		char file_ext[16];
		if(m == PS2ISO) sprintf(file_ext, ".ntfs[PS2ISO]"); else
		if(m == PSPISO) sprintf(file_ext, ".ntfs[PSPISO]"); else
						sprintf(file_ext, ".ntfs[BDFILE]");

		build_fake_iso(path, src_path, device_id, file_ext, file_size);

		sprintf(path, "/dev_hdd0/tmp/wmtmp/[%s] %s.png", c_path[m], iso_name);
		if(file_exists(path)) return;

		if(m == PKGFILE) copy_file("/dev_hdd0/game/BLES80616/USRDIR/icons/pkg.png", path);
		if(m == UPDATE)  copy_file("/dev_hdd0/game/BLES80616/USRDIR/icons/pkg.png", path);
		if(m == VIDEO || m == MOVIES) copy_file("/dev_hdd0/game/BLES80616/USRDIR/icons/video.png", path);
		if(m == MUSIC )  copy_file("/dev_hdd0/game/BLES80616/USRDIR/icons/music.png", path);
		if(m == THEME )  copy_file("/dev_hdd0/game/BLES80616/USRDIR/icons/theme.png", path);
		if(m == ROMS  )  copy_file("/dev_hdd0/game/BLES80616/USRDIR/icons/retro.png", path);
		if(m == PS2ISO)  copy_file("/dev_hdd0/game/BLES80616/USRDIR/icons/ps2iso.png", path);
		if(m == PSPISO)  copy_file("/dev_hdd0/game/BLES80616/USRDIR/icons/pspiso.png", path);
		return;
	}
}
