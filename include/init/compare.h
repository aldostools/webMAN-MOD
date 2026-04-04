extern int extcmp(const char *s1, const char *s2, size_t n);
extern int extcasecmp(const char *s1, const char *s2, size_t n);

static bool IS(const char *a, const char *b)
{
	if(!a || !b ||!*a) return false;
	return !strcmp(a, b); // compare two strings. returns true if they are identical
}

static bool _IS(const char *a, const char *b)
{
	if(!a || !b || !*a) return false;
	return !strcasecmp(a, b);	// compare two strings. returns true if they are identical (case insensitive)
}

#ifndef LITE_EDITION
static bool _islike(const char *param, const char *text)
{
	if(!param || !text) return false;
	return strncasecmp(param, text, strlen(text)) == 0;
}
#endif

static bool islike(const char *param, const char *text)
{
	if(!param || !text) return false;
	while(*text && (*text == *param)) text++, param++;
	return !*text;
}

#if defined(PS3MAPI) || defined(DEBUG_MEM)
static u64 faster_find(const char *find, int len, char *mask)
{
	if(!mask || !find) return 0;

	u64 faster = *(u64*)find; // first 64bits

	replace_char(mask, '*', '?'); // use single type of wildcard

	if(len < 8)
		faster = 0; // do not fast find if len < 8
	else
	{
		char s = mask[8]; mask[8] = '\0'; // truncate to search only first 8 characters
		if(strchr(mask, '?')) faster = 0; // do not fast find if using mask
		mask[8] = s; // restore mask
	}

	return faster;
}

static bool bcompare(const char *a, const char *b, u8 len, const char *mask)
{
	while(len && ((*a++ == *b++) || (*mask == '?'))) {mask++,len--;}
	return len; // false = match, true = no match
}
#endif
