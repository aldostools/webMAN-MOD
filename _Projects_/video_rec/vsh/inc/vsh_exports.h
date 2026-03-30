#ifndef __VSH_EXPORTS_H__                                  #define __VSH_EXPORTS_H__

#include <stdio.h>                                         #include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>                                        
#include <sys/prx.h>                                       #include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/syscall.h>
#include <sys/sys_time.h>

#include <cell/cell_fs.h>
#include <cell/pad.h>
#include <cell/rtc.h>

#include "stdc.h"
#include "vsh.h"
#include "vshtask.h"
#include "vshmain.h"
#include "paf.h"                                           #include "xregistry.h"
#include "game_plugin.h"
#include "rec_plugin.h"

/* View_Find — wrapper do paf usado no VSH */
extern uint32_t *View_Find(const char *name);

#endif /* __VSH_EXPORTS_H__ */
