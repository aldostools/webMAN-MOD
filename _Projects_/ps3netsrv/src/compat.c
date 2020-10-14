#include "compat.h"

static const int FAILED		= -1;
static const int SUCCEEDED	=  0;

#ifdef WIN32

int create_start_thread(thread_t *thread, void *(*start_routine)(void*), void *arg)
{
	thread_t t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, arg, 0, NULL);
	if (!t)
		return GetLastError();

	*thread = t;
	return SUCCEEDED;
}

int join_thread(thread_t thread)
{
	DWORD ret = WaitForSingleObject(thread, INFINITE);

	return (ret == 0xFFFFFFFF) ? (int)ret : SUCCEEDED;
}

// Files

file_t open_file(const char *path, int oflag)
{
	if (!path)
		return INVALID_HANDLE_VALUE;

	file_t f;
	DWORD dwDesiredAccess;
	DWORD dwCreationDisposition;

	if ((oflag & (O_RDONLY | O_WRONLY | O_RDWR)) == O_RDONLY)
		dwDesiredAccess = GENERIC_READ;
	else if ((oflag & (O_RDONLY | O_WRONLY | O_RDWR)) == O_WRONLY)
		dwDesiredAccess = GENERIC_WRITE;
	else
		dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;

	if ((oflag & O_TRUNC) && ((oflag & O_ACCMODE) != O_RDONLY))
		dwCreationDisposition = (oflag & O_CREAT) ? CREATE_ALWAYS : TRUNCATE_EXISTING;
	else
		dwCreationDisposition = (oflag & O_CREAT) ? OPEN_ALWAYS : OPEN_EXISTING;

	if ((oflag & O_EXCL) && (oflag & O_CREAT))
		dwCreationDisposition = CREATE_NEW;

	f = CreateFileA(path, dwDesiredAccess, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f == INVALID_HANDLE_VALUE)
	{
		//printf("Error: %x\n", GetLastError());
		return f;
	}

	SetFilePointer(f, 0, NULL, (oflag & O_APPEND) ? FILE_END : FILE_BEGIN);

	return f;
}

int close_file(file_t fd)
{
	return (CloseHandle(fd)) ? SUCCEEDED : FAILED;
}

ssize_t read_file(file_t fd, void *buf, size_t nbyte)
{
	DWORD rd;

	return (ReadFile(fd, buf, nbyte, &rd, NULL)) ? rd : FAILED;
}

ssize_t write_file(file_t fd, void *buf, size_t nbyte)
{
	DWORD wr;

	return (WriteFile(fd, buf, nbyte, &wr, NULL)) ? wr : FAILED;
}

int64_t seek_file(file_t fd, int64_t offset, int whence)
{
	LONG low;
	LONG high;

	low = offset & 0xFFFFFFFF;
	high = (offset >> 32);

	if (whence == SEEK_SET)
	{
		whence = FILE_BEGIN;
	}
	else if (whence == SEEK_CUR)
	{
		whence = FILE_CURRENT;
	}
	else if (whence == SEEK_END)
	{
		whence = FILE_END;
	}
	else
	{
		return FAILED;
	}

	low = SetFilePointer(fd, low, &high, whence);
	if (low == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
	{
		//printf("SetFilePointer failed: %x\n", GetLastError());
		return FAILED;
	}

	uint64_t low64 = (uint64_t)low;
	uint64_t high64 = (uint64_t)high;

	low64 &= 0xFFFFFFFF;
	high64 &= 0xFFFFFFFF;
	int64_t ret = (int64_t)((high64<<32ULL) | low64);

	//printf("ret = %I64x\n", ret);

	return ret;
}

#define USE_LONG_LONG 1

uint64_t FileTimeToUnixTime(const FILETIME *filetime, DWORD *remainder)
{
	/* Read the comment in the function DOSFS_UnixTimeToFileTime. */
#if USE_LONG_LONG

	long long int t = filetime->dwHighDateTime;
	t <<= 32;
	t += (UINT32)filetime->dwLowDateTime;
	t -= 116444736000000000LL;
	if (t < 0)
	{
		if (remainder) *remainder = 9999999 - (-t - 1) % 10000000;
			return -1 - ((-t - 1) / 10000000);
	}
	else
	{
		if (remainder)
			*remainder = t % 10000000;

		return t / 10000000;
	}

#else  /* ISO version */

	UINT32 a0;			/* 16 bit, low	bits */
	UINT32 a1;			/* 16 bit, medium bits */
	UINT32 a2;			/* 32 bit, high   bits */
	UINT32 r;			/* remainder of division */
	unsigned int carry;		/* carry bit for subtraction */
	int negative;		/* whether a represents a negative value */

	/* Copy the time values to a2/a1/a0 */
	a2 =  static_cast<UINT32>(filetime->dwHighDateTime);
	a1 = (static_cast<UINT32>(filetime->dwLowDateTime)) >> 16;
	a0 = (static_cast<UINT32>(filetime->dwLowDateTime)) & 0xffff;

	/* Subtract the time difference */
	if (a0 >= 32768		   )	a0 -=			  32768		   , carry = 0;
	else						a0 += (1 << 16) - 32768		   , carry = 1;

	if (a1 >= 54590	+ carry)    a1 -=			  54590 + carry, carry = 0;
	else						a1 += (1 << 16) - 54590 - carry, carry = 1;

	a2 -= 27111902 + carry;

	/* If a is negative, replace a by (-1-a) */
	negative = (a2 >= (static_cast<UINT32>(1)) << 31);
	if (negative)
	{
		/* Set a to -a - 1 (a is a2/a1/a0) */
		a0 = 0xffff - a0;
		a1 = 0xffff - a1;
		a2 = ~a2;
	}

	/* Divide a by 10000000 (a = a2/a1/a0), put the rest into r.
	   Split the divisor into 10000 * 1000 which are both less than 0xffff. */
	a1 += (a2 % 10000) << 16;
	a2 /=		10000;
	a0 += (a1 % 10000) << 16;
	a1 /=		10000;
	 r  =  a0 % 10000;
	a0 /=		10000;

	a1 += (a2 % 1000) << 16;
	a2 /=		1000;
	a0 += (a1 % 1000) << 16;
	a1 /=		1000;
	 r += (a0 % 1000) * 10000;
	a0 /=		1000;

	/* If a was negative, replace a by (-1-a) and r by (9999999 - r) */
	if (negative)
	{
		/* Set a to -a - 1 (a is a2/a1/a0) */
		a0 = 0xffff - a0;
		a1 = 0xffff - a1;
		a2 = ~a2;

		r  = 9999999 - r;
	}

	if (remainder)
		*remainder = r;

	/* Do not replace this by << 32, it gives a compiler warning and it does
	not work. */
	return (((static_cast<time_t>(a2)) << 16) << 16) + (a1 << 16) + a0;
#endif
}

int fstat_file(file_t fd, file_stat_t *fs)
{
	BY_HANDLE_FILE_INFORMATION  FileInformation;

	if (!GetFileInformationByHandle(fd, &FileInformation))
		return FAILED;

	fs->file_size = ((uint64_t)FileInformation.nFileSizeHigh << 32) | FileInformation.nFileSizeLow;

	fs->ctime = FileTimeToUnixTime(&FileInformation.ftCreationTime, NULL);
	fs->atime = FileTimeToUnixTime(&FileInformation.ftLastAccessTime, NULL);
	fs->mtime = FileTimeToUnixTime(&FileInformation.ftLastWriteTime, NULL);

	fs->atime = (fs->atime == 0) ? fs->mtime : fs->atime;
	fs->ctime = (fs->ctime == 0) ? fs->mtime : fs->ctime;

	fs->mode = S_IREAD;
	fs->mode |= (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? S_IFDIR | S_IEXEC : S_IFREG;

	if (!(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
		fs->mode |= S_IWRITE;

	return SUCCEEDED;
}

int stat_file(const char *path, file_stat_t *fs)
{
	if (!path)
		return FAILED;

	WIN32_FIND_DATA wfd;
	HANDLE fh;

	fh = FindFirstFile(path, &wfd);
	if (fh == INVALID_HANDLE_VALUE)
	{
		//printf("Stat failed here: %d\n", GetLastError());
		return FAILED;
	}

	fs->file_size = ((uint64_t)wfd.nFileSizeHigh << 32) | wfd.nFileSizeLow;

	fs->ctime = FileTimeToUnixTime(&wfd.ftCreationTime, NULL);
	fs->atime = FileTimeToUnixTime(&wfd.ftLastAccessTime, NULL);
	fs->mtime = FileTimeToUnixTime(&wfd.ftLastWriteTime, NULL);

	fs->atime = (fs->atime == 0) ? fs->mtime : fs->atime;
	fs->ctime = (fs->ctime == 0) ? fs->mtime : fs->ctime;

	fs->mode = S_IREAD;
	fs->mode |= (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? S_IFDIR : S_IFREG;

	if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
		fs->mode |= S_IWRITE | S_IEXEC;

	if (FindNextFile(fh, &wfd))
	{
		FindClose(fh);
		//printf("Stat failed here.\n");
		return FAILED;
	}

	FindClose(fh);
	return SUCCEEDED;
}

#else

int create_start_thread(thread_t *thread, void *(*start_routine)(void*), void *arg)
{
	return pthread_create(thread, NULL, start_routine, arg);
}

int join_thread(thread_t thread)
{
	return pthread_join(thread, NULL);
}

file_t open_file(const char *path, int oflag)
{
	return (path) ? open(path, oflag) : INVALID_FD;
}

int close_file(file_t fd)
{
	return close(fd);
}

ssize_t read_file(file_t fd, void *buf, size_t nbyte)
{
	return read(fd, buf, nbyte);
}

ssize_t write_file(file_t fd, void *buf, size_t nbyte)
{
	return write(fd, buf, nbyte);
}

int64_t seek_file(file_t fd, int64_t offset, int whence)
{
	return lseek(fd, offset, whence);
}

int fstat_file(file_t fd, file_stat_t *fs)
{
	struct stat st;

	int ret = fstat(fd, &st);
	if (ret < SUCCEEDED)
		return ret;

	fs->file_size = st.st_size;
	fs->mtime = st.st_mtime;
	fs->ctime = st.st_ctime;
	fs->atime = st.st_atime;
	fs->mode = st.st_mode;
	return SUCCEEDED;
}

int stat_file(const char *path, file_stat_t *fs)
{
	if (!path)
		return FAILED;

	struct stat st;

	int ret = stat(path, &st);
	if (ret < SUCCEEDED)
		return ret;

	fs->file_size = st.st_size;
	fs->mtime = st.st_mtime;
	fs->ctime = st.st_ctime;
	fs->atime = st.st_atime;
	fs->mode = st.st_mode;
	return SUCCEEDED;
}

#endif
