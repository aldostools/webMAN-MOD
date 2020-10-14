#ifndef __ABSTRACTFILE_H__
#define __ABSTRACTFILE_H__

#include <cstdlib>
#include <cstdint>
#include "compat.h"

class AbstractFile
{
public:
	virtual ~AbstractFile() {}

	virtual int open(const char *path, int flags) = 0;
	virtual int close(void) = 0;
	virtual ssize_t read(void *buf, size_t nbyte) = 0;
	virtual ssize_t write(void *buf, size_t nbyte) = 0;
	virtual int64_t seek(int64_t offset, int whence) = 0;
	virtual int fstat(file_stat_t *fs) = 0;	
};


#endif
