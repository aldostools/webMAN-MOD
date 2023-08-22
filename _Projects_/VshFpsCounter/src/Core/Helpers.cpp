#include "Helpers.hpp"

Helpers g_Helpers;

Helpers::Helpers()
{
   m_IsHen = IsConsoleHen();
}

void Helpers::OnUpdate()
{
   game_ext_plugin = vsh::paf::View::Find("game_ext_plugin");
   game_plugin = vsh::paf::View::Find("game_plugin");
   system_plugin = vsh::paf::View::Find("system_plugin");
   xmb_plugin = vsh::paf::View::Find("xmb_plugin");

   page_notification = system_plugin ? system_plugin->FindWidget("page_notification") : nullptr;
   page_xmb_indicator = system_plugin ? system_plugin->FindWidget("page_xmb_indicator") : nullptr;


   MonitorGameState();
}

void Helpers::MonitorGameState()
{
   uint64_t timeNow = GetTimeNow();
   if (vsh::GetCooperationMode() != vsh::eCooperationMode::XMB && !m_StateGameRunning)
   {
      m_StateGameRunning = true;
      m_StateGameJustLaunched = true;
      m_GameLaunchTime = timeNow;
   }
   else if (vsh::GetCooperationMode() == vsh::eCooperationMode::XMB)
      m_StateGameRunning = false;

   if (timeNow - m_GameLaunchTime > 15 * 1000)
       m_StateGameJustLaunched = false;
}