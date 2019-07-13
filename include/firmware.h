#define LATEST_CFW	4.84f

#ifndef COBRA_ONLY
static u64 base_addr = 0;
static u64 open_hook = 0;

static u64 sc_600 = 0;
static u64 sc_604 = 0;
static u64 sc_142 = 0;
#endif

#define CEX		0x4345580000000000ULL
#define DEX		0x4445580000000000ULL
#define DEH		0x4445480000000000ULL

#define IS_CEX (dex_mode == 0)
#define IS_DEH (dex_mode == 1)
#define IS_DEX (dex_mode == 2)

// ofw build date
#define FW466	0x323031342F31312FULL // 2014/11/
#define FW475	0x323031352F30342FULL // 2015/04/
#define FW476	0x323031352F30382FULL // 2015/08/
#define FW478	0x323031352F31322FULL // 2015/12/
#define FW480	0x323031362F30342FULL // 2016/04/
#define FW481	0x323031362F31302FULL // 2016/10/
#define FW482	0x323031372F30382FULL // 2017/08/
#define FH482	0x323031372F30392FULL // 2017/09/
#define FW483	0x323031382F30392FULL // 2018/09/
#define FW484	0x323031392F30312FULL // 2019/01/

#define SC_GET_PLATFORM_INFO		(387)
#define SC_GET_CONSOLE_TYPE			(985)

#define SYSCALL8_OPCODE_HEN_REV		0x1339

//#define IS_HEN (lv2_peek_hen(0x8000000000003d90ULL)!=0x386000014e800020ULL)

struct platform_info {
	u32 firmware_version;
} info;

static inline int lv2_get_platform_info(struct platform_info *info)
{
	system_call_1(SC_GET_PLATFORM_INFO, (u32) info);
	return (s32)p1;
}

static float get_firmware_version(void)
{
	lv2_get_platform_info(&info);
	char FW[8]; sprintf(FW, "%02X", info.firmware_version);
	return (float)(FW[0] & 0x0F) + val(FW + 2)*0.00001f;
}

static inline int get_kernel_type(void)
{
	u64 type;
	system_call_1(SC_GET_CONSOLE_TYPE, (u32)&type);
	return (int)(type - 1);
}

static inline u64 find_syscall_table(void)
{
	#ifdef LAST_FIRMWARE_ONLY
	for(u64 addr = dex_mode ? SYSCALL_TABLE_421D : SYSCALL_TABLE_421; addr < 0x8000000000400000ULL; addr += 4)
	#else
	for(u64 addr = dex_mode ? SYSCALL_TABLE_470D : SYSCALL_TABLE_470; addr < 0x8000000000400000ULL; addr += 4)
	#endif
	{
		if(peekq(addr) == 0x3235352E3235352EULL) return (addr + (dex_mode ? 0x1228 : 0x1220));
	}
	return 0;
}

static void detect_firmware(void)
{
	if((c_firmware > 3.40f) || SYSCALL_TABLE || syscalls_removed) return;

	dex_mode = 0;

	// detect ps3hen payload
	payload_ps3hen = (lv1_peek_cfw(0x1337) == 0x1337); // <= {system_call_1(SC_COBRA_SYSCALL8, 0x1337); payload_ps3hen = (p1 == 0x1337);}
	if(payload_ps3hen)
	{
		peekq = lv2_peek_hen;
		pokeq = lv2_poke_hen;
		lv2_poke_fan = lv2_poke_fan_hen;
	}

	// get payload type & cobra_version
	sys_get_cobra_version();

	// detect firmware version & dex_mode using known offsets (in case of spoofed version)
	for(u8 lv2_offset = 1; lv2_offset < 0x10; lv2_offset++)
	{
		if(SYSCALL_TABLE) break;
		LV2_OFFSET_ON_LV1 = (u64)lv2_offset * 0x1000000ULL;

		if(peek(0x2ED818) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_482;  c_firmware = (peek(0x2FCB68) == FW484) ? 4.84f :
																					(peek(0x2FCB68) == FW483) ? 4.83f :
																					(peek(0x2FCB68) == FW482) ? 4.82f :
																					(peek(0x2FCB68) == FW481) ? 4.81f :
#ifndef LAST_FIRMWARE_ONLY
																					(peek(0x2FCB68) == FW478) ? 4.78f :
																					(peek(0x2FCB68) == FW476) ? 4.76f :
																					(peek(0x2FCB68) == FW475) ? 4.75f :
#endif
																					get_firmware_version();} else
#ifndef LAST_FIRMWARE_ONLY
		if(peek(0x2ED808) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_480;  c_firmware = 4.80f;}	else
		if(peek(0x2ED778) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_470;  c_firmware = 4.70f;}	else
		if(peek(0x2ED860) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_465;  c_firmware = (peek(0x2FC938) == FW466) ? 4.66f : 4.65f;} else
		if(peek(0x2ED850) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_460;  c_firmware = 4.60f;}	else
		if(peek(0x2EC5E0) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_455;  c_firmware = 4.55f;}	else
		if(peek(0x2E9D70) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_453;  c_firmware = 4.53f;}	else
		if(peek(0x2E9BE0) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_450;  c_firmware = 4.50f;}	else
		if(peek(0x2EA9B8) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_446;  c_firmware = 4.46f;}	else
		if(peek(0x2E8610) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_421;  c_firmware = 4.21f;}	else
		if(peek(0x2D83D0) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_355;  c_firmware = 3.55f;}	else

		//No cobra cfw but as mamba compatibility
		if(peek(0x2EA498) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_441;  c_firmware = 4.41f;}	else
		if(peek(0x2EA488) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_440;  c_firmware = 4.40f;}	else
		if(peek(0x2E9F18) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_431;  c_firmware = 4.31f;}	else
		if(peek(0x2E9F08) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_430;  c_firmware = 4.30f;}	else
#endif  // #ifndef LAST_FIRMWARE_ONLY

#ifdef DEX_SUPPORT
		if(peek(0x30F3B0) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_481D; c_firmware = (peek(0x31F028) == FW484) ? 4.84f :
																					(peek(0x31F028) == FW482) ? 4.82f :
																					(peek(0x31F028) == FW481) ? 4.81f :
																					get_firmware_version(); dex_mode = 2;} else
 #ifndef LAST_FIRMWARE_ONLY
		if(peek(0x30F3A0) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_480D; c_firmware = 4.80f; dex_mode = 2;}	else
		if(peek(0x30F2D0) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_475D; c_firmware = (peek(0x31EF48) == FW478) ? 4.78f :
																					(peek(0x31EF48) == FW476) ? 4.76f : 4.75f; dex_mode = 2;} else
		if(peek(0x30F240) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_470D; c_firmware = 4.70f; dex_mode = 2;}	else
		if(peek(0x30F1A8) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_465D; c_firmware = (peek(0x31EBA8) == FW466) ? 4.66f : 4.65f; dex_mode = 2;} else
		if(peek(0x30F198) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_460D; c_firmware = 4.60f; dex_mode = 2;}	else
		if(peek(0x30D6A8) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_455D; c_firmware = 4.55f; dex_mode = 2;}	else
		if(peek(0x30AEA8) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_453D; c_firmware = 4.53f; dex_mode = 2;}	else
		if(peek(0x309698) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_450D; c_firmware = 4.50f; dex_mode = 2;}	else
		if(peek(0x305410) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_446D; c_firmware = 4.46f; dex_mode = 2;}	else
		if(peek(0x304EF0) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_441D; c_firmware = 4.41f; dex_mode = 2;}	else
		if(peek(0x304EE0) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_440D; c_firmware = 4.40f; dex_mode = 2;}	else
		if(peek(0x304640) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_431D; c_firmware = 4.31f; dex_mode = 2;}	else
		if(peek(0x304630) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_430D; c_firmware = 4.30f; dex_mode = 2;}	else
		if(peek(0x302D88) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_421D; c_firmware = 4.21f; dex_mode = 2;}	else
		if(peek(0x2EFE20) == DEX) {SYSCALL_TABLE = SYSCALL_TABLE_355D; c_firmware = 3.55f; dex_mode = 2;}	else
 #endif // #ifndef LAST_FIRMWARE_ONLY
#endif

#ifdef DECR_SUPPORT
 #ifndef LAST_FIRMWARE_ONLY
		if(peek(0x319F78) == DEH) {SYSCALL_TABLE = SYSCALL_TABLE_355H; c_firmware = 3.55f; dex_mode = 1;}	else
	  //if(peek(0x32B270) == DEH) {SYSCALL_TABLE = SYSCALL_TABLE_450H; c_firmware = 4.50f; dex_mode = 1;}	else
		if(peek(0x32EDC8) == DEH) {SYSCALL_TABLE = SYSCALL_TABLE_460H; c_firmware = 4.60f; dex_mode = 1;}	else
 #endif // #ifndef LAST_FIRMWARE_ONLY
		if(peek(0x32EB60) == DEH) {SYSCALL_TABLE = SYSCALL_TABLE_475H; c_firmware = (peek(0x344B70) == FH482) ? 4.82f :
																					(peek(0x344B70) == FW481) ? 4.81f :
																					(peek(0x344B70) == FW480) ? 4.80f :
 #ifndef LAST_FIRMWARE_ONLY
																					(peek(0x344B70) == FW478) ? 4.78f :
																					(peek(0x344B70) == FW476) ? 4.76f :
																					(peek(0x344B70) == FW475) ? 4.75f :
 #endif
																					get_firmware_version(); dex_mode = 1;}	else
#endif

#ifndef LAST_FIRMWARE_ONLY
 #ifndef COBRA_ONLY
		if(peek(0x2CFF98) == CEX) {SYSCALL_TABLE = SYSCALL_TABLE_341;  c_firmware = 3.41f;} else
		//if(peek(0x2E79C8) == DEX) {c_firmware = 3.41f; dex_mode = 2;}	else
 #endif
#endif // #ifndef LAST_FIRMWARE_ONLY

		if(SYSCALL_TABLE) break;
	}

	// alternative fw detection method if LV2 peek (SC6) is disabled
	if(!SYSCALL_TABLE)
	{
		c_firmware = get_firmware_version(); dex_mode = get_kernel_type();

		if(IS_CEX)
		{
			/*
			if(c_firmware >= 4.75f && c_firmware <= LATEST_CFW) SYSCALL_TABLE = SYSCALL_TABLE_480; // same for 4.75, 4.76, 4.78, 4.80, 4.81, 4.82, 4.83, 4.84
			#ifndef LAST_FIRMWARE_ONLY
			if(c_firmware == 4.70f) SYSCALL_TABLE = SYSCALL_TABLE_470;
			if(c_firmware >= 4.60f && c_firmware <= 4.66f) SYSCALL_TABLE = SYSCALL_TABLE_460; // same for 4.60, 4.65, 4.66
			if(c_firmware == 4.55f) SYSCALL_TABLE = SYSCALL_TABLE_455;
			if(c_firmware == 4.53f) SYSCALL_TABLE = SYSCALL_TABLE_453;
			if(c_firmware == 4.50f) SYSCALL_TABLE = SYSCALL_TABLE_450;
			if(c_firmware == 4.46f) SYSCALL_TABLE = SYSCALL_TABLE_446;
			if(c_firmware >= 4.40f && c_firmware <= 4.41f) SYSCALL_TABLE = SYSCALL_TABLE_440; // same for 4.40, 4.41
			if(c_firmware >= 4.30f && c_firmware <= 4.31f) SYSCALL_TABLE = SYSCALL_TABLE_430; // same for 4.30, 4.31
			if(c_firmware == 4.21f) SYSCALL_TABLE = SYSCALL_TABLE_421;
			if(c_firmware == 3.55f) SYSCALL_TABLE = SYSCALL_TABLE_355;
			#endif
			*/
			LV2_OFFSET_ON_LV1 = 0x01000000ULL;
		}
		else if(IS_DEX)
		{
			/*
			if(c_firmware >= 4.80f) SYSCALL_TABLE = SYSCALL_TABLE_480D; // same for 4.80-4.81
			#ifndef LAST_FIRMWARE_ONLY
			if(c_firmware >= 4.75f && c_firmware <= 4.78f) SYSCALL_TABLE = SYSCALL_TABLE_475D; // same for 4.75, 4.76, 4.78
			if(c_firmware == 4.70f) SYSCALL_TABLE = SYSCALL_TABLE_470D;
			if(c_firmware >= 4.60f && c_firmware <= 4.66f) SYSCALL_TABLE = SYSCALL_TABLE_460D; // same for 4.60, 4.65, 4.66
			if(c_firmware == 4.55f) SYSCALL_TABLE = SYSCALL_TABLE_455D;
			if(c_firmware == 4.53f) SYSCALL_TABLE = SYSCALL_TABLE_453D;
			if(c_firmware == 4.50f) SYSCALL_TABLE = SYSCALL_TABLE_450D;
			if(c_firmware == 4.46f) SYSCALL_TABLE = SYSCALL_TABLE_446D;
			if(c_firmware >= 4.40f && c_firmware <= 4.41f) SYSCALL_TABLE = SYSCALL_TABLE_440D; // same for 4.40, 4.41
			if(c_firmware >= 4.30f && c_firmware <= 4.31f) SYSCALL_TABLE = SYSCALL_TABLE_430D; // same for 4.30, 4.31
			if(c_firmware == 4.21f) SYSCALL_TABLE = SYSCALL_TABLE_421D;
			if(c_firmware == 3.55f) SYSCALL_TABLE = SYSCALL_TABLE_355D;
			#endif
			*/
			LV2_OFFSET_ON_LV1 = 0x08000000ULL;
		}

		if(dex_mode < 2) SYSCALL_TABLE = find_syscall_table();
	}

	if(!SYSCALL_TABLE) {c_firmware = 0.00f; return;}

	sprintf(fw_version, "%i.%02i", (int)c_firmware, ((u32)(c_firmware * 1000.0f) % 1000) / 10);

#ifndef COBRA_ONLY
	if(IS_CEX)
	{   // CEX
 #ifndef LAST_FIRMWARE_ONLY
		if(c_firmware == 3.41f) {base_addr = 0x2B5D30, open_hook = 0x2AAFC8;} else
		if(c_firmware == 3.55f) {base_addr = 0x2BE0D0, open_hook = 0x2B3274;} else
		if(c_firmware == 4.21f) {base_addr = 0x2D0C98, open_hook = 0x2C2558;} else
		if(c_firmware == 4.30f) {base_addr = 0x2D2418, open_hook = 0x2C3CD4;} else
		if(c_firmware == 4.31f) {base_addr = 0x2D2428, open_hook = 0x2C3CE0;} else
		if(c_firmware == 4.40f) {base_addr = 0x2D29A8, open_hook = 0x2C4284;} else
		if(c_firmware == 4.41f) {base_addr = 0x2D29B8, open_hook = 0x2C4290;} else
		if(c_firmware == 4.46f) {base_addr = 0x2D2ED8, open_hook = 0x2C47B0;} else
		if(c_firmware == 4.50f) {base_addr = 0x2D4CB8, open_hook = 0x29DD20;} else
		if(c_firmware == 4.53f) {base_addr = 0x2D4E48, open_hook = 0x29DEF8;} else
		if(c_firmware == 4.55f) {base_addr = 0x2D7660, open_hook = 0x29F748;} else
		if(c_firmware == 4.60f) {base_addr = 0x2D88D0, open_hook = 0x2A02BC;} else
		if(c_firmware >= 4.65f && c_firmware <= 4.66f)
								{base_addr = 0x2D88E0, open_hook = 0x2A02C8;} else
		if(c_firmware == 4.70f) {base_addr = 0x2D8A70, open_hook = 0x2975C0;} else
 #endif  // #ifndef LAST_FIRMWARE_ONLY
		if(c_firmware == 4.80f) {base_addr = 0x2D8AE0, open_hook = 0x29762C;} else
		if(c_firmware >= 4.75f /*&& c_firmware <= LATEST_CFW*/)
								{base_addr = 0x2D8AF0, open_hook = 0x297638;}
	}

 #ifdef DECR_SUPPORT
	else if(IS_DEH)
	{   // DECR
	#ifndef LAST_FIRMWARE_ONLY
		if(c_firmware == 3.55f) {base_addr = 0x2F5320, open_hook = 0x2E31F4;} else
	  //if(c_firmware == 4.50f) {base_addr = 0x30D2C0, open_hook = 0x2CEF08;} else
		if(c_firmware == 4.60f) {base_addr = 0x310EE0, open_hook = 0x2D1464;} else
	#endif  // #ifndef LAST_FIRMWARE_ONLY
		if(c_firmware == 4.80f) {base_addr = 0x3110F0, open_hook = 0x2C87D4;} else
		if(c_firmware >= 4.75f /*&& c_firmware <= LATEST_CFW*/)
								{base_addr = 0x3110F0, open_hook = 0x2C87E0;}
	}
 #endif //#ifdef DECR_SUPPORT

 #ifdef DEX_SUPPORT
	else if(IS_DEX)
	{   // DEX
  #ifndef LAST_FIRMWARE_ONLY
		if(c_firmware == 3.55f) {base_addr = 0x2D5B20, open_hook = 0x2C8A94;} else
		if(c_firmware == 4.21f) {base_addr = 0x2EB418, open_hook = 0x2D9718;} else
		if(c_firmware == 4.30f) {base_addr = 0x2ECB48, open_hook = 0x2DAE4C;} else
		if(c_firmware == 4.31f) {base_addr = 0x2ECB58, open_hook = 0x2DAE58;} else
		if(c_firmware == 4.40f) {base_addr = 0x2ED408, open_hook = 0x2DB730;} else
		if(c_firmware == 4.41f) {base_addr = 0x2ED418, open_hook = 0x2DB73C;} else
		if(c_firmware == 4.46f) {base_addr = 0x2ED938, open_hook = 0x2DBC5C;} else
		if(c_firmware == 4.50f) {base_addr = 0x2F4778, open_hook = 0x2B81E8;} else
		if(c_firmware == 4.53f) {base_addr = 0x2F5F88, open_hook = 0x2B83C0;} else
		if(c_firmware == 4.55f) {base_addr = 0x2F8730, open_hook = 0x2B9C14;} else
		if(c_firmware == 4.60f) {base_addr = 0x2FA220, open_hook = 0x2BB004;} else
		if(c_firmware >= 4.65f && c_firmware <= 4.66f)
								{base_addr = 0x2FA230, open_hook = 0x2BB010;} else
		if(c_firmware == 4.70f) {base_addr = 0x2FA540, open_hook = 0x2B2480;} else
		if(c_firmware >= 4.75f && c_firmware <= 4.78f)
								{base_addr = 0x2FA5B0, open_hook = 0x2B24F8;} else
  #endif // #ifndef LAST_FIRMWARE_ONLY
		if(c_firmware == 4.80f) {base_addr = 0x2FA680, open_hook = 0x2B25C4;} else
		if(c_firmware >= 4.81f /*&& c_firmware <= LATEST_CFW*/)
								{base_addr = 0x2FA690, open_hook = 0x2B25D0;}
	}
 #endif //#ifdef DEX_SUPPORT

	base_addr |= 0x8000000000000000ULL;
	open_hook |= 0x8000000000000000ULL;
#endif //#ifndef COBRA_ONLY

	if(IS_CEX)
	{ // CEX
		if(c_firmware >= 4.55f /*&& c_firmware <= LATEST_CFW*/)
		{
			get_fan_policy_offset = 0x8000000000009E38ULL; // sys 409 get_fan_policy  4.55/4.60/4.65/4.70/4.75/4.76/4.78/4.80/4.81/4.82/4.83/4.84
			set_fan_policy_offset = 0x800000000000A334ULL; // sys 389 set_fan_policy

#ifdef SPOOF_CONSOLEID
			// idps / psid cex
 #ifndef LAST_FIRMWARE_ONLY
			if(c_firmware == 4.55f)
			{
				idps_offset1 = 0x80000000003E17B0ULL;
				idps_offset2 = 0x8000000000474F1CULL;
			}
			else if(c_firmware >= 4.60f && c_firmware <= 4.66f)
			{
				idps_offset1 = 0x80000000003E2BB0ULL;
				idps_offset2 = 0x8000000000474F1CULL;
			}
			else if(c_firmware == 4.70f)
			{
				idps_offset1 = 0x80000000003E2DB0ULL;
				idps_offset2 = 0x8000000000474AF4ULL;
			}
			else
 #endif //#ifndef LAST_FIRMWARE_ONLY
			if(c_firmware >= 4.75f /*&& c_firmware <= LATEST_CFW*/)
			{
				idps_offset1 = 0x80000000003E2E30ULL;
				idps_offset2 = 0x8000000000474AF4ULL;
			}
#endif //#ifdef SPOOF_CONSOLEID
		}
#ifndef LAST_FIRMWARE_ONLY
		else if(c_firmware >= 4.21f && c_firmware <= 4.53f)
		{
			get_fan_policy_offset = 0x8000000000009E28ULL; // sys 409 get_fan_policy  4.21/4.30/4.31/4.40/4.41/4.46/4.50/4.53
			set_fan_policy_offset = 0x800000000000A324ULL; // sys 389 set_fan_policy
		}
		else if(c_firmware == 3.55f)
		{
			get_fan_policy_offset = 0x8000000000008CBCULL; // sys 409 get_fan_policy
			set_fan_policy_offset = 0x80000000000091B8ULL; // sys 389 set_fan_policy
		}
		#ifndef COBRA_ONLY
		else if(c_firmware == 3.41f)
		{
			get_fan_policy_offset = 0x8000000000008644ULL; // sys 409 get_fan_policy
			set_fan_policy_offset = 0x8000000000008B40ULL; // sys 389 set_fan_policy
		}
		#endif
#endif //#ifndef LAST_FIRMWARE_ONLY
	}

#if defined(DEX_SUPPORT) || defined(DECR_SUPPORT)
	else if(dex_mode)
	{ // DEX & DEH
		if(c_firmware >= 4.55f /*&& c_firmware <= LATEST_CFW*/)
		{
				get_fan_policy_offset = 0x8000000000009EB8ULL; // sys 409 get_fan_policy  4.55/4.60/4.65/4.70/4.75/4.76/4.78/4.80/4.81/4.82
				set_fan_policy_offset = 0x800000000000A3B4ULL; // sys 389 set_fan_policy

 #ifdef SPOOF_CONSOLEID
  #ifdef DECR_SUPPORT
			// idps / psid dex
			if(IS_DEH)
			{ // DEH
		#ifndef LAST_FIRMWARE_ONLY
				if(c_firmware >= 4.60f && c_firmware <= 4.66f)
				{
					idps_offset1 = 0x8000000000432430ULL;
					idps_offset2 = 0x80000000004C4F1CULL;
				}
				else
		#endif
				if(c_firmware >= 4.75f /*&& c_firmware <= LATEST_CFW*/)
				{
					idps_offset1 = 0x80000000004326B0ULL;
					idps_offset2 = 0x80000000004C4AF4ULL;
				}
			}
			else
  #endif //#ifdef DECR_SUPPORT
  #ifndef LAST_FIRMWARE_ONLY
			if(c_firmware == 4.55f)
			{
				idps_offset1 = 0x8000000000407930ULL;
				idps_offset2 = 0x8000000000494F1CULL;
			}
			else if(c_firmware >= 4.60f && c_firmware <= 4.66f)
			{
				idps_offset1 = 0x80000000004095B0ULL;
				idps_offset2 = 0x800000000049CF1CULL;
			}
			else if(c_firmware == 4.70f)
			{
				idps_offset1 = 0x80000000004098B0ULL;
				idps_offset2 = 0x800000000049CAF4ULL;
			}
			else
  #endif //#ifndef LAST_FIRMWARE_ONLY
			if(c_firmware >= 4.75f && c_firmware <= 4.78f)
			{
				idps_offset1 = 0x8000000000409930ULL;
				idps_offset2 = 0x800000000049CAF4ULL;
			}
			else // if(c_firmware >= 4.80f /*&& c_firmware <= LATEST_CFW*/)
			{
				idps_offset1 = 0x8000000000409A30ULL;
				idps_offset2 = 0x800000000049CAF4ULL;
			}
 #endif //#ifdef SPOOF_CONSOLEID
		}

 #ifndef LAST_FIRMWARE_ONLY
		else if(c_firmware >= 4.21f && c_firmware <= 4.53f)
		{
				get_fan_policy_offset = 0x8000000000009EA8ULL; // sys 409 get_fan_policy  4.21/4.30/4.31/4.40/4.41/4.46/4.50/4.53
				set_fan_policy_offset = 0x800000000000A3A4ULL; // sys 389 set_fan_policy
		}
		else if(c_firmware == 3.55f)
		{
				get_fan_policy_offset = 0x8000000000008D3CULL; // sys 409 get_fan_policy
				set_fan_policy_offset = 0x8000000000009238ULL; // sys 389 set_fan_policy
		}
 #endif

	}
#endif //#if defined(DEX_SUPPORT) || defined(DECR_SUPPORT)

#ifdef SPOOF_CONSOLEID
	if(idps_offset2) psid_offset = idps_offset2 + 0x18ULL;
#endif
#ifndef COBRA_ONLY
	install_peek_poke();
#endif
}

static void apply_lv2_patches(u64 addr_3C, u64 addr_3D, u64 addr_3E,
						 u64 addr_jump, u64 addr_09)
{
	//patches by deank
	pokeq(addr_3C    , 0x4E80002038600000ULL ); // fix 8001003C error  Original: 0x4E8000208003026CULL
	pokeq(addr_3C + 8, 0x7C6307B44E800020ULL ); // fix 8001003C error  Original: 0x3D201B433C608001ULL
	pokeq(addr_3D    , 0x63FF003D60000000ULL ); // fix 8001003D error  Original: 0x63FF003D419EFFD4ULL
	pokeq(addr_3E    , 0x3FE080013BE00000ULL ); // fix 8001003E error  Original: 0x3FE0800163FF003EULL

	pokeq(addr_jump    , 0x419E00D860000000ULL ); // Original: 0x419E00D8419D00C0ULL
	pokeq(addr_jump + 8, 0x2F84000448000098ULL ); // Original: 0x2F840004409C0048ULL //PATCH_JUMP
	pokeq(addr_09      , 0x2F83000060000000ULL ); // fix 80010009 error  Original: 0x2F830000419E00ACULL
	pokeq(addr_09 +0X14, 0x2F83000060000000ULL ); // fix 80010009 error  Original: 0x2F830000419E00ACULL
}

static void apply_lv2_patches_new(u64 addr_3C, u64 addr_3D, u64 addr_3E,
							 u64 addr_jump, u64 addr_09,
							 u64 addr_lic, u64 addr_17,
							 u64 addr_restore1, u64 addr_restore2)
{
	//patches by deank
	apply_lv2_patches(addr_3C, addr_3D, addr_3E, addr_jump, addr_09);

	if(addr_lic) pokeq(addr_lic, 0x386000012F830000ULL ); // ignore LIC.DAT check
	if(addr_17 ) pokeq(addr_17 , 0x38600000F8690000ULL ); // fix 0x8001002B / 80010017 errors  Original: 0xE86900007C6307B4

	if(addr_restore1) pokeq(addr_restore1, 0xF821FE917C0802A6ULL ); // just restore the original
	if(addr_restore2) pokeq(addr_restore2, 0x419E0038E8610098ULL ); // just restore the original
}

static void patch_lv2(void)
{
	if(IS_CEX)
	{ // CEX
#ifndef LAST_FIRMWARE_ONLY
		if(c_firmware == 3.55f)
		{
			apply_lv2_patches(0x80000000002909B0ULL, 0x8000000000055EA0ULL, 0x8000000000055F64ULL,
							  0x8000000000055F10ULL, 0x800000000007AF64ULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x325B50;
			sc_604 = 0x325C58;
			sc_142 = 0x2E8FF8;
			#endif
		}
		else
		if(c_firmware == 4.21f)
		{
			apply_lv2_patches(0x8000000000296264ULL, 0x8000000000057020ULL, 0x80000000000570E4ULL,
							  0x8000000000057090ULL, 0x800000000005AA54ULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x33B2E0;
			sc_604 = 0x33B448;
			sc_142 = 0x2FD810;
			#endif
		}
		else
		if(c_firmware == 4.30f)
		{
			pokeq(0x8000000000057238ULL, 0x3BE000004BFFFF0CULL ); // bug introduced by deank

			apply_lv2_patches(0x80000000002979D8ULL, 0x8000000000057170ULL, 0x8000000000057234ULL,
							  0x80000000000571E0ULL, 0x800000000005ABA4ULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x33D158; //35EEA0
			sc_604 = 0x33D2C0; //35EEC0
			sc_142 = 0x2FF460; //35E050
			#endif
		}
		else
		if(c_firmware == 4.31f)
		{
			apply_lv2_patches(0x80000000002979E0ULL, 0x8000000000057174ULL, 0x800000000005723CULL,
							  0x80000000000571E8ULL, 0x800000000005ABACULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x33D168;
			sc_604 = 0x33D2D0;
			sc_142 = 0x2FF470;
			#endif
		}
		else
		if(c_firmware == 4.40f)
		{
			apply_lv2_patches(0x8000000000296DE8ULL, 0x80000000000560BCULL, 0x8000000000056180ULL,
							  0x800000000005612CULL, 0x8000000000059AF0ULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x33D720;
			sc_604 = 0x33D888;
			sc_142 = 0x2FF9E0;
			#endif
		}
		else
		if(c_firmware == 4.41f)
		{
			apply_lv2_patches(0x8000000000296DF0ULL, 0x80000000000560C0ULL, 0x8000000000056184ULL,
							  0x8000000000056130ULL, 0x8000000000059AF4ULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x33D730;
			sc_604 = 0x33D898;
			sc_142 = 0x2FF9F0;
			#endif
		}
		else
		if(c_firmware == 4.46f)
		{
			apply_lv2_patches(0x8000000000297310ULL, 0x80000000000560C0ULL, 0x8000000000056184ULL,
							  0x8000000000056130ULL, 0x8000000000059AF4ULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x33DD40;
			sc_604 = 0x33DEA8;
			sc_142 = 0x2FFF58;
			#endif
		}
		else
		if(c_firmware == 4.50f)
		{
			apply_lv2_patches(0x800000000026F61CULL, 0x80000000000560BCULL, 0x8000000000056180ULL,
							  0x800000000005612CULL, 0x8000000000059AF0ULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x33C180;
			sc_604 = 0x33C2E8;
			sc_142 = 0x302100;
			#endif
		}
		else
		if(c_firmware == 4.53f)
		{
			apply_lv2_patches(0x800000000026F7F0ULL, 0x80000000000560C0ULL, 0x8000000000056184ULL,
							  0x8000000000056130ULL, 0x8000000000059AF4ULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x33C308;
			sc_604 = 0x33C470;
			sc_142 = 0x302108;
			#endif
		}
		else
		if(c_firmware == 4.55f)
		{
			apply_lv2_patches(0x800000000027103CULL, 0x8000000000056380ULL, 0x8000000000056444ULL,
							  0x80000000000563F0ULL, 0x800000000005A2ECULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x33F5C8;
			sc_604 = 0x33F730;
			sc_142 = 0x3051D0;
			#endif
		}
		else
		if(c_firmware == 4.60f)
		{
			apply_lv2_patches_new(0x800000000026FDD4ULL, 0x8000000000056588ULL, 0x800000000005664CULL,
								  0x80000000000565F8ULL, 0x800000000005A654ULL,
								  0x800000000005622CULL, 0x80000000002302E8ULL,
								  0x8000000000055C58ULL, 0x8000000000058DACULL);

			//apply_lv2_patches(0x80000000002925D8ULL, 0x8000000000056588ULL, 0x800000000005664CULL,
			//			 0x80000000000565F8ULL, 0x800000000005A654ULL,
			//			 0, 0, 0, 0);
			/*
			pokeq(0x80000000002925D8ULL, 0x4E80002038600000ULL ); // fix 8001003C error
			pokeq(0x80000000002925E0ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error

			pokeq(0x80000000002A1054ULL, 0x386000014E800020ULL); // fix 0x80010017 error   Original: 0xFBC1FFF0EBC225B0ULL
			pokeq(0x8000000000055C58ULL, 0x386000004E800020ULL); // fix 0x8001002B error   Original: 0xF821FE917C0802A6ULL

			//lv2poke32(0x8000000000058DACULL, 0x60000000);      // fix 0x80010017 error (found by @smhabib)

			// Booting of game discs and backups speed increased
			//lv2poke32(0x8000000000058DA0ULL, 0x38600001);
			//lv2poke32(0x800000000005A96CULL, 0x38600000);

			// enable new habib patches
			pokeq(0x8000000000058DACULL + 0x00, 0x60000000E8610098ULL);
			pokeq(0x8000000000058DACULL + 0x08, 0x2FA30000419E000CULL);
			pokeq(0x8000000000058DACULL + 0x10, 0x388000334800BE15ULL);
			pokeq(0x8000000000058DACULL + 0x18, 0xE80100F07FE307B4ULL);

			pokeq(0x8000000000055C5CULL + 0x00, 0x386000004E800020ULL);
			pokeq(0x8000000000055C5CULL + 0x08, 0xFBC10160FBE10168ULL);
			pokeq(0x8000000000055C5CULL + 0x10, 0xFB610148FB810150ULL);
			pokeq(0x8000000000055C5CULL + 0x18, 0xFBA10158F8010180ULL);
			*/
			#ifndef COBRA_ONLY
			sc_600 = 0x340630; //0x363A18 + 600*8 = 00364CD8 -> 80 00 00 00 00 34 06 30
			sc_604 = 0x340798; //0x363A18 + 604*8 = 00364CF8 -> 80 00 00 00 00 34 07 98
			sc_142 = 0x306478; //0x363A18 + 142*8 = 00363E88 -> 80 00 00 00 00 30 64 78
			#endif
		}
		else
		if(c_firmware == 4.65f || c_firmware == 4.66f)
		{
			apply_lv2_patches_new(0x800000000026FDDCULL, 0x800000000005658CULL, 0x8000000000056650ULL,
								  0x80000000000565FCULL, 0x800000000005A658ULL,
								  0x8000000000056230ULL, 0x80000000002302F0ULL,
								  0x8000000000055C5CULL, 0x8000000000058DB0ULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x340640; //0x363A18 + 600*8 = 00364CD8 -> 80 00 00 00 00 34 06 40
			sc_604 = 0x3407A8; //0x363A18 + 604*8 = 00364CF8 -> 80 00 00 00 00 34 07 A8
			sc_142 = 0x306488; //0x363A18 + 142*8 = 00363E88 -> 80 00 00 00 00 30 64 88
			#endif
		}
		else
		if(c_firmware == 4.70f)
		{
			apply_lv2_patches_new(0x80000000002670D8ULL, 0x8000000000056588ULL, 0x800000000005664CULL,
								  0x80000000000565F8ULL, 0x800000000005A658ULL,
								  0x800000000005622CULL, 0x80000000002275ECULL,
								  0x8000000000055C58ULL, 0x8000000000058E18ULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x33FE88;
			sc_604 = 0x33FFF0;
			sc_142 = 0x306618;
			#endif
		}
		else
#endif //#ifndef LAST_FIRMWARE_ONLY
		if(c_firmware == 4.80f)
		{
			apply_lv2_patches_new(0x8000000000267144ULL, 0x800000000005688CULL, 0x800000000005664CULL,
								  0x80000000000565F8ULL, 0x800000000005A6DCULL,
								  0x800000000005622CULL, 0x80000000002275ECULL,
								  0x8000000000055C58ULL, 0x8000000000058E18ULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x33FF18;
			sc_604 = 0x340080;
			sc_142 = 0x3066A8;
			#endif
		}
		else
		if(c_firmware >= 4.75f /*&& c_firmware <= LATEST_CFW*/)
		{
			apply_lv2_patches_new(0x800000000026714CULL, 0x800000000005658CULL, 0x8000000000056650ULL,
								  0x80000000000565FCULL, 0x800000000005A6E0ULL,
								  0x8000000000056230ULL, 0x80000000002275F4ULL,
								  0x8000000000055C5CULL, 0x8000000000058E1CULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x33FF28;
			sc_604 = 0x340090;
			sc_142 = 0x3066B8;
			#endif
		}
	}

#ifdef DECR_SUPPORT
	else if(IS_DEH)
	{ // DEH
 #ifndef LAST_FIRMWARE_ONLY

		if(c_firmware == 3.55f)
		{
			apply_lv2_patches(0x8000000000291794ULL, 0x8000000000059C58ULL, 0x8000000000059D1CULL,
							  0x8000000000059CC8ULL, 0x800000000007F6D0ULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x371138;
			sc_604 = 0x371228;
			sc_142 = 0x32FE88;
			#endif
		}
		else
		/*
		if(c_firmware == 4.50f)
		{
			apply_lv2_patches(0x8000000000276A00ULL, 0x8000000000059E74ULL, 0x8000000000059F38ULL,
						 0x8000000000059EE4ULL, 0x800000000005D8A8ULL,
						 0x8000000000059B40ULL, 0x800000000023705CULL, 0, 0);
			#ifndef COBRA_ONLY
			sc_600 = 0x360208;
			sc_604 = 0x388E10;
			sc_142 = 0x34CA38;
			#endif
		}
		else
		*/
		if(c_firmware == 4.60f)
		{
			apply_lv2_patches_new(0x80000000002771B8ULL, 0x800000000005A340ULL, 0x800000000005A404ULL,
								  0x800000000005A3B0ULL, 0x800000000005E408ULL,
								  0x8000000000059FE4ULL, 0x800000000023740CULL, 0, 0);
			#ifndef COBRA_ONLY
			sc_600 = 0x364628;
			sc_604 = 0x38D2C0;
			sc_142 = 0x350ED0;
			#endif
		}
		else
 #endif
		if(c_firmware == 4.80f)
		{
			apply_lv2_patches_new(0x800000000026E528ULL, 0x800000000005A340ULL, 0x800000000005A020ULL,
								  0x800000000005A3B0ULL, 0x800000000005E494ULL,
								  0x8000000000059FE4ULL, 0x800000000022E710ULL, 0, 0);
			#ifndef COBRA_ONLY
			sc_600 = 0x38CB60; //0x3B28F8 + 600*8 = 003B3BB8 -> 80 00 00 00 00 38 CB 60
			sc_604 = 0x38CC50; //0x3B28F8 + 604*8 = 003B3BD8 -> 80 00 00 00 00 38 CC 50
			sc_142 = 0x34ED30; //0x3B28F8 + 142*8 = 003B2D68 -> 80 00 00 00 00 34 ED 30
			#endif
		}
		else
		if(c_firmware >= 4.75f /*&& c_firmware <= LATEST_CFW*/)
		{
			apply_lv2_patches_new(0x800000000026E530ULL, (c_firmware >= 4.81f) ? 0x800000000005A584ULL : 0x800000000005A344ULL, 0x800000000005A408ULL,
								  0x800000000005A3B4ULL, 0x800000000005E498ULL,
								  0x8000000000059FE8ULL, 0x800000000022E718ULL, 0, 0);
			#ifndef COBRA_ONLY
			sc_600 = 0x38CB60; //003B28F8 + 600*8 = 003B3BB8 -> 80 00 00 00 00 38 CB 60
			sc_604 = 0x38CC50; //003B28F8 + 604*8 = 003B3BD8 -> 80 00 00 00 00 38 CC 50
			sc_142 = 0x34ED30; //003B28F8 + 142*8 = 003B2D68 -> 80 00 00 00 00 34 ED 30
			#endif
		}
	}
#endif //#ifdef DECR_SUPPORT

#ifdef DEX_SUPPORT
	else if(IS_DEX)
	{ // DEX
 #ifndef LAST_FIRMWARE_ONLY
		if(c_firmware == 3.55f)
		{
			apply_lv2_patches(0x8000000000298790ULL, 0x800000000005978CULL, 0x8000000000059850ULL,
							  0x80000000000597FCULL, 0x800000000007EF5CULL);
  #ifndef COBRA_ONLY
			sc_600 = 0x33FAC8;
			sc_604 = 0x33FC30;
			sc_142 = 0x3010E0;
  #endif
		}
		else
		if(c_firmware == 4.21f)
		{
			apply_lv2_patches(0x800000000029C8C0ULL, 0x800000000005A938ULL, 0x800000000005A9FCULL,
							  0x800000000005A9A8ULL, 0x800000000005E36CULL);
  #ifndef COBRA_ONLY
			sc_600 = 0x3583F8;
			sc_604 = 0x3584D0;
			sc_142 = 0x318BA0;
  #endif
		}
		else
		if(c_firmware == 4.30f)
		{
			apply_lv2_patches(0x800000000029E034ULL, 0x800000000005AA88ULL, 0x800000000005AB4CULL,
							  0x800000000005AAF8ULL, 0x800000000005E4BCULL);
  #ifndef COBRA_ONLY
			sc_600 = 0x35A220;
			sc_604 = 0x35A2F8;
			sc_142 = 0x31A7A0;
  #endif
		}
		else
		if(c_firmware == 4.31f)
		{
			apply_lv2_patches(0x800000000029E03CULL, 0x800000000005AA8CULL, 0x800000000005AB50ULL,
							  0x800000000005AAFCULL, 0x800000000005E4C0ULL);
  #ifndef COBRA_ONLY
			sc_600 = 0x35A230;
			sc_604 = 0x35A308;
			sc_142 = 0x31A7B0;
  #endif
		}
		else
		if(c_firmware == 4.40f)
		{
			apply_lv2_patches(0x800000000029D444ULL, 0x80000000000599D4ULL, 0x8000000000059A98ULL,
							  0x8000000000059A44ULL, 0x800000000005D408ULL);
  #ifndef COBRA_ONLY
			sc_600 = 0x35AB30;
			sc_604 = 0x35AC08;
			sc_142 = 0x31B050;
  #endif
		}
		else
		if(c_firmware == 4.41f)
		{
			apply_lv2_patches(0x800000000029D44CULL, 0x80000000000599D8ULL, 0x8000000000059A9CULL,
							  0x8000000000059A48ULL, 0x800000000005D40CULL);
  #ifndef COBRA_ONLY
			sc_600 = 0x35AB40;
			sc_604 = 0x35AC18;
			sc_142 = 0x31B060;
  #endif
		}
		else
		if(c_firmware == 4.46f)
		{
			apply_lv2_patches(0x800000000029D96CULL, 0x80000000000599D8ULL, 0x8000000000059A9CULL,
							  0x8000000000059A48ULL, 0x800000000005D40CULL);
  #ifndef COBRA_ONLY
			sc_600 = 0x35B150;
			sc_604 = 0x35B228;
			sc_142 = 0x31B5C8;
  #endif
		}
		else
		if(c_firmware == 4.50f)
		{
			apply_lv2_patches(0x8000000000275D38ULL, 0x8000000000059A8CULL, 0x8000000000059B50ULL,
							  0x8000000000059AFCULL, 0x800000000005D4C0ULL);
  #ifndef COBRA_ONLY
			sc_600 = 0x35EA90;
			sc_604 = 0x35EB68;
			sc_142 = 0x322B38;
  #endif
		}
		else
		if(c_firmware == 4.53f)
		{
			apply_lv2_patches(0x8000000000275F0CULL, 0x8000000000059A90ULL, 0x8000000000059B54ULL,
							  0x8000000000059B00ULL, 0x800000000005D4C4ULL);
  #ifndef COBRA_ONLY
			sc_600 = 0x3602A8; //0x385108 + 600*8 = 003863C8 -> 80 00 00 00 00 36 02 A8
			sc_604 = 0x360380; //0x385108 + 604*8 = 003863E8 -> 80 00 00 00 00 36 03 80
			sc_142 = 0x3242F0; //0x385108 + 142*8 = 00385578 -> 80 00 00 00 00 32 42 F0
  #endif
		}
		else
		if(c_firmware == 4.55f)
		{
			apply_lv2_patches(0x8000000000277758ULL, 0x8000000000059D50ULL, 0x8000000000059E14ULL,
							  0x8000000000059DC0ULL, 0x800000000005DCB8ULL);
  #ifndef COBRA_ONLY
			sc_600 = 0x3634F8; //0x388488 + 600*8 = 00389748 -> 80 00 00 00 00 36 34 F8
			sc_604 = 0x3635D0; //0x388488 + 604*8 = 00389768 -> 80 00 00 00 00 36 35 D0
			sc_142 = 0x327348; //0x388488 + 142*8 = 003888F8 -> 80 00 00 00 00 32 73 48
  #endif
		}
		else
		if(c_firmware == 4.60f)
		{
			apply_lv2_patches_new(0x80000000002764F0ULL, 0x8000000000059F58ULL, 0x800000000005A01CULL,
								  0x8000000000059FC8ULL, 0x800000000005E024ULL,
								  0x8000000000059BFCULL, 0x80000000002367C4ULL,
								  0x8000000000059628ULL, 0x800000000005C77CULL);
  #ifndef COBRA_ONLY
			sc_600 = 0x364DE0; //0x38A120 + 600*8 = 0038B3E0 -> 80 00 00 00 00 36 4D E0
			sc_604 = 0x364EB8; //0x38A120 + 604*8 = 0038B400 -> 80 00 00 00 00 36 4E B8
			sc_142 = 0x328E70; //0x38A120 + 142*8 = 0038A590 -> 80 00 00 00 00 32 8E 70
  #endif
		}
		else
		if(c_firmware == 4.65f || c_firmware == 4.66f)
		{
			apply_lv2_patches_new(0x80000000002764F8ULL, 0x8000000000059F5CULL, 0x800000000005A020ULL,
								  0x8000000000059FCCULL, 0x800000000005E028ULL,
								  0x8000000000059C00ULL, 0x80000000002367CCULL,
								  0x800000000005962CULL, 0x800000000005C780ULL);
  #ifndef COBRA_ONLY
			sc_600 = 0x364DF0; //0x38A120 + 600*8 = 0038B3E0 -> 80 00 00 00 00 36 4D F0
			sc_604 = 0x364EC8; //0x38A120 + 604*8 = 0038B400 -> 80 00 00 00 00 36 4E C8
			sc_142 = 0x328E80; //0x38A120 + 142*8 = 0038A590 -> 80 00 00 00 00 32 8E 80
  #endif
		}
		else
		if(c_firmware == 4.70f)
		{
			apply_lv2_patches_new(0x800000000026D7F4ULL, 0x8000000000059F58ULL, 0x800000000005A01CULL,
								  0x8000000000059FC8ULL, 0x800000000005E0ACULL,
								  0x8000000000059BFCULL, 0x800000000022DAC8ULL,
								  0x8000000000059628ULL, 0x800000000005C7E8ULL);
  #ifndef COBRA_ONLY
			sc_600 = 0x3647B8; //0x38A368 + 600*8 = 0038B628 -> 80 00 00 00 00 36 47 B8
			sc_604 = 0x364890; //0x38A368 + 604*8 = 0038B648 -> 80 00 00 00 00 36 48 90
			sc_142 = 0x329190; //0x38A368 + 142*8 = 0038A7D8 -> 80 00 00 00 00 32 91 90
  #endif
		}
		else
 #endif //#ifndef LAST_FIRMWARE_ONLY
		if(c_firmware == 4.80f)
		{
			apply_lv2_patches_new(0x800000000026D860ULL, 0x8000000000059F58ULL, 0x800000000005A01CULL,
								  0x8000000000059FC8ULL, 0x800000000005E0ACULL,
								  0x8000000000059BFCULL, 0x800000000022DAC8ULL,
								  0x8000000000059628ULL, 0x800000000005C7E8ULL);
			#ifndef COBRA_ONLY
			sc_600 = 0x364918; //0x38A4E8 + 600*8 = 0038B7A8 -> 80 00 00 00 00 36 49 18
			sc_604 = 0x3649F0; //0x38A4E8 + 604*8 = 0038B7C8 -> 80 00 00 00 00 36 49 F0
			sc_142 = 0x3292F0; //0x38A4E8 + 142*8 = 0038A958 -> 80 00 00 00 00 32 92 F0
			#endif
		}
		else
		if(c_firmware >= 4.75f /*&& c_firmware <= LATEST_CFW*/)
		{
			apply_lv2_patches_new(0x800000000026D868ULL, 0x8000000000059F5CULL, 0x800000000005A020ULL,
								  0x8000000000059FCCULL, 0x800000000005E0B0ULL,
								  0x8000000000059C00ULL, 0x800000000022DAD0ULL,
								  0x800000000005962CULL, 0x800000000005C7ECULL);
			#ifndef COBRA_ONLY
			if(c_firmware >= 4.81f /*&& c_firmware <= LATEST_CFW*/)
			{
				sc_600 = 0x364928;
				sc_604 = 0x364A00;
				sc_142 = 0x329300;
			}
			else
			{
				sc_600 = 0x364848; //0x38A3E8 + 600*8 = 0038B6A8 -> 80 00 00 00 00 36 48 48
				sc_604 = 0x364920; //0x38A3E8 + 604*8 = 0038B6C8 -> 80 00 00 00 00 36 49 20
				sc_142 = 0x329220; //0x38A3E8 + 142*8 = 0038A858 -> 80 00 00 00 00 32 92 20
			}
			#endif
		}
	}
#endif //#ifdef DEX_SUPPORT
}

#ifndef LITE_EDITION
static u32 GetApplicableVersion(void * data)
{
	system_call_8(863, 0x6011, 1, (uint32_t)data, 0, 0, 0, 0, 0);
	return_to_user_prog(u32);
}
#endif
