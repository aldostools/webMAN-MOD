#ifdef PS3MAPI

static int ps3mapi_create_process_thread(sys_pid_t pid, sys_ppu_thread_t* thread, void* entry, uint64_t arg, int prio, size_t stacksize, const char* threadname)
{
	system_call_8(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PROC_CREATE_THREAD, (uint64_t)(uint32_t)pid, (uint64_t)(uint32_t)thread, (uint64_t)(uint32_t)entry, (uint64_t)(uint32_t)arg, (uint64_t)(uint32_t)prio, (uint64_t)(uint32_t)stacksize, (uint64_t)(uint32_t)threadname);
	return_to_user_prog(int);
}
static int ps3mapi_process_page_allocate(sys_pid_t pid, uint64_t size, uint64_t page_size, uint64_t flags, uint64_t is_executable, uint64_t* page_table)
{
	system_call_8(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PROC_PAGE_ALLOCATE, (uint64_t)(uint32_t)pid, (uint64_t)(uint32_t)size, (uint64_t)(uint32_t)page_size, (uint64_t)(uint32_t)flags, (uint64_t)(uint32_t)is_executable, (uint64_t)(uint32_t)page_table);
	return_to_user_prog(int);
}

static int ps3mapi_process_page_free(sys_pid_t pid, uint64_t flags, uint64_t* page_table)
{
	system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PROC_PAGE_FREE, (uint64_t)(uint32_t)pid, (uint64_t)(uint32_t)flags, (uint64_t)(uint32_t)page_table);
	return_to_user_prog(int);
}

static int ps3mapi_get_proc_memory(sys_pid_t pid, void* destination, void* source, size_t size)
{
	system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MEM, (uint64_t)(uint32_t)pid, (uint64_t)(uint32_t)destination, (uint64_t)(uint32_t)source, size);
	return_to_user_prog(int);
}

static int ps3mapi_set_proc_memory(sys_pid_t pid, void* destination, const void* source, size_t size)
{
	system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PROC_MEM, (uint64_t)(uint32_t)pid, (uint64_t)(uint32_t)destination, (uint64_t)(uint32_t)source, (uint64_t)(uint32_t)size);
	return_to_user_prog(int);
}

static int WritePayload(sys_pid_t pid, const char* fileName, uintptr_t startAddr)
{
	const int BYTES_PER_WRITE = _64KB_;
	int next_write = startAddr;
	
	int fd = 0; uint64_t read_amount = 1;

	if (cellFsOpen(fileName, CELL_FS_O_RDONLY, &fd, NULL, 0) ==	CELL_FS_SUCCEEDED)
	{
		sys_addr_t sysmem = sys_mem_allocate(BYTES_PER_WRITE);
		if(!sysmem) {cellFsClose(fd); return FAILED;}

		char *data = (char*)sysmem;
		while (cellFsRead(fd, (void*)(data), BYTES_PER_WRITE, &read_amount) == CELL_FS_SUCCEEDED && (read_amount > 0))
		{
			ps3mapi_set_proc_memory(pid, (void*)(next_write), data, (size_t)read_amount);
			next_write += read_amount;
		}
		cellFsClose(fd);
		sys_memory_free(sysmem);
	}
	else
	{
		return CELL_FS_ENOENT;
	}

	return SUCCEEDED;
}

// StartGamePayload(GetGameProcessID(), "/dev_hdd0/payload.bin", 0x7D0, 0x4000, outPageTable, error_msg);

static uint64_t StartGamePayload(int pid, const char* fileName, int prio, size_t stacksize, uint64_t outPageTable[2], char *error_msg)
{
	// Starting to inject payload fileName
	*error_msg = NULL;

	uint64_t fileSizeOnDisk = file_size(fileName);
	if (fileSizeOnDisk < 8)
	{
		// Invalid payload size
		strcpy(error_msg, "Invalid payload size");
		show_error(error_msg);
		return 0;
	}

	// I need to find a way to get file size on disk; see https://imgur.com/a/bSFHbGT --- attempt here: https://github.com/TheRouletteBoi/RouLetteVshMenu/blob/939f0c3fc5cb38c96d356e94116429f0c0a2472b/src/Core/Menu/Submenus.cpp#L296-L350
	fileSizeOnDisk = (fileSizeOnDisk + _4KB_) & 0xFFFF000;

	int ret = ps3mapi_process_page_allocate(pid, fileSizeOnDisk, 0x100, 0x2F, 0x1, outPageTable);
	if (ret)
	{
		// Failed to allocate executable memory
		strcpy(error_msg, "Failed to allocate executable memory");
		show_error(error_msg);
		return 0;
	}

	sys_ppu_thread_sleep(1);

	// outPageTable[0] is the userland page memory address 
	// outPageTable[1] is the kernel page memory address
	uint64_t executableMemoryAddress = outPageTable[0];

	uint32_t temp_bytes = 0;
	ret = ps3mapi_get_proc_memory(pid, (void*)(uintptr_t)executableMemoryAddress, (void*)&temp_bytes, 4);
	if (ret)
	{
		// Failed to read executable memory
		strcpy(error_msg, "Failed to read executable memory");
		show_error(error_msg);
		ps3mapi_process_page_free(pid, 0x2F, outPageTable);
		return 0;
	}

	sys_ppu_thread_sleep(1);

	ret = WritePayload(pid, fileName, (uintptr_t)executableMemoryAddress);
	if (ret)
	{
		// Failedto read payload fileName
		strcpy(error_msg, "Failed to read payload file");
		show_error(error_msg);
		ps3mapi_process_page_free(pid, 0x2F, outPageTable);
		return 0;
	}

	sys_ppu_thread_sleep(1);

	uint64_t threadOpd[2];
	threadOpd[0] = executableMemoryAddress;
	threadOpd[1] = 0x00000000;
	sys_ppu_thread_t th;
	if(stacksize < 0x1000) stacksize = 0x1000;
	ret = ps3mapi_create_process_thread(pid, &th, threadOpd, 0, prio, stacksize, "PayloadThread");
	if (ret)
	{
		// Failed to start payload
		strcpy(error_msg, "Failed to start payload");
		show_error(error_msg);
		ps3mapi_process_page_free(pid, 0x2F, outPageTable);
		return 0;
	}

	// Successfully started payload
	sprintf(error_msg, "Started payload @ 0x%llX - 0x%llX bytes allocated", executableMemoryAddress, fileSizeOnDisk);
	return executableMemoryAddress;
}

#endif // #ifdef PS3MAPI
