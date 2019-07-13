#ifndef _COBRA_H
#define _COBRA_H

#ifdef __cplusplus
extern "C" {
#endif

enum DiscEmu
{
	EMU_OFF = 0,
	EMU_PS3,
	EMU_PS2_DVD,
	EMU_PS2_CD,
	EMU_PSX,
	EMU_BD,
	EMU_DVD,
	EMU_MAX,
	EMU_PSP,   // not a DiscEmu
	EMU_ROMS,  // not a DiscEmu
	EMU_GAMEI, // not a DiscEmu
};

#ifdef COBRA_ONLY

enum DiscType
{
	DISC_TYPE_NONE, /* No disc inserted */
	DISC_TYPE_PS3_BD, /* A PS3 game or game hybrid BD */
	DISC_TYPE_PS3_DVD, /* A PS3 DVD. This concept only exists in debug consoles */
	DISC_TYPE_PS2_DVD,
	DISC_TYPE_PS2_CD,
	DISC_TYPE_PSX_CD,
	DISC_TYPE_BDROM, /* Original non PS3 BD */
	DISC_TYPE_BDMR_SR, /* BD-R sequential record */
	DISC_TYPE_BDMR_RR, /* BD-R random record */
	DISC_TYPE_BDMRE, /* BD-RE */
	DISC_TYPE_DVD, /* Any kind of DVD (ROM, +-R, +-RW, etc) that is not a PS game. To distinguish between types, use cobra_get_disc_phys_info to check booktype */
	DISC_TYPE_CD, /* Any kind of CD (ROM, CDDA, -R, -RW) that is not a PS game. No idea how to distinguish the type :) */
	DISC_TYPE_UNKNOWN /* You shouldn't see this value. There is a posibility of this value to be reported on SCDA, since I don't have any, I haven't been able to verify */
};
/*
enum BookType
{
	DVD_ROM,
	DVD_RAM,
	DVD_MR,
	DVD_MRW,
	HD_DVD_ROM,
	HD_DVD_RAM,
	HD_DVD_MR,
	RESERVED0,
	RESERVED1,
	DVD_PRW,
	DVD_PR,
	RESERVED2,
	RESERVED3,
	DVD_PRW_DL,
	DVD_PR_DL,
	RESERVED4
};
*/
enum PS2EmuType
{
	PS2_EMU_HW, // the so called "full" bc models, CECHA and CECHB
	PS2_EMU_GX, // the so called partial bc models, CECHC and CECHE
	PS2_EMU_SW, // the non bc models
};
/*
enum DVDRegion
{
	DVD_VIDEO_REGION_1 = 1,
	DVD_VIDEO_REGION_2 = 2,
	DVD_VIDEO_REGION_3 = 4,
	DVD_VIDEO_REGION_4 = 8,
	DVD_VIDEO_REGION_5 = 16,
	DVD_VIDEO_REGION_6 = 32,
	DVD_VIDEO_REGION_MAX = DVD_VIDEO_REGION_6
};

enum BDRegion
{
	BD_VIDEO_REGION_A = 1,
	BD_VIDEO_REGION_B = 2,
	BD_VIDEO_REGION_C = 4,
	BD_VIDEO_REGION_MAX = BD_VIDEO_REGION_C
};

enum CobraLed
{
	COBRA_LED_NONE,
	COBRA_LED_BLUE,
	COBRA_LED_GREEN,
	COBRA_LED_RED = 4
};

enum PSPEmu
{
	EMU_AUTO,
	EMU_355,
	EMU_400
};
*/
typedef struct
{
	uint32_t lba;
	int is_audio;
} TrackDef;

/*
// This structure represents the Disc structure format 0 as defined in MMC-6 document
typedef struct
{
	uint8_t disc_category : 4; // one of BookType
	uint8_t part_version : 4;
	uint8_t disc_size : 4; // 0 -> 120 mm, 1 -> 80 mm
	uint8_t max_rate : 4;
	uint8_t reserved0 : 1;
	uint8_t num_layers : 2; // 0 -> 1 layer, 1 -> 2 layers
	uint8_t track_path : 1; // 0 -> PTP, 1 -> OTP
	uint8_t layer_type : 4;
	uint8_t linear_density : 4;
	uint8_t track_density : 4;
	uint8_t zero0;
	uint8_t start_phys_sector[3];
	uint8_t zero1;
	uint8_t end_phys_sector[3];
	uint8_t zero2;
	uint8_t end_phys_sector_layer0[3];
	uint8_t bca_present : 1;
	uint8_t reserved1 : 7;
} __attribute__((packed)) DiscPhysInfo;
*/
typedef struct
{
	uint16_t size; // size of structure, this will be set automatically by the library
	uint16_t checksum; // Only for core, don't mess with this
	uint8_t bd_video_region; // One of BDRegion, or 0 for default
	uint8_t dvd_video_region; // One of DVDRegion or 0 for default
	uint8_t ps2softemu; // Non-zero to show ps2 discs in non bc consoles
	uint32_t spoof_version; // version in BCD, eg. 0x0484, Max value: 0x0999 // feature is unavailable (use SEN Enabler for version spoofer)
	uint32_t spoof_revision; // revision number (decimal), MAx value: 99999 // feature is unavailable (use SEN Enabler for version spoofer)
	uint8_t fan_speed; // 0 = SYSCON, 1 = Dynamic Fan Controller, 0x33 to 0xFF = Set manual fan speed
} __attribute__((packed)) CobraConfig;

/*
 * Inits the cobra library. Call this function before any other
 *
 * Return values: 0 on sucess, ENOSYS if not in Cobra.
 */
//int cobra_lib_init(void);


/*
 * Finalizes the cobra library. Call this function before exiting the program.
 *
 * Return values: 0 on sucess, ENOSYS if not in Cobra.
 */
//int cobra_lib_finalize(void);


/*
 * Gets the disc type, one of the values in DiscType enum.
 *
 * real_disctype (out): the type of the disc that there is really in the tray. Only Cobra core and the user of this function know this value
 * effective_disctype (out): the type of the disc that the system (kernel, vsh, etc) "thinks" is inside.
 * iso_disctype (out): the disc type of the iso mounted, DISC_TYPE_NONE if no iso is mounted
 *
 * Return values: 0 on sucess, ENOSYS if not in Cobra.
 *
 * Remarks:
 * Any of the three params can be NULL if you don't need their values.
 *
 * Examples of returned values:
 *
 * There is a PS3 game in the tray and no iso mounted -> rdt = DISC_TYPE_PS3_BD, eft = DISC_TYPE_PS3_BD, idt = DISC_TYPE_NONE
 * There is a PSX original game in the tray and a iso mounted as normal DVD -> rdt = DISC_TYPE_PSX_CD, edt = DISC_TYPE_DVD, idt = DISC_TYPE_DVD
 * There is no disc in the tray and a PS2 DVD iso is mounted discless -> rdt = DISC_TYPE_NONE, edt = DISC_TYPE_PS2_DVD, idt = DISC_TYPE_PS2_DVD
 * There is a PS2 CD-R in the tray and no iso mounted -> rdt = DISC_TYPE_CD, eft = DISC_TYPE_PS2_CD, idt = DISC_TYPE_NONE
 * There is no disc and no iso is mounted either -> rdt = DISC_TYPE_NONE, edt = DISC_TYPE_NONE, idt = DISC_TYPE_NONE
 * There is no disc, a BD iso is mounted, but disc state is not inserted -> rdt = DISC_TYPE_NONE, edt = DISC_TYPE_NONE, idt = DISC_TYPE_BDMR_SR
 *
 * Additional info:
 * if an iso is mounted and disc is in inserted state, effective_disctype and iso_disctype are guaranteed to have same value.
 * if disc is in not inserted state and an iso is mounted, then effective_disctype is DISC_TYPE_NONE, but iso_disctype still preserves its value.
 * In any case, iso_disctype is guaranteed to not be DISC_TYPE_NONE if there is an iso mounted, regardles of insert state.
 *
 * Even if there is a real disc in the tray, if state is not inserted, then real_disctype is DISC_TYPE_NONE.
 *
 * If there is a disc in the tray, and no iso is mounted, real_disctype and effective_disctype are guaranteed to have same values except in the following 3 cases:
 * PSX CD-R(W): rdt = DISC_TYPE_CD, edt = DISC_TYPE_PSX_CD
 * PS2 CD-R(W): rdt = DISC_TYPE_CD, edt = DISC_TYPE_PS2_CD
 * PS2 DVD+-R(W): rdt = DISC_TYPE_DVD, edt = DISC_TYPE_PS2_DVD
 *
 * Exception  to those three rules happen when disc is not auhenticated, then edt could still be DISC_TYPE_CD or DISC_TYPE_DVD.
 * See the readme for the authentification problem info on PSX and PS2 not original discs, and the workaround.
 */
int cobra_get_disc_type(unsigned int *real_disctype, unsigned int *effective_disctype, unsigned int *iso_disctype);


/*
 * Issues the apropiated disc auth for the current real disc
 *
 * Return values:
 * 0 -> success
 * ENODEV -> there is not a real disc
 * ENOSYS -> not in cobra
 *
 * Remarks: call this function only when necessary, e.g. when you detect a disc change.
 */
int cobra_disc_auth(void);


/*
 * Issues a fake disc eject event to the system.
 * This function doesn't do any hardware access.
 *
 * Return values:
 * 0 -> success
 * ENOSYS -> not in cobra
 *
 * Additional info: the fake eject event is sent regardless of if there is a real or effective disc
 */
int cobra_send_fake_disc_eject_event(void);


/*
 * Issues a fake disc insert event to the system.
 * This function doesn't do any hardware access.
 *
 * Return values:
 * 0 -> success
 * ENOSYS -> not in cobra
 *
 * Additional info: the fake insert event is sent regardless of if there is a real or effective disc
 */
int cobra_send_fake_disc_insert_event(void);


/*
 * Mounts a ps3 disc image. This function should be called with the disc ejected or fake ejected
 *
 * files (in): path(s) to the file(s) containing the disc image parts.
 * Cobra core doesn't restrict the names in anyways, although Cobra USB Manager uses .0, .1, ... convention.
 * More over, cobra core doesn't even check that all files are on same device.
 * Each file must have a size greater than 4K. There is no upper limit other than the one in the FS
 * Cobra USB Manager uses 0xFFFF0000 sized parts (except the last)
 *
 * num (in): the number of part files, a number >= 1 and <= 32
 *
 * Return values:
 * 0 -> success
 * EINVAL -> An error in parameters
 * EBUSY -> An iso (of any type) is already mounted.
 * ENOSYS -> Not in cobra
 * Other -> An error from the kernel, e.g. when a file doesn't exist.
 *
 * Additional info: upon succesful completion of this function, and a later insert (or fake insert) event, effective_disctype will be DISC_TYPE_PS3_BD
 *
 * Discless mode is chosen by cobra core: if a disc is inserted, even if not ready yet, cobra core will choose disc mode. Otherwise, it will use discless mode.
 * In disc mode, an eject causes the system to unmount the disc. However the iso itself remains mounted in cobra core, and if a disc is inserted again, the iso
 * becomes effective again.
 *
 * See readme about how to properly mount an iso.
 */
int cobra_mount_ps3_disc_image(char *files[], unsigned int num);


/*
 * Mounts a DVD disc image. This function should be called with the disc ejected or fake ejected
 *
 * files (in): path(s) to the file(s) containing the disc image parts.
 * Cobra core doesn't restrict the names in anyways, although Cobra USB Manager uses .0, .1, ... convention.
 * More over, cobra core doesn't even check that all files are on same device.
 * Each file must have a size greater than 4K. There is no upper limit other than the one in the FS
 * Cobra USB Manager uses 0xFFFF0000 sized parts (except the last)
 *
 * num (in): the number of part files, a number >= 1 and <= 32
 *
 * Return values:
 * 0 -> success
 * EINVAL -> An error in parameters
 * EBUSY -> An iso (of any type) is already mounted.
 * ENOSYS -> Not in cobra
 * Other -> An error from the kernel, e.g. when a file doesn't exist.
 *
 * Additional info: upon succesful completion of this function, and a later insert (or fake insert) event, effective_disctype will be DISC_TYPE_DVD
 *
 * Discless mode is chosen by cobra core: if a disc is inserted, even if not ready yet, cobra core will choose disc mode. Otherwise, it will use discless mode.
 * In disc mode, an eject causes the system to unmount the disc. However the iso itself remains mounted in cobra core, and if a disc is inserted again, the iso
 * becomes effective again.
 *
 * See readme about how to properly mount an iso.
 */
int cobra_mount_dvd_disc_image(char *files[], unsigned int num);


/*
 * Mounts a bluray disc image. This function should be called with the disc ejected or fake ejected
 *
 * files (in): path(s) to the file(s) containing the disc image parts.
 * Cobra core doesn't restrict the names in anyways, although Cobra USB Manager uses .0, .1, ... convention.
 * More over, cobra core doesn't even check that all files are on same device.
 * Each file must have a size greater than 4K. There is no upper limit other than the one in the FS
 * Cobra USB Manager and genps3iso use 0xFFFF0000 sized parts (except the last) In any case, a multiple of 0x10000 is desired for perfomance reasons.
 *
 * num (in): the number of part files, a number >= 1 and <= 32
 *
 * Return values:
 * 0 -> success
 * EINVAL -> An error in parameters
 * EBUSY -> An iso (of any type) is already mounted.
 * ENOSYS -> Not in cobra
 * Other -> An error from the kernel, e.g. when a file doesn't exist.
 *
 * Additional info: upon succesful completion of this function, and a later insert (or fake insert) event, effective_disctype will be :
 * DISC_TYPE_BDMR_SR -> if there is no disc in the tray or the disc inside is not a BD-R/BD-RE
 * same as real_disctype -> if the disc in the tray is a BD-R/BD-RE
 *
 * Discless mode is chosen by cobra core: if a disc is inserted, even if not ready yet, cobra core will choose disc mode. Otherwise, it will use discless mode.
 * In disc mode, an eject causes the system to unmount the disc. However the iso itself remains mounted in cobra core, and if a disc is inserted again, the iso
 * becomes effective again.
 *
 * See readme about how to properly mount an iso.
 */
int cobra_mount_bd_disc_image(char *files[], unsigned int num);


/*
 * Mounts a psx disc image. This function should be called with the disc ejected or fake ejected.
 *
 * file (in): the file containing the disc image. Currently only 2352 isos are supported.
 * tracks (in): an array containing the track definitions. Although no checks are done explicitly by this function, cobra core only works with isos where first track
 * starts at lba 0 and where all tracks, except the first one, are audio.
 * num_tracks (in): the number of tracks of the image. A number between 1 and 99.
 *
 * Return values:
 * 0 -> success
 * EINVAL -> An error in parameters
 * EBUSY -> An iso (of any type) is already mounted.
 * ENOSYS -> Not in cobra
 * Other -> An error from the kernel, e.g. when the file doesn't exist.
 *
 * Additional info: upon succesful completion of this function, and a later insert (or fake insert) event, effective_disctype will be DISC_TYPE_PSX_CD
 *
 * Discless mode is chosen by cobra core: if a disc is inserted, even if not ready yet, cobra core will choose disc mode. Otherwise, it will use discless mode.
 * In disc mode, an eject causes the system to unmount the disc. However the iso itself remains mounted in cobra core, and if a disc is inserted again, the iso
 * becomes effective again.
 *
 * See readme about how to properly mount an iso and specific details about psx.
 */
int cobra_mount_psx_disc_image_iso(char *file, TrackDef *tracks, unsigned int num_tracks);
int cobra_mount_psx_disc_image(char *file, TrackDef *tracks, unsigned int num_tracks);


/*
 * Mounts a ps2 cd or dvd image. This function should be called with the disc ejected or fake ejected.
 *
 * files (in): the file(s) containing the disc image (ccurrently only 1). Iso can be 2048 or 2352.
 * num (in): the number of files. Currently, only 1 can be specified, as multipart is not yet supported.
 * tracks (in): an array containing the track definitions.
 * num_tracks (in): the number of tracks of the image, a number between 1 and 99. If the iso is a 2048 one, pass 1 here and NULL in tracks.
 *
 * Return values:
 * 0 -> success
 * EINVAL -> An error in parameters
 * EBUSY -> An iso (of any type) is already mounted.
 * ENOSYS -> Not in cobra
 * Other -> An error from the kernel, e.g. when the file doesn't exist.
 *
 * Additional info: upon succesful completion of this function, and a later insert (or fake insert) event, effective_disctype will be DISC_TYPE_PS2_CD or DISC_TYPE_PS2_DVD
 * If num_tracks > 1, a 2352 iso and DISC_TYPE_PS2_CD are assumed.
 * Otherwise, if there is only one track, a check is done to determine if it is a 2352 or 2048 one. If it is a 2352 one, DISC_TYPE_PS2_CD is assumed, otherwise
 * if it is a 2048 iso, the core determines if it is a DVD or CD iso by using other methods.
 *
 * See readme about how to properly mount an iso.
 */
int cobra_mount_ps2_disc_image(char *files[], int num, TrackDef *tracks, unsigned int num_tracks);


/*
 * Umounts the currently mounted disc image. This function should be called with the disc ejected or fake ejected.
 *
 * Return values:
 * 0 -> success
 * ENODEV -> no iso mounted
 * ENOSYS -> Not in cobra
 *
 * See readme about how to properly umount an iso.
 */
int cobra_umount_disc_image(void);


/*
 * Reads ps3 sectors. Encrypted sectors are decrypted.
 *
 * buf (out): buffer that receives the data. It should have at least a size of count*2048
 * sector (in): the start sector to read
 * count (in): the number of sectors to read
 *
 * Return values:
 * 0 -> success
 * ENOSYS -> Not in cobra
 * Other -> An error from the kernel
 *
 * Additional info: this function requires that the ps3 disc filesystem is mounted in /dev_bdvd
 * If the disc is not authentificated, encrypted sectors will remain encrypted.
 */
//int cobra_read_ps3_disc(void *buf, uint64_t sector, uint32_t count);


/*
 * Gets physical info about a disc.
 *
 * handle (in): a handle returned by sys_storage_open
 * layer (in): the layer to query the phys info. 0 for first layer, 1 for second layer.
 * info (out): the returned physical info
 *
 * Return values:
 * 0 -> success
 * EINVAL -> invalid params
 * Other -> An error from the kernel
 */
//int cobra_get_disc_phys_info(uint32_t handle, uint8_t layer, DiscPhysInfo *info);


/*
 * Gets the tracks definition of a CD
 *
 * handle (in): a handle returned by sys_storage_open
 * tracks (out): the tracks definitions of the CD
 * max_tracks (in): the max number of tracks that the buffer pointed by tracks can hold
 * num_tracks (out): the number of tracks of the CD, or max_tracks if  the number of tracks is greater than max_tracks
 * lba_end (out): the end lba start of the CD. Actually this value is not the final lba, but the final lba + 1. With lba_end-tracks[0].lba you can get the size of the CD in sectors
 *
 * Return values:
 * 0 -> success
 * EINVAL -> invalid params
 * ENOTSUP -> the track info returned by the driver is odd and cannot be processed.
 * Other -> An error from the kernel
 */
//int cobra_get_cd_td(uint32_t handle, TrackDef *tracks, unsigned int max_tracks, unsigned int *num_tracks, uint32_t *lba_end);


/*
 * Reads raw sectors from a CD
 *
 * handle (in): a handle returned by sys_storage_open
 * buf (out): buffer that receives the data. It should have at least a size of count*2352
 * sector (in): the start sector to read
 * count (in): the number of sectors to read
 * is_audio (in): 0 if the track we are currently reading is not an audio track. Non zero otherwise.
 * num_errors (out): the number of sectors that couldn't be read and were zeroed. This value only has sense if return value is EIO.
 *
 * Return values:
 * 0 -> success
 * EINVAL -> invalid params
 * ENODEV -> there is no disc or it was removed while calling the function
 * EIO -> there were some io errors in some sectors. When this value is returned, check num_errors to know how many errors happened
 *
 * Additional info: it is very important that sector+count doesn't cross the boundary of a track. This is mainly important when switching from a type of track to another.
 * (audio -> data, data->audio, etc)
 * This function already does sector by sector retry, in concrete it does 10 retries. When all retries for a specific sector fail, num_errors is increased and the buf
 * data for that sectors is zeroed. IF num_errors >= 1, EIO is returned.
 * Cobra USB manager currently continues dumping the disc despite of that. It would be probably a better solution to report at the end the total number of errors.
 * Apprently, a small number of errors is something normal.
 */
//int cobra_cd_read(uint32_t handle, void *buf, uint32_t sector, uint32_t count, int is_audio, int *num_errors);


/*
 * Parses the data from a cue text file and converts it to tracks definition.
 * Currently, only cue supported for psx/ps2 cd emulation are properly passed, any other will return ENOTSUP.
 *
 * cue (in): a buffer with the full data of the cue text file.
 * size (in): the size of cue buffer
 * tracks (out): the tracks definition.
 * max_tracks (in): the max number of tracks that the buffer pointed by tracks can hold
 * num_tracks (out): the number of tracks of the CD, or max_tracks if  the number of tracks is greater than max_tracks
 * filename (out): on success, filename will contain the filename that was specified in the cue after FILE directive. If the content of the cue specified a full path,
 * only the final part of the path, this is, the filename, will be returned
 * fn_size (in): the maximum number of characters, not including NULL one, that filename can hold
 *
 * Return values:
 * 0 -> success
 * EINVAL -> invalid parameter
 * ENOTSUP -> the cue is not valid for cobra psx/ps2 cd emulation and has not been parsed
 * EABORT -> a parse error happened
 *
 * Additional info: if the cue file contained wav files paths for audio tracks, the function may succeed, but those tracks won't be parsed.
 */
//int cobra_parse_cue(void *cue, uint32_t size, TrackDef *tracks, unsigned int max_tracks, unsigned int *num_tracks, char *filename, unsigned int fn_size);


/*
 * Creates a cue file from tracks definition
 *
 * path (in): the path to the output file
 * filename (in): the filename to put in the "FILE" section (it should be the bin filename)
 * tracks (in): the tracks definition
 * num_tracks (in): number of tracks
 *
 * Return values:
 * 0 -> success
 * EINVAL -> invalid parameter
 * Other -> error from the kernel
 */
//int cobra_create_cue(char *path, char *filename, TrackDef *tracks, unsigned int num_tracks);


/*
 * Creates a mds file from the size and the physical info of a disc
 *
 * path (in): the path to the output file
 * size_in_sectors (in): the size in sectors of the disc
 * layer0 (in): the physical information of layer 0
 * layer1 (in): the physical information of layer 1
 *
 * Return values:
 * 0 -> success
 * EINVAL -> invalid parameter
 * Other -> error from the kernel
*/
//int cobra_create_mds(char *path, uint64_t size_in_sectors, DiscPhysInfo *layer0, DiscPhysInfo *layer1);


/*
 * Maps a game in jailbreak directory format.
 *
 * path (in): the path to the directory with the game. If path is NULL, the curent game (if any) will be unmapped instead
 * title_id (in): the title_id of the game. If path is NULL, this value is ignore, otherwise it must be a 9 characters string. E.g. BLUS00001
 * special_mode (out): if special mode has been used, 1 will be returned here. 0 otherwise. If path is NULL, this variable is unused.
 *
 * Return values:
 * 0 -> success
 * EINVAL -> invalid parameter
 * ENOSYS -> Not in cobra
 *
 * Additional info: if path is not NULL, this function maps /dev_bdvd and /app_home to path
 * Additionally, if at the moment of calling this function, there is not a real disc, cobra will mount a dummy iso for the special discless mode.
 * Be sure to umount any iso before calling this function. Otherwise if it cannot mount the iso, the function will still succeed but special discless mode won't be used
 * Please note that this function may internally issue a fake_insert.
 *
 * Note for process spawn: it is possible that when you use the spawn function to direct boot a game, if you detect that there is no disc, you use /app_home/... path
 * to launch EBOOT.BIN. You should check in that case the value of special_mode: if true, you should use /dev_bdvd path instead for better compatibility.
 *
 * When the game is unmapped, this function remaps /app_home to /dev_usb000 instead of deleting the entry
 */
int cobra_map_game(const char *path, const char *title_id, int *special_mode);


/*
 * Maps several paths to others
 *
 * paths (in): an array with the original paths. If NULL, this function unmaps all paths that were mapped with this function
 * new_paths (in): an array with the new paths. If paths is NULL, this param is ignored, otherwise it cannot be NULL
 * num (in): the number of paths in both arrays. If paths is NULL, this param is ignored
 *
 * Return values (in):
 * 0 -> success
 * EINVAL -> invalid parameter
 * ENOSYS -> not in cobra or prior to cobra 4.2/5.0
 *
 * Additional info: cobra core evaluates the paths in the reverse order they were inserted in the table.
 * Assume that we want to map the following files:
 * /dev_bdvd/USRDIR/Dir1/Data.bin to /dev_hdd0/Data.bin
 * /dev_bdvd/USRDIR/Dir1  to /dev_hdd0/Dir1
 *
 * If you do the following:
 * char *paths[2], *new_paths[2];
 * paths[0] = "/dev_bdvd/USRDIR/Dir1/Data.bin", new_paths[0] = "/dev_hdd0/Data.bin";
 * paths[1] = "/dev_bdvd/USRDIR/Dir1", new_paths[1] = "/dev_hdd0/Dir1";
 * cobra_map_paths(paths, new_paths, 2);
 *
 * If the system tries to access "/dev_bdvd/USRDIR/Dir1/Data.bin", as paths[1] is evaluated first, the path would be
 * mapped to /dev_hdd0/Dir1/Data.bin, that is not the desired action.
 * The correct call would be:
 * paths[0] = "/dev_bdvd/USRDIR/Dir1", new_paths[0] = "/dev_hdd0/Dir1";
 * paths[1] = "/dev_bdvd/USRDIR/Dir1/Data.bin", new_paths[1] = "/dev_hdd0/Data.bin";
 * cobra_map_paths(paths, new_paths, 2);
 *
 * When mapping files of a game in jailbreak format, it is recommended to call this function before cobra_map_game
 * Setting paths to NULL ensures that all entries that were once added to the path table using this function are removed.
 * But those that were added using syscall35 will remain.
 *
 * If path is not NULL, and any of the paths in the "paths" array is NULL, EINVAL is returned.
 * If path is not NULL, and any of the paths in the "new_paths" array is NULL, that specific entry is unmapped instead of mapped.
 */
//int cobra_map_paths(char *paths[], char *new_paths[], unsigned int num);


/*
 * Sets the umd iso for psp emulation and sets the apropiated emulation parameters.
 *
 * path (in): the path to the iso. There is no support for splitted psp isos.
 * umd_root (in): if umd_root is NULL, this function will mount temporally the umd iso as a DVD iso, to read some files and set some emulation parameters, and upon
 * exit the iso is unmounted. If this param is not NULL, it assumes that you have already done the same and that umd_root is the path to the root of the
 * filesystem (e.g. /dev_bdvd"). In that case, no iso unmount will happen automatically
 * icon_save_path (in): path where to copy the icon0.png of the psp iso. Cobra USB manager uses a file called temp_icon.png in its own USRDIR directory, you may do
 * something alike.
 *
 * Return values:
 * 0 -> success
 * EINVAL -> invalid parameter
 * ENOSYS -> not in cobra or in old cobra version
 * EABORT -> PSP Launcher is not installed on the system. You should show specific error to the user in this case.
 * EBUSY -> umd_root is NULL and the iso cannot be mounted temporally because there is already one mounted in the system
 * EIO -> the function cannot read some files, for example, if the iso is corrupted or totally invalid, or if you specified a bad path in umd_root
 * Other -> an error from the kernel
 *
 * Additional info, if umd_root is not null, fake eject (some times) and fake insert events will happen.
 * Upon success, the file in icon_save_path will have the psp icon and the icon of psp launcher is mapped to it.
 * This function allocates temporally a 512 KB buffer, so at least that memory should be available
 */
int cobra_set_psp_umd(char *path, char *umd_root, char *icon_save_path);

/*
 * Sets the umd iso for psp emulation and sets the apropiated emulation parameters.
 *
 * path (in): the path to the iso. There is no support for splitted psp isos.
 * umd_root (in): if umd_root is NULL, this function will mount temporally the umd iso as a DVD iso, to read some files and set some emulation parameters, and upon
 * exit the iso is unmounted. If this param is not NULL, it assumes that you have already done the same and that umd_root is the path to the root of the
 * filesystem (e.g. /dev_bdvd"). In that case, no iso unmount will happen automatically
 * icon_save_path (in): path where to copy the icon0.png of the psp iso. Cobra USB manager uses a file called temp_icon.png in its own USRDIR directory, you may do
 * something alike.
 *
 * Return values:
 * 0 -> success
 * EINVAL -> invalid parameter
 * ENOSYS -> not in cobra or in old cobra version
 * EABORT -> PSP Launcher is not installed on the system. You should show specific error to the user in this case.
 * EBUSY -> umd_root is NULL and the iso cannot be mounted temporally because there is already one mounted in the system
 * EIO -> the function cannot read some files, for example, if the iso is corrupted or totally invalid, or if you specified a bad path in umd_root
 * ESYSVER -> psp launcher is installed, but needs update
 * ECANCELED -> psp launcher is installed and it is a supported version, but an important file of it is corrupted
 * Other -> an error from the kernel
 *
 * Additional info, if umd_root is not null, fake eject (some times) and fake insert events will happen.
 * Upon success, the file in icon_save_path will have the psp icon and the icon of psp launcher is mapped to it.
 * This function allocates temporally a 512 KB buffer, so at least that memory should be available
 */
//int cobra_set_psp_umd2(char *path, char *umd_root, char *icon_save_path, uint64_t options);


/*
 * Unsets the current umd iso (if any)
 * It also unmaps the psp launcher icon (if mapped)
 *
 * Return values:
 * 0 -> success
 * ENOSYS -> not in cobra or in old cobra version
 *
 * Additional info: the function succeeds even if there is no psp iso set.
 */
int cobra_unset_psp_umd(void);


/*
 * Gets an storage usb device name, just like they show in XMB.
 *
 * mount_point (in): the mount point of the usb device
 * dev_name (out): variable that receives the usb device name. It should have at least a size of 17 bytes.
 *
 * Return values:
 * 0 -> success
 * Other -> error from the kernel
 *
 * Additional info: don't call this function in a loop, it has some cost.
 */
//int cobra_get_usb_device_name(char *mount_point, char *dev_name);

/*
 * Gets the ps2 emu type of this console
 *
 * Return values:
 * >= 0 -> the emu type, one of PS2EmuType
 * Other -> error from the kernel
 */
int cobra_get_ps2_emu_type(void);


/*
 * Gets the cobra and ps3 firmwares versions
 *
 * cobra_version (out) -> variable that receives the cobra fw version in BCD, e.g. 3.2 ->0x0320; 4.1 -> 0x0410. This param can be NULL if not needed.
 * ps3_version (out) -> variable that receives the ps3 fw version in BCD, e.g. 3.55 -> 0x0355. This param can be NULL if not needed.
 *
 * Return values:
 * 0 -> success
 * ENOSYS -> not in cobra
 */
//int cobra_get_version(uint16_t *cobra_version, uint16_t *ps3_version);
int sys_get_version2(uint16_t *version);

/*
 * Reads the cobra configuration from RAM cache.
 *
 * cfg (out): pointer to the config structure.
 *
 * Return values:
 * 0 -> success
 * EINVAL -> invalid parameter
 * ENOSYS -> not in cobra.
 *
 * Additional info: this function succeeds regardless of cobra device being present or not.
 */
int cobra_read_config(CobraConfig *cfg);


/*
 * Writes the cobra config to the RAM cache and, if possible, to the cobra spi flash.
 *
 * Return values:
 * 0 -> success
 * EINVAL -> invalid parameter
 * ENOSYS -> not in cobra
 * ENODEV -> cobra device is not present. If this value is returned, the new cobra config will remain active until ps3 poweroff.
 *
 * Additional info: currently, none of the config fields requires a ps3 reboot to be effective.
 */
int cobra_write_config(CobraConfig *cfg);


/*
 * Controls cobra leds
 *
 * led (in): the led(s). One or more ored values of CobraLed.
 *
 * Return values:
 * 0 -> success
 * ENOSYS -> not in cobra old version of cobra
 * Other -> error from kernel, for example when cobra is not present
 */
//int cobra_led_control(unsigned int led);


/*
 * Builds the params for netiso
 *
 * param_buf (out): the pointer that receives the parameters. Pass a buf of exactly 65536 bytes.
 * server (in): the server ip/name
 * port (in): the server port
 * remote_path (in): the path of the file with the image.
 * emu_mode (in): the disc emulation mode, one of DiscEmu values.
 * num_tracks (in): number of tracks, for EMU_PSX only currently, pass 0 otherwise
 * tracks (in): the tracks, for EMU_PSX only currently, pass NULL otherwise.
 *
 * Return values:
 * 0 -> success
 * EINVAL -> invalid param(s)
 */
//int cobra_build_netiso_params(void *param_buf, char *server, uint16_t port, char *remote_path, int emu_mode, int num_tracks, TrackDef *tracks);

/*
 * Loads a module in the vsh process.
 *
 * slot (in): the slot where to load the plugin. Currently only 0 can be specified.
 * path (in): the path of the module
 * arg (in): the parameters for the module, NULL if not required. The argument is copied before the function finishes, so it can be deallocated if necessary after the call.
 * arg_size (in): the number of bytes of arg
 *
 * Return values:
 * 0 -> success
 * ENOSYS -> not in cobra or in cobra < 5.0
 * EINVAL -> invalid parameter
 * EKRESOURCE -> a module is already loaded at that slot
 * Other -> error from kernel (invalid prx, etc)
 */
int cobra_load_vsh_plugin(unsigned int slot, const char *path, void *arg, uint32_t arg_size);


/*
 * Unloads a module of vsh process, previously loaded with cobra_load_vsh_plugin
 *
 * slot (in): the slot where the plugin was loaded. Currently only 0 can be specified.
 *
 * Return values:
 * 0 -> success
 * ENOSYS -> not in cobra or in cobra < 5.0
 * EINVAL -> invalid slot
 * ENOENT -> there is no module loaded in that slot.
 */
int cobra_unload_vsh_plugin(unsigned int slot);

#ifdef __cplusplus
}
#endif

#endif /* _COBRA_H */

#endif //#ifdef COBRA_ONLY
