#ifdef OVERCLOCKING
if(islike(param, "/gpuclock.ps3") || islike(param, "/memclock.ps3") || islike(param, "/memclock2.ps3"))
{
	u8 gpu = islike(param, "/memclock.ps3") ? 1 : islike(param, "/memclock2.ps3") ? 2 : 0;
	u8 pos = (gpu == 2) ? 14 : 13;

	if(param[pos] == '?')
	{;
		u16 mhz = (u16)val(param + pos + 1); // new gpu clock speed (300-1200)
		overclock(mhz, gpu);

		char *slash = strchr(param, '|');
		if(slash)
		{
			mhz = (u16)val(++slash); // new gpu/vram clock speed (300-1200)
			overclock(mhz, !gpu);

			slash = strchr(slash, '|');
			if(slash)
			{
				mhz = (u16)val(++slash); // new vram clock2 speed (300-1200)
				overclock(mhz, 2);
			}
		}
	}

	show_rsxclock(param);

	keep_alive = http_response(conn_s, header, "/gpuclock.ps3", CODE_HTTP_OK, param);

	goto exit_handleclient_www;
}
#endif
