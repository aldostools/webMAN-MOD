#include <sdk_version.h>
#include <string.h>
#include <cell/rtc.h>

#include <sys/prx.h>
#include <sys/socket.h>
#include <sys/ppu_thread.h>

#include <netinet/in.h>

#include "types.h"

SYS_MODULE_INFO(wm_proxy, 0, 1, 1);
SYS_MODULE_START(prx_start);
SYS_MODULE_STOP(prx_stop);
SYS_MODULE_EXIT(prx_stop);

int prx_start(size_t args, void *argp);
int prx_stop(void);

extern char *stdc_C5C09834(const char *str1, const char *str2);							// strstr()
inline char* strstr(const char *str1, const char *str2) {return stdc_C5C09834(str1, str2);}

static int (*vshtask_notify)(int, const char *) = NULL;

static void * getNIDfunc(const char * vsh_module, uint32_t fnid)
{
	uint32_t table = (*(uint32_t*)0x1008C) + 0x984; // vsh table address

	while(((uint32_t)*(uint32_t*)table) != 0)
	{
		uint32_t* export_stru_ptr = (uint32_t*)*(uint32_t*)table;

		const char* lib_name_ptr =  (const char*)*(uint32_t*)((char*)export_stru_ptr + 0x10);

		if(strncmp(vsh_module, lib_name_ptr, strlen(lib_name_ptr))==0)
		{
			uint32_t lib_fnid_ptr = *(uint32_t*)((char*)export_stru_ptr + 0x14);
			uint32_t lib_func_ptr = *(uint32_t*)((char*)export_stru_ptr + 0x18);
			uint16_t count = *(uint16_t*)((char*)export_stru_ptr + 6); // number of exports
			for(int i = 0; i < count; i++)
			{
				if(fnid == *(uint32_t*)((char*)lib_fnid_ptr + i*4))
				{
					return (void**)*((uint32_t*)(lib_func_ptr) + i);
				}
			}
		}
		table += 4;
	}
	return 0;
}

static void show_msg(const char* msg)
{
	//if(!vshtask_notify)
		vshtask_notify = getNIDfunc("vshtask", 0xA02D46E7);

	//if(strlen(msg)>128) msg[128]=0;

	if(vshtask_notify)
		vshtask_notify(0, msg);
}

static void wm_plugin_init (int view);
static int  wm_plugin_start(void * view);
//static int  wm_plugin_stop (void);
static void wm_plugin_exit (void);
static void wm_plugin_action(const char * action);
static int setInterface(unsigned int view);

static int (*plugin_SetInterface)(int view, int interface, void * Handler);
static int (*plugin_SetInterface2)(int view, int interface, void * Handler);

static void *wm_plugin_action_if[3] = {(void*)(wm_plugin_action), 0, 0};

static void wm_plugin_init (int view)		{plugin_SetInterface( view, 0x41435430 /*ACT0*/, wm_plugin_action_if);}
static int  wm_plugin_start(void * view)	{return SYS_PRX_START_OK;}
//static int  wm_plugin_stop (void)			{return SYS_PRX_STOP_OK;}
static void wm_plugin_exit (void)			{return;}

#define wm_plugin_stop prx_stop

static void *wm_plugin_functions[4] =
	{
		(void*)(wm_plugin_init),
		(int* )(wm_plugin_start),
		(int* )(wm_plugin_stop),
		(void*)(wm_plugin_exit)
	};

static int setInterface(unsigned int view)
{
	plugin_SetInterface  = getNIDfunc("paf", 0xA1DC401);
	plugin_SetInterface2 = getNIDfunc("paf", 0x3F7CB0BF);
	plugin_SetInterface2(view, 1, (void*)wm_plugin_functions);
	return 0;
}

static int connect_to_webman(void)
{
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0)
	{
		return -1;
	}

	struct sockaddr_in sin;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0x7F000001;	//127.0.0.1 (localhost)
	sin.sin_port = htons(80);			//http port (80)

	struct timeval tv;
	tv.tv_usec = 0;

	tv.tv_sec = 3;
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		return -1;
	}

	tv.tv_sec = 60;
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	return s;
}

static void sclose(int *socket_e)
{
	//if(*socket_e != -1)
	{
		shutdown(*socket_e, SHUT_RDWR);
		socketclose(*socket_e);
		//*socket_e = -1;
	}
}

#define HTML_RECV_SIZE	2048
#define HTML_RECV_LAST	2045 // HTML_RECV_SIZE-3
#define HTML_RECV_LASTP	2042 // HTML_RECV_LAST-3

static void wm_plugin_action(const char * action)
{
	int s = connect_to_webman();
	if(s >= 0)
	{
		char proxy_action[HTML_RECV_SIZE];
		memcpy(proxy_action, "GET ", 4);

		u32 pa = 4;

		if(*action == 'G') action += 4;  // skip GET
		if(*action != '/') action += 16; // using http://127.0.0.1/ or http://localhost/

		if(*action == '/')
		{
			u8 is_path = !strstr(action, ".ps") && !strstr(action, "_ps");

			for(;*action && (pa < HTML_RECV_LAST); action++, pa++)
			{
				if(*action == 0x20)
					proxy_action[pa] = 0x2B;
				else if((*action == 0x2B) && is_path)
				{
					if(pa > HTML_RECV_LASTP) break;
					memcpy(proxy_action + pa, "%2B", 3); pa += 2; //+
				}
				else
					proxy_action[pa] = *action;
			}

			memcpy(proxy_action + pa, "\r\n\0", 3); pa +=2;
			send(s, proxy_action, pa, 0);
		}
		sclose(&s);
	}
	else show_msg("webMAN not ready!");
}

int prx_start(size_t args, void *argp)
{
	setInterface(*(unsigned int*)argp);
	return SYS_PRX_RESIDENT;
}

int prx_stop(void)
{
	return SYS_PRX_STOP_OK;
}
