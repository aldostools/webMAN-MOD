#ifdef PKG_HANDLER

// /install_ps3<pkg-path>             Keeps pkg after installation
// /install.ps3<pkg-path>             Deletes pkg after installation
// /install.ps3<pkg-path>?            Installs only if it's a new game, othewise navigates to the installation folder
// /install.ps3?url=<url>             Downloads pkg to /dev_hdd0/packages & installs the pkg after download (pkg is deleted after installation)
// /install_ps3?url=<url>             Downloads pkg to /dev_hdd0/packages & installs the pkg after download (pkg is kept after installation)
// /install.ps3?to=<path>&url=<url>   Same as /download.ps3

// /download.ps3?url=<url>            Downloads file to /dev_hdd0/packages. pkg files are downloaded to /dev_hdd0/tmp/downloader then moved to /dev_hdd0/packages
// /download.ps3?to=<path>&url=<url>  Downloads file to <path>

#include <cell/http.h>

#include "../vsh/xmb_plugin.h"
#include "../vsh/game_ext_plugin.h"
#include "../vsh/download_plugin.h"
#include "../vsh/stdc.h"

#define MAX_URL_LEN    360
#define MAX_DLPATH_LEN 240

#define MAX_PKGPATH_LEN 240
static char pkg_path[MAX_PKGPATH_LEN];

static wchar_t pkg_durl[MAX_URL_LEN];
static wchar_t pkg_dpath[MAX_DLPATH_LEN];

static u8 pkg_dcount = 0;
static u8 pkg_auto_install = 0;
static bool pkg_delete_after_install = true;
static char install_path[64];
static time_t pkg_install_time = 0;

static bool install_in_progress = false;

#define INT_HDD_ROOT_PATH		"/dev_hdd0/"
#define DEFAULT_PKG_PATH		"/dev_hdd0/packages/"
#define TEMP_DOWNLOAD_PATH		"/dev_hdd0/tmp/downloader/"

#define PKG_MAGIC				0x7F504B47
#define XMM0					0x584d4d30

#define IS_INSTALLING		(View_Find("game_plugin") != 0)
#define IS_INSTALLING_NAS	(View_Find("nas_plugin")  != 0)
#define IS_DOWNLOADING		(View_Find("download_plugin") != 0)

typedef struct {
	u32 magic; // 0x7F504B47 //
	u32 version;
	u16 sdk_type;
	u16 SCE_header_type;
	u32 meta_offset;
	u64 size; // size of sce_hdr + sizeof meta_hdr
	u64 pkg_size;
	u64 unk1;
	u64 data_size;
	char publisher[6]; // ??
	char sep;
	char title_id[9];
} _pkg_header;

static u64 get_pkg_size_and_install_time(char *pkgfile)
{
	_pkg_header pkg_header;

	int fd; *install_path = pkg_header.pkg_size = pkg_install_time = 0;

	if(cellFsOpen(pkgfile, CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		if(cellFsRead(fd, (void *)&pkg_header, sizeof(pkg_header), NULL) == CELL_FS_SUCCEEDED && pkg_header.magic == PKG_MAGIC)
		{
			sprintf(install_path, "%s%s%s", HDD0_GAME_DIR, pkg_header.title_id, "/PS3LOGO.DAT");

			struct CellFsStat s;
			if(cellFsStat(install_path, &s) == CELL_FS_SUCCEEDED) pkg_install_time = s.st_mtime; // prevents pkg deletion if user cancels install

			install_path[24] = NULL;
		}
		cellFsClose(fd);
	}

	return pkg_header.pkg_size; // also returns module variableS: pkg_install_time & install_path
}

static void wait_for_xml_download(char *filename, char *param)
{
	char *xml = strstr(filename, ".xm!");
	if(xml)
	{
		xml = strstr(filename, "~");

		struct CellFsStat s; u64 size = 475000; if(xml) size = val(xml + 1); else xml = strstr(filename, ".xm!");
/*
		while(IS_DOWNLOADING)
		{
			sys_timer_sleep(1);
		}
*/
		for(u8 retry = 0; retry < 15; retry++)
			{if(!working || (cellFsStat(filename, &s) == CELL_FS_SUCCEEDED && s.st_size >= size)) break; sys_ppu_thread_sleep(2);}

		if(s.st_size < size) {cellFsUnlink(filename); return;}

		strcpy(param, filename); strcpy(xml, ".xml");
		cellFsUnlink(filename);
		cellFsRename(param, filename);

#ifdef VIRTUAL_PAD
		if(IS_DOWNLOADING) press_cancel_button();
#endif
	}
}

static void wait_for_pkg_install(void)
{
	sys_ppu_thread_sleep(5);

	while (working && IS_INSTALLING) sys_ppu_thread_sleep(2);
	while (working && IS_INSTALLING_NAS) sys_ppu_thread_sleep(2);

	time_t install_time = pkg_install_time;  // set time before install
	get_pkg_size_and_install_time(pkg_path); // get time after install

	if(working && pkg_delete_after_install && islike(pkg_path, INT_HDD_ROOT_PATH) && (strstr(pkg_path, "/GAME") == NULL))
	{
		if(pkg_install_time != install_time) cellFsUnlink(pkg_path);
	}

	if(do_restart && (pkg_install_time == install_time)) do_restart = false; // installation canceled
}

static int LoadPluginById(int id, void *handler)
{
	if(xmm0_interface == 0) // getting xmb_plugin xmm0 interface for loading plugin sprx
	{
		xmm0_interface = (xmb_plugin_xmm0 *)plugin_GetInterface(View_Find("xmb_plugin"), XMM0);
		if(xmm0_interface == 0) return FAILED;
	}
	return xmm0_interface->LoadPlugin3(id, handler, 0);
}

static int UnloadPluginById(int id, void *handler)
{
	if(xmm0_interface == 0) // getting xmb_plugin xmm0 interface for loading plugin sprx
	{
		xmm0_interface = (xmb_plugin_xmm0 *)plugin_GetInterface(View_Find("xmb_plugin"), XMM0);
		if(xmm0_interface == 0) return FAILED;
	}
	return xmm0_interface->Shutdown(id, handler, 1);
}

static void unloadSysPluginCallback(void)
{
	//Add potential callback process
	//show_msg((char *)"plugin shutdown via xmb call launched");
}

static void unload_web_plugins(void)
{
	u8 retry = 0;

	while(View_Find("webrender_plugin"))
	{
		UnloadPluginById(0x1C, (void *)unloadSysPluginCallback);
		sys_ppu_thread_sleep(1); if(++retry > 10) break;
	}

	while(View_Find("webbrowser_plugin"))
	{
		UnloadPluginById(0x1B, (void *)unloadSysPluginCallback);
		sys_ppu_thread_sleep(1); if(++retry > 10) break;
	}

#ifdef VIRTUAL_PAD
	if(IS_ON_XMB)
	{
		press_cancel_button();
	}
#endif

	if(explore_interface == 0) 
	{
		explore_interface = (explore_plugin_interface *)plugin_GetInterface(View_Find("explore_plugin"), 1);
		if(explore_interface == 0) return;
	}
	explore_interface->ExecXMBcommand("close_all_list", 0, 0);
}

static void downloadPKG_thread(void)
{

	if(download_interface == 0) // test if download_interface is loaded for interface access
	{
		download_interface = (download_plugin_interface *)plugin_GetInterface(View_Find("download_plugin"), 1);
		if(download_interface == 0) return;
	}
	download_interface->DownloadURL(0, pkg_durl, pkg_dpath);
}

static void installPKG_thread(void)
{
	if(game_ext_interface == 0) // test if game_ext_plugin is loaded for interface access
	{
		game_ext_interface = (game_ext_plugin_interface *)plugin_GetInterface(View_Find("game_ext_plugin"), 1);
		if(game_ext_interface == 0) return;
	}

	game_ext_interface->LoadPage();

	if(!extcasecmp(pkg_path, ".p3t", 4))
		game_ext_interface->installTheme(pkg_path, (char*)"");
	else
		game_ext_interface->installPKG(pkg_path);
}

static int download_file(const char *param, char *msg)
{
	int ret = FAILED;

	s32 net_enabled = 0;
	xsetting_F48C0548()->GetSettingNet_enable(&net_enabled);

	if(!net_enabled)
	{
		sprintf(msg, "ERROR: %s", "network disabled");
		return ret;
	}

	if(IS_INGAME)
	{
		sprintf(msg, "ERROR: %s", "download from XMB");
		return ret;
	}

	char *msg_durl = msg;
	char *msg_dpath = msg + MAX_URL_LEN + 16;

	sprintf(msg_durl,  "ERROR: %s", "Invalid URL");
	sprintf(msg_dpath, "Download canceled");

	char pdurl[MAX_URL_LEN];
	char pdpath[MAX_DLPATH_LEN];

	wmemset(pkg_durl, 0, MAX_URL_LEN); // Use wmemset from stdc.h instead of reinitialising wchar_t with a loop.
	wmemset(pkg_dpath, 0, MAX_DLPATH_LEN);

	memset(pdurl, 0, MAX_URL_LEN);
	memset(pdpath, 0, MAX_DLPATH_LEN);

	int len;
	size_t conv_num = 0;

	if(islike(param, "?to="))  //Use of the optional parameter
	{
		len = get_param("&url=", pdurl, param, MAX_DLPATH_LEN); if(!islike(pdurl, "http")) goto end_download_process;
		conv_num = mbstowcs((wchar_t *)pkg_durl, (const char *)pdurl, len + 1);  //size_t stdc_FCAC2E8E(wchar_t *dest, const char *src, size_t max)

		len = get_param("?to=", pdpath, param, MAX_DLPATH_LEN); if(*pdpath != '/') goto end_download_process; 
		filepath_check(pdpath);
	}
	else if(islike(param, "?url="))
	{
		len = get_param("?url=", pdurl, param, MAX_DLPATH_LEN); if(!islike(pdurl, "http")) goto end_download_process;
		conv_num = mbstowcs((wchar_t *)pkg_durl,(const char *)pdurl, len + 1);  //size_t stdc_FCAC2E8E(wchar_t *dest, const char *src, size_t max)

		len = sprintf(pdpath, "%s", DEFAULT_PKG_PATH);
	}

	if(conv_num)
	{
		mkdir_tree(pdpath);

		if(isDir(pdpath) || (cellFsMkdir(pdpath, DMODE) == CELL_FS_SUCCEEDED)) ;

		else if(isDir(DEFAULT_PKG_PATH) || cellFsMkdir(DEFAULT_PKG_PATH, DMODE) == CELL_FS_SUCCEEDED)
		{
			len = sprintf(pdpath, DEFAULT_PKG_PATH);
		}
		else
		{
			len = sprintf(pdpath, INT_HDD_ROOT_PATH);
		}

		sprintf(msg_dpath, "To: %s", pdpath);
		if(IS(pdpath, DEFAULT_PKG_PATH) && (strstr(pdurl, ".pkg") != NULL))
		{
			len = sprintf(pdpath, TEMP_DOWNLOAD_PATH); pkg_dcount++;
		}

		conv_num = mbstowcs((wchar_t *)pkg_dpath, (const char *)pdpath, len + 1);

		if(conv_num)
		{
			unload_web_plugins();

			mkdir_tree(pdpath); cellFsMkdir(pdpath, DMODE);

			sprintf(msg_durl, "Downloading %s", pdurl);

			LoadPluginById(0x29, (void *)downloadPKG_thread);
			ret = CELL_OK;
		}
		else
			sprintf(msg_durl, "ERROR: %s", "Setting storage location");
	}

end_download_process:
	sprintf(msg, "%s\n%s", msg_durl, msg_dpath);
	return ret;
}

static int installPKG(const char *pkgpath, char *msg)
{
	int ret = FAILED;

	unload_web_plugins();

	if(IS_INGAME)
	{
		unload_web_plugins();

		if(IS_INGAME)
		{
			sprintf(msg, "ERROR: %s", "install from XMB");
			return ret;
		}
	}

	size_t pkg_path_len = strlen(pkgpath);

	if (pkg_path_len < MAX_PKGPATH_LEN)
	{
#ifdef COBRA_ONLY
		if(islike(pkgpath, "/net"))
		{
			cache_file_to_hdd((char*)pkgpath, pkg_path, "/tmp/downloader", msg); pkg_dcount++;
		}
		else
#endif
		if(*pkgpath == '?')
		{
			pkg_auto_install++;
			return download_file(pkgpath, msg);
		}
		else
			snprintf(pkg_path, MAX_PKGPATH_LEN, "%s", pkgpath);

		if( file_exists(pkg_path) )
		{
			if(!extcasecmp(pkg_path, ".pkg", 4) || !extcasecmp(pkg_path, ".p3t", 4)) //check if file has a .pkg extension or not and treat accordingly
			{
				unload_web_plugins();

				LoadPluginById(0x16, (void *)installPKG_thread);

				get_pkg_size_and_install_time(pkg_path); // set original pkg_install_time

				sprintf(msg, "%s%s\nTo: %s", "Installing ", pkg_path, install_path);

				ret = CELL_OK;
			}
		}
	}

	if(ret) sprintf(msg, "ERROR: %s", pkgpath);
	return ret;
}

static void installPKG_combo_thread(__attribute__((unused)) u64 arg)
{
	if(install_in_progress) return;

	install_in_progress = true;

	int fd, ret = FAILED;

	if(cellFsOpendir(DEFAULT_PKG_PATH, &fd) == CELL_FS_SUCCEEDED)
	{
		CellFsDirent dir; u64 read_e;

		pkg_delete_after_install = true;

		while(working && (cellFsReaddir(fd, &dir, &read_e) == CELL_FS_SUCCEEDED) && (read_e > 0))
		{
			if(!extcasecmp(dir.d_name, ".pkg", 4))
			{
				if(!webman_config->nobeep) { BEEP1 }

				char pkgfile[MAX_PKGPATH_LEN];
				sprintf(pkgfile, "%s%s", DEFAULT_PKG_PATH, dir.d_name);

				char msg[MAX_PATH_LEN];
				ret = installPKG(pkgfile, msg); show_msg(msg);
				if(ret == CELL_OK) wait_for_pkg_install();

				ret = CELL_OK; 
			}
		}
		cellFsClosedir(fd);

		if(ret == FAILED) { BEEP2 }
	}

	sys_ppu_thread_sleep(2);
	install_in_progress = false; 

	sys_ppu_thread_exit(0);
}

static void poll_downloaded_pkg_files(char *msg)
{
	if(pkg_dcount)
	{
		CellFsDirent entry; u64 read_e; int fd; u16 pkg_count = 0;

		if(cellFsOpendir(TEMP_DOWNLOAD_PATH, &fd) == CELL_FS_SUCCEEDED)
		{
			char *dlfile = msg;

			while(working && (cellFsReaddir(fd, &entry, &read_e) == CELL_FS_SUCCEEDED) && (read_e > 0))
			{
				if(!extcmp(entry.d_name, ".pkg", 4))
				{
					sprintf(dlfile, "%s%s", TEMP_DOWNLOAD_PATH, entry.d_name); pkg_count++;
					cellFsChmod(dlfile, MODE);

					u64 pkg_size = get_pkg_size_and_install_time(dlfile); if(pkg_size == 0) continue;

					struct CellFsStat s;
					if(cellFsStat(dlfile, &s) == CELL_FS_SUCCEEDED && pkg_size == s.st_size)
					{
						char pkgfile[MAX_PATH_LEN];
						u16 pkg_len = sprintf(pkgfile, "%s%s", DEFAULT_PKG_PATH, entry.d_name);
						for(u8 retry = 1; retry < 255; retry++)
						{
							if(cellFsRename(dlfile, pkgfile) == CELL_FS_SUCCEEDED) break;
							sprintf(pkgfile + pkg_len - 4, " (%i).pkg", retry);
						}

						if(pkg_dcount) pkg_dcount--;

						if(pkg_auto_install) {pkg_auto_install--; installPKG(pkgfile, msg); if(pkg_delete_after_install) wait_for_pkg_install();}
					}
				}
			}
			cellFsClosedir(fd);

			if(pkg_count == 0) pkg_auto_install = pkg_dcount = 0; // disable polling if no pkg files were found (e.g. changed to background download)
		}
	}
}
#endif // #ifdef PKG_HANDLER
