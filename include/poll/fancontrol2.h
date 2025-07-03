if(is_ingame_first_15_seconds() == false)
{
	if(fan_ps2_mode || ps2_classic_mounted) /* skip dynamic fan control */; else

	// dynamic fan control
	if(max_temp)
	{
		t1 = t2 = 0;
		get_temperature(0, &t1); // CPU: 3E030000 -> 3E.03°C -> 62.(03/256)°C
		get_temperature(1, &t2); // RSX: 3E030000 -> 3E.03°C -> 62.(03/256)°C

		#ifndef LITE_EDITION
		if(webman_config->chart && ++chart_count >= 4)
		{
			chart_count = 0;
			CellRtcTick pTick; cellRtcGetCurrentTick(&pTick); u32 dd, hh, mm, ss;
			ss = (u32)((pTick.tick - rTick.tick)/1000000);
			dd = (u32)(ss / 86400); ss %= 86400; hh = (u32)(ss / 3600); ss %= 3600; mm = (u32)(ss / 60); ss %= 60;

			if(!chart_init)
			{
				if(file_exists("/dev_hdd0/cpursx.html"))
					force_copy("/dev_hdd0/cpursx.html", (char*)(CPU_RSX_CHART)); // copy the chart template
				else
				{
					char title[40]; get_sys_info(title, 30, false);
					sprintf(msg, "%s15\"><h2>%s</h2>"
								 "<LINK href=\"chart.css\" rel=\"stylesheet\" type=\"text/css\">"
								 "<div class='canvas'>", HTML_REFRESH, title);
					save_file(CPU_RSX_CHART, msg, SAVE_ALL);
					chart_init = 250;
				}
			}
			else
				--chart_init;

			u8 fs = (old_fan * 100) / 255;
			sprintf(msg, "%02d:%02d:%02d CPU: %i&deg;C  RSX: %i&deg;C FAN: %i%%"
						 "<div class='cpu' style='width:%i%%'>%i&deg;</div>"
						 "<div class='rsx' style='width:%i%%'>%i&deg;</div>"
						 "<div class='fan' style='width:%i%%'>%i%%</div>",
					hh, mm, ss, t1, t2, fs, t1, t1, t2, t2, fs, fs);
			save_file(CPU_RSX_CHART, msg, APPEND_TEXT);
		}
		#endif // #ifndef LITE_EDITION

		//t1 -= 5; // allow a gap of +5°C between CPU (t1) and RSX (t2)
		if(t2 > t1) t1 = t2;

		if(!lasttemp) lasttemp = t1;

		delta = (lasttemp - t1), lasttemp = t1;

		////////////////////////// DYNAMIC FAN CONTROL #2 ////////////////////////
		if(webman_config->fanc == FAN_AUTO2)
		{
			if(delta)
			{
				// ORIGINAL CURVE
				// 60°C=31%, 61°C=33%, 62°C=35%, 63°C=37%, 64°C=39%, 65°C=41%, 66°C=43%, 67°C=45%, 68°C=47%, 69°C=49%
				// 70°C=50%, 71°C=53%, 72°C=56%, 73°C=59%, 74°C=62%, 75°C=65%, 76°C=68%, 77°C=71%, 78°C=74%, 79°C=77%,+80°C=80%

				// NEW SETTINGS
				// The fan curve will now follow the settings for Auto, with a target max temp, min fan speed, and max fan speed.
				// This makes it more adjustable to a user's preference.

				u8 fan_speed = 0;

				if(t1 >= max_temp)
					fan_speed = max_fan_speed;
				else if(t1 >= 60)
					fan_speed = (min_fan_speed + fan_step_size * (t1 - 60)); // Adjusted based on target temp and fan min / max.

				if(fan_speed)
				{
					old_fan = MAX(min_fan_speed, fan_speed);
					set_fan_speed(old_fan);
				}
				else
					sys_sm_set_fan_policy(0, 1, 0); // SYSCON < 60°C
			}
		}
		////////////////////////// DYNAMIC FAN CONTROL ///////////////////////////
		else if(webman_config->man_speed == FAN_AUTO)
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
						if(t1 <= (max_temp - cc)) {fan_speed--; if(fan_speed > 0x66) fan_speed -= 2;} // decrease fan speed faster if > 40% & cpu is very cool
				}
			}

			if((t1 > 76) && (old_fan < 0x66)) fan_speed += step_up; // increase fan speed faster if < 40% and cpu is too hot

			if(fan_speed < PERCENT_TO_8BIT(webman_config->minfan)) fan_speed = PERCENT_TO_8BIT(webman_config->minfan);
			if(fan_speed > MAX_FANSPEED_8BIT) fan_speed = MAX_FANSPEED_8BIT;

			if((old_fan != fan_speed) || (stall > 35))
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
						if(fan_speed < MAX_FANSPEED_8BIT) fan_speed += 8;

					old_fan = fan_speed;
					set_fan_speed(fan_speed);

					show_msg(STR_OVERHEAT2);
				}
			}
		}
	}
}
