#ifndef __VISOFILE_H__
#define __VISOFILE_H__

#include "AbstractFile.h"
#include "File.h"
#include "compat.h"
#include "iso9660.h"

typedef struct _FileList
{
	char *path;
	uint32_t rlba;
	off64_t size;
	bool multipart;
	struct _FileList *next;
} FileList;

typedef struct _DirList
{
	char *path;
	uint16_t path_len;
	uint8_t *content;
	uint8_t *contentJoliet;
	size_t contentSize;
	size_t contentJolietSize;
	int idx;
	FileList *fileList;
	struct _DirList *next;
} DirList;

typedef struct 
{
	/*00*/uint32_t startSector;// first sector of the range (inclusive)
	/*04*/uint32_t endSector; // last sector of the range (inclusive)
} SectorRangeEntry;

typedef struct
{
	/*00*/uint32_t numRanges; // number of unencrypted sector ranges
	/*04*/uint32_t zero; // ? (this is set to zero)
	/*08*/SectorRangeEntry ranges[255]; // all unencrypted sector ranges
} DiscRangesSector;

// 0x200 bytes
typedef struct 
{
	/*000*/char consoleId[0x10];	// "PlayStation3\0\0\0\0"
	/*010*/char productId[0x20];	// "BCES-00104	  ", "				"
	/*030*/uint8_t zeros[0x10]; 	// all 0x00
	/*040*/uint8_t info[0x1B0]; 	// disc info
	/*1F0*/uint8_t	hash[0x10]; 	// a hash 0x1B0 bytes of 'info' above
} DiscInfoSector;

class VIsoFile : public AbstractFile
{
private:
	
	bool ps3Mode;
	
	off64_t vFilePtr;
	
	uint8_t *fsBuf;
	uint8_t *tempBuf;
	
	size_t fsBufSize;
	size_t tempBufSize;
		
	DirList *rootList;
	
	uint32_t filesSizeSectors;
	uint32_t dirsSizeSectors;
	uint32_t dirsSizeSectorsJoliet;
	
	uint8_t *pathTableL;
	uint8_t *pathTableM;	
	uint8_t *pathTableJolietL;
	uint8_t *pathTableJolietM;
	
	size_t pathTableSize;
	size_t pathTableSizeJoliet;	
	
	uint32_t volumeSize;
	off64_t totalSize;
	off64_t padAreaStart;
	off64_t padAreaSize;
	
	void reset(void);
	
	DirList *getParent(DirList *dirList);	
	bool isDirectChild(DirList *dir, DirList *parentCheck);
	Iso9660DirectoryRecord *findDirRecord(char *dirName, Iso9660DirectoryRecord *parentRecord, size_t size, bool joliet);
	
	uint8_t *buildPathTable(bool msb, bool joliet, size_t *retSize);
	bool buildContent(DirList *dirList, bool joliet);
	void fixDirLba(Iso9660DirectoryRecord *record, size_t size, uint32_t dirLba, uint32_t filesLba);
	void fixPathTableLba(uint8_t *pathTable, size_t size, uint32_t dirLba, bool msb);
	void fixLba(uint32_t isoLba, uint32_t jolietLba, uint32_t filesLba);
	bool build(char *inDir);
	void write(const char *volumeName, const char *gameCode);
	bool generate(char *inDir, const char *volumeName, const char *gameCode);
	
public:
	VIsoFile(bool ps3Mode);
	~VIsoFile();
	
	virtual int open(const char *path, int flags);
	virtual int close(void);
	virtual ssize_t read(void *buf, size_t nbyte);
	virtual ssize_t write(void *buf, size_t nbyte);
	virtual int64_t seek(int64_t offset, int whence);
	virtual int fstat(file_stat_t *fs);
};

#endif


