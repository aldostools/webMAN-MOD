#include <cellstatus.h>
#include <sys/prx.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <cell/pad.h>
#include "Utils/NewDeleteOverride.hpp"
#include "Utils/SystemCalls.hpp"
#include "Utils/Timers.hpp"
#include "Core/Overlay.hpp"
#include "Core/Hooks.hpp"
#include "Core/Configuration.hpp"

// Macros defining our module informations as well as our entry/exit point
SYS_MODULE_INFO(VshFpsCounter, 0, 1, 1);
SYS_MODULE_START(module_start);
SYS_MODULE_STOP(module_stop);

sys_ppu_thread_t gVshMenuPpuThreadId = SYS_PPU_THREAD_ID_INVALID;

CDECL_BEGIN
int module_start(unsigned int args, void* argp)
{
   sys_ppu_thread_create(&gVshMenuPpuThreadId, [](uint64_t arg) -> void
   {
      do
          Sleep(1000);
      while (!vsh::paf::View::Find("explore_plugin"));

      g_Render = Render();
      g_Helpers = Helpers();
      g_Config = Config();
      g_Overlay = Overlay();

      InstallHooks();

      sys_ppu_thread_exit(0);

   }, 0, 1059, 1500, SYS_PPU_THREAD_CREATE_JOINABLE, "VshFpsCounterStart");

   // Exit thread using directly the syscall and not the user mode library or else we will crash
   _sys_ppu_thread_exit(0);

   return 0;
}

int module_stop(unsigned int args, void* argp)
{
   sys_ppu_thread_t stopPpuThreadId;
   int ret = sys_ppu_thread_create(&stopPpuThreadId, [](uint64_t arg) -> void
   {
      RemoveHooks();

      g_Overlay.OnShutdown();
      g_Render.DestroyWidgets();

      // Prevent unload too fast (give time to other threads to finish)
      sys_ppu_thread_yield();
      Sleep(1000);

      if (gVshMenuPpuThreadId != SYS_PPU_THREAD_ID_INVALID)
      {
         uint64_t exitCode;
         sys_ppu_thread_join(gVshMenuPpuThreadId, &exitCode);
      }

      sys_ppu_thread_exit(0);
   },
   0, 2816, 1024, SYS_PPU_THREAD_CREATE_JOINABLE, "VshFpsCounterStop");

   if (ret == SUCCEEDED)
   {
      uint64_t exitCode;
      sys_ppu_thread_join(stopPpuThreadId, &exitCode);
   }

   Sleep(5);

   // unloading prx from memory
   UnloadMyModule();

   // Exit thread using directly the syscall and not the user mode library or else we will crash
   _sys_ppu_thread_exit(0);

   return 0;
}
CDECL_END