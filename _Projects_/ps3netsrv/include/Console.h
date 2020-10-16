#ifndef PS3NETSRV_CONSOLE_H
#define PS3NETSRV_CONSOLE_H

#include <cstdio>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef NO_COLOR
#define _GetConsoleScreenBufferInfo(a, b)
#define _set_text_color(a)
#else
#ifdef WIN32
#define _GetConsoleScreenBufferInfo(a, b) GetConsoleScreenBufferInfo(a, b)
#define _set_text_color(a) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), a)
#else
#define _set_text_color(a) printf("\033[1;%dm", a)
#endif
#endif

enum Colors {
	COLOR_NORMAL = 0,
	COLOR_WHITE,
	COLOR_RED,
	COLOR_GRAY
};

class Console
{
public:
	static void init(Colors color = COLOR_NORMAL);

	static int get_text_color();
	static void set_text_color(Colors color);

	static void print(char const *fmt, ...);
	static void print(Colors color, char const *fmt, ...);

	static void wait();

private:
	explicit Console(Colors color = COLOR_NORMAL);

	static Console *_console;
	static Colors curColor;
	static int colorMap[4];
#ifdef WIN32
	static CONSOLE_SCREEN_BUFFER_INFO console_info;
#endif
};

#endif //PS3NETSRV_CONSOLE_H