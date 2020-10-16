#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <ctime>
#include <memory>

#include "common.h"

#include "VIsoFile.h"

#ifdef WIN32
#include "dirent2.h"


extern "C" int alphasort(const dirent2 **_a, const dirent2 **_b);
extern "C" int scandir(const char *dirname, struct dirent2 ***ret_namelist,
			int (*select)(const struct dirent2 *),
			int (*compar)(const struct dirent2 **, const struct dirent2 **));

#else
#include <dirent.h>
#define dirent2 dirent
#endif

#ifndef MAX_PATH
#define MAX_PATH	2048
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define MULTIEXTENT_PART_SIZE	0xFFFFF800

#define TEMP_BUF_SIZE	( 4*1024*1024)
#define FS_BUF_SIZE		(32*1024*1024)

#define SECTOR_SIZE		0x800ULL
#define SECTOR_MASK		0x7FFULL

static const int FAILED		= -1;
static const int SUCCEEDED	=  0;
static const int NONE		= -1;

static inline uint32_t bytesToSectors(off64_t size)
{
	return ((size + SECTOR_MASK) & ~SECTOR_MASK) / SECTOR_SIZE;
}

static char *dupString(const char *str, uint16_t len)
{
	char *ret = new char[len + 1];
	if (!ret)
		return NULL;

	strcpy(ret, str);
	return ret;
}

static uint8_t strncpy_upper(char *s1, const char *s2, uint8_t n)
{
	if ((!s1) || (!s2))
		return 0;

	for (uint8_t i = 0; i < n; i++)
	{
		s1[i] = s2[i];

		if (s2[i] == 0)
			return i;

		if ((s1[i] >= 'a') && (s1[i] <= 'z'))
			s1[i] -= ('a'-'A');
	}

	return n;
}

static char *createPath(uint16_t dlen, const char *dir, uint16_t flen, const char *file)
{
	char *ret = new char[dlen + flen + 2];
	sprintf(ret, "%s/%s", dir, file);
	return ret;
}

static bool getFileSizeAndProcessMultipart(char *file, off64_t *size, bool multipart, uint16_t len)
{
	file_stat_t statbuf;

	if (stat_file(file, &statbuf) < 0)
		return false;

	*size = statbuf.file_size;

	if ((!multipart) || (len < 6))
		return true;

	char *p = file + len - 6;
	if ((!p) || (strcmp(p + 1, ".66600") != SUCCEEDED))
		return true;

	off64_t prev_size = 0;

	for (int i = 1; i < 100 ; i++)
	{
		p[4] = '0' + (i / 10);
		p[5] = '0' + (i % 10);

		if (stat_file(file, &statbuf) < SUCCEEDED)
			break;

		*size += statbuf.file_size;

		if ((i > 1) && (prev_size & SECTOR_MASK))
			fprintf(stderr, "666XX file must be multiple of sector, except last fragment. (file=%s)\n", file);

		prev_size = statbuf.file_size;
	}

	*p = '\0';
	return true;
}

static void genIso9660Time(time_t t, Iso9660DirectoryRecord *record)
{
	struct tm *timeinfo = localtime(&t);
	record->year   = timeinfo->tm_year;
	record->month  = timeinfo->tm_mon + 1;
	record->day    = timeinfo->tm_mday;
	record->hour   = timeinfo->tm_hour;
	record->minute = timeinfo->tm_min;
	record->second = timeinfo->tm_sec;
}

static void genIso9660TimePvd(time_t t, char *volumeTime)
{
	struct tm *timeinfo = localtime(&t);
	int year  = timeinfo->tm_year + 1900;
	int month = timeinfo->tm_mon  + 1;

	volumeTime[0]  = (year / 1000) & 0xF;
	volumeTime[1]  = ((year - (volumeTime[0] * 1000)) / 100) & 0xF;
	volumeTime[2]  = ((year - (volumeTime[0] * 1000) - (volumeTime[1] * 100)) / 10) & 0xF;
	volumeTime[3]  = year  % 10;
	volumeTime[4]  = (month >= 10) ? 1 : 0;
	volumeTime[5]  = month % 10;
	volumeTime[6]  = (timeinfo->tm_mday / 10) & 0xF;
	volumeTime[7]  = (timeinfo->tm_mday % 10);
	volumeTime[8]  = (timeinfo->tm_hour / 10) & 0xF;
	volumeTime[9]  = (timeinfo->tm_hour % 10);
	volumeTime[10] = (timeinfo->tm_min  / 10) & 0xF;
	volumeTime[11] = (timeinfo->tm_min  % 10);
	volumeTime[12] = (timeinfo->tm_sec  / 10) & 0xF;
	volumeTime[13] = (timeinfo->tm_sec  % 10);
	volumeTime[14] = volumeTime[15] = volumeTime[16] = 0;

	for (int i = 0; i < 16; i++)
		volumeTime[i] += '0';
}

#ifdef WIN32

static int get_ucs2_from_utf8(const unsigned char *input, const unsigned char **end_ptr)
{
	// We are not really getting utf8, but 8-bits local charset. We only support ansi in win32, atm
	if (!input)
	{
		printf("viso error: no input in get_ucs2_from_utf8\n");
		return FAILED;
	}

	*end_ptr = input;

	if (input[0] == 0)
	{
		printf("viso error: blank input in get_ucs2_from_utf8\n");
		return FAILED;
	}

	//if (input[0] < 0x80)
	{
		*end_ptr = input + 1;
		return input[0];
	}

	fprintf(stderr, "Fatal error: windows version of ps3netsrv currently doesn't support non-ansi characters.\n, character giving error: 0x%02x\n", *input);
	return FAILED;
}

#else

static int get_ucs2_from_utf8(const unsigned char * input, const unsigned char ** end_ptr)
{
	if (!input)
	{
		printf("viso error: no input in get_ucs2_from_utf8\n");
		return FAILED;
	}

	*end_ptr = input;

	if (input[0] == 0)
	{
		printf("viso error: blank input in get_ucs2_from_utf8\n");
		return FAILED;
	}

	// assumes proper encoding
	if (input[0] < 0x80)
	{
		*end_ptr = input + 1;
		return input[0];
	}
	if ((input[0] & 0xE0) == 0xC0)
	{
		/*if (input[1] == 0)
		{
			printf("viso error 0xC0: in get_ucs2_from_utf8\n");
			return FAILED;
		}*/

		*end_ptr = input + 2;
		return (input[0] & 0x1F)<<6 | (input[1] & 0x3F);
	}
	if ((input[0] & 0xF0) == 0xE0)
	{
		/*if ((input[1] == 0) || (input[2] == 0))
		{
			printf("viso error 0xE0: in get_ucs2_from_utf8\n");
			return FAILED;
		}*/

		*end_ptr = input + 3;
		return (input[0] & 0x0F)<<12 | (input[1] & 0x3F)<<6 | (input[2] & 0x3F);
	}
	if ((input[0] & 0xF8) == 0xF0)
	{
		/*if ((input[1] == 0) || (input[2] == 0) || (input[3] == 0))
		{
			printf("viso error 0xF0: in get_ucs2_from_utf8\n");
			return FAILED;
		}*/

		*end_ptr = input + 4;
		return (input[0] & 0x07)<<18 | (input[1] & 0x3F)<<12 | (input[2] & 0x3F)<<6 | (input[3] & 0x3F);
	}
	if ((input[0] & 0xFC) == 0xF8)
	{
		/*if ((input[1] == 0) || (input[2] == 0) || (input[3] == 0) || (input[4] == 0))
		{
			printf("viso error 0xF8: in get_ucs2_from_utf8\n");
			return FAILED;
		}*/

		*end_ptr = input + 4;
		return (input[0] & 0x03)<<24 | (input[1] & 0x3F)<<18 | (input[2] & 0x3F)<<12 | (input[3] & 0x3F)<<6 | (input[4] & 0x3F);
	}
	if ((input[0] & 0xFE) == 0xFC)
	{
		/*if ((input[1] == 0) || (input[2] == 0) || (input[3] == 0) || (input[4] == 0) || (input[5] == 0))
		{
			printf("viso error 0xFC: in get_ucs2_from_utf8\n");
			return FAILED;
		}*/

		*end_ptr = input + 5;
		return (input[0] & 0x01)<<30 | (input[1] & 0x3F)<<24 | (input[2] & 0x3F)<<18 | (input[3] & 0x3F)<<12 | (input[4] & 0x3F)<<6 | (input[5] & 0x3F);
	}

	printf("fatal viso error: in get_ucs2_from_utf8\n");
	return FAILED;
}

#endif

static int utf8_to_ucs2(const unsigned char *utf8, uint16_t *ucs2, uint16_t maxLength)
{
	if ((!utf8) || (!ucs2) || (!maxLength))
		return 0;

	const unsigned char *p = utf8;
	int length = 0;

	while ((*p) && (length < maxLength))
	{
		int ch = get_ucs2_from_utf8(p, &p);
		if (ch < 0)
			break;

		ucs2[length++] = BE16(ch & 0xFFFF);
	}

	return length;
}

#ifdef WIN32
#include <windows.h>
#include <wincrypt.h>
static void get_rand(void *bfr, uint32_t size)
{
	if (size == 0)
		return;

	HCRYPTPROV hProv;

	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		fprintf(stderr, "Error aquiring crypt context.\n");

	if (!CryptGenRandom(hProv, size, static_cast<BYTE *>(bfr)))
		fprintf(stderr, "Errorgetting random numbers.\n");

	CryptReleaseContext(hProv, 0);
}
#else
static void get_rand(void *bfr, uint32_t size)
{
	if (size == 0)
		return;

	FILE *fp;

	fp = fopen("/dev/urandom", "r");
	if (fp == NULL)
		fprintf(stderr, "Error aquiring crypt context.\n");

	if (fread(bfr, size, 1, fp) != 1)
		fprintf(stderr, "Error getting random numbers.\n");

	fclose(fp);
}
#endif

static int parse_param_sfo(file_t fd, const char *field, char *field_value, int field_len)
{
	if ((!field) || (!field_len) || (!field_value) || (!field[0]))
	{
		printf("viso error: failed to parse PARAM.SFO\n");
		return FAILED;
	}

	if (FD_OK(fd))
	{
		unsigned len, pos, str;
		unsigned char *mem = NULL;

		len = seek_file(fd, 0, SEEK_END);

		mem = static_cast<unsigned char *>(malloc(len + 16));

		if (!mem)
		{
			close_file(fd);
			return -2;
		}

		memset(mem, 0, len + 16);

		seek_file(fd, 0, SEEK_SET);
		read_file(fd, mem, len);
		close_file(fd);

		str = (mem[0x8] + (mem[0x9] << 8));
		pos = (mem[0xc] + (mem[0xd] << 8));

		int indx = 0;

		while (str < len)
		{
			if (mem[str] == 0)
				break;

			if (!strcmp(reinterpret_cast<char *>(&mem[str]), field))
			{
				if (pos >= len)
					break;

				strncpy(field_value, reinterpret_cast<char *>(&mem[pos]), field_len);
				free(mem);
				return SUCCEEDED;
			}
			while (mem[str])
				str++;

			str++;
			pos += (mem[0x1c + indx] + (mem[0x1d + indx] << 8));
			indx += 16;
		}
		if (mem)
			free(mem);
	}

	printf("viso error: failed to parse PARAM.SFO\n");
	return FAILED;
}

static bool get_title_id(const char *dir, char *title_id)
{
	if ((!dir) || (!title_id))
		return false;

	char sfo_path[MAX_PATH];
	snprintf(sfo_path, sizeof(sfo_path) - 1, "%s/PS3_GAME/PARAM.SFO", dir);

	file_t fd;
	fd = open_file(sfo_path, O_RDONLY);
	if (!FD_OK(fd))
	{
		fprintf(stderr, "Cannot find %s\n", sfo_path);
		return false;
	}

	if (parse_param_sfo(fd, "TITLE_ID", title_id, 10) != SUCCEEDED)
	{
		fprintf(stderr, "TITLE_ID not found\n");
		return false;
	}

	return true;
}

static int select_directories(const struct dirent2 *entry)
{
	if (!entry)
		return false;

	if (entry->d_type & DT_DIR)
	{
		if ((strcmp(entry->d_name, ".") == SUCCEEDED) || (strcmp(entry->d_name, "..") == SUCCEEDED))
			return false;

		return true;
	}
	return false;
}

static int select_files(const struct dirent2 *entry)
{
	if ((!entry) || (entry->d_type & DT_DIR))
		return false;

	return true;
}

VIsoFile::VIsoFile(bool ps3Mode)
{
	this->ps3Mode = ps3Mode;

	fsBuf = NULL;
	tempBuf = NULL;
	pathTableL = NULL;
	pathTableM = NULL;
	pathTableJolietL = NULL;
	pathTableJolietM = NULL;
	rootList = NULL;

	vFilePtr = 0;
	filesSizeSectors = 0;
	dirsSizeSectors = 0;
	dirsSizeSectorsJoliet = 0;
	pathTableSize = 0;
	pathTableSizeJoliet = 0;
	totalSize = 0;
	padAreaStart = 0;
	padAreaSize = 0;
}

VIsoFile::~VIsoFile()
{
	DPRINTF("VISO file destructor\n");
	close();
	reset();
}

void VIsoFile::reset(void)
{
	if (fsBuf)
	{
		delete [] fsBuf;
		fsBuf = NULL;
	}

	if (tempBuf)
	{
		delete [] tempBuf;
		tempBuf = NULL;
	}

	if (pathTableL)
	{
		delete [] pathTableL;
		pathTableL = NULL;
	}

	if (pathTableM)
	{
		delete [] pathTableM;
		pathTableM = NULL;
	}

	if (pathTableJolietL)
	{
		delete [] pathTableJolietL;
		pathTableJolietL = NULL;
	}

	if (pathTableJolietM)
	{
		delete [] pathTableJolietM;
		pathTableJolietM = NULL;
	}

	while (rootList)
	{
		DirList *next = rootList->next;

		if (rootList->path)
			delete [] rootList->path;

		if (rootList->content)
			delete [] rootList->content;

		if (rootList->contentJoliet)
			delete [] rootList->contentJoliet;

		FileList *fileList = rootList->fileList;

		while (fileList)
		{
			FileList *nextFile = fileList->next;

			if (fileList->path)
				delete [] fileList->path;

			delete fileList;
			fileList = nextFile;
		}

		delete rootList;
		rootList = next;
	}

	vFilePtr              = 0;
	filesSizeSectors      = 0;
	dirsSizeSectors       = 0;
	dirsSizeSectorsJoliet = 0;
	pathTableSize         = 0;
	pathTableSizeJoliet   = 0;
	totalSize             = 0;
	padAreaStart          = 0;
	padAreaSize           = 0;
}

DirList *VIsoFile::getParent(DirList *dirList)
{
	if (!dirList)
		return dirList;

	if (dirList == rootList)
		return dirList;

	DirList *tempList = rootList;

	std::unique_ptr<char []> parentPath(dupString(dirList->path, dirList->full_len));
	if (!parentPath)
		return dirList;

	char *slash = strrchr(parentPath.get() + dirList->path_len, '/');
	if (slash)
		*slash = 0;

	while (tempList)
	{
		if (strcmp(parentPath.get(), tempList->path) == SUCCEEDED)
			return tempList;

		tempList = tempList->next;
	}

	return dirList;
}

bool VIsoFile::isDirectChild(DirList *dir, DirList *parentCheck)
{
	if ((!dir) || (!parentCheck))
		return false;

	if (strcmp(dir->path, parentCheck->path) == SUCCEEDED)
		return false;

	char *p = strstr(dir->path, parentCheck->path);
	if (p != dir->path)
		return false;

	p += parentCheck->full_len + 1;
	if (strchr(p, '/'))
		return false;

	return true;
}

Iso9660DirectoryRecord *VIsoFile::findDirRecord(const char *dirName, Iso9660DirectoryRecord *parentRecord, size_t size, bool joliet)
{
	uint8_t strCheck[256];
	uint8_t *buf, *p;
	uint32_t pos = 0;
	uint8_t strCheckSize;

	memset(strCheck, 0, 256);

	strCheckSize = (!joliet)
	        ? strncpy_upper(reinterpret_cast<char *>(strCheck), dirName, MAX_ISONAME - 2)
	        : utf8_to_ucs2(reinterpret_cast<const unsigned char *>(dirName), reinterpret_cast<uint16_t *>(strCheck), MAX_ISODIR / 2) * 2;

	buf = p = reinterpret_cast<uint8_t *>(parentRecord);

	while ((p < (buf + size)))
	{
		Iso9660DirectoryRecord *current = reinterpret_cast<Iso9660DirectoryRecord *>(p);

		if (!current)
			break;

		if (current->len_dr == 0)
		{
			p += (SECTOR_SIZE - (pos & SECTOR_MASK));
			pos += (SECTOR_SIZE - (pos & SECTOR_MASK));
			if (p >= (buf + size))
				break;

			current = reinterpret_cast<Iso9660DirectoryRecord *>(p);
			if (current->len_dr == 0)
				break;
		}

		if ((current->len_fi == strCheckSize) && (memcmp(&current->fi, strCheck, strCheckSize) == SUCCEEDED))
			return current;

		p += current->len_dr;
		pos += current->len_dr;
	}

	//printf("%s not found (joliet=%d)!!!!!!!\n", dirName, joliet);
	return NULL;
}

uint8_t *VIsoFile::buildPathTable(bool msb, bool joliet, size_t *retSize)
{
	DirList *dirList;
	uint8_t *p;
	int i = 0;

	memset(tempBuf, 0, tempBufSize);
	p = tempBuf;
	dirList = rootList;
	while ((dirList) && (i < 65536))
	{
		Iso9660PathTable *table = reinterpret_cast<Iso9660PathTable *>(p);
		Iso9660DirectoryRecord *record;
		uint16_t parentIdx;

		record = reinterpret_cast<Iso9660DirectoryRecord *>((joliet) ? dirList->contentJoliet : dirList->content);

		if (dirList == rootList)
		{
			table->len_di = 1;
			table->dirID = 0;
			parentIdx = 1;
		}
		else
		{
			DirList *parent;

			char *fileName = dirList->path + dirList->path_len + 1;

			table->len_di = (!joliet)
			        ? strncpy_upper(&table->dirID, fileName, MAX_ISODIR)
			        : utf8_to_ucs2(reinterpret_cast<const unsigned char *>(fileName), reinterpret_cast<uint16_t *>(&table->dirID), MAX_ISODIR / 2) * 2;

			parent = getParent(dirList);
			parentIdx = static_cast<uint16_t>(parent->idx) + 1;
		}

		if (msb)
		{
			table->dirLocation = record->msbStart;
			table->parentDN = BE16(parentIdx);
		}
		else
		{
			table->dirLocation = record->lsbStart;
			table->parentDN = LE16(parentIdx);
		}

		p += 8 + table->len_di;
		p += (table->len_di & 1) ? 1 : 0;

		dirList = dirList->next;
		i++;
	}

	*retSize = (p - tempBuf);
	uint8_t *ret = new uint8_t[*retSize];
	if (!ret)
		return 0;

	memcpy(ret, tempBuf, *retSize);
	return ret;
}

bool VIsoFile::buildContent(DirList *dirList, bool joliet)
{
	if ((!tempBuf) || (!tempBufSize) || (!dirList))
		return false;

	Iso9660DirectoryRecord *record, *parentRecord = NULL;
	DirList *tempList, *parent;
	uint8_t *p = tempBuf;
	file_stat_t statbuf;

	parent = getParent(dirList);

	if (!parent)
		return false;

	memset(tempBuf, 0, tempBufSize);

	// . entry
	record = reinterpret_cast<Iso9660DirectoryRecord *>(p);
	record->len_dr = 0x28;
	record->lsbStart = (!joliet) ? LE32(dirsSizeSectors) : LE32(dirsSizeSectorsJoliet);
	record->msbStart = (!joliet) ? BE32(dirsSizeSectors) : BE32(dirsSizeSectorsJoliet);

	if (stat_file(dirList->path, &statbuf) < 0)
		return false;

	genIso9660Time(statbuf.mtime, record);
	record->fileFlags = ISO_DIRECTORY;
	record->lsbVolSetSeqNum = LE16(1);
	record->msbVolSetSeqNum = BE16(1);
	record->len_fi = 1;
	p += 0x28;

	// .. entry
	record = reinterpret_cast<Iso9660DirectoryRecord *>(p);
	record->len_dr = 0x28;

	if (parent != dirList)
	{
		parentRecord = reinterpret_cast<Iso9660DirectoryRecord *>((joliet) ? parent->contentJoliet : parent->content);
		record->lsbStart = parentRecord->lsbStart;
		record->msbStart = parentRecord->msbStart;
		record->lsbDataLength = parentRecord->lsbDataLength;
		record->msbDataLength = parentRecord-> msbDataLength;
	}

	if (stat_file(parent->path, &statbuf) < 0)
		return false;

	genIso9660Time(statbuf.mtime, record);
	record->fileFlags = ISO_DIRECTORY;
	record->lsbVolSetSeqNum = LE16(1);
	record->msbVolSetSeqNum = BE16(1);
	record->len_fi = 1;
	record->fi = 1;
	p += 0x28;

	// Files entries
	FileList *fileList = dirList->fileList;

	while (fileList)
	{
		unsigned int parts = 1;
		uint32_t lba = fileList->rlba;

		if (fileList->size > 0xFFFFFFFF)
		{
		    parts = fileList->size / MULTIEXTENT_PART_SIZE;
		    parts += (fileList->size % MULTIEXTENT_PART_SIZE) ? 1 : 0;
		}

		for (unsigned int i = 0; i < parts; i++)
		{
			uint32_t offs;
			record = static_cast<Iso9660DirectoryRecord *>(malloc(SECTOR_SIZE));
			if (!record)
				return false;

			memset(record, 0, SECTOR_SIZE);

			record->lsbStart = LE32(lba);
			record->msbStart = BE32(lba);

			if (!fileList->multipart)
			{
				if (stat_file(fileList->path, &statbuf) < 0)
				{
					free(record);
					return false;
				}
			}
			else
			{
				std::unique_ptr<char []> s(new char[fileList->path_len + fileList->file_len + 8]);
				if (!s)
				{
					free(record);
					return false;
				}

				sprintf(s.get(), "%s.66600", fileList->path);

				if (stat_file(s.get(), &statbuf) < 0)
				{
					free(record);
					return false;
				}
			}

			genIso9660Time(statbuf.mtime, record);

			if (parts == 1)
			{
				record->fileFlags = ISO_FILE;
				record->lsbDataLength = LE32(fileList->size);
				record->msbDataLength = BE32(fileList->size);
			}
			else
			{
				uint32_t size;

				if (i == parts - 1)
				{
					size = (fileList->size) - (i * MULTIEXTENT_PART_SIZE);
					record->fileFlags = ISO_FILE;
				}
				else
				{
					size = MULTIEXTENT_PART_SIZE;
					record->fileFlags = ISO_MULTIEXTENT;
					lba += bytesToSectors(MULTIEXTENT_PART_SIZE);
				}

				record->lsbDataLength = LE32(size);
				record->msbDataLength = BE32(size);
			}

			record->lsbVolSetSeqNum = LE16(1);
			record->msbVolSetSeqNum = BE16(1);

			char *fileName = fileList->path + fileList->path_len + 1;

			if (!joliet)
			{
				record->len_fi = strncpy_upper(&record->fi, fileName, MAX_ISONAME - 2);
				strcat(&record->fi + record->len_fi, ";1");
				record->len_fi += 2;
			}
			else
			{
				uint16_t len = fileList->file_len;
				std::unique_ptr<char []> s(new char[len + 3]);
				if (!s) {
					free(record);
					return false;
				}
				strcpy(s.get(), fileName);
				strcpy(s.get() + len, ";1");
				record->len_fi = utf8_to_ucs2(reinterpret_cast<const unsigned char *>(s.get()), reinterpret_cast<uint16_t *>(&record->fi), MAX_ISONAME / 2) * 2;
			}

			record->len_dr = 0x27 + record->len_fi;
			record->len_dr += (record->len_dr & 1) ? 1 : 0;

			offs = (p - tempBuf);

			if ((offs / SECTOR_SIZE) < ((offs + record->len_dr) / SECTOR_SIZE))
			{
				offs = (offs + SECTOR_MASK) & ~SECTOR_MASK;
				p = (tempBuf + offs);
			}

			if ((p + record->len_dr) >= (tempBuf + tempBufSize))
			{
				free(record);
				return false;
			}

			memcpy(p, record, record->len_dr);
			p += record->len_dr;
			free(record);
		}

		fileList = fileList->next;
	}

	tempList = dirList->next;
	while (tempList)
	{
		if (isDirectChild(tempList, dirList))
		{
			uint32_t offs;
			record = static_cast<Iso9660DirectoryRecord *>(malloc(SECTOR_SIZE));
			memset(record, 0, SECTOR_SIZE);

			if (stat_file(tempList->path, &statbuf) < 0)
			{
				free(record);
				return false;
			}

			genIso9660Time(statbuf.mtime, record);
			record->fileFlags = ISO_DIRECTORY;

			record->lsbVolSetSeqNum = LE16(1);
			record->msbVolSetSeqNum = BE16(1);

			char *fileName = tempList->path + tempList->path_len + 1;

			record->len_fi = (!joliet)
				? strncpy_upper(&record->fi, fileName, MAX_ISODIR)
				: utf8_to_ucs2(reinterpret_cast<const unsigned char *>(fileName), reinterpret_cast<uint16_t *>(&record->fi), MAX_ISODIR / 2) * 2;

			record->len_dr = 0x27 + record->len_fi;
			record->len_dr += (record->len_dr & 1) ? 1 : 0;

			offs = (p - tempBuf);

			if ((offs / SECTOR_SIZE) < ((offs + record->len_dr) / SECTOR_SIZE))
			{
				offs = (offs + SECTOR_MASK) & ~SECTOR_MASK;
				p = (tempBuf + offs);
			}

			if ((p + record->len_dr) >= (tempBuf + tempBufSize))
			{
				free(record);
				return false;
			}

			memcpy(p, record, record->len_dr);
			p += record->len_dr;
			free(record);
		}

		tempList = tempList->next;
	}

	size_t size = (p - tempBuf);
	size = (size + SECTOR_MASK) & ~SECTOR_MASK;

	if (size > tempBufSize)
	{
		if (record)
			free(record);

		return false;
	}

	p = new uint8_t[size];
	if (!p)
	{
		if (record)
			free(record);

		return false;
	}
	memcpy(p, tempBuf, size);

	record = reinterpret_cast<Iso9660DirectoryRecord *>(p);
	record->lsbDataLength = LE32(size);
	record->msbDataLength = BE32(size);

	if (dirList == parent)
	{
		parentRecord = reinterpret_cast<Iso9660DirectoryRecord *>(p + 0x28);
		parentRecord->lsbStart = record->lsbStart;
		parentRecord->msbStart = record->msbStart;
		parentRecord->lsbDataLength = record->lsbDataLength;
		parentRecord->msbDataLength = record-> msbDataLength;
	}
	else
	{
		Iso9660DirectoryRecord *childRecord = findDirRecord(dirList->path + dirList->path_len + 1, parentRecord, (joliet) ? parent->contentJolietSize : parent->contentSize, joliet);
		if (!childRecord)
		{
			delete [] p;
			return false;
		}

		childRecord->lsbStart = record->lsbStart;
		childRecord->msbStart = record->msbStart;
		childRecord->lsbDataLength = record->lsbDataLength;
		childRecord->msbDataLength = record->msbDataLength;
	}

	if (!joliet)
	{
		dirList->content = p;
		dirList->contentSize = size;
		dirsSizeSectors += bytesToSectors(size);
	}
	else
	{
		dirList->contentJoliet = p;
		dirList->contentJolietSize = size;
		dirsSizeSectorsJoliet += bytesToSectors(size);
	}

	return true;
}

void VIsoFile::fixDirLba(Iso9660DirectoryRecord *record, size_t size, uint32_t dirLba, uint32_t filesLba)
{
	uint8_t *p, *buf;
	uint32_t pos = 0;

	buf = p = reinterpret_cast<uint8_t *>(record);

	while ((p < (buf + size)))
	{
		Iso9660DirectoryRecord *current = reinterpret_cast<Iso9660DirectoryRecord *>(p);

		if (current->len_dr == 0)
		{
			p   += (SECTOR_SIZE - (pos & SECTOR_MASK));
			pos += (SECTOR_SIZE - (pos & SECTOR_MASK));
			if (p >= (buf + size))
				break;

			current = reinterpret_cast<Iso9660DirectoryRecord *>(p);
			if (current->len_dr == 0)
				break;
		}

		if (current->fileFlags & ISO_DIRECTORY)
		{
			current->lsbStart = LE32(LE32(current->lsbStart) + dirLba);
			current->msbStart = BE32(BE32(current->msbStart) + dirLba);
		}
		else
		{
			current->lsbStart = LE32(LE32(current->lsbStart) + filesLba);
			current->msbStart = BE32(BE32(current->msbStart) + filesLba);
		}

		p += current->len_dr;
		pos += current->len_dr;
	}
}

void VIsoFile::fixPathTableLba(uint8_t *pathTable, size_t size, uint32_t dirLba, bool msb)
{
	uint8_t *p = pathTable;

	while ((p < (pathTable + size)))
	{
		Iso9660PathTable *table = reinterpret_cast<Iso9660PathTable *>(p);

		table->dirLocation = (msb) ? BE32(BE32(table->dirLocation) + dirLba) : LE32(LE32(table->dirLocation) + dirLba);

		p = p + 8 + table->len_di;
		p += (table->len_di & 1) ? 1 : 0;
	}
}

void VIsoFile::fixLba(uint32_t isoLba, uint32_t jolietLba, uint32_t filesLba)
{
	DirList *dirList = rootList;

	while (dirList)
	{
		fixDirLba(reinterpret_cast<Iso9660DirectoryRecord *>(dirList->content), dirList->contentSize, isoLba, filesLba);
		fixDirLba(reinterpret_cast<Iso9660DirectoryRecord *>(dirList->contentJoliet), dirList->contentJolietSize, jolietLba, filesLba);
		dirList = dirList->next;
	}

	fixPathTableLba(pathTableL, pathTableSize, isoLba, false);
	fixPathTableLba(pathTableM, pathTableSize, isoLba, true);
	fixPathTableLba(pathTableJolietL, pathTableSizeJoliet, jolietLba, false);
	fixPathTableLba(pathTableJolietM, pathTableSizeJoliet, jolietLba, true);
}

bool VIsoFile::build(const char *inDir)
{
	uint16_t dlen = strlen(inDir);
	if (dlen < 1)
		return false;

	rootList = new DirList;
	if (!rootList)
		return false;

	DirList *dirList, *tail;
	struct dirent2 **dirs;
	int count;
	int idx = 0;
	uint16_t flen;

	rootList->path = dupString(inDir, dlen);
	if (rootList->path == NULL)
		return false;

	rootList->path_len = dlen;
	rootList->full_len = dlen;
	rootList->content = NULL;
	rootList->contentJoliet = NULL;
	rootList->fileList = NULL;
	rootList->idx = idx++;
	rootList->next = NULL;
	dirList = tail = rootList;

	while (dirList)
	{
		count = scandir(dirList->path, &dirs, select_directories, alphasort);
		if (count < 0)
			return false;

		dlen = dirList->full_len;
		for (int i = 0; i < count; i++)
		{
			#ifdef WIN32
			flen = dirs[i]->d_namlen;
			#else
			flen = strlen(dirs[i]->d_name);
			#endif
			tail = tail->next = new DirList;
			tail->path = createPath(dlen, dirList->path, flen, dirs[i]->d_name);
			tail->path_len = dlen;
			tail->full_len = dlen + flen + 1;
			tail->content = NULL;
			tail->contentJoliet = NULL;
			tail->idx = idx++;
			tail->fileList = NULL;
			tail->next = NULL;

			free(dirs[i]);
		}

		if (dirs)
			free(dirs);

		dirList = dirList->next;
	}

	dirList = rootList;
	while (dirList)
	{
		struct dirent2 **files;
		FileList *fileList = NULL;
		bool error = false;
		uint16_t dlen;

		count = scandir(dirList->path, &files, select_files, alphasort);

		if (files) {
			dlen = dirList->full_len;
		} else {
			dlen = 0;
			count = 0;
		}

		for (int i = 0; i < count; i++)
		{
			if (!error)
			{
				bool multipart = false;

				#ifdef WIN32
				flen = files[i]->d_namlen;
				#else
				flen = strlen(files[i]->d_name);
				#endif

				if (flen > 6)
				{
					char *p = files[i]->d_name + flen - 6;

					if ((p[0] == '.') && (p[1] == '6') && (p[2] == '6') && (p[3] == '6') && (isdigit(p[4])) && (isdigit(p[5])))
					{
						multipart = true;

						if ((p[4] != '0') || (p[5] != '0'))
						{
							free(files[i]);
							continue;
						}
					}
				}

				fileList = (i == 0) ? (dirList->fileList = new FileList) : (fileList->next = new FileList);

				if (fileList)
				{
					fileList->path = createPath(dlen, dirList->path, flen, files[i]->d_name);
					fileList->path_len = dlen;
					fileList->file_len = flen;
					fileList->multipart = multipart;
					fileList->next = NULL;

					if (getFileSizeAndProcessMultipart(fileList->path, &fileList->size, multipart, dlen + flen))
					{
						fileList->rlba = filesSizeSectors;
						filesSizeSectors += bytesToSectors(fileList->size);
					}
					else {
						error = true;
					}
				}
				else {
					error = true;
				}
			}
			free(files[i]);
		}

		if (files)
			free(files);

		if (error)
			return false;

		dirList = dirList->next;
	}

	// Add iso directories
	dirList = rootList;
	while (dirList)
	{
		if (!buildContent(dirList, false))
			return false;

		dirList = dirList->next;
	}

	// Add joliet directories
	dirList = rootList;
	while (dirList)
	{
		if (!buildContent(dirList, true))
			return false;

		dirList = dirList->next;
	}

	pathTableL = buildPathTable(false, false, &pathTableSize);
	pathTableM = buildPathTable(true, false, &pathTableSize);
	pathTableJolietL = buildPathTable(false, true, &pathTableSizeJoliet);
	pathTableJolietM = buildPathTable(true, true, &pathTableSizeJoliet);

	uint32_t isoLba = (0xA000 / SECTOR_SIZE) + (bytesToSectors(pathTableSize) * 2) + (bytesToSectors(pathTableSizeJoliet) * 2);
	uint32_t jolietLba = isoLba + dirsSizeSectors;
	uint32_t filesLba = jolietLba + dirsSizeSectorsJoliet;

	fixLba(isoLba, jolietLba, filesLba);

	return true;
}

void VIsoFile::write(const char *volumeName, const char *gameCode)
{
	DirList *dirList;
	Iso9660PVD *pvd;
	uint8_t *p;

	time_t t = time(NULL);

	// Write first 16 empty sectors
	memset(fsBuf, 0, 0x8000);

	if (ps3Mode)
	{
		DiscRangesSector *rangesSector;
		DiscInfoSector *infoSector;

		rangesSector = reinterpret_cast<DiscRangesSector *>(fsBuf);
		infoSector = reinterpret_cast<DiscInfoSector *>(fsBuf + SECTOR_SIZE);

		// Sector 0
		rangesSector->numRanges = BE32(1);
		rangesSector->ranges[0].startSector = BE32(0);
		rangesSector->ranges[0].endSector = BE32(volumeSize - 1);

		//Sector 1
		strcpy(infoSector->consoleId, "PlayStation3");
		memset(infoSector->productId, ' ', sizeof(infoSector->productId));
		memcpy(infoSector->productId, gameCode, 4);
		infoSector->productId[4] = '-';
		strncpy(infoSector->productId + 5, gameCode + 4, 5);
		get_rand(infoSector->info,  sizeof(infoSector->info));
		get_rand(infoSector->hash, sizeof(infoSector->hash));
	}

	// Generate and write iso pvd
	pvd = reinterpret_cast<Iso9660PVD *>(fsBuf + 0x8000);
	memset(pvd, 0, SECTOR_SIZE);

	pvd->VDType = 1;
	memcpy(pvd->VSStdId, "CD001", sizeof(pvd->VSStdId));
	pvd->VSStdVersion = 1;
	memset(pvd->systemIdentifier, ' ', sizeof(pvd->systemIdentifier));

	uint8_t i = strncpy_upper(pvd->volumeIdentifier, volumeName, sizeof(pvd->volumeIdentifier));
	for (; i < sizeof(pvd->volumeIdentifier); i++)
	{
		pvd->volumeIdentifier[i] = ' ';
	}

	pvd->lsbVolumeSpaceSize = LE32(volumeSize);
	pvd->msbVolumeSpaceSize = BE32(volumeSize);
	pvd->lsbVolumeSetSize = LE16(1);
	pvd->msbVolumeSetSize = BE16(1);
	pvd->lsbVolumeSetSequenceNumber = LE16(1);
	pvd->msbVolumeSetSequenceNumber = BE16(1);
	pvd->lsbLogicalBlockSize = LE16(SECTOR_SIZE);
	pvd->msbLogicalBlockSize = BE16(SECTOR_SIZE);
	pvd->lsbPathTableSize = LE32(pathTableSize);
	pvd->msbPathTableSize = BE32(pathTableSize);
	pvd->lsbPathTable1 = LE32(0xA000 / SECTOR_SIZE);
	pvd->msbPathTable1 = BE32((0xA000 / SECTOR_SIZE) + bytesToSectors(pathTableSize));
	memset(pvd->volumeSetIdentifier, ' ', sizeof(pvd->volumeSetIdentifier));
	memcpy(pvd->volumeSetIdentifier, pvd->volumeIdentifier, sizeof(pvd->volumeIdentifier));
	memset(pvd->publisherIdentifier, ' ', sizeof(pvd->publisherIdentifier));
	memset(pvd->dataPreparerIdentifier, ' ', sizeof(pvd->dataPreparerIdentifier));
	memset(pvd->applicationIdentifier, ' ', sizeof(pvd->applicationIdentifier));
	memset(pvd->copyrightFileIdentifier, ' ', sizeof(pvd->copyrightFileIdentifier));
	memset(pvd->abstractFileIdentifier, ' ', sizeof(pvd->abstractFileIdentifier));
	memset(pvd->bibliographicFileIdentifier, ' ', sizeof(pvd->bibliographicFileIdentifier));
	genIso9660TimePvd(t, pvd->volumeCreation);
	memset(pvd->volumeModification, '0', 16);
	memset(pvd->volumeExpiration, '0', 16);
	memset(pvd->volumeEffective, '0', 16);
	pvd->FileStructureStandardVersion = 1;
	memcpy(pvd->rootDirectoryRecord, rootList->content, sizeof(pvd->rootDirectoryRecord));
	pvd->rootDirectoryRecord[0] = sizeof(pvd->rootDirectoryRecord);

	// Write joliet pvd
	pvd = reinterpret_cast<Iso9660PVD *>(fsBuf + 0x8800);

	memset(pvd, 0, SECTOR_SIZE);

	pvd->VDType = 2;
	memcpy(pvd->VSStdId, "CD001", sizeof(pvd->VSStdId));
	pvd->VSStdVersion = 1;
	memset(pvd->systemIdentifier, 0, sizeof(pvd->systemIdentifier));
	utf8_to_ucs2(reinterpret_cast<const unsigned char *>(volumeName), reinterpret_cast<uint16_t *>(pvd->volumeIdentifier), sizeof(pvd->volumeIdentifier) / 2);
	pvd->lsbVolumeSpaceSize = LE32(volumeSize);
	pvd->msbVolumeSpaceSize = BE32(volumeSize);
	pvd->escapeSequences[0] = '%';
	pvd->escapeSequences[1] = '/';
	pvd->escapeSequences[2] = '@';
	pvd->lsbVolumeSetSize = LE16(1);
	pvd->msbVolumeSetSize = BE16(1);
	pvd->lsbVolumeSetSequenceNumber = LE16(1);
	pvd->msbVolumeSetSequenceNumber = BE16(1);
	pvd->lsbLogicalBlockSize = LE16(SECTOR_SIZE);
	pvd->msbLogicalBlockSize = BE16(SECTOR_SIZE);
	pvd->lsbPathTableSize = LE32(pathTableSizeJoliet);
	pvd->msbPathTableSize = BE32(pathTableSizeJoliet);
	pvd->lsbPathTable1 = LE32(0xA000 / SECTOR_SIZE + (bytesToSectors(pathTableSize)*2));
	pvd->msbPathTable1 = BE32(0xA000 / SECTOR_SIZE + (bytesToSectors(pathTableSize)*2) + bytesToSectors(pathTableSizeJoliet));
	memcpy(pvd->volumeSetIdentifier, pvd->volumeIdentifier, sizeof(pvd->volumeIdentifier));
	genIso9660TimePvd(t, pvd->volumeCreation);
	memset(pvd->volumeModification, '0', 16);
	memset(pvd->volumeExpiration, '0', 16);
	memset(pvd->volumeEffective, '0', 16);
	pvd->FileStructureStandardVersion = 1;
	memcpy(pvd->rootDirectoryRecord, rootList->contentJoliet, sizeof(pvd->rootDirectoryRecord));
	pvd->rootDirectoryRecord[0] = sizeof(pvd->rootDirectoryRecord);

	// Write sector 18
	p = fsBuf + 0x9000;
	memset(p, 0, SECTOR_SIZE);
	p[0] = 0xFF;
	memcpy(p + 1, "CD001", 5);

	// Write empty sector 19
	p = fsBuf + 0x9800;
	memset(p, 0, SECTOR_SIZE);

	// Write pathTableL
	p = fsBuf + 0xA000;
	memset(p, 0, bytesToSectors(pathTableSize) * SECTOR_SIZE);
	memcpy(p, pathTableL, pathTableSize);

	// Write pathTableM
	p += (bytesToSectors(pathTableSize) * SECTOR_SIZE);
	memcpy(p, pathTableM, pathTableSize);

	// Write pathTableJolietL
	p += (bytesToSectors(pathTableSize) * SECTOR_SIZE);
	memset(p, 0, bytesToSectors(pathTableSizeJoliet) * SECTOR_SIZE);
	memcpy(p, pathTableJolietL, pathTableSizeJoliet);

	// Write pathTableJolietM
	p += (bytesToSectors(pathTableSizeJoliet) * SECTOR_SIZE);
	memcpy(p, pathTableJolietM, pathTableSizeJoliet);

	p += (bytesToSectors(pathTableSizeJoliet) * SECTOR_SIZE);

	delete [] pathTableL;
	delete [] pathTableM;
	delete [] pathTableJolietL;
	delete [] pathTableJolietM;
	pathTableL = NULL;
	pathTableM = NULL;
	pathTableJolietL = NULL;
	pathTableJolietM = NULL;

	// Write iso directories
	dirList = rootList;
	while (dirList)
	{
		memcpy(p, dirList->content, dirList->contentSize);
		p += dirList->contentSize;
		dirList = dirList->next;
	}

	// Write joliet directories
	dirList = rootList;
	while (dirList)
	{
		memcpy(p, dirList->contentJoliet, dirList->contentJolietSize);
		p += dirList->contentJolietSize;
		dirList = dirList->next;
	}
}

bool VIsoFile::generate(const char *inDir, const char *volumeName, const char *gameCode)
{
	if (stat_file(inDir, &st) != SUCCEEDED)
	{
		printf("viso error: path not found \"%s\"\n", inDir);
		return FAILED;
	}

	tempBufSize = TEMP_BUF_SIZE;
	tempBuf = new uint8_t[TEMP_BUF_SIZE];
	if (!tempBuf)
		return false;

	off64_t padSectors;
	bool ret;

	ret = build(inDir);
	delete[] tempBuf;
	tempBuf = NULL;

	if (!ret)
		return false;

	fsBufSize = (0xA000 / SECTOR_SIZE) + (bytesToSectors(pathTableSize) * 2) + (bytesToSectors(pathTableSizeJoliet) * 2) + dirsSizeSectors + dirsSizeSectorsJoliet;
	volumeSize =  fsBufSize + filesSizeSectors;
	padAreaStart = static_cast<uint64_t>(volumeSize) * SECTOR_SIZE;
	padSectors = 0x20;

	if (volumeSize & 0x1F)
		padSectors += (0x20 - (volumeSize & 0x1F));

	padAreaSize = padSectors * SECTOR_SIZE;
	volumeSize += padSectors;
	totalSize = volumeSize;

	fsBufSize = fsBufSize * SECTOR_SIZE;
	totalSize = totalSize * SECTOR_SIZE;

	if (fsBuf)
		delete [] fsBuf;

	fsBuf = new uint8_t[fsBufSize];
	if (!fsBuf)
		return false;

	memset(fsBuf, 0, fsBufSize);

	write(volumeName, gameCode);
	return true;
}

int VIsoFile::open(const char *path, int flags)
{
	if (!path)
	{
		printf("viso error: no path in open\n");
		return FAILED;
	}

	if (flags != O_RDONLY)
	{
		printf("viso error: flags not read only\n");
		return FAILED;
	}

	if (fsBuf)
		close();

	if (stat_file(path, &st) != SUCCEEDED)
	{
		printf("viso error: path not found \"%s\"\n", path);
		return FAILED;
	}

	if ((st.mode & S_IFDIR) != S_IFDIR)
	{
		printf("viso error: not a directory \"%s\"\n", path);
		return FAILED;
	}

	char gameCode[16];
	char volumeName[32];

	if (ps3Mode)
	{
		if (!get_title_id(path, gameCode))
		{
			printf("viso error: getting title id in PS3 MODE\n");
			return FAILED;
		}

		strcpy(volumeName, "PS3VOLUME");
	}
	else
	{
		const char *dn = strrchr(path, '/');
		dn = (dn) ? dn + 1 : path;

		snprintf(volumeName, sizeof(volumeName), "%s", dn);
	}

	if (!generate(path, volumeName, gameCode))
	{
		printf("viso error: failed to create virtual iso\n");
		return FAILED;
	}

	return SUCCEEDED;
}

int VIsoFile::close(void)
{
	if (!fsBuf)
		return FAILED;

	reset();
	return SUCCEEDED;
}

ssize_t VIsoFile::read(void *buf, size_t nbyte)
{
	if (!buf)
	{
		printf("viso error: no read buffer\n");
		return FAILED;
	}

	DirList *dirList;
	uint64_t remaining, to_read;
	uint64_t r;
	uint8_t *p;

	if (!fsBuf)
	{
		printf("viso error: file error in read\n");
		return FAILED;
	}

	remaining = nbyte;
	r = 0;
	p = static_cast<uint8_t *>(buf);

	if ((vFilePtr >= totalSize) || (remaining == 0))
	{
		return SUCCEEDED;
	}
	else if (vFilePtr < 0)
	{
		printf("viso error: pointer error in read\n");
		return FAILED;
	}

	if (vFilePtr < static_cast<off64_t>(fsBufSize))
	{
		// Read FS structure from RAM
		to_read = MIN(fsBufSize - vFilePtr, remaining);
		memcpy(p, fsBuf + vFilePtr, to_read);

		remaining -= to_read;
		r += to_read;
		p += to_read;
		vFilePtr += to_read;
	}

	if ((remaining == 0) || (vFilePtr >= totalSize))
		return r;

	if (vFilePtr < padAreaStart)
	{
		// Read from file(s)
		dirList = rootList;
		while (dirList)
		{
			FileList *fileList = dirList->fileList;

			while (fileList)
			{
				off64_t fStart = static_cast<uint64_t>(fsBufSize) + static_cast<uint64_t>(fileList->rlba) * SECTOR_SIZE;
				off64_t fEnd = fStart + fileList->size;
				off64_t fEndSector = ((fEnd + SECTOR_MASK) & ~SECTOR_MASK);

				if ((vFilePtr >= fStart) && (vFilePtr < fEndSector))
				{
					if (fileList->multipart)
					{
						fprintf(stderr, "Sorry no support for 666 files. I have the feeling that your game is about to crash ^_^\n");
						return r;
					}

					if (vFilePtr < fEnd)
					{
						file_t fd;
						int64_t this_r;

						to_read = MIN(static_cast<uint64_t>(fileList->size - (vFilePtr - fStart)), remaining);
						fd = open_file(fileList->path, O_RDONLY);

						if (!FD_OK(fd))
						{
							fprintf(stderr, "VISO: file %s cannot be opened!\n", fileList->path);
							return r;
						}

						seek_file(fd, vFilePtr - fStart, SEEK_SET);
						this_r = read_file(fd, p, to_read);
						close_file(fd);

						if (this_r < 0)
						{
							fprintf(stderr, "VISO: read_file failed on %s\n", fileList->path);
							return r;
						}

						if (static_cast<uint64_t>(this_r) != to_read)
						{
							fprintf(stderr, "VISO: read on file %s returned less data than expected (file modified?)\n", fileList->path);
							return r;
						}

						remaining -= to_read;
						r += to_read;
						p += to_read;
						vFilePtr += to_read;
					}

					if ((remaining > 0) && (fEnd != fEndSector))
					{
						// This is a zero area after the file to fill the sector
						to_read = MIN(static_cast<uint64_t>((fEndSector - fEnd) - (vFilePtr - fEnd)), remaining);
						memset(p, 0, to_read);

						remaining -= to_read;
						r += to_read;
						p += to_read;
						vFilePtr += to_read;
					}

					if (remaining == 0)
						return r;
				}

				fileList = fileList->next;
			}

			dirList = dirList->next;
		}
	}

	if ((vFilePtr >= padAreaStart) && (vFilePtr < totalSize))
	{
		// Pad at the end
		off64_t read_size = padAreaSize - (vFilePtr - padAreaStart);
		if (read_size > 0)
			to_read = MIN(static_cast<uint64_t>(read_size), remaining);
		else
			return r; // to_read = 0;

		memset(p, 0, to_read);

		remaining -= to_read;
		r += to_read;
		p += to_read;
		vFilePtr += to_read;
	}

	return r;
}

ssize_t VIsoFile::write(void *buf, size_t nbyte)
{
	(void) buf;
	(void) nbyte;
	printf("viso error: write error\n");
	return FAILED;
}

int64_t VIsoFile::seek(int64_t offset, int whence)
{
	if (!fsBuf)
	{
		printf("viso error: seek error\n");
		return FAILED;
	}

	if (whence == SEEK_SET)
		vFilePtr = offset;
	else if (whence == SEEK_CUR)
		vFilePtr += offset;
	else if (whence == SEEK_END)
		vFilePtr = totalSize + offset;

	return vFilePtr;
}

int VIsoFile::fstat(file_stat_t *fs)
{
	if ((!fsBuf) || (!fs))
	{
		printf("viso error: fstat error\n");
		return FAILED;
	}

	fs->file_size = totalSize;
	fs->mode = S_IFREG;

	fs->mtime = st.mtime;
	fs->atime = st.atime;
	fs->ctime = st.ctime;

	return SUCCEEDED;
}
