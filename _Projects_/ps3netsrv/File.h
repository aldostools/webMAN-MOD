#ifndef __FILE_H__
#define __FILE_H__

// 2015 added support for multi-part ISOs by Aldo Vargas
// 2019 added support for encrypted PS3 ISOs (3k3y / Redump) by NvrBst 2015-01-21 (issue #225 posted by SpaceAgeHero)
// More info: http://forum.redump.org/topic/14472/ps3netsrv-modified-for-encrypted-3k3yredump-isos/

/*
QUOTE: NvrBst; 2015-01-21 @ redump.org
I recently updated it to use .dkeys instead of .keys (the r2).
Basically it's the origional ps3netsrv with support for 3k3y/redump ps3 isos.
ISO's have to be in the ".../PS3ISO/" folder (as seen by ps3netsvr),
.dkeys can also be with the .iso (origional logic)
or in the ".../REDKEY/" folder (with same name as the corresponding encrypted redump iso).
keep in mind webMAN has a 866 file limit for share directories and having .dkey/.iso together can have you more quickly hit this limit.

NOTE by AV: the limit has been increased to 4096

ps3netsrv-src-nvrbst information:
* Only files edited are "File.h" and "File.cpp" (2 methods).
  I hijacked the "open" method to test if the file is encrypted and sets a flag.
  Flag is set if the file being opened is ".iso" and the 3k3y watermark exsists at 0xF70,
  or if a ".key" file with the same name exists.
  If the flag is set the "read" method decrypts the data before sending it the ps3 (if needed).
* 3k3y (decrypted / encrypted) format is now supported (.iso).
* Redump (encrypted) format is now supported (.iso & .dkey).
* Decrypted isos (along with all other origional features of ps3netsvr) are still supported.
* Encrypted split iso's are unsupported
* I made a small change in main.cpp to fix a warning about undefined behavior with deleting a non-virtual base class.
* I made small changes to fix all gcc warnings.
* I modified the Makefile to have some common flags for release/debug and upped the stack size.
* To build g++ 4.6.1 is needed (g++ 4.8.1 builds break JB games).


EDIT: Also I didn't look into the g++ 4.8.1 problem much. If anyone knows why, able to confirm the problem, or has built ps3netsrv (origional or modified) with g++ 4.8.1 for win32 (and jb format games still work) then please let me know smile, thanks.
*/

#include <mbedtls/aes.h>

#include "AbstractFile.h"


// Struct to store region information (storing addrs instead of lba since we need to compare the addr anyway, so would have to multiply or divide every read if storing lba).
struct PS3RegionInfo
{
	bool encrypted;
	int64_t regions_first_addr;
	int64_t regions_last_addr;
};

// Enum to decide the Files encryption type.
enum PS3EncDiscType
{
	kDiscTypeNone,
	kDiscType3k3yDec,
	kDiscType3k3yEnc,
	kDiscTypeRedump
};

class File : public AbstractFile
{
 protected:
	file_t fd;

	/////////////////////////////////////////////////
	//// support multi-part isos by Aldo Vargas /////
	/////////////////////////////////////////////////

	file_t fp[64];
	int8_t is_multipart;
	int64_t part_size;
	int8_t index;

 public:
	File();
	virtual ~File();

	virtual int open(const char *path, int flags);
	virtual int close(void);
	virtual ssize_t read(void *buf, size_t nbyte);
	virtual ssize_t write(void *buf, size_t nbyte);
	virtual int64_t seek(int64_t offset, int whence);
	virtual int fstat(file_stat_t *fs);

#ifndef NOSSL
	/////////////////////////////////////////////////
	///// encrypted-3k3yredump-isos by NvrBst ///////
	/////////////////////////////////////////////////

 private:
	static const size_t kSectorSize = 2048;

	// Decryption related functions.
	unsigned char asciischar_to_byte(char input);
	void keystr_to_keyarr(const char (&str)[32], unsigned char (&arr)[16]);
	unsigned int char_arr_BE_to_uint(unsigned char *arr);
	void reset_iv(unsigned char (&iv)[16], unsigned int lba);
	void decrypt_data(mbedtls_aes_context &aes, unsigned char *data, int sector_count, unsigned int start_lba);
	void init_region_info(void);

	// Decryption related variables.
	PS3EncDiscType enc_type_;
	mbedtls_aes_context aes_dec_;
	size_t region_count_;
	PS3RegionInfo *region_info_;

	// Micro optimization, only ever used by decrypt_data(...).
	unsigned char iv_[16];
#endif
};

#endif
