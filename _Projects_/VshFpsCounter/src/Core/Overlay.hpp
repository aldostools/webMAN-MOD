#pragma once
#include <string>
#include <sys/ppu_thread.h>
#include <vsh/netctl_main.h>
#include <vsh/vshmain.h>
#include <vsh/vshcommon.h>
#include <vsh/pafView.h>
#include <vsh/explore_plugin.h>
#include <vsh/game_plugin.h>
#include "Utils/ConsoleInfo.hpp"
#include "Core/Helpers.hpp"
#include "Core/Rendering.hpp"
#include "Core/Configuration.hpp"

class Overlay
{
public:
    enum class TempType : uint8_t
    {
        Fahrenheit,
        Celsius,
    };

public:
   Overlay();

   void OnUpdate();
   void OnShutdown();
   void Lv2LabelUpdate();
   void WaitAndQueueTextInLV2();

private:
   void DrawOverlay();
   void UpdatePosition();
   void CalculateFps();
   void GetGameName(char outTitleId[16], char outTitleName[64]);
   uint32_t GetGpuClockSpeed();
   uint32_t GetGpuGddr3RamClockSpeed();
   uint32_t GetCpuClockSpeed();
   static void UpdateInfoThread(uint64_t arg);
   static void LoadExternalOffsets(uint64_t arg);

public:
   sys_ppu_thread_t UpdateInfoThreadId = SYS_PPU_THREAD_ID_INVALID;
   sys_ppu_thread_t LoadExternalOffsetsThreadId = SYS_PPU_THREAD_ID_INVALID;
   bool m_StateRunning{};
   float m_CPUTemp{};
   float m_GPUTemp{};
   float m_FanSpeed{};
   memUsage_s m_MemoryUsage{};
   uint64_t m_KernelType{};
   float m_FirmwareVersion{};
   uint16_t m_PayloadVersion{};
   std::wstring payloadText;
   bool m_CheckedHenVersion = false;
   //bool m_CachedPayloadText = false;
   uint16_t m_CachedPayloadVersion = 0;
   float m_CachedFirmwareVersion = 0;
   uint64_t m_CachedKernelType = 0;
   std::wstring m_CachedPayloadText;  // For the version line at the bottom
   bool m_CachedPayloadTextBuilt = false;
   uint64_t m_TemperatureCycleTime{};
   bool m_CycleTemperatureType{};
   TempType m_TempType{};

   uint32_t m_CpuClock{};
   uint32_t m_GpuClock{};
   uint32_t m_GpuGddr3RamClock{};
   uint64_t m_CpuClockSpeedOffsetInLv1{};

private:
    // Common
    vsh::eCooperationMode m_CooperationMode;

    // Positioning
   vsh::vec2 m_Position{};
   Render::Align m_HorizontalAlignment{};
   Render::Align m_VerticalAlignment{};
   vsh::vec2 m_SafeArea{ 31, 18 };
   vsh::vec4 m_ColorText{ 1, 1, 1, 1 };
   static const int refreshDelay = 1;

   // FPS
   float m_FPS = 100.0f;
   float m_FpsLastTime = 0;
   int m_FpsFrames = 0;
   int m_FpsFramesLastReport = 0;
   double m_FpsTimeElapsed = 0;
   double m_FpsTimeReport = 0;
   double m_FpsTimeLastReport = 0;
   float m_FpsREPORT_TIME = 1.0f;

   // LV2 Label Text
   static constexpr size_t MAX_LV2_STRING_SIZE = 0x80;
   std::string m_Lv2Label;
   uint64_t m_NotificationOffsetInLv2 = 0x8000000000700000;

   // Config 
   uint64_t m_ReloadConfigTime{};
};

extern Overlay g_Overlay;