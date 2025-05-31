#include "Overlay.hpp"
#include "Memory/Pattern.hpp"

Overlay g_Overlay;

Overlay::Overlay()
{
    m_ReloadConfigTime = GetTimeNow() + 10000;

    // find clock speed offsets only when they are displayed
    if (g_Config.overlay.mode[Config::DisplayMode::XMB].showClockSpeeds 
        || g_Config.overlay.mode[Config::DisplayMode::GAME].showClockSpeeds)
        sys_ppu_thread_create(&LoadExternalOffsetsThreadId, LoadExternalOffsets, 0, 0xB02, 512, SYS_PPU_THREAD_CREATE_JOINABLE, "Overlay::LoadExternalOffsets()");

    sys_ppu_thread_create(&UpdateInfoThreadId, UpdateInfoThread, 0, 0xB01, 512, SYS_PPU_THREAD_CREATE_JOINABLE, "Overlay::UpdateInfoThread()");
}

void Overlay::OnUpdate()
{
   /*if (GetTimeNow() > m_ReloadConfigTime)
   {
       g_Config.Load();
       m_ReloadConfigTime = GetTimeNow() + 10000;
   }*/

   UpdatePosition();
   CalculateFps();
   DrawOverlay();
   Lv2LabelUpdate();
}

void Overlay::OnShutdown()
{
   if (LoadExternalOffsetsThreadId != SYS_PPU_THREAD_ID_INVALID)
   {
       uint64_t exitCode;
       sys_ppu_thread_join(LoadExternalOffsetsThreadId, &exitCode);
   }

   if (UpdateInfoThreadId != SYS_PPU_THREAD_ID_INVALID)
   {
      m_StateRunning = false;

      sys_ppu_thread_yield();
      Sleep(refreshDelay * 1000 + 500);

      uint64_t exitCode;
      sys_ppu_thread_join(UpdateInfoThreadId, &exitCode);
   }
}

void Overlay::DrawOverlay()
{
   m_CooperationMode = vsh::GetCooperationMode();

   if (g_Config.overlay.displayMode == Config::DisplayMode::XMB && m_CooperationMode != vsh::eCooperationMode::XMB)
       return;

   if (g_Config.overlay.displayMode == Config::DisplayMode::GAME && m_CooperationMode == vsh::eCooperationMode::XMB)
       return;

   vsh::paf::View* gamePlugin = vsh::paf::View::Find("game_plugin");
   std::wstring overlayText;

   if (g_Config.overlay.mode[(int)m_CooperationMode].showFPS)
   {
       overlayText += L"FPS: " + to_wstring(m_FPS, 2) + L"\n";
   }

   if (g_Config.overlay.mode[(int)m_CooperationMode].showCpuInfo)
   {
       std::wstring tempTypeStr = m_TempType == TempType::Fahrenheit ? L"\u2109" : L"\u2103";
       overlayText += L"CPU: " + to_wstring(m_CPUTemp) + tempTypeStr;

       if (m_CpuClock != 0 && g_Config.overlay.mode[(int)m_CooperationMode].showClockSpeeds)
           overlayText += L" / " + to_wstring(m_CpuClock / 1000.0f, 1) + L" GHz";

       if (!g_Config.overlay.mode[(int)m_CooperationMode].showGpuInfo || g_Config.overlay.mode[(int)m_CooperationMode].showClockSpeeds)
           overlayText += L"\n";
   }

   if (g_Config.overlay.mode[(int)m_CooperationMode].showGpuInfo)
   {
       std::wstring tempTypeStr = m_TempType == TempType::Fahrenheit ? L"\u2109" : L"\u2103";

       if (g_Config.overlay.mode[(int)m_CooperationMode].showCpuInfo && !g_Config.overlay.mode[(int)m_CooperationMode].showClockSpeeds)
           overlayText += L" / ";

       overlayText += L"GPU: " + to_wstring(m_GPUTemp) + tempTypeStr;

       if (m_GpuGddr3RamClock != 0 && g_Config.overlay.mode[(int)m_CooperationMode].showClockSpeeds)
       {
           overlayText += L" / " + to_wstring(m_GpuClock) + L" MHz";
           overlayText += L" / " + to_wstring(m_GpuGddr3RamClock) + L" MHz";
       }
       overlayText += L"\n";
   }

   if (g_Config.overlay.mode[(int)m_CooperationMode].showRamInfo)
   {
       overlayText += L"RAM: " + to_wstring(m_MemoryUsage.percent, 1) 
           + L"% " + to_wstring(m_MemoryUsage.used, 1) 
           + L" / " + to_wstring(m_MemoryUsage.total, 1) 
           + L" MB" 
           + L"\n";
   }

   if (g_Config.overlay.mode[(int)m_CooperationMode].showFanSpeed)
   {
       overlayText += L"Fan speed: " 
           + to_wstring(m_FanSpeed) 
           + L"%\n";
   }

   if (g_Config.overlay.mode[(int)m_CooperationMode].showFirmware)
   {
		// Only build once
		if (!m_CachedPayloadTextBuilt && m_PayloadVersion != 0 && m_FirmwareVersion > 0)
		{
			m_CachedPayloadVersion = m_PayloadVersion;
			m_CachedFirmwareVersion = m_FirmwareVersion;
			m_CachedKernelType = m_KernelType;
		
       std::wstring kernelName;
			switch (m_CachedKernelType)
       {
           case 1: kernelName = L"CEX"; break;
           case 2: kernelName = L"DEX"; break;
           case 3: kernelName = L"DEH"; break;
           default: kernelName = L"N/A";  break;
       }
		
       std::wstring payloadName = IsConsoleHen() ? L"PS3HEN" : IsConsoleMamba() ? L"Mamba" : L"Cobra";
		
			std::wstring payloadVerStr = to_wstring(m_CachedPayloadVersion >> 8) + L"." +
										to_wstring((m_CachedPayloadVersion & 0xF0) >> 4);
       if (IsConsoleHen())
				payloadVerStr += L"." + to_wstring(m_CachedPayloadVersion & 0xF);
		
			m_CachedPayloadText = to_wstring(m_CachedFirmwareVersion, 2) + L" " + kernelName + L" " +
								payloadName + L" " + payloadVerStr + L"\n";
		
			m_CachedPayloadTextBuilt = true;
		}
		
		// ✅ Always append, every frame
		overlayText += m_CachedPayloadText;
   }

   if (g_Config.overlay.mode[(int)m_CooperationMode].showAppName && gamePlugin)
   {
       char gameTitleId[16]{};
       char gameTitleName[64]{};
       GetGameName(gameTitleId, gameTitleName);

       bool isTitleIdEmpty = (gameTitleId[0] == NULL) || (gameTitleId[0] == ' ');

       wchar_t appName[100]{};
       vsh::swprintf(appName, sizeof(appName), L"%s %c %s\n", gameTitleName, isTitleIdEmpty ? ' ' : '/', gameTitleId);
       overlayText += appName;
   }

   if (g_Config.overlay.mode[(int)m_CooperationMode].showPlayTime && gamePlugin)
   {
       uint64_t msec = 0;

       if (!msec)
           msec = GetCurrentTick();
       else
           msec = 0;

       if (msec)
       {
           uint32_t sec = ((msec + 500) / 1000); // milliseconds to seconds

           uint32_t totalSeconds = sec;
           //uint32_t days = totalSeconds / 86400;
           totalSeconds = totalSeconds % 86400;

           uint32_t hours = totalSeconds / 3600;
           totalSeconds = totalSeconds % 3600;

           uint32_t minutes = totalSeconds / 60;
           totalSeconds = totalSeconds % 60;

           uint32_t seconds = totalSeconds;

           wchar_t playTimeStr[60]{};
           vsh::swprintf(playTimeStr, sizeof(playTimeStr), L"Play Time: %02d:%02d:%02d\n", hours, minutes, seconds);

           overlayText += playTimeStr;
       }
   }


   g_Render.Text(
      overlayText,
      m_Position,
      g_Config.overlay.mode[(int)m_CooperationMode].textSize,
      m_HorizontalAlignment,
      m_VerticalAlignment,
      m_ColorText);
}

void Overlay::UpdatePosition()
{
    switch (g_Config.overlay.mode[(int)m_CooperationMode].positionStyle)
    {
        case Config::PostionStyle::TOP_LEFT:
        {
            m_Position.x = -vsh::paf::PhWidget::GetViewportWidth() / 2 + m_SafeArea.x + 5;
            m_Position.y = vsh::paf::PhWidget::GetViewportHeight() / 2 - m_SafeArea.y - 5;

            m_VerticalAlignment = Render::Align::Top;
            m_HorizontalAlignment = Render::Align::Left;
            break;
        }
        case Config::PostionStyle::TOP_RIGHT:
        {
            m_Position.x = vsh::paf::PhWidget::GetViewportWidth() / 2 - m_SafeArea.x - 5;
            m_Position.y = vsh::paf::PhWidget::GetViewportHeight() / 2 - m_SafeArea.y - 5;

            m_VerticalAlignment = Render::Align::Top;
            m_HorizontalAlignment = Render::Align::Right;
            break;
        }
        case Config::PostionStyle::BOTTOM_LEFT:
        {
            m_Position.x = -vsh::paf::PhWidget::GetViewportWidth() / 2 + m_SafeArea.x + 5;
            m_Position.y = -vsh::paf::PhWidget::GetViewportHeight() / 2 + m_SafeArea.y + 5;

            m_VerticalAlignment = Render::Align::Bottom;
            m_HorizontalAlignment = Render::Align::Left;
            break;
        }
        case Config::PostionStyle::BOTTOM_RIGHT:
        {
            m_Position.x = vsh::paf::PhWidget::GetViewportWidth() / 2 - m_SafeArea.x - 5;
            m_Position.y = -vsh::paf::PhWidget::GetViewportHeight() / 2 + m_SafeArea.y + 5;

            m_VerticalAlignment = Render::Align::Bottom;
            m_HorizontalAlignment = Render::Align::Right;
            break;
        }
    }
}

void Overlay::CalculateFps()
{
   // FPS REPORTING
   // get current timing info
   float timeNow = (float)sys_time_get_system_time() * .000001f;
   float fElapsedInFrame = (float)(timeNow - m_FpsLastTime);
   m_FpsLastTime = timeNow;
   ++m_FpsFrames;
   m_FpsTimeElapsed += fElapsedInFrame;

   // report fps at appropriate interval
   if (m_FpsTimeElapsed >= m_FpsTimeReport)
   {
      m_FPS = (m_FpsFrames - m_FpsFramesLastReport) * 1.f / (float)(m_FpsTimeElapsed - m_FpsTimeLastReport);

      m_FpsTimeReport += m_FpsREPORT_TIME;
      m_FpsTimeLastReport = m_FpsTimeElapsed;
      m_FpsFramesLastReport = m_FpsFrames;
   }
}

void Overlay::GetGameName(char outTitleId[16], char outTitleName[64])
{
    vsh::paf::View* gamePlugin = vsh::paf::View::Find("game_plugin");
    if (!gamePlugin)
        return;

    vsh::game_plugin_interface* game_interface = gamePlugin->GetInterface<vsh::game_plugin_interface*>(1);
    if (!game_interface)
        return;

    char _gameInfo[0x120]{};
    game_interface->gameInfo(_gameInfo);
    vsh::snprintf(outTitleId, 10, "%s", _gameInfo + 0x04);
    vsh::snprintf(outTitleName, 63, "%s", _gameInfo + 0x14);
}

union clock_s
{
public:
	struct
	{
	public:
		uint32_t junk0;
		uint8_t junk1;
		uint8_t junk2;
		uint8_t mul;
		uint8_t junk3;
	};

	uint64_t value;
};

uint32_t Overlay::GetGpuClockSpeed()
{
	clock_s clock;
	clock.value = PeekLv1(0x28000004028);

	if (clock.value == 0xFFFFFFFF80010003) // if cfw syscalls are disabled 
		return 0;

	return (clock.mul * 50);
}

uint32_t Overlay::GetGpuGddr3RamClockSpeed()
{
	clock_s clock;
	clock.value = PeekLv1(0x28000004010);

	if (clock.value == 0xFFFFFFFF80010003) // if cfw syscalls are disabled 
		return 0;

	return (clock.mul * 25);
}

uint32_t Overlay::GetCpuClockSpeed()
{
#if 0

    if (IsConsoleHen())
        return 0;

    if (!m_CpuClockSpeedOffsetInLv1)
        return 0;

    uint64_t frequency = PeekLv1(m_CpuClockSpeedOffsetInLv1);

    if (frequency == 0xFFFFFFFF80010003) // if cfw syscalls are disabled 
        return 0;

    return ((static_cast<uint32_t>(frequency >> 32) / 0xF4240) & 0x1FFF) * 8;

#endif

	return 3200;
}

void Overlay::Lv2LabelUpdate()
{
    if (m_Lv2Label.empty())
        return;

    /* 
    float textWrap = paf_6941C365();
    int lineCount = GetLineCount();
    float textHeight = GetTextHeight();
    float rectangleHeight = (static_cast<float>(lineCount) * textHeight);
    g_Render.Rectangle(
        vsh::vec2(vsh::paf::PhWidget::GetViewportWidth() / 2 - m_SafeArea.x - 5, vsh::paf::PhWidget::GetViewportHeight() / 2 - m_SafeArea.y - 100),
        notificationWidth,
        rectangleHeight);*/

    g_Render.Text(
        m_Lv2Label,
        vsh::vec2(vsh::paf::PhWidget::GetViewportWidth() / 2 - m_SafeArea.x - 5, vsh::paf::PhWidget::GetViewportHeight() / 2 - m_SafeArea.y - 100),
        g_Config.overlay.mode[(int)m_CooperationMode].textSize,
        Render::Align::Right,
        Render::Align::Top,
        m_ColorText);
}

void Overlay::WaitAndQueueTextInLV2()
{
    const int size = MAX_LV2_STRING_SIZE / 8;
    char text[size][8]{};
    vsh::memset(text, 0, sizeof(text));

    for (uint64_t i = 0; i < size; i++)
    {
        uint64_t bytes = PeekLv2(m_NotificationOffsetInLv2 + (i * 8));

        if (bytes == 0xFFFFFFFF80010003) // if cfw syscalls are disabled 
            goto clear_text;

        if (bytes != 0)
            vsh::memcpy(text[i], &bytes, 8);
    }

    // force null terminator
    text[size - 1][8 - 1] = 0;

    // check if string is not empty
    if (text[0] != 0 && text[0][0] != '\0')
        m_Lv2Label = reinterpret_cast<char*>(text);
    else
    {
    clear_text:
        if (!m_Lv2Label.empty())
            m_Lv2Label.clear();
    }
}

void Overlay::UpdateInfoThread(uint64_t arg)
{
	if (!g_Overlay.m_CachedPayloadTextBuilt)
		g_Overlay.m_PayloadVersion = GetPayloadVersion();
   g_Overlay.m_StateRunning = true;

   while (g_Overlay.m_StateRunning)
   {
      Sleep(refreshDelay * 1000);

      // Using syscalls in a loop on hen will cause a black screen when launching a game
      // so in order to fix this we need to sleep 10/15 seconds when a game is launched
      if (g_Helpers.m_StateGameJustLaunched)
      {
          Sleep(15 * 1000);
          g_Helpers.m_StateGameJustLaunched = false;
      }

      g_Overlay.m_MemoryUsage = GetMemoryUsage();
      // Convert to MB
      g_Overlay.m_MemoryUsage.total /= 1024;
      g_Overlay.m_MemoryUsage.free /= 1024;
      g_Overlay.m_MemoryUsage.used /= 1024;

      g_Overlay.m_FanSpeed = GetFanSpeed();

      switch (g_Config.overlay.mode[(int)g_Overlay.m_CooperationMode].temperatureType)
      {
      case Config::TemperatureType::BOTH:
          if (g_Overlay.m_CycleTemperatureType)
          {
              g_Overlay.m_CPUTemp = GetTemperatureFahrenheit(0);
              g_Overlay.m_GPUTemp = GetTemperatureFahrenheit(1);
              g_Overlay.m_TempType = TempType::Fahrenheit;
          }
          else
          {
              g_Overlay.m_CPUTemp = GetTemperatureCelsius(0);
              g_Overlay.m_GPUTemp = GetTemperatureCelsius(1);
              g_Overlay.m_TempType = TempType::Celsius;
          }
          break;
      case Config::TemperatureType::CELSIUS:
          g_Overlay.m_CPUTemp = GetTemperatureCelsius(0);
          g_Overlay.m_GPUTemp = GetTemperatureCelsius(1);
          g_Overlay.m_TempType = TempType::Celsius;
          break;
      case Config::TemperatureType::FAHRENHEIT:
          g_Overlay.m_CPUTemp = GetTemperatureFahrenheit(0);
          g_Overlay.m_GPUTemp = GetTemperatureFahrenheit(1);
          g_Overlay.m_TempType = TempType::Fahrenheit;
          break;
      default:
          if (g_Overlay.m_CycleTemperatureType)
          {
              g_Overlay.m_CPUTemp = GetTemperatureFahrenheit(0);
              g_Overlay.m_GPUTemp = GetTemperatureFahrenheit(1);
              g_Overlay.m_TempType = TempType::Fahrenheit;
          }
          else
          {
              g_Overlay.m_CPUTemp = GetTemperatureCelsius(0);
              g_Overlay.m_GPUTemp = GetTemperatureCelsius(1);
              g_Overlay.m_TempType = TempType::Celsius;
          }
          break;
      }

      uint64_t timeNow = GetTimeNow();
      if (timeNow - g_Overlay.m_TemperatureCycleTime > 5000)
      {
          g_Overlay.m_CycleTemperatureType ^= 1;
          g_Overlay.m_TemperatureCycleTime = timeNow;
      }

      int ret = get_target_type(&g_Overlay.m_KernelType);
      if (ret != SUCCEEDED)
          g_Overlay.m_KernelType = 0;

      g_Overlay.m_FirmwareVersion = GetFirmwareVersion();

      //g_Overlay.m_PayloadVersion = GetPayloadVersion();

      if (g_Config.overlay.mode[(int)g_Overlay.m_CooperationMode].showClockSpeeds)
      {
          g_Overlay.m_CpuClock = g_Overlay.GetCpuClockSpeed();
          g_Overlay.m_GpuClock = g_Overlay.GetGpuClockSpeed();
          g_Overlay.m_GpuGddr3RamClock = g_Overlay.GetGpuGddr3RamClockSpeed();
      }

      g_Overlay.WaitAndQueueTextInLV2();
   }

   sys_ppu_thread_exit(0);
}

void Overlay::LoadExternalOffsets(uint64_t arg)
{
#if 0
    // Reference: https://github.com/Evilnat/xai_plugin/blob/fbfd74636f9f947bea097a1b6990d46115fd1c31/xai_plugin/cfw_settings.cpp#L575-L627
    char rsxData[120];
    char core[25], memory[25];
    uint8_t hex[8];
    uint64_t rsx_values = 0;

    for (uint64_t offset = 0x600000; offset < 0x700000; offset++)
    {
        if (PeekUint32LV1(offset) == 0x7670653A &&
            (PeekUint32LV1(offset + 7) == 0x7368643A || PeekUint32LV1(offset + 8) == 0x7368643A) &&
            PeekUint8LV1(offset - 5) == 0x2F)
        {
            rsx_values = PeekLv1(offset - 8);
            break;
        }
    }


    vsh::memcpy(hex, &rsx_values, 8);

    for (int i = 0; i < 8; i++)
        vsh::snprintf(&rsxData[i], sizeof(rsxData), "%c", hex[i]);

    vsh::strncpy(core, rsxData, 3);
    vsh::strncpy(memory, rsxData + 4, 3);
#else
    //std::vector<uint32_t> foundOffsets;
    //foundOffsets.reserve(1); // reserve 1 offsets for our use case

    //std::vector<Pattern> patterns = { 
    //    { "be.0.ref_clk", "xxxxxxxxxxxx", false }
    //};

    //FindPatternsHypervisorInParallel(patterns, foundOffsets);
    
    //g_Overlay.m_CpuClockSpeedOffsetInLv1 = foundOffsets[0] + 0x24;

#ifdef OLD_CODE
    uint32_t addr = FindPatternHypervisor(
        "be.0.ref_clk",
        vsh::strlen("be.0.ref_clk"),
        "xxxxxxxxxxxx");
    g_Overlay.m_CpuClockSpeedOffsetInLv1 = addr + 0x24;

    addr = FindPatternHypervisor(
        "\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x01\x00\x00\x40\x28\x00\x00\x40\x2C",
        20,
        "???x???x???x??xx??xx");
    g_Overlay.m_GpuClockSpeedOffsetInLv1 = addr + 0x14;

    addr = FindPatternHypervisor(
        "\x00\x00\x00\x05\x00\x00\x00\x03\x00\x00\x00\x06\x00\x00\x40\x10\x00\x00\x40\x14",
        20,
        "???x???x???x??xx??xx");
    g_Overlay.m_GpuGddr3RamClockSpeedOffsetInLv1 = addr + 0x14;
#endif // OLD_CODE

#endif

    sys_ppu_thread_exit(0);
}