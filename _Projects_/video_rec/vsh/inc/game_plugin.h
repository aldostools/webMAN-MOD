// Mysis game_plugin.h v0.1
// 3141card (C style version)
#ifndef __GAME_PLUGIN_H__
#define __GAME_PLUGIN_H__

typedef struct game_plugin_interface_t
{
	uint32_t (*DoUnk0)(void); // set Widget "page_game_main" and activate
	uint32_t (*DoUnk1)(void *); // uint8_t [0x5B8]
	uint32_t (*DoUnk2)(void *); // uint8_t [0x230]
	uint32_t (*DoUnk3)(int); // 3 = "CB" Category
	uint32_t (*DoUnk4)(int, void *); // uint8_t [0x1A0]
	uint32_t (*DoUnk5)(void);
	uint32_t (*DoUnk6)(void);
	uint32_t (*DoUnk7)(void);
	uint32_t (*DoUnk8)(void *); // uint8_t [0x114]
	uint32_t (*DoUnk9)(void *); // uint8_t [0x80]
	uint32_t (*DoUnk10)(char *); // char [8]
	uint32_t (*DoUnk11)(unsigned long, int);
	uint32_t (*DoUnk12)(void);
	uint32_t (*DoUnk13)(void);
	uint32_t (*DoUnk14)(void);
	uint32_t (*GetExecAppType)(int *, int *); // apptype, extended type?
	uint32_t (*DoUnk16)(int *);
	uint32_t (*DoUnk17)(void);
	uint32_t (*DoUnk18)(int *, char *); // char [0x20]
	uint32_t (*DoUnk19)(int *, char *, char *); // char [0x20]
	uint32_t (*DoUnk20)(void *); // uint8_t [0x5B8]
	uint32_t (*DoUnk21)(void);
	uint32_t (*commerce2ExecuteStoreBrowse)(int, char *, int, int); //targetType,targetId,flags,userdata
	uint32_t (*DoUnk23)(void *); // uint8_t [0xA4]
	uint32_t (*DoUnk24)(void *); // uint8_t [0xA4]
	uint32_t (*wakeupWithGameExit)(char *, int); // char [0x800], userdata
	uint32_t (*commerce2Reboot4PrgDl)(int); // taskId
	uint32_t (*DoUnk27)(char *); // [0x800]
	uint32_t (*DoUnk28)(void);
	uint32_t (*DoUnk29)(void *); // [0xxA4]
	uint32_t (*commerce2GetOptions)(int *); // userdata
	uint32_t (*GetUsrdataOnGameExit)(int *);
	uint32_t (*GetManualPath)(char *); // [0x80]
	uint32_t (*DoUnk33)(void);
	uint32_t (*DoUnk34)(char *); // [0x20]
	uint32_t (*DoUnk35)(char *); // [0x20]
	uint32_t (*DoUnk36)(int, char *); // no size check
	uint32_t (*DoUnk37)(void);
	uint32_t (*DoUnk38)(unsigned long);
	uint32_t (*DoUnk39)(char *); // titleId[0x20]
	uint32_t (*DoUnk40)(char *, int *, int); // titleId[0x20]
	uint32_t (*DoUnk41)(char *, char *, int); // titleId[0x20], char [4]
	uint32_t (*DoUnk42)(char *, int, char *, int); //titleid, flags
	uint32_t (*DoUnk43)(void);
	uint32_t (*DoUnk44)(void);
	uint32_t (*initGameData)(int, int); // memContainer, NoCalcFlag
	uint32_t (*EndGameData)(void);
	uint32_t (*getGameDataStat)(char *, char *, void *); // [0x1450]
	uint32_t (*updateGameData)(char *, char *, void *, void *);//callback, sysparam[0x1450]
	uint32_t (*DoUnk49)(int, int, int, void *, char *);
	uint32_t (*DoUnk50)(void);
	uint32_t (*DoUnk51)(void);
	uint32_t (*cacheInit)(void *, void *); // callback, SysCacheParam[0x444]
	uint32_t (*cacheClear)(void);
	uint32_t (*GetBootInfo)(void *);// [0x20]
	uint32_t (*GetTitleId)(void *);
	uint32_t (*kbGetSize)(int *, int);
	uint32_t (*SetSysVersion)(char *);
	uint32_t (*GetDiscInfo)(void *); //[0x20]
	uint32_t (*DoUnk59)(int, int, void *, int);
	uint32_t (*SetEjectMode)(int); //int mode
	uint32_t (*gameExec)(char *,char *,int,int,int,int,int);
	uint32_t (*getList)(int *, int *, int *, int *, int);
	uint32_t (*DoUnk63_GetBootInfo)(int *, char *, int *); //[0x20]
	uint32_t (*SetExecDataParam)(int *);
	uint32_t (*GetExitInfo)(int *, int *, char *, int *, int *);
	uint32_t (*HomePath)(char *);
	uint32_t (*DeleteGame)(char *, int);
	uint32_t (*OptionPath)(char *, char *);
	uint32_t (*ExportPath)(char *);
	uint32_t (*ImportPath)(char *);
	uint32_t (*Open)(int,char *, int, int *);
	uint32_t (*BootCheck)(int *, int *, int, int *, char *);
	uint32_t (*PatchCheck)(int,int *);
	uint32_t (*Create)(void *, char *, char *, int);
	uint32_t (*getInt)(int, int*, int);
	uint32_t (*getStr)(int, char *, int, int);
	uint32_t (*setInt)(int, int, int);
	uint32_t (*setStr)(int, char *, int);
	uint32_t (*Close)(char *, char *);
	uint32_t (*DoUnk80)(int, int, char *);
	uint32_t (*getSizeKB)(int *);
	uint32_t (*tInstall)(char *, char *, int, int);
	uint32_t (*mtInstall)(int);
	uint32_t (*mtWrite)(int, void *);
	uint32_t (*mtClose)(int, int);
	uint32_t (*getUpPath)(char *);
	uint32_t (*getWConPath)(char *);
	uint32_t (*delGameData)(char *);
	uint32_t (*getDevIdList)(int *, void *);
	uint32_t (*getDevInfo)(unsigned long, void *);
	uint32_t (*getUsbDevInfo)(unsigned long,void *);
	uint32_t (*storageMode)(void);
	uint32_t (*notifyCtrl)(int);
	uint32_t (*allreadyDisp)(void);
	uint32_t (*pspLoad)(void *, int);
	uint32_t (*pspSave)(void *, int);
	uint32_t (*vmcAssign)(int,char *, void *, int);
	uint32_t (*ps1End)(int);
	uint32_t (*SetPadRumble)(int);
	uint32_t (*DoUnk100)(void *, int);
	uint32_t (*DoUnk101)(void);
	uint32_t (*DoUnk102)(char *);
	uint32_t (*DoUnk103_DeleteGame)(char *);
	uint32_t (*DoUnk104)(void);
} game_plugin_interface;

game_plugin_interface *game_interface;

#endif // __GAME_PLUGIN_H__
