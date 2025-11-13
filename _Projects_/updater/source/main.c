#include <fcntl.h>
#include <ppu-lv2.h>
#include <sys/file.h>
#include <stdio.h>
#include <string.h>

#include <io/pad.h>

#include "gui.h"

#define SUCCESS 0
#define FAILED -1

#define SC_SYS_POWER 					(379)
#define SYS_REBOOT				 		0x8201

#define BUTTON_SQUARE        0x80
#define BUTTON_CROSS         0x40	/* install lite edition */
#define BUTTON_CIRCLE        0x20	/* install lite edition */
#define BUTTON_TRIANGLE      0x10
#define BUTTON_R1            0x08	/* install standard edition */
#define BUTTON_L1            0x04	/* install full edition */
#define BUTTON_R2            0x02	/* force update images */
#define BUTTON_L2            0x01	/* force update images */

#define CELL_FS_S_IFDIR      0040000	/* directory */
#define MODE                 0777
#define DMODE                (CELL_FS_S_IFDIR | MODE)

#define APP_DIR              "/dev_hdd0//game/UPDWEBMOD"
#define APP_USRDIR           APP_DIR "/USRDIR"

#define PS2CONFIG_DIR        "/dev_hdd0//game/PS2CONFIG"
#define PS2CONFIG_USRDIR     PS2CONFIG_DIR "/USRDIR"

#define HDDROOT_DIR          "/dev_hdd0"
#define PLUGINS_DIR          "/dev_hdd0/plugins"

#define HDD0ROOT_DIR         "/dev_hdd0/"

#define TMP_DIR              "/dev_hdd0/tmp"
#define LSD_DIR              TMP_DIR "/lsd"
#define LANG_DIR             TMP_DIR "/wm_lang"
#define COMBO_DIR            TMP_DIR "/wm_combo"
#define ICONS_DIR            TMP_DIR "/wm_icons"
#define RES_DIR              TMP_DIR "/wm_res"
#define SETUP_DIR            RES_DIR "/setup"

#define XMLHOST_DIR          "/dev_hdd0/xmlhost/game_plugin"

#define XMBMANPLS_DIR        "/dev_hdd0//game/XMBMANPLS"
#define XMBMANPLS_FEATS_DIR  XMBMANPLS_DIR "/USRDIR/FEATURES"
#define XMBMANPLS_IMAGES_DIR XMBMANPLS_DIR "/USRDIR/IMAGES"

#define RELOADXMB_DIR        "/dev_hdd0//game/RELOADXMB"
#define PKGLAUNCH_DIR        "/dev_hdd0//game/PKGLAUNCH"

#define IRISMAN_USRDIR       "/dev_hdd0//game/IRISMAN01/USRDIR"
#define PRXLOADER_USRDIR     "/dev_hdd0//game/PRXLOADER/USRDIR"

#define REBUG_DIR            "/dev_flash/rebug"
#define FLASH_VSH_MODULE_DIR "/dev_flash/vsh/module"
#define REBUG_VSH_MODULE_DIR "/dev_blind/vsh/module"

#define XMB_CATEGORY_GAME_XML "/vsh/resource/explore/xmb/category_game.xml"

bool full = false;
bool lite = false;
bool update_images = false;
bool full_on_nocobra = false;

bool plugins_dir_exists = false;

bool not_exists(const char *path)
{
	sysFSStat stat;
	return (sysLv2FsStat(path, &stat) != SUCCESS);
}

bool file_exists(const char *path)
{
	sysFSStat stat;
	return (sysLv2FsStat(path, &stat) == SUCCESS);
}

int is_ps3hen(void)
{
	lv2syscall1(8, 0x1337);
	return_to_user_prog(int);
}

int sys_fs_mount(char const* deviceName, char const* deviceFileSystem, char const* devicePath, int writeProt)
{
	lv2syscall8(837, (u64) deviceName, (u64) deviceFileSystem, (u64) devicePath, 0, (u64) writeProt, 0, 0, 0 );
	return_to_user_prog(int);
}

int file_copy(char* path, char* path2)
{
	int ret = 0;
	s32 fd = -1;
	s32 fd2 = -1;
	u64 length = 0LL;

	u64 pos = 0ULL;
	u64 readed = 0, writed = 0;

	char *mem = NULL;

	sysFSStat stat;

	ret = sysLv2FsStat(path, &stat);
	length = stat.st_size;

	if(ret) goto skip;

	if(strstr(path, HDD0ROOT_DIR) != NULL && strstr(path2, HDD0ROOT_DIR) != NULL)
	{
		if(strcmp(path, path2)==0) return ret;

		if(!update_images && (file_exists(path2)))
		{
			if(strstr(path, ".png")) return SUCCESS;
			if(strstr(path, ".jpg")) return SUCCESS;
			if(strstr(path, ".gif")) return SUCCESS;
		}

		sysLv2FsUnlink(path2);
		sysLv2FsLink(path, path2);

		if (file_exists(path2)) return SUCCESS;
	}

	ret = sysLv2FsOpen(path, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0);
	if(ret) goto skip;

	ret = sysLv2FsOpen(path2, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd2, MODE, NULL, 0);
	if(ret) {sysLv2FsClose(fd);goto skip;}

	mem = malloc(0x100000);
	if (mem == NULL) return FAILED;

	while(pos < length)
	{
		readed = length - pos; if(readed > 0x100000ULL) readed = 0x100000ULL;
		ret=sysLv2FsRead(fd, mem, readed, &writed);
		if(ret<0) goto skip;
		if(readed != writed) {ret = 0x8001000C; goto skip;}

		ret=sysLv2FsWrite(fd2, mem, readed, &writed);
		if(ret<0) goto skip;
		if(readed != writed) {ret = 0x8001000C; goto skip;}

		pos += readed;
	}

skip:

	if(mem) free(mem);
	if(fd >=0) sysLv2FsClose(fd);
	if(fd2>=0) sysLv2FsClose(fd2);
	if(ret) return ret;

	ret = sysLv2FsStat(path2, &stat);
	if((ret == SUCCESS) && (stat.st_size == length)) ret = SUCCESS; else ret = FAILED;

	return ret;
}

int sys_get_version2(u32 *version)
{
	lv2syscall2(8, 0x7001, (u64)version);
	return_to_user_prog(int);
}

int sys_get_version(u32 *version)
{
	lv2syscall2(8, 0x7000, (u64)version);
	return_to_user_prog(int);
}

int sys_get_mamba(void)
{
	lv2syscall1(8, 0x7FFF);
	return_to_user_prog(int);
}

bool is_mamba(void)
{
	if(file_exists(HDDROOT_DIR "/mamba_plugins.txt")) return true;

	u32 version = 0x99999999;
	if (sys_get_version(&version) < 0) return false;
	if (sys_get_mamba() == 0x666) return true;
	return false;
}

bool is_disabled(char *filename, char *filename2)
{
	if(not_exists("/dev_blind"))
		sys_fs_mount("CELL_FS_IOS:BUILTIN_FLSH1", "CELL_FS_FAT", "/dev_blind", 0);

	if(file_exists(filename))
	{
		sysLv2FsRename(filename, filename2); // re-enable stage2.bin

		return true;
	}

	return false;
}

bool is_cobra(void)
{
	bool ret = false;

	if(is_disabled("/dev_blind/sys/stage2.cex.bak", "/dev_blind/sys/stage2.cex")) ret = true;
	if(is_disabled("/dev_blind/sys/stage2.dex.bak", "/dev_blind/sys/stage2.dex")) ret = true;
	if(is_disabled("/dev_blind/sys/stage2.bin.bak", "/dev_blind/sys/stage2.bin")) return true;
	if(is_disabled("/dev_blind/sys/stage2_disabled.bin", "/dev_blind/sys/stage2.bin")) return true;
	if(is_disabled("/dev_blind/rebug/cobra/stage2.cex.bak", "/dev_blind/rebug/cobra/stage2.cex")) ret = true;
	if(is_disabled("/dev_blind/rebug/cobra/stage2.dex.bak", "/dev_blind/rebug/cobra/stage2.dex")) ret = true;
	if(is_disabled("/dev_blind/habib/cobra/stage2_disabled.cex", "/dev_blind/habib/cobra/stage2.cex")) return true;

	if(ret) return true;

	if(file_exists("/dev_flash/sys/stage2.bin")) return true;
	if(file_exists("/dev_flash/sys/stage2.cex")) return true;
	if(file_exists("/dev_flash/sys/stage2.dex")) return true;

	if(file_exists("/dev_flash/rebug/cobra")) return true;
	if(file_exists(HDDROOT_DIR "/boot_plugins.txt")) return true;

	if (is_mamba()) return false;

	u32 version = 0x99999999;
	if (sys_get_version(&version) < 0) return false;
	if ((version & 0xFF00FF) == 0x04000F || (version & 0xFFFFFF) == 0x03550F)  return true;

	return ret;
}

#define SYSCALL8_OPCODE_PS3MAPI			 		0x7777
#define PS3MAPI_OPCODE_GET_CORE_MINVERSION		0x0012

int sys_get_minversion_ps3mapi(void)
{
	lv2syscall2(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_MINVERSION);
	return_to_user_prog(int);
}


bool is_ps3mapi(void)
{
	if (0x0111 <= sys_get_minversion_ps3mapi()) return true;
	else return false;
}

int add_mygame()
{
// -2 failed and cannot rename the backup
// -1 failed
//  0 already
//  1 done

	FILE* f;
	long size, gexit_size, len;

	//read original cat
	f = fopen("/dev_flash" XMB_CATEGORY_GAME_XML, "r");
	if(f == NULL) return FAILED;
	fseek (f , 0 , SEEK_END);
	size = ftell (f);
	fseek(f, 0, SEEK_SET);

	char *cat = (char*) malloc (sizeof(char)*size);
	if (cat == NULL) {fclose (f); return FAILED;}

	size_t result = fread(cat, 1, size, f);
	fclose (f);

	if (result != size) {free (cat); return FAILED;}

	// is fb.xml entry in cat file ?
	if(strstr(cat, "fb.xml")!=NULL) {free (cat); return SUCCESS;}

	// search position of game exit
	char gameexit1[] = {0x73, 0x72, 0x63, 0x3d, 0x22, 0x73, 0x65, 0x6c, 0x3a, 0x2f, 0x2f, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x2f, 0x69, 0x6e, 0x67, 0x61, 0x6d, 0x65, 0x3f, 0x70, 0x61, 0x74, 0x68, 0x3d, 0x63, 0x61, 0x74, 0x65, 0x67, 0x6f, 0x72, 0x79, 0x5f, 0x67, 0x61, 0x6d, 0x65, 0x2e, 0x78, 0x6d, 0x6c, 0x23, 0x73, 0x65, 0x67, 0x5f, 0x67, 0x61, 0x6d, 0x65, 0x65, 0x78, 0x69, 0x74, 0x26, 0x74, 0x79, 0x70, 0x65, 0x3d, 0x67, 0x61, 0x6d, 0x65, 0x22, 0x0d, 0x0a, 0x09, 0x09, 0x09, 0x09, 0x2f, 0x3e};
	gexit_size = sizeof(gameexit1); len = gexit_size - 1;

	int i, j, pos=0;
	for(i = 0; i < size - gexit_size; i++)
	{
		for(j = 0; j < gexit_size; j++)
		{
			if(cat[i+j] != gameexit1[j]) break;
			if(j==len) {pos = i; goto patch_xml;}
		}
	}

	// search position of game exit (Unix style)
	char gameexit2[] = {0x73, 0x72, 0x63, 0x3d, 0x22, 0x73, 0x65, 0x6c, 0x3a, 0x2f, 0x2f, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x2f, 0x69, 0x6e, 0x67, 0x61, 0x6d, 0x65, 0x3f, 0x70, 0x61, 0x74, 0x68, 0x3d, 0x63, 0x61, 0x74, 0x65, 0x67, 0x6f, 0x72, 0x79, 0x5f, 0x67, 0x61, 0x6d, 0x65, 0x2e, 0x78, 0x6d, 0x6c, 0x23, 0x73, 0x65, 0x67, 0x5f, 0x67, 0x61, 0x6d, 0x65, 0x65, 0x78, 0x69, 0x74, 0x26, 0x74, 0x79, 0x70, 0x65, 0x3d, 0x67, 0x61, 0x6d, 0x65, 0x22, 0x0a, 0x09, 0x09, 0x09, 0x09, 0x2f, 0x3e};
	gexit_size = sizeof(gameexit2); len = gexit_size - 1;

	for(i = 0; i < size - gexit_size; i++)
	{
		for(j = 0; j < gexit_size; j++)
		{
			if(cat[i+j] != gameexit2[j]) break;
			if(j==len) {pos = i; goto patch_xml;}
		}
	}

	free(cat);
	return FAILED;

patch_xml:

	//write patched cat
	f = fopen(APP_USRDIR "/category_game.xml", "w");
	if(f == NULL) {free(cat); return FAILED;}

	char fb[] = {0x0d, 0x0a, 0x09, 0x09, 0x09, 0x3c, 0x51, 0x75, 0x65, 0x72, 0x79, 0x20, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x3d, 0x22, 0x74, 0x79, 0x70, 0x65, 0x3a, 0x78, 0x2d, 0x78, 0x6d, 0x62, 0x2f, 0x66, 0x6f, 0x6c, 0x64, 0x65, 0x72, 0x2d, 0x70, 0x69, 0x78, 0x6d, 0x61, 0x70, 0x22, 0x0d, 0x0a, 0x09, 0x09, 0x09, 0x09, 0x6b, 0x65, 0x79, 0x3d, 0x22, 0x78, 0x6d, 0x62, 0x5f, 0x61, 0x70, 0x70, 0x33, 0x22, 0x20, 0x0d, 0x0a, 0x09, 0x09, 0x09, 0x09, 0x61, 0x74, 0x74, 0x72, 0x3d, 0x22, 0x78, 0x6d, 0x62, 0x5f, 0x61, 0x70, 0x70, 0x33, 0x22, 0x20, 0x0d, 0x0a, 0x09, 0x09, 0x09, 0x09, 0x73, 0x72, 0x63, 0x3d, 0x22, 0x78, 0x6d, 0x62, 0x3a, 0x2f, 0x2f, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x2f, 0x64, 0x65, 0x76, 0x5f, 0x68, 0x64, 0x64, 0x30, 0x2f, 0x78, 0x6d, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x2f, 0x67, 0x61, 0x6d, 0x65, 0x5f, 0x70, 0x6c, 0x75, 0x67, 0x69, 0x6e, 0x2f, 0x66, 0x62, 0x2e, 0x78, 0x6d, 0x6c, 0x23, 0x73, 0x65, 0x67, 0x5f, 0x66, 0x62, 0x22, 0x0d, 0x0a, 0x09, 0x09, 0x09, 0x09, 0x2f, 0x3e};
	fwrite(cat, 1, pos, f);
	fwrite(fb, 1, sizeof(fb), f);
	fwrite(&cat[pos], 1, size-pos, f);
	fclose(f);


	// set target path for category_game.xml
	char cat_path[80];
	strcpy(cat_path, "/dev_blind" XMB_CATEGORY_GAME_XML);
	if(not_exists(cat_path)) {
		strcpy(cat_path, "/dev_habib" XMB_CATEGORY_GAME_XML);
		if(not_exists(cat_path)) {
			strcpy(cat_path, "/dev_rewrite" XMB_CATEGORY_GAME_XML);

			// mount /dev_blind if category_game.xml is not found
			if(not_exists(cat_path)) {
				if(sys_fs_mount("CELL_FS_IOS:BUILTIN_FLSH1", "CELL_FS_FAT", "/dev_blind", 0) == SUCCESS) {
					strcpy(cat_path, "/dev_blind" XMB_CATEGORY_GAME_XML);
					if(not_exists(cat_path)) { free(cat); return FAILED;}
				} else { free(cat); return FAILED;}
			}
		}
	}

	// rename category_game.xml as category_game.xml.bak
	char cat_path_bak[80];
	sprintf(cat_path_bak, "%s.bak", cat_path);
	sysLv2FsUnlink(cat_path_bak);
	if(sysLv2FsRename(cat_path, cat_path_bak) != SUCCESS) {free(cat); return FAILED;} ;

	// update category_game.xml
	if(file_copy(APP_USRDIR "/xmb/category_game.xml", cat_path) != SUCCESS)
	{
		sysLv2FsUnlink(cat_path);
		if(sysLv2FsRename(cat_path_bak, cat_path)) { //restore category_game.xml from category_game.xml.bak
			{lv2syscall3(392, 0x1004, 0xa, 0x1b6);} ///TRIPLE BIP
			free(cat);
			return -2;
		}
	}
	free(cat);
	return 1;
}

int main()
{
	FILE* f=NULL;
	sysFSStat stat;

//--- hold CROSS

	unsigned button = 0;

	padInfo padinfo;
	padData paddata;

	ioPadInit(7);

	int n, r;
	for(r = 0; r < 10; r++)
	{
		ioPadGetInfo(&padinfo);
		for(n = 0; n < 7; n++)
		{
			if(padinfo.status[n])
			{
				ioPadGetData(n, &paddata);
				button = (paddata.button[2] << 8) | (paddata.button[3] & 0xff);
				break;
			}
		}
		if(button) break; else usleep(20000);
	}
	ioPadEnd();

	if(button & (BUTTON_L1)) full = true; else
	if(button & (BUTTON_CROSS | BUTTON_CIRCLE)) lite = true; else
	if(	(!(button & BUTTON_R1) && ((sysLv2FsStat(PLUGINS_DIR "/webftp_server.sprx", &stat) == SUCCESS) && (stat.st_size > 330000))) ||
		(is_ps3hen() == 0x1337) ||
		(file_exists("/dev_flash/hen/PS3HEN.BIN")) ||
		(file_exists(HDDROOT_DIR "/hen/PS3HEN.BIN")) ||
		(file_exists("/dev_usb000/PS3HEN.BIN")) ||
		(file_exists("/dev_usb001/PS3HEN.BIN"))
		) full = true;
	if(button & (BUTTON_L2 | BUTTON_R2)) update_images = true;
//---

	// Show message
	Init_Graph();
	cls();
	if(full)
		DrawString(20, 216+24, "Installing webMAN MOD FULL...");
	else if(lite)
		DrawString(20, 216+24, "Installing webMAN MOD Lite...");
	else
		DrawString(20, 216+24, "Installing webMAN MOD...");
	tiny3d_Flip();
	sleep(2);

	full_on_nocobra = (file_exists(HDDROOT_DIR "/kernel/mamba_484C.bin")) &&
					 ((file_exists(HDDROOT_DIR "/boot_plugins_kernel_nocobra.txt")) || (file_exists(PLUGINS_DIR "/boot_plugins_kernel_nocobra _dex.txt")));

	// Create webman folders
	sysLv2FsMkdir(TMP_DIR,   DMODE);
	sysLv2FsMkdir(LANG_DIR,  DMODE);
	sysLv2FsMkdir(COMBO_DIR, DMODE);
	sysLv2FsMkdir(ICONS_DIR, DMODE);
	sysLv2FsMkdir(RES_DIR,   DMODE);
	sysLv2FsMkdir(SETUP_DIR, DMODE);

	sysLv2FsMkdir(HDDROOT_DIR "/xmlhost", DMODE);
	sysLv2FsMkdir(XMLHOST_DIR, DMODE);

	sysLv2FsMkdir(HDDROOT_DIR "/game", DMODE);

	sysLv2FsMkdir(XMBMANPLS_DIR, DMODE);
	sysLv2FsMkdir(XMBMANPLS_DIR "/USRDIR", DMODE);
	sysLv2FsMkdir(XMBMANPLS_IMAGES_DIR, DMODE);
	sysLv2FsMkdir(XMBMANPLS_FEATS_DIR, DMODE);

	// Create backup folders
	sysLv2FsMkdir(HDDROOT_DIR "/packages", DMODE);
	sysLv2FsMkdir(HDDROOT_DIR "/PS3ISO", DMODE);
	sysLv2FsMkdir(HDDROOT_DIR "/PSXISO", DMODE);
	sysLv2FsMkdir(HDDROOT_DIR "/PS2ISO", DMODE);
	sysLv2FsMkdir(HDDROOT_DIR "/PSPISO", DMODE);
	sysLv2FsMkdir(HDDROOT_DIR "/DVDISO", DMODE);
	sysLv2FsMkdir(HDDROOT_DIR "/BDISO",  DMODE);
	sysLv2FsMkdir(HDDROOT_DIR "/GAMES",  DMODE);
	sysLv2FsMkdir(HDDROOT_DIR "/ROMS",   DMODE);

	// remove language files (old location)
	sysLv2FsUnlink(TMP_DIR "/LANG_EN.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_AR.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_CN.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_DE.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_ES.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_FR.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_GR.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_DK.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_HU.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_HR.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_BG.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_CZ.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_SK.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_IN.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_JP.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_KR.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_IT.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_NL.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_PL.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_PT.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_RU.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_TR.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_ZH.TXT");
	sysLv2FsUnlink(TMP_DIR "/LANG_XX.TXT");

	// remove language files
	sysLv2FsUnlink(LANG_DIR "/LANG_EN.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_AR.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_CN.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_DE.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_ES.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_FR.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_GR.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_DK.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_HU.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_HR.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_BG.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_CZ.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_SK.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_IN.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_JP.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_KR.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_IT.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_NL.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_PL.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_PT.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_RU.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_TR.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_ZH.TXT");
	sysLv2FsUnlink(LANG_DIR "/LANG_ROMS.TXT");

	// remove old files
	sysLv2FsUnlink(APP_USRDIR "/webftp_server_rebug_cobra_multi19.sprx");
	sysLv2FsUnlink(APP_USRDIR "/webftp_server_rebug_cobra_multi20.sprx");
	sysLv2FsUnlink(APP_USRDIR "/webftp_server_rebug_cobra_multi21.sprx");
	sysLv2FsUnlink(APP_USRDIR "/webftp_server_webchat.sprx");

	sysLv2FsUnlink(XMLHOST_DIR "/jquery-1.11.3.min.js");
	sysLv2FsUnlink(APP_USRDIR  "/jquery-1.11.3.min.js");
	sysLv2FsUnlink(XMLHOST_DIR "/jquery-1.12.3.min.js");
	sysLv2FsUnlink(APP_USRDIR  "/jquery-1.12.3.min.js");

	sysLv2FsUnlink(TMP_DIR "/psp_icon.png");

	// update languages
	file_copy(APP_USRDIR "/lang/LANG_EN.TXT", LANG_DIR "/LANG_EN.TXT");
	file_copy(APP_USRDIR "/lang/LANG_AR.TXT", LANG_DIR "/LANG_AR.TXT");
	file_copy(APP_USRDIR "/lang/LANG_CN.TXT", LANG_DIR "/LANG_CN.TXT");
	file_copy(APP_USRDIR "/lang/LANG_DE.TXT", LANG_DIR "/LANG_DE.TXT");
	file_copy(APP_USRDIR "/lang/LANG_ES.TXT", LANG_DIR "/LANG_ES.TXT");
	file_copy(APP_USRDIR "/lang/LANG_FR.TXT", LANG_DIR "/LANG_FR.TXT");
	file_copy(APP_USRDIR "/lang/LANG_GR.TXT", LANG_DIR "/LANG_GR.TXT");
	file_copy(APP_USRDIR "/lang/LANG_DK.TXT", LANG_DIR "/LANG_DK.TXT");
	file_copy(APP_USRDIR "/lang/LANG_HU.TXT", LANG_DIR "/LANG_HU.TXT");
	file_copy(APP_USRDIR "/lang/LANG_HR.TXT", LANG_DIR "/LANG_HR.TXT");
	file_copy(APP_USRDIR "/lang/LANG_BG.TXT", LANG_DIR "/LANG_BG.TXT");
	file_copy(APP_USRDIR "/lang/LANG_CZ.TXT", LANG_DIR "/LANG_CZ.TXT");
	file_copy(APP_USRDIR "/lang/LANG_SK.TXT", LANG_DIR "/LANG_SK.TXT");
	file_copy(APP_USRDIR "/lang/LANG_IN.TXT", LANG_DIR "/LANG_IN.TXT");
	file_copy(APP_USRDIR "/lang/LANG_IT.TXT", LANG_DIR "/LANG_IT.TXT");
	file_copy(APP_USRDIR "/lang/LANG_JP.TXT", LANG_DIR "/LANG_JP.TXT");
	file_copy(APP_USRDIR "/lang/LANG_KR.TXT", LANG_DIR "/LANG_KR.TXT");
	file_copy(APP_USRDIR "/lang/LANG_NL.TXT", LANG_DIR "/LANG_NL.TXT");
	file_copy(APP_USRDIR "/lang/LANG_PL.TXT", LANG_DIR "/LANG_PL.TXT");
	file_copy(APP_USRDIR "/lang/LANG_PT.TXT", LANG_DIR "/LANG_PT.TXT");
	file_copy(APP_USRDIR "/lang/LANG_RU.TXT", LANG_DIR "/LANG_RU.TXT");
	file_copy(APP_USRDIR "/lang/LANG_TR.TXT", LANG_DIR "/LANG_TR.TXT");
	file_copy(APP_USRDIR "/lang/LANG_ZH.TXT", LANG_DIR "/LANG_ZH.TXT");

	file_copy(APP_USRDIR "/lang/LANG_ROMS.TXT", LANG_DIR "/LANG_ROMS.TXT");

	// copy html
	sysLv2FsMkdir(XMLHOST_DIR, DMODE);
	file_copy(APP_USRDIR "/html/games.html",     XMLHOST_DIR "/games.html");
	file_copy(APP_USRDIR "/html/mobile.html",    XMLHOST_DIR "/mobile.html");
	file_copy(APP_USRDIR "/html/background.gif", XMLHOST_DIR "/background.gif");
	file_copy(APP_USRDIR "/html/sman.htm",       XMLHOST_DIR "/sman.htm");
	file_copy(APP_USRDIR "/html/sman.css",       XMLHOST_DIR "/sman.css");
	file_copy(APP_USRDIR "/html/sman.js",        XMLHOST_DIR "/sman.js");
	file_copy(APP_USRDIR "/html/help.html",      XMLHOST_DIR "/help.html");
	file_copy(APP_USRDIR "/html/xmb.html",       XMLHOST_DIR "/xmb.html");
	file_copy(APP_USRDIR "/html/combos.html",    XMLHOST_DIR "/combos.html");
	file_copy(APP_USRDIR "/html/combolist.js",   XMLHOST_DIR "/combolist.js");
	file_copy(APP_USRDIR "/html/chart.css",      XMLHOST_DIR "/chart.css");
	file_copy(APP_USRDIR "/html/pad.html",       XMLHOST_DIR "/pad.html");
	file_copy(APP_USRDIR "/html/pad.jpg",        XMLHOST_DIR "/pad.jpg");

	// copy javascripts
	file_copy(APP_USRDIR "/html/jquery.min.js",    XMLHOST_DIR "/jquery.min.js");  // jQuery v3.1.1
	file_copy(APP_USRDIR "/html/jquery-ui.min.js", XMLHOST_DIR "/jquery-ui.min.js"); // jQuery UI v1.12.1
	file_copy(APP_USRDIR "/html/qrcode.min.js",    XMLHOST_DIR "/qrcode.min.js"); // QRCode.js
	
	file_copy(APP_USRDIR "/html/fm.js",     XMLHOST_DIR "/fm.js");
	file_copy(APP_USRDIR "/html/fs.js",     XMLHOST_DIR "/fs.js");
	file_copy(APP_USRDIR "/html/games.js",  XMLHOST_DIR "/games.js");
	file_copy(APP_USRDIR "/html/common.js", XMLHOST_DIR "/common.js");

	// copy css
	file_copy(APP_USRDIR "/html/common.css",  XMLHOST_DIR "/common.css");

	// crossdomain.xml
	file_copy(APP_USRDIR "/html/crossdomain.xml", XMLHOST_DIR "/crossdomain.xml");

	// temperature monitor
	file_copy(APP_USRDIR "/html/gauge.min.js", XMLHOST_DIR "/gauge.min.js");
	file_copy(APP_USRDIR "/html/temp.js",      XMLHOST_DIR "/temp.js");
	file_copy(APP_USRDIR "/html/tempc.html",   XMLHOST_DIR "/tempc.html");
	file_copy(APP_USRDIR "/html/tempf.html",   XMLHOST_DIR "/tempf.html");
	
	// overclocking panel
	file_copy(APP_USRDIR "/html/oc.html",      XMLHOST_DIR "/oc.html");

	// fb.xml
	if(is_ps3hen() == 0x1337)
	{
		sysLv2FsUnlink(XMLHOST_DIR "/fb.xml");

		if((not_exists(XMLHOST_DIR "/fb-hen.xml")))
			file_copy(APP_USRDIR "/xmb/fb.xml", XMLHOST_DIR "/fb-hen.xml");
	}
	else
	{
		if((not_exists(XMLHOST_DIR "/fb.xml")))
			file_copy(APP_USRDIR "/xmb/fb.xml", XMLHOST_DIR "/fb.xml");
	}

	// copy new icons
	sysLv2FsMkdir(TMP_DIR "/wm_icons", DMODE);

	file_copy(APP_USRDIR "/icons/icon_wm_album_ps3.png", ICONS_DIR "/icon_wm_album_ps3.png");
	file_copy(APP_USRDIR "/icons/icon_wm_album_psx.png", ICONS_DIR "/icon_wm_album_psx.png");
	file_copy(APP_USRDIR "/icons/icon_wm_album_ps2.png", ICONS_DIR "/icon_wm_album_ps2.png");
	file_copy(APP_USRDIR "/icons/icon_wm_album_psp.png", ICONS_DIR "/icon_wm_album_psp.png");
	file_copy(APP_USRDIR "/icons/icon_wm_album_dvd.png", ICONS_DIR "/icon_wm_album_dvd.png");
	file_copy(APP_USRDIR "/icons/icon_wm_album_emu.png", ICONS_DIR "/icon_wm_album_emu.png");

	file_copy(APP_USRDIR "/icons/icon_wm_ps3.png"      , ICONS_DIR "/icon_wm_ps3.png");
	file_copy(APP_USRDIR "/icons/icon_wm_psx.png"      , ICONS_DIR "/icon_wm_psx.png");
	file_copy(APP_USRDIR "/icons/icon_wm_ps2.png"      , ICONS_DIR "/icon_wm_ps2.png");
	file_copy(APP_USRDIR "/icons/icon_wm_psp.png"      , ICONS_DIR "/icon_wm_psp.png");
	file_copy(APP_USRDIR "/icons/icon_wm_bdv.png"      , ICONS_DIR "/icon_wm_bdv.png");
	file_copy(APP_USRDIR "/icons/icon_wm_dvd.png"      , ICONS_DIR "/icon_wm_dvd.png");
	file_copy(APP_USRDIR "/icons/icon_wm_retro.png"    , ICONS_DIR "/icon_wm_retro.png");

	file_copy(APP_USRDIR "/icons/icon_wm_settings.png" , ICONS_DIR "/icon_wm_settings.png");
	file_copy(APP_USRDIR "/icons/icon_wm_eject.png"    , ICONS_DIR "/icon_wm_eject.png");

	file_copy(APP_USRDIR "/icons/power.png",             ICONS_DIR "/power.png");
	file_copy(APP_USRDIR "/icons/time.png",              ICONS_DIR "/time.png");
	file_copy(APP_USRDIR "/icons/find.png",              ICONS_DIR "/find.png");

	// default PKGLAUNCH icon
	file_copy(APP_USRDIR "/icons/ICON0.PNG",             ICONS_DIR "/ICON0.PNG");

	if((sysLv2FsStat(APP_USRDIR "/multiman.png", &stat) == SUCCESS) && (stat.st_size == 9894))
		file_copy(APP_USRDIR "/icons/icon_wm_root.png" , ICONS_DIR "/icon_wm_root.png");
	else
		sysLv2FsUnlink(ICONS_DIR "/icon_wm_root.png");

	// file manager icons
	file_copy(APP_USRDIR "/icons/psx.png"     , ICONS_DIR "/psx.png");
	file_copy(APP_USRDIR "/icons/ps2.png"     , ICONS_DIR "/ps2.png");
	file_copy(APP_USRDIR "/icons/ps3.png"     , ICONS_DIR "/ps3.png");
	file_copy(APP_USRDIR "/icons/psp.png"     , ICONS_DIR "/psp.png");
	file_copy(APP_USRDIR "/icons/dvd.png"     , ICONS_DIR "/dvd.png");
	file_copy(APP_USRDIR "/icons/pkg.png"     , ICONS_DIR "/pkg.png");
	file_copy(APP_USRDIR "/icons/retro.png"   , ICONS_DIR "/retro.png");
	file_copy(APP_USRDIR "/icons/video.png"   , ICONS_DIR "/video.png");
	file_copy(APP_USRDIR "/icons/photo.png"   , ICONS_DIR "/photo.png");
	file_copy(APP_USRDIR "/icons/music.png"   , ICONS_DIR "/music.png");
	file_copy(APP_USRDIR "/icons/settings.png", ICONS_DIR "/settings.png");

	file_copy(APP_USRDIR "/icons/blank.png", ICONS_DIR "/blank.png");

	sysLv2FsUnlink(RES_DIR "/reloadxmb.pkg");
	sysLv2FsUnlink(RES_DIR "/ROMS_PKG_Launcher.pkg");
	sysLv2FsUnlink(TMP_DIR "/wm_online_ids.txt");

	if(not_exists(RES_DIR "/wm_online_ids.txt"))
		file_copy(APP_USRDIR "/res/wm_online_ids.txt", RES_DIR "/wm_online_ids.txt");
	if(not_exists(RES_DIR "/wm_offline_ids.txt"))
		file_copy(APP_USRDIR "/res/wm_offline_ids.txt", RES_DIR "/wm_offline_ids.txt");
	if(not_exists(RES_DIR "/wm_ignore.txt"))
		file_copy(APP_USRDIR "/res/wm_ignore.txt",     RES_DIR "/wm_ignore.txt");
	if(not_exists(RES_DIR "/roms_extensions.txt"))
		file_copy(APP_USRDIR "/res/roms_extensions.txt", RES_DIR "/roms_extensions.txt");
	if(not_exists(RES_DIR "/roms_paths.txt"))
		file_copy(APP_USRDIR "/res/roms_paths.txt", RES_DIR "/roms_paths.txt");
	if(not_exists(RES_DIR "/ps1_bios.bin"))
		file_copy(APP_USRDIR "/res/ps1_bios.bin", RES_DIR "/ps1_bios.bin");

	//file_copy(APP_USRDIR "/res/npsignin_plugin.rco",   RES_DIR "/npsignin_plugin.rco");

	// webMAN ADD-ONS
	file_copy(APP_USRDIR "/addons/prepISO.pkg",    RES_DIR "/prepISO.pkg");
	file_copy(APP_USRDIR "/addons/load_wmm.pkg",   RES_DIR "/load_wmm.pkg");
	file_copy(APP_USRDIR "/addons/ps3mon.sprx",    RES_DIR "/ps3mon.sprx");
	file_copy(APP_USRDIR "/addons/RELOAD_XMB.ISO", RES_DIR "/RELOAD_XMB.ISO");
	file_copy(APP_USRDIR "/addons/boot_mamba.pkg",    RES_DIR "/boot_mamba.pkg");
	file_copy(APP_USRDIR "/addons/boot_fake_ofw.pkg", RES_DIR "/boot_fake_ofw.pkg");
	file_copy(APP_USRDIR "/addons/PSP_Minis_Launcher.pkg",     RES_DIR "/PSP_Minis_Launcher.pkg");
	file_copy(APP_USRDIR "/addons/PSP_Remasters_Launcher.pkg", RES_DIR "/PSP_Remasters_Launcher.pkg");
	file_copy(APP_USRDIR "/addons/wm_theme_colorful.pkg"		, RES_DIR "/wm_theme_colorful.pkg");
	file_copy(APP_USRDIR "/addons/wm_theme_standard.pkg",         RES_DIR "/wm_theme_standard.pkg");
	file_copy(APP_USRDIR "/addons/wm_theme_metalification.pkg"	, RES_DIR "/wm_theme_metalification.pkg");
	file_copy(APP_USRDIR "/addons/wm_theme_rebugification.pkg"	, RES_DIR "/wm_theme_rebugification.pkg");
	file_copy(APP_USRDIR "/addons/wm_theme_flowerification.pkg"	, RES_DIR "/wm_theme_flowerification.pkg");

	file_copy(APP_USRDIR "/setup/setup0.dat", SETUP_DIR "/setup0.dat");
	file_copy(APP_USRDIR "/setup/setup1.dat", SETUP_DIR "/setup1.dat");
	file_copy(APP_USRDIR "/setup/setup2.dat", SETUP_DIR "/setup2.dat");
	file_copy(APP_USRDIR "/setup/setup3.dat", SETUP_DIR "/setup3.dat");
	file_copy(APP_USRDIR "/setup/setup4.dat", SETUP_DIR "/setup4.dat");
	file_copy(APP_USRDIR "/setup/setup5.dat", SETUP_DIR "/setup5.dat");
	file_copy(APP_USRDIR "/setup/setup6.dat", SETUP_DIR "/setup6.dat");
	file_copy(APP_USRDIR "/setup/setup7.dat", SETUP_DIR "/setup7.dat");
	file_copy(APP_USRDIR "/setup/setup8.dat", SETUP_DIR "/setup8.dat");
	file_copy(APP_USRDIR "/setup/setupa.dat", SETUP_DIR "/setupa.dat");
	file_copy(APP_USRDIR "/setup/setupv.dat", SETUP_DIR "/setupv.dat");
	file_copy(APP_USRDIR "/setup/setupd.dat", SETUP_DIR "/setupd.dat");
	file_copy(APP_USRDIR "/setup/setupc.dat", SETUP_DIR "/setupc.dat");
	file_copy(APP_USRDIR "/setup/setupg.dat", SETUP_DIR "/setupg.dat");
	file_copy(APP_USRDIR "/setup/setupi.dat", SETUP_DIR "/setupi.dat");
	file_copy(APP_USRDIR "/setup/setupj.dat", SETUP_DIR "/setupj.dat");
	file_copy(APP_USRDIR "/setup/setupk.dat", SETUP_DIR "/setupk.dat");
	file_copy(APP_USRDIR "/setup/setupm.dat", SETUP_DIR "/setupm.dat");
	file_copy(APP_USRDIR "/setup/setupn.dat", SETUP_DIR "/setupn.dat");
	file_copy(APP_USRDIR "/setup/setupo.dat", SETUP_DIR "/setupo.dat");
	file_copy(APP_USRDIR "/setup/setupp.dat", SETUP_DIR "/setupp.dat");
	file_copy(APP_USRDIR "/setup/setups.dat", SETUP_DIR "/setups.dat");
	file_copy(APP_USRDIR "/setup/setupt.dat", SETUP_DIR "/setupt.dat");
	file_copy(APP_USRDIR "/setup/setupx.dat", SETUP_DIR "/setupx.dat");
	file_copy(APP_USRDIR "/setup/setupz.dat", SETUP_DIR "/setupz.dat");

	sysLv2FsRename(APP_USRDIR "/xmb/RELOADXMB", RELOADXMB_DIR);
	sysLv2FsRename(APP_USRDIR "/lsd", LSD_DIR);

	// fallback to manual copy
	sysLv2FsMkdir(RELOADXMB_DIR, DMODE);
	sysLv2FsMkdir(RELOADXMB_DIR "/USRDIR", DMODE);
	file_copy(APP_USRDIR "/xmb/RELOADXMB/ICON0.PNG", RELOADXMB_DIR "/ICON0.PNG");
	file_copy(APP_USRDIR "/xmb/RELOADXMB/PARAM.SFO", RELOADXMB_DIR "/PARAM.SFO");
	file_copy(APP_USRDIR "/xmb/RELOADXMB/EBOOT.BIN", RELOADXMB_DIR "/USRDIR/EBOOT.BIN");

	sysLv2FsRename(APP_USRDIR "/xmb/PKGLAUNCH", PKGLAUNCH_DIR);

	// fallback to manual copy
	sysLv2FsMkdir(PKGLAUNCH_DIR, DMODE);
	sysLv2FsMkdir(PKGLAUNCH_DIR"/USRDIR", DMODE);
	sysLv2FsMkdir(PKGLAUNCH_DIR"/PS3_GAME", DMODE);
	sysLv2FsMkdir(PKGLAUNCH_DIR"/PS3_GAME/USRDIR", DMODE);
	file_copy(APP_USRDIR "/xmb/PKGLAUNCH/ICON0.PNG",                     PKGLAUNCH_DIR "/ICON0.PNG");
	file_copy(APP_USRDIR "/xmb/PKGLAUNCH/PARAM.SFO",                     PKGLAUNCH_DIR "/PARAM.SFO");
	file_copy(APP_USRDIR "/xmb/PKGLAUNCH/USRDIR/EBOOT.BIN",              PKGLAUNCH_DIR "/USRDIR/EBOOT.BIN");
	file_copy(APP_USRDIR "/xmb/PKGLAUNCH/USRDIR/unrar.self",             PKGLAUNCH_DIR "/USRDIR/unrar.self");
	file_copy(APP_USRDIR "/xmb/PKGLAUNCH/PS3_GAME/ICON0.PNG",            PKGLAUNCH_DIR "/PS3_GAME/ICON0.PNG");
	file_copy(APP_USRDIR "/xmb/PKGLAUNCH/PS3_GAME/PARAM.SFO",            PKGLAUNCH_DIR "/PS3_GAME/PARAM.SFO");
	file_copy(APP_USRDIR "/xmb/PKGLAUNCH/PS3_GAME/USRDIR/EBOOT.BIN",     PKGLAUNCH_DIR "/PS3_GAME/USRDIR/EBOOT.BIN");
	file_copy(APP_USRDIR "/xmb/PKGLAUNCH/PS3_GAME/USRDIR/retroarch.bak", PKGLAUNCH_DIR "/PS3_GAME/USRDIR/retroarch.bak");
	file_copy(APP_USRDIR "/xmb/PKGLAUNCH/PS3_GAME/USRDIR/retroarch.cce", PKGLAUNCH_DIR "/PS3_GAME/USRDIR/retroarch.cce");

	sysLv2FsMkdir(PS2CONFIG_DIR, DMODE);
	sysLv2FsMkdir(PS2CONFIG_USRDIR, DMODE);
	file_copy(APP_USRDIR "/CONFIG/ICON0.PNG", PS2CONFIG_DIR "/ICON0.PNG");
	file_copy(APP_USRDIR "/CONFIG/PARAM.SFO", PS2CONFIG_DIR "/PARAM.SFO");

	if(not_exists(PS2CONFIG_USRDIR "/CONFIG/ENC"))
	{
		// copy PS2 CONFIG files
		char path1[80], path2[80];
		int fd; sysFSDirent dir; u64 read_e;

		sysLv2FsMkdir(PS2CONFIG_USRDIR "/CONFIG", DMODE);
		sysLv2FsMkdir(PS2CONFIG_USRDIR "/CONFIG/GX", DMODE);
		sysLv2FsMkdir(PS2CONFIG_USRDIR "/CONFIG/NET", DMODE);
		sysLv2FsMkdir(PS2CONFIG_USRDIR "/CONFIG/ENC", DMODE);
		sysLv2FsMkdir(PS2CONFIG_USRDIR "/CONFIG/SOFT", DMODE);
		sysLv2FsMkdir(PS2CONFIG_USRDIR "/CONFIG/CUSTOM", DMODE);

		if(sysLv2FsOpenDir(APP_USRDIR "/CONFIG/GX", &fd) == SUCCESS)
		{
			while((sysLv2FsReadDir(fd, &dir, &read_e) == SUCCESS) && (read_e > 0))
			{
				sprintf(path1, APP_USRDIR       "/CONFIG/%s/%s", "GX", dir.d_name);
				sprintf(path2, PS2CONFIG_USRDIR "/CONFIG/%s/%s", "GX", dir.d_name);
				sysLv2FsUnlink(path2);
				sysLv2FsLink(path1, path2);
			}
			sysLv2FsCloseDir(fd);
		}
		if(sysLv2FsOpenDir(APP_USRDIR "/CONFIG/NET", &fd) == SUCCESS)
		{
			while((sysLv2FsReadDir(fd, &dir, &read_e) == SUCCESS) && (read_e > 0))
			{
				sprintf(path1, APP_USRDIR       "/CONFIG/%s/%s", "NET", dir.d_name);
				sprintf(path2, PS2CONFIG_USRDIR "/CONFIG/%s/%s", "NET", dir.d_name);
				sysLv2FsUnlink(path2);
				sysLv2FsLink(path1, path2);
			}
			sysLv2FsCloseDir(fd);
		}
		if(sysLv2FsOpenDir(APP_USRDIR "/CONFIG/ENC", &fd) == SUCCESS)
		{
			while((sysLv2FsReadDir(fd, &dir, &read_e) == SUCCESS) && (read_e > 0))
			{
				sprintf(path1, APP_USRDIR       "/CONFIG/%s/%s", "ENC", dir.d_name);
				sprintf(path2, PS2CONFIG_USRDIR "/CONFIG/%s/%s", "ENC", dir.d_name);
				sysLv2FsUnlink(path2);
				sysLv2FsLink(path1, path2);
			}
			sysLv2FsCloseDir(fd);
		}
		if(sysLv2FsOpenDir(APP_USRDIR "/CONFIG/SOFT", &fd) == SUCCESS)
		{
			while((sysLv2FsReadDir(fd, &dir, &read_e) == SUCCESS) && (read_e > 0))
			{
				sprintf(path1, APP_USRDIR       "/CONFIG/%s/%s", "SOFT", dir.d_name);
				sprintf(path2, PS2CONFIG_USRDIR "/CONFIG/%s/%s", "SOFT", dir.d_name);
				sysLv2FsUnlink(path2);
				sysLv2FsLink(path1, path2);
			}
			sysLv2FsCloseDir(fd);
		}
		if(sysLv2FsOpenDir(APP_USRDIR "/CONFIG/CUSTOM", &fd) == SUCCESS)
		{
			while((sysLv2FsReadDir(fd, &dir, &read_e) == SUCCESS) && (read_e > 0))
			{
				sprintf(path1, APP_USRDIR       "/CONFIG/%s/%s", "CUSTOM", dir.d_name);
				sprintf(path2, PS2CONFIG_USRDIR "/CONFIG/%s/%s", "CUSTOM", dir.d_name);
				sysLv2FsUnlink(path2);
				sysLv2FsLink(path1, path2);
			}
			sysLv2FsCloseDir(fd);
		}
		if(sysLv2FsOpenDir(APP_USRDIR "/CONFIG/vm", &fd) == SUCCESS)
		{
			sysLv2FsMkdir("/dev_hdd0/vm", DMODE);
			sysLv2FsMkdir("/dev_hdd0/vm/gx", DMODE);

			while((sysLv2FsReadDir(fd, &dir, &read_e) == SUCCESS) && (read_e > 0))
			{
				sprintf(path1, APP_USRDIR "/CONFIG/%s/%s", "vm", dir.d_name);
				sprintf(path2, "/dev_hdd0/vm/gx/%s", dir.d_name);
				sysLv2FsUnlink(path2);
				sysLv2FsLink(path1, path2);
			}
			sysLv2FsCloseDir(fd);
		}
	}

	// webMAN LaunchPad icons
	file_copy(APP_USRDIR "/icons/icon_lp_ps3.png"    , ICONS_DIR "/icon_lp_ps3.png");
	file_copy(APP_USRDIR "/icons/icon_lp_psx.png"    , ICONS_DIR "/icon_lp_psx.png");
	file_copy(APP_USRDIR "/icons/icon_lp_ps2.png"    , ICONS_DIR "/icon_lp_ps2.png");
	file_copy(APP_USRDIR "/icons/icon_lp_psp.png"    , ICONS_DIR "/icon_lp_psp.png");
	file_copy(APP_USRDIR "/icons/icon_lp_dvd.png"    , ICONS_DIR "/icon_lp_dvd.png");
	file_copy(APP_USRDIR "/icons/icon_lp_blu.png"    , ICONS_DIR "/icon_lp_blu.png");
	file_copy(APP_USRDIR "/icons/icon_lp_nocover.png", ICONS_DIR "/icon_lp_nocover.png");

	// XMBM+ webMAN
	sysLv2FsMkdir(XMBMANPLS_DIR, DMODE);
	sysLv2FsMkdir(XMBMANPLS_DIR "/USRDIR", DMODE);
	sysLv2FsMkdir(XMBMANPLS_IMAGES_DIR, DMODE);
	sysLv2FsMkdir(XMBMANPLS_FEATS_DIR, DMODE);

	if(not_exists(XMBMANPLS_DIR "/PARAM.SFO"))
		file_copy(APP_USRDIR "/xmbm/PARAM.SFO", XMBMANPLS_DIR "/PARAM.SFO");

	if((sysLv2FsStat(XMBMANPLS_DIR "/ICON0.PNG", &stat) == SUCCESS) && (stat.st_size == 2138))
		file_copy(APP_USRDIR "/xmbm/ICON0.PNG", XMBMANPLS_DIR "/ICON0.PNG");

	if(not_exists(XMBMANPLS_DIR "/ICON0.PNG"))
		file_copy(APP_USRDIR "/xmbm/ICON0.PNG", XMBMANPLS_DIR "/ICON0.PNG");

	//if(not_exists(HDDROOT_DIR "/boot_init.bak"))
	//	file_copy(APP_DIR "/boot_init.bak", HDDROOT_DIR "/boot_init.bak");

	file_copy(APP_USRDIR "/xml/webMAN.xml"    , XMBMANPLS_FEATS_DIR "/webMAN.xml");
	file_copy(APP_USRDIR "/xml/webMAN_EN.xml" , XMBMANPLS_FEATS_DIR "/webMAN_EN.xml");
	file_copy(APP_USRDIR "/xml/webMAN_AR.xml" , XMBMANPLS_FEATS_DIR "/webMAN_AR.xml");
	file_copy(APP_USRDIR "/xml/webMAN_CN.xml" , XMBMANPLS_FEATS_DIR "/webMAN_CN.xml");
	file_copy(APP_USRDIR "/xml/webMAN_DE.xml" , XMBMANPLS_FEATS_DIR "/webMAN_DE.xml");
	file_copy(APP_USRDIR "/xml/webMAN_ES.xml" , XMBMANPLS_FEATS_DIR "/webMAN_ES.xml");
	file_copy(APP_USRDIR "/xml/webMAN_FR.xml" , XMBMANPLS_FEATS_DIR "/webMAN_FR.xml");
	file_copy(APP_USRDIR "/xml/webMAN_GR.xml" , XMBMANPLS_FEATS_DIR "/webMAN_GR.xml");
	file_copy(APP_USRDIR "/xml/webMAN_DK.xml" , XMBMANPLS_FEATS_DIR "/webMAN_DK.xml");
	file_copy(APP_USRDIR "/xml/webMAN_HU.xml" , XMBMANPLS_FEATS_DIR "/webMAN_HU.xml");
	file_copy(APP_USRDIR "/xml/webMAN_HR.xml" , XMBMANPLS_FEATS_DIR "/webMAN_HR.xml");
	file_copy(APP_USRDIR "/xml/webMAN_BG.xml" , XMBMANPLS_FEATS_DIR "/webMAN_BG.xml");
	file_copy(APP_USRDIR "/xml/webMAN_CZ.xml" , XMBMANPLS_FEATS_DIR "/webMAN_CZ.xml");
	file_copy(APP_USRDIR "/xml/webMAN_SK.xml" , XMBMANPLS_FEATS_DIR "/webMAN_SK.xml");
	file_copy(APP_USRDIR "/xml/webMAN_IN.xml" , XMBMANPLS_FEATS_DIR "/webMAN_IN.xml");
	file_copy(APP_USRDIR "/xml/webMAN_JP.xml" , XMBMANPLS_FEATS_DIR "/webMAN_JP.xml");
	file_copy(APP_USRDIR "/xml/webMAN_KR.xml" , XMBMANPLS_FEATS_DIR "/webMAN_KR.xml");
	file_copy(APP_USRDIR "/xml/webMAN_IT.xml" , XMBMANPLS_FEATS_DIR "/webMAN_IT.xml");
	file_copy(APP_USRDIR "/xml/webMAN_NL.xml" , XMBMANPLS_FEATS_DIR "/webMAN_NL.xml");
	file_copy(APP_USRDIR "/xml/webMAN_PL.xml" , XMBMANPLS_FEATS_DIR "/webMAN_PL.xml");
	file_copy(APP_USRDIR "/xml/webMAN_PT.xml" , XMBMANPLS_FEATS_DIR "/webMAN_PT.xml");
	file_copy(APP_USRDIR "/xml/webMAN_RU.xml" , XMBMANPLS_FEATS_DIR "/webMAN_RU.xml");
	file_copy(APP_USRDIR "/xml/webMAN_TR.xml" , XMBMANPLS_FEATS_DIR "/webMAN_TR.xml");
	file_copy(APP_USRDIR "/xml/webMAN_ZH.xml" , XMBMANPLS_FEATS_DIR "/webMAN_ZH.xml");
	if(not_exists(XMBMANPLS_FEATS_DIR "/webMAN_XX.xml"))
		file_copy(APP_USRDIR "/xml/webMAN_XX.xml" , XMBMANPLS_FEATS_DIR "/webMAN_XX.xml");
	file_copy(APP_USRDIR "/xml/wm_apps.xml"   , XMBMANPLS_FEATS_DIR "/wm_apps.xml");

	file_copy(APP_USRDIR "/xmbm/bd.png"			, XMBMANPLS_IMAGES_DIR "/bd.png");
	file_copy(APP_USRDIR "/xmbm/blockpsn.png"	, XMBMANPLS_IMAGES_DIR "/blockpsn.png");
	file_copy(APP_USRDIR "/xmbm/cachefiles.png"	, XMBMANPLS_IMAGES_DIR "/cachefiles.png");
	file_copy(APP_USRDIR "/xmbm/dev_blind.png"	, XMBMANPLS_IMAGES_DIR "/dev_blind.png");
	file_copy(APP_USRDIR "/xmbm/filemanager.png", XMBMANPLS_IMAGES_DIR "/filemanager.png");
	file_copy(APP_USRDIR "/xmbm/gamedata.png"	, XMBMANPLS_IMAGES_DIR "/gamedata.png");
	file_copy(APP_USRDIR "/xmbm/gamefix.png"	, XMBMANPLS_IMAGES_DIR "/gamefix.png");
	file_copy(APP_USRDIR "/xmbm/gamesbrowser.png", XMBMANPLS_IMAGES_DIR "/gamesbrowser.png");
	file_copy(APP_USRDIR "/xmbm/icon_help.png"	, XMBMANPLS_IMAGES_DIR "/icon_help.png");
	file_copy(APP_USRDIR "/xmbm/multiman.png"	, XMBMANPLS_IMAGES_DIR "/multiman.png");
	file_copy(APP_USRDIR "/xmbm/network.png"	, XMBMANPLS_IMAGES_DIR "/network.png");
	file_copy(APP_USRDIR "/xmbm/package.png"	, XMBMANPLS_IMAGES_DIR "/package.png");
	file_copy(APP_USRDIR "/xmbm/pkgmanager.png"	, XMBMANPLS_IMAGES_DIR "/pkgmanager.png");
	file_copy(APP_USRDIR "/xmbm/refreshhtml.png", XMBMANPLS_IMAGES_DIR "/refreshhtml.png");
	file_copy(APP_USRDIR "/xmbm/refreshxml.png"	, XMBMANPLS_IMAGES_DIR "/refreshxml.png");
	file_copy(APP_USRDIR "/xmbm/restartps3.png"	, XMBMANPLS_IMAGES_DIR "/restartps3.png");
	file_copy(APP_USRDIR "/xmbm/settings.png"	, XMBMANPLS_IMAGES_DIR "/settings.png");
	file_copy(APP_USRDIR "/xmbm/shutdownps3.png", XMBMANPLS_IMAGES_DIR "/shutdownps3.png");
	file_copy(APP_USRDIR "/xmbm/sysinfo.png"	, XMBMANPLS_IMAGES_DIR "/sysinfo.png");
	file_copy(APP_USRDIR "/xmbm/sysfiles.png"	, XMBMANPLS_IMAGES_DIR "/sysfiles.png");
	file_copy(APP_USRDIR "/xmbm/tools.png"		, XMBMANPLS_IMAGES_DIR "/tools.png");
	file_copy(APP_USRDIR "/xmbm/unload.png"		, XMBMANPLS_IMAGES_DIR "/unload.png");
	file_copy(APP_USRDIR "/xmbm/usbredirect.png", XMBMANPLS_IMAGES_DIR "/usbredirect.png");
	file_copy(APP_USRDIR "/xmbm/vshmenu.png"	, XMBMANPLS_IMAGES_DIR "/vshmenu.png");
	file_copy(APP_USRDIR "/xmbm/slaunch.png"	, XMBMANPLS_IMAGES_DIR "/slaunch.png");
	file_copy(APP_USRDIR "/xmbm/webman.png"		, XMBMANPLS_IMAGES_DIR "/webman.png");
	file_copy(APP_USRDIR "/xmbm/eject.png"		, XMBMANPLS_IMAGES_DIR "/eject.png");
	file_copy(APP_USRDIR "/xmbm/setup.png"		, XMBMANPLS_IMAGES_DIR "/setup.png");
	file_copy(APP_USRDIR "/xmbm/refresh.png"	, XMBMANPLS_IMAGES_DIR "/refresh.png");
	//file_copy(APP_USRDIR "/xmbm/clear.png"	, XMBMANPLS_IMAGES_DIR "/clear.png");
	//file_copy(APP_USRDIR "/xmbm/cache.png"	, XMBMANPLS_IMAGES_DIR "/cache.png");
	//file_copy(APP_USRDIR "/xmbm/restart.png"	, XMBMANPLS_IMAGES_DIR "/restart.png");

	file_copy(APP_USRDIR "/icons/icon_wm_eject.png",  XMBMANPLS_IMAGES_DIR "/icon_wm_eject.png");
	file_copy(APP_USRDIR "/icons/icon_wm_insert.png", XMBMANPLS_IMAGES_DIR "/icon_wm_insert.png");
	file_copy(APP_USRDIR "/icons/icon_wm_ps3.png"  ,  XMBMANPLS_IMAGES_DIR "/icon_wm_ps3.png");

	sysLv2FsUnlink(XMLHOST_DIR "/mygames.xml");

	sysLv2FsUnlink(APP_USRDIR "webftp_server.sprx");
	sysLv2FsUnlink(APP_USRDIR "webftp_server_ps3mapi.sprx");
	sysLv2FsUnlink(APP_USRDIR "webftp_server_english.sprx");
	sysLv2FsUnlink(APP_USRDIR "webftp_server_ccapi.sprx");
	sysLv2FsUnlink(APP_USRDIR "webftp_server_rebug_cobra_multi23.sprx");
	sysLv2FsUnlink(APP_USRDIR "webftp_server_rebug_cobra_english.sprx");

	sysLv2FsMkdir(PLUGINS_DIR, DMODE);

	plugins_dir_exists = (file_exists(PLUGINS_DIR));

	// install vsh menu
	if(file_exists(RES_DIR))
	{
		// remove old resource path for images
		sysLv2FsUnlink(PLUGINS_DIR "/images/wm_vsh_menu.png");
		sysLv2FsUnlink(PLUGINS_DIR "/images/wm_vsh_menu_1.png");
		sysLv2FsUnlink(PLUGINS_DIR "/images/wm_vsh_menu_2.png");
		sysLv2FsUnlink(PLUGINS_DIR "/images/wm_vsh_menu_3.png");
		sysLv2FsUnlink(PLUGINS_DIR "/images/wm_vsh_menu_4.png");
		sysLv2FsUnlink(PLUGINS_DIR "/images/wm_vsh_menu_5.png");
		sysLv2FsUnlink(PLUGINS_DIR "/images/wm_vsh_menu_6.png");
		sysLv2FsUnlink(PLUGINS_DIR "/images/wm_vsh_menu_7.png");
		sysLv2FsUnlink(PLUGINS_DIR "/images/wm_vsh_menu_8.png");

		sysLv2FsUnlink(PLUGINS_DIR "/images/slaunch_fav.jpg");
		sysLv2FsUnlink(PLUGINS_DIR "/images/slaunch_PSX.jpg");
		sysLv2FsUnlink(PLUGINS_DIR "/images/slaunch_PS2.jpg");
		sysLv2FsUnlink(PLUGINS_DIR "/images/slaunch_PS3.jpg");
		sysLv2FsUnlink(PLUGINS_DIR "/images/slaunch_PSP.jpg");
		sysLv2FsUnlink(PLUGINS_DIR "/images/slaunch_ROMS.jpg");
		sysLv2FsUnlink(PLUGINS_DIR "/images/slaunch_video.jpg");

		sysLv2FsRmdir(PLUGINS_DIR "/images");

		// remove old path for VSH Menu and sLaunch Menu
		sysLv2FsUnlink(PLUGINS_DIR "/slaunch.sprx");
		sysLv2FsUnlink(PLUGINS_DIR "/wm_vsh_menu.sprx");

		// update images
		sysLv2FsMkdir(RES_DIR "/images", DMODE);
		file_copy(APP_USRDIR "/images/wm_vsh_menu.png",   RES_DIR "/images/wm_vsh_menu.png");
		file_copy(APP_USRDIR "/images/wm_vsh_menu_1.png", RES_DIR "/images/wm_vsh_menu_1.png");
		file_copy(APP_USRDIR "/images/wm_vsh_menu_2.png", RES_DIR "/images/wm_vsh_menu_2.png");
		file_copy(APP_USRDIR "/images/wm_vsh_menu_3.png", RES_DIR "/images/wm_vsh_menu_3.png");
		file_copy(APP_USRDIR "/images/wm_vsh_menu_4.png", RES_DIR "/images/wm_vsh_menu_4.png");
		file_copy(APP_USRDIR "/images/wm_vsh_menu_5.png", RES_DIR "/images/wm_vsh_menu_5.png");
		file_copy(APP_USRDIR "/images/wm_vsh_menu_6.png", RES_DIR "/images/wm_vsh_menu_6.png");
		file_copy(APP_USRDIR "/images/wm_vsh_menu_7.png", RES_DIR "/images/wm_vsh_menu_7.png");
		file_copy(APP_USRDIR "/images/wm_vsh_menu_8.png", RES_DIR "/images/wm_vsh_menu_8.png");

		file_copy(APP_USRDIR "/images/slaunch_fav.jpg",   RES_DIR "/images/slaunch_fav.jpg");
		file_copy(APP_USRDIR "/images/slaunch_PSX.jpg",   RES_DIR "/images/slaunch_PSX.jpg");
		file_copy(APP_USRDIR "/images/slaunch_PS2.jpg",   RES_DIR "/images/slaunch_PS2.jpg");
		file_copy(APP_USRDIR "/images/slaunch_PS3.jpg",   RES_DIR "/images/slaunch_PS3.jpg");
		file_copy(APP_USRDIR "/images/slaunch_PSP.jpg",   RES_DIR "/images/slaunch_PSP.jpg");
		file_copy(APP_USRDIR "/images/slaunch_ROMS.jpg",  RES_DIR "/images/slaunch_ROMS.jpg");
		file_copy(APP_USRDIR "/images/slaunch_video.jpg", RES_DIR "/images/slaunch_video.jpg");

		// update VSH Menu and sLaunch Menu
		file_copy(APP_USRDIR "/res/slaunch.sprx", RES_DIR "/slaunch.sprx");
		file_copy(APP_USRDIR "/res/wm_vsh_menu.sprx", RES_DIR "/wm_vsh_menu.sprx");
		file_copy(APP_USRDIR "/res/VshFpsCounter.sprx", RES_DIR "/VshFpsCounter.sprx");
		file_copy(APP_USRDIR "/res/VshFpsCounter.yaml", RES_DIR "/VshFpsCounter.yaml");
		file_copy(APP_USRDIR "/res/VshFpsCounterM.sprx",RES_DIR "/VshFpsCounterM.sprx");
	}

	// skip update custom language file
	if(not_exists(LANG_DIR "/LANG_XX.TXT"))
		file_copy(APP_USRDIR "/lang/LANG_XX.TXT", LANG_DIR "/LANG_XX.TXT");

	sysLv2FsUnlink(COMBO_DIR "wm_custom_r2_square"); // delete bad file

	// skip update custom combo file
	if(not_exists(COMBO_DIR "/wm_custom_r2_square"))
		file_copy(APP_USRDIR "/wm_custom_combo", COMBO_DIR "/wm_custom_r2_square");
	if(file_exists(COMBO_DIR "/wm_custom_r2_square"))
		sysLv2FsUnlink(TMP_DIR "/wm_custom_combo");

	// resource files sprx (wm_proxy, libfs, raw_iso, netiso, video_rec)
	sysLv2FsUnlink(TMP_DIR "/idle_plugin.sprx");
	sysLv2FsUnlink(TMP_DIR "/eula_cddb_plugin.sprx");

	file_copy(APP_USRDIR "/res/wm_proxy.sprx", RES_DIR "/wm_proxy.sprx");
	file_copy(APP_USRDIR "/res/psp_emulator.self", RES_DIR "/psp_emulator.self"); // supports decrypted MINIS.EDAT / MINIS2.EDAT (4.75-4.86)

	sysLv2FsUnlink(TMP_DIR "/libfs.sprx");
	file_copy(APP_USRDIR "/res/libfs.sprx", RES_DIR "/libfs.sprx");

	// copy raw_iso.sprx to dev_flash
	sysLv2FsStat(APP_USRDIR "/res/raw_iso.sprx", &stat);
	u64 raw_iso_size = stat.st_size;

	if(sysLv2FsStat(RES_DIR "/raw_iso.sprx", &stat) != SUCCESS || (stat.st_size != raw_iso_size))
	{
		file_copy(APP_USRDIR "/res/raw_iso.sprx", RES_DIR "/raw_iso.sprx");

		if(file_exists(RES_DIR "/raw_iso.sprx"))
		{
			sysLv2FsUnlink(PLUGINS_DIR "/raw_iso.sprx");
			sysLv2FsUnlink(HDDROOT_DIR "/raw_iso.sprx");
		}
	}

	// copy netiso.sprx to dev_flash
	sysLv2FsStat(APP_USRDIR "/res/netiso.sprx", &stat);
	u64 netiso_size = stat.st_size;

	if(sysLv2FsStat(RES_DIR "/res/netiso.sprx", &stat) != SUCCESS || (stat.st_size != netiso_size))
	{
		file_copy(APP_USRDIR "/res/netiso.sprx", RES_DIR "/netiso.sprx");

		if(file_exists(RES_DIR "/netiso.sprx"))
		{
			sysLv2FsUnlink(PLUGINS_DIR "/netiso.sprx");
			sysLv2FsUnlink(HDDROOT_DIR "/netiso.sprx");
		}
	}

	// copy standalone video recorder plugin (video_rec.sprx) to /wm_res folder
	sysLv2FsUnlink(PLUGINS_DIR "/video_rec.sprx");
	if((file_exists(RES_DIR)))
		file_copy(APP_USRDIR "/res/video_rec.sprx", RES_DIR "/video_rec.sprx");

	// update PRX+Mamba Loader
	if((file_exists(IRISMAN_USRDIR "/webftp_server.sprx")) || (file_exists(IRISMAN_USRDIR "/webftp_server_ps3mapi.sprx")))
	{
		sysLv2FsChmod(IRISMAN_USRDIR "/webftp_server.sprx", MODE);
		sysLv2FsUnlink(IRISMAN_USRDIR "/webftp_server.sprx");

		sysLv2FsChmod(IRISMAN_USRDIR "/webftp_server_ps3mapi.sprx", MODE);
		sysLv2FsUnlink(IRISMAN_USRDIR "/webftp_server_ps3mapi.sprx");

		if(full)
			file_copy(APP_USRDIR "/webftp_server_full.sprx", IRISMAN_USRDIR "/webftp_server.sprx");
		else if(lite)
			file_copy(APP_USRDIR "/webftp_server_lite.sprx", IRISMAN_USRDIR "/webftp_server.sprx");
		else
			file_copy(APP_USRDIR "/webftp_server_rebug_cobra_ps3mapi.sprx", IRISMAN_USRDIR "/webftp_server.sprx");
	}

	char line[255];

	// update PRX Loader
	if(file_exists(PRXLOADER_USRDIR "/plugins.txt"))
	{
		f = fopen(PRXLOADER_USRDIR "/plugins.txt", "r");
		while(fgets(line, 255, f) != NULL)
		{
			if(strstr(line,"webftp_server") != NULL)
			{
				fclose(f);
				strtok(line, "\r\n");
				sysLv2FsUnlink(line);
				file_copy(APP_USRDIR "/webftp_server_noncobra.sprx",line);
				goto cont;
			}
		}
		fclose(f);
		f = fopen(PRXLOADER_USRDIR "/plugins.txt", "a");
		fputs("\r\n" PRXLOADER_USRDIR "/webftp_server_noncobra.sprx", f);
		fclose(f);

		sysLv2FsChmod(PRXLOADER_USRDIR "/webftp_server_noncobra.sprx", MODE);
		sysLv2FsUnlink(PRXLOADER_USRDIR "/webftp_server_noncobra.sprx");

		file_copy(APP_USRDIR "/webftp_server_noncobra.sprx", PRXLOADER_USRDIR "/webftp_server_noncobra.sprx");
	}

cont:
	is_cobra(); // re-enable cobra if it's disabled

	// update dev_flash (rebug)
	if(file_exists(FLASH_VSH_MODULE_DIR "/webftp_server.sprx"))
	{
		if(not_exists("/dev_blind"))
			sys_fs_mount("CELL_FS_IOS:BUILTIN_FLSH1", "CELL_FS_FAT", "/dev_blind", 0);

		sysLv2FsChmod(REBUG_VSH_MODULE_DIR "/webftp_server.sprx", MODE);
		sysLv2FsUnlink(REBUG_VSH_MODULE_DIR "/webftp_server.sprx");

		sysLv2FsChmod(REBUG_VSH_MODULE_DIR "/webftp_server.sprx.bak", MODE);
		sysLv2FsUnlink(REBUG_VSH_MODULE_DIR "/webftp_server.sprx.bak");

		if(full)
			file_copy(APP_USRDIR "/webftp_server_full.sprx", REBUG_VSH_MODULE_DIR "/webftp_server.sprx");
		else if(lite)
			file_copy(APP_USRDIR "/webftp_server_lite.sprx", REBUG_VSH_MODULE_DIR "/webftp_server.sprx");
		else
			file_copy(APP_USRDIR "/webftp_server_rebug_cobra_ps3mapi.sprx", REBUG_VSH_MODULE_DIR "/webftp_server.sprx");


		// delete webMAN from hdd0
		if((file_exists(REBUG_VSH_MODULE_DIR "/webftp_server.sprx")))
		{
			sysLv2FsChmod(HDDROOT_DIR "/webftp_server.sprx", MODE);
			sysLv2FsUnlink(HDDROOT_DIR "/webftp_server.sprx");

			sysLv2FsChmod(PLUGINS_DIR "/webftp_server.sprx", MODE);
			sysLv2FsUnlink(PLUGINS_DIR "/webftp_server.sprx");

			if(file_exists(HDDROOT_DIR "/boot_plugins.txt"))
			{
				f = fopen(HDDROOT_DIR "/boot_plugins.txt", "r");
				while(fgets(line, 255, f) != NULL)
				{
					if((strstr(line,"webftp_server") != NULL) && (strstr(line,"/dev_blind") == NULL) && (strstr(line,"/dev_flash") == NULL))
					{
						strtok(line, "\r\n");
						sysLv2FsChmod(line, MODE);
						sysLv2FsUnlink(line);
						break;
					}
				}
				fclose(f);
			}

			if(file_exists(HDDROOT_DIR "/boot_plugins_nocobra.txt"))
			{
				f = fopen(HDDROOT_DIR "/boot_plugins_nocobra.txt", "r");
				while(fgets(line, 255, f) != NULL)
				{
					if((strstr(line,"webftp_server") != NULL) && (strstr(line,"/dev_blind") == NULL) && (strstr(line,"/dev_flash") == NULL))
					{
						strtok(line, "\r\n");
						sysLv2FsChmod(line, MODE);
						sysLv2FsUnlink(line);
						break;
					}
				}
				fclose(f);
			}

			if(file_exists(HDDROOT_DIR "/boot_plugins_nocobra_dex.txt"))
			{
				f = fopen(HDDROOT_DIR "/boot_plugins_nocobra_dex.txt", "r");
				while(fgets(line, 255, f) != NULL)
				{
					if((strstr(line,"webftp_server") != NULL) && (strstr(line,"/dev_blind") == NULL) && (strstr(line,"/dev_flash") == NULL))
					{
						strtok(line, "\r\n");
						sysLv2FsChmod(line, MODE);
						sysLv2FsUnlink(line);
						break;
					}
				}
				fclose(f);
			}

		}

		// reboot
		sysLv2FsUnlink(TMP_DIR "/turnoff");
		//{lv2syscall4(379,0x200,0,0,0); return_to_user_prog(int);}
		//{lv2syscall4(379,0x1200,0,0,0); return_to_user_prog(int);}
		{lv2syscall3(SC_SYS_POWER, SYS_REBOOT, 0, 0); return_to_user_prog(int);}
		//{lv2syscall3(SC_SYS_POWER, SYS_REBOOT, 0, 0);}

		return SUCCESS;
	}
	else if(file_exists(FLASH_VSH_MODULE_DIR "/webftp_server.sprx.bak"))
	{
		if(not_exists("/dev_blind"))
			sys_fs_mount("CELL_FS_IOS:BUILTIN_FLSH1", "CELL_FS_FAT", "/dev_blind", 0);

		sysLv2FsChmod(REBUG_VSH_MODULE_DIR "/webftp_server.sprx.bak", MODE);
		sysLv2FsUnlink(REBUG_VSH_MODULE_DIR "/webftp_server.sprx.bak");

		if(full)
			file_copy(APP_USRDIR "/webftp_server_full.sprx", REBUG_VSH_MODULE_DIR "/webftp_server.sprx.bak");
		else if(lite)
			file_copy(APP_USRDIR "/webftp_server_lite.sprx", REBUG_VSH_MODULE_DIR "/webftp_server.sprx.bak");
		else
			file_copy(APP_USRDIR "/webftp_server_rebug_cobra_ps3mapi.sprx", REBUG_VSH_MODULE_DIR "/webftp_server.sprx.bak");
	}


	u32 cobra_version = 0;
	if((file_exists("/dev_flash/rebug/cobra")) && (sys_get_version2(&cobra_version) >= 0 && cobra_version >= 0x0810))
	{
		// parse boot_plugins_nocobra.txt (update existing path)
		if(file_exists(HDDROOT_DIR "/boot_plugins_nocobra.txt"))
		{
			f = fopen(HDDROOT_DIR "/boot_plugins_nocobra.txt", "r");
			while(fgets(line, 255, f) != NULL)
			{
				if(strstr(line,"webftp_server") != NULL)
				{
					fclose(f);
					strtok(line, "\r\n");
					sysLv2FsChmod(line, MODE);
					sysLv2FsUnlink(line);
					if(full_on_nocobra)
						file_copy(APP_USRDIR "/webftp_server_full.sprx", line);
					else
						file_copy(APP_USRDIR "/webftp_server_noncobra.sprx", line);
					break;
				}
			}
			fclose(f);
		}
		else
		{
			// append line to boot_plugins_nocobra.txt
			if(file_exists(HDDROOT_DIR "/boot_plugins_nocobra.txt"))
				f = fopen(HDDROOT_DIR "/boot_plugins_nocobra.txt", "a");
			else
				f = fopen(HDDROOT_DIR "/boot_plugins_nocobra.txt", "w");
			if(plugins_dir_exists)
			{
				if(full_on_nocobra)
				{
					file_copy(APP_USRDIR "/webftp_server_full.sprx", PLUGINS_DIR "/webftp_server.sprx");
					fputs("\r\n" PLUGINS_DIR "/webftp_server.sprx", f);
				}
				else
				{
					file_copy(APP_USRDIR "/webftp_server_noncobra.sprx", PLUGINS_DIR "/webftp_server_noncobra.sprx");
					fputs("\r\n" PLUGINS_DIR "/webftp_server_noncobra.sprx", f);
				}
			}
			else
			{
				if(full_on_nocobra)
				{
					file_copy(APP_USRDIR "/webftp_server_full.sprx", HDDROOT_DIR "/webftp_server.sprx");
					fputs("\r\n" HDDROOT_DIR "/webftp_server.sprx", f);
				}
				else
				{
					file_copy(APP_USRDIR "/webftp_server_noncobra.sprx", HDDROOT_DIR "/webftp_server_noncobra.sprx");
					fputs("\r\n" HDDROOT_DIR "/webftp_server_noncobra.sprx", f);
				}
			}
			fclose(f);
		}

		// parse boot_plugins_nocobra_dex.txt (update existing path)
		if(file_exists(HDDROOT_DIR "/boot_plugins_nocobra_dex.txt"))
		{
			f = fopen(HDDROOT_DIR "/boot_plugins_nocobra_dex.txt", "r");
			while(fgets(line, 255, f) != NULL)
			{
				if(strstr(line,"webftp_server") != NULL)
				{
					fclose(f);
					strtok(line, "\r\n");
					sysLv2FsChmod(line, MODE);
					sysLv2FsUnlink(line);
					if(full_on_nocobra)
						file_copy(APP_USRDIR "/webftp_server_full.sprx", line);
					else
						file_copy(APP_USRDIR "/webftp_server_noncobra.sprx", line);
					break;
				}
			}
			fclose(f);
		}
		else
		{
			// append line to boot_plugins_nocobra_dex.txt
			if(file_exists(HDDROOT_DIR "/boot_plugins_nocobra_dex.txt"))
				f = fopen(HDDROOT_DIR "/boot_plugins_nocobra_dex.txt", "a");
			else
				f = fopen(HDDROOT_DIR "/boot_plugins_nocobra_dex.txt", "w");
			if(plugins_dir_exists)
			{
				if(full_on_nocobra)
				{
					file_copy(APP_USRDIR "/webftp_server_full.sprx", PLUGINS_DIR "/webftp_server.sprx");
					fputs("\r\n" PLUGINS_DIR "/webftp_server.sprx", f);
				}
				else
				{
					file_copy(APP_USRDIR "/webftp_server_noncobra.sprx", PLUGINS_DIR "/webftp_server_noncobra.sprx");
					fputs("\r\n" PLUGINS_DIR "/webftp_server_noncobra.sprx", f);
				}
			}
			else
			{
				if(full_on_nocobra)
				{
					file_copy(APP_USRDIR "/webftp_server_full.sprx", HDDROOT_DIR "/webftp_server.sprx");
					fputs("\r\n" HDDROOT_DIR "/webftp_server.sprx", f);
				}
				else
				{
					file_copy(APP_USRDIR "/webftp_server_noncobra.sprx", HDDROOT_DIR "/webftp_server_noncobra.sprx");
					fputs("\r\n" HDDROOT_DIR "/webftp_server_noncobra.sprx", f);
				}
			}
			fclose(f);
		}
	}

	// update boot_plugins.txt
	if(lite || full || is_cobra())
	{
		// parse boot_plugins.txt (update existing path)
		if(file_exists(HDDROOT_DIR "/boot_plugins.txt"))
		{
			f = fopen(HDDROOT_DIR "/boot_plugins.txt", "r");
			while(fgets(line, 255, f) != NULL)
			{
				if(strstr(line,"webftp_server") != NULL)
				{
					fclose(f);
					strtok(line, "\r\n");
					sysLv2FsChmod(line, MODE);
					sysLv2FsUnlink(line);
					if(full)
						file_copy(APP_USRDIR "/webftp_server_full.sprx", line);
					else if(lite)
						file_copy(APP_USRDIR "/webftp_server_lite.sprx", line);
					else
						file_copy(APP_USRDIR "/webftp_server_rebug_cobra_ps3mapi.sprx", line);
					goto exit;
				}
			}
			fclose(f);
		}

		// append line to boot_plugins.txt
		if(file_exists(HDDROOT_DIR "/boot_plugins.txt"))
			f = fopen(HDDROOT_DIR "/boot_plugins.txt", "a");
		else
			f = fopen(HDDROOT_DIR "/boot_plugins.txt", "w");
		if(plugins_dir_exists)
			fputs("\r\n" PLUGINS_DIR "/webftp_server.sprx", f);
		else
			fputs("\r\n" HDDROOT_DIR "/webftp_server.sprx", f);
		fclose(f);

		// delete old sprx
		sysLv2FsChmod(HDDROOT_DIR "/webftp_server.sprx", MODE);
		sysLv2FsUnlink(HDDROOT_DIR "/webftp_server.sprx");

		sysLv2FsChmod(PLUGINS_DIR "/webftp_server.sprx", MODE);
		sysLv2FsUnlink(PLUGINS_DIR "/webftp_server.sprx");

		sysLv2FsChmod(HDDROOT_DIR "/webftp_server_ps3mapi.sprx", MODE);
		sysLv2FsUnlink(HDDROOT_DIR "/webftp_server_ps3mapi.sprx");

		sysLv2FsChmod(PLUGINS_DIR "/webftp_server_ps3mapi.sprx", MODE);
		sysLv2FsUnlink(PLUGINS_DIR "/webftp_server_ps3mapi.sprx");

		// copy ps3mapi/cobra/rebug/lite sprx
		if(plugins_dir_exists)
		{
			if(full)
				file_copy(APP_USRDIR "/webftp_server_full.sprx", PLUGINS_DIR "/webftp_server.sprx");
			else if(lite)
				file_copy(APP_USRDIR "/webftp_server_lite.sprx", PLUGINS_DIR "/webftp_server.sprx");
			else
				file_copy(APP_USRDIR "/webftp_server_rebug_cobra_ps3mapi.sprx", PLUGINS_DIR "/webftp_server.sprx");

			file_copy(APP_USRDIR "/webftp_server_lite.sprx", PLUGINS_DIR "/webftp_server_lite.sprx");
		}
		else
		{
			if(full)
				file_copy(APP_USRDIR "/webftp_server_full.sprx", HDDROOT_DIR "/webftp_server.sprx");
			else if(lite)
				file_copy(APP_USRDIR "/webftp_server_lite.sprx", HDDROOT_DIR "/webftp_server.sprx");
			else
				file_copy(APP_USRDIR "/webftp_server_rebug_cobra_ps3mapi.sprx", HDDROOT_DIR "/webftp_server.sprx");
		}
	}

	// update mamba_plugins.txt
	if(is_mamba())
	{
		// parse mamba_plugins.txt (update existing path)
		if(file_exists(HDDROOT_DIR "/mamba_plugins.txt"))
		{
			f = fopen(HDDROOT_DIR "/mamba_plugins.txt", "r");
			while(fgets(line, 255, f) != NULL)
			{
				if(strstr(line,"webftp_server") != NULL)
				{
					fclose(f);
					strtok(line, "\r\n");
					sysLv2FsChmod(line, MODE);
					sysLv2FsUnlink(line);

					if(full)
						file_copy(APP_USRDIR "/webftp_server_full.sprx", line);
					else if(lite)
						file_copy(APP_USRDIR "/webftp_server_lite.sprx", line);
					else
						file_copy(APP_USRDIR "/webftp_server_rebug_cobra_ps3mapi.sprx", line);
					goto exit;
				}
			}
			fclose(f);
		}

		// append line to mamba_plugins.txt (Mamba/PRX Loader - PS3MAPI)
		if(file_exists(HDDROOT_DIR "/mamba_plugins.txt"))
			f = fopen(HDDROOT_DIR "/mamba_plugins.txt", "a");
		else
			f = fopen(HDDROOT_DIR "/mamba_plugins.txt", "w");
		if(plugins_dir_exists)
			fputs("\r\n" PLUGINS_DIR "/webftp_server.sprx", f);
		else
			fputs("\r\n" HDDROOT_DIR "/webftp_server.sprx", f);
		fclose(f);

		// delete old sprx
		sysLv2FsChmod(HDDROOT_DIR "/webftp_server_ps3mapi.sprx", MODE);
		sysLv2FsUnlink(HDDROOT_DIR "/webftp_server_ps3mapi.sprx");

		sysLv2FsChmod(PLUGINS_DIR "/webftp_server_ps3mapi.sprx", MODE);
		sysLv2FsUnlink(PLUGINS_DIR "/webftp_server_ps3mapi.sprx");

		// copy ps3mapi sprx
		if(plugins_dir_exists)
		{
			if(full)
				file_copy(APP_USRDIR "/webftp_server_full.sprx", PLUGINS_DIR "/webftp_server.sprx");
			else if(lite)
				file_copy(APP_USRDIR "/webftp_server_lite.sprx", PLUGINS_DIR "/webftp_server.sprx");
			else
				file_copy(APP_USRDIR "/webftp_server_rebug_cobra_ps3mapi.sprx", PLUGINS_DIR "/webftp_server.sprx");
		}
		else
		{
			if(full)
				file_copy(APP_USRDIR "/webftp_server_full.sprx", HDDROOT_DIR "/webftp_server.sprx");
			else if(lite)
				file_copy(APP_USRDIR "/webftp_server_lite.sprx", HDDROOT_DIR "/webftp_server.sprx");
			else
				file_copy(APP_USRDIR "/webftp_server_rebug_cobra_ps3mapi.sprx", HDDROOT_DIR "/webftp_server.sprx");
		}
	}

	// update prx_plugins.txt (PRX LOADER)
	if(file_exists(HDDROOT_DIR "/prx_plugins.txt"))
	{
		// parse prx_plugins.txt (update existing path)
		f = fopen(HDDROOT_DIR "/prx_plugins.txt", "r");
		while(fgets(line, 255, f) != NULL)
		{
			if(strstr(line,"webftp_server") != NULL)
			{
				fclose(f);
				strtok(line, "\r\n");
				sysLv2FsUnlink(line);
				file_copy(APP_USRDIR "/webftp_server_noncobra.sprx",line);
				goto exit;
			}
		}
		fclose(f);

		// append line to prx_plugins.txt
		f = fopen(HDDROOT_DIR "/prx_plugins.txt", "a");

		if(plugins_dir_exists)
			fputs("\r\n" PLUGINS_DIR "/webftp_server_noncobra.sprx", f);
		else
			fputs("\r\n" HDDROOT_DIR "/webftp_server_noncobra.sprx", f);
		fclose(f);

		// delete old sprx
		sysLv2FsChmod(HDDROOT_DIR "/webftp_server_noncobra.sprx", MODE);
		sysLv2FsUnlink(HDDROOT_DIR "/webftp_server_noncobra.sprx");

		sysLv2FsChmod(PLUGINS_DIR "/webftp_server_noncobra.sprx", MODE);
		sysLv2FsUnlink(PLUGINS_DIR "/webftp_server_noncobra.sprx");

		// copy non cobra sprx
		if(plugins_dir_exists)
			file_copy(APP_USRDIR "/webftp_server_noncobra.sprx", PLUGINS_DIR "/webftp_server_noncobra.sprx");
		else
			file_copy(APP_USRDIR "/webftp_server_noncobra.sprx", HDDROOT_DIR "/webftp_server_noncobra.sprx");
	}
	// exit
exit:

	// update category_game.xml (add fb.xml)
	if(add_mygame() != -2);

	//if(is_ps3hen() == 0x1337) return SUCCESS;

	// reboot
	sysLv2FsUnlink(TMP_DIR "/turnoff");
	//{lv2syscall4(379,0x200,0,0,0); return_to_user_prog(int);}
	//{lv2syscall4(379,0x1200,0,0,0); return_to_user_prog(int);}
	{lv2syscall3(SC_SYS_POWER, SYS_REBOOT, 0, 0); return_to_user_prog(int);}

	return SUCCESS;
}
