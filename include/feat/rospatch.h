#ifdef PATCH_ROS
static bool stop_ros = false;
static bool ros_patching = false;

#define ROS0_overlay_path "/dev_hdd0/ros0_overlay.bin"
#define ROS1_overlay_path "/dev_hdd0/ros1_overlay.bin"

static int sys_storage_write(int fd, u32 start_sector, u32 sectors, void *bounce_buf, u32 *sectors_read)
{
    system_call_7(603, fd, 0, start_sector, sectors, (u64)(u32) bounce_buf, (u64)(u32) sectors_read, FLASH_FLAGS);
    return (int)p1;
}

static void sys_ss_get_cache_of_flash_ext_flag(u8 *flag)
{
    system_call_1(874, (u64)(u32) flag);
}

static bool is_nor(void)
{
    u8 flag = 0;
    sys_ss_get_cache_of_flash_ext_flag(&flag);
    return !(1 & flag);
}

// TODO: resolve unused "full", add missing bool usb_debug
static int patch_ros(const char *patch_file, u8 *mem, int conn_s, u8 ros, bool full)
{
    send(conn_s, "<p>Patch process start</p>", 26, 0);
    if(ros_patching || file_size(patch_file) != 7340000) {BEEP3; return FAILED;}

    const char ROS_HEADER[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0xFF, 0xE0 };

    ros_patching = true; // prevent call this function while processing

    // Redirect all flash reads and writes to dev_usb000
    // USB storage is supposed to contain a flash dump written directly from the beginning
    // i.e. dd if=nor_dump.bin of=/dev/sde     or     Win32DiskImager / ImageUSB for Windows
    bool usb_debug = true;

    // open nofsm_patch.bin
    int fd, ret = CELL_OK;
    if(cellFsOpen(patch_file, CELL_FS_O_RDONLY, &fd, NULL, 0) != CELL_FS_SUCCEEDED) {BEEP3; ros_patching = false; return FAILED;}

    // open flash
    const u32 flags = 0x01000000;
    u32 offset1, offset2, metldr_sec = 0x4, r; // metldr is always at sector 4, offset 0x20
    u64 start_ros0, start_ros1; // NOR ros0 = 0xC0000, ros1 = 0x7C0000
    sys_device_handle_t flash_id;

    // TODO: Add check for minver < 3.60

    if(is_nor())
    {
        // Open NOR flash
        send(conn_s, "<p>Opening NOR...</p>", 22, 0);
        if(!usb_debug)
        {
            if(sys_storage_open(FLASH_DEVICE_NOR, 0, &flash_id, flags) != CELL_OK) {BEEP3; cellFsClose(fd); ros_patching = false; return FAILED;}
        }
        else
        {
            send(conn_s, "<p>USB debug enabled</p>", 24, 0);
            if(sys_storage_open(0x10300000000000AULL, 0, &flash_id, flags) != CELL_OK) {BEEP3; cellFsClose(fd); ros_patching = false; return FAILED;}
        }
        offset1 = 0x10;
        offset2 = 0x10;
        start_ros0 = 0x600;
        start_ros1 = 0x3e00;
    }
    else
    {
        // Open NAND flash
        send(conn_s, "<p>Opening NAND...</p>", 23, 0);
        if(!usb_debug)
        {
            if(sys_storage_open(FLASH_DEVICE_NAND, 0, &flash_id, flags) != CELL_OK) {BEEP3; cellFsClose(fd); ros_patching = false; return FAILED;}
        }
        else
        {
            send(conn_s, "<p>USB debug enabled</p>", 24, 0);
            if(sys_storage_open(0x10300000000000AULL, 0, &flash_id, flags) != CELL_OK) {BEEP3; cellFsClose(fd); ros_patching = false; return FAILED;}
        }
        offset1 = 0x30;
        offset2 = 0x20;
        start_ros0 = 0x400;
        start_ros1 = 0x3c00;
    }

    const int sec_size = 0x200; // 512 bytes

    char read_buf[sec_size + 0x30]; // length of one sector + something extra for more room to play
    memset(&read_buf, 0, sec_size + 0x30);

    char *item_name = ((char *) read_buf) + 0x20;

    // abort if metldr is not found at metldr_sec (or if metldr.2 is found)
    sys_storage_read(flash_id, 0, metldr_sec, 1, read_buf, &r, FLASH_FLAGS);
    if(!(IS(item_name, "metldr") && !IS(item_name, "metldr.2")))
    {
        send(conn_s, "<p>Wrong metldr version!</p>", 28, 0);
        ret = FAILED; goto exit_ros;
    }

    // Prepare ROS overlays
    for(int cur_ros = 0; cur_ros <= 1; cur_ros++)
    {
        send(conn_s, "<p>Preparing ROS overlay...</p>", 31, 0);
        int fd_overlay;
        int curr_offset;
        if(cur_ros == 0)
        {
            if(cellFsOpen(ROS0_overlay_path, CELL_FS_O_CREAT|CELL_FS_O_WRONLY|CELL_FS_O_TRUNC, &fd_overlay, NULL, 0) != CELL_FS_SUCCEEDED) {BEEP3; ros_patching = false; return FAILED;}
            curr_offset = offset1;
        }
        else
        {
            if(cellFsOpen(ROS1_overlay_path, CELL_FS_O_CREAT|CELL_FS_O_WRONLY|CELL_FS_O_TRUNC, &fd_overlay, NULL, 0) != CELL_FS_SUCCEEDED) {BEEP3; ros_patching = false; return FAILED;}
            curr_offset = offset2;
        }

        //Write ROS header
        u64 n_written;
        memcpy(&read_buf, ROS_HEADER, sizeof(ROS_HEADER));
        
        // Special care for NAND based models
        if(!is_nor())
        {
            // Preserve first 16 bytes of ROS0
            char read_buf2[sec_size];
            sys_storage_read(flash_id, 0, start_ros0, 1, &read_buf2, &r, FLASH_FLAGS);
            memcpy(&read_buf, &read_buf2, 16);

            if(cur_ros == 1)
            {
                // Zero fill 16 bytes at offset 0x10 in ROS1 header for NAND
                memset(&read_buf[0x10], 0x00, 16);
            }
        }

        cellFsWrite(fd_overlay, &read_buf[sizeof(ROS_HEADER) - curr_offset], curr_offset, &n_written);
        // TODO: Checks for number of actually written bytes/sectors

        //Copy noFSM patch
        u64 n_read;
        for(int sector = 0; sector <= 0x37FF; sector++)
        {
            // Read 1 sector
            cellFsReadWithOffset(fd, sector * sec_size, &read_buf, sec_size, &n_read);

            if(sector == 0x37FF)
            {
                if(curr_offset < 0x20)
                {
                    // If we are at the very last sector of noFSM patch and the offset isn't 32 bytes or more.
                    // It means that the last noFSM read wasn't able to get a whole sector. (noFSM patch is always 32 bytes shorter)
                    // We are supposed to add padding so it's aligned exactly to 0x3800 sectors
                    char padding_val = is_nor() ? 0xFF: 0x00;

                    memset(&read_buf[n_read], padding_val, sec_size - n_read);
                    cellFsWrite(fd_overlay, read_buf, sec_size - (0x20 - curr_offset), &n_written);
                }
                
                if(curr_offset >= 0x20)
                {
                    // Write the last incomplete noFSM sector
                    // Write less if the offset is longer than 0x20
                    cellFsWrite(fd_overlay, read_buf, n_read - (curr_offset - 0x20), &n_written);
                }
            }
            else
            {
                cellFsWrite(fd_overlay, read_buf, sec_size, &n_written);
            }
        }

        send(conn_s, "<p>ROS overlay prepared</p>", 27, 0);
        cellFsClose(fd_overlay);
        // TODO: Check that newly written overlay file is indeed 0x3800 sectors long, 7 340 032 bytes
    }

    // Write ROS overlays to flash
    for(int cur_ros = 0; cur_ros <= 1; cur_ros++)
    {
        send(conn_s, "<p>Writing ROS overlay...</p>", 29, 0);
        int fd_overlay;
        int curr_ros_start_sec;
        if(cur_ros == 0)
        {
            if(cellFsOpen(ROS0_overlay_path, CELL_FS_O_RDONLY, &fd_overlay, NULL, 0) != CELL_FS_SUCCEEDED) {BEEP3; ros_patching = false; return FAILED;}
            curr_ros_start_sec = start_ros0;
        }
        else
        {
            if(cellFsOpen(ROS1_overlay_path, CELL_FS_O_RDONLY, &fd_overlay, NULL, 0) != CELL_FS_SUCCEEDED) {BEEP3; ros_patching = false; return FAILED;}
            curr_ros_start_sec = start_ros1;
        }

        u64 n_read;
        u32 n_written;
        for(int sector = 0; sector <= 0x37FF; sector++)
        {
            cellFsReadWithOffset(fd_overlay, sector * sec_size, &read_buf, sec_size, &n_read);
            sys_storage_write(flash_id, curr_ros_start_sec + sector, 1, (void *) &read_buf, &n_written);
        }
        
        send(conn_s, "<p>ROS overlay written</p>", 26, 0);
        cellFsClose(fd_overlay);
    }


    // TODO: Verify final ROS hashes.

exit_ros:
    // close & exit
    sys_storage_close(flash_id);
    cellFsClose(fd);

    if(ret) {BEEP3} else {BEEP2}

    stop_ros = ros_patching = false;
    return ret;
}
#endif //#ifdef PATCH_ROS