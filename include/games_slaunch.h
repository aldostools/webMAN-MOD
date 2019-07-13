#ifdef SLAUNCH_FILE

static bool custom_icon = false; // override default cover mode using index.ps3?cover=<cover type> (icon0, mm, disc, online)
static int create_slaunch_file(void);
static void add_slaunch_entry(int fd, const char *neth, const char *path, const char *filename, const char *icon, const char *name, const char *id, u8 f1);
static void close_slaunch_file(int fd);

typedef struct // 1MB for 2000+1 titles
{
	u8  type;
	char id[10];
	u8  path_pos; // start position of path
	u16 icon_pos; // start position of icon
	u16 padd;
	char name[508]; // name + path + icon
} __attribute__((packed)) _slaunch;

#ifdef WM_REQUEST
static bool photo_gui = true;
static bool usb0_picture_exists = false;
#endif

static int create_slaunch_file(void)
{
#ifdef WM_REQUEST
	usb0_picture_exists = photo_gui && isDir("/dev_usb000/PICTURE");
#endif
	int fd; custom_icon = false;
	if(cellFsOpen(SLAUNCH_FILE, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
		return fd;
	else
		return 0;
}

static void add_slaunch_entry(int fd, const char *neth, const char *path, const char *filename, const char *icon, const char *name, const char *id, u8 f1)
{
	if(!fd) return;

	_slaunch slaunch; memset(&slaunch, 0, sizeof(_slaunch));

	char enc_filename[MAX_PATH_LEN];

	slaunch.type = IS_ROMS_FOLDER ? TYPE_ROM : IS_PS3_TYPE ? TYPE_PS3 : IS_PSX_FOLDER ? TYPE_PS1 : IS_PS2_FOLDER ? TYPE_PS2 : IS_PSP_FOLDER ? TYPE_PSP : TYPE_VID;

#ifdef WM_REQUEST
	if(usb0_picture_exists)
	{
		u8 f1 = (slaunch.type == TYPE_PS1) ? id_PSXISO :
				(slaunch.type == TYPE_PS2) ? id_PS2ISO :
				(slaunch.type == TYPE_PS3) ? id_PS3ISO :
				(slaunch.type == TYPE_PSP) ? id_PSPISO :
				(slaunch.type == TYPE_VID) ? id_BDISO  : id_ROMS;

		sprintf(enc_filename, "%s%s/", "/dev_usb000/PICTURE/", paths[f1]); cellFsMkdir(enc_filename, DMODE);
		strcat(enc_filename, filename);
		strcat(enc_filename, strcasestr(icon, ".png") ? ".PNG" : ".JPG");

		if(file_exists(enc_filename) == false) file_copy((char*)icon, enc_filename, COPY_WHOLE_FILE);
	}
#endif

	snprintf(slaunch.id, sizeof(slaunch.id), "%s", id); urlenc_ex(enc_filename, filename, false);

	slaunch.path_pos = snprintf(slaunch.name, 128, "%s", name) + 1;
	slaunch.icon_pos = snprintf(slaunch.name + slaunch.path_pos, 454 - slaunch.path_pos, "/mount_ps3%s%s/%s", neth, path, enc_filename) + slaunch.path_pos + 1;
					   snprintf(slaunch.name + slaunch.icon_pos, 507 - slaunch.icon_pos, "%s", icon);

	cellFsWrite(fd, (void *)&slaunch, sizeof(_slaunch), NULL);
}

static void close_slaunch_file(int fd)
{
	if(!fd) return;

	cellFsClose(fd);
	cellFsChmod(SLAUNCH_FILE, MODE);
}
#endif
