#define SC_PEEK_LV2						(6)
#define SC_POKE_LV2						(7)
#define SC_PEEK_LV1 					(8)
#define SC_POKE_LV1 					(9)
#define SC_PEEK_LV1_COBRA				(11)

#define PS3MAPI_OPCODE_LV2_PEEK			0x1006
#define PS3MAPI_OPCODE_LV2_POKE			0x1007
#define PS3MAPI_OPCODE_LV1_PEEK			0x1008
#define PS3MAPI_OPCODE_LV1_POKE			0x1009

#define SYSCALL8_OPCODE_PS3MAPI			0x7777
#define PS3MAPI_OPCODE_LV1_POKE			0x1009

#define CFW_SYSCALLS_REMOVED(a)			((lv2_peek_hen(a) & 0xFFFFFFFFFF000000ULL) != BASE_MEMORY)

/////////////////// LV1 PEEK //////////////////////
static u64 lv1_peek_cfw(u64 addr)
{
	system_call_1(SC_PEEK_LV1, addr);
	return (u64) p1;
}

#ifdef OVERCLOCKING
static u64 lv1_peek_cobra(u64 addr)
{
	system_call_1(SC_PEEK_LV1_COBRA, addr);
	return (u64) p1;
}
#endif

#ifdef COBRA_ONLY
/*
static process_id_t vsh_pid = 0;
static void get_vsh_pid(void)
{
	#define MAX_PROCESS 16
	char name[25];
	u32 tmp_pid_list[MAX_PROCESS];
	system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (u64)(u32)tmp_pid_list);
	for (int i = 0; i < MAX_PROCESS; i++)
	{
		system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID, tmp_pid_list[i], (u64)(u32)name);
		if (strstr(name, "vsh"))
		{
			vsh_pid = tmp_pid_list[i];
			break;
		}
	}
}

static void poke_vsh(u64 address, char *buf, int size)
{
	if(!vsh_pid) get_vsh_pid();
	if (vsh_pid)
		system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PROC_MEM, vsh_pid, address, (u64)(u32)buf, size);
}
*/
static u64 lv1_peek_ps3mapi(u64 addr)
{
	system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LV1_PEEK, addr);
	return (u64) p1;
}

/////////////////// LV1 POKE //////////////////////
static void lv1_poke_ps3mapi(u64 addr, u64 value)
{
	system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LV1_POKE, addr, value);
}
#endif //#ifdef COBRA_ONLY

static void lv1_poke_cfw( u64 addr, u64 value)
{
	system_call_2(SC_POKE_LV1, addr, value);
}
/////////////////// LV2 PEEK //////////////////////
static u64 lv2_peek_cfw(u64 addr) //sc8 + LV2_OFFSET_ON_LV1
{
	system_call_1(SC_PEEK_LV1, addr + LV2_OFFSET_ON_LV1); //old: {system_call_1(SC_PEEK_LV2, addr);}
	return (u64) p1;
}

static u64 lv2_peek_hen(u64 addr) //sc6
{
	system_call_1(SC_PEEK_LV2, addr);
	return (u64) p1;
}

#ifdef COBRA_ONLY
static u64 lv2_peek_ps3mapi(u64 addr) //sc8 + ps3mapi
{
	if (is_qcfw) addr &= 0x7FFFFFFFFFFFFFFFULL; // fix by W0lf4llo: qCFW's PS3MAPI is adding 0x8000000000000000ULL

	system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LV2_PEEK, addr);
	return (u64) p1;
}

/////////////////// LV2 POKE //////////////////////
static void lv2_poke_hen(u64 addr, u64 value) //sc7
{
	system_call_2(SC_POKE_LV2, addr, value);
}

static void lv2_poke_ps3mapi(u64 addr, u64 value) //sc8 + ps3mapi
{
	if (is_qcfw) addr &= 0x7FFFFFFFFFFFFFFFULL; // fix by W0lf4llo: qCFW's PS3MAPI is adding 0x8000000000000000ULL

	system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LV2_POKE, addr, value);
}
#endif //#ifdef COBRA_ONLY

static void lv2_poke_cfw(u64 addr, u64 value) //sc8 + LV2_OFFSET_ON_LV1
{
	system_call_2(SC_POKE_LV1, addr + LV2_OFFSET_ON_LV1, value);
}
///////////////////////////////////////////////////

static void (*lv2_poke_fan)(u64, u64) = lv2_poke_cfw;  // ps3hen: lv2_poke_fan = lv2_poke_fan_hen;

static u64  (*peekq)(u64) = lv2_peek_cfw;
static void (*pokeq)(u64, u64) = lv2_poke_cfw;

static u64  (*peek_lv1)(u64) = lv1_peek_cfw;
static void (*poke_lv1)(u64, u64) = lv1_poke_cfw;

static u64 peek(u64 addr)
{
	return peekq(addr | BASE_MEMORY);
}

static bool is_ingame_first_15_seconds(void)
{
	if(payload_ps3hen && IS_INGAME)
	{
		CellRtcTick pTick; cellRtcGetCurrentTick(&pTick); if(gTick.tick == rTick.tick) gTick.tick = pTick.tick;
		if(pTick.tick < gTick.tick + 15000000) return true; // do not poke within the first 15 seconds ingame
	}
	return false;
}

#ifdef COBRA_ONLY

///////////////// LV1/LV2 POKE HEN ////////////////
static void lv2_poke_fan_hen(u64 addr, u64 value)
{
	if(is_ingame_first_15_seconds()) return; // do not poke within the first 15 seconds ingame

	system_call_2(SC_POKE_LV2, addr, value); //{system_call_3(SC_COBRA_SYSCALL8, 0x7003ULL, addr, value);} // advanced poke (requires restore original value)
}

static void lv1_poke_hen(u64 addr, u64 value)
{
	if(addr >= LV2_OFFSET_ON_LV1)
		pokeq((addr - LV2_OFFSET_ON_LV1) | BASE_MEMORY, value);
	else
		poke_lv1(addr, value);
}

static u64 lv1_peek_hen(u64 addr)
{
	if(addr >= LV2_OFFSET_ON_LV1)
		return peek(addr - LV2_OFFSET_ON_LV1);
	else
		return peek_lv1(addr);
}
///////////////////////////////////////////////////
#endif

#ifndef LITE_EDITION
/***********************************************************************
* lv2 peek 32 bit
***********************************************************************/
static u32 lv2_peek_32(u64 addr)
{
	return (u32)(peekq(addr) >>32);
}

/***********************************************************************
* lv2 poke 32 bit
***********************************************************************/
static void lv2_poke_32(u64 addr, u32 value)
{
	u64 value_org = peekq(addr);
	pokeq(addr, (value_org & 0xFFFFFFFFULL) | (((u64)value) <<32));
}
#endif

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
	if(c_firmware>=4.55f /*&& c_firmware<=LATEST_CFW*/)
	{
		HV_START_OFFSET = HV_START_OFFSET_455; // same for 4.55-4.92
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

	if(c_firmware>=4.30f /*&& c_firmware<=LATEST_CFW*/)
	{
		#define INSTALL_PEEK_POKE_OFFSET	0x800000000000171CULL

		// add lv2 peek/poke + lv1 peek/poke
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x00, 0x7C0802A6F8010010ULL);
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x08, 0x396000B644000022ULL);
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x10, 0x7C832378E8010010ULL);
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x18, 0x7C0803A64E800020ULL);
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x20, 0x7C0802A6F8010010ULL);
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x28, 0x396000B744000022ULL);
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x30, 0x38600000E8010010ULL);
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x38, 0x7C0803A64E800020ULL);
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x40, 0x7C0802A6F8010010ULL);
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x48, 0x7D4B537844000022ULL);
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x50, 0xE80100107C0803A6ULL);
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x58, 0x4E80002080000000ULL); // sc6  @ 0x8000000000001778 = 800000000000170C
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x60, 0x0000170C80000000ULL); // sc7  @ 0x8000000000001780 = 8000000000001714
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x68, 0x0000171480000000ULL); // sc8  @ 0x8000000000001788 = 800000000000171C
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x70, 0x0000171C80000000ULL); // sc9  @ 0x8000000000001790 = 800000000000173C
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x78, 0x0000173C80000000ULL); // sc10 @ 0x8000000000001798 = 800000000000175C
		pokeq(INSTALL_PEEK_POKE_OFFSET + 0x80, 0x0000175C00000000ULL);

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

		strcpy(file_to_map[max_mapped].src, path1);
		strcpy(file_to_map[max_mapped].dst, path2);
		max_mapped++;
	}
}

static u16 string_to_lv2(const char *path, u64 addr)
{
	u16 len = MIN(strlen(path), MAX_PATH_MAP - 1);

	u8 data[MAX_PATH_MAP];
	u64 *data2 = (u64 *)data;
	_memset(data, MAX_PATH_MAP);
	memcpy(data, path, len);

	len = (len + 7) >> 3;
	for(u8 n = 0; n < (MAX_PATH_MAP / 8); n++, addr += 8)
	{
		pokeq(addr, data2[n]);
	}
	return len * 8;
}
#endif //#ifndef COBRA_ONLY
