#define SC_PEEK_LV2						(6)
#define SC_POKE_LV2						(7)
#define SC_PEEK_LV1 					(8)
#define SC_POKE_LV1 					(9)

#define PS3MAPI_OPCODE_LV2_PEEK			0x1006
#define PS3MAPI_OPCODE_LV2_POKE			0x1007
#define PS3MAPI_OPCODE_LV1_PEEK			0x1008
#define PS3MAPI_OPCODE_LV1_POKE			0x1009

#define SYSCALL8_OPCODE_PS3MAPI			0x7777
#define PS3MAPI_OPCODE_LV1_POKE			0x1009

/////////////////// LV1 PEEK //////////////////////
static u64 lv1_peek_cfw(u64 addr)
{
	system_call_1(SC_PEEK_LV1, addr);
	return (u64) p1;
}

#ifdef COBRA_ONLY
static u64 lv1_peek_ps3mapi(u64 addr)
{
	system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LV1_PEEK, addr);
	return (u64) p1;
}
#endif

/////////////////// LV1 PEEK //////////////////////
static void lv1_poke_cfw( u64 addr, u64 value)
{
	system_call_2(SC_POKE_LV1, addr, value);
}

#ifdef COBRA_ONLY
static void lv1_poke_ps3mapi(u64 addr, u64 value)
{
	system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LV1_POKE, addr, value);
}
#endif

/////////////////// LV2 PEEK //////////////////////
static u64 lv2_peek_hen(u64 addr) //sc6
{
	system_call_1(SC_PEEK_LV2, addr);
	return (u64) p1;
}

static u64 lv2_peek_cfw(u64 addr) //sc8 + LV2_OFFSET_ON_LV1
{
	system_call_1(SC_PEEK_LV1, addr + LV2_OFFSET_ON_LV1); //old: {system_call_1(SC_PEEK_LV2, addr);}
	return (u64) p1;
}

#ifdef COBRA_ONLY
static u64 lv2_peek_ps3mapi(u64 addr) //sc8 + ps3mapi
{
	system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LV2_PEEK, addr);
	return (u64) p1;
}
#endif

/////////////////// LV2 POKE //////////////////////
static void lv2_poke_hen(u64 addr, u64 value) //sc7
{
	system_call_2(SC_POKE_LV2, addr, value);
}

static void lv2_poke_cfw(u64 addr, u64 value) //sc8 + LV2_OFFSET_ON_LV1
{
	system_call_2(SC_POKE_LV1, addr + LV2_OFFSET_ON_LV1, value);
}

#ifdef COBRA_ONLY
static void lv2_poke_ps3mapi(u64 addr, u64 value) //sc8 + ps3mapi
{
	system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LV2_POKE, addr, value);
}
#endif
///////////////////////////////////////////////////

static void (*lv2_poke_fan)(u64, u64) = lv2_poke_cfw;  // ps3hen: lv2_poke_fan = lv2_poke_fan_hen;

static u64  (*peekq)(u64) = lv2_peek_cfw;
static void (*pokeq)(u64, u64) = lv2_poke_cfw;

static u64  (*peek_lv1)(u64) = lv1_peek_cfw;
static void (*poke_lv1)(u64, u64) = lv1_poke_cfw;

static u64 peek(u64 addr)
{
	return peekq(addr | 0x8000000000000000ULL);
}
///////////////// LV2 POKE32 //////////////////////
static void lv2_poke_fan_hen(u64 addr, u64 value)
{
	system_call_2(SC_POKE_LV2, addr, value); //{system_call_3(SC_COBRA_SYSCALL8, 0x7003ULL, addr, value);} // advanced poke (requires restore original value)
}

///////////////////////////////////////////////////

/*
static void lv2poke32(u64 addr, u32 value)
{
	u64 new_value = (((u64) value) <<32) | (peekq(addr) & 0xffffffffULL);
	pokeq(addr, new_value);
}

static u32 lv2peek32(u64 addr)
{
	u32 ret = (u32) (peekq(addr) >> 32ULL);
	return ret;
}
*/

#ifndef COBRA_ONLY
static inline void remove_lv2_memory_protection(void)
{
	u64 HV_START_OFFSET = 0;

	//Remove Lv2 memory protection
	if(c_firmware==3.55f)
	{
		HV_START_OFFSET = HV_START_OFFSET_355;
	}
	else
	if(c_firmware==4.21f)
	{
		HV_START_OFFSET = HV_START_OFFSET_421;
	}
	else
	if(c_firmware>=4.30f && c_firmware<=4.53f)
	{
		HV_START_OFFSET = HV_START_OFFSET_430; // same for 4.30-4.53
	}
	else
	if(c_firmware>=4.55f /*&& c_firmware<=4.84f*/)
	{
		HV_START_OFFSET = HV_START_OFFSET_455; // same for 4.55-4.84
	}

	if(!HV_START_OFFSET) return;

	poke_lv1(HV_START_OFFSET + 0x00, 0x0000000000000001ULL);
	poke_lv1(HV_START_OFFSET + 0x08, 0xe0d251b556c59f05ULL);
	poke_lv1(HV_START_OFFSET + 0x10, 0xc232fcad552c80d7ULL);
	poke_lv1(HV_START_OFFSET + 0x18, 0x65140cd200000000ULL);
}

static void install_peek_poke(void)
{
	remove_lv2_memory_protection();

	if(c_firmware>=4.30f /*&& c_firmware<=4.84f*/)
	{	// add lv2 peek/poke + lv1 peek/poke
		pokeq(0x800000000000171CULL + 0x00, 0x7C0802A6F8010010ULL);
		pokeq(0x800000000000171CULL + 0x08, 0x396000B644000022ULL);
		pokeq(0x800000000000171CULL + 0x10, 0x7C832378E8010010ULL);
		pokeq(0x800000000000171CULL + 0x18, 0x7C0803A64E800020ULL);
		pokeq(0x800000000000171CULL + 0x20, 0x7C0802A6F8010010ULL);
		pokeq(0x800000000000171CULL + 0x28, 0x396000B744000022ULL);
		pokeq(0x800000000000171CULL + 0x30, 0x38600000E8010010ULL);
		pokeq(0x800000000000171CULL + 0x38, 0x7C0803A64E800020ULL);
		pokeq(0x800000000000171CULL + 0x40, 0x7C0802A6F8010010ULL);
		pokeq(0x800000000000171CULL + 0x48, 0x7D4B537844000022ULL);
		pokeq(0x800000000000171CULL + 0x50, 0xE80100107C0803A6ULL);
		pokeq(0x800000000000171CULL + 0x58, 0x4E80002080000000ULL); // sc6  @ 0x8000000000001778 = 800000000000170C
		pokeq(0x800000000000171CULL + 0x60, 0x0000170C80000000ULL); // sc7  @ 0x8000000000001780 = 8000000000001714
		pokeq(0x800000000000171CULL + 0x68, 0x0000171480000000ULL); // sc8  @ 0x8000000000001788 = 800000000000171C
		pokeq(0x800000000000171CULL + 0x70, 0x0000171C80000000ULL); // sc9  @ 0x8000000000001790 = 800000000000173C
		pokeq(0x800000000000171CULL + 0x78, 0x0000173C80000000ULL); // sc10 @ 0x8000000000001798 = 800000000000175C
		pokeq(0x800000000000171CULL + 0x80, 0x0000175C00000000ULL);

		// enable syscalls 6, 7, 8, 9, 10
		for(u8 sc = 6; sc < 11; sc++)
			pokeq(SYSCALL_PTR(sc), 0x8000000000001748ULL + sc * 8ULL); // 0x8000000000001778 (sc6) to 0x8000000000001798 (sc10)
	}
}

#define MAX_PATH_MAP	384

typedef struct
{
	char src[MAX_PATH_MAP];
	char dst[MAX_PATH_MAP];
} redir_files_struct;

static redir_files_struct file_to_map[10];

static void add_to_map(const char *path1, const char *path2)
{
	if(max_mapped == 0) pokeq(MAP_BASE + 0x00, 0x0000000000000000ULL);

	if(max_mapped < 10)
	{
		for(u8 n = 0; n < max_mapped; n++)
		{
			if(IS(file_to_map[n].src, path1)) return;
		}

		sprintf(file_to_map[max_mapped].src, "%s", path1);
		sprintf(file_to_map[max_mapped].dst, "%s", path2);
		max_mapped++;
	}
}

static u16 string_to_lv2(char* path, u64 addr)
{
	u16 plen = strlen(path); 
	u16 len  = (plen + 8) & 0x7f8;

	len = RANGE(len, 8, MAX_PATH_MAP);
	if(plen > len) plen = len;

	u8 path_buf[MAX_PATH_MAP];
	u8 *data  = path_buf;

	memcpy(data, path, plen);
	memset(data + plen, 0, MAX_PATH_MAP - plen);

	u64 *data2 = (u64 *)path_buf;
	for(u64 n = 0; n < len; n += 8)
	{
		pokeq(addr + n, data2[n]);
	}
	return len;
}
#endif

#if defined(PS3MAPI) || defined(DEBUG_MEM) || defined(SPOOF_CONSOLEID)

static u64 convertH(char *val)
{
	u64 ret = 0; char c;

	for(u8 buff, i = 0, n = 0; i < 16 + n; i++)
	{
		if(val[i]==' ') {n++; continue;}

		c = (val[i] | 0x20);
		if(c >= '0' && c <= '9') buff = (c - '0');      else
		if(c >= 'a' && c <= 'f') buff = (c - 'a' + 10); else
		return ret;

		ret = (ret << 4) | buff;
	}

	return ret;
}

#endif
#ifdef PS3MAPI

static void Hex2Bin(const char* src, char* target)
{
	char value[3]; value[2] = NULL;
	while(*src && src[1])
	{
		value[0] = src[0], value[1] = src[1];
		*(target++) = (u8)convertH(value);
		src += 2;
	}
}

#endif
