#if defined(SPOOF_CONSOLEID) || defined(PS3MAPI)
u64 IDPS[2] = {0, 0};
u64 PSID[2] = {0, 0};
#endif

#ifdef SPOOF_CONSOLEID

#define SC_GET_IDPS 					(870)
#define SC_GET_PSID 					(872)

#define PS3MAPI_OPCODE_GET_IDPS 		0x0081
#define PS3MAPI_OPCODE_SET_IDPS 		0x0082
#define PS3MAPI_OPCODE_GET_PSID 		0x0083
#define PS3MAPI_OPCODE_SET_PSID			0x0084

#define FLASH_DEVICE_NAND				0x0100000000000001ULL
#define FLASH_DEVICE_NOR				0x0100000000000004ULL
#define FLASH_FLAGS						0x22ULL

u64 idps_offset1 = 0;
u64 idps_offset2 = 0;
u64 psid_offset  = 0;

u64 eid0_idps[2] = {0, 0};

/*
static s32 sys_ss_appliance_info_manager(u32 packet_id, u64 arg)
{
	system_call_2(867, packet_id, arg);
	return_to_user_prog(s32);
}

static s32 ss_aim_get_device_id(u8 *idps)
{
	return sys_ss_appliance_info_manager(0x19003, (u32)(idps));
}

static s32 ss_aim_get_open_psid(u8 *psid)
{
	return sys_ss_appliance_info_manager(0x19005, (u32)(psid));
}
*/

static void get_idps_psid(void)
{
	{system_call_2(867, 0x19003, (u32)IDPS);}
	{system_call_2(867, 0x19005, (u32)PSID);}
}

static void spoof_idps_psid(void)
{
	{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

	if(webman_config->spsid)
	{
		u64 newPSID[2] = {0, 0};

		newPSID[0] = convertH(webman_config->vPSID1);
		newPSID[1] = convertH(webman_config->vPSID2);

#ifdef COBRA_ONLY
		{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PSID, (u64)newPSID[0], (u64)newPSID[1]);}
#else
#ifndef LAST_FIRMWARE_ONLY
		if(c_firmware <= 4.53f)
		{
			{system_call_1(SC_GET_PSID, (u64) PSID);}
			for(u64 addr = 0x80000000003B0000ULL; addr < 0x80000000004A0000ULL; addr+=4)
			{
				if((peekq(addr) == PSID[0]) && (peekq(addr+8) == PSID[1]))
				{
					pokeq(addr, newPSID[0]); addr+=8;
					pokeq(addr, newPSID[1]); addr+=8;
				}
			}
		}
		else
#endif
		if(psid_offset)
		{
			pokeq(psid_offset  , newPSID[0]);
			pokeq(psid_offset+8, newPSID[1]);
		}
#endif
	}

	if(webman_config->sidps)
	{
		u64 newIDPS[2] = {0, 0};

		newIDPS[0] = convertH(webman_config->vIDPS1);
		newIDPS[1] = convertH(webman_config->vIDPS2);

		// IDPS must be like 00000001008*00**
		if((newIDPS[0] & 0xFFFFFFFFFFF0FF00ULL) == 0x0000000100800000ULL && newIDPS[1] != 0)
		{
#ifdef COBRA_ONLY
			{system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_IDPS, (u64)newIDPS[0], (u64)newIDPS[1]);}
#else
#ifndef LAST_FIRMWARE_ONLY
			if(c_firmware <= 4.53f)
			{
				{system_call_1(SC_GET_IDPS, (u64) IDPS);}
				for(u64 addr = 0x80000000003B0000ULL; addr < 0x80000000004A0000ULL; addr+=4)
				{
					if((peekq(addr) == IDPS[0]) && (peekq(addr + 8) == IDPS[1]))
					{
						pokeq(addr, newIDPS[0]); addr+=8;
						pokeq(addr, newIDPS[1]); addr+=8;
					}
				}
			}
			else
#endif
			if(idps_offset1 | idps_offset2)
			{
				pokeq(idps_offset1  , newIDPS[0]);
				pokeq(idps_offset1+8, newIDPS[1]);
				pokeq(idps_offset2  , newIDPS[0]);
				pokeq(idps_offset2+8, newIDPS[1]);
			}
#endif
		}
	}

	get_idps_psid();
}

static void get_eid0_idps(void)
{
	get_idps_psid();

	if(eid0_idps[0]) return;

	u64 buffer[0x40], start_sector = 0x178; // NOR
	u32 read;
	sys_device_handle_t dev_id;
	if(sys_storage_open(FLASH_DEVICE_NOR, 0, &dev_id, 0) != CELL_OK)
	{
		sys_storage_open(FLASH_DEVICE_NAND, 0, &dev_id, 0);
		start_sector = 0x204; // NAND
	}
	sys_storage_read(dev_id, 0, start_sector, 1, buffer, &read, FLASH_FLAGS);
	sys_storage_close(dev_id);

	eid0_idps[0] = buffer[0x0E];
	eid0_idps[1] = buffer[0x0F];

	if((eid0_idps[0] & 0xFFFFFFFFFFF0FF00ULL) != 0x0000000100800000ULL) eid0_idps[0] = eid0_idps[1] = 0;
}

static void show_idps(char *msg)
{
	if(!sys_admin) return;

	get_eid0_idps();

	#define SEP "\n                  "
	sprintf(msg, "IDPS EID0 : %016llX%s"
							 "%016llX\n"
				 "IDPS LV2  : %016llX%s"
							 "%016llX\n"
				 "PSID LV2 : %016llX%s"
							"%016llX", eid0_idps[0], SEP, eid0_idps[1], IDPS[0], SEP, IDPS[1], PSID[0], SEP, PSID[1]);
	#undef SEP

	show_msg(msg);
	sys_ppu_thread_sleep(2);
}

static void save_idps_psid(bool is_psid, bool is_idps, char *header, char *param)
{
	char *filename = param + 9, *act_dat = header;
	bool is_default = (*filename != '/') || (is_psid & is_idps);

	show_idps(header);

	*act_dat = NULL; int i = 0;

	cellFsUnlink("/dev_hdd0/psid.hex");
	cellFsUnlink("/dev_hdd0/idps.hex");
	cellFsUnlink("/dev_hdd0/act.dat");

	if(is_psid)
	{
		for(i = 1; i >= 0; i--)
		{
			if(is_default) {sprintf(filename, "%s/psid.hex", drives[i]); sprintf(act_dat, "%s/act.dat", drives[i]);}
			save_file(filename, (char*)&PSID[0], 16);
			if(file_exists(filename)) break; is_default = true;
		}
	}

	if(is_idps)
	{
		for(i = 1; i >= 0; i--)
		{
			if(is_default) {sprintf(filename, "%s/idps.hex", drives[i]); sprintf(act_dat, "%s/act.dat", drives[i]);}
			save_file(filename, (char*)&IDPS[0], 16);
			if(file_exists(filename)) break; is_default = true;
		}
	}

	if(*act_dat)
	{
		sprintf(filename, "%s/home/%08i/exdata/act.dat", drives[0], xsetting_CC56EB2D()->GetCurrentUserNumber());
		_file_copy(filename, act_dat);
	}

	*param = NULL;

	if(is_default)
	{
		if(file_exists(act_dat))
		{
			add_breadcrumb_trail(param, act_dat); strcat(param, " from: ");
			sprintf(header, "%s/home/%08i", drives[0], xsetting_CC56EB2D()->GetCurrentUserNumber());
			add_breadcrumb_trail(param, header); strcat(param, "<br>");
		}
	}
	else
		strcpy(header, filename); // show custom filename

	if(is_idps)
	{
		if(is_default) sprintf(header, "%s/idps.hex", drives[i]);
		if(file_exists(header)) {add_breadcrumb_trail(param, header); sprintf(header, " • %016llX%016llX<br>", IDPS[0], IDPS[1]); strcat(param, header);}
	}
	if(is_psid)
	{
		if(is_default) sprintf(header, "%s/psid.hex", drives[i]);
		if(file_exists(header)) {add_breadcrumb_trail(param, header); sprintf(header, " • %016llX%016llX<br>", PSID[0], PSID[1]); strcat(param, header);}
	}
}

#endif // #ifdef SPOOF_CONSOLEID
