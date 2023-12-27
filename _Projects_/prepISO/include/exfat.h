#define MAX_ISOS 2000

static int  nn = 0;
static size_t nt[MAX_ISOS];
static char ff[MAX_ISOS][256];

static char subpath[MAX_PATH_LEN];
/*
void addlog(char *msg1, char *msg2)
{
	FILE* f;
	f = fopen("/dev_hdd0/prepiso2.log", "a+b");
	if(f)
	{
		char msg[300];
		sprintf(msg, "%s %s\r\n", msg1, msg2);
		fwrite(msg, 1, strlen(msg), f);
		fclose (f);
	}
}
*/
static int s_mode; // 0 = png/jpg/sfo, 1=iso/bin/img/mdf

int copy_exfat(char *src_file, char *out_file, u64 size)
{
	if(size == 0) return FAILED;

	FIL fd;	   // File objects

	if (!f_open (&fd, src_file, FA_READ))
	{
		char *mem = NULL;

		if((mem = malloc(size)) != NULL)
		{
			f_lseek (&fd, 0);
			UINT re;
			f_read (&fd, (void *)mem, size, &re);
			f_close (&fd);

			SaveFile(out_file, mem, re);

			free(mem);

			return (re == size) ? SUCCESS : FAILED;
		}
		else
		{
			f_close (&fd);
		}
	}
	return FAILED;
}

//dir contents
static int dir_read (char *dpath)
{
	nn = 0;

	/* Register work area to the default drive */
	FATFS fs;   /* Work area (filesystem object) for logical drive */
	char drn[6];
	snprintf(drn, 5, "%s", dpath);
	if(drn[2]=='/')drn[3]=0;
	if(drn[3]=='/')drn[4]=0;
	if(drn[4]=='/')drn[5]=0;
	f_mount(&fs, drn, 0);

	char fn[256];

	FDIR dir;
	FRESULT res;

	res = f_opendir(&dir, dpath);					   /* Open the directory */
	if (res == FR_OK)
	{
		bool is_iso;
		static FILINFO fno;
		for(;;)
		{
			FRESULT res1 = f_readdir(&dir, &fno);	   /* Read a directory item */
			if (res1 != FR_OK || fno.fname[0] == 0)
			{
				break;
			}
			if (fno.fattrib & AM_DIR)
			{				/* scan directories recursively */
				if(fno.fname[0] == '.') continue;
				snprintf (subpath, 255, "%s", fno.fname);
				snprintf (fn, 255, "%s/%s", dpath, fno.fname);
				dir_read (fn);
			}
			else if(s_mode == 1)
			{
				int flen = strlen(fno.fname);
				if(flen < 4) continue; flen -= 4;
				char *ext = fno.fname + flen;

				//--- create .ntfs[BDFILES] for 4="VIDEO", 5="MOVIES", 6="PKG", 7="Packages", 8="packages", 9="BDFILE", 10="PS2ISO", 11="PSPISO", 12="MUSIC", 13="THEME", 14="UPDATE", 15="ROMS"
				if(g_mode >= 4)
				{
					// store path for conversion after the directory scan (fflib_file_to_sectors breaks the scan)
					nt[nn] = fno.fsize;
					snprintf (ff[nn], 255, "%s", fno.fname);
					nn++; if(nn >= MAX_ISOS) break;
					continue;
				}
				//---------------
				 
				// If the key exists, copy it to "/dev_hdd0/tmp/wmtmp" to 
				// decrypt on-the-fly with Cobra when the ISO is mounted (By Evilnat) 				
				if(strcasestr(ext, ".key"))
				{
					FIL fd;
					char output[256];
					snprintf (fn, 255, "%s/%s", dpath, fno.fname);
					sprintf(output, "/dev_hdd0/tmp/wmtmp/%s", fno.fname);
					
					if (!f_open(&fd, fn, FA_READ))
					{
						UINT re;
						uint8_t data[0x10];
						f_read(&fd, data, 0x10, &re);
						f_close (&fd);

						int fda = ps3ntfs_open(output, O_WRONLY | O_CREAT | O_TRUNC, 0777);
						if(fda >= 0)
						{
							ps3ntfs_write(fda, (void *)data, 0x10);
							ps3ntfs_close(fda);
						}
					}

				}

				//--- is ISO?
				is_iso =	( (strcasestr(ext, ".iso")) ) ||
				(g_mode && (( (strcasestr(ext, ".bin")) ) ||
							( (strcasestr(ext, ".img")) ) ||
							( (strcasestr(ext, ".mdf")) ) ));

				if(is_iso)
				{
					cd_sector_size = 2352;
					cd_sector_size_param = 0;
					num_tracks = 1;

					if(g_mode == PSXISO)
					{
						FIL fd;
						int ret;

						// detect CD sector size
						if(!f_open (&fd, fn, FA_READ))
						{
							char buffer[20];
							u16 sec_size[7] = {2352, 2048, 2336, 2448, 2328, 2340, 2368};
							for(u8 n = 0; n < 7; n++)
							{
								f_lseek (&fd, ((sec_size[n]<<4) + 0x18));
								UINT lbr;
								f_read (&fd, (void *)buffer, 20, &lbr);
								if(  (memcmp(buffer + 8, "PLAYSTATION ", 0xC) == 0) ||
									((memcmp(buffer + 1, "CD001", 5) == 0) && buffer[0] == 0x01) ) {cd_sector_size = sec_size[n]; break;}
							}
							f_close(&fd);

							if(cd_sector_size & 0xf) cd_sector_size_param = cd_sector_size<<8;
							else if(cd_sector_size != 2352) cd_sector_size_param = cd_sector_size<<4;
						}

						// parse CUE file
						int path_len = snprintf (fn, 255, "%s/%s", dpath, fno.fname);
						const char *cue_ext[4] = {".cue", ".ccd", ".CUE", ".CCD"};
						for(u8 e = 0; e < 4; e++)
						{
							strcpy(fn + path_len - 4, cue_ext[e]);
							ret = f_open (&fd, fn, FA_READ);
							if(ret == SUCCESS) break;
						}

						if (ret >= SUCCESS)
						{
							UINT r = 0;
							f_read (&fd, (char *)cue_buf, sizeof(cue_buf), &r);
							f_close(&fd);

							if (r > 0)
							{
								char templn[MAX_LINE_LEN];
								num_tracks = parse_cue(templn, (char *)cue_buf, r, tracks);
							}
						}
					}

					// store path for conversion after the directory scan (fflib_file_to_sectors breaks the scan)
					nt[nn] = num_tracks;
					snprintf (ff[nn], 255, "%s", fno.fname);
					nn++; if(nn >= MAX_ISOS) break;
				}
			}
			else if(s_mode == 0)
			{
				if(strstr(fno.fname, ".png") || strstr(fno.fname, ".PNG") || strstr(fno.fname, ".jpg") || strstr(fno.fname, ".JPG") || strstr(fno.fname, ".SFO"))
				{
					if(*subpath && (strncmp(subpath, fno.fname, strlen(fno.fname)) != 0))
					{
						snprintf (wm_path, 255, "/dev_hdd0/tmp/wmtmp/[%s] %s", subpath, fno.fname);
					}
					else
						snprintf (wm_path, 255, "/dev_hdd0/tmp/wmtmp/%s", fno.fname);

					if((not_exists(wm_path)) && (fno.fsize < 4194304))
					{
						snprintf(fn, 255, "%s/%s", dpath, fno.fname);
						copy_exfat(fn, wm_path, fno.fsize);
					}
				}
			}
		}
		f_closedir(&dir);
	}
	//
	if(nn)
	{
		int flen, dlen, plen;
		for(dlen = strlen(dpath); dlen; dlen--) if(dpath[dlen] == '/') break; // find subdir name

		char *subdir = &dpath[dlen + 1]; sprintf(subpath, "%s", dpath);
		for(int c = 0; subpath[c]; c++)
			if(subpath[c] == '[') subpath[c] = '('; else
			if(subpath[c] == ']') subpath[c] = ')';

		int parts;

		for(int f = 0; f < nn; f++)
		{
			for(int i = 0; i < MAX_SECTIONS; i++) sections[i] = sections_size[i] = 0;

			flen = snprintf (fn, 255, "%s/%s", subpath, ff[f]);

			parts = fflib_file_to_sectors (fn, sections, sections_size, MAX_SECTIONS, 1);
			if (parts >= MAX_SECTIONS) continue;

			if(dlen >= 10)
				plen = snprintf (wm_path, 255, "[%s] %s", subdir, ff[f]);
			else
				plen = snprintf (wm_path, 255, "%s", ff[f]);

			if(!g_mmcm) wm_path[plen - 4] = 0;

			num_tracks = nt[f];

			if(g_mode >= 4)
			{
				char *ext = fn + flen - 4;
				make_fake_iso(g_mode, ext, ff[f], fn, device_id, nt[f]);
			}
			else
				build_file(wm_path, parts, num_tracks, device_id, g_profile, g_mode);
		}
		nn = 0;
	}
	//
	f_mount(0, drn, 0);
	//
	return res;
}

static void scan_exfat(void)
{
	fflib_init();
	for(u8 port = 0; port < FF_VOLUMES; port++) // 0-7, 64-79, 120-130
	{
		if(port == 8)  port = 64;
		if(port == 80) port = 120;

		sprintf(path, "/dev_usb%03u/", port);
		if(file_exists(path)) continue; // skip scanning of this USB port if it is readable by PS3 file system

		device_id = USB_MASS_STORAGE(port);
		int ret = fflib_attach (port, device_id, 1);
		if (ret == FR_OK)
		{
			for (u8 p = 0; p < 2; p++)
			{
				for (u8 profile = 0; profile < 6; profile++)
				{
					g_profile = profile;
					for(u8 m = 0; m < MAX_MODES; m++) //0="PS3ISO", 1="BDISO", 2="DVDISO", 3="PSXISO", 4="VIDEO", 5="MOVIES", 6="PKG", 7="Packages", 8="packages", 9="BDFILE", 10="PS2ISO", 11="PSPISO", 12="MUSIC", 13="THEME", 14="UPDATE", 15="ROMS"
					{
						g_mode = m;
						*subpath = 0;

						snprintf(path, sizeof(path), "%u:/%s%s%s", port, prefix[p], c_path[m], SUFIX(profile));
						for(s_mode = 0; s_mode < 2; s_mode++)
							dir_read (path);
					}
				}
			}
		}
		fflib_detach (port);
	}
}
