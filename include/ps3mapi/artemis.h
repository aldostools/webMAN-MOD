#ifdef ARTEMIS_PRX

#define THREAD_NAME_ART "artt"
#define uint			unsigned int
#define process_id_t	u32
#define sys_prx_id_t	int32_t
#define init_log(a)		save_file(ARTEMIS_CODES_LOG, a, SAVE_ALL)
#define add_log(a)		save_file(ARTEMIS_CODES_LOG, a, APPEND_TEXT)

#define ISVALID_CHAR(c)	(ISHEX(c) || (c == ' ') || (c == '\r') || (c == '\n'))

static vu8 art_cmd = 0;
static vu8 artemis_working = 0;
static u8 is_artemis_app = 0;

static sys_addr_t sysmem_art = NULL;
static sys_addr_t typeA_Copy = NULL;

// pad bit flags
#define	PAD_SELECT		(1<<0)
#define	PAD_L3			(1<<1)
#define	PAD_R3			(1<<2)
#define	PAD_START		(1<<3)
#define	PAD_UP			(1<<4)
#define	PAD_RIGHT		(1<<5)
#define	PAD_DOWN		(1<<6)
#define	PAD_LEFT		(1<<7)
#define	PAD_L2			(1<<8)
#define	PAD_R2			(1<<9)
#define	PAD_L1			(1<<10)
#define	PAD_R1			(1<<11)
#define	PAD_TRIANGLE	(1<<12)
#define	PAD_CIRCLE		(1<<13)
#define	PAD_CROSS		(1<<14)
#define	PAD_SQUARE		(1<<15)

static u8 doForceWrite = 0;
static u8 isConstantWrite = 0;
static process_id_t attachedPID = 0;
static int userCodesLen = 0;

#define get_process_mem		ps3mapi_get_process_mem
#define set_process_mem		ps3mapi_set_process_mem

static char *userCodes = NULL;

static double tofloat(const char *s)
{
	double d, ret = 0.0, sign = 1.0;
	int e = 0;

	/* remove leading white spaces. */
	for (; ISSPACE(*s); ) ++s;
	if (*s == '-') {
		/* negative value. */
		++s; sign = -1.0;
	}
	else if (*s == '+') ++s;
	for (; ISDIGIT(*s); ++s) {
		/* process digits before decimal point. */
		ret *= 10.0;
		ret += (double)(int)(*s - '0');
	}
	if (*s == '.') {
		for (d = 0.1, ++s; ISDIGIT(*s); ++s) {
			/* process digits after decimal point. */
			ret += (d * (double)(int)(*s - '0'));
			d *= 0.1;
		}
	}
	if (ret) {
		/* test for exponent token. */
		if ((*s == 'e') || (*s == 'E')) {
			++s; d = 10.0; // default positive exponent
			if (*s == '-') {
				/* negative exponent. */
				++s; d = 0.1;
			}
			else if (*s == '+') ++s;
			for (; ISDIGIT(*s); ++s) {
				/* process exponent digits. */
				e *= 10;
				e += (int)(*s - '0');
			}
			for (; e; --e) ret *= d;
		}
	}

	return (ret * sign);
}

static int ps3mapi_get_process_mem(process_id_t pid, u64 addr, char *buf, int size)
{
	system_call_6(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MEM, (u64)pid, (u64)addr, (u64)((u32)buf), (u64)size);
	return_to_user_prog(int);
}

static int ps3mapi_set_process_mem(process_id_t pid, u64 addr, char *buf, int size )
{
	system_call_6(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PROC_MEM, (u64)pid, (u64)addr, (u64)((u32)buf), (u64)size);
	return_to_user_prog(int);
}

/*
 * Code Parsing Functions
 */

// Converts a hex string into an array of bytes
// In cases where the len is less than 4, it will shift the value over such that something like "011" will be 0x11
static char *ReadHexPartial(char *read, int start, int len, char *buf, int bufSize)
{
	_memset(buf, bufSize);
	int str = (start + len - 1);
	int cnt = 0;
	for (int i = str; i >= start; i--, cnt++)
	{
		while (ISSPACE(read[i]) && i >= start)
			i--;
		if (i < start)
			break;

		unsigned char c = h2b(read[i]);

		int bufOff = ((bufSize < 4) ? 4 : bufSize) - (cnt/2) - 1;

		if (cnt & 1)
			buf[bufOff] |= (unsigned char)c;
		else
			buf[bufOff] |= (unsigned char)(c << 4);
	}

	return buf;
}

//	Converts a hex string into an array of bytes
//	In cases where the len is less than 4, (unlike ReadHexPartial) it will NOT shift the value
static const char *ReadHex(const char *read, int start, int len, char *buf, int bufSize)
{
	for (int i = start, n = 0; n < len; n++, i += 2)
	{
		buf[n] = (h2b(read[i])<<4) | h2b(read[i + 1]);
	}
	return buf;
}

// Writes the contents of buf to addr
static int WriteMem(process_id_t pid, u64 addr, char *buf, int size)
{
	if (addr)
	{
		char line[100];
		sprintf(line, "Write at 0x%llx => %x (%i bytes)", addr, INT32(buf), size);
		add_log(line);

		return set_process_mem(pid, addr, buf, size);
	}
	return 0;
}

// Compares the bytes at addr to buf (equal to)
static int CompareMemory(process_id_t pid, u64 addr, char *buf, int bufLen)
{
	char cmp[1];

	for (int i = 0; i < bufLen; i++)
	{
		if (get_process_mem(pid, addr + i, cmp, 1) < 0)
			return 0;
		if (buf[i] != cmp[0])
			return 0;
	}

	return 1;
}

// Compares the bytes at source to buf (equal to)
static int CompareMemoryBuffered(char *source, int sourceOff, char *buf, int bufLen)
{
	for (int i = 0; i < bufLen; i++)
	{
		if (buf[i] != source[sourceOff + i])
			return 0;
	}

	return 1;
}

// Compares the bytes at addr to buf (and equal to)
static int CompareMemoryAnd(process_id_t pid, u64 addr, char *buf, int bufLen)
{
	char cmp[bufLen];
	if (get_process_mem(pid, addr, cmp, bufLen) < 0)
		return 0;

	for (int i = 0; i < bufLen; i++)
	{
		cmp[i] &= buf[i];
		if (cmp[i] != buf[i])
			return 0;
	}

	return 1;
}

// Checks if the line is a valid code
static bool isCodeLineValid(char *line)
{
	int lineLen = strlen(line); if(lineLen < 2) return false;

	int spaceCnt = 0;
	for (int i = 0; i < lineLen; i++)
		if (line[i] == ' ')
			spaceCnt++;

	int mode =  (line[0] == '1' && line[1] == ' ') ? 1 :	// isText
				(line[0] == '2' && line[1] == ' ') ? 2 : 0;	// isFloat

	if (mode == 1)
	{
		//Check number of spaces
		if (spaceCnt < 2)
			return false;

		//Check hex
		int textSpaceCnt = 0;
		for (int i = 0; i < lineLen; i++)
		{
			char c = line[i];
			if (c == ' ')
				textSpaceCnt++;
			if (textSpaceCnt >= 2)
				return true;
		}
	}
	else // isFloat || isHex
	{
		//Check number of spaces
		if (spaceCnt != 2)
			return false;
	}

	//Check hex
	for (int i = 0; i < lineLen; i++)
	{
		char c = line[i];

		if(!ISVALID_CHAR(c))
		{
			return false;
		}
	}

	return true;
}

// Artemis PS3 Engine
static u64 ptrAddr = 0;
static uint typeA_Size = 0;
static int ParseLine(process_id_t pid, char *lines, int start, int *skip, int doForceWrite)
{
	if (pid == NULL)
		return userCodesLen;

	int lineLen;
	for (lineLen = start; lineLen < userCodesLen; lineLen++)
	{
		if (lines[lineLen] == '\n' || lines[lineLen] == '\r')
			break;
	}
	lineLen -= start; if(lineLen <= 0) return start;

	const int lineEnd = (start + lineLen);

	if (skip[0] <= 0)
	{
		skip[0] = 0;

		int arg2Len = 0, arg1Off = 0, arg0, arg0_2 = 0;
		int arg3Off = lineEnd, arg4Off = arg3Off + 1, arg4Len = arg4Off;

		//Parse first line
		while (lines[lineEnd - arg2Len] != ' ')
			arg2Len++;
		while (lines[start + arg1Off] != ' ')
			arg1Off++;

		if (arg2Len < 0)
			return userCodesLen;

		char addrBuf[4]; _memset(addrBuf, 4);

		ReadHexPartial(lines, start + 1, (arg1Off - 1), addrBuf, (arg1Off - 1)/2);
		arg0 = (int)(INT32(addrBuf));

		ReadHex(lines, start + arg1Off + 1, 8, addrBuf, 4);
		u64 addr = (u64)(INT32(addrBuf));
		if (ptrAddr)
		{
			addr = ptrAddr;
			ptrAddr = 0;
		}

		char buf0[arg2Len]; _memset(buf0, arg2Len);
		const int buf0size = arg2Len - 1;
		const int buf0Len  = arg2Len / 2;

		if (arg0 < 0)
			arg0 = 0;
		char buf1[arg0]; _memset(buf1, arg0);

		//Check if theres a second line
		if ((lineEnd + 1) < userCodesLen)
		{
			//Parse second line vars (for codes that need the second line
			//Get next code arguments
			while (arg3Off < userCodesLen && lines[arg3Off] != ' ')
				arg3Off++;
			arg4Off = arg3Off + 1;
			while (arg4Off < userCodesLen && lines[arg4Off] != ' ')
				arg4Off++;
			arg4Len = arg4Off + 1;
			while (arg4Len < userCodesLen && lines[arg4Len] != '\r' && lines[arg4Len] != '\n')
				arg4Len++;
			arg4Len -= arg4Off;
		}
		else
			arg4Len = 0;

		char buf0_2[arg4Len/2];
		char buf1_2[(arg4Off - arg3Off)/2];

		if (arg4Len)
		{
			//Get args for second line
			ReadHexPartial(lines, lineEnd + 2, (arg3Off) - (lineEnd + 2), addrBuf, ((arg3Off) - (lineEnd + 2))/2);
			arg0_2 = (uint)(INT32(addrBuf));

			//Get address for second line
			ReadHexPartial(lines, arg3Off + 1, arg4Off - arg3Off - 1, buf0_2, (arg4Off - arg3Off - 1)/2);

			//Get value for second line
			ReadHexPartial(lines, arg4Off + 1, arg4Len - 1, buf1_2, (arg4Len - 1)/2);
		}

		int startPos = lineEnd - arg2Len + 1;

		lines[lineEnd] = '\0';
		add_log(lines + start); // write line to log file
		lines[lineEnd] = '\n';

		char cType = lines[start];
		switch (cType)
		{
			case '0': ; //Write bytes (1=OR,2=AND,3=XOR,rest=write)
				ReadHex(lines, startPos, buf0size, buf0, 4);

				//Get source bytes
				if(arg0) get_process_mem(pid, addr, buf1, buf0Len);

				switch (arg0)
				{
					case 1: //OR
						for (int cnt0 = 0; cnt0 < buf0Len; cnt0++)
							buf0[cnt0] |= buf1[cnt0];
						break;
					case 2: //AND
						for (int cnt0 = 0; cnt0 < buf0Len; cnt0++)
							buf0[cnt0] &= buf1[cnt0];
						break;
					case 3: //XOR
						for (int cnt0 = 0; cnt0 < buf0Len; cnt0++)
							buf0[cnt0] ^= buf1[cnt0];
						break;
				}

				//Write bytes to dest
				WriteMem(pid, addr, buf0, buf0Len);
				break;
			case '1': //Write text
				//Get total text write size
				arg1Off++;
				while (lines[start + arg1Off] != ' ')
					arg1Off++;

				arg2Len = lineLen - arg1Off;
				memcpy64(buf0, lines + startPos, buf0size);
				buf0[buf0size] = '\0';
				WriteMem(pid, addr, buf0, buf0size);
				break;
			case '2': //Write float
				strncpy(buf0, lines + startPos, buf0size);
				float buf2Flt = (float)tofloat(buf0);
				WriteMem(pid, addr, (char*)&buf2Flt, buf0size);
				break;
			case '4': ; //Write condensed
				//Get count
				uint count = (uint)(INT32(buf1_2));

				//Get increment
				u64 inc = (u64)(INT32(buf0_2));

				//Get write
				ReadHex(lines, startPos, buf0size, buf0, buf0Len);

				for (uint cnt4 = 0; cnt4 < count; cnt4++)
				{
					WriteMem(pid, addr + (u64)(cnt4 * inc), buf0, buf0Len);
					if (arg0_2)
						*(uint*)buf0 += (arg0_2 << ((buf0Len % 4) * 8));
				}

				skip[0]++;
				break;
			case '6': //Write pointer

				//Get offset
				ReadHexPartial(lines, startPos, buf0size, buf0, buf0Len);
				u64 offset = (u64)(INT32(buf0));

				//Get address at pointer
				get_process_mem(pid, addr, buf0, 4);
				ptrAddr = (u64)(INT32(buf0));

				ptrAddr += offset;

				break;
			case 'A': //Copy paste

				switch (arg0)
				{
					case 1:
						//Get count
						ReadHexPartial(lines, startPos, buf0size, buf0, buf0Len);
						uint count = (uint)(INT32(buf0));

						if(count > _64KB_) {typeA_Size = 0; break;} // disable paste if copy is too large

						if(!typeA_Copy)
							typeA_Copy = sys_mem_allocate(_64KB_);

						if(typeA_Copy)
						{
							typeA_Size = count;
							get_process_mem(pid, addr, (char *)typeA_Copy, 4);
						}
						else
						{
							typeA_Size = 0;
							typeA_Copy = NULL;
						}
						break;
					case 2:
						if (!typeA_Copy || typeA_Size <= 0)
							break;

						WriteMem(pid, addr, (char *)typeA_Copy, typeA_Size);
						break;
				}

				break;
			case 'B': //Find Replace
				//Only work when doForceWrite is true (1) which means everytime the user activates Artemis from the in game XMB
				//Don't want to waste time constantly searching

				if (!doForceWrite)
					break;

				//Get end addr
				ReadHex(lines, startPos, buf0size, addrBuf, 4);
				u64 endAddr = (u64)(INT32(addrBuf));

				//new (COP) length
				uint binc = arg4Len/2;

				//original (OGP) length
				uint cmpSize = (arg4Off - arg3Off)/2;

				//Flip addresses
				u64 temp = 0;
				if (endAddr < addr) { temp = addr; addr = endAddr; endAddr = temp; }

				const size_t scanInc = _64KB_;
				sys_addr_t sysmem = sys_mem_allocate(scanInc);

				if(sysmem)
				{
					for (u64 curAddr = addr; curAddr < endAddr; curAddr += (scanInc - cmpSize))
					{
						if (get_process_mem(pid, curAddr, (char *)sysmem, scanInc) >= 0)
						{
							//So we stop it each loop before (scanInc - cmpSize) in the instance that
							//the result is the last 2 bytes, for instance, and the compare is actually 4 bytes (so it won't result even though it should)
							//This fixes that
							for (u64 boff = 0; boff < (scanInc - cmpSize); boff++)
							{
								//Break if count reached
								if (arg0 > 0 && temp >= (u64)arg0)
									break;
								if ((curAddr + boff) >= endAddr)
									break;

								if (CompareMemoryBuffered((char *)sysmem, boff, buf0_2, cmpSize))
								{
									//printf ("Instance found at 0x%08x, writing 0x%i (%d)\n", curAddr + boff, *(uint*)buf1_2, binc);
									WriteMem(pid, curAddr + boff, buf1_2, binc);
									//Just skip in case the replacement has, within itself, the ogp
									//We subtract one because it gets added back at the top of the loop
									boff += binc - 1;
									temp++;
								}
							}
						}
					}
					sys_memory_free(sysmem);
				}

				skip[0]++;
				break;
			case 'D': //Write conditional
				ReadHex(lines, startPos, buf0size, buf0, 4);
				int DisCond = CompareMemory(pid, addr, buf0, buf0Len);

				if (!DisCond)
				{
					skip[0] += arg0;
				}

				break;
			case 'E': //Write conditional (bitwise)
				ReadHex(lines, startPos, buf0size, buf0, 4);
				int EisCond = CompareMemoryAnd(pid, addr, buf0, buf0Len);

				if (!EisCond)
				{
					skip[0] += arg0;
				}

				break;
			case 'F': //Copy bytes

				//Get destination
				ReadHex(lines, startPos, buf0size, buf0, 4);
				u64 dest = (u64)(INT32(buf0));

				//Get source bytes
				get_process_mem(pid, addr, buf1, arg0);
				//Write bytes to dest
				WriteMem(pid, dest, buf1, arg0);

				break;

		}
	}
	else
		skip[0]--;

	return lineEnd;
}

// Loops through each line of the code list, interprets the codes & writes to process id
static void ConvertCodes(process_id_t pid, char *userCodes)
{
	char lineBuf[100];

	int lineNum = 0, codeNum = 0, lineCharInd = 0;
	int skip[1]; skip[0] = 0;

	if(file_size(ARTEMIS_CODES_LOG) >= _2MB_)
		create_file(ARTEMIS_CODES_LOG); // truncate log file larger than 2MB

	for (int i = 0; i < userCodesLen; i++)
	{
		if (userCodes[i] != '#')
		{
			if (userCodes[i] == '\n')
			{
				lineNum++;
				lineCharInd = -1;
			}
			else
			{
				if (lineCharInd == 0)
				{
					int n;
					for (n = i; n < userCodesLen; n++)
					{
						if (userCodes[n] == '\n' || userCodes[n] == '\r')
							break;
					}
					n -= i; // line length
					if(n > 100) n = sizeof(lineBuf);
					memcpy64(lineBuf, userCodes + i, n);
					lineBuf[n] = 0;
				}

				switch (lineNum)
				{
					case 1: //cWrite
						if (*lineBuf == 0)
							lineNum--;
						else
						{
							if (lineCharInd == 0)
								isConstantWrite = (userCodes[i] == '1' || userCodes[i] == 'T');

							if (!isConstantWrite && !doForceWrite) //skip this code if not constant or force write
							{
								while ((i < userCodesLen) && (userCodes[i] != '#'))
									i++;
								i--;
							}
						}
						break;
					case 0: //Name
						break;

					default: //codes

						if (lineNum > 1 && isCodeLineValid(lineBuf))
						{
							if(!(isConstantWrite || doForceWrite))
								skip[0]++;
							i = ParseLine(pid, userCodes, i, skip, doForceWrite);
						}
						break;
				}
			}
			lineCharInd++;
		}
		else // skip # line
		{
			lineNum = -1;
			isConstantWrite = 0;
			skip[0] = 0;
			codeNum++;
			lineCharInd = 0;

			while ((i < userCodesLen) && (userCodes[i] == '\n'))
				i++;
		}
	}
}

// Release memory buffers, reset codelist & attached PID
static void release_art(bool init)
{
	if(init)
		userCodesLen = attachedPID = 0;
	if(sysmem_art)
		{sys_memory_free(sysmem_art); sysmem_art = NULL;}
	if(typeA_Copy)
		{sys_memory_free(typeA_Copy); typeA_Copy = NULL;}
	userCodes = NULL;
	art_cmd = 0;
}

// Processes an entire codelist once
static void art_process(int forceWrite)
{
	if (attachedPID && working & artemis_working)
	{
		doForceWrite = forceWrite;

		if (!userCodes || forceWrite)
		{
			const char *CODES_LIST[3] = { ARTEMIS_CODES_FILE, ARTEMIS_CODES_L2, ARTEMIS_CODES_R2 };
			const char *CODES_FILE = BETWEEN(1, forceWrite, 3) ? CODES_LIST[forceWrite - 1] : ARTEMIS_CODES_FILE;

			if(not_exists(CODES_FILE)) CODES_FILE = ARTEMIS_CODES_FILE;

			release_art(0);

			int mem_size = _64KB_;

			userCodesLen = file_size(CODES_FILE);

			if(userCodesLen)
			{
				mem_size = _64KB_ + (int)(userCodesLen / _64KB_) * _64KB_;

				if((userCodesLen + 1) < mem_size)
					sysmem_art = sys_mem_allocate(mem_size);
			}

			if(sysmem_art)
			{
				userCodes = (char *)sysmem_art;

				sprintf(userCodes, "Attached PID=0x%x\n%s\n", attachedPID, CODES_FILE);
				show_msg(userCodes);
				add_log(userCodes);

				userCodesLen = read_file(CODES_FILE, userCodes, mem_size, 0);
				if(userCodesLen)
				{
					userCodes[userCodesLen] = '\n'; // append line break
					userCodes[++userCodesLen] = '\0';
				}
				else
					release_art(1);
			}

			if(!sysmem_art)
			{
				show_error("Artemis PS3\nFailed to Attach");
				release_art(1);
			}
		}

		if (userCodes && (attachedPID == GetGameProcessID()))
		{
			ConvertCodes(attachedPID, userCodes);
			sys_timer_usleep(100000);
		}
	}

	doForceWrite = 0;
}

static void clear_codelist(void)
{
	if(webman_config->artemis)
	{
		cellFsUnlink(ARTEMIS_CODES_FILE);
	}
	cellFsUnlink(ARTEMIS_CODES_LOG);
	cellFsUnlink(ARTEMIS_CODES_L2);
	cellFsUnlink(ARTEMIS_CODES_R2);
}

static void init_codelist(char *last_path)
{
	if(*last_path == '/')
	{
		char *ext = strrchr(last_path, '.');
		if(ext)
			strcpy(ext, ".ncl"); // replace file extension
		else
			strcat(last_path, ".ncl"); // append file extension

		size_t size = file_size(last_path);
		if(BETWEEN(1, size, _1MB_))
		{
			clear_codelist();
			force_copy(last_path, (char*)ARTEMIS_CODES_FILE);
		}

		ext = strstr(last_path, ".ncl");
		if(ext)
		{
			strcpy(ext, "_l2.ncl"); force_copy(last_path, (char*)ARTEMIS_CODES_L2);
			strcpy(ext, "_r2.ncl"); force_copy(last_path, (char*)ARTEMIS_CODES_R2);
			strcpy(ext,    ".ncl");
		}
	}
}

static bool art_wait(int n)
{
	for (int i = 0; i < n; i++)
	{
		if(!working || !artemis_working) return true;
		sys_timer_usleep(100000); // 1/10 second
		sys_ppu_thread_yield();
	}
	return false;
}

// Read user input and calls art_process()
static void art_thread(u64 arg)
{
	artemis_working = 1;

	init_log("Artemis started\n");

	int GameProcessID = 0, lastGameProcessID = 0;

	art_wait(100); // 10.0 secs

	CellPadData data;
	CellPadInfo2 info;

	while (working && artemis_working)
	{
		GameProcessID = GetGameProcessID();

		if (GameProcessID)
		{
			if (GameProcessID != lastGameProcessID)
			{
				if(art_wait(50)) break; //5.0 seconds delay
				if(!art_cmd) show_msg("Artemis PS3\nStart To Attach");
			}

			cellPadGetInfo2(&info);
			if ((info.port_status[0] && (cellPadGetData(0, &data) | 1) && data.len > 0) || art_cmd)
			{
				u32 pad = data.button[2] | (data.button[3] << 8);

				if (attachedPID) // Run codes
				{
					art_process(0);
				}

				if ((pad & PAD_START) || (art_cmd == 1))
				{
					release_art(0);

					attachedPID = GameProcessID;

					if (attachedPID)
					{
						char line[32];
						sprintf(line, "Attached PID=0x%x\n", attachedPID);
						init_log(line);

						if(pad & PAD_R2) art_process(3); else
						if(pad & PAD_L2) art_process(2); else
										 art_process(1);
					}
					else
					{
						show_error("Artemis PS3\nFailed to Attach");
						release_art(1);
					}

					// wait for release START button
					while ((cellPadGetData(0, &data) | 1) && data.len > 0)
					{
						if (!((data.button[2] | (data.button[3] << 8)) & PAD_START))
							break;

						if(art_wait(10)) break; // 1.0 second
					}
				}
				else if ((pad & PAD_SELECT) || (art_cmd == 2))
				{
					if (attachedPID)
						show_msg("Artemis PS3\nDetached");

					add_log("Detached PID");
					release_art(1);
					if(art_wait(10)) break; // 1.0 second
				}
			}

			if(!working || !artemis_working || art_wait(1)) break; //0.1 second delay
		}
		else break; // exit thread on XMB

		lastGameProcessID = GameProcessID;

		if(art_wait(2)) break; //0.2 second delay
	}


	release_art(1);
	artemis_working = 0;
	sys_ppu_thread_exit(0);
}

static void start_artemis(void)
{
	if(!artemis_working && IS_INGAME)
		sys_ppu_thread_create(&thread_id_art, art_thread, NULL, THREAD_PRIO_ARTEMIS, THREAD_STACK_SIZE_ARTEMIS, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_ART);
}
#endif // #ifdef ARTEMIS_PRX
