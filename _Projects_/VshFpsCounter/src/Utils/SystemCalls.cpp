#include "SystemCalls.hpp"

int sys_map_path(char** paths, char** new_paths, int num)
{
   system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_MAP_PATHS, (uint64_t)(uint32_t)paths, (uint64_t)(uint32_t)new_paths, (uint64_t)num);
   return_to_user_prog(int);
}

int cobra_load_vsh_plugin(unsigned int slot, char* path, void* arg, uint32_t arg_size)
{
   system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_LOAD_VSH_PLUGIN, (uint64_t)slot, (uint64_t)(uint32_t)path, (uint64_t)arg, arg_size);
   return_to_user_prog(int);
}

int cobra_unload_vsh_plugin(unsigned int slot)
{
   system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_UNLOAD_VSH_PLUGIN, slot);
   return_to_user_prog(int);
}

int ps3mapi_get_core_version()
{
   system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_VERSION);
   return_to_user_prog(int);
}

int ps3mapi_get_core_minversion()
{
   system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_MINVERSION);
   return_to_user_prog(int);
}

int ps3mapi_get_fw_type(char* fw)
{
   system_call_3(SC_COBRA_SYSCALL8, (uint64_t)SYSCALL8_OPCODE_PS3MAPI, (uint64_t)PS3MAPI_OPCODE_GET_FW_TYPE, (uint64_t)((uint32_t)fw));
   return_to_user_prog(int);
}

int ps3mapi_get_fw_version()
{
   system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_FW_VERSION);
   return_to_user_prog(int);
}

int ps3mapi_get_all_process_modules_prx_id(sys_pid_t pid, sys_prx_id_t* prx_id_list)
{
   system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_MODULE_PID, (uint64_t)pid, (uint64_t)prx_id_list);
   return_to_user_prog(int);
}

int ps3mapi_get_process_module_name_by_prx_id(sys_pid_t pid, sys_prx_id_t prx_id, char* name)
{
   system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_NAME, (uint64_t)pid, (uint64_t)prx_id, (uint64_t)name);
   return_to_user_prog(int);
}

int ps3mapi_get_process_module_filename_by_prx_id(sys_pid_t pid, sys_prx_id_t prx_id, char* filename)
{
   system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_FILENAME, (uint64_t)pid, (uint64_t)prx_id, (uint64_t)filename);
   return_to_user_prog(int);
}

int ps3mapi_load_process_modules(sys_pid_t pid, char* path, void* arg, uint32_t arg_size)
{
   system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LOAD_PROC_MODULE, (uint64_t)pid, (uint64_t)((uint32_t)path), (uint64_t)((uint32_t)arg), (uint64_t)arg_size);
   return_to_user_prog(int);
}

int ps3mapi_unload_process_modules(sys_pid_t pid, sys_prx_id_t prx_id)
{
   system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_UNLOAD_PROC_MODULE, (uint64_t)pid, (uint64_t)prx_id);
   return_to_user_prog(int);
}

int ps3mapi_unload_vsh_plugin(char* name)
{
   system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_UNLOAD_VSH_PLUGIN, (uint64_t)((uint32_t)name));
   return_to_user_prog(int);
}

int ps3mapi_get_vsh_plugin_info(unsigned int slot, char* name, char* filename)
{
   system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO, (uint64_t)slot, (uint64_t)((uint32_t)name), (uint64_t)((uint32_t)filename));
   return_to_user_prog(int);
}

int ps3mapi_load_kernel_plugin(uint8_t* payload, int size)
{
   system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_RUN_PAYLOAD, (uint64_t)payload, size);
   return_to_user_prog(uint32_t);
}

int ps3mapi_load_dynamic_kernel_plugin(uint8_t* payload, int size, uint64_t* residence)
{
   system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_RUN_PAYLOAD_DYNAMIC, (uint64_t)payload, size, (uint64_t)residence);
   return_to_user_prog(uint32_t);
}

int ps3mapi_unload_dynamic_kernel_plugin(uint64_t residence)
{
   system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_UNLOAD_PAYLOAD_DYNAMIC, residence);
   return_to_user_prog(uint32_t);
}

int ps3mapi_get_is_hen()
{
   system_call_1(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_IS_HEN);
   return_to_user_prog(int);
}

int ps3mapi_get_hen_rev()
{
   system_call_1(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_HEN_REV);
   return_to_user_prog(int);
}

int ps3mapi_poke_hen(uint64_t address, uint64_t value)
{
   system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_POKE_LV2, address, value);
   return_to_user_prog(int);
}

int ps3mapi_process_page_allocate(sys_pid_t pid, uint64_t size, uint64_t page_size, uint64_t flags, uint64_t is_executable, uint64_t* page_address)
{
   system_call_8(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PROC_PAGE_ALLOCATE, (uint64_t)pid, (uint64_t)size, (uint64_t)page_size, (uint64_t)flags, (uint64_t)is_executable, (uint64_t)page_address);
   return_to_user_prog(int);
}

int ps3mapi_get_process_module_segments(sys_pid_t pid, sys_prx_id_t prx_id, sys_prx_module_info_t* info)
{
   system_call_5(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_SEGMENTS, (uint64_t)pid, (uint64_t)prx_id, (uint64_t)info);
   return_to_user_prog(int);
}

int ps3mapi_create_process_thread(sys_pid_t pid, thread_t* thread, void* entry, uint64_t arg, int prio, size_t stacksize, const char* threadname)
{
   system_call_8(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PROC_CREATE_THREAD, (uint64_t)pid, (uint64_t)thread, (uint64_t)entry, (uint64_t)arg, (uint64_t)prio, (uint64_t)stacksize, (uint64_t)threadname);
   return_to_user_prog(int);
}

int ps3mapi_get_all_processes_pid(uint32_t* pid_list)
{
   system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (uint64_t)((uint32_t)pid_list));
   return_to_user_prog(int);
}

int ps3mapi_get_process_name_by_pid(uint32_t pid, char* pid_name)
{
   system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID, (uint64_t)((uint32_t)pid), (uint64_t)((uint32_t)pid_name));
   return_to_user_prog(int);
}

int ps3mapi_get_process_by_pid(uint32_t pid, process_t process)
{
   system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_BY_PID, (uint64_t)pid, (uint64_t)((uint32_t)process));
   return_to_user_prog(int);
}

int ps3mapi_get_current_process_critical(process_t process)
{
   system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CURRENT_PROC_CRIT, (uint64_t)((uint32_t)process));
   return_to_user_prog(int);
}

int ps3mapi_get_current_process(process_t process)
{
   system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CURRENT_PROC, (uint64_t)((uint32_t)process));
   return_to_user_prog(int);
}

int ps3mapi_check_syscall(int num)
{
   system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_CHECK_SYSCALL, (uint64_t)num);
   return_to_user_prog(int);
}

int ps3mapi_disable_syscall(int num)
{
   system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, (uint64_t)num);
   return_to_user_prog(int);
}

int ps3mapi_pdisable_syscall8(int mode)
{
   system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, (uint64_t)mode);
   return_to_user_prog(int);
}

int ps3mapi_pcheck_syscall8()
{
   system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PCHECK_SYSCALL8);
   return_to_user_prog(int);
}

int ps3mapi_get_idps(uint64_t* idps)
{
   system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_IDPS, (uint64_t)((uint32_t)idps));
   return_to_user_prog(int);
}

int ps3mapi_set_idps(uint64_t part1, uint64_t part2)
{
   system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_IDPS, (uint64_t)part1, (uint64_t)part2);
   return_to_user_prog(int);
}

int ps3mapi_get_psid(uint64_t* psid)
{
   system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PSID, (uint64_t)((uint32_t)psid));
   return_to_user_prog(int);
}

int ps3mapi_set_psid(uint64_t part1, uint64_t part2)
{
   system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PSID, (uint64_t)part1, (uint64_t)part2);
   return_to_user_prog(int);
}

int ps3mapi_remove_hook(void)
{
   system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_REMOVE_HOOK);
   return_to_user_prog(int);
}

int ps3mapi_enable_access_syscall8(uint64_t key)
{
   system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_REQUEST_ACCESS, key);
   return_to_user_prog(int);
}

int ps3mapi_disable_access_syscall8(uint64_t key)
{
   system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_ACCESS_KEY, key);
   return_to_user_prog(int);
}

int sys_get_mamba()
{
   system_call_1(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_GET_MAMBA);
   return_to_user_prog(int);
}

int sys_get_version(uint32_t* version)
{
   system_call_2(8, SYSCALL8_OPCODE_GET_VERSION, (uint64_t)(uint32_t)version);
   return_to_user_prog(int);
}

int sys_get_version2(uint16_t* version)
{
   system_call_2(8, SYSCALL8_OPCODE_GET_VERSION2, (uint64_t)(uint32_t)version);
   return_to_user_prog(int);
}

__attribute__((noinline)) uint64_t ps3mapi_lv1_peek(uint64_t addr)
{
   system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LV1_PEEK, (uint64_t)addr);
   return_to_user_prog(uint64_t);
}

__attribute__((noinline)) uint64_t ps3mapi_lv1_poke(uint64_t addr, uint64_t value)
{
   system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LV1_POKE, (uint64_t)addr, (uint64_t)value);
   return_to_user_prog(uint64_t);
}

__attribute__((noinline)) uint64_t ps3mapi_lv2_peek(uint64_t addr)
{
   system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LV2_PEEK, (uint64_t)addr);
   return_to_user_prog(uint64_t);
}

__attribute__((noinline)) uint64_t ps3mapi_lv2_poke(uint64_t addr, uint64_t value)
{
   system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LV2_POKE, (uint64_t)addr, (uint64_t)value);
   return_to_user_prog(uint64_t);
}

__attribute__((noinline)) int PS3MAPIGetMemory(sys_pid_t pid, void* destination, void* source, size_t size)
{
   system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MEM, (uint64_t)pid, (uint64_t)destination, (uint64_t)source, size);
   return_to_user_prog(int);
}

__attribute__((noinline)) int PS3MAPISetMemory(sys_pid_t pid, void* destination, const void* source, size_t size)
{
   system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PROC_MEM, (uint64_t)pid, (uint64_t)destination, (uint64_t)source, (uint64_t)size);
   return_to_user_prog(int);
}

__attribute__((noinline)) uint64_t PeekLv1(uint64_t addr)
{
   system_call_1(8, (uint64_t)addr);
   return_to_user_prog(uint64_t);
}

__attribute__((noinline)) void PokeLv1(uint64_t addr, uint64_t value)
{
   system_call_2(9, (uint64_t)addr, (uint64_t)value);
   return_to_user_prog(void);
}

__attribute__((noinline)) uint64_t PeekLv2(uint64_t address)
{
   system_call_1(6, (uint64_t)address);
   return_to_user_prog(uint64_t);
}

__attribute__((noinline)) void PokeLv2(uint64_t addr, uint64_t val)
{
   system_call_2(7, (uint64_t)addr, (uint64_t)val);
   return_to_user_prog(void);
}

uint8_t PeekUint8LV1(uint64_t addr)
{
    return (PeekLv1(addr) >> 56) & 0xFFUL;
}

uint32_t PeekUint32LV1(uint64_t addr)
{
    return (PeekLv1(addr) >> 32) & 0xFFFFFFFFUL;
}

bool PeekChunkLV1(uint64_t start, uint64_t* buffer, uint64_t size)
{
    if (PeekLv1(start) == 0xFFFFFFFF80010003)
        return false;

    for (uint64_t offset = 0; offset < size; offset += 8)
        *(buffer++) = PeekLv1(start + offset);

    return true;
}

bool PeekChunkLV2(uint64_t start, uint64_t* buffer, uint64_t size)
{
    if (PeekLv2(start) == 0xFFFFFFFF80010003)
        return false;

    for (uint64_t offset = 0; offset < size; offset += 8)
        *(buffer++) = PeekLv2(start + offset);

    return true;
}

__attribute__((noinline)) uint64_t CallLv1Function(uint64_t addr, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7)
{
   system_call_8(10, (uint64_t)arg1, (uint64_t)arg2, (uint64_t)arg3, (uint64_t)arg4, (uint64_t)arg5, (uint64_t)arg6, (uint64_t)arg7, (uint64_t)addr);
   return_to_user_prog(uint64_t);
}

__attribute__((noinline)) uint64_t CallLv2Function(uint64_t addr, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7)
{
   system_call_8(15, (uint64_t)arg1, (uint64_t)arg2, (uint64_t)arg3, (uint64_t)arg4, (uint64_t)arg5, (uint64_t)arg6, (uint64_t)arg7, (uint64_t)addr);
   return_to_user_prog(uint64_t);
}

int sys_dbg_read_process_memory(sys_pid_t pid, void* destination, void* source, size_t size)
{
   system_call_4(904, (uint64_t)pid, (uint64_t)destination, size, (uint64_t)source);
   return_to_user_prog(int);
}

int sys_dbg_write_process_memory(sys_pid_t pid, void* destination, const void* source, size_t size)
{
   system_call_4(905, (uint64_t)pid, (uint64_t)destination, size, (uint64_t)source);
   return_to_user_prog(int);
}

int sys_storage_get_device_info(uint64_t device, device_info_t* device_info)
{
   system_call_2(609, device, (uint64_t)device_info);
   return_to_user_prog(int);
}

int sys_storage_open(uint64_t id, int* fd)
{
   system_call_4(SC_STORAGE_OPEN, id, 0, (uint64_t)fd, 0);
   return_to_user_prog(int);
}

int sys_storage_close(int fd)
{
   system_call_1(SC_STORAGE_CLOSE, fd);
   return_to_user_prog(int);
}

int sys_storage_read(int fd, uint32_t start_sector, uint32_t sectors, uint8_t* bounce_buf, uint32_t* sectors_read)
{
   system_call_7(602, fd, 0, start_sector, sectors, (uint64_t)bounce_buf, (uint64_t)sectors_read, 0);
   return_to_user_prog(int);
}

int sys_storage_read_with_flags(int fd, uint32_t start_sector, uint32_t sectors, uint8_t* bounce_buf, uint32_t* sectors_read, uint32_t flags)
{
   system_call_7(602, fd, 0, start_sector, sectors, (uint64_t)bounce_buf, (uint64_t)sectors_read, flags);
   return_to_user_prog(int);
}

int sys_storage_write(int fd, uint64_t start_sector, uint64_t sectors, uint8_t* buf, uint32_t* sectors_written, uint64_t flags)
{
   system_call_7(603, fd, 0, start_sector, sectors, (uint64_t)buf, (uint64_t)sectors_written, flags);
   return_to_user_prog(int);
}

int sys_storage_get_cache_of_flash_ext_flag(uint8_t* flag)
{
   system_call_1(874, (uint64_t)flag);
   return_to_user_prog(int);
}

bool is_nor()
{
   uint8_t vf_flag;
   sys_storage_get_cache_of_flash_ext_flag(&vf_flag);
   return !(vf_flag & 0x1);
}

int lv2_get_platform_info(platform_info_t* info)
{
    system_call_1(387, (uint32_t)info);
    return_to_user_prog(int);
}

int get_target_type(uint64_t* type) // 1-CEX, 2-DEX, 3-DECR/RefTool
{
   system_call_3(985, (uint64_t)type, 0, 0);
   return_to_user_prog(int);
}

bool IsConsoleCex()
{
   uint64_t type;
   get_target_type(&type);
   return type == 1;
}

bool IsConsoleDex()
{
   uint64_t type;
   get_target_type(&type);
   return type == 2;
}

bool IsConsoleDeh()
{
   uint64_t type;
   get_target_type(&type);
   return type == 3;
}

bool IsConsoleHen()
{
   return ps3mapi_get_is_hen() == 0x1337;
}

bool IsConsoleMamba()
{
   return sys_get_mamba() == 0x666;
}

bool IsConsoleCobra()
{
   uint32_t version = 0x99999999;
   if (sys_get_version(&version) < 0)
      return false;

   if (version != 0x99999999) // If value changed, it is cobra
      return true;

   return false;
}

uint16_t GetPayloadVersion()
{
   uint16_t payloadVersion = 0;
   if (IsConsoleHen())
      payloadVersion = ps3mapi_get_hen_rev();
   else
      sys_get_version2(&payloadVersion);

   return payloadVersion;
}

bool DoesConsoleHaveCCAPI()
{
    CellFsStat sb;
    return (cellFsStat("/dev_flash/sys/internal/sys_plugin.sprx", &sb) == CELL_FS_SUCCEEDED);
}

bool DoesConsoleHaveWebman()
{
    CellFsStat sb;
    if (cellFsStat("/dev_hdd0/plugins/webftp_server.sprx", &sb) == CELL_FS_SUCCEEDED
        || cellFsStat("/dev_hdd0/plugins/webftp_server_noncobra.sprx", &sb) == CELL_FS_SUCCEEDED
        || cellFsStat("/dev_hdd0/plugins/webftp_server_cobra_ps3mapi.sprx", &sb) == CELL_FS_SUCCEEDED)
        return true;

    return false;
}

int update_mgr_write_eprom(uint64_t flag_offset, uint64_t value)
{
   system_call_7(SC_UPDATE_MANAGER_IF, UPDATE_MGR_PACKET_ID_WRITE_EPROM, flag_offset, value, 0, 0, 0, 0);
   return_to_user_prog(int);
}

int update_mgr_read_eprom(uint64_t* flag_offset, uint64_t value)
{
   system_call_7(SC_UPDATE_MANAGER_IF, UPDATE_MGR_PACKET_ID_READ_EPROM, (uint64_t)flag_offset, value, 0, 0, 0, 0);
   return_to_user_prog(int);
}

int set_recovery_mode()
{
   return update_mgr_write_eprom(RECOVER_MODE_FLAG_OFFSET, 0x00);
}

int sys_net_eurus_etherAddr(uint8_t* etherAddr)
{
   system_call_3(SYS_NET_EURUS_POST_COMMAND, CMD_GET_MAC_ADDRESS, (uint64_t)(uint32_t)etherAddr, 0x13);
   return_to_user_prog(int);
}

int sys_ss_appliance_info_manager(uint32_t packet_id, uint8_t* out)
{
   system_call_8(SC_APPLIANCE_INFO_MANAGER, packet_id, (uint64_t)out, 0, 0, 0, 0, 0, 0);
   return_to_user_prog(int);
}

void get_idps_info_manager()
{
   uint8_t idps[16];
   sys_ss_appliance_info_manager(PACKET_ID_GET_DEVICE_ID, idps);
   // 0x00 0x00 0x00 0x01 0x00 0x89 0x00 0x0B 0x14 0x00 0xEF 0xDD 0xCA 0x25 0x52 0x66
}

int GetGameID(char titleId[])
{
   system_call_1(986, (uint64_t)titleId);
   return_to_user_prog(int);
}

int ps3_led(uint64_t color, uint64_t mode)
{
   system_call_2(386, (uint64_t)color, (uint64_t)mode);
   return_to_user_prog(int);
}

int ring_buzzer_simple(void)
{
   system_call_3(SC_RING_BUZZER, 0x1004, 0x4, 0x6);
   return_to_user_prog(int);
}

int ring_buzzer_double(void)
{
   system_call_3(SC_RING_BUZZER, 0x1004, 0x7, 0x36);
   return_to_user_prog(int);
}

int ring_buzzer_triple(void)
{
   system_call_3(SC_RING_BUZZER, 0x1004, 0xa, 0x1b6);
   return_to_user_prog(int);
}

CellFsErrno sys_fs_write(int fd, const void* buf, uint64_t nbytes, uint64_t* nwrite)
{
   system_call_4(803, (uint64_t)fd, (uint64_t)buf, (uint64_t)nbytes, (uint64_t)nwrite);
   return_to_user_prog(CellFsErrno);
}

CellFsErrno sys_fs_close(int fd)
{
   system_call_1(803, (uint64_t)fd);
   return_to_user_prog(CellFsErrno);
}

CellFsErrno sys_fs_rename(const char* from, const char* to)
{
   system_call_2(812, (uint64_t)from, (uint64_t)to);
   return_to_user_prog(CellFsErrno);
}

CellFsErrno sys_fs_chmod(const char* path, CellFsMode mode)
{
   system_call_2(834, (uint64_t)path, (uint64_t)mode);
   return_to_user_prog(CellFsErrno);
}

int sys_fs_chown(const char* path, int uid, int gid)
{
   system_call_3(835, (uint64_t)path, (uint64_t)(uint32_t)uid, (uint64_t)(uint32_t)gid);
   return_to_user_prog(int);
}

int sys_fs_mount(const char* device_path, const char* device_name, const char* file_system)
{
   system_call_8(837, (uint64_t)device_name, (uint64_t)file_system, (uint64_t)device_path, 0, 0, 0, 0, 0);
   return_to_user_prog(int);
}

int sys_fs_unmount(const char* device_path)
{
   system_call_3(838, (uint64_t)device_path, 0, 1);
   return_to_user_prog(int);
}

void del_turnoff(uint8_t beeps)
{
   cellFsUnlink("/dev_hdd0/tmp/turnoff");  // to avoid bad shutdown

   if (beeps == 1) { ring_buzzer_simple(); }
   if (beeps == 2) { ring_buzzer_double(); }
}

int ps3_shutdown(void)
{
   del_turnoff(1);
   system_call_4(SC_SYS_POWER, SYS_SHUTDOWN, 0, 0, 0);
   return_to_user_prog(int);
}

int ps3_hard_restart(void)
{
   del_turnoff(2);
   system_call_3(SC_SYS_POWER, SYS_HARD_REBOOT, NULL, 0);  // hard reboot
   return_to_user_prog(int);
}

int ps3_quick_restart(void)
{
   del_turnoff(2);
   system_call_3(SC_SYS_POWER, SYS_REBOOT, NULL, 0);  // (quick reboot) load LPAR id 1
   return_to_user_prog(int);
}

int ps3_soft_restart(void)
{
   del_turnoff(2);
   system_call_3(SC_SYS_POWER, SYS_SOFT_REBOOT, NULL, 0);  // soft reboot
   return_to_user_prog(int);
}

void ps3_exit_xmb()
{
   sys_fs_unmount("/dev_flash");
}

void enable_dev_blind()
{
   sys_fs_mount("/dev_blind", "CELL_FS_IOS:BUILTIN_FLSH1", "CELL_FS_FAT");
}

void disable_dev_blind()
{
   sys_fs_unmount("/dev_blind");
}

int sys_dbg_get_thread_list(sys_pid_t pid, sys_ppu_thread_t* ids, uint64_t* ids_num, uint64_t* all_ids_num)
{
   system_call_4(909, (uint64_t)pid, (uint64_t)ids, (uint64_t)ids_num, (uint64_t)all_ids_num);
   return_to_user_prog(int);
}

int sys_game_get_temperature(uint32_t _dev, uint32_t* temperature)
{
   system_call_2(383, (uint64_t)_dev, (uint64_t)((uint32_t)temperature));
   return_to_user_prog(int);
}

int get_temperature_celcius(uint32_t& cpu_temp, uint32_t& rsx_temp)
{
   int ret = sys_game_get_temperature(0, &cpu_temp);
   sys_game_get_temperature(1, &rsx_temp);
   cpu_temp = cpu_temp >> 24;
   rsx_temp = rsx_temp >> 24;
   return ret;
}

int sys_sm_set_fan_policy(uint8_t arg0, uint8_t arg1, uint8_t arg2)
{
   system_call_3(SC_SET_FAN_POLICY, (uint64_t)arg0, (uint64_t)arg1, (uint64_t)arg2);
   return_to_user_prog(int);
}

int sys_sm_get_fan_policy(uint8_t id, uint8_t* st, uint8_t* mode, uint8_t* speed, uint8_t* unknown)
{
   system_call_5(SC_GET_FAN_POLICY, (uint64_t)id, (uint64_t)(uint32_t)st, (uint64_t)(uint32_t)mode, (uint64_t)(uint32_t)speed, (uint64_t)(uint32_t)unknown);
   return_to_user_prog(int);
}

void ps3_set_recovery_mode()
{
   sys_timer_sleep(1);
   set_recovery_mode();
   ps3_hard_restart();
}

int _sys_prx_unload_module(sys_prx_id_t id, sys_prx_flags_t flags, const sys_prx_unload_module_option_t* pOpt)
{
   system_call_3(483, (uint64_t)id, (uint64_t)flags, (uint64_t)pOpt);
   return_to_user_prog(int);
}

int _sys_prx_stop_module(sys_prx_id_t id, size_t args, void* argp, int* modres, sys_prx_flags_t flags, sys_prx_stop_module_option_t* pOpt)
{
   system_call_6(482, (uint64_t)id, (uint64_t)args, (uint64_t)argp, (uint64_t)modres, (uint64_t)flags, (uint64_t)pOpt);
   return_to_user_prog(int);
}

sys_prx_id_t _sys_prx_get_module_id_by_name(const char* name, sys_prx_flags_t flags, sys_prx_get_module_id_by_name_option_t* pOpt)
{
   system_call_3(496, (uint64_t)name, (uint64_t)flags, (uint64_t)pOpt);
   return_to_user_prog(sys_prx_id_t);
}

sys_prx_id_t _sys_prx_get_module_id_by_address(void* addr)
{
   system_call_1(461, (uint64_t)addr);
   return_to_user_prog(sys_prx_id_t);
}

sys_prx_id_t _sys_prx_get_my_module_id()
{
   return _sys_prx_get_module_id_by_address((void*)_sys_prx_get_my_module_id);
}

void _sys_ppu_thread_exit(uint64_t val)
{
   system_call_1(SC_PPU_THREAD_EXIT, val);
}

void UnloadMyModule()
{
    sys_prx_id_t prxId = _sys_prx_get_my_module_id();
    uint64_t meminfo[5]{ 0x28, 2, 0, 0, 0 };
    _sys_prx_stop_module(prxId, 0, meminfo, NULL, 0, NULL);
}