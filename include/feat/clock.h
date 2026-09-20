#ifdef OVERCLOCKING
// (C) 2025 by Chattrapat Sangmanee for the overclocking code
// https://github.com/aomsin2526

static bool pergame_overclocking = false;

typedef union
{
	struct
	{
		uint8_t  junk0;
		uint8_t  junk1;
		uint8_t  mul2; // vram clock2
		uint8_t  junk2;
		
		uint8_t  junk3;
		uint8_t  junk4;
		uint8_t  mul; // vram/core clock
		uint8_t  junk5;
	};

	uint64_t value;
} clock_s;

#define GPU_CORE_CLOCK		0x28000004028
#define GPU_VRAM_CLOCK		0x28000004010
#define GPU_VRAM_CLOCK2		0x28000004020

// Enforce In-order Execution of I/O
#define eieio()                \
	{                          \
		asm volatile("eieio"); \
		asm volatile("sync");  \
	}

static void overclock(u16 mhz, u8 gpu)
{
	if(BETWEEN(300, mhz, 1200))
	{
		u64 clock_address = (gpu == 1) ? GPU_VRAM_CLOCK : (gpu == 2) ? GPU_VRAM_CLOCK2 : GPU_CORE_CLOCK;

		clock_s clock;
		clock.value = lv1_peek_cobra(clock_address);

		if (clock.value == 0xFFFFFFFF80010003) // if cfw syscalls are disabled
			return;

		if(gpu == 0)
		{
			clock.mul = (u8)(mhz / 50); // GPU Core Clock speed
			lv1_poke_cfw(GPU_CORE_CLOCK, clock.value);
			eieio();
		}
		else // apply vram frequency must be applied slowly in 25mhz step, wait, repeat until reach target
 		{
			u8 target_mul = (u8)(mhz / 25);

			if(gpu == 1)
			{
				bool up = (target_mul > clock.mul);
				while (clock.mul != target_mul)
				{
					sys_timer_usleep(125000); // 1/8 sec = 125ms

					// overclock VRAM Clock Speed (original)
					clock.mul += up ? 1 : -1;
					lv1_poke_cfw(GPU_VRAM_CLOCK, clock.value);

					eieio();
				}
			}
			if(gpu == 2)
			{
				bool up = (target_mul > clock.mul2);
				while (clock.mul2 != target_mul)
				{
					sys_timer_usleep(125000); // 1/8 sec = 125ms

					// overclock VRAM Clock 2 Speed (new address found by aomsin2526)
					clock.mul2 += up ? 1 : -1;

					lv1_poke_cfw(GPU_VRAM_CLOCK2, clock.value);

					eieio();
				}
			}
		}
	}
}
////

static void set_rsxclocks(u8 gpu_core, u8 gpu_vram)
{
	overclock(50 * (int)(gpu_core), 0);
	overclock(25 * (int)(gpu_vram), 1);
	overclock(25 * (int)(gpu_vram), 2);

	pergame_overclocking = false;
}

static int get_rsxclock(u64 clock_address) // clock_address = GPU_CORE_CLOCK or GPU_VRAM_CLOCK
{
	clock_s clock;
	clock.value = lv1_peek_cobra(clock_address);
	if(clock_address == GPU_CORE_CLOCK)
		return 50 * (int)clock.mul;
	if(clock_address == GPU_VRAM_CLOCK)
		return 25 * (int)clock.mul;
	if(clock_address == GPU_VRAM_CLOCK2)
		return 25 * (int)clock.mul2;
	return 0;
}

static void show_rsxclock(char *msg)
{
	if(get_rsxclock(GPU_CORE_CLOCK))
	{
		sprintf(msg, "GPU: %i Mhz | VRAM: %i/%i Mhz", get_rsxclock(GPU_CORE_CLOCK), get_rsxclock(GPU_VRAM_CLOCK), get_rsxclock(GPU_VRAM_CLOCK2)); show_msg(msg);
	}
}
#endif //#ifdef OVERCLOCKING

#ifdef FIX_CLOCK
static int sysSetCurrentTime(u64 sec, u64 nsec)
{
	system_call_2(146, (u32)sec, (u32)nsec);
	return_to_user_prog(int);
}

static int sys_ss_secure_rtc(u64 time)
{
	system_call_4(0x362, 0x3003, time / 1000000, 0, 0);
	return_to_user_prog(int);
}

static int sysGetCurrentTime(u64 *sec, u64 *nsec)
{
	system_call_2(145,(u32)sec, (u32)nsec);
	return_to_user_prog(int);
}

static int sys_time_get_rtc(u64 *real_time_clock)
{
	system_call_1(119, (u32)real_time_clock);
	return_to_user_prog(int);
}

static void fix_clock(char *newDate)
{
	#define DATE_2000_01_01		0x00E01D003A63A000ULL
	#define DATE_1970_01_01		0x00DCBFFEFF2BC000ULL

	u64 clock, diff;
	u64 sec, nsec;
	u64 currentTick;

	u64 patchedDate;

	static int (*_cellRtcGetCurrentTick)(u64 *pTick) = NULL;
	static int (*_cellRtcSetCurrentTick)(u64 *pTick) = NULL;
	static xsetting_8B69F85A_class*(*xSettingDateGetInterface)(void) = NULL;

	_cellRtcGetCurrentTick = getNIDfunc("cellRtc", 0x9DAFC0D9, 0);
	_cellRtcSetCurrentTick = getNIDfunc("cellRtc", 0xEB22BB86, 0);
	xSettingDateGetInterface = getNIDfunc("xsetting", 0x8B69F85A, 0);

	CellRtcDateTime rDate;

	u8 mdays[12] = {31,28,31,30,31,30,31,31,30,31,30,31}; u16 ndays;

	if(!newDate)
	{
		patchedDate = FIX_CLOCK_DATE;
	}
	else if((newDate[0] == '2') && (newDate[1] == '0') && (newDate[4] == '-') && (newDate[7] == '-')) // 2024-12-24 hh:mm:ss
	{
		newDate[4] = newDate[7] = newDate[10] = '\0';
		rDate.year = val(newDate); ndays = 120 + (int)((rDate.year - 2001) / 4); // leap days
		rDate.month = val(newDate + 5); for(u8 i = 0; i < rDate.month - 1; i++) ndays +=  mdays[i]; // year days
		rDate.day = val(newDate + 8); 
		patchedDate = ((rDate.year * 365) + rDate.day + ndays) * 86400000000ULL;
		if((newDate[13] == ':') && (newDate[16] == ':'))
		{
			newDate[13] = newDate[16] = newDate[20] = '\0';
			rDate.hour   = val(newDate + 11); patchedDate += rDate.hour * 3600000000ULL; patchedDate -= (20) * 3600000000ULL;
			rDate.minute = val(newDate + 14); patchedDate += rDate.minute * 60000000ULL;
			rDate.second = val(newDate + 17); patchedDate += rDate.second *  1000000ULL;
		}
	}
	else
	{
		patchedDate = convertH(newDate);
		if(patchedDate < DATE_2000_01_01) {patchedDate = FIX_CLOCK_DATE; newDate = NULL;}
	}

	///////////
	u64 a1, a2;
	{ system_call_4(0x362, 0x3002, 0, (u64)(u32)&a1, (u64)(u32)&a2); }
	_cellRtcGetCurrentTick(&currentTick);

	if((currentTick < FIX_CLOCK_DATE) || newDate)
	{
		_cellRtcSetCurrentTick(&patchedDate);
		sysGetCurrentTime(&sec, &nsec);
		sys_time_get_rtc(&clock);
		diff = sec - clock;
		xSettingDateGetInterface()->SaveDiffTime(diff);
	}

	u64 timedata = patchedDate - DATE_2000_01_01;
	
	if(!a1)
		sys_ss_secure_rtc(timedata);

	{ system_call_4(0x362, 0x3002, 0, (u64)(u32)&a1, (u64)(u32)&a2); }
	_cellRtcGetCurrentTick(&currentTick);
	u64 result_time2 = (currentTick - DATE_2000_01_01);

	u64 rtc_clock = (a1 * 1000000) + DATE_2000_01_01;

	if(rtc_clock < currentTick)
	{
		sys_ss_secure_rtc(result_time2);
	}
	else if(rtc_clock > currentTick)
	{
		sysSetCurrentTime(((rtc_clock - DATE_1970_01_01) / 1000000), 0);
		sysGetCurrentTime(&sec, &nsec);
		sys_time_get_rtc(&clock);
		diff = sec - clock;
		xSettingDateGetInterface()->SaveDiffTime(diff);
	}
}

static void update_clock_from_server_time(char *data)
{
	char url[36] = "http://ps3.aldostools.org/date.php";
	if(get_server_data(url, data, 250))
	{
		fix_clock(data);
	}
}
#endif