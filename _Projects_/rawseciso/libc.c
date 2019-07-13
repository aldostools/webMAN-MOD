#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *memset(void *m, int c, size_t n)
{
	char *s = (char *) m;

	while (n-- != 0)
	{
		*s++ = (char) c;
	}

	return m;
}

void *memcpy(void *dst0, const void *src0, size_t len0)
{
	char *dst = (char *)dst0;
	char *src = (char *)src0;

	void *save = dst0;

	while (len0--)
		*dst++ = *src++;

	return save;
}

size_t strlen(const char *str)
{
	const char *start = str;

	while (*str)
		str++;

	return str - start;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
	while((n > 0) && *s1 && (*s1==*s2)) {s1++, s2++, n--;} if(n == 0) return 0;

	return *(const unsigned char*)s1-*(const unsigned char*)s2;
}
