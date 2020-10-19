#ifndef PS3NETSRV_CONSOLE_H
#define PS3NETSRV_CONSOLE_H

#include <map>

#ifndef NDEBUG
#define debug_enabled true
#else
#define debug_enabled false
#endif

enum class Color {
	Normal = 0,
	White,
	Red,
	Gray
};

class Console
{
public:
	Console(const Console& other) = delete;
	Console(Console&&) = delete;
	Console& operator=(const Console&) = delete;
	Console& operator=(Console&&) = delete;

	static Console& get(Color defaultColor = Color::Normal);

	Color get_textColor();
	void set_textColor(Color color);

	void print(const char *format ...);
	void print(Color color, const char *format ...);
	static inline void debug_print(const char *format ...) { if (debug_enabled) get().print(format); }

	void wait();

private:
	explicit Console(Color defaultColor);
	~Console() = default;

	std::map<Color, int> colorMap;
	Color textColor;
};

#endif //PS3NETSRV_CONSOLE_H