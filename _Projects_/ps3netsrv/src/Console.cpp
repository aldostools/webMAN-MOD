#include <cstdio>
#include <cstdarg>
#ifdef WIN32
#include <windows.h>
#endif

#include "Console.h"

Console::Console(const Color defaultColor)
	: textColor { Color::Normal }
{
#ifdef WIN32
	CONSOLE_SCREEN_BUFFER_INFO console_info;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &console_info);
	colorMap = {
		{ Color::Normal, console_info.wAttributes },
		{ Color::White, 0x0F },
		{ Color::Red, 0x0C },
		{ Color::Gray, 0x08 }
	};
#else
	colorMap = {
		{ Color::Normal, 37 },
		{ Color::White, 37 },
		{ Color::Red, 31 },
		{ Color::Gray, 30 }
	};
#endif
	set_textColor(defaultColor);
}

Console& Console::get(const Color defaultColor)
{
	static Console console(defaultColor);
	return console;
}

Color Console::get_textColor()
{
	return textColor;
}

void Console::set_textColor(const Color color)
{
#ifndef NO_COLOR
	textColor = color;
#ifdef WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colorMap[color]);
#else
	std::printf("\033[1;%dm", colorMap[color]);
#endif
#endif
}

void Console::print(const char *format...)
{
	va_list args;
	va_start(args, format);
	std::vprintf(format, args);
	va_end(args);
}

void Console::print(const Color color, const char *format...)
{
	Color prevColor = textColor;
	set_textColor(color);
	print(format);
	set_textColor(prevColor);
}

void Console::wait()
{
	print("\n\nPress ENTER to continue...");
	std::getchar();
}