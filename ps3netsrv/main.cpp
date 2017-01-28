#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"
#include "compat.h"
#include "netiso.h"

#include "File.h"
#include "VIsoFile.h"


#define BUFFER_SIZE  (3 * 1048576)
#define MAX_CLIENTS  5

#define MAX_ENTRIES  4096
#define MAX_PATH_LEN 510

#define MIN(a, b)         ((a) <= (b) ? (a) : (b))
#define IS_RANGE(a, b, c) (((a) >= (b)) && ((a) <= (c)))
#define IS_PARENT_DIR(a)  ((a[0] == '.') && ((a[1] == 0) || ((a[1] == '.') && (a[2] == 0))))

//#define MERGE_DRIVES 1

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
} client_t;

static client_t clients[MAX_CLIENTS];

static char root_directory[MAX_PATH_LEN];
static size_t root_len = 0;

#ifdef WIN32
#ifdef MERGE_DRIVES
static char *ignore_drives;
static int ignore_drives_len = 0;
#endif
#else
#undef MERGE_DRIVES
#endif

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
		DPRINTF("Error in setsockopt(REUSEADDR): %d\n", get_network_error());
		closesocket(s);
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		DPRINTF("Error in bind: %d\n", get_network_error());
		return -1;
	}

	if(listen(s, 1) < 0)
	{
		DPRINTF("Error in listen: %d\n", get_network_error());
		return -1;
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


static int initialize_client(client_t *client)
{
	memset(client, 0, sizeof(client_t));

	client->buf = (uint8_t *)malloc(BUFFER_SIZE);
	if(!client->buf)
	{
		DPRINTF("Memory allocation error!\n");
		return -1;
	}

	client->ro_file = NULL;
	client->wo_file = NULL;
	client->dir = NULL;
	client->dirpath = NULL;
	client->connected = 1;
	client->CD_SECTOR_SIZE = 2352;
	return 0;
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

	memset(client, 0, sizeof(client_t));
}

static char *translate_path(char *path, int del, int *viso)
{
	char *p;

	if(!path) return NULL;

	if(path[0] != '/')
	{
		DPRINTF("path must start by '/'. Path received: %s\n", path);
		if(del)
		{
			if(path) free(path);
		}
		return NULL;
	}

	// check unsecure path
	p = path;
	while ((p = strstr(p, "..")))
	{
		if(strlen(p) >= 2)
		{
			if(*(p-1) == '/' || *(p-1) == '\\')
			{
				if(p[2] == 0 || p[2] == '/' || p[2] == '\\')
				{
					DPRINTF("The path \"%s\" is unsecure!\n", path);
					if(del)
					{
						if(path) free(path);
					}
					return NULL;
				}
			}
		}

		p += 2;
	}

	size_t path_len = strlen(path);

	p = (char *)malloc(root_len + path_len + 1);
	if(!p)
	{
		printf("Memory allocation error\n");
		exit(-1);
	}

	sprintf(p, "%s%s", root_directory, path);

	if(viso)
	{
		char *q = p + root_len;

		if(strstr(q, "/***PS3***/") == q)
		{
			memmove(q, q + 10, strlen(q + 10) + 1);
			DPRINTF("p -> %s\n", p);
			*viso = VISO_PS3;
		}
		else if(strstr(q, "/***DVD***/") == q)
		{
			memmove(q, q + 10, strlen(q + 10) + 1);
			DPRINTF("p -> %s\n", p);
			*viso = VISO_ISO;
		}
		else
		{
			*viso = VISO_NONE;
		}
	}

#ifdef WIN32
	file_stat_t st;
	path_len = strlen(p);
	for(int i = 0; i < path_len; i++) if(p[i] == '\\') p[i] = '/';

	#ifdef MERGE_DRIVES
	if(stat_file(p, &st) < 0)
	{
		for(int drive = 'C'; drive <= 'Z'; drive++)
		{
			if(ignore_drives)
			{
				bool ignore; ignore = false;

				for(uint8_t d = 0; d < ignore_drives_len; d++)
					if((ignore_drives[d] & 0xFF) == drive) {ignore = true; break;}

				if(ignore) continue;
			}

			sprintf(p, "%c:%s", drive, path);
			if(stat_file(p, &st) >= 0) break;
		}
	}
	#endif

#endif

	if(del)
	{
		if(path) free(path);
	}

	return p;
}

static int64_t calculate_directory_size(char *path)
{
	int64_t result = 0;
	DIR *d;
	struct dirent *entry;

	//DPRINTF("Calculate %s\n", path);

	//file_stat_t st;
	//if(stat_file(path, &st) < 0) return -1;

	d = opendir(path);
	if(!d)
		return -1;

	size_t d_name_len, path_len;
	path_len = strlen(path);

	while ((entry = readdir(d)))
	{
		if(IS_PARENT_DIR(entry->d_name)) continue;

		#ifdef WIN32
		d_name_len = entry->d_namlen;
		#else
		d_name_len = strlen(entry->d_name);
		#endif

		if(IS_RANGE(d_name_len, 1, 65535))
		{
			//DPRINTF("name: %s\n", entry->d_name);

			file_stat_t st;
			char newpath[path_len + d_name_len + 2];

			sprintf(newpath, "%s/%s", path, entry->d_name);

			if(stat_file(newpath, &st) < 0)
			{
				DPRINTF("calculate_directory_size: stat failed on %s\n", newpath);
				result = -1;
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

	closedir(d);
	return result;
}

// NOTE: All process_XXX function return an error ONLY if connection must be aborted. If only a not critical error must be returned to the client, that error will be
// sent using network, but the function must return 0

static int process_open_cmd(client_t *client, netiso_open_cmd *cmd)
{
	file_stat_t st;
	netiso_open_result result;
	char *filepath;
	uint16_t fp_len;
	int ret, viso = VISO_NONE;

	result.file_size = BE64(-1);
	result.mtime = BE64(0);

	fp_len = BE16(cmd->fp_len);
	if(fp_len == 0)
	{
		DPRINTF("ERROR: invalid path length for open command\n");
		return -1;
	}

	//DPRINTF("fp_len = %d\n", fp_len);
	filepath = (char *)malloc(fp_len + 1);
	if(!filepath)
	{
		DPRINTF("CRITICAL: memory allocation error\n");
		return -1;
	}

	ret = recv_all(client->s, (void *)filepath, fp_len);
	filepath[fp_len] = 0;

	if(ret != fp_len)
	{
		DPRINTF("recv failed, getting filename for open: %d %d\n", ret, get_network_error());
		free(filepath);
		return -1;
	}

	if((fp_len >= 10) && !strcmp(filepath, "/CLOSEFILE"))
	{
		free(filepath);
		return 0;
	}

	filepath = translate_path(filepath, 1, &viso);
	if(!filepath)
	{
		DPRINTF("Path cannot be translated. Connection with this client will be aborted.\n");
		return -1;
	}

	if(client->ro_file)
	{
		delete client->ro_file;
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

	client->CD_SECTOR_SIZE = 2352;

	if(client->ro_file->open(filepath, O_RDONLY) < 0)
	{
		printf("open error on \"%s\" (viso=%d)\n", filepath + root_len, viso);
		delete client->ro_file;
		client->ro_file = NULL;
	}
	else
	{
		if(client->ro_file->fstat(&st) < 0)
		{
			DPRINTF("Error in fstat\n");
		}
		else
		{
			result.file_size = BE64(st.file_size);
			result.mtime = BE64(st.mtime);

			if(viso != VISO_NONE || BE64(st.file_size) > 0x400000UL) printf("open %s\n", filepath + root_len);

			// detect cd sector size (2MB - 848MB)
			if(IS_RANGE(st.file_size, 0x200000UL, 0x35000000UL))
			{
				char buffer[0x10] = ""; client->CD_SECTOR_SIZE = 0;

				client->ro_file->seek(0x8020UL, SEEK_SET); client->ro_file->read(buffer, 0xC); if(memcmp(buffer, "PLAYSTATION ", 0xC) == 0) client->CD_SECTOR_SIZE = 2048; else {
				client->ro_file->seek(0x9220UL, SEEK_SET); client->ro_file->read(buffer, 0xC); if(memcmp(buffer, "PLAYSTATION ", 0xC) == 0) client->CD_SECTOR_SIZE = 2336; else {
				client->ro_file->seek(0x9320UL, SEEK_SET); client->ro_file->read(buffer, 0xC); if(memcmp(buffer, "PLAYSTATION ", 0xC) == 0) client->CD_SECTOR_SIZE = 2352; else {
				client->ro_file->seek(0x9920UL, SEEK_SET); client->ro_file->read(buffer, 0xC); if(memcmp(buffer, "PLAYSTATION ", 0xC) == 0) client->CD_SECTOR_SIZE = 2448; }}}

				if(client->CD_SECTOR_SIZE > 0) printf("CD sector size: %i\n", client->CD_SECTOR_SIZE); else client->CD_SECTOR_SIZE = 2352;
			}
		}
	}

#ifdef WIN32
	DPRINTF("File size: %I64x\n", st.file_size);
#else
	DPRINTF("File size: %llx\n", (long long unsigned int)st.file_size);
#endif

	free(filepath);

	ret = send(client->s, (char *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		DPRINTF("open, send result error: %d %d\n", ret, get_network_error());
		return -1;
	}

	return 0;
}

static int process_read_file_critical(client_t *client, netiso_read_file_critical_cmd *cmd)
{
	uint64_t offset;
	uint32_t remaining;

	offset = BE64(cmd->offset);
	remaining = BE32(cmd->num_bytes);

	if(!client->ro_file)
		return -1;

#ifdef WIN32
	DPRINTF("Read %I64x %x\n", (long long unsigned int)offset, remaining);
#else
	DPRINTF("Read %llx %x\n", (long long unsigned int)offset, remaining);
#endif

	if(client->ro_file->seek(offset, SEEK_SET) < 0)
	{
		DPRINTF("seek_file failed!\n");
		return -1;
	}

	uint32_t read_size = MIN(BUFFER_SIZE, remaining);

	while (remaining > 0)
	{

		if(remaining < read_size)
		{
			read_size = remaining;
		}

		if(client->ro_file->read(client->buf, read_size) != read_size)
		{
			DPRINTF("read_file failed on read file critical command!\n");
			return -1;
		}

		if(send(client->s, (char *)client->buf, read_size, 0) != read_size)
		{
			DPRINTF("send failed on read file critical command!\n");
			return -1;
		}

		remaining -= read_size;
	}

	return 0;
}

static int process_read_cd_2048_critical_cmd(client_t *client, netiso_read_cd_2048_critical_cmd *cmd)
{
	uint64_t offset;
	uint32_t sector_count;
	uint8_t *buf;

	offset = BE32(cmd->start_sector)*(client->CD_SECTOR_SIZE);
	sector_count = BE32(cmd->sector_count);

	DPRINTF("Read CD 2048 (%i) %x %x\n", client->CD_SECTOR_SIZE, BE32(cmd->start_sector), sector_count);

	if(!client->ro_file)
		return -1;

	if((sector_count * 2048) > BUFFER_SIZE)
	{
		// This is just to save some uneeded code. PS3 will never request such a high number of sectors
		DPRINTF("This situation wasn't expected, too many sectors read!\n");
		return -1;
	}

	buf = client->buf;
	for (uint32_t i = 0; i < sector_count; i++)
	{
		client->ro_file->seek(offset + 24, SEEK_SET);
		if(client->ro_file->read(buf, 2048) != 2048)
		{
			DPRINTF("read_file failed on read cd 2048 critical command!\n");
			return -1;
		}

		buf += 2048;
		offset += client->CD_SECTOR_SIZE;
	}

	if(send(client->s, (char *)client->buf, sector_count * 2048, 0) != (sector_count * 2048))
	{
		DPRINTF("send failed on read cd 2048 critical command!\n");
		return -1;
	}

	return 0;
}

static int process_read_file_cmd(client_t *client, netiso_read_file_cmd *cmd)
{
	uint64_t offset;
	uint32_t remaining;
	int32_t bytes_read;
	netiso_read_file_result result;

	offset = BE64(cmd->offset);
	remaining = BE32(cmd->num_bytes);

	if(!client->ro_file)
	{
		bytes_read = -1;
		goto send_result_read_file;
	}

	if(remaining > BUFFER_SIZE)
	{
		bytes_read = -1;
		goto send_result_read_file;
	}

	if(client->ro_file->seek(offset, SEEK_SET) < 0)
	{
		bytes_read = -1;
		goto send_result_read_file;
	}

	bytes_read = client->ro_file->read(client->buf, remaining);
	if(bytes_read < 0)
	{
		bytes_read = -1;
	}

send_result_read_file:

	result.bytes_read = (int32_t)BE32(bytes_read);

	if(send(client->s, (char *)&result, sizeof(result), 0) != 4)
	{
		DPRINTF("send failed on send result (read file)\n");
		return -1;
	}

	if(bytes_read > 0 && send(client->s, (char *)client->buf, bytes_read, 0) != bytes_read)
	{
		DPRINTF("send failed on read file!\n");
		return -1;
	}

	return 0;
}

static int process_create_cmd(client_t *client, netiso_create_cmd *cmd)
{
	netiso_create_result result;
	char *filepath;
	uint16_t fp_len;
	int ret;

	fp_len = BE16(cmd->fp_len);
	//DPRINTF("fp_len = %d\n", fp_len);
	filepath = (char *)malloc(fp_len+1);
	if(!filepath)
	{
		DPRINTF("CRITICAL: memory allocation error\n");
		return -1;
	}

	filepath[fp_len] = 0;
	ret = recv_all(client->s, (void *)filepath, fp_len);
	if(ret != fp_len)
	{
		DPRINTF("recv failed, getting filename for create: %d %d\n", ret, get_network_error());
		free(filepath);
		return -1;
	}

	filepath = translate_path(filepath, 1, NULL);
	if(!filepath)
	{
		DPRINTF("Path cannot be translated. Connection with this client will be aborted.\n");
		return -1;
	}

	DPRINTF("create %s\n", filepath);

	if(client->wo_file)
	{
		delete client->wo_file;
	}

	client->wo_file = new File();

	if(client->wo_file->open(filepath, O_WRONLY|O_CREAT|O_TRUNC) < 0)
	{
		DPRINTF("create error on \"%s\"\n", filepath);
		result.create_result = BE32(-1);
		delete client->wo_file;
		client->wo_file = NULL;
	}
	else
	{
		result.create_result = BE32(0);
	}

	free(filepath);

	ret = send(client->s, (char *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		DPRINTF("create, send result error: %d %d\n", ret, get_network_error());
		return -1;
	}

	return 0;
}

static int process_write_file_cmd(client_t *client, netiso_write_file_cmd *cmd)
{
	uint32_t remaining;
	int32_t bytes_written;
	netiso_write_file_result result;

	remaining = BE32(cmd->num_bytes);

	if(!client->wo_file)
	{
		bytes_written = -1;
		goto send_result_write_file;
	}

	if(remaining > BUFFER_SIZE)
	{
		return -1;
	}

	//DPRINTF("Remaining: %d\n", remaining);

	if(remaining > 0)
	{
		int ret = recv_all(client->s, (void *)client->buf, remaining);
		if(ret != remaining)
		{
			DPRINTF("recv failed on write file: %d %d\n", ret, get_network_error());
			return -1;
		}
	}

	bytes_written = client->wo_file->write(client->buf, remaining);
	if(bytes_written < 0)
	{
		bytes_written = -1;
	}

send_result_write_file:

	result.bytes_written = (int32_t)BE32(bytes_written);

	if(send(client->s, (char *)&result, sizeof(result), 0) != 4)
	{
		DPRINTF("send failed on send result (read file)\n");
		return -1;
	}

	return 0;
}

static int process_delete_file_cmd(client_t *client, netiso_delete_file_cmd *cmd)
{
	netiso_delete_file_result result;
	char *filepath;
	uint16_t fp_len;
	int ret;

	fp_len = BE16(cmd->fp_len);
	filepath = (char *)malloc(fp_len+1);
	if(!filepath)
	{
		DPRINTF("CRITICAL: memory allocation error\n");
		return -1;
	}

	filepath[fp_len] = 0;
	ret = recv_all(client->s, (void *)filepath, fp_len);
	if(ret != fp_len)
	{
		DPRINTF("recv failed, getting filename for delete file: %d %d\n", ret, get_network_error());
		free(filepath);
		return -1;
	}

	filepath = translate_path(filepath, 1, NULL);
	if(!filepath)
	{
		DPRINTF("Path cannot be translated. Connection with this client will be aborted.\n");
		return -1;
	}

	printf("delete %s\n", filepath + root_len);

	result.delete_result = BE32(unlink(filepath));
	free(filepath);

	ret = send(client->s, (char *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		DPRINTF("delete, send result error: %d %d\n", ret, get_network_error());
		return -1;
	}

	return 0;
}

static int process_mkdir_cmd(client_t *client, netiso_mkdir_cmd *cmd)
{
	netiso_mkdir_result result;
	char *dirpath;
	uint16_t dp_len;
	int ret;

	dp_len = BE16(cmd->dp_len);
	dirpath = (char *)malloc(dp_len+1);
	if(!dirpath)
	{
		DPRINTF("CRITICAL: memory allocation error\n");
		return -1;
	}

	dirpath[dp_len] = 0;
	ret = recv_all(client->s, (void *)dirpath, dp_len);
	if(ret != dp_len)
	{
		DPRINTF("recv failed, getting dirname for mkdir: %d %d\n", ret, get_network_error());
		free(dirpath);
		return -1;
	}

	dirpath = translate_path(dirpath, 1, NULL);
	if(!dirpath)
	{
		DPRINTF("Path cannot be translated. Connection with this client will be aborted.\n");
		return -1;
	}

	printf("mkdir %s\n", dirpath + root_len);

#ifdef WIN32
	result.mkdir_result = BE32(mkdir(dirpath));
#else
	result.mkdir_result = BE32(mkdir(dirpath, 0777));
#endif
	free(dirpath);

	ret = send(client->s, (char *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		DPRINTF("open dir, send result error: %d %d\n", ret, get_network_error());
		return -1;
	}

	return 0;
}

static int process_rmdir_cmd(client_t *client, netiso_rmdir_cmd *cmd)
{
	netiso_rmdir_result result;
	char *dirpath;
	uint16_t dp_len;
	int ret;

	dp_len = BE16(cmd->dp_len);
	dirpath = (char *)malloc(dp_len+1);
	if(!dirpath)
	{
		DPRINTF("CRITICAL: memory allocation error\n");
		return -1;
	}

	dirpath[dp_len] = 0;
	ret = recv_all(client->s, (void *)dirpath, dp_len);
	if(ret != dp_len)
	{
		DPRINTF("recv failed, getting dirname for rmdir: %d %d\n", ret, get_network_error());
		free(dirpath);
		return -1;
	}

	dirpath = translate_path(dirpath, 1, NULL);
	if(!dirpath)
	{
		DPRINTF("Path cannot be translated. Connection with this client will be aborted.\n");
		return -1;
	}

	printf("rmdir %s\n", dirpath + root_len);

	result.rmdir_result = BE32(rmdir(dirpath));
	free(dirpath);

	ret = send(client->s, (char *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		DPRINTF("open dir, send result error: %d %d\n", ret, get_network_error());
		return -1;
	}

	return 0;
}

static int process_open_dir_cmd(client_t *client, netiso_open_dir_cmd *cmd)
{
	netiso_open_dir_result result;
	char *dirpath;
	uint16_t dp_len;
	int ret;

	dp_len = BE16(cmd->dp_len);

	dirpath = (char *)malloc(dp_len+1);
	if(!dirpath)
	{
		DPRINTF("CRITICAL: memory allocation error\n");
		return -1;
	}

	dirpath[dp_len] = 0;
	ret = recv_all(client->s, (void *)dirpath, dp_len);
	if(ret != dp_len)
	{
		DPRINTF("recv failed, getting dirname for open dir: %d %d\n", ret, get_network_error());
		free(dirpath);
		return -1;
	}

	dirpath = translate_path(dirpath, 1, NULL);
	if(!dirpath)
	{
		DPRINTF("Path cannot be translated. Connection with this client will be aborted.\n");
		return -1;
	}

	DPRINTF("open dir %s\n", dirpath);

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

	client->dir = opendir(dirpath);
	if(!client->dir)
	{
		DPRINTF("open dir error on \"%s\"\n", dirpath);
		result.open_result = BE32(-1);
	}
	else
	{
		client->dirpath = dirpath;
		result.open_result = BE32(0);
	}

	if(!client->dirpath)
	{
		free(dirpath);
	}

	ret = send(client->s, (char *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		DPRINTF("open dir, send result error: %d %d\n", ret, get_network_error());
		return -1;
	}

	return 0;
}

static int process_read_dir_entry_cmd(client_t *client, netiso_read_dir_entry_cmd *cmd, int version)
{
	char *path;
	file_stat_t st;
	struct dirent *entry;
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

	if(!client->dir || !client->dirpath)
	{
		if(version == 1)
		{
			result_v1.file_size = BE64(-1);
		}
		else
		{
			result_v2.file_size = BE64(-1);
		}

		goto send_result_read_dir;
	}

	while ((entry = readdir(client->dir)))
	{
		if(IS_PARENT_DIR(entry->d_name)) continue;

		#ifdef WIN32
		d_name_len = entry->d_namlen;
		#else
		d_name_len = strlen(entry->d_name);
		#endif

		if(IS_RANGE(d_name_len, 1, 65535)) break;
	}

	if(!entry)
	{
		closedir(client->dir);
		if(client->dirpath) free(client->dirpath);
		client->dir = NULL;
		client->dirpath = NULL;

		if(version == 1)
		{
			result_v1.file_size = BE64(-1);
		}
		else
		{
			result_v2.file_size = BE64(-1);
		}
		goto send_result_read_dir;
	}

	path = (char *)malloc(strlen(client->dirpath) + d_name_len + 2);
	if(!path)
	{
		DPRINTF("CRITICAL: memory allocation error\n");
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
			result_v1.file_size = BE64(-1);
		}
		else
		{
			result_v2.file_size = BE64(-1);
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
			DPRINTF("send error on read dir entry (%d)\n", get_network_error());
			return -1;
		}
	}
	else
	{
		if(send(client->s, (char *)&result_v2, sizeof(result_v2), 0) != sizeof(result_v2))
		{
			DPRINTF("send error on read dir entry (%d)\n", get_network_error());
			return -1;
		}
	}

	if((version == 1 && result_v1.file_size != BE64(-1)) || (version == 2 && result_v2.file_size != BE64(-1)))
	{
		if(send(client->s, (char *)entry->d_name, d_name_len, 0) != d_name_len)
		{
			DPRINTF("send file name error on read dir entry (%d)\n", get_network_error());
			return -1;
		}
	}

	return 0;
}

static int process_read_dir_cmd(client_t *client, netiso_read_dir_entry_cmd *cmd)
{
	(void) cmd;
	int64_t dir_size = 0;

	netiso_read_dir_result result;
	memset(&result, 0, sizeof(result));

	netiso_read_dir_result_data *dir_entries = (netiso_read_dir_result_data *) malloc(sizeof(netiso_read_dir_result_data) * MAX_ENTRIES);
	memset(dir_entries, 0, sizeof(netiso_read_dir_result_data) * MAX_ENTRIES);

	if(!client->dir || !client->dirpath || !dir_entries)
	{
		result.dir_size = (0);
		goto send_result_read_dir_cmd;
	}

	size_t d_name_len, dirpath_len;
	dirpath_len = strlen(client->dirpath);

	file_stat_t st;
	struct dirent *entry;

	while ((entry = readdir(client->dir)))
	{
		if(!entry) break;
		if(IS_PARENT_DIR(entry->d_name)) continue;

		#ifdef WIN32
		d_name_len = entry->d_namlen;
		#else
		d_name_len = strlen(entry->d_name);
		#endif

		if(IS_RANGE(d_name_len, 1, MAX_PATH_LEN))
		{
			char *path = (char*)malloc(dirpath_len + d_name_len + 2);

			if(!path) break;

			sprintf(path, "%s/%s", client->dirpath, entry->d_name);

			st.file_size = 0;
			st.mode = S_IFDIR;
			st.mtime = 0;
			st.atime = 0;
			st.ctime = 0;

			stat_file(path, &st);

			if(!st.mtime) st.mtime = st.ctime;
			if(!st.mtime) st.mtime = st.atime;

			if((st.mode & S_IFDIR) == S_IFDIR)
			{
				dir_entries[dir_size].file_size = (0);
				dir_entries[dir_size].is_directory = 1;
			}
			else
			{
				dir_entries[dir_size].file_size =  BE64(st.file_size);
				dir_entries[dir_size].is_directory = 0;
			}

			snprintf(dir_entries[dir_size].name, MAX_PATH_LEN, "%s", entry->d_name);
			dir_entries[dir_size].mtime = BE64(st.mtime);

			free(path);
			dir_size++;
			if(dir_size >= MAX_ENTRIES) break;
		}
	}

#ifdef WIN32
	#ifdef MERGE_DRIVES
	if(root_len > 2 && dirpath_len > (root_len + 1) && strncmp(client->dirpath, root_directory, root_len) == 0)
	{
		memmove(client->dirpath + 2, client->dirpath + root_len, strlen(client->dirpath + root_len) + 1);

		client->dirpath[1] = ':';

		dirpath_len = strlen(client->dirpath);

		if(client->dir) {closedir(client->dir); client->dir = NULL;}

		for(int drive = 'C'; drive <= 'Z'; drive++)
		{
			if(dir_size >= MAX_ENTRIES) break;

			if(ignore_drives)
			{
				bool ignore = false;

				for(uint8_t d = 0; d < ignore_drives_len; d++)
					if((ignore_drives[d]  & 0xFF) == drive) {ignore = true; break;}

				if(ignore) continue;
			}

			client->dirpath[0] = drive;

			if(stat_file(client->dirpath, &st) < 0) continue;
			client->dir = opendir(client->dirpath);

			while ((entry = readdir(client->dir)))
			{
				if(!entry) break;
				if(IS_PARENT_DIR(entry->d_name)) continue;

				#ifdef WIN32
				d_name_len = entry->d_namlen;
				#else
				d_name_len = strlen(entry->d_name);
				#endif

				if(IS_RANGE(d_name_len, 1, MAX_PATH_LEN))
				{
					char *path = (char*)malloc(dirpath_len + d_name_len + 2);

					if(path) break;

					sprintf(path, "%s/%s", client->dirpath, entry->d_name);
					st.file_size = 0;
					st.mode = S_IFDIR;
					st.mtime = 0;
					st.atime = 0;
					st.ctime = 0;
					stat_file(path, &st);

					if(!st.mtime) st.mtime = st.ctime;
					if(!st.mtime) st.mtime = st.atime;

					if((st.mode & S_IFDIR) == S_IFDIR)
					{
						dir_entries[dir_size].file_size = (0);
						dir_entries[dir_size].is_directory = 1;
					}
					else
					{
						dir_entries[dir_size].file_size =  BE64(st.file_size);
						dir_entries[dir_size].is_directory = 0;
					}

					snprintf(dir_entries[dir_size].name, MAX_PATH_LEN, "%s", entry->d_name);
					dir_entries[dir_size].mtime = BE64(st.mtime);

					free(path);
					dir_size++;
					if(dir_size >= MAX_ENTRIES) break;
				}
			}
		}
	}
	#endif
#endif

	if(client->dir) {closedir(client->dir); client->dir = NULL;}

send_result_read_dir_cmd:

	result.dir_size = BE64(dir_size);
	if(send(client->s, (const char*)&result, sizeof(result), 0) != sizeof(result))
	{
		if(dir_entries) free(dir_entries);
		return -1;
	}

	if(dir_size > 0)
	{
		if(send(client->s, (const char*)dir_entries, (sizeof(netiso_read_dir_result_data)*dir_size), 0) != (int)(sizeof(netiso_read_dir_result_data)*dir_size))
		{
			if(dir_entries) free(dir_entries);
			return -1;
		}
	}

	if(dir_entries) free(dir_entries);
	return 0;
}

static int process_stat_cmd(client_t *client, netiso_stat_cmd *cmd)
{
	netiso_stat_result result;
	file_stat_t st;
	char *filepath;
	uint16_t fp_len;
	int ret;

	fp_len = BE16(cmd->fp_len);
	//DPRINTF("fp_len = %d\n", fp_len);
	filepath = (char *)malloc(fp_len+1);
	if(!filepath)
	{
		DPRINTF("CRITICAL: memory allocation error\n");
		return -1;
	}

	filepath[fp_len] = 0;
	ret = recv_all(client->s, (char *)filepath, fp_len);
	if(ret != fp_len)
	{
		DPRINTF("recv failed, getting filename for stat: %d %d\n", ret, get_network_error());
		free(filepath);
		return -1;
	}

	filepath = translate_path(filepath, 1, NULL);
	if(!filepath)
	{
		DPRINTF("Path cannot be translated. Connection with this client will be aborted.\n");
		return -1;
	}

	DPRINTF("stat %s\n", filepath);
	if((stat_file(filepath, &st) < 0) && (!strstr(filepath, "/is_ps3_compat1/")))
	{
		DPRINTF("stat error on \"%s\"\n", filepath);
		result.file_size = BE64(-1);
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
		DPRINTF("stat, send result error: %d %d\n", ret, get_network_error());
		return -1;
	}

	return 0;
}

static int process_get_dir_size_cmd(client_t *client, netiso_get_dir_size_cmd *cmd)
{
	netiso_get_dir_size_result result;
	char *dirpath;
	uint16_t dp_len;
	int ret;

	dp_len = BE16(cmd->dp_len);
	dirpath = (char *)malloc(dp_len+1);
	if(!dirpath)
	{
		DPRINTF("CRITICAL: memory allocation error\n");
		return -1;
	}

	dirpath[dp_len] = 0;
	ret = recv_all(client->s, (char *)dirpath, dp_len);
	if(ret != dp_len)
	{
		DPRINTF("recv failed, getting dirname for get_dir_size: %d %d\n", ret, get_network_error());
		free(dirpath);
		return -1;
	}

	dirpath = translate_path(dirpath, 1, NULL);
	if(!dirpath)
	{
		DPRINTF("Path cannot be translated. Connection with this client will be aborted.\n");
		return -1;
	}

	DPRINTF("get_dir_size %s\n", dirpath);

	result.dir_size = BE64(calculate_directory_size(dirpath));
	free(dirpath);

	ret = send(client->s, (char *)&result, sizeof(result), 0);
	if(ret != sizeof(result))
	{
		DPRINTF("get_dir_size, send result error: %d %d\n", ret, get_network_error());
		return -1;
	}

	return 0;
}

void *client_thread(void *arg)
{
	client_t *client = (client_t *)arg;

	for(;;)
	{
		netiso_cmd cmd;
		int ret;

		ret = recv_all(client->s, (void *)&cmd, sizeof(cmd));
		if(ret != sizeof(cmd))
		{
			break;
		}

		switch (BE16(cmd.opcode))
		{
			case NETISO_CMD_READ_FILE_CRITICAL:
				ret = process_read_file_critical(client, (netiso_read_file_critical_cmd *)&cmd);
			break;

			case NETISO_CMD_READ_CD_2048_CRITICAL:
				ret = process_read_cd_2048_critical_cmd(client, (netiso_read_cd_2048_critical_cmd *)&cmd);
			break;

			case NETISO_CMD_READ_FILE:
				ret = process_read_file_cmd(client, (netiso_read_file_cmd *)&cmd);
			break;

			case NETISO_CMD_WRITE_FILE:
				ret = process_write_file_cmd(client, (netiso_write_file_cmd *)&cmd);
			break;

			case NETISO_CMD_READ_DIR_ENTRY:
				ret = process_read_dir_entry_cmd(client, (netiso_read_dir_entry_cmd *)&cmd, 1);
			break;

			case NETISO_CMD_READ_DIR_ENTRY_V2:
				ret = process_read_dir_entry_cmd(client, (netiso_read_dir_entry_cmd *)&cmd, 2);
			break;

			case NETISO_CMD_STAT_FILE:
				ret = process_stat_cmd(client, (netiso_stat_cmd *)&cmd);
			break;

			case NETISO_CMD_OPEN_FILE:
				ret = process_open_cmd(client, (netiso_open_cmd *)&cmd);
			break;

			case NETISO_CMD_CREATE_FILE:
				ret = process_create_cmd(client, (netiso_create_cmd *)&cmd);
			break;

			case NETISO_CMD_DELETE_FILE:
				ret = process_delete_file_cmd(client, (netiso_delete_file_cmd *)&cmd);
			break;

			case NETISO_CMD_OPEN_DIR:
				ret = process_open_dir_cmd(client, (netiso_open_dir_cmd *)&cmd);
			break;

			case NETISO_CMD_READ_DIR:
				ret = process_read_dir_cmd(client, (netiso_read_dir_entry_cmd *)&cmd);
			break;

			case NETISO_CMD_GET_DIR_SIZE:
				ret = process_get_dir_size_cmd(client, (netiso_get_dir_size_cmd *)&cmd);
			break;

			case NETISO_CMD_MKDIR:
				ret = process_mkdir_cmd(client, (netiso_mkdir_cmd *)&cmd);
			break;

			case NETISO_CMD_RMDIR:
				ret = process_rmdir_cmd(client, (netiso_rmdir_cmd *)&cmd);
			break;

			default:
				DPRINTF("Unknown command received: %04X\n", BE16(cmd.opcode));
				ret = -1;
		}

		if(ret != 0)
		{
			break;
		}
	}

	finalize_client(client);
	return NULL;
}

int main(int argc, char *argv[])
{
	int s;
	uint32_t whitelist_start = 0;
	uint32_t whitelist_end = 0;
	uint16_t port = NETISO_PORT;

#ifdef WIN32
	CONSOLE_SCREEN_BUFFER_INFO console_info;
	GetConsoleScreenBufferInfo( GetStdHandle( STD_OUTPUT_HANDLE ), &console_info );

	SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), 0x0F );
#endif

	printf("ps3netsrv build 20170106");

#ifdef WIN32
	SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), 0x04 );
#endif

	printf(" (mod by aldostools)\n");

#ifdef WIN32
	SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), console_info.wAttributes );
#endif

#ifndef WIN32
	if(sizeof(off_t) < 8)
	{
		DPRINTF("off_t too small!\n");
//		return -1;
	}
#endif

	file_stat_t fs;

	if(argc < 2)
	{
		char *filename = strrchr(argv[0], '/');
		if(filename) filename++; else {filename = strrchr(argv[0], '\\'); if(filename) filename++;}

		if((stat_file("./PS3ISO", &fs) >= 0) || (stat_file("./PSXISO", &fs) >= 0) || (stat_file("./GAMES", &fs) >= 0) || (stat_file("./GAMEZ", &fs) >= 0)  || (stat_file("./DVDISO", &fs) >= 0) || (stat_file("./BDISO", &fs) >= 0) || (stat_file("./PS3_NET_Server.cfg", &fs) >= 0))
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
		#ifdef MERGE_DRIVES
		printf( "\nUsage: %s [rootdirectory] [port] [whitelist] [ignore drive letters]\n\n"
				" Default port: %d\n\n"
				" Whitelist: x.x.x.x, where x is 0-255 or *\n"
				" (e.g 192.168.1.* to allow only connections from 192.168.1.0-192.168.1.255)\n", filename, NETISO_PORT);
		#else
		printf( "\nUsage: %s [rootdirectory] [port] [whitelist]\n\n"
				" Default port: %d\n\n"
				" Whitelist: x.x.x.x, where x is 0-255 or *\n"
				" (e.g 192.168.1.* to allow only connections from 192.168.1.0-192.168.1.255)\n", filename, NETISO_PORT);
		#endif
		#ifdef WIN32
		printf("\n\nPress ENTER to continue...");
		getchar();
		#endif
		return -1;
		}
	}

	if(strlen(argv[1]) >= sizeof(root_directory))
	{
		printf("Directory name too long!\n");
		return -1;
	}

	strcpy(root_directory, argv[1]);

	for (int i = strlen(root_directory) - 1; i >= 0; i--)
	{
		if(root_directory[i] == '/' || root_directory[i] == '\\')
			root_directory[i] = 0;
		else
			break;
	}

	printf("Path: %s\n\n", root_directory);

	root_len = strlen(root_directory);

	if(root_len == 0)
	{
		printf("/ can't be specified as root directory!\n");
		return -1;
	}

	if(argc > 2)
	{
		uint32_t u;

		if(sscanf(argv[2], "%u", &u) != 1)
		{
			printf("Wrong port specified.\n");
			return -1;
		}

#ifdef WIN32
		uint32_t min = 1;
#else
		uint32_t min = 1024;
#endif

		if(u < min || u > 65535)
		{
			printf("Port must be in %d-65535 range.\n", min);
			return -1;
		}

		port = u;
	}

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
					if(strcmp(p, "*") != 0)
					{
						printf("Wrong whitelist format.\n");
						return -1;
					}

				}
				else
				{
					if(p[0] != '*' || p[1] != '.')
					{
						printf("Wrong whitelist format.\n");
						return -1;
					}
				}

				wildcard = 1;
			}
			else
			{
				if(u > 0xFF)
				{
					printf("Wrong whitelist format.\n");
					return -1;
				}
			}

			if(wildcard)
			{
				whitelist_end |= (0xFF<<(i*8));
			}
			else
			{
				whitelist_start |= (u<<(i*8));
				whitelist_end |= (u<<(i*8));
			}

			if(i != 0)
			{
				p = strchr(p, '.');
				if(!p)
				{
					printf("Wrong whitelist format.\n");
					return -1;
				}

				p++;
			}
		}

		DPRINTF("Whitelist: %08X-%08X\n", whitelist_start, whitelist_end);
	}

#ifdef MERGE_DRIVES
	if(argc > 4)
	{
		ignore_drives = argv[4];
	}
	else if(whitelist_end == 0x0A000008)
	{
		ignore_drives = (char*)"E"; // ignore E:\ by default only if whitelist is 10.0.0.8
	}

	// convert to upper case
	if(ignore_drives)
	{
		ignore_drives_len = strlen(ignore_drives);

		for(uint8_t d = 0; d < ignore_drives_len; d++)
			if((ignore_drives[d] >= 'a') && (ignore_drives[d] <= 'z')) ignore_drives[d] -= ('a'-'A');
	}
#endif

	s = initialize_socket(port);
	if(s < 0)
	{
		printf("Error in initialization.\n");
		return -1;
	}

	memset(clients, 0, sizeof(clients));
	printf("Waiting for client...\n");

	for (;;)
	{
		struct sockaddr_in addr;
		unsigned int size;
		int cs;
		int i;

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
			if(clients[i].connected && clients[i].ip_addr.s_addr == addr.sin_addr.s_addr)
				break;
		}

		if(i != MAX_CLIENTS)
		{
			// Shutdown socket and wait for thread to complete
			shutdown(clients[i].s, SHUT_RDWR);
			closesocket(clients[i].s);
			join_thread(clients[i].thread);
			printf("Reconnection from %s\n",  inet_ntoa(addr.sin_addr));
		}
		else
		{
			if(whitelist_start != 0)
			{
				uint32_t ip = BE32(addr.sin_addr.s_addr);

				if(ip < whitelist_start || ip > whitelist_end)
				{
					printf("Rejected connection from %s (not in whitelist)\n", inet_ntoa(addr.sin_addr));
					closesocket(cs);
					continue;
				}
			}

			for (i = 0; i < MAX_CLIENTS; i++)
			{
				if(!clients[i].connected)
					break;
			}

			if(i == MAX_CLIENTS)
			{
				printf("Too many connections! (rejected client: %s)\n", inet_ntoa(addr.sin_addr));
				closesocket(cs);
				continue;
			}

			printf("Connection from %s\n",  inet_ntoa(addr.sin_addr));
		}

		if(initialize_client(&clients[i]) != 0)
		{
			printf("System seems low in resources.\n");
			break;
		}

		clients[i].s = cs;
		clients[i].ip_addr = addr.sin_addr;
		create_start_thread(&clients[i].thread, client_thread, &clients[i]);
	}

#ifdef WIN32
	#ifdef MERGE_DRIVES
	if(ignore_drives)
	{
		free(ignore_drives);
	}
	#endif
	WSACleanup();
#endif

	return 0;
}
