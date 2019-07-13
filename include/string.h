typedef struct {
	u32  size;
	char *str;
} t_string;

static void _alloc(t_string *item, char *mem)
{
	if(!mem) return;
	*mem = NULL;
	item->str  = mem;
	item->size = 0;
}

static void _set(t_string *item, char *mem, u32 len)
{
	if(!mem) return;
	item->str  = mem;
	item->size = len;
}

static char *_concat(t_string *dest, const char *src)
{
	char *str = dest->str + dest->size;

	while (*str) {str++, dest->size++;} // find last byte

	while ((*str++ = *src++)) dest->size++; *str = 0; // append src

	return dest->str;
}
