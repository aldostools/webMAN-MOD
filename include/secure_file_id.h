#ifdef SECURE_FILE_ID

#include "../vsh/ps3_savedata_plugin.h"

static int (*ps3_savedata_plugin_init)(void *);

static bool securfileid_hooked = false;

static int (*DoUnk13_)(int*, int*, char*, int, void *) = 0;

static int ps3_interface_function13_hook(int* r3, int* r4, char* filename, int r6, void * key)
{
	char buffer[MAX_PATH_LEN];
	sprintf(buffer, "Filename: %s\nSecure File ID: %08X%08X%08X%08X", filename, *(int*)key, *((int*)key + 1), *((int*)key + 2), *((int*)key + 3));
	save_file("/dev_hdd0/secureid.log", buffer, APPEND_TEXT);

	return DoUnk13_(r3, r4, filename, r6, key);
}

static int ps3_savedata_plugin_init__(void * view){};

static int (*ps3_savedata_plugin_init_bk)(void * view) = ps3_savedata_plugin_init__;

static int ps3_savedata_plugin_init_hook(void * view)
{
	ps3_savedata_plugin_game_interface * ps3_savedata_interface;

	ps3_savedata_interface = (ps3_savedata_plugin_game_interface *) plugin_GetInterface(View_Find("ps3_savedata_plugin"),1);
	if(ps3_savedata_interface->DoUnk13 != ps3_interface_function13_hook)
	{
		DoUnk13_ = ps3_savedata_interface->DoUnk13;
		ps3_savedata_interface->DoUnk13 = ps3_interface_function13_hook;
		save_file("/dev_hdd0/secureid.log", "Secure File Id Hooked", APPEND_TEXT);
	}

	return ps3_savedata_plugin_init_bk(view);
}

static void restore_func(void * original,void * backup)
{
	memcpy(original,backup,8); // copy original function offset + toc
}

static void hook_func(void * original,void * backup, void * hook_function)
{
	memcpy(backup, original,8); // copy original function offset + toc
	memcpy(original, hook_function ,8); // replace original function offset + toc by hook
}

static void hook_savedata_plugin(void)
{
	if(securfileid_hooked)
	{
		restore_func((void *)ps3_savedata_plugin_init, (void*)ps3_savedata_plugin_init_bk);
		securfileid_hooked = false;
	}
	else
	{
		// init
		ps3_savedata_plugin_init = getNIDfunc("vshmain", 0xBEF63A14, -(0x130*2));

		hook_func((void *)ps3_savedata_plugin_init, (void*)ps3_savedata_plugin_init_bk, (void*)ps3_savedata_plugin_init_hook );
		securfileid_hooked = true;
	}
}

#endif