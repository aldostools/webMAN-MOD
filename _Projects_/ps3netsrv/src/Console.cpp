#include <cstdarg>

#include "Console.h"

Console* Console::_console = NULL;
Colors Console::curColor = COLOR_NORMAL;
int Console::colorMap[4] = {};
#ifdef WIN32
CONSOLE_SCREEN_BUFFER_INFO Console::console_info = {};
#endif

Console::Console(Colors color)
{
#ifdef WIN32
	_GetConsoleScreenBufferInfo( GetStdHandle( STD_OUTPUT_HANDLE ), &console_info );
	colorMap[COLOR_NORMAL] = (console_info.wAttributes) ? console_info.wAttributes : 0x0F;
	colorMap[COLOR_WHITE] = 0x0F;
	colorMap[COLOR_RED] = 0x0C;
	colorMap[COLOR_GRAY] = 0x08;
#else
	colorMap[COLOR_NORMAL] = 37;
	colorMap[COLOR_WHITE] = 37;
	colorMap[COLOR_RED] = 31;
	colorMap[COLOR_GRAY] = 30;
#endif
	set_text_color(color);
}

void Console::init(Colors color)
{
	if (_console == NULL)
		_console = new Console(color);
}

int Console::get_text_color()
{
	return curColor;
}

void Console::set_text_color(Colors color)
{
	curColor = color;
	_set_text_color(colorMap[color]);
}

void Console::print(char const *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

void Console::print(Colors color, char const *fmt, ...)
{
	Colors prevColor = curColor;
	set_text_color(color);
	print(fmt);
	set_text_color(prevColor);
}

void Console::wait()
{
	print("\n\nPress ENTER to continue...");
	getchar();
}