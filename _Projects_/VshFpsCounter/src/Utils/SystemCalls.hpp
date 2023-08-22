#pragma once
#include <sys/prx.h> // for sys_prx_id_t
#include <stddef.h> // NULL
#include <cell/cell_fs.h> // CellFsErrno
#include <sys/ppu_thread.h> // sys_ppu_thread_yield
#include <sys/timer.h> // sys_timer_sleep

#define SC_PPU_THREAD_EXIT                      41
#define SC_SYS_POWER                            379
#define SC_GET_TEMPERATURE                      383
#define SC_SET_FAN_POLICY                       389
#define SC_RING_BUZZER                          392
#define SC_GET_FAN_POLICY                       409
#define SC_STORAGE_OPEN                         600
#define SC_STORAGE_CLOSE                        601
#define SC_STORAGE_INSERT_EJECT                 616
#define SC_FS_LINK                              810
#define SC_UPDATE_MANAGER_IF                    863
#define SC_APPLIANCE_INFO_MANAGER               867


#define FLASH_DEVICE_NOR                    0x0100000000000004ULL
#define FLASH_DEVICE_NAND                   0x0100000000000001ULL
#define FLASH_FLAGS							0x22ULL

#define UPDATE_MGR_PACKET_ID_READ_EPROM		0x600B
#define UPDATE_MGR_PACKET_ID_WRITE_EPROM	0x600C
#define RECOVER_MODE_FLAG_OFFSET			0x48C61
#define EPROM_FSELF_FLAG_OFFSET				0x48C06
#define EPROM_DEBUG_OFFSET					0x48C08

#define SYS_SOFT_REBOOT 				0x0200
#define SYS_HARD_REBOOT					0x1200
#define SYS_REBOOT						0x8201 /*load LPAR id 1*/
#define SYS_SHUTDOWN					0x1100

#define SYS_NET_EURUS_POST_COMMAND		726
#define CMD_GET_MAC_ADDRESS				0x103f

// https://www.psdevwiki.com/ps3/AIM_Manager
#define PACKET_ID_GET_DEVICE_TYPE 0x19002
#define PACKET_ID_GET_DEVICE_ID 0x19003
#define PACKET_ID_GET_PSCODE 0x19004
#define PACKET_ID_GET_OPEN_PSID 0x19005
#define PACKET_ID_UNKNOWN 0x19006



#define SC_COBRA_SYSCALL8                               8
#define SYSCALL8_OPCODE_DISABLE_COBRA              0x0000
#define SYSCALL8_OPCODE_ENABLE_COBRA               0x0001
#define SYSCALL8_DISABLE_COBRA_CAPABILITY          0x0002
#define SYSCALL8_DISABLE_COBRA_STATUS              0x0003
#define SYSCALL8_DISABLE_COBRA_OK                  0x5555
#define SYSCALL8_OPCODE_GET_MAMBA                  0x7FFFULL
#define SYSCALL8_OPCODE_GET_VERSION                0x7000
#define SYSCALL8_OPCODE_GET_VERSION2               0x7001
#define SYSCALL8_OPCODE_POKE_LV2                   0x7003 //8.1 HABIB
#define SYSCALL8_OPCODE_POKE8_LV2                  0x7005 //8.3 AV
#define SYSCALL8_OPCODE_POKE16_LV2                 0x7007 //8.3 AV
#define SYSCALL8_OPCODE_POKE32_LV2                 0x7009 //8.3 AV
#define SYSCALL8_OPCODE_POKE_LV1                   0x7013 //8.3 AV
#define SYSCALL8_OPCODE_POKE8_LV1                  0x7015 //8.3 AV
#define SYSCALL8_OPCODE_POKE16_LV1                 0x7017 //8.3 AV
#define SYSCALL8_OPCODE_POKE32_LV1                 0x7019 //8.3 AV
#define SYSCALL8_OPCODE_IS_HEN                     0x1337
#define SYSCALL8_OPCODE_HEN_REV                    0x1339
#define SYSCALL8_OPCODE_GET_MAP_PATH               0x7967
#define SYSCALL8_OPCODE_GET_DISC_TYPE              0x7020
#define SYSCALL8_OPCODE_READ_PS3_DISC              0x7021
#define SYSCALL8_OPCODE_FAKE_STORAGE_EVENT         0x7022
#define SYSCALL8_OPCODE_GET_EMU_STATE              0x7023
#define SYSCALL8_OPCODE_MOUNT_PS3_DISCFILE         0x7024
#define SYSCALL8_OPCODE_MOUNT_DVD_DISCFILE         0x7025
#define SYSCALL8_OPCODE_MOUNT_BD_DISCFILE          0x7026
#define SYSCALL8_OPCODE_MOUNT_PSX_DISCFILE         0x7027
#define SYSCALL8_OPCODE_MOUNT_PS2_DISCFILE         0x7028
#define SYSCALL8_OPCODE_MOUNT_DISCFILE_PROXY       0x6808
#define SYSCALL8_OPCODE_UMOUNT_DISCFILE            0x702C
#define SYSCALL8_OPCODE_MOUNT_ENCRYPTED_IMAGE      0x702D
#define SYSCALL8_OPCODE_GET_ACCESS                 0x8097
#define SYSCALL8_OPCODE_REMOVE_ACCESS              0x8654
#define SYSCALL8_OPCODE_READ_COBRA_CONFIG          0x7050
#define SYSCALL8_OPCODE_WRITE_COBRA_CONFIG         0x7051
#define SYSCALL8_OPCODE_SET_PSP_UMDFILE            0x9003
#define SYSCALL8_OPCODE_SET_PSP_DECRYPT_OPTIONS    0x9002
#define SYSCALL8_OPCODE_READ_PSP_HEADER            0x1028
#define SYSCALL8_OPCODE_READ_PSP_UMD               0x1029
#define SYSCALL8_OPCODE_PSP_PRX_PATCH              0x204C
#define SYSCALL8_OPCODE_PSP_POST_SAVEDATA_INITSTART      0x3008
#define SYSCALL8_OPCODE_PSP_POST_SAVEDATA_SHUTDOWNSTART  0x3009
#define SYSCALL8_OPCODE_PSP_CHANGE_EMU             0x9752
#define SYSCALL8_OPCODE_COBRA_USB_COMMAND          0x7714
#define SYSCALL8_OPCODE_AIO_COPY_ROOT              0x6637
#define SYSCALL8_OPCODE_MAP_PATHS                  0x7964
#define SYSCALL8_OPCODE_VSH_SPOOF_VERSION          0x2C0F
#define SYSCALL8_OPCODE_LOAD_VSH_PLUGIN            0x1EE7
#define SYSCALL8_OPCODE_USE_PS2NETEMU              0x1EE9  //unused
#define SYSCALL8_OPCODE_UNLOAD_VSH_PLUGIN          0x364F
#define SYSCALL8_OPCODE_DRM_GET_DATA               0x6A11
#define SYSCALL8_OPCODE_RUN_PAYLOAD                0x6CDF
#define SYSCALL8_OPCODE_RUN_PAYLOAD_DYNAMIC        0x6CE1
#define SYSCALL8_OPCODE_UNLOAD_PAYLOAD_DYNAMIC     0x6CE3
#define SYSCALL8_OPCODE_DISABLE_COBRA_STAGE        0x6A13


#define SYSCALL8_OPCODE_PS3MAPI                    0x7777
#define PS3MAPI_OPCODE_GET_CORE_VERSION            0x0011
#define PS3MAPI_OPCODE_GET_CORE_MINVERSION         0x0012
#define PS3MAPI_OPCODE_GET_FW_TYPE                 0x0013
#define PS3MAPI_OPCODE_GET_FW_VERSION              0x0014
#define PS3MAPI_OPCODE_GET_ALL_PROC_PID            0x0021
#define PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID        0x0022
#define PS3MAPI_OPCODE_GET_PROC_BY_PID             0x0023
#define PS3MAPI_OPCODE_GET_CURRENT_PROC            0x0024
#define PS3MAPI_OPCODE_GET_CURRENT_PROC_CRIT       0x0025
#define PS3MAPI_OPCODE_GET_PROC_MEM                0x0031
#define PS3MAPI_OPCODE_SET_PROC_MEM                0x0032
#define PS3MAPI_OPCODE_PROC_PAGE_ALLOCATE          0x0033
#define PS3MAPI_OPCODE_GET_ALL_PROC_MODULE_PID     0x0041
#define PS3MAPI_OPCODE_GET_PROC_MODULE_NAME        0x0042
#define PS3MAPI_OPCODE_GET_PROC_MODULE_FILENAME    0x0043
#define PS3MAPI_OPCODE_LOAD_PROC_MODULE            0x0044
#define PS3MAPI_OPCODE_UNLOAD_PROC_MODULE          0x0045
#define PS3MAPI_OPCODE_UNLOAD_VSH_PLUGIN           0x0046
#define PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO         0x0047
#define PS3MAPI_OPCODE_GET_PROC_MODULE_SEGMENTS    0x0048 // TheRouletteBoi
#define PS3MAPI_OPCODE_GET_VSH_PLUGIN_BY_NAME      0x004F
#define SYSCALL8_OPCODE_PROC_CREATE_THREAD         0x6E03 // not eough params for PS3MAPI_OPCODE
#define SYSCALL8_OPCODE_STEALTH_TEST               0x3993 //KW PSNPatch stealth extension compatibility
#define SYSCALL8_OPCODE_STEALTH_ACTIVATE           0x3995 //KW PSNPatch stealth extension compatibility
#define SYSCALL8_STEALTH_OK                        0x5555 //KW PSNPatch stealth extension compatibility
#define PS3MAPI_OPCODE_CHECK_SYSCALL               0x0091
#define PS3MAPI_OPCODE_DISABLE_SYSCALL             0x0092
#define PS3MAPI_OPCODE_PDISABLE_SYSCALL8           0x0093 //Look in main.c for code.
#define PS3MAPI_OPCODE_PCHECK_SYSCALL8             0x0094 //Look in main.c for code.
#define PS3MAPI_OPCODE_CREATE_CFW_SYSCALLS         0x0095
#define PS3MAPI_OPCODE_ALLOW_RESTORE_SYSCALLS      0x0096
#define PS3MAPI_OPCODE_GET_RESTORE_SYSCALLS        0x0097
#define PS3MAPI_OPCODE_SUPPORT_SC8_PEEK_POKE       0x1000
#define PS3MAPI_OPCODE_LV2_PEEK                    0x1006
#define PS3MAPI_OPCODE_LV2_POKE                    0x1007
#define PS3MAPI_OPCODE_LV1_PEEK                    0x1008
#define PS3MAPI_OPCODE_LV1_POKE                    0x1009
#define PS3MAPI_OPCODE_SUPPORT_SC8_PEEK_POKE_OK    0x6789
#define PS3MAPI_OPCODE_SET_ACCESS_KEY              0x2000
#define PS3MAPI_OPCODE_REQUEST_ACCESS              0x2001
#define PS3MAPI_OPCODE_REMOVE_HOOK                 0x0101 //Look in main.c for code.
#define PS3MAPI_OPCODE_GET_IDPS                    0x0081
#define PS3MAPI_OPCODE_SET_IDPS                    0x0082
#define PS3MAPI_OPCODE_GET_PSID                    0x0083
#define PS3MAPI_OPCODE_SET_PSID                    0x0084
#define PS3MAPI_OPCODE_AUTO_DEV_BLIND              0x2221
#define PS3MAPI_OPCODE_PHOTO_GUI                   0x2222
#define PS3MAPI_OPCODE_GET_FAN_SPEED               0x2233
#define PS3MAPI_OPCODE_SET_FAN_SPEED               0x2235
#define PS3MAPI_OPCODE_SET_PS2_FAN_SPEED           0x2236
#define PS3MAPI_OPCODE_GET_SKIP_EXISTING_RIF       0x2240
#define PS3MAPI_OPCODE_SKIP_EXISTING_RIF           0x2241
#define PS3MAPI_OPCODE_RING_BUZZER                 0x2245
#define PS3MAPI_OPCODE_CONVERT_SAVEDATA            0x2246
#define PS3MAPI_OPCODE_SET_FAKE_ACCOUNTID          0x2247
#define PS3MAPI_OPCODE_ACTIVATE_ACCOUNT            0x2248
#define PS3MAPI_OPCODE_CREATE_RIF                  0x2249
#define PS3MAPI_OPCODE_CHECK_QA                    0x2242
#define PS3MAPI_OPCODE_SET_QA                      0x2243

#define PS3MAPI_FIND_FREE_SLOT                     NULL

enum class ShutdownFlag : uint16_t
{
    lpar_id_1 = (1 << 0),
    lpar_id_2 = (1 << 1),
    lpar_id_3 = (1 << 2),
    sys_shutdown = (1 << 8),
    sys_reboot = (1 << 9),
    sys_hard_shutdown = (1 << 12),
    sys_load_lpar = (1 << 15)
};

enum class LedColor : uint8_t
{
    led_green = 1,
    led_red = 2
};

enum class LedAction : uint8_t
{
    led_action_off,
    led_action_on,
    led_action_blink_fast,
    led_action_blink_slow
};

enum class RingBuzzerAction : uint16_t
{
    ring_buzzer_single = 0x0006,
    ring_buzzer_double = 0x0036,
    ring_buzzer_triple = 0x01B6,
    ring_buzzer_continuous = 0x0FFF
};

enum class ConsoleType
{
    CEX = 1,
    DEX,
    DEH
};

typedef struct
{
    char firmware_version[7];
    char platform_id[9];
    uint32_t build_number;
    uint32_t unknown;
} platform_info_t; // size: 0x18

typedef struct
{
   void* unk_00; // 0
   char name[24]; // 8
               // ...
} __attribute__((packed)) UnkProcessStruct;

typedef struct _process_t
{
   void* syscall_table; // 0
   uint64_t unk_8[4]; // 8
   uint32_t pid; // 0x28
   int status; // 0x2C
   void* mem_object; // 0x30
   UnkProcessStruct* unk_38; // 0x38
   uint64_t unk_40; // 0x40
   void* first_thread; // 0x48 
   uint64_t unk_50; // 0x50
   uint64_t unk_58; // 0x58
   void* unk_60; // 0x60
   void* unk_68; // 0x68 vshprocess -> mios2_SPU_Service.elf
   void* unk_70; // 0x70 vshprocess -> mios2_SPU_Service.elf
   uint64_t unk_78; // 0x78
   uint64_t unk_80; // 0x80
   uint64_t unk_88[4]; // 0x88
   uint64_t unk_A8; // 0xA8  user address?
   struct _process_t* parent; // 0xB0
   struct _process_t* first_child; // 0xB8  
   struct _process_t* next_sibling; // 0xC0
   uint64_t num_children; // 0xC8
   void* unk_D0; // 0xD0
   uint64_t unk_D8; // 0xD8
   uint64_t unk_E0; // 0xE0
   uint64_t unk_E8; // 0xE8
   uint64_t unk_F0[2]; // 0xF0
   uint64_t unk_100; // 0x100
   uint64_t unk_108; // 0x108
   void* unk_110; // 0x110
   void* unk_118; // 0x118  vshprocess -> pointer to unk_D0
   uint64_t unk_120; // 0x120
   void* unk_128; // 0x128  only on vshprocess -> same as first_thread
   void* unk_130; // 0x130 only on vsh process -> same as first thread
   uint64_t unk_138; // 0x138
   uint64_t unk_140[4]; // 0x140
   char* process_image; // 0x160
   void* unk_168; // 0x168
   uint64_t unk_170; // 0x170
   uint64_t unk_178; // 0x178
   uint64_t unk_180; // 0x180
   uint64_t unk_188[4]; // 0x188
   uint64_t unk_1A8; // 0x1A8
   uint64_t unk_1B0; // 0x1B0
   uint64_t unk_1B8; // 0x1B8
   uint64_t unk_1C0; // 0x1C0
   uint64_t unk_1C8; // 0x1C8
   uint64_t unk_1D0; // 0x1D0
   uint64_t unk_1D8; // 0x1D8
   uint64_t unk_1E0; // 0x1E0
   uint64_t unk_1E8[4]; // 0x1E8
   void* object_table; // 0x208 waiting for a better name...
   // ...?
   // 0x26C -> sdk version 32bits
} __attribute__((packed))* process_t;

typedef struct sys_process_info_t {
   sys_pid_t pid;
   uint32_t status;
   uint32_t ppu_thread_count;
   uint32_t spu_thread_count;
   uint32_t raw_spu_count;
   sys_pid_t parent_pid;
   char* image_file_name;
   uint32_t memory_size;
   uint8_t osabi_type;
   uint8_t pad[3];
   uint64_t interrupt_bitmap_mask;
   uint32_t dbg_trace_id;
} sys_process_info_t;

typedef struct
{
   void* unk_0; // ptr to some funcs
   uint64_t unk_8;
   uint32_t unk_10;
   uint32_t unk_14;
   void* unk_18;
   void* unk_20; // same as unk_18? :S
   uint64_t unk_28[3];
   void* unk_40; // same as unk_0?
   // ...
} thread_t;

typedef struct
{
   uint8_t     name[7];
   uint8_t     unknown01;
   uint32_t    unknown02; // random nr?
   uint32_t    zero01;
   uint32_t    unknown03; // 0x28?
   uint32_t    unknown04; // 0xd000e990?
   uint8_t     zero02[16];
   uint64_t    total_sectors;
   uint32_t    sector_size;
   uint32_t    unknown05;
   uint8_t     writable;
   uint8_t     unknown06[3];
   uint32_t    unknown07;
} __attribute__((packed)) device_info_t;


int sys_map_path(char** paths, char** new_paths, int num);
int cobra_load_vsh_plugin(unsigned int slot, char* path, void* arg, uint32_t arg_size);
int cobra_unload_vsh_plugin(unsigned int slot);
int ps3mapi_get_core_version();
int ps3mapi_get_core_minversion();
int ps3mapi_get_fw_type(char* fw);
int ps3mapi_get_fw_version();
int ps3mapi_get_all_process_modules_prx_id(sys_pid_t pid, sys_prx_id_t* prx_id_list);
int ps3mapi_get_process_module_name_by_prx_id(sys_pid_t pid, sys_prx_id_t prx_id, char* name);
int ps3mapi_get_process_module_filename_by_prx_id(sys_pid_t pid, sys_prx_id_t prx_id, char* filename);
int ps3mapi_load_process_modules(sys_pid_t pid, char* path, void* arg, uint32_t arg_size);
int ps3mapi_unload_process_modules(sys_pid_t pid, sys_prx_id_t prx_id);
int ps3mapi_unload_vsh_plugin(char* name);
int ps3mapi_get_vsh_plugin_info(unsigned int slot, char* name, char* filename);
int ps3mapi_load_kernel_plugin(uint8_t* payload, int size);
int ps3mapi_load_dynamic_kernel_plugin(uint8_t* payload, int size, uint64_t* residence);
int ps3mapi_unload_dynamic_kernel_plugin(uint64_t residence);
int ps3mapi_get_is_hen();
int ps3mapi_get_hen_rev();
int ps3mapi_poke_hen(uint64_t address, uint64_t value);
int ps3mapi_process_page_allocate(sys_pid_t pid, uint64_t size, uint64_t page_size, uint64_t flags, uint64_t is_executable, uint64_t* page_address);
int ps3mapi_get_process_module_segments(sys_pid_t pid, sys_prx_id_t prx_id, sys_prx_module_info_t* info);
int ps3mapi_create_process_thread(sys_pid_t pid, thread_t* thread, void* entry, uint64_t arg, int prio, size_t stacksize, const char* threadname);
int ps3mapi_get_all_processes_pid(uint32_t* pid_list);
int ps3mapi_get_process_name_by_pid(uint32_t pid, char* pid_name);
int ps3mapi_get_process_by_pid(uint32_t pid, process_t process);
int ps3mapi_get_current_process_critical(process_t process);
int ps3mapi_get_current_process(process_t process);
int ps3mapi_check_syscall(int num);
int ps3mapi_disable_syscall(int num);
int ps3mapi_pdisable_syscall8(int mode);
int ps3mapi_pcheck_syscall8();
int ps3mapi_get_idps(uint64_t* idps);
int ps3mapi_set_idps(uint64_t part1, uint64_t part2);
int ps3mapi_get_psid(uint64_t* psid);
int ps3mapi_set_psid(uint64_t part1, uint64_t part2);
int ps3mapi_remove_hook(void);
int ps3mapi_enable_access_syscall8(uint64_t key);
int ps3mapi_disable_access_syscall8(uint64_t key);
int sys_get_mamba();
int sys_get_version(uint32_t* version);
int sys_get_version2(uint16_t* version);
uint64_t ps3mapi_lv1_peek(uint64_t addr);
uint64_t ps3mapi_lv1_poke(uint64_t addr, uint64_t value);
uint64_t ps3mapi_lv2_peek(uint64_t addr);
uint64_t ps3mapi_lv2_poke(uint64_t addr, uint64_t value);
int PS3MAPIGetMemory(sys_pid_t pid, void* destination, void* source, size_t size);
int PS3MAPISetMemory(sys_pid_t pid, void* destination, const void* source, size_t size);
uint64_t PeekLv1(uint64_t addr);
void PokeLv1(uint64_t addr, uint64_t value);
uint64_t PeekLv2(uint64_t address);
void PokeLv2(uint64_t addr, uint64_t val);
uint8_t PeekUint8LV1(uint64_t addr);
uint32_t PeekUint32LV1(uint64_t addr);
bool PeekChunkLV1(uint64_t start, uint64_t* buffer, uint64_t size);
bool PeekChunkLV2(uint64_t start, uint64_t* buffer, uint64_t size);
// lv2_lv1_call - rebug // r10 is the lv1 syscall number. The registers r3 - r9 are arguments.
uint64_t CallLv1Function(uint64_t addr, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7);
// lv2_func_call - habib //  Allow execution of any LV2 internal function // r10 is the address to the lv2 function. The registers r3 - r9 are arguments
uint64_t CallLv2Function(uint64_t addr, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7);
int sys_dbg_read_process_memory(sys_pid_t pid, void* destination, void* source, size_t size);
int sys_dbg_write_process_memory(sys_pid_t pid, void* destination, const void* source, size_t size);
int sys_storage_get_device_info(uint64_t device, device_info_t* device_info);
int sys_storage_open(uint64_t id, int* fd);
int sys_storage_close(int fd);
int sys_storage_read(int fd, uint32_t start_sector, uint32_t sectors, uint8_t* bounce_buf, uint32_t* sectors_read);
int sys_storage_read_with_flags(int fd, uint32_t start_sector, uint32_t sectors, uint8_t* bounce_buf, uint32_t* sectors_read, uint32_t flags);
int sys_storage_write(int fd, uint64_t start_sector, uint64_t sectors, uint8_t* buf, uint32_t* sectors_written, uint64_t flags);
int sys_storage_get_cache_of_flash_ext_flag(uint8_t* flag);
bool is_nor();
int lv2_get_platform_info(platform_info_t* info);
int get_target_type(uint64_t* type); // 1-CEX, 2-DEX, 3-DECR/RefTool
bool IsConsoleCex();
bool IsConsoleDex();
bool IsConsoleDeh();
bool IsConsoleHen();
bool IsConsoleMamba();
bool IsConsoleCobra();
uint16_t GetPayloadVersion();
bool DoesConsoleHaveCCAPI();
bool DoesConsoleHaveWebman();
int update_mgr_write_eprom(uint64_t flag_offset, uint64_t value);
int update_mgr_read_eprom(uint64_t* flag_offset, uint64_t value);
int set_recovery_mode();
int sys_net_eurus_etherAddr(uint8_t* etherAddr);
int sys_ss_appliance_info_manager(uint32_t packet_id, uint8_t* out);
void get_idps_info_manager();
int GetGameID(char titleId[]);
int ps3_led(uint64_t color, uint64_t mode);
int ring_buzzer_simple(void);
int ring_buzzer_double(void);
int ring_buzzer_triple(void);
CellFsErrno sys_fs_write(int fd, const void* buf, uint64_t nbytes, uint64_t* nwrite);
CellFsErrno sys_fs_close(int fd);
CellFsErrno sys_fs_rename(const char* from, const char* to);
CellFsErrno sys_fs_chmod(const char* path, CellFsMode mode);
int sys_fs_chown(const char* path, int uid, int gid);
int sys_fs_mount(const char* device_path, const char* device_name, const char* file_system);
int sys_fs_unmount(const char* device_path);
void del_turnoff(uint8_t beeps);
int ps3_shutdown(void);
int ps3_hard_restart(void);
int ps3_quick_restart(void);
int ps3_soft_restart(void);
void ps3_exit_xmb();
void enable_dev_blind();
void disable_dev_blind();
int sys_dbg_get_thread_list(sys_pid_t pid, sys_ppu_thread_t* ids, uint64_t* ids_num, uint64_t* all_ids_num);
int sys_game_get_temperature(uint32_t _dev, uint32_t* temperature);
int get_temperature_celcius(uint32_t& cpu_temp, uint32_t& rsx_temp);
int sys_sm_set_fan_policy(uint8_t arg0, uint8_t arg1, uint8_t arg2);
int sys_sm_get_fan_policy(uint8_t id, uint8_t* st, uint8_t* mode, uint8_t* speed, uint8_t* unknown);
void ps3_set_recovery_mode();
int _sys_prx_unload_module(sys_prx_id_t id, sys_prx_flags_t flags, const sys_prx_unload_module_option_t* pOpt);
int _sys_prx_stop_module(sys_prx_id_t id, size_t args, void* argp, int* modres, sys_prx_flags_t flags, sys_prx_stop_module_option_t* pOpt);
sys_prx_id_t _sys_prx_get_module_id_by_name(const char* name, sys_prx_flags_t flags, sys_prx_get_module_id_by_name_option_t* pOpt);
sys_prx_id_t _sys_prx_get_module_id_by_address(void* addr);
sys_prx_id_t _sys_prx_get_my_module_id();
// prxloader = mandatory; cobra = optional; ccapi = don't use !!!
void _sys_ppu_thread_exit(uint64_t val);
void UnloadMyModule();