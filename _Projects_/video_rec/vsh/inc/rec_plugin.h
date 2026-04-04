// Mysis game_plugin.h v0.1
// 3141card (C style version)
#ifndef __REC_PLUGIN_H__
#define __REC_PLUGIN_H__

typedef struct rec_plugin_interface_t
{
	uint32_t (*DoUnk0)(void); 
	uint32_t (*start)(void); //RecStart
	uint32_t (*stop)(void); //RecStop
	uint32_t (*close)(int32_t isdiscard); 
	uint32_t (*geti)(int32_t giprm);  // RecGetInfo
	uint32_t (*md)(void * mdarg, int32_t); //RecSetInfo
	uint32_t (*etis)(int32_t start_time_msec); //RecSetInfo
	uint32_t (*etie)(int32_t end_time_msec); //RecSetInfo
} rec_plugin_interface;

rec_plugin_interface *rec_interface;

#endif // __REC_PLUGIN_H__
