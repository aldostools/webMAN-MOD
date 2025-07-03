#define DEFAULT_MIN_FANSPEED	(25) /* % */
#define DEFAULT_MAX_FANSPEED	(80) /* % */
#define MIN_FANSPEED			(20) /* % */
#define MIN_FANSPEED_8BIT		(0x33) /* 20% */
#define MAX_FANSPEED_8BIT		(0xF4) /* 95% */
#define MAX_TEMPERATURE			(85) /* °C */
#define MY_TEMP 				(68) /* °C */

#define FAN_AUTO 				(0)	/* webman_config->man_speed: 0 = FAN_AUTO, > 0 = manual speed */
#define FAN_MANUAL				(0)	/* webman_config->dyn_temp = max_temp: 0 = FAN_MANUAL, > 0 = target temp */

#define FAN_AUTO2				(2)	/* webman_config->fanc: 0 = SYSCON, 1 = DYNAMIC, 2 = FAN_AUTO2) */

#define PERCENT_TO_8BIT(val)	(u8)(((int)(val) * 255) / 100)

/* enable_fan_control */
#define DISABLED		(0)
#define ENABLED			(1)
#define TOGGLE_MODE		(2)
#define ENABLE_SC8		(3)
#define PS2_MODE_OFF	(4)
#define ENABLE_AUTO2	(5)

#define SET_PS2_MODE	(0)
#define SYSCON_MODE		(1)
#define MANUAL_MODE		(2)

static u8 fan_speed = MIN_FANSPEED_8BIT; // 0x33
static u8 old_fan   = MIN_FANSPEED_8BIT; // 0x33
static u8 min_fan_speed = MIN_FANSPEED_8BIT; // 0x33
static u8 max_fan_speed = MAX_FANSPEED_8BIT; // 0xF4
static u8 max_temp  = 0; //target temperature (0 = FAN_MANUAL/syscon)
static u8 fan_step_size = 0x05;

#define SC_SET_FAN_POLICY		(389)
#define SC_GET_FAN_POLICY		(409)
#define SC_GET_TEMPERATURE		(383)

u64 get_fan_policy_offset  = 0;
u64 set_fan_policy_offset  = 0;
u64 restore_set_fan_policy = 0; // set in firmware.h

static bool ps2_classic_mounted = false;
static bool fan_ps2_mode = false; // temporary disable dynamic fan control

static void get_temperature(u32 _dev, u8 *temp)
{
	u32 _temp;
	system_call_2(SC_GET_TEMPERATURE, (u64)(u32) _dev, (u64)(u32)&_temp); *temp = _temp >> 24; // return °C
}

static void sys_sm_set_fan_policy(u8 unknown , u8 fan_mode, u8 fan_speed)
{
	if(is_ingame_first_15_seconds()) return; // skip fan control first 15secs in-game

	// syscon mode: 0, 1, 0x0
	// manual mode: 0, 2, fan_speed (0x33 - 0xFF)
	u64 enable_set_fan_policy = 0x3860000100000000ULL | (restore_set_fan_policy & 0xffffffffULL);

	lv2_poke_fan(set_fan_policy_offset, enable_set_fan_policy);
	system_call_3(SC_SET_FAN_POLICY, (u64) unknown, (u64) fan_mode, (u64) fan_speed);
	if(fan_mode == SYSCON_MODE || payload_ps3hen) lv2_poke_fan(set_fan_policy_offset, restore_set_fan_policy);
}

static void sys_sm_get_fan_policy(u8 id, u8 *st, u8 *mode, u8 *speed, u8 *unknown)
{
	if(is_ingame_first_15_seconds()) return; // skip fan control first 15secs in-game

	u64 restore_get_fan_policy = peekq(get_fan_policy_offset); // sys 409 get_fan_policy
	u64 enable_get_fan_policy = 0x3860000100000000ULL | (restore_get_fan_policy & 0xffffffffULL);

	lv2_poke_fan(get_fan_policy_offset, enable_get_fan_policy);
	system_call_5(SC_GET_FAN_POLICY, (u64) id, (u64)(u32) st, (u64)(u32) mode, (u64)(u32) speed, (u64)(u32) unknown);
	lv2_poke_fan(get_fan_policy_offset, restore_get_fan_policy);
}

static void set_fan_speed(u8 new_fan_speed)
{
	if(fan_ps2_mode || ps2_classic_mounted) return; //do not change fan settings while PS2 game is mounted

	if(is_ingame_first_15_seconds()) return;

	if(get_fan_policy_offset)
	{
		{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

		min_fan_speed = PERCENT_TO_8BIT(webman_config->minfan); if(min_fan_speed < MIN_FANSPEED_8BIT) min_fan_speed = MIN_FANSPEED_8BIT; // 20%
		max_fan_speed = PERCENT_TO_8BIT(webman_config->maxfan); if(max_fan_speed < 0x66) max_fan_speed = 0xCC; // 80% (0xCC) if < 40% (0x66)
		fan_step_size = (max_fan_speed - min_fan_speed) / MAX(webman_config->dyn_temp - 60, 1); // Anything under 60 is controlled by SYSCON

		if(!max_temp && IS_INGAME) new_fan_speed += PERCENT_TO_8BIT(webman_config->man_ingame); // add in-game increment for manual fan speed

		if(new_fan_speed < MIN_FANSPEED_8BIT)
		{
			u8 st, mode, unknown;
			u8 fan_speed8 = 0;
			sys_sm_get_fan_policy(0, &st, &mode, &fan_speed8, &unknown);
			fan_speed = RANGE(fan_speed8, min_fan_speed, max_fan_speed);
		}
		else
			fan_speed = RANGE(new_fan_speed, min_fan_speed , max_fan_speed);

		old_fan = fan_speed;
		sys_sm_set_fan_policy(0, MANUAL_MODE, fan_speed);

		{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }
	}
}

static void restore_fan(u8 set_syscon_mode)
{
	if(set_fan_policy_offset)
	{
		//pokeq(syscall_base + (u64) (130 * 8), backup[3]);
		//pokeq(syscall_base + (u64) (138 * 8), backup[4]);
		//pokeq(syscall_base + (u64) (379 * 8), backup[5]);

		{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

		if(set_syscon_mode)
			sys_sm_set_fan_policy(0, SYSCON_MODE, 0); //syscon
		else // SET_PS2_MODE
		{
			webman_config->ps2_rate = RANGE(webman_config->ps2_rate, MIN_FANSPEED, webman_config->maxfan); //%
			sys_sm_set_fan_policy(0, MANUAL_MODE, PERCENT_TO_8BIT(webman_config->ps2_rate)); // PS2_MODE
			fan_ps2_mode = true;
		}

		{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }
	}
}

static u8 original_fanc = ENABLED;

static void enable_fan_control(u8 enable)
{
	if(enable == PS2_MODE_OFF)	ps2_classic_mounted = fan_ps2_mode = false; else
	if(enable == ENABLE_SC8)	webman_config->fanc = original_fanc;	else
	if(enable == ENABLE_AUTO2)	webman_config->fanc = FAN_AUTO2;		else // 2 = AUTO2
	if(enable <= ENABLED)		webman_config->fanc = enable;			else // 1 = ENABLED / 0 = SYSCON
	/* enable==TOGGLE_MODE*/	webman_config->fanc = (webman_config->fanc ? DISABLED : original_fanc);

	max_temp = 0; // syscon
	if(webman_config->fanc)
	{
		if(webman_config->man_speed == FAN_AUTO) max_temp = webman_config->dyn_temp;
		set_fan_speed(webman_config->man_speed);
	}
	else
	{
		restore_fan(SYSCON_MODE); //syscon
	}
	save_settings();

	if(enable != PS2_MODE_OFF)
	{
		show_status(STR_FANCTRL3, (webman_config->fanc));
	}

	if(enable == ENABLE_SC8) { PS3MAPI_ENABLE_ACCESS_SYSCALL8 }
}

static void reset_fan_mode(void)
{
	fan_ps2_mode = false;

	webman_config->man_speed = PERCENT_TO_8BIT(webman_config->man_rate + 1); // manual fan speed
	webman_config->man_speed = RANGE(webman_config->man_speed, MIN_FANSPEED_8BIT, MAX_FANSPEED_8BIT);
	set_fan_speed(webman_config->man_speed);

	if(max_temp) webman_config->man_speed = FAN_AUTO; // enable dynamic fan mode
}
