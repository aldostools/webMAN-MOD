#define SC_GET_PRX_MODULE_BY_ADDRESS	(461)
#define SC_STOP_PRX_MODULE 				(482)
#define SC_UNLOAD_PRX_MODULE 			(483)
#define SC_PPU_THREAD_EXIT				(41)

static inline sys_prx_id_t prx_get_module_id_by_address(void *addr)
{
	system_call_1(SC_GET_PRX_MODULE_BY_ADDRESS, (u64)(u32)addr);
	return (int)p1;
}

static void unload_prx_module(void)
{
	sys_prx_id_t prx = prx_get_module_id_by_address(unload_prx_module);
	{system_call_3(SC_UNLOAD_PRX_MODULE, prx, 0, NULL);}
}

static void stop_prx_module(void)
{
	show_msg((char*)STR_WMUNL);

#ifdef REMOVE_SYSCALLS
	remove_cfw_syscall8(); // remove cobra if syscalls were disabled
#endif

	working = 0;

	sys_prx_id_t prx = prx_get_module_id_by_address(stop_prx_module);

	// int *result = NULL;
	// {system_call_6(SC_STOP_PRX_MODULE, (u64)(u32)prx, 0, NULL, (u64)(u32)result, 0, NULL);}

	u64 meminfo[5];

	meminfo[0] = 0x28;
	meminfo[1] = 2;
	meminfo[3] = 0;

	{system_call_3(SC_STOP_PRX_MODULE, prx, 0, (u64)(u32)meminfo);}
}

static inline void _sys_ppu_thread_exit(u64 val)
{
	system_call_1(SC_PPU_THREAD_EXIT, val); // prxloader = mandatory; cobra = optional; ccapi = don't use !!!
}
