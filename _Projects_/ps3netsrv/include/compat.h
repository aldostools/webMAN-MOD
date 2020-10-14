#ifndef __COMPAT_H__
#define __COMPAT_H__

#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _file_stat_t
{
	uint64_t file_size;
	uint64_t mtime;
	uint64_t ctime;
	uint64_t atime;
	uint32_t mode;
} file_stat_t;

#ifdef WIN32

#include <winsock2.h>
#include <windows.h>

// Threads
typedef HANDLE thread_t;

// Files
#define INVALID_FD	INVALID_HANDLE_VALUE
#define FD_OK(fd) 	(fd != INVALID_HANDLE_VALUE)

typedef HANDLE file_t;

// Sockets
typedef int socklen_t;

#ifndef SHUT_RD
#define SHUT_RD SD_RECEIVE
#endif

#ifndef SHUT_WR
#define SHUT_WR SD_SEND
#endif

#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif

#ifndef MSG_WAITALL
#define MSG_WAITALL 0x8
#endif

#define get_network_error()	WSAGetLastError()

#else

#include <sys/errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>

// Threads
typedef pthread_t thread_t;

// Files
#define INVALID_FD	-1
#define FD_OK(fd) 	(fd >= 0)

typedef int file_t;

// Sockets
#define closesocket close
#define get_network_error() (errno)

#endif

int create_start_thread(thread_t *thread, void *(*start_routine)(void*), void *arg);
int join_thread(thread_t thread);

file_t open_file(const char *path, int oflag);
int close_file(file_t fd);
ssize_t read_file(file_t fd, void *buf, size_t nbyte);
ssize_t write_file(file_t fd, void *buf, size_t nbyte);
int64_t seek_file(file_t fd, int64_t offset, int whence);
int fstat_file(file_t fd, file_stat_t *fs);
int stat_file(const char *path, file_stat_t *fs);

#ifdef __cplusplus
}
#endif

#endif /* __COMPAT_H__ */
