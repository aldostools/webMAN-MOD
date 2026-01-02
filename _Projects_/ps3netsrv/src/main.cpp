#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
#include <ifaddrs.h>
#endif

static const int FAILED		= -1;
static const int SUCCEEDED	=  0;
static const int NONE		= -1;

#include "color.h"
#include "common.h"
#include "compat.h"
#include "netiso.h"

#include "File.h"
#include "VIsoFile.h"

#define BUFFER_SIZE  (4 * 1048576)
#define MAX_CLIENTS  5

#define MAX_ENTRIES  4096
#define MAX_PATH_LEN 510
#define MAX_FILE_LEN 255

#define MAX_LINK_LEN 2048

#define MIN(a, b)		 ((a) <= (b) ? (a) : (b))
#define IS_RANGE(a, b, c) (((a) >= (b)) && ((a) <= (c)))
#define IS_PARENT_DIR(a)  ((a[0] == '.') && ((a[1] == 0) || ((a[1] == '.') && (a[2] == 0))))

#define MERGE_DIRS 1

enum
{
	VISO_NONE,
	VISO_PS3,
	VISO_ISO
};

typedef struct _client_t
{
	int s;
	AbstractFile *ro_file;
	AbstractFile *wo_file;
	DIR *dir;
	char *dirpath;
	uint8_t *buf;
	int connected;
	struct in_addr ip_addr;
	thread_t thread;
	uint16_t CD_SECTOR_SIZE;
	int subdirs;
} client_t;

int make_iso = VISO_NONE;

static char root_directory[MAX_PATH_LEN];

#ifndef MAKEISO
size_t root_len = 0;
static client_t clients[MAX_CLIENTS];

static int initialize_socket(uint16_t port)
{
	int s;
	struct sockaddr_in addr;

#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0)
	{
		DPRINTF("Socket creation error: %d\n", get_network_error());
		return s;
	}

	int flag = 1;
	if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&flag, sizeof(flag)) < 0)
	{
		printf("ERROR in setsockopt(REUSEADDR): %d\n", get_network_error());
		closesocket(s);
		return FAILED;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		printf("ERROR in bind: %d\n", get_network_error());
		return FAILED;
	}

	if(listen(s, 1) < 0)
	{
		printf("ERROR in listen: %d\n", get_network_error());
		return FAILED;
	}

	return s;
}

#ifndef WIN32
static int recv_all(int s, void *buf, int size)
{
	return recv(s, buf, size, MSG_WAITALL);
}
#else
// stupid windows...
static int recv_all(int s, void *buf, int size)
{
	int total_read = 0;
	char *buf_b = (char *)buf;

	while (size > 0)
	{
		int r = recv(s, buf_b, size, 0);
		if(r <= 0)
			return r;

		total_read += r;
		buf_b += r;
		size -= r;
	}

	return total_read;
}
#endif

#endif // #ifndef MAKEISO

static int normalize_path(char *path, int8_t del_last_slash)
{
	if(!path) return 0;

	char *p = path;

	while(*p)
	{
		if(*p == '\\') *p = '/';
		p++;
	}

	while(p > path)
	{
		if(*(p - 1) == '\r')
			*(--p) = 0; // remove last CR if found
		else
			break;
	}

	if(del_last_slash)
		while(p > path)
		{
			if(*(p - 1) == '/')
				*(--p) = 0; // remove last slash
			else
				break;
		}

	if(p > path)
		return (p - path);

	return 0;
}

static int initialize_client(client_t *client)
{
	memset(client, 0, sizeof(client_t));

	client->buf = (uint8_t *)malloc(BUFFER_SIZE);
	if(!client->buf)
	{
		printf("CRITICAL: Memory allocation error!\n");
		return FAILED;
	}

	client->ro_file = NULL;
	client->wo_file = NULL;
	client->dir = NULL;
	client->dirpath = NULL;
	client->connected = 1;
	client->CD_SECTOR_SIZE = 2352;
	client->subdirs = 0;

	return SUCCEEDED;
}

static void finalize_client(client_t *client)
{
	shutdown(client->s, SHUT_RDWR);
	closesocket(client->s);

	if(client->ro_file)
	{
		delete client->ro_file;
		client->ro_file = NULL;
	}

	if(client->wo_file)
	{
		delete client->wo_file;
		client->wo_file = NULL;
	}

	if(client->dir)
	{
		closedir(client->dir);
	}

	if(client->dirpath)
	{
		free(client->dirpath);
	}

	if(client->buf)
	{
		free(client->buf);
	}

	client->ro_file = NULL;
	client->wo_file = NULL;
	client->dir = NULL;
	client->dirpath = NULL;
	client->connected = 0;
	client->CD_SECTOR_SIZE = 2352;
	client->subdirs = 0;

	memset(client, 0, sizeof(client_t));
}

static int create_iso(char *folder_path, char *fileout, int viso)
{
	int ret = FAILED;
	char *filepath = NULL;
	client_t client;

	if(!folder_path || *folder_path == '\0' || !fileout || *fileout == '\0')
	{
		printf("ERROR: invalid path length for open command\n");
		goto exit_function;
	}

	initialize_client(&client);

	if(!client.buf)
	{
		printf("CRITICAL: memory allocation error\n");
		goto exit_function;
	}

	filepath = (char *)malloc(MAX_PATH_LEN + strlen(folder_path) + 1);
	if(!filepath)
	{
		printf("CRITICAL: memory allocation error\n");
		goto exit_function;
	}

	strcpy(filepath, folder_path);

	printf("output: %s\n", fileout);
	client.wo_file = new File();

	if(strstr(filepath, ".iso") || strstr(filepath, ".ISO"))
	{
		client.ro_file = new File();
		viso = VISO_NONE;
	}
	else
	{
		printf("building virtual iso...\n");
		client.ro_file = new VIsoFile((viso == VISO_PS3));
	}

	if(client.ro_file->open(filepath, O_RDONLY) < 0)
	{
		printf("open error on \"%s\" (viso=%d)\n", filepath, viso);

		delete client.ro_file;
		client.ro_file = NULL;
		goto exit_function;
	}
	else
	{
		file_stat_t st;
		if(client.ro_file->fstat(&st) < 0)
		{
			printf("fstat error on \"%s\" (viso=%d)\n", filepath, viso);

			delete client.ro_file;
			client.ro_file = NULL;
			goto exit_function;
		}
		else
		{
			file_t fd_out;
			fd_out = open_file(fileout, O_WRONLY|O_CREAT|O_TRUNC);
			if (!FD_OK(fd_out))
			{
				printf("ERROR: create error on \"%s\"\n", filepath);
				goto exit_function;
			}
			else
			{
				const uint32_t buf_size = 0x10000;
				char *buffer = (char *)client.buf;
				uint64_t offset = 0;
				uint64_t file_size = st.file_size;
				#ifdef NO_UPDATE
				if(client.ro_file->ps3Mode && (viso == VISO_NONE) && (file_size > 268435456))
					file_size -= 268435456; // truncate last 256 MB (PS3UPDAT.PUP) from encrypted ISO
				#endif
				uint64_t rem_size = file_size % buf_size;
				uint64_t iso_size = file_size - rem_size;
				uint16_t count = 0x100;
				if(iso_size >= buf_size)
				{
					for(; offset < iso_size; offset += buf_size)
					{
						if(++count >= 0x600) {count = 0; printf("Dumping ISO: offset %llu of %llu\n", (long long unsigned int)offset, (long long unsigned int)st.file_size);}
						client.ro_file->read(buffer, buf_size);
						write_file(fd_out, buffer, buf_size);
					}
				}
				if(rem_size > 0)
				{
					printf("Dumping ISO: offset %llu of %llu\n", (long long unsigned int)offset, (long long unsigned int)st.file_size);
					client.ro_file->read(buffer, rem_size);
					write_file(fd_out, buffer, rem_size);
				}
				printf("Dumped ISO: %llu bytes\n", (long long unsigned int)file_size);
			}

			close_file(fd_out);

			ret = SUCCEEDED;
		}
	}

exit_function:

	finalize_client(&client);
	
	if(filepath) free(filepath);

	return ret;
}

#ifndef MAKEISO

static char *translate_path(char *path, int *viso)
{
	if(!path) return NULL;

	size_t path_len = normalize_path(path, false);

	if(path[0] != '/')
	{
		printf("ERROR: path must start by '/'. Path received: %s\n", path);
		if(path) free(path);

		return NULL;
	}

	// check unsecure path
	char *p = strstr(path, "/..");
	if(p)
	{
		p += 3;
		if ((*p == 0) || (*p == '/'))
		{
			printf("ERROR: The path \"%s\" is unsecure!\n", path);
			if(path) free(path);
			return NULL;
		}
	}

	p = (char *)malloc(MAX_PATH_LEN + root_len + path_len + 1);
	if(!p)
	{
		printf("CRITICAL: Memory allocation error\n");
		if(path) free(path);

		return NULL;
	}

	sprintf(p, "%s%s", root_directory, path);

	if(viso)
	{
		char *q = p + root_len;

		if(strncmp(q, "/***PS3***/", 11) == 0)
		{
			path_len -= 10;
			memmove(q, q + 10, path_len + 1); // remove "/***PS3***"
			DPRINTF("p -> %s\n", p);
			*viso = VISO_PS3;
		}
		else if(strncmp(q, "/***DVD***/", 11) == 0)
		{
			path_len -= 10;
			memmove(q, q + 10, path_len + 1); // remove "/***DVD***"
			DPRINTF("p -> %s\n", p);
			*viso = VISO_ISO;
		}
		else
		{
			*viso = VISO_NONE;
		}
	}

	normalize_path(p, true);

#ifdef MERGE_DIRS
	file_stat_t st;
	if(stat_file(p, &st) < 0)
	{
		// get path only (without file name)
		char lnk_file[MAX_LINK_LEN];
		size_t p_len = root_len + path_len;
		char *filename = NULL;

		for(size_t i = p_len; i >= root_len; i--)
		{
			if((p[i] == '/') || (i == p_len))
			{
				p[i] = 0;
				sprintf(lnk_file, "%s.INI", p); // e.g. /BDISO.INI
				if (i < p_len) p[i] = '/';

				if(stat_file(lnk_file, &st) >= 0) {filename = p + i; break;}
			}
		}

		if(filename)
		{
			file_t fd = open_file(lnk_file, O_RDONLY);
			if (FD_OK(fd))
			{
				// read INI
				_memset(lnk_file, MAX_LINK_LEN);
				read_file(fd, lnk_file, MAX_LINK_LEN);
				close_file(fd);

				int flen = strlen(filename);

				// check all paths in INI
				char *dir_path = lnk_file;
				while(*dir_path)
				{
					int dlen;
					while ((*dir_path == '\r') || (*dir_path == '\n') || (*dir_path == '\t') || (*dir_path == ' ')) dir_path++;
					char *eol = strstr(dir_path, "\n"); if(eol) {*eol = 0, dlen = eol - dir_path;} else dlen = strlen(dir_path);

					char *filepath = (char *)malloc(MAX_PATH_LEN + dlen + flen + 1);

					if(filepath)
					{
						normalize_path(dir_path, true);

						// return filepath if found
						sprintf(filepath, "%s%s", dir_path, filename);
						normalize_path(filepath + dlen, true);

						if(stat_file(filepath, &st) >= 0)
						{
							if(p) free(p);
							if(path) free(path);

							return filepath;
						}
						free(filepath);
					}

					// read next line
					if(eol) dir_path = eol + 1; else break;
				}
			}
		}
	}
#endif

	if(path) free(path);

	normalize_path(p, false);

	return p;
}

static int64_t calculate_directory_size(char *path)
{
	int64_t result = 0;
	struct dirent *entry;

	//DPRINTF("Calculate %s\n", path);

	file_stat_t st;
	if(stat_file(path, &st) < 0) return FAILED;

	DIR *d = opendir(path);
	if(!d)
		return FAILED;

	size_t d_name_len, path_len;
	path_len = strlen(path);

	char *newpath = new char[path_len + MAX_FILE_LEN + 2];
	path_len = sprintf(newpath, "%s/", path);

	while ((entry = readdir(d)))
	{
		if(IS_PARENT_DIR(entry->d_name)) continue;

		#ifdef WIN32
		d_name_len = entry->d_namlen;
		#else
		d_name_len = strlen(entry->d_name);
		#endif

		if(IS_RANGE(d_name_len, 1, MAX_FILE_LEN))
		{
			//DPRINTF("name: %s\n", entry->d_name);
			sprintf(newpath + path_len, "%s", entry->d_name);

			if(stat_file(newpath, &st) < 0)
			{
				DPRINTF("calculate_directory_size: stat failed on %s\n", newpath);
				result = FAILED;
				break;
			}

			if((st.mode & S_IFDIR) == S_IFDIR)
			{
				int64_t temp = calculate_directory_size(newpath);
				if(temp < 0)
				{
					result = temp;
					break;
				}

				result += temp;
			}
			else if((st.mode & S_IFREG) == S_IFREG)
			{
				result += st.file_size;
			}
		}
	}

	delete[] newpath;
	closedir(d);
	return result;
}

// NOTE: All process_XXX function return an error ONLY if connection must be aborted. If only a not critical error must be returned to the client, that error will be
// sent using network, but the function must return 0

static int process_open_cmd(client_t *client, netiso_open_cmd *cmd)
{
	netiso_open_result result;

	char *filepath = NULL;
	int ret = FAILED, viso = VISO_NONE;
	uint16_t rlen;
	uint16_t fp_len;

	result.file_size = BE64(NONE);
	result.mtime = BE64(0);

	if(!client->buf)
	{
		printf("CRITICAL: memory allocation error\n");
		goto send_result; // return FAILED;
	}

	fp_len = BE16(cmd->fp_len);
	if(fp_len == 0)
	{
		printf("ERROR: invalid path length for open command\n");
		goto send_result; // return FAILED;
	}

	//DPRINTF("fp_len = %d\n", fp_len);
	filepath = (char *)malloc(MAX_PATH_LEN + fp_len + 1);
	if(!filepath)
	{
		printf("CRITICAL: memory allocation error\n");
		goto send_result; // return FAILED;
	}

	rlen = recv_all(client->s, (void *)filepath, fp_len);
	filepath[fp_len] = 0;

	if(rlen != fp_len)
	{
		printf("ERROR: open failed receiving filename: %d %d\n", rlen, get_network_error());
		goto send_result; // return FAILED;
	}

	if(client->ro_file)
	{
		delete client->ro_file;
		client->ro_file = NULL;
	}

	if((fp_len == 10) && (!strcmp(filepath, "/CLOSEFILE")))
	{
		ret = SUCCEEDED;
		goto send_result; // return SUCCEEDED;
	}

	filepath = translate_path(filepath, &viso);
	if(!filepath)
	{
		printf("ERROR: Path cannot be translated. Connection with this client will be aborted.\n");
		goto send_result; // return FAILED;
	}

	if(viso == VISO_NONE)
	{
		client->ro_file = new File();
	}
	else
	{
		printf("building virtual iso...\n");
		client->ro_file = new VIsoFile((viso == VISO_PS3));
	}

	rlen = 0;
	if(!strncmp(filepath, root_directory, root_len)) rlen = root_len;

	client->CD_SECTOR_SIZE = 2352;

	if(client->ro_file->open(filepath, O_RDONLY) < 0)
	{
		printf("open error on \"%s\" (viso=%d)\n", filepath + rlen, viso);

		delete client->ro_file;
		client->ro_file = NULL;
		goto send_result; // return FAILED;
	}
	else
	{
		file_stat_t st;
		if(client->ro_file->fstat(&st) < 0)
		{
			printf("fstat error on \"%s\" (viso=%d)\n", filepath + rlen, viso);

			delete client->ro_file;
			client->ro_file = NULL;
			goto send_result; // return FAILED;
		}
		else
		{
			result.file_size = BE64(st.file_size);
			result.mtime = BE64(st.mtime);

			fp_len = rlen + strlen(filepath + rlen);

			if ((fp_len > 4) && (strstr(".PNG.JPG.png.jpg.SFO", filepath + fp_len - 4) != NULL))
				; // don't cluther console with messages
			else if ((viso != VISO_NONE) || (BE64(st.file_size) > 0x400000UL))
				printf("open %s\n", filepath + rlen);

			// detect cd sector size if image is (2MB to 848MB)
			if(IS_RANGE(st.file_size, 0x200000UL, 0x35000000UL))
			{
				uint16_t sec_size[7] = {2352, 2048, 2336, 2448, 2328, 2368, 2340};

				char *buffer = (char *)client->buf;
				for(uint8_t n = 0; n < 7; n++)
				{
					client->ro_file->seek((sec_size[n]<<4) + 0x18, SEEK_SET);

					client->ro_file->read(buffer, 0xC); if(memcmp(buffer + 8, "PLAYSTATION ", 0xC) == 0) {client->CD_SECTOR_SIZE = sec_size[n]; break;}
					client->ro_file->read(buffer, 5);   if((memcmp(buffer + 1, "CD001", 5) == 0) && (buffer[0] == 0x01)) {client->CD_SECTOR_SIZE = sec_size[n]; break;}
				}

				if(client->CD_SECTOR_SIZE != 2352) printf("CD sector size: %i\n", client->CD_SECTOR_SIZE);
			}

			ret = SUCCEEDED;
		}

		#ifdef WIN32
			DPRINTF("File size: %I64x\n", st.file_size);
		#else
			DPRINTF("File size: %llx\n", (long long unsigned int)st.file_size);
		#endif
	}

send_result:

	if(filepath) free(filepath);

	if(send(client->s, (char *)&result, sizeof(result), 0) != sizeof(result))
	{
		printf("open error, send result error: %d %d\n", ret, get_network_error());
		return FAILED;
	}

	return ret;
}

static int process_read_file_critical(client_t *client, netiso_read_file_critical_cmd *cmd)
{
	if ((!client->ro_file) || (!client->buf))
		return FAILED;

	uint64_t offset = BE64(cmd->offset);
	uint32_t remaining = BE32(cmd->num_bytes);

#ifdef WIN32
	DPRINTF("Read %I64x %x\n", (long long unsigned int)offset, remaining);
#else
	DPRINTF("Read %llx %x\n", (long long unsigned int)offset, remaining);
#endif

	if(client->ro_file->seek(offset, SEEK_SET) < 0)
	{
		DPRINTF("seek_file failed!\n");
		return FAILED;
	}

	uint32_t read_size = MIN(BUFFER_SIZE, remaining);

	while (remaining > 0)
	{

		if(read_size > remaining)
		{
			read_size = remaining;
		}

		ssize_t read_ret = client->ro_file->read(client->buf, read_size);
		if ((read_ret < 0) || (static_cast<size_t>(read_ret) != read_size))
		{
			printf("ERROR: read_file failed on read file critical command!\n");
			return FAILED;
		}

		int send_ret = send(client->s, (char *)client->buf, read_size, 0);
		if ((send_ret < 0) || (static_cast<unsigned int>(send_ret) != read_size))
		{
			printf("ERROR: send failed on read file critical command!\n");
			return FAILED;
		}

		remaining -= read_size;
	}

	return SUCCEEDED;
}

static int process_read_cd_2048_critical_cmd(client_t *client, netiso_read_cd_2048_critical_cmd *cmd)
{
	if ((!client->ro_file) || (!client->buf))
		return FAILED;

	uint64_t offset = BE32(cmd->start_sector)*(client->CD_SECTOR_SIZE);
	uint32_t sector_count = BE32(cmd->sector_count);

	DPRINTF("Read CD 2048 (%i) %x %x\n", client->CD_SECTOR_SIZE, BE32(cmd->start_sector), sector_count);

	if((sector_count * 2048) > BUFFER_SIZE)
	{
		// This is just to save some uneeded code. PS3 will never request such a high number of sectors
		printf("ERROR: Too many sectors read!\n");
		return FAILED;
	}

	uint8_t *buf = client->buf;
	for (uint32_t i = 0; i < sector_count; i++)
	{
		client->ro_file->seek(offset + 24, SEEK_SET);
		if(client->ro_file->read(buf, 2048) != 2048)
		{
			printf("ERROR: read_file failed on read cd 2048 critical command!\n");
			return FAILED;
		}

		buf += 2048;
		offset += client->CD_SECTOR_SIZE; // skip subchannel data
	}

	int send_ret = send(client->s, (char *)client->buf, sector_count * 2048, 0);
	if ((send_ret < 0) || (static_cast<unsigned int>(send_ret) != (sector_count * 2048)))
	{
		printf("ERROR: send failed on read cd 2048 critical command!\n");
		return FAILED;
	}

	return SUCCEEDED;
}

static int process_read_file_cmd(client_t *client, netiso_read_file_cmd *cmd)
{
	int32_t bytes_read = NONE;
	netiso_read_file_result result;

	uint32_t read_size = BE32(cmd->num_bytes);
	uint64_t offset = BE64(cmd->offset);

	if ((!client->ro_file) || (!client->buf))
	{
		goto send_result_read_file;
	}

	if(read_size > BUFFER_SIZE)
	{
		goto send_result_read_file;
	}

	if(client->ro_file->seek(offset, SEEK_SET) < 0)
	{
		goto send_result_read_file;
	}

	bytes_read = client->ro_file->read(client->buf, read_size);
	if(bytes_read < 0)
	{
		bytes_read = NONE;
	}

send_result_read_file:

	result.bytes_read = (int32_t)BE32(bytes_read);

	if(send(client->s, (char *)&result, sizeof(result), 0) != 4)
	{
		printf("ERROR: send failed on send result (read file)\n");
		return FAILED;
	}

	if((bytes_read > 0) && (send(client->s, (char *)client->buf, bytes_read, 0) != bytes_read))
	{
		printf("ERROR: send failed on read file!\n");
		return FAILED;
	}

	return SUCCEEDED;
}

#ifndef READ_ONLY
static int process_create_cmd(client_t *client, netiso_create_cmd *cmd)
{
	netiso_create_result result;
	result.create_result = BE32(NONE);

	int created = FAILED;
	uint16_t fp_len = BE16(cmd->fp_len);

	char *filepath = (char *)malloc(MAX_PATH_LEN + fp_len + 1);
	if(!filepath)
	{
		printf("CRITICAL: memory allocation error\n");
		return FAILED;
	}

	filepath[fp_len] = 0;
	int ret = recv_all(client->s, (void *)filepath, fp_len);
	if(ret != fp_len)
	{
		printf("ERROR: recv failed, getting filename for create: %d %d\n", ret, get_network_error());
		goto send_result;
	}

	filepath = translate_path(filepath, NULL);
	if(!filepath)
	{
		printf("ERROR: Path cannot be translated. Connection with this client will be aborted.\n");
		goto send_result;
	}

	if(client->wo_file)
	{
		delete client->wo_file;
	}

	// filepath is a directory / closing file
	file_stat_t st;
	if((stat_file(filepath, &st) >= 0) && ((st.mode & S_IFDIR) == S_IFDIR))
	{
		client->wo_file = NULL;
		goto send_result;
	}

	DPRINTF("create %s\n", filepath);
	client->wo_file = new File();

	if(client->wo_file->open(filepath, O_WRONLY|O_CREAT|O_TRUNC) < 0)
	{
		printf("ERROR: create error on \"%s\"\n", filepath);
		delete client->wo_file;
		client->wo_file = NULL;
	}
	else
	{
		result.create_result = BE32(SUCCEEDED);
		created = SUCCEEDED;
	}

send_result:

	free(filepath);

	ret = send(client->s, (char *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		printf("ERROR: create, send result error: %d %d\n", ret, get_network_error());
		return FAILED;
	}

	return created;
}

static int process_write_file_cmd(client_t *client, netiso_write_file_cmd *cmd)
{
	netiso_write_file_result result;
	result.bytes_written = (int32_t)BE32(NONE);

	int bytes_written;
	uint32_t write_size = BE32(cmd->num_bytes);
	int file_written = FAILED;

	if ((!client->wo_file) || (!client->buf))
	{
		goto send_result_write_file;
	}

	if(write_size > BUFFER_SIZE)
	{
		printf("ERROR: data to write (%i) is larger than buffer size (%i)", write_size, BUFFER_SIZE);
		goto send_result_write_file;
	}

	//DPRINTF("write size: %d\n", write_size);

	if(write_size > 0)
	{
		int ret = recv_all(client->s, (void *)client->buf, write_size);
		if ((ret < 0) || (static_cast<unsigned int>(ret) != write_size))
		{
			printf("ERROR: recv failed on write file: %d %d\n", ret, get_network_error());
			goto send_result_write_file;
		}
	}

	bytes_written = client->wo_file->write(client->buf, write_size);
	if(bytes_written >= 0)
	{
		result.bytes_written = (int32_t)BE32(bytes_written);
		file_written = SUCCEEDED;
	}

send_result_write_file:

	if(send(client->s, (char *)&result, sizeof(result), 0) != 4)
	{
		printf("ERROR: send failed on send result (read file)\n");
		return FAILED;
	}

	return file_written;
}

static int process_delete_file_cmd(client_t *client, netiso_delete_file_cmd *cmd)
{
	netiso_delete_file_result result;

	uint16_t fp_len = BE16(cmd->fp_len);

	char *filepath = (char *)malloc(MAX_PATH_LEN + fp_len + 1);
	if(!filepath)
	{
		printf("CRITICAL: memory allocation error\n");
		return FAILED;
	}

	filepath[fp_len] = 0;
	int ret = recv_all(client->s, (void *)filepath, fp_len);
	if(ret != fp_len)
	{
		printf("ERROR: recv failed, getting filename for delete file: %d %d\n", ret, get_network_error());
		free(filepath);
		return FAILED;
	}

	filepath = translate_path(filepath, NULL);
	if(!filepath)
	{
		printf("ERROR: Path cannot be translated. Connection with this client will be aborted.\n");
		return FAILED;
	}

	size_t rlen = 0;
	if(!strncmp(filepath, root_directory, root_len)) rlen = root_len;

	printf("delete %s\n", filepath + rlen);

	result.delete_result = BE32(unlink(filepath));
	free(filepath);

	ret = send(client->s, (char *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		printf("ERROR: delete, send result error: %d %d\n", ret, get_network_error());
		return FAILED;
	}

	return SUCCEEDED;
}

static int process_mkdir_cmd(client_t *client, netiso_mkdir_cmd *cmd)
{
	netiso_mkdir_result result;

	uint16_t dp_len = BE16(cmd->dp_len);

	char *dirpath = (char *)malloc(MAX_PATH_LEN + dp_len + 1);
	if(!dirpath)
	{
		printf("CRITICAL: memory allocation error\n");
		return FAILED;
	}

	dirpath[dp_len] = 0;
	int ret = recv_all(client->s, (void *)dirpath, dp_len);
	if(ret != dp_len)
	{
		printf("ERROR: recv failed, getting dirname for mkdir: %d %d\n", ret, get_network_error());
		free(dirpath);
		return FAILED;
	}

	dirpath = translate_path(dirpath, NULL);
	if(!dirpath)
	{
		printf("ERROR: Path cannot be translated. Connection with this client will be aborted.\n");
		return FAILED;
	}

	size_t rlen = 0;
	if(!strncmp(dirpath, root_directory, root_len)) rlen = root_len;

	printf("mkdir %s\n", dirpath + rlen);

#ifdef WIN32
	result.mkdir_result = BE32(mkdir(dirpath));
#else
	result.mkdir_result = BE32(mkdir(dirpath, 0777));
#endif
	free(dirpath);

	ret = send(client->s, (char *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		printf("ERROR: open dir, send result error: %d %d\n", ret, get_network_error());
		return FAILED;
	}

	return SUCCEEDED;
}

static int process_rmdir_cmd(client_t *client, netiso_rmdir_cmd *cmd)
{
	netiso_rmdir_result result;

	uint16_t dp_len = BE16(cmd->dp_len);

	char *dirpath = (char *)malloc(MAX_PATH_LEN + dp_len + 1);
	if(!dirpath)
	{
		printf("CRITICAL: memory allocation error\n");
		return FAILED;
	}

	dirpath[dp_len] = 0;
	int ret = recv_all(client->s, (void *)dirpath, dp_len);
	if(ret != dp_len)
	{
		printf("ERROR: recv failed, getting dirname for rmdir: %d %d\n", ret, get_network_error());
		free(dirpath);
		return FAILED;
	}

	dirpath = translate_path(dirpath, NULL);
	if(!dirpath)
	{
		printf("ERROR: Path cannot be translated. Connection with this client will be aborted.\n");
		return FAILED;
	}

	size_t rlen = 0;
	if(!strncmp(dirpath, root_directory, root_len)) rlen = root_len;

	printf("rmdir %s\n", dirpath + rlen);

	result.rmdir_result = BE32(rmdir(dirpath));
	free(dirpath);

	ret = send(client->s, (char *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		printf("ERROR: open dir, send result error: %d %d\n", ret, get_network_error());
		return FAILED;
	}

	return SUCCEEDED;
}
#endif // #ifndef READ_ONLY

static int process_open_dir_cmd(client_t *client, netiso_open_dir_cmd *cmd)
{
	netiso_open_dir_result result;

	uint16_t dp_len = BE16(cmd->dp_len);

	char *dirpath = (char *)malloc(MAX_PATH_LEN + dp_len + 1);
	if(!dirpath)
	{
		printf("CRITICAL: memory allocation error\n");
		return FAILED;
	}

	dirpath[dp_len] = 0;
	int ret = recv_all(client->s, (void *)dirpath, dp_len);
	if(ret != dp_len)
	{
		printf("ERROR: recv failed, getting dirname for open dir: %d %d\n", ret, get_network_error());
		free(dirpath);
		return FAILED;
	}

	client->subdirs = strstr(dirpath, "//") ? 1 : 0;

	dirpath = translate_path(dirpath, NULL);
	if(!dirpath)
	{
		printf("ERROR: Path cannot be translated. Connection with this client will be aborted.\n");
		return FAILED;
	}

	if(client->dir)
	{
		closedir(client->dir);
		client->dir = NULL;
	}

	if(client->dirpath)
	{
		free(client->dirpath);
	}

	client->dirpath = NULL;

	normalize_path(dirpath, true);

	file_stat_t st;
	if(stat_file(dirpath, &st) >= 0)
		client->dir = opendir(dirpath);

	if(!client->dir)
	{
		//printf("open dir error on \"%s\"\n", dirpath);
		result.open_result = BE32(NONE);
	}
	else
	{
		uint16_t rlen = 0;
		if(!strncmp(dirpath, root_directory, root_len)) rlen = root_len;

		client->dirpath = dirpath;
		printf("open dir %s\n", dirpath + rlen);
		strcat(dirpath, "/");
		result.open_result = BE32(SUCCEEDED);
	}

	if(!client->dirpath)
	{
		free(dirpath);
	}

	ret = send(client->s, (char *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		printf("ERROR: open dir, send result error: %d %d\n", ret, get_network_error());
		return FAILED;
	}

	return SUCCEEDED;
}

#ifndef READ_ONLY
static int process_read_dir_entry_cmd(client_t *client, netiso_read_dir_entry_cmd *cmd, int version)
{
	(void) cmd;
	char *path = NULL;

	file_stat_t st;
	struct dirent *entry = NULL;
	size_t d_name_len = 0;

	netiso_read_dir_entry_result result_v1;
	netiso_read_dir_entry_result_v2 result_v2;

	if(version == 1)
	{
		memset(&result_v1, 0, sizeof(result_v1));
	}
	else
	{
		memset(&result_v2, 0, sizeof(result_v2));
	}

	if ((!client->dir) || (!client->dirpath))
	{
		if(version == 1)
		{
			result_v1.file_size = BE64(NONE);
		}
		else
		{
			result_v2.file_size = BE64(NONE);
		}

		goto send_result_read_dir;
	}

	// ignore parent dir & files larger than MAX_FILE_LEN
	while ((entry = readdir(client->dir)))
	{
		if(IS_PARENT_DIR(entry->d_name)) continue;

		#ifdef WIN32
		d_name_len = entry->d_namlen;
		#else
		d_name_len = strlen(entry->d_name);
		#endif

		if(IS_RANGE(d_name_len, 1, MAX_FILE_LEN)) break;
	}

	if(!entry)
	{
		closedir(client->dir);
		if(client->dirpath) free(client->dirpath);

		client->dir = NULL;
		client->dirpath = NULL;

		if(version == 1)
		{
			result_v1.file_size = BE64(NONE);
		}
		else
		{
			result_v2.file_size = BE64(NONE);
		}
		goto send_result_read_dir;
	}

	path = (char *)malloc(MAX_PATH_LEN + strlen(client->dirpath) + d_name_len + 2);
	if(!path)
	{
		printf("CRITICAL: memory allocation error\n");
		goto send_result_read_dir;
	}

	sprintf(path, "%s/%s", client->dirpath, entry->d_name);

	DPRINTF("Read dir entry: %s\n", path);
	if(stat_file(path, &st) < 0)
	{
		closedir(client->dir);
		if(client->dirpath) free(client->dirpath);

		client->dir = NULL;
		client->dirpath = NULL;

		if(version == 1)
		{
			result_v1.file_size = BE64(NONE);
		}
		else
		{
			result_v2.file_size = BE64(NONE);
		}

		DPRINTF("Stat failed on read dir entry: %s\n", path);
		goto send_result_read_dir;
	}

	if((st.mode & S_IFDIR) == S_IFDIR)
	{
		if(version == 1)
		{
			result_v1.file_size = BE64(0);
			result_v1.is_directory = 1;
		}
		else
		{
			result_v2.file_size = BE64(0);
			result_v2.is_directory = 1;
		}
	}
	else
	{
		if(version == 1)
		{
			result_v1.file_size = BE64(st.file_size);
			result_v1.is_directory = 0;
		}
		else
		{
			result_v2.file_size = BE64(st.file_size);
			result_v2.is_directory = 0;
		}
	}

	if(version == 1)
	{
		result_v1.fn_len = BE16(d_name_len);
	}
	else
	{
		result_v2.fn_len = BE16(d_name_len);
		result_v2.atime  = BE64(st.atime);
		result_v2.ctime  = BE64(st.ctime);
		result_v2.mtime  = BE64(st.mtime);
	}

send_result_read_dir:

	if(path) free(path);

	if(version == 1)
	{
		if(send(client->s, (char *)&result_v1, sizeof(result_v1), 0) != sizeof(result_v1))
		{
			printf("ERROR: send error on read dir entry (%d)\n", get_network_error());
			return FAILED;
		}
	}
	else
	{
		if(send(client->s, (char *)&result_v2, sizeof(result_v2), 0) != sizeof(result_v2))
		{
			printf("ERROR: send error on read dir entry (%d)\n", get_network_error());
			return FAILED;
		}
	}

	if (((version == 1) && (static_cast<uint64_t>(result_v1.file_size) != BE64(NONE))) || ((version == 2) && (static_cast<uint64_t>(result_v2.file_size) != BE64(NONE))))
	{
		int send_ret = send(client->s, (char *)entry->d_name, d_name_len, 0);
		if ((send_ret < 0) || (static_cast<unsigned int>(send_ret) != d_name_len))
		{
			printf("ERROR: send file name error on read dir entry (%d)\n", get_network_error());
			return FAILED;
		}
	}

	return SUCCEEDED;
}
#endif // #ifndef READ_ONLY

static void process_read_dir(netiso_read_dir_result_data *dir_entries, const char *dir_path, size_t dirpath_len, size_t path_len, int max_items, int *nitems, int subdirs)
{
	int items = *nitems;

	file_stat_t st;
	struct dirent *entry;
	size_t d_name_len;

	char path[MAX_PATH_LEN]; strcpy(path, dir_path);

	normalize_path(path, true);
	DIR *dir2 = opendir(path);
	strcat(path, "/");
	size_t dir2path_len = strlen(path);

	if(dir2)
	{
		while ((entry = readdir(dir2)))
		{
			if(IS_PARENT_DIR(entry->d_name)) continue;

			#ifdef WIN32
			d_name_len = entry->d_namlen;
			#else
			d_name_len = strlen(entry->d_name);
			#endif

			if(dir2path_len + d_name_len >= path_len) continue;

			sprintf(path + dir2path_len, "%s", entry->d_name);

			if(stat_file(path, &st) < 0)
			{
				st.file_size = 0;
				st.mode = S_IFDIR;
				st.mtime = 0;
				st.atime = 0;
				st.ctime = 0;
			}

			if((st.mode & S_IFDIR) == S_IFDIR)
			{
				if(subdirs)
				{
					if(subdirs < 2) process_read_dir(dir_entries, path, dirpath_len, path_len, max_items, &items, 2);

					path[dir2path_len] = '\0';
					continue;
				}

				dir_entries[items].file_size = (0);
				dir_entries[items].is_directory = 1;
			}
			else
			{
				dir_entries[items].file_size =  BE64(st.file_size);
				dir_entries[items].is_directory = 0;
			}

			if(!st.mtime) {st.mtime = st.ctime;
			if(!st.mtime)  st.mtime = st.atime;}

			dir_entries[items].mtime = BE64(st.mtime);

			if(subdirs)
				sprintf(dir_entries[items].name, "%s", path + dirpath_len);
			else
				sprintf(dir_entries[items].name, "%s", entry->d_name);

			path[dir2path_len] = '\0';

			items++;
			if(items >= max_items) break;
		}
		closedir(dir2); dir2 = NULL;
	}
	*nitems = items;
}

static int process_read_dir_cmd(client_t *client, netiso_read_dir_entry_cmd *cmd)
{
	(void) cmd;

	int items = 0;
	int max_items = MAX_ENTRIES;
	size_t path_len = 0;

	char *path = NULL;
	netiso_read_dir_result_data *dir_entries = NULL;

	netiso_read_dir_result result;
	memset(&result, 0, sizeof(result));
	result.dir_size = BE64(0);

	if ((!client->dir) || (!client->dirpath))
	{
		goto send_result_read_dir_cmd;
	}

	for (; max_items >= 0x10; max_items -= 0x10)
	{
		dir_entries = (netiso_read_dir_result_data *) malloc(sizeof(netiso_read_dir_result_data) * max_items);
		if (dir_entries) break;
	}

	if (!dir_entries)
	{
		goto send_result_read_dir_cmd;
	}

	path_len = MAX_PATH_LEN + root_len + strlen(client->dirpath + root_len) + MAX_FILE_LEN + 2;

	path = (char*)malloc(path_len);

	if (!path)
	{
		goto send_result_read_dir_cmd;
	}

	_memset(dir_entries, sizeof(netiso_read_dir_result_data) * max_items);

	file_stat_t st;

	size_t dirpath_len;
	dirpath_len = sprintf(path, "%s", client->dirpath);

	// list dir
	process_read_dir(dir_entries, path, dirpath_len, path_len, max_items, &items, client->subdirs);

	if(client->dir) {closedir(client->dir); client->dir = NULL;}

#ifdef MERGE_DIRS
	char *p;
	size_t slen;
	char *ini_file;

	// get INI file for directory
	p = client->dirpath;
	slen = dirpath_len - 1; //strlen(p);
	ini_file = path;
	for(size_t i = slen; i >= root_len; i--)
	{
		if((p[i] == '/') || (i == slen))
		{
			p[i] = 0;
			sprintf(ini_file, "%s.INI", p); // e.g. /BDISO.INI
			if(i < slen) p[i] = '/';

			if(stat_file(ini_file, &st) >= 0) break;
		}
	}

	file_t fd;
	fd = open_file(ini_file, O_RDONLY);
	if (FD_OK(fd))
	{
		// read INI
		char lnk_file[MAX_LINK_LEN];
		_memset(lnk_file, MAX_LINK_LEN);
		read_file(fd, lnk_file, MAX_LINK_LEN);
		close_file(fd);

		// scan all paths in INI
		char *dir_path = lnk_file;
		while(*dir_path)
		{
			while ((*dir_path == '\r') || (*dir_path == '\n') || (*dir_path == '\t') || (*dir_path == ' ')) dir_path++;
			char *eol = strstr(dir_path, "\n"); if(eol) *eol = 0;

			normalize_path(dir_path, true);

			// check dir exists
			if(stat_file(dir_path, &st) >= 0)
			{
				printf("-> %s\n", dir_path);
				dirpath_len = (strncmp(dir_path, root_directory, root_len) == 0) ? root_len : 0;

				process_read_dir(dir_entries, dir_path, dirpath_len, path_len, max_items, &items, client->subdirs);
			}

			// read next line
			if(eol) dir_path = eol + 1; else break;
		}
	}
#endif

send_result_read_dir_cmd:

	if(path) free(path);

	result.dir_size = BE64(items);
	if(send(client->s, (const char*)&result, sizeof(result), 0) != sizeof(result))
	{
		if(dir_entries) free(dir_entries);
		return FAILED;
	}

	if(items > 0)
	{
		if(send(client->s, (const char*)dir_entries, (sizeof(netiso_read_dir_result_data) * items), 0) != (int)(sizeof(netiso_read_dir_result_data) * items))
		{
			if(dir_entries) free(dir_entries);
			return FAILED;
		}
	}

	if(dir_entries) free(dir_entries);
	return SUCCEEDED;
}

static int process_stat_cmd(client_t *client, netiso_stat_cmd *cmd)
{
	netiso_stat_result result;

	uint16_t fp_len = BE16(cmd->fp_len);

	char *filepath = (char *)malloc(MAX_PATH_LEN + fp_len + 1);
	if(!filepath)
	{
		printf("CRITICAL: memory allocation error\n");
		return FAILED;
	}

	filepath[fp_len] = 0;
	int ret = recv_all(client->s, (char *)filepath, fp_len);
	if(ret != fp_len)
	{
		printf("ERROR: recv failed, getting filename for stat: %d %d\n", ret, get_network_error());
		free(filepath);
		return FAILED;
	}

	filepath = translate_path(filepath, NULL);
	if(!filepath)
	{
		printf("ERROR: Path cannot be translated. Connection with this client will be aborted.\n");
		return FAILED;
	}

	file_stat_t st;
	DPRINTF("stat %s\n", filepath);
	if((stat_file(filepath, &st) < 0) && (!strstr(filepath, "/is_ps3_compat1/")))
	{
		DPRINTF("ERROR: stat error on \"%s\"\n", filepath);
		result.file_size = BE64(NONE);
	}
	else
	{
		if((st.mode & S_IFDIR) == S_IFDIR)
		{
			result.file_size = BE64(0);
			result.is_directory = 1;
		}
		else
		{
			result.file_size = BE64(st.file_size);
			result.is_directory = 0;
		}

		result.mtime = BE64(st.mtime);
		result.ctime = BE64(st.ctime);
		result.atime = BE64(st.atime);
	}

	free(filepath);

	ret = send(client->s, (char *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		printf("ERROR: stat, send result error: %d %d\n", ret, get_network_error());
		return FAILED;
	}

	return SUCCEEDED;
}

static int process_get_dir_size_cmd(client_t *client, netiso_get_dir_size_cmd *cmd)
{
	netiso_get_dir_size_result result;

	uint16_t dp_len = BE16(cmd->dp_len);

	char *dirpath = (char *)malloc(MAX_PATH_LEN + dp_len + 1);
	if(!dirpath)
	{
		printf("CRITICAL: memory allocation error\n");
		return FAILED;
	}

	dirpath[dp_len] = 0;
	int ret = recv_all(client->s, (char *)dirpath, dp_len);
	if(ret != dp_len)
	{
		printf("ERROR: recv failed, getting dirname for get_dir_size: %d %d\n", ret, get_network_error());
		free(dirpath);
		return FAILED;
	}

	dirpath = translate_path(dirpath, NULL);
	if(!dirpath)
	{
		printf("ERROR: Path cannot be translated. Connection with this client will be aborted.\n");
		return FAILED;
	}

	DPRINTF("get_dir_size %s\n", dirpath);

	result.dir_size = BE64(calculate_directory_size(dirpath));
	free(dirpath);

	ret = send(client->s, (char *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		printf("ERROR: get_dir_size, send result error: %d %d\n", ret, get_network_error());
		return FAILED;
	}

	return SUCCEEDED;
}

void *client_thread(void *arg)
{
	client_t *client = (client_t *)arg;

	for(;;)
	{
		netiso_cmd cmd;
		int ret = recv_all(client->s, (void *)&cmd, sizeof(cmd));

		if(ret != sizeof(cmd))
		{
			break;
		}

		switch (BE16(cmd.opcode))
		{
			case NETISO_CMD_READ_FILE_CRITICAL:
				ret = process_read_file_critical(client, (netiso_read_file_critical_cmd *)&cmd);
			break;

			case NETISO_CMD_READ_FILE:
				ret = process_read_file_cmd(client, (netiso_read_file_cmd *)&cmd);
			break;

			case NETISO_CMD_READ_CD_2048_CRITICAL:
				ret = process_read_cd_2048_critical_cmd(client, (netiso_read_cd_2048_critical_cmd *)&cmd);
			break;

#ifndef READ_ONLY
			case NETISO_CMD_WRITE_FILE:
				ret = process_write_file_cmd(client, (netiso_write_file_cmd *)&cmd);
			break;

			case NETISO_CMD_READ_DIR_ENTRY:
				ret = process_read_dir_entry_cmd(client, (netiso_read_dir_entry_cmd *)&cmd, 1);
			break;

			case NETISO_CMD_READ_DIR_ENTRY_V2:
				ret = process_read_dir_entry_cmd(client, (netiso_read_dir_entry_cmd *)&cmd, 2);
			break;
#endif

			case NETISO_CMD_STAT_FILE:
				ret = process_stat_cmd(client, (netiso_stat_cmd *)&cmd);
			break;

			case NETISO_CMD_OPEN_FILE:
				ret = process_open_cmd(client, (netiso_open_cmd *)&cmd);
			break;

#ifndef READ_ONLY
			case NETISO_CMD_CREATE_FILE:
				ret = process_create_cmd(client, (netiso_create_cmd *)&cmd);
			break;

			case NETISO_CMD_DELETE_FILE:
				ret = process_delete_file_cmd(client, (netiso_delete_file_cmd *)&cmd);
			break;
#endif

			case NETISO_CMD_OPEN_DIR:
				ret = process_open_dir_cmd(client, (netiso_open_dir_cmd *)&cmd);
			break;

			case NETISO_CMD_READ_DIR:
				ret = process_read_dir_cmd(client, (netiso_read_dir_entry_cmd *)&cmd);
			break;

			case NETISO_CMD_GET_DIR_SIZE:
				ret = process_get_dir_size_cmd(client, (netiso_get_dir_size_cmd *)&cmd);
			break;

#ifndef READ_ONLY
			case NETISO_CMD_MKDIR:
				ret = process_mkdir_cmd(client, (netiso_mkdir_cmd *)&cmd);
			break;

			case NETISO_CMD_RMDIR:
				ret = process_rmdir_cmd(client, (netiso_rmdir_cmd *)&cmd);
			break;
#endif

			default:
				printf("ERROR: Unknown command received: %04X\n", BE16(cmd.opcode));
				ret = FAILED;
		}

		if(ret != SUCCEEDED)
		{
			break;
		}
	}

	finalize_client(client);
	return NULL;
}
#endif //#ifndef MAKEISO

int main(int argc, char *argv[])
{
#ifndef MAKEISO
	int s;
	uint16_t port = NETISO_PORT;
	uint32_t whitelist_start = 0;
	uint32_t whitelist_end   = 0;
#endif

	get_normal_color();

	// Show build number
	set_white_text();
#ifndef MAKEISO
	printf("ps3netsrv build 20260101");
	#ifdef READ_ONLY
	set_gray_text();
	printf(" [READ-ONLY]");
	#endif
#else
	printf("makeiso build 20260101");
#endif

	set_red_text();
	printf(" (mod by aldostools)\n");
	set_normal_color();

#ifndef WIN32
	if(sizeof(off_t) < 8)
	{
		printf("ERROR: off_t too small!\n");
		goto exit_error;
	}
#endif

	if(argc < 2)
	{
#ifdef MAKEISO
		printf( "\nUsage: makeiso [directory/encrypted iso] [PS3/ISO]\n");
		goto exit_error;
#else
		file_stat_t fs;
		char *filename = strrchr(argv[0], '/');
		if(!filename) filename = strrchr(argv[0], '\\');
		if( filename) filename++;

		// Use current path as default shared directory
		if( (filename != NULL) && (
			(stat_file("./PS3ISO", &fs) >= 0) ||
			(stat_file("./PSXISO", &fs) >= 0) ||
			(stat_file("./GAMES",  &fs) >= 0) ||
			(stat_file("./GAMEZ",  &fs) >= 0) ||
			(stat_file("./DVDISO", &fs) >= 0) ||
			(stat_file("./BDISO",  &fs) >= 0) ||
			(stat_file("./ROMS",   &fs) >= 0) ||
			(stat_file("./PKG",    &fs) >= 0) ||
			(stat_file("./PS3ISO.INI", &fs) >= 0) ||
			(stat_file("./PSXISO.INI", &fs) >= 0) ||
			(stat_file("./GAMES.INI",  &fs) >= 0) ||
			(stat_file("./GAMEZ.INI",  &fs) >= 0) ||
			(stat_file("./DVDISO.INI", &fs) >= 0) ||
			(stat_file("./BDISO.INI",  &fs) >= 0) ||
			(stat_file("./ROMS.INI",   &fs) >= 0) ||
			(stat_file("./PKG.INI",    &fs) >= 0) ||
			(stat_file("./PS3_NET_Server.cfg", &fs) >= 0)
			))
		{
			argv[1] = argv[0];
			*(filename - 1) = 0;
			argc = 2;
		#ifdef WIN32
			file_t fd = open_file("./PS3_NET_Server.cfg", O_RDONLY);
			if (FD_OK(fd))
			{
				char buf[2048];
				read_file(fd, buf, 2048);
				close_file(fd);

				char *path = strstr(buf, "path0=\"");
				if(path)
				{
					argv[1] = path + 7;
					char *pos  = strchr(path + 7, '"');
					if(pos) *pos = 0;
				}
			}
		#endif
		}
		else
		{
			if(!filename) filename = argv[0];

			printf( "\nUsage: %s [rootdirectory] [port] [whitelist]\n\n"
					" Default port: %d\n\n"
					" Whitelist: x.x.x.x, where x is 0-255 or *\n"
					" (e.g 192.168.1.* to allow only connections from 192.168.1.0-192.168.1.255)\n", filename, NETISO_PORT);

			goto exit_error;
		}
#endif //#ifdef MAKEISO
	}

	// Check shared directory
	if(strlen(argv[1]) >= sizeof(root_directory))
	{
		printf("Directory name too long!\n");
		goto exit_error;
	}

	strcpy(root_directory, argv[1]);
	normalize_path(root_directory, true);

	if(argc < 3)
	{
		char sfo_path[sizeof(root_directory) + 20];
		snprintf(sfo_path, sizeof(sfo_path) - 1, "%s/PS3_GAME/PARAM.SFO", root_directory);

		file_stat_t fs;
		if(stat_file(sfo_path, &fs) >= 0)
			make_iso =  VISO_PS3;
		else if(strstr(root_directory, ".iso") || strstr(root_directory, ".ISO"))
			make_iso =  VISO_ISO;
#ifdef MAKEISO
		else
			make_iso =  VISO_ISO;
#endif
	}
#ifdef MAKEISO
	else if(argc >= 3)
		make_iso =  VISO_ISO;
#endif

#ifndef MAKEISO
	// Use current path as default
	if(*root_directory == 0)
	{
		if (getcwd(root_directory, sizeof(root_directory)) != NULL)
			strcat(root_directory, "/");
		else
			strcpy(root_directory, argv[0]);

		char *filename = strrchr(root_directory, '/'); if(filename) *(++filename) = 0;
	}

	// Show shared directory
	root_len = normalize_path(root_directory, true);
	printf("Path: %s\n\n", root_directory);

	// Check for root directory
	if(strcmp(root_directory, "/") == 0)
	{
		printf("ERROR: / can't be specified as root directory!\n");
		goto exit_error;
	}

	// Parse port argument
	if(argc > 2)
	{
		uint32_t u;

		if(sscanf(argv[2], "%u", &u) != 1)
		{
			if(strstr(argv[2], "PS3") || strstr(argv[2], "ps3"))
				make_iso = VISO_PS3;
			else if(strstr(argv[2], "ISO") || strstr(argv[2], "iso"))
				make_iso = VISO_ISO;
			else
			{
				printf("Wrong port specified.\n");
				goto exit_error;
			}
		}
		else
		{
#ifdef WIN32
			uint32_t min = 1;
#else
			uint32_t min = 1024;
#endif

			if ((u < min) || (u > 65535))
			{
				printf("Port must be in %d-65535 range.\n", min);
				goto exit_error;
			}

			port = u;
		}
	}

	// Parse whitelist argument
	if(argc > 3)
	{
		char *p = argv[3];

		for (int i = 3; i >= 0; i--)
		{
			uint32_t u;
			int wildcard = 0;

			if(sscanf(p, "%u", &u) != 1)
			{
				if(i == 0)
				{
					if(strcmp(p, "*") != SUCCEEDED)
					{
						printf("Wrong whitelist format.\n");
						goto exit_error;
					}

				}
				else
				{
					if ((p[0] != '*') || (p[1] != '.'))
					{
						printf("Wrong whitelist format.\n");
						goto exit_error;
					}
				}

				wildcard = 1;
			}
			else
			{
				if(u > 0xFF)
				{
					printf("Wrong whitelist format.\n");
					goto exit_error;
				}
			}

			if(wildcard)
			{
				whitelist_end |= (0xFF<<(i*8));
			}
			else
			{
				whitelist_start |= (u<<(i*8));
				whitelist_end   |= (u<<(i*8));
			}

			if(i != 0)
			{
				p = strchr(p, '.');
				if(!p)
				{
					printf("Wrong whitelist format.\n");
					goto exit_error;
				}

				p++;
			}
		}

		DPRINTF("Whitelist: %08X-%08X\n", whitelist_start, whitelist_end);
	}

	// Initialize port
	s = initialize_socket(port);
	if(s < 0)
	{
		printf("Error in port initialization.\n");
		goto exit_error;
	}


	/////////////////
	// Show Host IP
	/////////////////
	if(make_iso == VISO_NONE)
#ifdef WIN32
	{
		char host[256];
		struct hostent *host_entry;
		int hostname = gethostname(host, sizeof(host)); //find the host name
		if(hostname != FAILED)
		{
			printf("Current Host Name: %s\n", host);
			host_entry = gethostbyname(host); //find host information
			if(host_entry);
			{
				set_gray_text();
				for(int i = 0; host_entry->h_addr_list[i]; i++)
				{
					char *IP = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[i])); //Convert into IP string
					printf("Host IP: %s:%i\n", IP, port);
				}
			}
			printf("\n");
		}
	}
#else
	{
		struct ifaddrs *addrs, *tmp;
		getifaddrs(&addrs);
		tmp = addrs;

		set_gray_text();
		int i = 0;
		while (tmp)
		{
			if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
			{
				struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
				if(!(!strcmp(inet_ntoa(pAddr->sin_addr), "0.0.0.0") || !strcmp(inet_ntoa(pAddr->sin_addr), "127.0.0.1")))
					printf("Host IP #%x: %s:%i\n", ++i, inet_ntoa(pAddr->sin_addr), port);
			}

			tmp = tmp->ifa_next;
		}

		freeifaddrs(addrs);
	}
#endif

	if(make_iso)
	{
		if(strlen(root_directory) >= MAX_PATH_LEN - 10)
		{
			printf("Path too long: %s\n", root_directory);
			goto exit_error;
		}

		char outfile[MAX_PATH_LEN];
		sprintf(outfile, "%s.iso", strrchr(root_directory, '/') + 1);

		char *pos1 = strstr(outfile, ".iso."); if(pos1) sprintf(pos1, ".new.iso");
		char *pos2 = strstr(outfile, ".ISO."); if(pos2) sprintf(pos2, ".new.iso");

		create_iso(root_directory, outfile, make_iso);
		goto exit_error;
	}

#else // if defined(MAKEISO)

	if(make_iso)
	{
		for(int i = 1; i < argc; i++)
		{
			file_stat_t st;
			if(stat_file(argv[i], &st) < 0)
			{
				printf("Invalid path: %s\n", argv[i]);
				continue;
			}
			
			if(strlen(argv[i]) >= MAX_PATH_LEN - 10)
			{
				printf("Path too long: %s\n", argv[i]);
				goto exit_error;
			}

			strcpy(root_directory, argv[i]);
			normalize_path(root_directory, true);

			char outfile[MAX_PATH_LEN];
			sprintf(outfile, "%s.iso", strrchr(root_directory, '/') + 1);

			char *pos1 = strstr(outfile, ".iso."); if(pos1) sprintf(pos1, ".new.iso");
			char *pos2 = strstr(outfile, ".ISO."); if(pos2) sprintf(pos2, ".new.iso");

			char sfo_path[sizeof(root_directory) + 20];
			snprintf(sfo_path, sizeof(sfo_path) - 1, "%s/PS3_GAME/PARAM.SFO", root_directory);

			if(stat_file(sfo_path, &st) >= 0)
				make_iso =  VISO_PS3;
			else
				make_iso =  VISO_ISO;

			create_iso(root_directory, outfile, make_iso);
		}
		goto exit_error;
	}
#endif // #ifndef MAKEISO

#ifndef MAKEISO
	//////////////
	// main loop
	//////////////
	set_normal_color();
	printf("Waiting for client...\n");
	memset(clients, 0, sizeof(clients));

	char last_ip[16], conn_ip[16];
	memset(last_ip, 0, 16);

	for (;;)
	{
		struct sockaddr_in addr;
		unsigned int size;
		int cs;
		int i;

		// accept request
		size = sizeof(addr);
		cs = accept(s, (struct sockaddr *)&addr, (socklen_t *)&size);

		if(cs < 0)
		{
			printf("Network error: %d\n", get_network_error());
			break;
		}

		// Check for same client
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if((clients[i].connected) && (clients[i].ip_addr.s_addr == addr.sin_addr.s_addr))
				break;
		}

		sprintf(conn_ip, "%s", inet_ntoa(addr.sin_addr));

		if(i < MAX_CLIENTS)
		{
			// Shutdown socket and wait for thread to complete
			shutdown(clients[i].s, SHUT_RDWR);
			closesocket(clients[i].s);
			join_thread(clients[i].thread);

			if(strcmp(last_ip, conn_ip))
			{
				printf("[%i] Reconnection from %s\n",  i, conn_ip);
			}
		}
		else
		{
			// Check whitelist range
			if(whitelist_start)
			{
				uint32_t ip = BE32(addr.sin_addr.s_addr);

				if ((ip < whitelist_start) || (ip > whitelist_end))
				{
					printf("Rejected connection from %s (not in whitelist)\n", conn_ip);
					closesocket(cs);
					continue;
				}
			}

			// Check for free slot
			for (i = 0; i < MAX_CLIENTS; i++)
			{
				if(!clients[i].connected)
					break;
			}

			if(i >= MAX_CLIENTS)
			{
				printf("Too many connections! (rejected client: %s)\n", inet_ntoa(addr.sin_addr));
				closesocket(cs);
				continue;
			}

			// Show only new connections
			if(strcmp(last_ip, conn_ip))
			{
				printf("[%i] Connection from %s\n", i, conn_ip);
				sprintf(last_ip, "%s", conn_ip);
			}
		}

		/////////////////////////
		// create client thread
		/////////////////////////
		if(initialize_client(&clients[i]) != SUCCEEDED)
		{
			printf("System seems low in resources.\n");
			continue;
		}

		clients[i].s = cs;
		clients[i].ip_addr = addr.sin_addr;
		create_start_thread(&clients[i].thread, client_thread, &clients[i]);
	}

#ifdef WIN32
	WSACleanup();
#endif

	return SUCCEEDED;
#endif //#ifndef MAKEISO
	
exit_error:
	printf("\n\nPress ENTER to continue...");
	getchar();

	return FAILED;
}
