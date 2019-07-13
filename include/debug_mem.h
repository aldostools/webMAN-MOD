#ifdef GET_KLICENSEE

static char *hex_dump(char *buffer, int offset, int size)
{
	for (int k = 0; k < size ; k++)
	{
		sprintf(&buffer[2 * k],"%02X", (unsigned int)(((unsigned char*)offset)[k]));
	}
	return buffer;
}

#endif

#ifdef DEBUG_MEM

#define LV1_UPPER_MEMORY	0x8000000010000000ULL
#define LV2_UPPER_MEMORY	0x8000000000800000ULL

static void peek_chunk(u64 start, u64 size, u8 *buffer) // read from lv1
{
	for(u64 t, i = 0; i < size; i += 8)
	{
		t = peek_lv1(start + i); memcpy(buffer + i, &t, 8);
	}
}

static void dump_mem(char *file, u64 start, u32 dump_size)
{
	{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

	int fd;
	u32 mem_size = _128KB_, addr;
	sys_addr_t sys_mem = NULL;

	if(start < 0x0000028080000000ULL) start |= 0x8000000000000000ULL;

	if(sys_memory_allocate(mem_size, SYS_MEMORY_PAGE_SIZE_64K, &sys_mem) == CELL_OK)
	{
		u8 *mem_buf = (u8*)sys_mem;

		if(cellFsOpen(file, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
		{
			for(addr = 0; addr < dump_size; addr += mem_size)
			{
				peek_chunk(start + addr, mem_size, mem_buf);
				cellFsWrite(fd, mem_buf, mem_size, NULL);
			}
			cellFsClose(fd);
		}
		sys_memory_free(sys_mem);
		show_msg((char*)"Memory dump completed!");
	}

	{ BEEP2 }

	{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }
}

static void ps3mapi_mem_dump(char *buffer, char *templn, char *param)
{
	char dump_file[MAX_PATH_LEN]; u64 start=0; u32 size=8;
	strcat(buffer, "Dump: [<a href=\"/dump.ps3?mem\">Full Memory</a>] [<a href=\"/dump.ps3?lv1\">LV1</a>] [<a href=\"/dump.ps3?lv2\">LV2</a>]<hr>");

	if(param[9] == '?' && param[10] >= '0')
	{
		if(param[10] == 'l' && param[11] == 'v' && param[12] == '1') {size=16;} else
		if(param[10] == 'l' && param[11] == 'v' && param[12] == '2') {start=LV2_OFFSET_ON_LV1;} else
		//if(strstr(param,"?v") /*vsh  */) {start=0x910000;}  else
		if(param[10] == 'r' /*rsx  */) {start=0x0000028080000000ULL; size=256;}  else
		if(param[10] == 'f' /*full */) {size=(dex_mode==1) ? 512 : 256;} else
		if(param[10] == 'm' /*mem  */) {size=(dex_mode==1) ? 512 : 256;} else
		{
			start = convertH(param + 10);
			if(start >= LV1_UPPER_MEMORY - ((u64)(size * _1MB_))) start = LV1_UPPER_MEMORY - ((u64)(size * _1MB_));
		}

		char *pos = strstr(param, "&size=");
		if(pos) size = convertH(pos + 6);

		sprintf(dump_file, "/dev_hdd0/dump_%s.bin", param + 10);
		dump_mem(dump_file, start, (size * _1MB_));
		sprintf(templn, "<p>Dumped: " HTML_URL " [" HTML_URL2 "]", dump_file, dump_file+10, "/delete.ps3", dump_file, STR_DELETE); strcat(buffer, templn);
	}
}

static void ps3mapi_find_peek_poke(char *buffer, char *templn, char *param)
{
	u64 address, addr, byte_addr, fvalue, value=0, upper_memory, found_address=0, step = 1;
	u8 byte = 0, p = 0, lv1 = 0;
	bool bits8 = false, bits16 = false, bits32 = false, found = false;
	u8 flen = 0;
	char *v;

	v = strstr(param + 10, "&");
	if(v) *v = NULL;

	address = convertH(param + 10);

	v = strstr(param + 10, "=");
	if(v)
	{
		flen = strlen(v + 1);
		for(p = 1; p <= flen; p++) if(!memcmp(v + p, " ", 1)) byte++; //ignore spaces
		flen -= byte; byte = p = 0;
	}

	bits32 = (flen >4) && (flen<=8);
	bits16 = (flen >2) && (flen<=4);
	bits8  = (flen<=2);

	buffer += concat(buffer, "<pre>");

	address|=0x8000000000000000ULL;

	lv1 = strstr(param,".lv1?") ? 1 : 0;
#ifdef COBRA_ONLY
	if(lv1) { system_call_1(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_DISABLE_COBRA); }
#endif
	if(islike(param, "/find.lv"))
	{
		if(strstr(param,"#")) {step = 4, address &= 0x80000000FFFFFFFCULL;} // find using aligned memory address (4X faster) e.g. /find.lv2?3000=3940ffff#
		upper_memory = (lv1 ? LV1_UPPER_MEMORY : LV2_UPPER_MEMORY) - 8;
	}
	else if(strstr(param, "#"))
		upper_memory = 0x8FFFFFFFFFFFFFF8ULL; // use # to peek/poke any memory address
	else
		upper_memory = (lv1 ? LV1_UPPER_MEMORY : LV2_UPPER_MEMORY) - 8; // safe memory adddress

	if(address > upper_memory) address = upper_memory - 0x200;

	if((v == NULL) || (address > upper_memory)) { /* ignore find/poke if value is not provided or invalid address */ }
	else
	if(islike(param, "/find.lv"))
	{
		fvalue = convertH(v + 1);

		if(bits8)  fvalue = (fvalue << 56);
		if(bits16) fvalue = (fvalue << 48);
		if(bits32) fvalue = (fvalue << 32);

		u64 (*peek_mem)(u64) = lv1 ? peek_lv1 : peekq;

		for(addr = address; addr < upper_memory; addr += step)
		{
			value = peek_mem(addr);

			if(bits32) value &= 0xffffffff00000000ULL; else
			if(bits16) value &= 0xffff000000000000ULL; else
			if(bits8 ) value &= 0xff00000000000000ULL;

			if(value == fvalue) {found = true; break;}
		}

		if(!found)
		{
			sprintf(templn, "<b><font color=red>%s</font></b><br>", STR_NOTFOUND); buffer += concat(buffer, templn);
		}
		else
		{
			found_address = address = addr;
			sprintf(templn, "Offset: 0x%08X<br><br>", (u32)address); buffer += concat(buffer, templn);
		}
	}
	else
	if(islike(param, "/poke.lv"))
	{
		value  = convertH(v + 1);
		fvalue = lv1 ? peek_lv1(address) : peekq(address);

		if(bits32) value = ((u64)(value << 32) | (u64)(fvalue & 0xffffffffULL));      else
		if(bits16) value = ((u64)(value << 48) | (u64)(fvalue & 0xffffffffffffULL));  else
		if(bits8)  value = ((u64)(value << 56) | (u64)(fvalue & 0xffffffffffffffULL));

		if(lv1)
			poke_lv1(address, value);
		else
			pokeq(address, value);

		found_address = address; found = true;
	}

	////////////////////////////////
	// show memory address in hex //
	////////////////////////////////

	if(address + 0x200 > (upper_memory + 8)) address = 0;

	flen = (bits8) ? 1 : (bits16) ? 2 : (bits32) ? 4 : 8;
	address &= 0xFFFFFFFFFFFFFFF0ULL;
	addr = address;

#ifdef COBRA_ONLY
	if(lv1) { system_call_1(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_DISABLE_COBRA); }
#endif
	for(u16 i = 0; i < 0x200; i++)
	{
		if(!p)
		{
			sprintf(templn, "%08X  ", (int)((address & 0xFFFFFFFFULL) + i));
			buffer += concat(buffer, templn);
		}

		byte_addr = address + i;
		byte = (u8)((lv1 ? peek_lv1(byte_addr) : peekq(byte_addr)) >> 56);

		if(found && byte_addr >= found_address && byte_addr < (found_address + flen)) buffer += concat(buffer, "<font color=yellow><b>");
		sprintf(templn, "%02X ", byte); buffer += concat(buffer, templn);
		if(found && byte_addr >= found_address && byte_addr < (found_address + flen)) buffer += concat(buffer, "</b></font>");

		if(p == 0x7) buffer += concat(buffer, " ");

		if(p == 0xF)
		{
			buffer += concat(buffer, " ");
			for(u8 c = 0; c < 0x10; c++, addr++)
			{
				byte = (u8)((lv1 ? peek_lv1(addr) : peekq(addr)) >> 56);
				if(byte<32 || byte>=127) byte='.';

				if(found && addr >= found_address && addr < (found_address + flen)) buffer += concat(buffer, "<font color=yellow><b>");
				if(byte==0x3C)
					buffer += concat(buffer, "&lt;");
				else if(byte==0x3E)
					buffer += concat(buffer, "&gt;");
				else
					{sprintf(templn,"%c", byte); buffer += concat(buffer, templn);}

				if(found && addr >= found_address && addr < (found_address + flen)) buffer += concat(buffer, "</b></font>");
			}
			buffer += concat(buffer, "<br>");
		}

		p++; if(p>=0x10) p=0;
	}

#ifdef COBRA_ONLY
	if(lv1) { system_call_1(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_ENABLE_COBRA); }
#endif
	// footer

	buffer += concat(buffer, "<hr>Dump: [<a href=\"/dump.ps3?mem\">Full Memory</a>] [<a href=\"/dump.ps3?lv1\">LV1</a>] [<a href=\"/dump.ps3?lv2\">LV2</a>]");
	sprintf(templn, " [<a href=\"/dump.ps3?%llx\">Dump 0x%llx</a>]", lv1?address:address + LV2_OFFSET_ON_LV1, lv1?address:address + LV2_OFFSET_ON_LV1); buffer += concat(buffer, templn);
	sprintf(templn, " <a id=\"pblk\" href=\"/peek.lv%i?%llx\">&lt;&lt;</a> <a id=\"back\" href=\"/peek.lv%i?%llx\">&lt;Back</a>", lv1?1:2, ((int)(address-0x1000)>=0)?(address-0x1000):0, lv1?1:2, ((int)(address-0x200)>=0)?(address-0x200):0); buffer += concat(buffer, templn);
	sprintf(templn, " <a id=\"next\" href=\"/peek.lv%i?%llx\">Next&gt;</a> <a id=\"nblk\" href=\"/peek.lv%i?%llx\">&gt;&gt;</a></pre>", lv1?1:2, ((int)(address+0x400)<(int)upper_memory)?(address+0x200):(upper_memory-0x200), lv1?1:2, ((int)(lv1+0x1200)<(int)upper_memory)?(address+0x1000):(upper_memory-0x200)); buffer += concat(buffer, templn);

	// add navigation with left/right keys
	concat(buffer,  "<script>"
					"document.addEventListener('keydown',kd,false);"
					"function kd(e)"
					"{e=e||window.event;var kc=e.keyCode;if(kc==37){self.location=e.ctrlKey?pblk.href:back.href;}if(kc==39){self.location=e.ctrlKey?nblk.href:next.href;}}"
					"</script>");
}

#endif

