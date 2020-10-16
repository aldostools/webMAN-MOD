#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>

#ifdef DEBUG
#include <stdio.h>
#define DPRINTF printf
#else
#define DPRINTF(...)
#endif

#ifndef _OS_WINDOWS
#include <endian.h>
#define __BIG_ENDIAN__ (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#endif
#if __BIG_ENDIAN__

static inline uint16_t BE16(uint16_t x)
{
	return x;
}

static inline uint32_t BE32(uint32_t x)
{
	return x;
}

static inline uint64_t BE64(uint64_t x)
{
	return x;
}

static inline uint16_t LE16(uint16_t x)
{
	uint16_t ret;
	ret  =  (x<<8)&0xff00;
	ret |= ((x>>8)&0xff);

	return ret;
}

static inline uint32_t LE32(uint32_t x)
{
	uint32_t ret;
	ret  = (((x) & 0xff) << 24);
	ret |= (((x) & 0xff00) << 8);
	ret |= (((x) & 0xff0000) >> 8);
	ret |= (((x) >> 24) & 0xff);

	return ret;
}

static inline uint64_t LE64(uint64_t x)
{
	uint64_t ret;
	ret  = ((x << 56) & 0xff00000000000000ULL);
	ret |= ((x << 40) & 0x00ff000000000000ULL);
	ret |= ((x << 24) & 0x0000ff0000000000ULL);
	ret |= ((x << 8)  & 0x000000ff00000000ULL);
	ret |= ((x >> 8)  & 0x00000000ff000000ULL);
	ret |= ((x >> 24) & 0x0000000000ff0000ULL);
	ret |= ((x >> 40) & 0x000000000000ff00ULL);
	ret |= ((x >> 56) & 0x00000000000000ffULL);

	return ret;
}

#else

static inline uint16_t BE16(uint16_t x)
{
	uint16_t ret;
	ret  =  (x << 8) & 0xff00;
	ret |= ((x >> 8) & 0xff);

	return ret;
}

static inline uint32_t BE32(uint32_t x)
{
	uint32_t ret;
	ret  = (((x) & 0xff) << 24);
	ret |= (((x) & 0xff00) << 8);
	ret |= (((x) & 0xff0000) >> 8);
	ret |= (((x) >> 24) & 0xff);

	return ret;
}

static inline uint64_t BE64(uint64_t x)
{
	uint64_t ret;
	ret  = ((x << 56) & 0xff00000000000000ULL);
	ret |= ((x << 40) & 0x00ff000000000000ULL);
	ret |= ((x << 24) & 0x0000ff0000000000ULL);
	ret |= ((x << 8)  & 0x000000ff00000000ULL);
	ret |= ((x >> 8)  & 0x00000000ff000000ULL);
	ret |= ((x >> 24) & 0x0000000000ff0000ULL);
	ret |= ((x >> 40) & 0x000000000000ff00ULL);
	ret |= ((x >> 56) & 0x00000000000000ffULL);

	return ret;
}

static inline uint16_t LE16(uint16_t x)
{
	return x;
}

static inline uint32_t LE32(uint32_t x)
{
	return x;
}

static inline uint64_t LE64(uint64_t x)
{
	return x;
}

#endif

#endif /* __COMMON_H__ */
