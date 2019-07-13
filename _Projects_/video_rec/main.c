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

bool rec_start(void);
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

bool rec_start()
{
	recOpt[1] = CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS;
	recOpt[2] = CELL_REC_PARAM_AUDIO_FMT_AAC_64K;
	recOpt[5] = (vsh_memory_container_by_id(1) == -1 ) ? vsh_memory_container_by_id(0) : vsh_memory_container_by_id(1);
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

	if(View_Find("rec_plugin") != 0)
	{
		rec_interface = (rec_plugin_interface *)plugin_GetInterface(View_Find("rec_plugin"), 1);

		if(rec_interface != 0)
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

		if(View_Find("rec_plugin") != 0)
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
	sys_timer_sleep(13);

	vshtask_notify("video_rec loaded.\nPress R3 to start recording");

	CellPadData data;

	//////////////////////////////////////////////////////////////////////
	// video_rec: class usage
	char nickname[0x80];
	int32_t nickname_len;

	xsetting_0AF1F161()->GetSystemNickname(nickname, &nickname_len);
	//////////////////////////////////////////////////////////////////////

	// get functionspointer for sub_163EB0() aka reco_open()
	reco_open = vshmain_BEF63A14; // base pointer, the export nearest to sub_163EB0()

	reco_open -= (50*8); // reco_open_opd (50 opd's above vshmain_BEF63A14_opd)

	// fetch recording utility vsh options struct (build address from instructions...)
	uint32_t addr = (*(uint32_t*)(*(uint32_t*)reco_open+0xC) & 0x0000FFFF) -1;
	recOpt = (uint32_t*)((addr << 16) + ((*(uint32_t*)(*(uint32_t*)reco_open+0x14)) & 0x0000FFFF)); // (uint32_t*)0x72EEC0;

	bool recording = false;

	while(!done)
	{
		if(cellPadGetData(0, &data) == CELL_PAD_OK && data.len > 0)
		{
			bool r3 = (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_R3);

			if(r3)
			{
				if(View_Find("game_plugin") != 0)    // if game_plugin is loaded -> there is a game/app running and we can recording...
				{
					if(recording == false)
					{
						// not recording yet
						vshtask_notify("Recording started.");

						if(rec_start() == false)
						{
							vshtask_notify("Recording Error.");
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

						vshtask_notify("Recording finished.");
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
