#include <sys/prx.h>                                       #include <sys/ppu_thread.h>
#include <sys/timer.h>                                     #include <sys/syscall.h>
#include <cell/cell_fs.h>                                  #include <sys/sys_time.h>
#include <sys/time_util.h>                                 #include <stdbool.h>
#include <cell/pad.h>                                      #include <cell/rtc.h>
                                                           #include "vsh_exports.h"
                                                           SYS_MODULE_INFO(VIDEO_REC, 0, 1, 0);
SYS_MODULE_START(video_rec_start);                         SYS_MODULE_STOP(video_rec_stop);
                                                           static sys_ppu_thread_t        thread_id  = (sys_ppu_thread_t)-1;                                                     static volatile int32_t        done       = 0;
static uint32_t               *recOpt     = NULL;          static int32_t               (*reco_open)(int32_t) = NULL;
                                                           rec_plugin_interface  *rec_interface  = NULL;
game_plugin_interface *game_interface = NULL;              
int32_t video_rec_start(uint64_t arg);
int32_t video_rec_stop(void);

static inline void _sys_ppu_thread_exit(uint64_t val)
{
    system_call_1(41, val);
}

static inline sys_prx_id_t prx_get_module_id_by_address(void *addr)
{
    system_call_1(461, (uint64_t)(uint32_t)addr);
    return (int)p1;
}

/* ------------------------------------------------------------------ */
/*  Formatos                                                           */
/* ------------------------------------------------------------------ */

#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_512K_30FPS    (0x0000)
#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_768K_30FPS    (0x0010)
#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_MIDDLE_512K_30FPS   (0x0100)
#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_MIDDLE_768K_30FPS   (0x0110)
#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_512K_30FPS    (0x0200)
#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_768K_30FPS    (0x0210)
#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_1024K_30FPS   (0x0220)
#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_1536K_30FPS   (0x0230)
#define CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_2048K_30FPS   (0x0240)
#define CELL_REC_PARAM_VIDEO_FMT_AVC_MP_SMALL_512K_30FPS   (0x1000)
#define CELL_REC_PARAM_VIDEO_FMT_AVC_MP_SMALL_768K_30FPS   (0x1010)
#define CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_512K_30FPS  (0x1100)
#define CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_768K_30FPS  (0x1110)
#define CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_1024K_30FPS (0x1120)
#define CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_1536K_30FPS (0x1130)
#define CELL_REC_PARAM_VIDEO_FMT_AVC_BL_SMALL_512K_30FPS   (0x2000)
#define CELL_REC_PARAM_VIDEO_FMT_AVC_BL_SMALL_768K_30FPS   (0x2010)
#define CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_512K_30FPS  (0x2100)
#define CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_768K_30FPS  (0x2110)
#define CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_1024K_30FPS (0x2120)
#define CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_1536K_30FPS (0x2130)
#define CELL_REC_PARAM_VIDEO_FMT_MJPEG_SMALL_5000K_30FPS   (0x3060)
#define CELL_REC_PARAM_VIDEO_FMT_MJPEG_MIDDLE_5000K_30FPS  (0x3160)
#define CELL_REC_PARAM_VIDEO_FMT_MJPEG_LARGE_11000K_30FPS  (0x3270)
#define CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_11000K_30FPS  (0x3670)
#define CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_20000K_30FPS  (0x3680)
#define CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_25000K_30FPS  (0x3690)
#define CELL_REC_PARAM_VIDEO_FMT_M4HD_SMALL_768K_30FPS     (0x4010)
#define CELL_REC_PARAM_VIDEO_FMT_M4HD_MIDDLE_768K_30FPS    (0x4110)
#define CELL_REC_PARAM_VIDEO_FMT_M4HD_LARGE_1536K_30FPS    (0x4230)
#define CELL_REC_PARAM_VIDEO_FMT_M4HD_LARGE_2048K_30FPS    (0x4240)
#define CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_2048K_30FPS    (0x4640)
#define CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS    (0x4660)
#define CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_11000K_30FPS   (0x4670)
#define CELL_REC_PARAM_VIDEO_FMT_YOUTUBE                   (0x0310)
#define CELL_REC_PARAM_VIDEO_FMT_YOUTUBE_LARGE             CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_2048K_30FPS
#define CELL_REC_PARAM_VIDEO_FMT_YOUTUBE_MJPEG             CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_11000K_30FPS

#define CELL_REC_PARAM_AUDIO_FMT_AAC_64K    (0x0002)
#define CELL_REC_PARAM_AUDIO_FMT_AAC_96K    (0x0000)
#define CELL_REC_PARAM_AUDIO_FMT_AAC_128K   (0x0001)
#define CELL_REC_PARAM_AUDIO_FMT_ULAW_384K  (0x1007)
#define CELL_REC_PARAM_AUDIO_FMT_ULAW_768K  (0x1008)
#define CELL_REC_PARAM_AUDIO_FMT_PCM_384K   (0x2007)
#define CELL_REC_PARAM_AUDIO_FMT_PCM_768K   (0x2008)
#define CELL_REC_PARAM_AUDIO_FMT_PCM_1536K  (0x2009)

#define NUM_VIDEO_FMTS 37
#define NUM_AUDIO_FMTS 8

static uint8_t  mc              = 4;
static uint8_t  rec_video_index = 32;
static uint8_t  rec_audio_index = 0;

static uint32_t rec_video_formats[NUM_VIDEO_FMTS] = {
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
    CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_1536K_30FPS,        CELL_REC_PARAM_VIDEO_FMT_AVC_BL_SMALL_512K_30FPS,
    CELL_REC_PARAM_VIDEO_FMT_AVC_BL_SMALL_768K_30FPS,          CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_512K_30FPS,
    CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_768K_30FPS,         CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_1024K_30FPS,
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

static uint32_t rec_audio_formats[NUM_AUDIO_FMTS] = {
    CELL_REC_PARAM_AUDIO_FMT_AAC_64K,
    CELL_REC_PARAM_AUDIO_FMT_AAC_96K,
    CELL_REC_PARAM_AUDIO_FMT_AAC_128K,
    CELL_REC_PARAM_AUDIO_FMT_ULAW_384K,
    CELL_REC_PARAM_AUDIO_FMT_ULAW_768K,
    CELL_REC_PARAM_AUDIO_FMT_PCM_384K,
    CELL_REC_PARAM_AUDIO_FMT_PCM_768K,
    CELL_REC_PARAM_AUDIO_FMT_PCM_1536K,
};

/* ------------------------------------------------------------------ */
/*  show_rec_format                                                    */
/* ------------------------------------------------------------------ */
static void show_rec_format(const char *prefix)
{
    char text[192];
    uint32_t vfmt = rec_video_formats[rec_video_index];        uint32_t afmt = rec_audio_formats[rec_audio_index];

    const char *vcodec =
        ((vfmt & 0xF000) == 0x0000) ? "MPEG4"  :
        ((vfmt & 0xF000) == 0x1000) ? "AVC MP" :
        ((vfmt & 0xF000) == 0x2000) ? "AVC BL" :
        ((vfmt & 0xF000) == 0x3000) ? "MJPEG"  :
        ((vfmt & 0xF000) == 0x4000) ? "M4HD"   : "?";

    const char *vres =
        ((vfmt & 0xF00) == 0x000) ? "240p" :
        ((vfmt & 0xF00) == 0x100) ? "272p" :
        ((vfmt & 0xF00) == 0x200) ? "368p" :
        ((vfmt & 0xF00) == 0x300) ? "480p" :
        ((vfmt & 0xF00) == 0x600) ? "720p" : "?";          
    const char *vbr =                                              ((vfmt & 0xFF) == 0x00) ? "512K"   :
        ((vfmt & 0xFF) == 0x10) ? "768K"   :                       ((vfmt & 0xFF) == 0x20) ? "1024K"  :
        ((vfmt & 0xFF) == 0x30) ? "1536K"  :
        ((vfmt & 0xFF) == 0x40) ? "2048K"  :
        ((vfmt & 0xFF) == 0x60) ? "5000K"  :
        ((vfmt & 0xFF) == 0x70) ? "11000K" :
        ((vfmt & 0xFF) == 0x80) ? "20000K" :
        ((vfmt & 0xFF) == 0x90) ? "25000K" : "?";

    const char *acodec =
        ((afmt & 0xF000) == 0x0000) ? "AAC"  :
        ((afmt & 0xF000) == 0x1000) ? "ULAW" :
        ((afmt & 0xF000) == 0x2000) ? "PCM"  : "?";

    const char *abr =
        ((afmt & 0xF) == 0x0) ? "96K"   :
        ((afmt & 0xF) == 0x1) ? "128K"  :
        ((afmt & 0xF) == 0x2) ? "64K"   :
        ((afmt & 0xF) == 0x7) ? "384K"  :                          ((afmt & 0xF) == 0x8) ? "768K"  :
        ((afmt & 0xF) == 0x9) ? "1536K" : "?";
                                                               snprintf(text, sizeof(text),
             "%s[%d] Video: %s %s @ %s\nAudio: %s %s",
             prefix,
             rec_video_index + (100 * rec_audio_index),
             vcodec, vres, vbr,
             acodec, abr);

    vshtask_notify(text);                                  }

/* ------------------------------------------------------------------ */
/*  rec_start                                                          */
/* ------------------------------------------------------------------ */
static bool rec_start(void)
{
    if (!recOpt || !reco_open)
    {
        vshtask_notify("rec_start: ponteiros nulos");
        return false;
    }

    recOpt[1] = rec_video_formats[rec_video_index];
    recOpt[2] = rec_audio_formats[rec_audio_index];
    recOpt[5] = (vsh_memory_container_by_id(mc) == -1)
                ? vsh_memory_container_by_id(0)
                : vsh_memory_container_by_id(mc);
    recOpt[0x208] = 0x80;

    CellRtcDateTime t;
    cellRtcGetCurrentClockLocalTime(&t);

    char g[0x120];
    game_interface = (game_plugin_interface *)
        plugin_GetInterface(View_Find("game_plugin"), 1);      game_interface->getGameTitle(g);

    cellFsMkdir("/dev_hdd0/VIDEO", 0777);

    snprintf((char *)&recOpt[0x6], 0x100,
             "%s/%s_%04d.%02d.%02d_%02d_%02d.mp4",                      "/dev_hdd0/VIDEO", g + 4,
             t.year, t.month, t.day, t.hour, t.minute);

    int attempt;
    for (attempt = 0; attempt < 2; attempt++)
    {
        reco_open(-1);
        sys_timer_sleep(attempt == 0 ? 4 : 3);

        if (View_Find("rec_plugin"))
        {
            rec_interface = (rec_plugin_interface *)
                plugin_GetInterface(View_Find("rec_plugin"), 1);
                                                                       if (rec_interface)
            {                                                              rec_interface->start();
                return true;
            }
        }
    }                                                      
    vshtask_notify("Erro: rec_plugin nao encontrado");
    return false;
}

/* ------------------------------------------------------------------ */
/*  Thread principal                                                   */
/* ------------------------------------------------------------------ */                                              static void video_rec_thread(uint64_t arg)
{
    vshtask_notify("video_rec loaded\n"
                   "R3 = gravar | L3+R3 = sair\n"
                   "L2/R2+L3 = mudar formato");
                                                               reco_open  = (int32_t(*)(int32_t))vshmain_BEF63A14;
    reco_open -= (50 * 8);

    if ((uint32_t)(uintptr_t)reco_open < 0x10000)
    {
        vshtask_notify("Erro: reco_open invalido");
        sys_ppu_thread_exit(0);
        return;
    }

    uint32_t addr = (*(uint32_t *)(*(uint32_t *)reco_open + 0xC) & 0x0000FFFF) - 1;                                       recOpt = (uint32_t *)((addr << 16) +
              (*(uint32_t *)(*(uint32_t *)reco_open + 0x14) & 0x0000FFFF));
                                                               bool recording = false;
    CellPadData data;

    while (!done)
    {
        if ((cellPadGetData(0, &data) == CELL_PAD_OK) && (data.len > 0))
        {
            uint8_t d1 = data.button[CELL_PAD_BTN_OFFSET_DIGITAL1];
            uint8_t d2 = data.button[CELL_PAD_BTN_OFFSET_DIGITAL2];

            bool l3 = (d1 & CELL_PAD_CTRL_L3) != 0;
            bool r3 = (d1 & CELL_PAD_CTRL_R3) != 0;
            bool l2 = (d2 & CELL_PAD_CTRL_L2) != 0;
            bool r2 = (d2 & CELL_PAD_CTRL_R2) != 0;

            if (l3 && r3)
            {
                vshtask_notify("video_rec descarregado");
                sys_timer_sleep(2);
                break;
            }

            if (l3 && !recording)
            {
                if (l2 && r2)
                {
                    rec_audio_index = 0;                                       rec_video_index = 32;
                }
                else if (r2)
                {
                    if (++rec_audio_index >= NUM_AUDIO_FMTS)
                        rec_audio_index = 0;                               }
                else if (l2)
                {
                    if (++rec_video_index >= NUM_VIDEO_FMTS)
                        rec_video_index = 0;
                }
                show_rec_format(" ");
            }

            if (r3)
            {
                if (View_Find("game_plugin"))
                {
                    if (!recording)
                    {
                        mc = (l2 || r2) ? 1 : 4;
                        show_rec_format(mc == 1
                                        ? "Gravando [1-app]\n"
                                        : "Gravando [4-bg]\n");
                        if (rec_start())
                            recording = true;
                        else
                            vshtask_notify("Erro ao iniciar gravacao");
                    }
                    else
                    {
                        if (rec_interface)
                        {
                            rec_interface->stop();
                            rec_interface->close(0);
                        }
                        recording = false;                                         vshtask_notify("Gravacao finalizada");
                    }
                }
                else
                {
                    vshtask_notify("Gravacao indisponivel no XMB");
                }
            }
        }

        sys_timer_usleep(70000);
    }

    sys_ppu_thread_exit(0);
}

/* ------------------------------------------------------------------ */
/*  Entradas do modulo                                                 */
/* ------------------------------------------------------------------ */
int video_rec_start(uint64_t arg)
{
    sys_ppu_thread_create(&thread_id, video_rec_thread, 0,
                          0xC0, 0x4000,
                          SYS_PPU_THREAD_CREATE_JOINABLE,
                          "video_rec_thread");
    _sys_ppu_thread_exit(0);
    return SYS_PRX_RESIDENT;
}

static void video_rec_stop_thread(uint64_t arg)
{
    done = 1;
    sys_timer_usleep(150000);

    if (thread_id != (sys_ppu_thread_t)-1)
    {                                                              uint64_t exit_code;                                        sys_ppu_thread_join(thread_id, &exit_code);
        thread_id = (sys_ppu_thread_t)-1;
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

int video_rec_stop(void)                                   {
    sys_ppu_thread_t t;
    uint64_t exit_code;

    sys_ppu_thread_create(&t, video_rec_stop_thread, 0, 0,                           0x2000,                                                    SYS_PPU_THREAD_CREATE_JOINABLE,                            "video_rec_stop_thread");
    sys_ppu_thread_join(t, &exit_code);

    finalize_module();
    _sys_ppu_thread_exit(0);
    return SYS_PRX_STOP_OK;
}
