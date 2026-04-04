#include <sys/prx.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/syscall.h>
#include <cell/cell_fs.h>

#include <sys/sys_time.h>
#include <sys/timer.h>
#include <sys/time_util.h>

#include <stdbool.h>

#include <cell/pad.h>
#include <cell/rtc.h>

#include "vsh_exports.h"


SYS_MODULE_INFO(VIDEO_REC, 0, 1, 0);
SYS_MODULE_START(video_rec_start);
SYS_MODULE_STOP(video_rec_stop);

static sys_ppu_thread_t thread_id=-1;
static int32_t done = 0;

int32_t video_rec_start(uint64_t arg);
int32_t video_rec_stop(void);

uint32_t *recOpt = NULL;              // recording utility vsh options struct
int32_t (*reco_open)(int32_t) = NULL; // base pointer

static inline void _sys_ppu_thread_exit(uint64_t val)
{
	system_call_1(41, val);
}

static inline sys_prx_id_t prx_get_module_id_by_address(void *addr)
{
	system_call_1(461, (uint64_t)(uint32_t)addr);
	return (int)p1;
}

//bool rec_start(void);
/***********************************************************************
*
***********************************************************************/

#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_512K_30FPS    ( 0x0000 )
#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_768K_30FPS    ( 0x0010 )

#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_MIDDLE_512K_30FPS   ( 0x0100 )
#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_MIDDLE_768K_30FPS   ( 0x0110 )

#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_512K_30FPS    ( 0x0200 )
#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_768K_30FPS    ( 0x0210 )
#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_1024K_30FPS   ( 0x0220 )
#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_1536K_30FPS   ( 0x0230 )
#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_2048K_30FPS   ( 0x0240 )

#define CELL_REC_PARAM_VIDEO_FMT_AVC_MP_SMALL_512K_30FPS   ( 0x1000 )
#define CELL_REC_PARAM_VIDEO_FMT_AVC_MP_SMALL_768K_30FPS   ( 0x1010 )

#define CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_512K_30FPS  ( 0x1100 )
#define CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_768K_30FPS  ( 0x1110 )
#define CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_1024K_30FPS ( 0x1120 )
#define CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_1536K_30FPS ( 0x1130 )

#define CELL_REC_PARAM_VIDEO_FMT_AVC_BL_SMALL_512K_30FPS   ( 0x2000 )
#define CELL_REC_PARAM_VIDEO_FMT_AVC_BL_SMALL_768K_30FPS   ( 0x2010 )

#define CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_512K_30FPS  ( 0x2100 )
#define CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_768K_30FPS  ( 0x2110 )
#define CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_1024K_30FPS ( 0x2120 )
#define CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_1536K_30FPS ( 0x2130 )

#define CELL_REC_PARAM_VIDEO_FMT_MJPEG_SMALL_5000K_30FPS   ( 0x3060 )
#define CELL_REC_PARAM_VIDEO_FMT_MJPEG_MIDDLE_5000K_30FPS  ( 0x3160 )
#define CELL_REC_PARAM_VIDEO_FMT_MJPEG_LARGE_11000K_30FPS  ( 0x3270 )
#define CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_11000K_30FPS  ( 0x3670 )
#define CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_20000K_30FPS  ( 0x3680 )
#define CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_25000K_30FPS  ( 0x3690 )

#define CELL_REC_PARAM_VIDEO_FMT_M4HD_SMALL_768K_30FPS     ( 0x4010 )
#define CELL_REC_PARAM_VIDEO_FMT_M4HD_MIDDLE_768K_30FPS    ( 0x4110 )
#define CELL_REC_PARAM_VIDEO_FMT_M4HD_LARGE_1536K_30FPS    ( 0x4230 )
#define CELL_REC_PARAM_VIDEO_FMT_M4HD_LARGE_2048K_30FPS    ( 0x4240 )
#define CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_2048K_30FPS    ( 0x4640 )
#define CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS    ( 0x4660 )
#define CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_11000K_30FPS   ( 0x4670 )

/* audio format ( in CellRecParam ) */
#define CELL_REC_PARAM_AUDIO_FMT_AAC_64K   ( 0x0002 )
#define CELL_REC_PARAM_AUDIO_FMT_AAC_96K   ( 0x0000 )
#define CELL_REC_PARAM_AUDIO_FMT_AAC_128K  ( 0x0001 )
#define CELL_REC_PARAM_AUDIO_FMT_ULAW_384K ( 0x1007 )
#define CELL_REC_PARAM_AUDIO_FMT_ULAW_768K ( 0x1008 )
#define CELL_REC_PARAM_AUDIO_FMT_PCM_384K  ( 0x2007 )
#define CELL_REC_PARAM_AUDIO_FMT_PCM_768K  ( 0x2008 )
#define CELL_REC_PARAM_AUDIO_FMT_PCM_1536K ( 0x2009 )

/* YouTube recommeded video/audio format */
#define CELL_REC_PARAM_VIDEO_FMT_YOUTUBE        ( 0x0310 )
#define CELL_REC_PARAM_VIDEO_FMT_YOUTUBE_LARGE  CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_2048K_30FPS
#define CELL_REC_PARAM_VIDEO_FMT_YOUTUBE_HD720  CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS
#define CELL_REC_PARAM_AUDIO_FMT_YOUTUBE        CELL_REC_PARAM_AUDIO_FMT_AAC_64K

#define CELL_REC_PARAM_VIDEO_FMT_YOUTUBE_MJPEG  CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_11000K_30FPS
#define CELL_REC_PARAM_AUDIO_FMT_YOUTUBE_MJPEG  CELL_REC_PARAM_AUDIO_FMT_PCM_768K

static uint8_t  mc = 4; // bg memory container
static uint8_t  rec_video_index = 32; // CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS
static uint32_t rec_video_formats[37] = {
											CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_512K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_768K_30FPS,

											CELL_REC_PARAM_VIDEO_FMT_MPEG4_MIDDLE_512K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_MPEG4_MIDDLE_768K_30FPS,

											CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_512K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_768K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_1024K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_1536K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_2048K_30FPS,

											CELL_REC_PARAM_VIDEO_FMT_AVC_MP_SMALL_512K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_AVC_MP_SMALL_768K_30FPS,

											CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_512K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_768K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_1024K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_1536K_30FPS,

											CELL_REC_PARAM_VIDEO_FMT_AVC_BL_SMALL_512K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_AVC_BL_SMALL_768K_30FPS,

											CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_512K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_768K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_1024K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_1536K_30FPS,

											CELL_REC_PARAM_VIDEO_FMT_MJPEG_SMALL_5000K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_MJPEG_MIDDLE_5000K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_MJPEG_LARGE_11000K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_11000K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_20000K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_25000K_30FPS,

											CELL_REC_PARAM_VIDEO_FMT_M4HD_SMALL_768K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_M4HD_MIDDLE_768K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_M4HD_LARGE_1536K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_M4HD_LARGE_2048K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_2048K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS,
											CELL_REC_PARAM_VIDEO_FMT_YOUTUBE,
											CELL_REC_PARAM_VIDEO_FMT_YOUTUBE_LARGE,
											CELL_REC_PARAM_VIDEO_FMT_YOUTUBE_MJPEG,
											CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_11000K_30FPS,
									};

static uint8_t  rec_audio_index = 0; // CELL_REC_PARAM_AUDIO_FMT_AAC_64K
static uint32_t rec_audio_formats[8] = {
											CELL_REC_PARAM_AUDIO_FMT_AAC_64K,
											CELL_REC_PARAM_AUDIO_FMT_AAC_96K,
											CELL_REC_PARAM_AUDIO_FMT_AAC_128K,
											CELL_REC_PARAM_AUDIO_FMT_ULAW_384K,
											CELL_REC_PARAM_AUDIO_FMT_ULAW_768K,
											CELL_REC_PARAM_AUDIO_FMT_PCM_384K,
											CELL_REC_PARAM_AUDIO_FMT_PCM_768K,
											CELL_REC_PARAM_AUDIO_FMT_PCM_1536K
										};

static void show_rec_format(const char *msg)
{
	char text[120];
	sprintf(text, "%s[%i] Video: ", msg, rec_video_index + (100 * rec_audio_index));

	uint32_t flags;
	flags = (rec_video_formats[rec_video_index] & 0xF000); // video format
	if(flags == 0x0000) strcat(text, "MPEG4 ");  else
	if(flags == 0x1000) strcat(text, "AVC MP "); else
	if(flags == 0x2000) strcat(text, "AVC BL "); else
	if(flags == 0x3000) strcat(text, "MJPEG ");  else
	if(flags == 0x4000) strcat(text, "M4HD ");

	flags = (rec_video_formats[rec_video_index] & 0xF00); // video size
	if(flags == 0x000) strcat(text, "240p @ "); else
	if(flags == 0x100) strcat(text, "272p @ "); else
	if(flags == 0x200) strcat(text, "368p @ "); else
	if(flags == 0x300) strcat(text, "480p @ "); else
	if(flags == 0x600) strcat(text, "720p @ ");
	//if(flags == 0x700) strcat(text, "1080p @ ");

	flags = (rec_video_formats[rec_video_index] & 0xF0); // video bitrate
	if(flags == 0x00) strcat(text, "512K");   else
	if(flags == 0x10) strcat(text, "768K");   else
	if(flags == 0x20) strcat(text, "1024K");  else
	if(flags == 0x30) strcat(text, "1536K");  else
	if(flags == 0x40) strcat(text, "2048K");  else
	if(flags == 0x60) strcat(text, "5000K");  else
	if(flags == 0x70) strcat(text, "11000K"); else
	if(flags == 0x80) strcat(text, "20000K"); else
	if(flags == 0x90) strcat(text, "25000K");
	//if(flags == 0xA0) strcat(text, "30000K");

	flags = (rec_audio_formats[rec_audio_index] & 0xF000); // audio format
	if(flags == 0x0000) strcat(text, "\nAudio: AAC ");  else
	if(flags == 0x1000) strcat(text, "\nAudio: ULAW "); else
	if(flags == 0x2000) strcat(text, "\nAudio: PCM ");

	flags = (rec_audio_formats[rec_audio_index] & 0xF); // audio bitrate
	if(flags == 0x0) strcat(text, "96K");  else
	if(flags == 0x1) strcat(text, "128K"); else
	if(flags == 0x2) strcat(text, "64K");  else
	if(flags == 0x7) strcat(text, "384K"); else
	if(flags == 0x8) strcat(text, "768K"); else
	if(flags == 0x9) strcat(text, "1536K");

	vshtask_notify((const char*)text);
}

static bool rec_start(void)
{
	recOpt[1] = rec_video_formats[rec_video_index];
	recOpt[2] = rec_audio_formats[rec_audio_index];
	recOpt[5] = (vsh_memory_container_by_id(mc) == -1 ) ? vsh_memory_container_by_id(0) : vsh_memory_container_by_id(mc);
	recOpt[0x208] = 0x80; // 0x90 show XMB || reduce memsize // 0x80; // allow show XMB

	CellRtcDateTime t;
	cellRtcGetCurrentClockLocalTime(&t);

	char g[0x120];
	game_interface = (game_plugin_interface *)plugin_GetInterface(View_Find("game_plugin"), 1);

	game_interface->DoUnk8(g);

	cellFsMkdir((char*)"/dev_hdd0/VIDEO", 0777);

	sprintf((char *)&recOpt[0x6], "%s/%s_%04d.%02d.%02d_%02d_%02d.mp4",
	                              "/dev_hdd0/VIDEO", g+4, t.year, t.month, t.day, t.hour, t.minute);

	reco_open(-1); // memory container
	sys_timer_sleep(4);

	if(View_Find("rec_plugin"))
	{
		rec_interface = (rec_plugin_interface *)plugin_GetInterface(View_Find("rec_plugin"), 1);

		if(rec_interface)
		{
			rec_interface->start();
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		reco_open(-1); //reco_open((vsh_E7C34044(1) == -1 ) ? vsh_E7C34044(0) : vsh_E7C34044(1));
		sys_timer_sleep(3);

		if(View_Find("rec_plugin"))
		{
			rec_interface = (rec_plugin_interface *)plugin_GetInterface(View_Find("rec_plugin"), 1);

			rec_interface->start();
			return true;
		}
		else
		{
			vshtask_notify("No rec_plugin view found.");
			return false;
		}
	}
}

/***********************************************************************
* plugin main thread
***********************************************************************/
static void video_rec_thread(uint64_t arg)
{
	//sys_timer_sleep(13);

	vshtask_notify( "video_rec loaded\n"
					"Press R3 to start recording\n"
					"L2/R2+L3 to change format");

	CellPadData data;

	//////////////////////////////////////////////////////////////////////
	// video_rec: class usage
	//char nickname[0x80];
	//int32_t nickname_len;
	//
	//xsetting_0AF1F161()->GetSystemNickname(nickname, &nickname_len);
	//////////////////////////////////////////////////////////////////////

	// get functionspointer for sub_163EB0() aka reco_open()
	reco_open = vshmain_BEF63A14; // base pointer, the export nearest to sub_163EB0()

	reco_open -= (50 * 8); // reco_open_opd (50 opd's above vshmain_BEF63A14_opd)

	// fetch recording utility vsh options struct (build address from instructions...)
	uint32_t addr = (*(uint32_t*)(*(uint32_t*)reco_open + 0xC) & 0x0000FFFF) -1;
	recOpt = (uint32_t*)((addr << 16) + ((*(uint32_t*)(*(uint32_t*)reco_open + 0x14)) & 0x0000FFFF)); // (uint32_t*)0x72EEC0;

	bool recording = false;

	while(!done)
	{
		if((cellPadGetData(0, &data) == CELL_PAD_OK) && (data.len > 0))
		{
			bool l3 = (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_L3);
			bool r3 = (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_R3);

			if(l3 && r3)
			{
				vshtask_notify( "video_rec unloaded");
				sys_timer_sleep(2);
				break;
			}
			if(l3)
			{
				if(recording) ; // show recording format
				else
				if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_L2 | CELL_PAD_CTRL_R2)) // L2+R2+L3 = default format
				{
					rec_audio_index = 0;
					rec_video_index = 32;
				}
				else
				if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) // R2+L3 = change audio format
				{
					if(++rec_audio_index >= 8) rec_audio_index = 0;
				}
				else
				if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2) // L2+L3 = change video format
				{
					if(++rec_video_index >= 37) rec_video_index = 0;
				}
				show_rec_format(" ");
			}
			if(r3)
			{
				if(View_Find("game_plugin"))    // if game_plugin is loaded -> there is a game/app running and we can recording...
				{
					if(recording == false)
					{
						if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) // R2+R3 = app mc
						{
							mc = 1; // app mc
						}
						else
						if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2) // L2+R3 = app mc
						{
							mc = 1; // app mc
						}
						else
							mc = 4; // bg mc

						// not recording yet
						if(mc == 1)
							show_rec_format("Recording started [1-app]\n");
						else
							show_rec_format("Recording started [4-bg]\n");

						if(rec_start() == false)
						{
							vshtask_notify("Recording Error");
						}
						else
						{
							recording = true;
						}
					}
					else
					{
						// we are already recording
						rec_interface->stop();
						rec_interface->close(0);

						vshtask_notify("Recording finished");
						recording = false;
					}
				}
				else
					vshtask_notify("Video recording is not available on XMB.");
			}

		}
		sys_timer_usleep(70000);
	}

	sys_ppu_thread_exit(0);
}

int video_rec_start(uint64_t arg)
{
	sys_ppu_thread_create(&thread_id, video_rec_thread, 0, 3000, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, "video_rec_thread");
	_sys_ppu_thread_exit(0);
	return SYS_PRX_RESIDENT;
}

static void video_rec_stop_thread(uint64_t arg)
{
	done = 1;
	if (thread_id != (sys_ppu_thread_t)-1){
		uint64_t exit_code;
		sys_ppu_thread_join(thread_id, &exit_code);
	}

	sys_ppu_thread_exit(0);
}

static void finalize_module(void)
{
	uint64_t meminfo[5];

	sys_prx_id_t prx = prx_get_module_id_by_address(finalize_module);

	meminfo[0] = 0x28;
	meminfo[1] = 2;
	meminfo[3] = 0;

	system_call_3(482, prx, 0, (uint64_t)(uint32_t)meminfo);
}

int video_rec_stop(void)
{
	sys_ppu_thread_t t;
	uint64_t exit_code;

	sys_ppu_thread_create(&t, video_rec_stop_thread, 0, 0, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, "video_rec_stop_thread");
	sys_ppu_thread_join(t, &exit_code);

	finalize_module();
	_sys_ppu_thread_exit(0);
	return SYS_PRX_STOP_OK;
}
