#define LAUNCHPAD_MODE			2
#define LAUNCHPAD_COVER_SVR		"http://gh-proxy.com/raw.githubusercontent.com/aldostools/resources/main/COV"

#ifdef LAUNCHPAD

#define LAUNCHPAD_FILE_XML		"/dev_hdd0/tmp/wm_launchpad.xml"
#define LAUNCHPAD_MAX_ITEMS		300

bool nocover_exists = false; // icon_lp_nocover.png

static void add_launchpad_header(void)
{
	const char *header = (char*)"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
								 "<nsx anno=\"\" lt-id=\"131\" min-sys-ver=\"1\" rev=\"1093\" ver=\"1.0\">\n"
								 "<spc anno=\"csxad=1&amp;adspace=9,10,11,12,13\" id=\"33537\" multi=\"o\" rep=\"t\">\n\n"; /* size: 196 */

	save_file(LAUNCHPAD_FILE_XML, header, SAVE_ALL);
}

static int add_launchpad_entry(char *buffer, const char *title, const char *url, const char *title_id, char *icon, int append)
{
	// add entry
	if(*title_id == NULL) title_id = "NOID";

	// fix &
	if(strchr(title, '&'))
	{
		char *new_title = buffer;

		size_t n = 0;
		for(size_t i = 0; title[i]; i++)
		{
			if(title[n] == '&')
			{
				n += sprintf(&new_title[n], "&amp;");
			}
			else
				new_title[n++] = title[i];
		}
		memcpy64((char *)title, new_title, n);
	}

	char *pos = strstr(icon + 22, "/icon_wm_");
	if(pos) {pos[6] = 'l', pos[7] = 'p'; if(pos[9] != 'a') {if(strstr(url, paths[3]) || strstr(url, ".ntfs[BDFILE]")) strcpy(pos + 9, "blu.png");}}

	if(*icon == NULL || ((title_id[0] == 'B' || title_id[1] == 'P') && (islike(icon, "/dev_flash") || strstr(icon, "/icon_wm_")))) sprintf(icon, "%s/IMAGES/%s", XMBMANPLS_PATH, title_id);

	if(file_exists(icon)) {urlenc(buffer, icon); sprintf(icon, "http://%s%s", local_ip, buffer);}
	else if(nocover_exists && IS(title_id, "NOID")) sprintf(icon, "http://%s%s", local_ip, WM_ICONS_PATH "/icon_lp_nocover.png");
	else concat_path2(icon, LAUNCHPAD_COVER_SVR, title_id, strstr(title_id, ".png") ? "" : ".JPG");

	int size;
	char *new_entry = buffer;

	if(append)
	{
		size = sprintf(new_entry, "<mtrl id=\"%lu\" until=\"2100-12-31T23:59:00.000Z\">\n"
									"<desc>%s</desc>\n"
									"<url type=\"2\">%s</url>\n"
									"<target type=\"u\">%s</target>\n"
									"<cntry agelmt=\"0\">all</cntry>\n"
									"<lang>all</lang></mtrl>\n\n", (1080000000UL + append), title, icon, url);

		save_file(LAUNCHPAD_FILE_XML, new_entry, -size); // append to XML
	}
	else
		size = sprintf(new_entry, "%s</desc>\n"
									"<url type=\"2\">%s</url>\n"
									"<target type=\"u\">%s</target>\n", title, icon, url);

	*icon = NULL;

	return size;
}

static void add_launchpad_extras(char *buffer, char *url)
{
	char STR_REFRESH_LP[64];
	concat_text(STR_REFRESH_LP, STR_REFRESH, "LaunchPad");

	char icon_url[MAX_PATH_LEN]; *icon_url = NULL;

	// --- launchpad extras
	sprintf(url, "http://%s/mount_ps3/unmount", local_ip);
	add_launchpad_entry(buffer, STR_UNMOUNT, url, "eject.png", icon_url, LAUNCHPAD_MAX_ITEMS + 1002);

	sprintf(url, "http://%s/setup.ps3", local_ip);
	add_launchpad_entry(buffer, STR_WMSETUP, url, "setup.png", icon_url, LAUNCHPAD_MAX_ITEMS + 1001);

	sprintf(url, "http://%s/index.ps3?launchpad", local_ip);
	add_launchpad_entry(buffer, STR_REFRESH_LP, url, "refresh.png", icon_url, LAUNCHPAD_MAX_ITEMS + 1004);
}

static void add_launchpad_footer(char *buffer)
{
	char server_url[64];
	char *icon_path = buffer;
	concat2(icon_path, WM_ICONS_PATH, "/blank.png");

	if(file_exists(icon_path))
		sprintf(server_url, "http://%s%s", local_ip, WM_ICONS_PATH);
	else
		strcopy(server_url, LAUNCHPAD_COVER_SVR);

	// --- add scroller placeholder
	u16 size = sprintf(buffer, "<mtrl id=\"1081000000\" lastm=\"9999-12-31T23:59:00.000Z\" until=\"2100-12-31T23:59:00.000Z\">\n"
								"<desc></desc>\n"
								"<url type=\"2\">%s/blank.png</url>\n"
								"<target type=\"u\"></target>\n"
								"<cntry agelmt=\"0\">all</cntry>\n"
								"<lang>all</lang></mtrl>\n\n"
								"</spc></nsx>", server_url);

	save_file(LAUNCHPAD_FILE_XML, buffer, -size); // append to XML
}

#endif //#ifdef LAUNCHPAD
