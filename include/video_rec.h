#include "../vsh/vshmain.h"
#include "../vsh/vsh.h"

#ifdef VIDEO_REC

#include "../vsh/rec_plugin.h"

#define REC_PLUGIN "rec_plugin"

/* video format ( in CellRecParam )
 *
 *   SMALL  = 320x240 (4:3) or 368x208 (16:9)
 *   MIDDLE = 368x272 (4:3) or 480x272 (16:9)
 *   LARGE  = 480x368 (4:3) or 640x368 (16:9)
 *
 *   HD720  = 1280x720 (16:9)
 *
 *   PS3 playable format; all
 *   PSP playable format: MPEG4 + SMALL, AVC + SMALL, AVC + MIDDLE
 */

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

static u8  rec_setting_to_change = 0; // 0 = video format, 1 = video size, 2 = video bitrate, 3 = audio format, 4 = audio bitrate
static u32 rec_video_format = CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS;
static u32 rec_audio_format = CELL_REC_PARAM_AUDIO_FMT_AAC_64K;

bool recording = false;

u32 *recOpt = NULL;           // recording utility vsh options struct
s32 (*reco_open)(s32) = NULL; // base pointer

static void set_setting_to_change(char *msg, const char *text)
{
	sprintf(msg, "%s", text);
	if(rec_setting_to_change == 0) strcat(msg, "Recording Options"); else
	if(rec_setting_to_change == 1) strcat(msg, "Video Format");      else
	if(rec_setting_to_change == 2) strcat(msg, "Video Size");        else
	if(rec_setting_to_change == 3) strcat(msg, "Video Bitrate");     else
	if(rec_setting_to_change == 4) strcat(msg, "Audio Format");      else
	if(rec_setting_to_change == 5) strcat(msg, "Audio Bitrate");
}

static void show_rec_format(const char *msg)
{
	char text[200]; u32 flags;
	sprintf(text, "%s\nVideo: ", msg);

	flags = (rec_video_format & 0xF000); // video format
	if(flags == 0x0000) strcat(text, "MPEG4 ");  else
	if(flags == 0x1000) strcat(text, "AVC MP "); else
	if(flags == 0x2000) strcat(text, "AVC BL "); else
	if(flags == 0x3000) strcat(text, "MJPEG ");  else
	if(flags == 0x4000) strcat(text, "M4HD ");

	flags = (rec_video_format & 0xF00); // video size
	if(flags == 0x000) strcat(text, "240p @ "); else
	if(flags == 0x100) strcat(text, "272p @ "); else
	if(flags == 0x200) strcat(text, "368p @ "); else
	if(flags == 0x300) strcat(text, "480p @ "); else
	if(flags == 0x600) strcat(text, "720p @ ");
	//if(flags == 0x700) strcat(text, "1080p @ ");

	flags = (rec_video_format & 0xF0); // video bitrate
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

	flags = (rec_audio_format & 0xF000); // audio format
	if(flags == 0x0000) strcat(text, "\nAudio: AAC ");  else
	if(flags == 0x1000) strcat(text, "\nAudio: ULAW "); else
	if(flags == 0x2000) strcat(text, "\nAudio: PCM ");

	flags = (rec_audio_format & 0xF); // audio bitrate
	if(flags == 0x0) strcat(text, "96K");  else
	if(flags == 0x1) strcat(text, "128K"); else
	if(flags == 0x2) strcat(text, "64K");  else
	if(flags == 0x7) strcat(text, "384K"); else
	if(flags == 0x8) strcat(text, "768K"); else
	if(flags == 0x9) strcat(text, "1536K");

	show_msg(text);
}

static bool rec_start(const char *param)
{
	char value[8];

	// set video format
	if(get_param("video=", value, param, 4))
	{
		rec_video_format = convertH(value);
	}
	else
	{
		if(strcasestr(param, ".mp4"))
		{
			rec_video_format = CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS;									// .mp4 (720p)

			if(strstr(param, "240"))   rec_video_format = CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_768K_30FPS;		// .mp4 (240p / 208p)
			if(strstr(param, "272"))   rec_video_format = CELL_REC_PARAM_VIDEO_FMT_MPEG4_MIDDLE_768K_30FPS;		// .mp4 (272p)
			if(strstr(param, "368"))   rec_video_format = CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_2048K_30FPS;		// .mp4 (368p)
		}

		if(strcasestr(param, "jpeg")) {rec_video_format = CELL_REC_PARAM_VIDEO_FMT_YOUTUBE_MJPEG; rec_audio_format = CELL_REC_PARAM_AUDIO_FMT_YOUTUBE_MJPEG;}	// mjpeg

		if(strcasestr(param, "psp"))
		{
			rec_video_format = CELL_REC_PARAM_VIDEO_FMT_AVC_MP_SMALL_768K_30FPS;								// psp (512k)
			if(strcasestr(param, "hd"))    rec_video_format = CELL_REC_PARAM_VIDEO_FMT_M4HD_SMALL_768K_30FPS;	// psp m4hd
		}
		else
		if(strcasestr(param, "hd"))
		{
			rec_video_format = CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_11000K_30FPS;								// hd (11000k)

			if(strstr(param, "768"))   rec_video_format = CELL_REC_PARAM_VIDEO_FMT_M4HD_MIDDLE_768K_30FPS;		// hd 768k  (272p)
			if(strstr(param, "1536"))  rec_video_format = CELL_REC_PARAM_VIDEO_FMT_M4HD_LARGE_1536K_30FPS;		// hd 1536k (368p)
			if(strstr(param, "2048"))  rec_video_format = CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_2048K_30FPS;		// hd 2048k (720p)
		}
		else
		{
			if(strcasestr(param, "avc"))  rec_video_format = CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_768K_30FPS;	// avc (psp/ps3)

			if(strstr(param, "512"))  rec_video_format = CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_512K_30FPS;		// avc 512k
			if(strstr(param, "768"))  rec_video_format = CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_768K_30FPS;		// avc 768k
			if(strstr(param, "1024")) rec_video_format = CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_1024K_30FPS;	// avc 1024k
			if(strstr(param, "1536")) rec_video_format = CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_1536K_30FPS;	// avc 1536k
		}
	}

	u8 n;

	// validate video format (use default if invalid)
	u32 video_formats[34] = {CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_512K_30FPS, CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_768K_30FPS, CELL_REC_PARAM_VIDEO_FMT_MPEG4_MIDDLE_512K_30FPS, CELL_REC_PARAM_VIDEO_FMT_MPEG4_MIDDLE_768K_30FPS, CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_512K_30FPS, CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_768K_30FPS, CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_1024K_30FPS, CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_1536K_30FPS, CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_2048K_30FPS, CELL_REC_PARAM_VIDEO_FMT_AVC_MP_SMALL_512K_30FPS, CELL_REC_PARAM_VIDEO_FMT_AVC_MP_SMALL_768K_30FPS, CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_512K_30FPS, CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_768K_30FPS, CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_1024K_30FPS, CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_1536K_30FPS, CELL_REC_PARAM_VIDEO_FMT_AVC_BL_SMALL_512K_30FPS, CELL_REC_PARAM_VIDEO_FMT_AVC_BL_SMALL_768K_30FPS, CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_512K_30FPS, CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_768K_30FPS, CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_1024K_30FPS, CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_1536K_30FPS, CELL_REC_PARAM_VIDEO_FMT_MJPEG_SMALL_5000K_30FPS, CELL_REC_PARAM_VIDEO_FMT_MJPEG_MIDDLE_5000K_30FPS, CELL_REC_PARAM_VIDEO_FMT_MJPEG_LARGE_11000K_30FPS, CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_11000K_30FPS, CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_20000K_30FPS, CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_25000K_30FPS, CELL_REC_PARAM_VIDEO_FMT_M4HD_SMALL_768K_30FPS, CELL_REC_PARAM_VIDEO_FMT_M4HD_MIDDLE_768K_30FPS, CELL_REC_PARAM_VIDEO_FMT_M4HD_LARGE_1536K_30FPS, CELL_REC_PARAM_VIDEO_FMT_M4HD_LARGE_2048K_30FPS, CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_2048K_30FPS, CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS, CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_11000K_30FPS};
	for(n = 0; n < sizeof(video_formats); n++) if(rec_video_format == video_formats[n]) break;
	if(n >= sizeof(video_formats)) rec_video_format = CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS;

	// set audio format
	if(get_param("audio=", value, param, 4))
	{
		rec_audio_format = convertH(value);
	}
	else
	if(strcasestr(param, "aac"))
	{
		rec_audio_format = CELL_REC_PARAM_AUDIO_FMT_AAC_96K;							// aac_96k
		if(strstr(param, "64"))  rec_audio_format = CELL_REC_PARAM_AUDIO_FMT_AAC_64K;	// aac_64k
		if(strstr(param, "128")) rec_audio_format = CELL_REC_PARAM_AUDIO_FMT_AAC_128K;	// aac_128k
	}
	else
	if(strcasestr(param, "pcm"))
	{
		rec_audio_format = CELL_REC_PARAM_AUDIO_FMT_PCM_768K;								// pcm_768k
		if(strstr(param, "384"))  rec_audio_format = CELL_REC_PARAM_AUDIO_FMT_PCM_384K;		// pcm_384k
		if(strstr(param, "1536")) rec_audio_format = CELL_REC_PARAM_AUDIO_FMT_PCM_1536K;	// pcm_1536k
	}

	// validate audio format (use default if invalid)
	u32 audio_formats[8] = {CELL_REC_PARAM_AUDIO_FMT_AAC_64K, CELL_REC_PARAM_AUDIO_FMT_AAC_96K, CELL_REC_PARAM_AUDIO_FMT_AAC_128K, CELL_REC_PARAM_AUDIO_FMT_ULAW_384K, CELL_REC_PARAM_AUDIO_FMT_ULAW_768K, CELL_REC_PARAM_AUDIO_FMT_PCM_384K, CELL_REC_PARAM_AUDIO_FMT_PCM_768K, CELL_REC_PARAM_AUDIO_FMT_PCM_1536K};
	for(n = 0; n < sizeof(audio_formats); n++) if(rec_audio_format == audio_formats[n]) break;
	if(n >= sizeof(audio_formats)) rec_audio_format = CELL_REC_PARAM_AUDIO_FMT_AAC_64K;

	// set video options
	recOpt[1] = rec_video_format;
	recOpt[2] = rec_audio_format;
	recOpt[5] = (vsh_memory_container_by_id(1) == NONE ) ? vsh_memory_container_by_id(0) : vsh_memory_container_by_id(1);
	recOpt[0x208] = 0x80; // 0x90 show XMB || reduce memsize // 0x80; // allow show XMB

	get_game_info();

	char *vidfile = strstr(param, "/dev_hdd0/");

	if(vidfile == param)
	{
		sprintf((char*)&recOpt[0x6], "%s", vidfile);
	}
	else
	{
		cellFsMkdir((char*)"/dev_hdd0/VIDEO", 0777);

		CellRtcDateTime t;
		cellRtcGetCurrentClockLocalTime(&t);
		sprintf((char*)&recOpt[0x6], "%s/%s_%04d.%02d.%02d_%02d_%02d_%02d.mp4",
									 "/dev_hdd0/VIDEO", _game_TitleID, t.year, t.month, t.day, t.hour, t.minute, t.second);
	}

	reco_open(-1); // memory container
	sys_ppu_thread_sleep(4);


	if(View_Find(REC_PLUGIN) != 0)
	{
		rec_interface = (rec_plugin_interface *)plugin_GetInterface(View_Find(REC_PLUGIN), 1);

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
		reco_open(-1); //reco_open((vsh_memory_container_by_id(1) == NONE ) ? vsh_memory_container_by_id(0) : vsh_memory_container_by_id(1));
		sys_ppu_thread_sleep(3);

		if(View_Find(REC_PLUGIN) != 0)
		{
			rec_interface = (rec_plugin_interface *)plugin_GetInterface(View_Find(REC_PLUGIN), 1);

			rec_interface->start();
			return true;
		}
		else
		{
			show_msg((char*)"No rec_plugin view found.");
			return false;
		}
	}
}

static void toggle_video_rec(const char *param)
{
	if(IS_INGAME)    // if game_plugin is loaded -> there is a game/app running and we can recording...
	{
		if(!reco_open)
		{
			// get functions pointer for sub_163EB0() aka reco_open()
			//reco_open = (void*)vshmain_BEF63A14; // base pointer, the export nearest to sub_163EB0()
			//reco_open -= (50 * 8); // reco_open_opd (50 opd's above vshmain_BEF63A14_opd)

			reco_open = getNIDfunc("vshmain", 0xBEF63A14, -(50 * 8));

			// fetch recording utility vsh options struct (build address from instructions...)
			u32 addr = (*(u32*)(*(u32*)reco_open+0xC) & 0x0000FFFF) -1;
			recOpt = (u32*)((addr << 16) + ((*(u32*)(*(u32*)reco_open+0x14)) & 0x0000FFFF)); // (u32*)0x72EEC0;
		}

		if(recording == false)
		{
			// not recording yet
			show_rec_format("Recording started");

			if(rec_start(param) == false)
			{
				show_msg((char*)"Recording Error!");
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
			show_msg((char*)"Recording finished");
			recording = false;
		}
	}
	else
		recording = false;
}

#endif // #ifdef VIDEO_REC
