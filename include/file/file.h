#define SAVE_ALL			0
#define APPEND_TEXT			(-0xADD0ADD0ADD000ALL)
#define DONT_CLEAR_DATA		-1
#define RECURSIVE_DELETE	2 // don't check for ADMIN/USER

#define create_file(file)	save_file(file, NULL, SAVE_ALL)
#define is_same_dev(a, b)	(!strncmp(a, b, 12) || ((a[9] == '/') && !strncmp(a, b, 10)))

#include "md5.h"
#include "hdd_unlock_space.h"

//////////////////////////////////////////////////////////////
#include "file_ntfs.h"
#include "file_devs.h"
#include "file_size.h"
#include "file_copy.h"
#include "file_delete.h"

#include "togglers.h"
static bool isDir(const char *path)
{
	if(!path) return false;  // sanity check

#ifdef USE_NTFS
	if(is_ntfs_path(path))
	{
		struct stat bufn;
		char *npath = (char*)ntfs_path(path);
		return ((ps3ntfs_stat(npath, &bufn) >= 0) && (bufn.st_mode & S_IFDIR));
	}
#endif

	struct CellFsStat s;
	if(cellFsStat(path, &s) == CELL_FS_SUCCEEDED)
		return ((s.st_mode & CELL_FS_S_IFDIR) != 0);
	else
		return false;
}

static bool file_exists(const char *path)
{
	return (file_ssize(path) >= 0);
}

static bool is_app_dir(const char *path, const char *app_name)
{
	char eboot[STD_PATH_LEN];
	snprintf(eboot, STD_PATH_LEN, "%s/%s%s", path, app_name, "/USRDIR/EBOOT.BIN");
	return file_exists(eboot);
}

#ifdef COBRA_ONLY
static bool is_iso_0(const char *filename)
{
	#ifdef MOUNT_PNG
	if(!extcasecmp(filename, ".0.PNG", 6)) return true;
	#endif
	return !extcasecmp(filename, ".iso.0", 6);
}
#endif

static const char *get_ext(const char *path)
{
	if(!path) return path;

	const int len = strlen(path); if(len <= 1) return path + len;
	const char *ext = path + len - 1;
	const int limit = (*ext == ']') ? 13 : 6; // search limit 6 or 13 chars
	const char *end = (len <= limit) ? path : ext - limit;
	if(*ext == '0') ext--;
	while(--ext > end)
		if((*ext == '.') || (*ext == '/')) break;
	if(*ext != '.') ext = path + len;
	return ext;
}

static char *get_filename(const char *path)
{
	if(!path) return NULL; // sanity check

	char *filename = strrchr(path, '/'); if(!filename) return (char *)path;
	return filename; // return with slash
}

static u16 normalized_len(const char *path)
{
	u16 flen = strlen(path);
	while((flen > 1) && (path[flen - 1] == '/')) flen--;
	return flen;
}

static void normalize_path(char *path, bool slash)
{
	if(!path) return; // sanity check

	u16 flen = normalized_len(path); path[flen] = '\0';
	if(!(*path) || (slash && (flen > 1) && (path[flen] != '/'))) strcat(path + flen, "/");
}

static char *remove_filename(const char *path)
{
	char *p = get_filename(path); if(p) *p = NULL; else p = (char*)path;
	return p;
}

static int remove_ext(const char *path)
{
	char *p = (char*)get_ext(path);
	if(p)
	{
		bool is_enc = IS(p, ".ENC");
		if (*p == '.')
			*p = '\0';
		if(is_enc) remove_ext(path);
	}
	return strlen(path);
}

#ifndef LITE_EDITION
static char *copy_path(char *path, const char *full_path)
{
	if(!path || !full_path) return NULL;  // sanity check

	strcpy(path, full_path);
	return remove_filename(path);
}
#endif

#ifdef COBRA_ONLY
static bool change_ext(char *filename, int num_ext, const char *file_ext[])
{
	if(!filename || !file_ext) return false;  // sanity check

	char *ext = (char*)get_ext(filename);
	for(u8 e = 0; e < num_ext; e++)
	{
		strcpy(ext, file_ext[e]);
		if(file_exists(filename)) return true;
	}
	return false;
}

static void change_cue2iso(char *cue_file)
{
	if(!cue_file) return; // sanity check

	if(is_ext(cue_file, ".cue") || is_ext(cue_file, ".ccd"))
	{
		change_ext(cue_file, 11, iso_ext);
	}
}

#define check_ps3_game(path)
#else
static void check_ps3_game(char *path)
{
	if(!path) return; // sanity check

	char *p = strstr(path, "/PS3_GAME");
	if(p)
	{
		p[6] = 'M', p[7] = '0', p[8] = '1';  // PS3_GM01
		if(file_exists(path)) return;
		p[6] = 'A', p[7] = 'M', p[8] = 'E';  // PS3_GAME
	}
}
#endif

static void check_path_alias(char *param)
{
	if(!param) return; // sanity check

	if(islike(param, "/dev_blind")) enable_dev_blind(NULL);

	if(islike(param, "/dev_usbxxx"))
	{
		// find first USB device mounted
		char dev_usb[14];
		for(u8 n = 0; n < 128; n++)
		{
			sprintf(dev_usb, "/dev_usb%03i", n); if(isDir(dev_usb)) {memcpy(param, dev_usb, 11); break;}
		}
	}

	if(not_exists(param))
	{
		check_path_tags(param);

		#ifdef USE_NTFS
		if(is_ntfs_path(param)) return;
		#endif

		if(!islike(param, "/dev_") && !islike(param, "/net"))
		{
			if(strstr(param, ".ps3")) return;

			char path[STD_PATH_LEN];
			int len = strncopy(path, STD_PATH_LEN, (*param == '/') ? param + 1 : param);
			char *wildcard = strchr(path, '*'); if(wildcard) *wildcard = 0;
			if((len == 4) && path[3] == '/') path[3] = 0; // normalize path
			if(IS(path, "pkg")) {strcopy(param, DEFAULT_PKG_PATH);} else
			if(IS(path, "xml")) {*path = 0;} else // use HTML_BASE_PATH
			if(IS(path, "xmb")) {enable_dev_blind(NULL); strcopy(param, "/dev_blind/vsh/resource/explore/xmb");} else
			if(IS(path, "res")) {enable_dev_blind(NULL); strcopy(param, "/dev_blind/vsh/resource");} else
			if(IS(path, "mod")) {enable_dev_blind(NULL); strcopy(param, "/dev_blind/vsh/module");} else
			if(IS(path, "cov")) {concat2(param, MM_ROOT_STD, "/covers");} else
			if(IS(path, "cvr")) {concat2(param, MM_ROOT_STD, "/covers_retro/psx");} else
			if(islike(path, "res/"))  {concat2(param, "/dev_blind/vsh/resource/", path + 4);} else
			if(isDir(html_base_path)) {snprintf(param, HTML_RECV_LAST, "%s/%s", html_base_path, path);} // use html path (if path is omitted)

			if(not_exists(param)) {snprintf(param, HTML_RECV_LAST, "%s/%s", HTML_BASE_PATH, path);} // try HTML_BASE_PATH
			if(not_exists(param)) {snprintf(param, HTML_RECV_LAST, "%s/%s", webman_config->home_url, path);} // try webman_config->home_url

			if(not_exists(param))
			{
				snprintf(param, HTML_RECV_LAST, "%s/%s/%s", TMP_DIR, "wm_alias", path);
				if(file_exists(param)) {read_file(param, param, HTML_RECV_LAST, 0); return;}
			}
			if(not_exists(param))
			{
				const char *ext = get_ext(param);
				const char *dir = IS(ext, ".pkg") ? DEFAULT_PKG_PATH : HDD0_GAME_DIR;
				snprintf(param, HTML_RECV_LAST, "%s%s", dir, path); // try /dev_hdd0/game
				if(not_exists(param)) {snprintf(param, HTML_RECV_LAST, "%s%s", _HDD0_GAME_DIR, path);} // try /dev_hdd0//game
			}
			if(not_exists(param))
			{
				for(u8 i = 0; i < (MAX_DRIVES + 1); i++)
				{
					if(i == NET) i = NTFS + 1;
					snprintf(param, HTML_RECV_LAST, "%s/%s", drives[i], path); // try hdd0, usb0, usb1, etc.
					if(file_exists(param)) break;
				}
			}
			if(not_exists(param)) {snprintf(param, HTML_RECV_LAST, "%s/%s", "/dev_hdd0/tmp", path);} // try hdd0
			if(not_exists(param)) {snprintf(param, HTML_RECV_LAST, "%s/%s", HDD0_HOME_DIR, path);} // try /dev_hdd0/home
			if(not_exists(param)) {snprintf(param, HTML_RECV_LAST, "%s/%08i/%s", HDD0_HOME_DIR, xusers()->GetCurrentUserNumber(), path);} // try /dev_hdd0/home
			if(wildcard) {*wildcard = '*'; strcat(param, wildcard);} else if(not_exists(param)) strcpy(param, path);
		}
	}
}

static bool is_empty_dir(const char *path)
{
	return folder_count(path, 1) == 0;
}

#if defined(COPY_PS3) || defined(PKG_HANDLER) || defined(MOUNT_GAMEI)
static void mkdir_tree(const char *full_path)
{
	if(!full_path) return; // sanity check

	char *path = (char *)full_path;
	size_t path_len = strlen(path);
#ifdef USE_NTFS
	if(is_ntfs_path(path))
	{
		for(u16 p = 7; p < path_len; p++)
			if(path[p] == '/') {path[p] = '\0'; ps3ntfs_mkdir(ntfs_path(path), DMODE); path[p] = '/';}
	}
	else
#endif
	{
		for(u16 p = 12; p < path_len; p++)
			if(path[p] == '/') {path[p] = '\0'; cellFsMkdir(path, DMODE); path[p] = '/';}
	}
}
#endif

static void mkdirs(char *param)
{
	if(!param) return; // sanity check

	cellFsMkdir(TMP_DIR, DMODE);
	cellFsMkdir(WMTMP, DMODE);
	cellFsMkdir("/dev_hdd0/exdata", DMODE);
	cellFsMkdir(HDD_PACKAGES_PATH, DMODE);
	//cellFsMkdir("/dev_hdd0/GAMES",  DMODE); //paths[0]
	//cellFsMkdir("/dev_hdd0/PS3ISO", DMODE); //paths[2]
	//cellFsMkdir("/dev_hdd0/BDISO",  DMODE); //paths[3]
	//cellFsMkdir("/dev_hdd0/DVDISO", DMODE); //paths[4]
	//cellFsMkdir("/dev_hdd0/PS2ISO", DMODE); //paths[5]
	//cellFsMkdir("/dev_hdd0/PSXISO", DMODE); //paths[6]
	//cellFsMkdir("/dev_hdd0/PSPISO", DMODE); //paths[8]
	#ifdef MOUNT_ROMS
	if(webman_config->roms) cellFsMkdir("/dev_hdd0/ROMS", DMODE);
	#endif


	const u8 dtype[9] = {PS3, 0, PS3, BLU, DVD, PS2, PS1, 0, PSP};

	for(u8 i = 0; i < sizeof(dtype); i++)
	{
		if(!dtype[i]) continue; // skip /GAMEZ & /PSXGAMES
		concat_path(param, drives[0], paths[i]);
		if(webman_config->cmask & dtype[i])
			cellFsRmdir(param);
		else
			cellFsMkdir(param, DMODE);
	}

	param[9] = '\0'; // <- return /dev_hdd0
}

#ifndef LITE_EDITION
static void rename_file(char *source, char *dest)
{
	if(!source || !dest) return; // sanity check

	normalize_path(source, false);
	if(not_exists(source)) return;
	normalize_path(dest, false);

#ifdef USE_NTFS
	if(is_ntfs_path(source) || is_ntfs_path(dest))
	{
		ps3ntfs_unlink(ntfs_path(dest));
		ps3ntfs_rename(ntfs_path(source), ntfs_path(dest));
	}
	else
#endif
	{
		cellFsRmdir(dest);
		cellFsUnlink(dest);
		cellFsRename(source, dest);
	}
}
#endif

size_t read_file(const char *file, char *data, const size_t size, s32 offset)
{
	if(!file || !data) return 0; // sanity check

	int fd = 0; u64 read_e = 0;

	if(offset < 0) offset = 0; else _memset(data, size);

#ifdef USE_NTFS
	if(is_ntfs_path(file))
	{
		if(mountCount <= NTFS_UNMOUNTED) mount_all_ntfs_volumes();
		fd = ps3ntfs_open(ntfs_path(file), O_RDONLY, 0);
		if(fd >= 0)
		{
			ps3ntfs_seek64(fd, offset, SEEK_SET);
			read_e = ps3ntfs_read(fd, (void *)data, size);
			ps3ntfs_close(fd);
		}
		return read_e;
	}
#endif

	if(cellFsOpen(file, CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		if(cellFsReadWithOffset(fd, offset, (void *)data, size, &read_e) != CELL_FS_SUCCEEDED) read_e = 0;
		cellFsClose(fd);
	}
#ifdef FIX_CLOCK
	else if(islike(file, "http://") && ((int)size > (fd = strlen(file))))
	{
		char *url = data + fd + 1; strcpy(url, file);
		read_e = get_server_data(url, data, size);
	}
#endif
	return read_e;
}

static u16 read_sfo(const char *file, char *data)
{
	size_t size = file_ssize(file); if(size > _4KB_) size = _4KB_;
	return (u16)read_file(file, data, size, 0);
}

static int write_file(const char *file, int flags, const char *data, u64 offset, int size, bool crlf)
{
	if(!file || !data) return FAILED; // sanity check

	int fd = 0;

#ifdef USE_NTFS
	if(is_ntfs_path(file))
	{
		int nflags = O_CREAT | O_WRONLY;
		if(flags & CELL_FS_O_APPEND) nflags |= O_APPEND;
		if(flags & CELL_FS_O_TRUNC)  nflags |= O_TRUNC;

		if(mountCount <= NTFS_UNMOUNTED) mount_all_ntfs_volumes();
		fd = ps3ntfs_open(ntfs_path(file), nflags, MODE);
		if(fd >= 0)
		{
			if(offset) ps3ntfs_seek64(fd, offset, SEEK_SET);
			if((size <= SAVE_ALL) && data) size = strlen(data);
			if( size ) ps3ntfs_write(fd, data, size);
			if( crlf ) ps3ntfs_write(fd, (void *)"\r\n", 2);
			ps3ntfs_close(fd);
			return CELL_FS_SUCCEEDED;
		}
		return FAILED;
	}
#endif

	cellFsChmod(file, MODE); // set permissions for overwrite

	if(cellFsOpen(file, flags, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		if(offset) cellFsLseek(fd, offset, CELL_FS_SEEK_SET, &offset);
		if((size <= SAVE_ALL) && data) size = strlen(data);
		if( size ) cellFsWrite(fd, (void *)data, size, NULL);
		if( crlf ) cellFsWrite(fd, (void *)"\r\n", 2, NULL);
		cellFsClose(fd);

		cellFsChmod(file, MODE); // set permissions if created

		return CELL_FS_SUCCEEDED;
	}

	return FAILED;
}

static int patch_file(const char *file, const char *data, u64 offset, int size)
{
	return write_file(file, CELL_FS_O_WRONLY, data, offset, size, false);
}

int save_file(const char *file, const char *mem, s64 size)
{
	if(!file || !mem) return FAILED; // sanity check

	bool crlf = (size == APPEND_TEXT); // auto add new line

	int flags = CELL_FS_O_CREAT | CELL_FS_O_WRONLY | CELL_FS_O_TRUNC;
	if( size < 0 )  {flags = CELL_FS_O_CREAT | CELL_FS_O_WRONLY | CELL_FS_O_APPEND; size = crlf ? SAVE_ALL : -size;} else
	if(!extcmp(file, "/PARAM.SFO", 10)) flags = CELL_FS_O_WRONLY;
	return write_file(file, flags, mem, 0, (int)size, crlf);
}

#if 0
static void addlog(const char *msg1, const char *msg2, int i)
{
	char msg[200];
	snprintf(msg, sizeof(msg), "%i %s %s", i, msg1, msg2);
	save_file("/dev_hdd0/wmm.log", msg, APPEND_TEXT);
}
#endif

//////////////////////////////////////////////////////////////

static int wait_path(const char *path, u8 timeout, bool found)
{
	if(!path || (*path != '/')) return FAILED;

	timeout *= 8;
	for(u8 n = 0; n < timeout; n++)
	{
		if(file_exists(path) == found) return CELL_FS_SUCCEEDED;
		if(!working) break;
		sys_ppu_thread_usleep(125000);
	}
	return FAILED;
}

int wait_for(const char *path, u8 timeout)
{
	return wait_path(path, timeout, true);
}

#define MAX_WAIT	30

static bool not_xmb(void)
{
	return working && (View_Find("explore_plugin") == 0) && (View_Find("game_plugin") == 0) && (View_Find("sysconf_plugin") == 0);
}

static bool wait_for_xmb(void)
{
	u8 t = 0;
	while(working && (View_Find("explore_plugin") == 0)) {if(++t > MAX_WAIT) break; sys_ppu_thread_sleep(1);}
	return (t > MAX_WAIT); // true = timeout
}

#ifdef WM_REQUEST
static void wait_for_user(void)
{
	while(working && !USER_LOGGEDIN) sys_ppu_thread_sleep(3); wait_for_xmb();
}
#endif

static void check_reload(void)
{
	from_reboot = file_exists(WM_NOSCAN_FILE);
	cellFsUnlink(WM_NOSCAN_FILE); // delete wm_noscan file

	if(!from_reboot) sys_ppu_thread_sleep(3); // wait for the unload of the previous plugin

	char prx_id[20];
	read_file(WM_RELOAD_FILE, prx_id, 20, 0); // read the prx_id of the other plugin
	cellFsUnlink(WM_RELOAD_FILE); // delete semaphore file

	sys_prx_id_t prx = val(prx_id);
	if(prx)
	{
		wm_reload = from_reboot = true;
		system_call_3(SC_UNLOAD_PRX_MODULE, prx, 0, NULL);
		sys_ppu_thread_sleep(3);
	}
}
