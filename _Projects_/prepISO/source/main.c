#include <stdio.h>
#include <string.h>
#include <lv2/sysfs.h>
#include <sys/file.h>
#include "ntfs.h"
#include "cobra.h"
#include "cue_file.h"
#include "scsi.h"
#include <dirent.h>
#include <unistd.h>
#include <sys/process.h>

#include <io/pad.h>

//----------------
#define SUFIX(a)	((a==1)? "_1" :(a==2)? "_2" :(a==3)? "_3" :(a==4)? "_4" :(a==5)? " [auto]" :"")
#define SUFIX2(a)	((a==1)?" (1)":(a==2)?" (2)":(a==3)?" (3)":(a==4)?" (4)":(a==5)? " [auto]" :"")

#define USB_MASS_STORAGE_1(n)	(0x10300000000000AULL+(n)) /* For 0-5 */
#define USB_MASS_STORAGE_2(n)	(0x10300000000001FULL+((n)-6)) /* For 6-127 */
#define USB_MASS_STORAGE(n)	(((n) < 6) ? USB_MASS_STORAGE_1(n) : USB_MASS_STORAGE_2(n))

//0="PS3ISO", 1="BDISO", 2="DVDISO", 3="PSXISO", 4="VIDEO", 5="MOVIES", 6="PKG", 7="Packages", 8="packages", 9="BDFILE", 10="PS2ISO", 11="PSPISO", 12="MUSIC", 13="THEME", 14="UPDATE", 15="ROMS"
enum emu_modes
{
	PS3ISO = 0,
	BDISO  = 1,
	DVDISO = 2,
	PSXISO = 3,
	/////////// fake ISO types
	VIDEO  = 4,
	MOVIES = 5,
//	PKG    = 6,
//	Packages=7,
//	packages=8,
	BDFILE = 9,
	PS2ISO = 10,
	PSPISO = 11,
	MUSIC  = 12,
	THEME  = 13,
	UPDATE = 14,
	ROMS   = 15,
	//////////// fake ISO types
	MAX_MODES = 16
};

#define PKGFILE 6 || (m == 7) || (m == 8)

#define FW_VERSION 4.86f

typedef struct
{
	uint64_t device;
	uint32_t emu_mode;
	uint32_t num_sections;
	uint32_t num_tracks;
} __attribute__((packed)) rawseciso_args;

#define cue_buf  plugin_args
#define PLUGIN_ARGS_SIZE	0x10000
#define MAX_SECTIONS		((PLUGIN_ARGS_SIZE-sizeof(rawseciso_args))/8)

uint8_t plugin_args[PLUGIN_ARGS_SIZE];

#define MAX_PATH_LEN  0x420

static char path[MAX_PATH_LEN];
static char wm_path[MAX_PATH_LEN];
static char image_file[MAX_PATH_LEN];

//0="PS3ISO", 1="BDISO", 2="DVDISO", 3="PSXISO", 4="VIDEO", 5="MOVIES", 6="PKG", 7="Packages", 8="packages", 9="BDFILE", 10="PS2ISO", 11="PSPISO", 12="MUSIC", 13="THEME", 14="UPDATE", 15="ROMS"
static const char *c_path[16] = {"PS3ISO", "BDISO", "DVDISO", "PSXISO", "VIDEO", "MOVIES", "PKG", "Packages", "packages", "BDFILE", "PS2ISO", "PSPISO", "MUSIC", "THEME", "UPDATE", "ROMS"};
static const char *cover_ext[4] = {".jpg", ".png", ".PNG", ".JPG"};
static const char *prefix[2] = {"", "PS3/"};

static uint32_t sections[MAX_SECTIONS], sections_size[MAX_SECTIONS];
static ntfs_md *mounts;
static int mountCount;

#include "ff.h"
#include "fflib.h"

#include "iso.h"
#include "file.h"
#include "firmware.h"
#include "fix_game.h"
#include "net.h"
#include "fake_iso.h"

//----------------

static uint64_t device_id;
static TrackDef tracks[100];
static ScsiTrackDescriptor *scsi_tracks;
static uint32_t num_tracks;

static uint8_t g_profile;
static uint8_t g_mode;

static rawseciso_args *p_args;
static int cd_sector_size, cd_sector_size_param;

#include "prep.h"
#include "exfat.h"

int main(int argc, const char* argv[])
{
	detect_firmware();

	int i, parts, slen = 0;
	int ext_len = 4;

	sysFSDirent dir;
	DIR_ITER *pdir = NULL, *psubdir= NULL;
	struct stat st;

	sysLv2FsUnlink((char*)"/dev_hdd0/tmp/wmtmp/games.html");

	int fd = -1;
	u64 read = 0;
	char path0[MAX_PATH_LEN], subpath[MAX_PATH_LEN];
	char direntry[MAX_PATH_LEN];
	char filename[MAX_PATH_LEN];
	bool has_dirs, is_iso = false;
	char *ext;
	u16 flen;

	bool mmCM_found = false; char mmCM_cache[64], mmCM_path[64], titleID[16];

	// detect if multiMAN is installed
	sprintf(mmCM_cache, "%s", "/dev_hdd0/game/BLES80608/USRDIR/cache");
	if(file_exists((char*)"/dev_hdd0/game/BLES80608/USRDIR/EBOOT.BIN")) {mmCM_found = true; mkdir(mmCM_cache, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);}
	else
	{sprintf(mmCM_cache, "%s", "/dev_hdd0/game/NPEA00374/USRDIR/cache");
	if(file_exists((char*)"/dev_hdd0/game/NPEA00374/USRDIR/EBOOT.BIN")) {mmCM_found = true; mkdir(mmCM_cache, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);}
	else
	{sprintf(mmCM_cache, "%s", "/dev_hdd0/tmp/game_repo/main/cache");
	if(file_exists((char*)"/dev_hdd0/tmp/game_repo/main/EBOOT.BIN"))    {mmCM_found = true; mkdir(mmCM_cache, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);}
	}
	}

	// create cache folder
	snprintf(path, sizeof(path), "/dev_hdd0/tmp/wmtmp");
	mkdir(path, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
	sysFsChmod(path, S_IFDIR | 0777);

//--- hold CROSS to keep previous cached files
	unsigned button = 0;

	padInfo padinfo;
	padData paddata;

	ioPadInit(7);

	for(u8 r = 0; r < 10; r++)
	{
		ioPadGetInfo(&padinfo);
		for(u8 n = 0; n < 7; n++)
		{
			if(padinfo.status[n])
			{
				ioPadGetData(n, &paddata);
				button = (paddata.button[2] << 8) | (paddata.button[3] & 0xff);
				if (paddata.len > 0) break;
			}
		}
		if(button) break; else usleep(20000);
	}
	ioPadEnd();

	if(button) ; // skip removal of .ntfs[ files
	else
//---

	{
		sysLv2FsOpenDir(path, &fd);
		if(fd >= 0)
		{
			while(!sysLv2FsReadDir(fd, &dir, &read) && read)
				if(strstr(dir.d_name, ".ntfs[") || (dir.d_namlen > 4 && strstr(dir.d_name + dir.d_namlen - 4, ".iso"))) {sprintf(path0, "%s/%s", path, dir.d_name); sysLv2FsUnlink(path0);}
			sysLv2FsCloseDir(fd);
		}
	}

	cobra_lib_init();

	int refresh_xml = connect_to_webman();
	if(refresh_xml >= 0) ssend(refresh_xml, "GET /mount.ps3/unmount HTTP/1.0\r\n");

	scan_exfat();

	mountCount = ntfsMountAll(&mounts, NTFS_DEFAULT | NTFS_RECOVER /* | NTFS_READ_ONLY */ );
	if (mountCount <= 0) goto exit;

	for (i = 0; i < mountCount; i++)
	{
		device_id = USB_MASS_STORAGE((mounts[i].interface->ioType & 0xff) - '0');
		if (strncmp(mounts[i].name, "ntfs", 4) == 0 || strncmp(mounts[i].name, "ext", 3) == 0)
		{
			for (u8 p = 0; p < 2; p++)
			{
				for (u8 profile = 0; profile < 6; profile++)
				{
					for(u8 m = 0; m < MAX_MODES; m++) //0="PS3ISO", 1="BDISO", 2="DVDISO", 3="PSXISO", 4="VIDEO", 5="MOVIES", 6="PKG", 7="Packages", 8="packages", 9="BDFILE", 10="PS2ISO", 11="PSPISO", 12="MUSIC", 13="THEME", 14="UPDATE", 15="ROMS"
					{
						has_dirs = false;

						snprintf(path, sizeof(path), "%s:/%s%s%s", mounts[i].name, prefix[p], c_path[m], SUFIX(profile));

						pdir = ps3ntfs_diropen(path);
						if(pdir != NULL)
						{
							while(ps3ntfs_dirnext(pdir, dir.d_name, &st) == 0)
							{
								flen = sprintf(filename, "%s", dir.d_name);

								ext_len = 4;
								if(flen < ext_len) continue; ext = filename + flen - ext_len;

								//--- create .ntfs[BDFILES] for 4="VIDEO", 5="MOVIES", 6="PKG", 7="Packages", 8="packages", 9="BDFILE", 10="PS2ISO", 11="PSPISO"
								if(m >= 4)
								{
									flen = snprintf(filename, sizeof(filename), "%s:/%s%s%s/%s", mounts[i].name, prefix[p], c_path[m], SUFIX(profile), dir.d_name);

									ext = filename + flen - ext_len;

									if((m == PSPISO) && (strcasestr(ext, ".iso") != NULL))
									{
										sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/[PSPISO] %s.PNG", dir.d_name);
										ExtractFileFromISO(filename, "/PSP_GAME/ICON0.PNG", wm_path);

										sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/[PSPISO] %s.PIC1.PNG", dir.d_name);
										ExtractFileFromISO(filename, "/PSP_GAME/PIC1.PNG", wm_path);

										sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/[PSPISO] %s.PIC0.PNG", dir.d_name);
										ExtractFileFromISO(filename, "/PSP_GAME/PIC0.PNG", wm_path);

										sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/[PSPISO] %s.SND0.AT3", dir.d_name);
										ExtractFileFromISO(filename, "/PSP_GAME/SND0.AT3", wm_path);

										sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/[PSPISO] %s.ICON1.PAM", dir.d_name);
										ExtractFileFromISO(filename, "/PSP_GAME/ICON1.PAM", wm_path);

										sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/[PSPISO] %s.EBOOT.BIN", dir.d_name);
										ExtractFileFromISO(filename, "/PSP_GAME/SYSDIR/EBOOT.BIN", wm_path);

										sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/[PSPISO] %s.EBOOT.OLD", dir.d_name);
										ExtractFileFromISO(filename, "/PSP_GAME/SYSDIR/EBOOT.OLD", wm_path);
									}

									make_fake_iso(m, ext, dir.d_name, filename, device_id, get_filesize(filename));
									continue;
								}
								//---------------

								//--- is ISO?
								is_iso =	( (strcasestr(ext, ".iso")) ) ||
								(m > 0 && ( ( (strcasestr(ext, ".bin")) ) ||
											( (strcasestr(ext, ".img")) ) ||
											( (strcasestr(ext, ".mdf")) ) ));

								if(!is_iso) {ext_len = 6; is_iso = (flen >= ext_len && strcasestr(filename + flen - ext_len, ".iso.0"));}

////////////////////////////////////////////////////////
								sprintf(direntry, "%s", dir.d_name);

								//--- is SUBFOLDER?
								if(!is_iso)
								{
									sprintf(subpath, "%s:/%s%s%s/%s", mounts[i].name, prefix[p], c_path[m], SUFIX(profile), dir.d_name);
									psubdir = ps3ntfs_diropen(subpath);
									if(psubdir == NULL) continue;
									slen = sprintf(subpath, "%s", filename); has_dirs = true;
	next_ntfs_entry:
									if(ps3ntfs_dirnext(psubdir, dir.d_name, &st) < 0) {ps3ntfs_dirclose(psubdir); has_dirs = false; continue;}
									if(dir.d_name[0] == '.') goto next_ntfs_entry;

									sprintf(direntry, "%s/%s", subpath, dir.d_name);

									if(strncmp(subpath, dir.d_name, slen))
									{
										flen = sprintf(filename, "[%s] %s", subpath, dir.d_name);
										for(int c = 1; c <= slen; c++)
											if(filename[c] == '[') filename[c] = '('; else
											if(filename[c] == ']') filename[c] = ')';
									}
									else
										flen = sprintf(filename, "%s", dir.d_name);

									ext_len = 4;
									if(flen < ext_len) goto next_ntfs_entry; ext = filename + flen - ext_len;

									is_iso =	( (strcasestr(ext, ".iso")) ) ||
									  (m>0 && ( ( (strcasestr(ext, ".bin")) ) ||
												( (strcasestr(ext, ".img")) ) ||
												( (strcasestr(ext, ".mdf")) ) ));

									if(!is_iso) {ext_len = 6; is_iso = (flen >= ext_len && strcasestr(filename + flen - ext_len, ".iso.0"));}
								}
	////////////////////////////////////////////////////////

								//--- cache ISO
								if( is_iso )
								{
									size_t path_len;
									filename[flen - ext_len] = '\0';
									path_len = snprintf(path, sizeof(path), "%s:/%s%s%s/%s", mounts[i].name, prefix[p], c_path[m], SUFIX(profile), direntry);

									//--- PS3ISO: fix game, cache SFO, ICON0 and PIC1 (if mmCM is installed)
									if(m == PS3ISO)
									{
										titleID[0] = '\0';

										sprintf(wm_path, "%s:/%s%s%s/%s.SFO", mounts[i].name, prefix[p], c_path[m], SUFIX(profile), filename);
										if(not_exists(wm_path))
											ExtractFileFromISO(path, "/PS3_GAME/PARAM.SFO;1", wm_path);

										sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/%s.SFO", filename);
										if(not_exists(wm_path))
											ExtractFileFromISO(path, "/PS3_GAME/PARAM.SFO;1", wm_path);

										if(c_firmware < FW_VERSION && need_fix(wm_path))
										{
											fix_iso(path, 0x100000UL);

											// refresh PARAM.SFO
											sysLv2FsUnlink(wm_path);
											ExtractFileFromISO(path, "/PS3_GAME/PARAM.SFO;1", wm_path);
										}

										if(mmCM_found && file_exists(wm_path))
										{
											get_titleid(wm_path, titleID);
											sprintf(mmCM_path, "%s/%s.SFO", mmCM_cache, titleID);
											if(not_exists(mmCM_path))
												sysLv2FsLink(wm_path, mmCM_path);
										}

										int plen = sprintf(image_file, "%s", path) - ext_len;

										u8 e;
										for(e = 0; e < 4; e++)
										{
											sprintf(image_file + plen, "%s", cover_ext[e]);
											if(file_exists(image_file))
											{
												sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/%s%s", filename, cover_ext[e]);
												if(not_exists(wm_path))
													copy_file(image_file, wm_path);
												break;
											}
										}

										if(e >= 4)
										{
											sprintf(wm_path, "%s:/%s%s%s/%s.PNG", mounts[i].name, prefix[p], c_path[m], SUFIX(profile), filename);
											if(not_exists(wm_path))
												ExtractFileFromISO(path, "/PS3_GAME/ICON0.PNG;1", wm_path);

											sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/%s.PNG", filename);
											if(not_exists(wm_path))
												ExtractFileFromISO(path, "/PS3_GAME/ICON0.PNG;1", wm_path);
										}

										if(mmCM_found && titleID[0]>' ' && file_exists(wm_path))
										{
											sprintf(mmCM_path, "%s/%s_320.PNG", mmCM_cache, titleID);
											if(not_exists(mmCM_path))
												sysLv2FsLink(wm_path, mmCM_path);

											sprintf(mmCM_path, "%s/%s_1920.PNG", mmCM_cache, titleID);
											if(not_exists(mmCM_path))
												ExtractFileFromISO(path, "/PS3_GAME/PIC1.PNG;1", mmCM_path);
										}
									}
									else
									{
										// cache cover image for BDISO, DVDISO, PSXISO
										int plen = sprintf(image_file, "%s", path) - ext_len;

										u8 e;
										for(e = 0; e < 4; e++)
										{
											sprintf(image_file + plen, "%s", cover_ext[e]);
											if(file_exists(image_file))
											{
												sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/%s%s", filename, cover_ext[e]);
												if(not_exists(wm_path))
													copy_file(image_file, wm_path);
												break;
											}
										}
										if(e >= 4)
										{
											sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/%s%s", filename, ".png");
											if(m == BDISO)  copy_file("/dev_hdd0/game/BLES80616/USRDIR/icons/bdiso.png", wm_path);
											if(m == DVDISO) copy_file("/dev_hdd0/game/BLES80616/USRDIR/icons/dvdiso.png", wm_path);
										}
									}

									// get file sectors
									parts = ps3ntfs_file_to_sectors(path, sections, sections_size, MAX_SECTIONS, 1);

									// get multi-part file sectors
									if(ext_len == 6)
									{
										char iso_name[MAX_PATH_LEN], iso_path[MAX_PATH_LEN];

										size_t nlen = sprintf(iso_name, "%s", path);
										iso_name[nlen - 1] = '\0';

										for (u8 o = 1; o < 64; o++)
										{
											if(parts >= MAX_SECTIONS) break;

											sprintf(iso_path, "%s%i", iso_name, o);
											if(not_exists(iso_path)) break;

											parts += ps3ntfs_file_to_sectors(iso_path, sections + parts, sections_size + parts, MAX_SECTIONS - parts, 1);
										}
									}

									if (parts >= MAX_SECTIONS) continue;

									if (parts > 0)
									{
										cd_sector_size = 2352;
										cd_sector_size_param = 0;

										num_tracks = 1;
										if(m == PSXISO)
										{
											int fd;

											// detect CD sector size
											fd = ps3ntfs_open(path, O_RDONLY, 0);
											if(fd >= 0)
											{
												char buffer[20];
												u16 sec_size[7] = {2352, 2048, 2336, 2448, 2328, 2340, 2368};
												for(u8 n = 0; n < 7; n++)
												{
													ps3ntfs_seek64(fd, ((sec_size[n]<<4) + 0x18), SEEK_SET);
													ps3ntfs_read(fd, (void *)buffer, 20);
													if(  (memcmp(buffer + 8, "PLAYSTATION ", 0xC) == 0) ||
														((memcmp(buffer + 1, "CD001", 5) == 0) && buffer[0] == 0x01) ) {cd_sector_size = sec_size[n]; break;}
												}
												ps3ntfs_close(fd);

												if(cd_sector_size & 0xf) cd_sector_size_param = cd_sector_size<<8;
												else if(cd_sector_size != 2352) cd_sector_size_param = cd_sector_size<<4;
											}

											// parse CUE file
											const char *cue_ext[4] = {".cue", ".ccd", ".CUE", ".CCD"};
											for(u8 e = 0; e < 4; e++)
											{
												strcpy(path + path_len - 4, cue_ext[e]);
												fd = ps3ntfs_open(path, O_RDONLY, 0);
												if(fd >= SUCCESS) break;
											}

											if (fd >= SUCCESS)
											{
												int r = ps3ntfs_read(fd, (char *)cue_buf, sizeof(cue_buf));
												ps3ntfs_close(fd);

												if (r > 0)
												{
													char templn[MAX_LINE_LEN];
													num_tracks = parse_cue(templn, (char *)cue_buf, r, tracks);
												}
											}
										}

										//--- build .ntfs[ file
										build_file(filename, parts, num_tracks, device_id, profile, m);
									} // if (parts > 0)
								} // if( is_iso )
	//////////////////////////////////////////////////////////////
								if(has_dirs) goto next_ntfs_entry;
	//////////////////////////////////////////////////////////////
							} // while(ps3ntfs_dirnext(pdir, dir.d_name, &st) == 0)
							ps3ntfs_dirclose(pdir);
						} // if(pdir != NULL)
					} // for(u8 m = 0; m < MAX_MODES; m++)
				} // for (u8 profile = 0; profile < 6; profile++)
			} // for (u8 p = 0; p < 2; p++)
		} // if (strncmp(mounts[i].name, "ntfs", 4) == 0 || strncmp(mounts[i].name, "ext", 3) == 0)
	} // for (i = 0; i < mountCount; i++)

exit:
	cobra_lib_finalize();

	//--- Unmount ntfs devices
	for (u8 u = 0; u < mountCount; u++) ntfsUnmount(mounts[u].name, 1);

	//--- Force refresh xml (webMAN)
	refresh_xml = connect_to_webman();
	if(refresh_xml >= 0) ssend(refresh_xml, "GET /refresh.ps3 HTTP/1.0\r\n");

	//--- Launch RELOAD.SELF
	char *self_path = path; memset(self_path, 0, MAX_PATH_LEN);
	if(argc > 0 && argv)
	{
		if(!strncmp(argv[0], "/dev_hdd0/game/", 15))
		{
			int n;

			strcpy(self_path, argv[0]);

			n = 15; while(self_path[n] != '/' && self_path[n] != 0) n++;

			if(self_path[n] == '/') self_path[n] = 0;

			strcat(self_path, "/USRDIR/RELOAD.SELF");
		}
	}

	if(file_exists(self_path))
		sysProcessExitSpawn2(self_path, NULL, NULL, NULL, 0, 1001, SYS_PROCESS_SPAWN_STACK_SIZE_1M);

	return 0;
}
