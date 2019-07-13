#include "fake_iso_data.h"

char * get_extension(char *path)
{
	int n = strlen(path);
	int m = n;

	while(m > 1 && path[m] != '.' && path[m] != '/') m--;

	if(!strcmp(&path[m], ".0"))
		while(m > 1 && path[m] != '.' && path[m] != '/') m--;

	if(path[m] == '.') return &path[m];

	return &path[n];
}

char * get_filename(char *path)
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

void UTF8_to_UTF16(u8 *stb, u16 *stw)
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

		if(UTF32 < 65536)
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

void set_directory_record(struct iso_directory_record * idr, u32 last_lba, u64 fsize, u8 flags)
{
	idr->length[0] = (idr->name_len[0] + sizeof(struct iso_directory_record) + 1) & ~1;
	idr->ext_attr_length[0] = 0;

	set733(idr->extent, last_lba);
	set733(idr->size, fsize);

	idr->date[0] = 0x71; idr->date[1] = 0x0B;
	idr->date[2] = 0x0A; idr->date[3] = 0x0D;
	idr->date[4] = 0x38; idr->date[5] = 0x00;
	idr->date[6] = 0x04;

	idr->flags[0] = flags;
	idr->file_unit_size[0] = 0;
	idr->interleave[0] = 0;

	set723(idr->volume_sequence_number, 1);
}

u8 *create_fake_file_iso_mem(char *filename, u64 size)
{
	u8 *mem = malloc(fake_iso_size);
	if(!mem) return NULL;
	u16 *string = (u16 *) malloc(256);
	if(!string) {free(mem); return NULL;}

	char name[65];
	strncpy(name, filename, 64);
	name[64] = 0;

	if(strlen(filename) > 64)
	{
		// break the string
		int pos = 63 - strlen(get_extension(filename));
		while(pos > 0 && (name[pos] & 192) == 128) pos--; // skip UTF extra codes
		strcpy(&name[pos], get_extension(filename));
	}

	UTF8_to_UTF16((u8 *) name, string);

	int len_string;
	for(len_string = 0; len_string < 512; len_string++) if(string[len_string] == 0) break;

	memset(mem, fake_iso_size, 0);
	memcpy(mem + 0x8000, build_iso_data, sizeof(build_iso_data));

	struct iso_primary_descriptor * ipd  = (struct iso_primary_descriptor *) &mem[0x8000];
	struct iso_primary_descriptor * ipd2 = (struct iso_primary_descriptor *) &mem[0x8800];
	struct iso_directory_record   * idr  = (struct iso_directory_record   *) &mem[0xB840];
	struct iso_directory_record   * idr2 = (struct iso_directory_record   *) &mem[0xC044];

	u32 last_lba = isonum_733 (ipd->volume_space_size);

	u64 fsize = size;

	int name_len = strlen(name);

	while(size > 0)
	{
		u8 flags = 0;

		if(size > 0xFFFFF800ULL) {flags = 0x80; fsize = 0xFFFFF800ULL;} else fsize = size;

		idr->name_len[0] = name_len;
		memcpy(idr->name, name, idr->name_len[0]);
		set_directory_record(idr, last_lba, fsize, flags);
		idr = (struct iso_directory_record *) (((char *) idr) + idr->length[0]);

		/////////////////////

		idr2->name_len[0] = len_string * 2;
		memcpy(idr2->name, string, idr2->name_len[0]);
		set_directory_record(idr2, last_lba, fsize, flags);
		idr2 = (struct iso_directory_record *) (((char *) idr2) + idr2->length[0]);

		/////////////////////
		last_lba += (fsize + 0x7ffULL) / 0x800ULL;
		size-= fsize;
	}

	last_lba += (size + SECTOR_FILL) / SECTOR_SIZE;
	set733(ipd->volume_space_size, last_lba);
	set733(ipd2->volume_space_size, last_lba);

	free(string);
	return mem;
}

int create_fake_file_iso(char *path, char *filename, u64 size)
{
	u8 *mem = create_fake_file_iso_mem(filename, size);
	if(!mem) return FAILED;

	int ret = SUCCESS;

	FILE *fp2 = fopen(path, "wb");

	if(fp2)
	{
		fwrite((void *) mem, 1, fake_iso_size, fp2);
		fclose(fp2);
	}
	else ret = FAILED;

	free(mem);

	return ret;
}

static int build_fake_iso(char *iso_path, char *src_path, int ioType, char *file_ext)
{
	int type = EMU_BD;

	if(file_exists(src_path) == false) return FAILED;

	int iso_path_len = strlen(iso_path) - 4; if(iso_path_len < 0) return FAILED;

	uint8_t *plugin_args = malloc(0x20000); // 128KB

	if(plugin_args)
	{
		u64 size;
		size = get_filesize(src_path);

		char filename[MAXPATHLEN]; //name only
		sprintf(filename, "%s", get_filename(src_path));
		create_fake_file_iso(iso_path, filename, size);

		if(file_exists(iso_path) == false) {free(plugin_args); return FAILED;}

		int r = FAILED;

		uint32_t *sections      = malloc(MAX_SECTIONS * sizeof(uint32_t));
		uint32_t *sections_size = malloc(MAX_SECTIONS * sizeof(uint32_t));

		if(plugin_args && sections && sections_size)
		{
			rawseciso_args *p_args;

			memset(sections, 0, MAX_SECTIONS * sizeof(uint32_t));
			memset(sections_size, 0, MAX_SECTIONS * sizeof(uint32_t));

			memset(plugin_args, 0, 0x10000);

			// create file section
			strncpy((char *) sections, iso_path, 0x1ff);
			((char *) sections)[0x1ff] = 0;

			size = get_filesize(iso_path);
			if (size == 0) goto skip_load_ntfs;
			sections_size[0] = size / 2048ULL;
			sections[0x200/4] = 0;
			int parts = 1;

			if(parts < MAX_SECTIONS)
				parts += ps3ntfs_file_to_sectors(src_path, sections + parts + 0x200/4, sections_size + parts, MAX_SECTIONS - parts - 0x200/4, 1);

			if (parts > 0 && parts < (MAX_SECTIONS - 0x200/4))
			{
				p_args = (rawseciso_args *)plugin_args;
				p_args->device = USB_MASS_STORAGE(ioType);
				p_args->emu_mode = type | 0x800;
				p_args->num_sections = parts;
				p_args->num_tracks = 0;


				memcpy(plugin_args + sizeof(rawseciso_args), sections, parts * sizeof(uint32_t) + 0x200);
				memcpy(plugin_args + sizeof(rawseciso_args) + (parts * sizeof(uint32_t) + 0x200), sections_size, parts * sizeof(uint32_t));

				// save sectors file
				iso_path[iso_path_len] = 0; strcat(iso_path, file_ext);

				int fd = ps3ntfs_open(iso_path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
				if(fd >= 0)
				{
					if(ps3ntfs_write(fd, (void *) plugin_args, 0x10000) == 0x10000) r = SUCCESS;
					ps3ntfs_close(fd);
				}
			}

		skip_load_ntfs:
			if(sections) free(sections);
			if(sections_size) free(sections_size);
		}

		if(plugin_args) free(plugin_args); plugin_args = NULL;

		if(r == 0) return SUCCESS;
	}

	return FAILED;
}
