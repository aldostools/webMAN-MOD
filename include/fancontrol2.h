	if(fan_ps2_mode) /* skip dynamic fan control */; else

	// dynamic fan control
	if(max_temp)
	{
		t1 = t2 = 0;
		get_temperature(0, &t1); // CPU: 3E030000 -> 3E.03°C -> 62.(03/256)°C
		get_temperature(1, &t2); // RSX: 3E030000 -> 3E.03°C -> 62.(03/256)°C

		if(t2 > t1) t1 = t2;

		if(!lasttemp) lasttemp = t1;

		delta = (lasttemp - t1), lasttemp = t1;

		////////////////////////// DYNAMIC FAN CONTROL #2 ////////////////////////
		if(webman_config->fanc == FAN_AUTO2)
		{
			if(delta)
			{
				// 60°C=31%, 61°C=33%, 62°C=35%, 63°C=37%, 64°C=39%, 65°C=41%, 66°C=43%, 67°C=45%, 68°C=47%, 69°C=49%
				// 70°C=50%, 71°C=55%, 72°C=60%, 73°C=65%, 74°C=70%, 75°C=75%, 76°C=80%, 77°C=85%, 78°C=90%,+79°C=98%

				u8 fan_speed = 0;

				if(t1 > 78)
					fan_speed = 0xF8; // 98%
				else if(t1 >= 70)
					fan_speed = (0x80 + 0xD * (t1 - 70)); // 50% + 5% per degree
				else if(t1 >= 60)
					fan_speed = (0x50 + 0x5 * (t1 - 60)); // 30% + 2% per degree

				if(fan_speed)
				{
					u8 min_speed = PERCENT_TO_8BIT(webman_config->minfan);
					set_fan_speed(MAX(min_speed, fan_speed));
				}
				else
					sys_sm_set_fan_policy(0, 1, 0); // SYSCON < 60°C
			}
		}
		////////////////////////// DYNAMIC FAN CONTROL ///////////////////////////
		else
		{
			if((t1 >= max_temp) || (t1 >= MAX_TEMPERATURE))
			{
				for(u8 cc = 0; cc < 5; cc++)
					if (delta  < -cc) fan_speed += 2;

				if (delta < 0)
				{
					if((t1 - max_temp) >= 2) fan_speed += step_up;
				}
				else
				if (delta == 0)
				{
					if(t1 != (max_temp - 1)) fan_speed++;
					if(t1 >= (max_temp + 1)) fan_speed += (2 + (t1 - max_temp));
				}
				else
				if (delta  > 0)
				{
					smoothstep++;
					if(smoothstep > 1)
					{
						smoothstep = 0, fan_speed--;
					}
				}

				if(t1 >= MAX_TEMPERATURE) fan_speed += step_up;
			}
			else
			{
				for(u8 cc = 0; cc < 5; cc++)
					if((delta < -cc) && (t1 >= (max_temp - 1))) fan_speed += 2; // increase fan speed faster proportionally to temp increase (delta)

				if((delta == 0) && (t1 <= (max_temp - 2)))
				{
					if(++smoothstep > 1) delta = 1;
				}

				if(delta > 0)
				{
					smoothstep = 0;

					fan_speed--;
					for(u8 cc = 1; cc < 5; cc++)
						if(t1 <= (max_temp - cc)) {fan_speed--; if(fan_speed>0x66) fan_speed -= 2;} // decrease fan speed faster if > 40% & cpu is very cool
				}
			}

			if((t1 > 76) && (old_fan < 0x66)) fan_speed += step_up; // increase fan speed faster if < 40% and cpu is too hot

			if(fan_speed < PERCENT_TO_8BIT(webman_config->minfan)) fan_speed = PERCENT_TO_8BIT(webman_config->minfan);
			if(fan_speed > MAX_FANSPEED) fan_speed = MAX_FANSPEED;

			if(old_fan!=fan_speed || stall > 35)
			{
				if(t1 > 78 && fan_speed < 0x50) fan_speed += 2; // <31%
				if(old_fan != fan_speed)
				{
					old_fan = fan_speed;
					set_fan_speed(fan_speed);
					stall = 0;
				}
			}
			else
				if( (old_fan > fan_speed) && ((old_fan - fan_speed) > 8) && (t1 < (max_temp - 3)) )
					stall++;
		}
	}

	//////////////////////////////////
	// Overheat control (over 83°C) //
	//////////////////////////////////
	if(++oc > 40)
	{
		oc = 0;

		if(!webman_config->nowarn)
		{
			get_temperature(0, &t1); // CPU
			get_temperature(1, &t2); // RSX

			u8 t = t1; if(t2 > t) t = t2;

			if(t > (MAX_TEMPERATURE - 2))
			{
				#ifndef ENGLISH_ONLY
				char STR_OVERHEAT[80];//	= "System overheat warning!";
				char STR_OVERHEAT2[120];//	= "  OVERHEAT DANGER!\nFAN SPEED INCREASED!";

				language("STR_OVERHEAT", STR_OVERHEAT, "System overheat warning!");
				language("STR_OVERHEAT2", STR_OVERHEAT2, "  OVERHEAT DANGER!\nFAN SPEED INCREASED!");

				close_language();
				#endif

				sprintf(msg, "%s\n CPU: %i°C   RSX: %i°C", STR_OVERHEAT, t1, t2);
				show_msg(msg);
				sys_ppu_thread_sleep(2);

				if(t > MAX_TEMPERATURE)
				{
					if(!max_temp) max_temp = (MAX_TEMPERATURE - 3);

					if(fan_speed < 0xB0) fan_speed = 0xB0; // 69%
					else
						if(fan_speed < MAX_FANSPEED) fan_speed += 8;

					old_fan = fan_speed;
					set_fan_speed(fan_speed);

					if(!webman_config->nowarn) show_msg((char*)STR_OVERHEAT2);
				}
			}
		}
	}
