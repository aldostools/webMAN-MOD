#define MIN_FANSPEED			(20) /* % */
#define DEFAULT_MIN_FANSPEED	(25) /* % */
#define MAX_FANSPEED			(0xF4)
#define MAX_TEMPERATURE			(85) /* °C */
#define MY_TEMP 				(68) /* °C */

#define FAN_AUTO 				(0)
#define FAN_MANUAL				(0)

#define FAN_AUTO2				(2)

#define PERCENT_TO_8BIT(val)	((val * 255) / 100)

#define DISABLED		(0)
#define ENABLED			(1)
#define TOGGLE_MODE		(2)
#define ENABLE_SC8		(3)
#define PS2_MODE_OFF	(4)

#define SET_PS2_MODE	(0)
#define SYSCON_MODE		(1)
#define MANUAL_MODE		(2)

static u8 fan_speed = 0x33;
static u8 old_fan = 0x33;
static u8 max_temp = 0; //syscon

#define SC_SET_FAN_POLICY		(389)
#define SC_GET_FAN_POLICY		(409)
#define SC_GET_TEMPERATURE		(383)

u64 get_fan_policy_offset  = 0;
u64 set_fan_policy_offset  = 0;
u64 restore_set_fan_policy = 0; // set in main.c

static bool fan_ps2_mode = false; // temporary disable dynamic fan control

static void get_temperature(u32 _dev, u8 *temp)
{
	u32 _temp;
	system_call_2(SC_GET_TEMPERATURE, (u64)(u32) _dev, (u64)(u32)&_temp); *temp = _temp >> 24; // return °C
}

static void sys_sm_set_fan_policy(u8 unknown , u8 fan_mode, u8 fan_speed)
{
	// syscon mode: 0, 1, 0x0
	// manual mode: 0, 2, fan_speed (0x33 - 0xFF)

	u64 enable_set_fan_policy = 0x3860000100000000ULL | (restore_set_fan_policy & 0xffffffffULL);

	lv2_poke_fan(set_fan_policy_offset, enable_set_fan_policy);
	system_call_3(SC_SET_FAN_POLICY, (u64) unknown, (u64) fan_mode, (u64) fan_speed);
	if(fan_mode == SYSCON_MODE || payload_ps3hen) lv2_poke_fan(set_fan_policy_offset, restore_set_fan_policy);
}

static void sys_sm_get_fan_policy(u8 id, u8 *st, u8 *mode, u8 *speed, u8 *unknown)
{
	u64 restore_get_fan_policy = peekq(get_fan_policy_offset); // sys 409 get_fan_policy
	u64 enable_get_fan_policy = 0x3860000100000000ULL | (restore_get_fan_policy & 0xffffffffULL);

	lv2_poke_fan(get_fan_policy_offset, enable_get_fan_policy);
	system_call_5(SC_GET_FAN_POLICY, (u64) id, (u64)(u32) st, (u64)(u32) mode, (u64)(u32) speed, (u64)(u32) unknown);
	lv2_poke_fan(get_fan_policy_offset, restore_get_fan_policy);
}

static void set_fan_speed(u8 new_fan_speed)
{
	if(fan_ps2_mode) return; //do not change fan settings while PS2 game is mounted

	if(get_fan_policy_offset)
	{
		{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

		u8 min_fan_speed = PERCENT_TO_8BIT(webman_config->minfan);

		if(new_fan_speed < 0x33)
		{
			u8 st, mode, unknown;
			u8 fan_speed8 = 0;
			sys_sm_get_fan_policy(0, &st, &mode, &fan_speed8, &unknown);
			fan_speed = RANGE(fan_speed8, min_fan_speed , 0xFC);
		}
		else
			fan_speed = RANGE(new_fan_speed, min_fan_speed , 0xFC);

		old_fan = fan_speed;
		sys_sm_set_fan_policy(0, MANUAL_MODE, fan_speed);

		{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }
	}
}

static void restore_fan(u8 set_syscon_mode)
{
	if(get_fan_policy_offset > 0)
	{
		//pokeq(syscall_base + (u64) (130 * 8), backup[3]);
		//pokeq(syscall_base + (u64) (138 * 8), backup[4]);
		//pokeq(syscall_base + (u64) (379 * 8), backup[5]);

		{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

		if(set_syscon_mode)
			sys_sm_set_fan_policy(0, SYSCON_MODE, 0); //syscon
		else
		{
			webman_config->ps2_rate = RANGE(webman_config->ps2_rate, MIN_FANSPEED, 99); //%
			sys_sm_set_fan_policy(0, MANUAL_MODE, PERCENT_TO_8BIT(webman_config->ps2_rate)); // PS2_MODE
			fan_ps2_mode = true;
		}

		{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }
	}
}

static void enable_fan_control(u8 enable, char *msg)
{
	if(enable == PS2_MODE_OFF) fan_ps2_mode = false;		else
	if(enable == ENABLE_SC8) webman_config->fanc = ENABLED;	else
	if(enable <= ENABLED)	 webman_config->fanc = enable;	else
							 webman_config->fanc = (webman_config->fanc ? DISABLED : ENABLED);

	max_temp = 0;
	if(webman_config->fanc)
	{
		if(webman_config->man_speed == FAN_AUTO) max_temp = webman_config->dyn_temp;
		set_fan_speed(webman_config->man_speed);
		sprintf(msg, "%s %s", STR_FANCTRL3, STR_ENABLED);
	}
	else
	{
		restore_fan(SYSCON_MODE); //syscon
		sprintf(msg, "%s %s", STR_FANCTRL3, STR_DISABLED);
	}
	save_settings();
	if(enable != PS2_MODE_OFF) show_msg(msg);

	if(enable == ENABLE_SC8) { PS3MAPI_ENABLE_ACCESS_SYSCALL8 }
}

static void reset_fan_mode(void)
{
	fan_ps2_mode = false;

	webman_config->man_speed = (u8)(((float)(webman_config->man_rate + 1) * 255.f) / 100.f); // manual fan speed
	webman_config->man_speed = RANGE(webman_config->man_speed, 0x33, MAX_FANSPEED);
	set_fan_speed(webman_config->man_speed);

	if(max_temp) webman_config->man_speed = FAN_AUTO; // enable dynamic fan mode
}
