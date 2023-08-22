#include "Rendering.hpp"

Render g_Render;

void Render::OnUpdate()
{
   if (!g_Helpers.system_plugin)
      return;

   if (!g_Helpers.page_notification)
   {
      DestroyWidgets();
      return;
   }

   vsh::paf::PhText* phText = GetText(0);
   if (phText)
   {
      if (!phText->IsAttached())
      {
         DestroyWidgets();
         return;
      }
   }
   else
   {
      CreateWidgets();
      return;
   }

   m_CurrentText = 0;
   ClearTexts();
}

void Render::Text(const std::wstring& text, vsh::vec2 position, float height, Align horizontalAlign, Align verticalAlign, vsh::vec4 color, float angle)
{
   vsh::paf::PhText* phText = GetText(m_CurrentText);
   if (!phText)
      return;

   if (!phText->IsAttached())
      return;

   position.y += (height * 0.15);

   phText->SetText(text)
      .SetTextHeight(height * 0.8)
      .SetPosition(position)
      .SetColor(color)
      .SetStyle(vsh::paf::PhWidget::Anchor, int(horizontalAlign | verticalAlign))
      .SetStyle(vsh::paf::PhWidget::TextAlignment, int(horizontalAlign))
      .SetRotation(angle, false);

   m_CurrentText++;
}

void Render::Text(const std::string& text, vsh::vec2 position, float height, Align horizontalAlign, Align verticalAlign, vsh::vec4 color, float angle)
{
    std::wstring ws(text.begin(), text.end());
    Text(ws, position, height, horizontalAlign, verticalAlign, color, angle);
}

vsh::paf::PhText* Render::CreateText(const std::string& widgetName)
{
   if (!g_Helpers.page_notification)
      return nullptr;

   vsh::paf::PhText* phText = new vsh::paf::PhText(g_Helpers.page_notification);
   phText->SetName(widgetName)
      .SetColor(vsh::vec4())
      .SetStyle(0x13, 0x70);
   return phText;
}

vsh::paf::PhText* Render::GetText(int index)
{
   if (m_TextList.empty())
      return nullptr;

   if (index > MAX_TEXTS || index < 0)
      return nullptr;

   return m_TextList[index];
}

void Render::CreateWidgets()
{
   if (!g_Helpers.page_notification)
      return;

   // don't create any more planes if we already created the first one
   if (vsh::paf::PhText* phText = GetText(0))
      return;
   else
   {
      m_TextList.clear();
   }

   vsh::srand(mix_time_seed(clock(), time(NULL), sys_process_getpid()));
   for (int i = 0; i < MAX_TEXTS; i++)
   {
      vsh::paf::PhText* phText = CreateText("CRenderText_" + to_string(vsh::rand()));
      if (!phText)
         break;

      m_TextList.push_back(phText);
   }
}

void Render::DestroyWidgets()
{
   if (!g_Helpers.page_notification)
      return;

   // clear pointers if no longer valid
   if (vsh::paf::PhText* phText = GetText(0))
   {
      if (!phText->IsAttached())
      {
         m_TextList.clear();
         return;
      }
   }

   for (int i = 0; i < MAX_TEXTS; i++)
   {
      vsh::paf::PhText* phText = GetText(i);
      if (!phText)
         continue;

      delete phText;
   }

   m_TextList.clear();
}

void Render::ClearTexts()
{
   if (!g_Helpers.page_notification)
      return;

   if (vsh::paf::PhText* phText = GetText(0))
      if (!phText->IsAttached())
         return;

   for (int i = 0; i < MAX_TEXTS; i++)
   {
      vsh::paf::PhText* phText = GetText(i);
      if (!phText)
         continue;

      if (!phText->IsAttached())
         continue;

      phText->SetColor(vsh::vec4());
   }
}