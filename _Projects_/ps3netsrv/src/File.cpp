#include "File.h"

#include <cstdio>
#include <cstring>
#include <memory>

#include "aes.h"
#include "common.h"
#include "compat.h"

static const int FAILED		= -1;
static const int SUCCEEDED	=  0;

#ifdef OPT_FILE_SEEK
#define add_last_seek(n)	last_seek += n;
#define set_last_seek(n)	last_seek = n;
#define  is_last_seek(n)	(last_seek == n)
#else
#define add_last_seek(n)
#define set_last_seek(n)
#define  is_last_seek(n)	(0)
#endif

#ifdef NOSSL
File::File()
#else
File::File() : enc_type_(kDiscTypeNone), region_count_(0), region_info_(NULL)
#endif
{
	fd = INVALID_FD;

	set_last_seek(0);

	is_multipart = 0;
	index = 0;

	for (uint8_t i = 0; i < 64; i++)
		fp[i] = INVALID_FD;
}

#ifndef NOSSL
void File::init_region_info(void)
{
	region_count_ = 0;
	enc_type_ = kDiscTypeNone;

	if (region_info_ != NULL)
	{
		delete [] region_info_;
		region_info_ = NULL;
	}
}
#else
#define init_region_info()
#endif

File::~File()
{
	DPRINTF("File destructor.\n");
	init_region_info();

	if (FD_OK(fd))
		this->close();
}

int File::open(const char *path, int flags)
{
	init_region_info();

	if (!path)
	{
		printf("file error: no path\n");
		return FAILED;
	}

	if (FD_OK(fd))
		this->close();

	set_last_seek(0); // not for multi-iso

	fd = open_file(path, flags);
	if (!FD_OK(fd))
	{
		printf("file error: opening \"%s\"\n", path);
		return FAILED;
	}

	// is multi part? (2015-2019 AV)
	int plen = strlen(path);
	int flen = plen - 6;

	if (flen < 0) {
		flen = 0;
		is_multipart = 0;
	} else {
		is_multipart = (strstr(path + flen, ".iso.0") != NULL) || (strstr(path + flen, ".ISO.0") != NULL);
	}

	///// check path for encrypted-3k3yredump-isos by NvrBst ///////

#ifndef NOSSL
	// Gather some path information to check if encryption makes sense, and help us find the dkey if so.
	const char *path_ps3iso_loc = strstr(path, "PS3ISO");
	path_ps3iso_loc = (path_ps3iso_loc != NULL) ? path_ps3iso_loc : strstr(path, "ps3iso");

	const char *path_ext_loc = NULL;
	if (path_ps3iso_loc)
	{
		path_ext_loc = strstr(path + flen, ".iso");
		path_ext_loc = (path_ext_loc != NULL) ? path_ext_loc : strstr(path + flen, ".ISO");
	}

	// Encryption only makes sense for .iso or .ISO files in the .../PS3ISO/ folder so exit quick if req is is not related.
	if (is_multipart || (path_ps3iso_loc == NULL) || (path_ext_loc == NULL) || (path_ext_loc < path_ps3iso_loc))
	{
		// Clean-up old region_info_ (even-though it shouldn't be needed).
		if (region_info_ != NULL)
		{
			delete [] region_info_;
			region_info_ = NULL;
		}
#endif
		/////////////////////////////////////////////////
		///// open multi-part isos by Aldo Vargas ///////
		/////////////////////////////////////////////////
		if (is_multipart)
		{
			std::unique_ptr<char []> filepath(new char[plen + 2]);

			if (!filepath)
				return FAILED;

			strcpy(filepath.get(), path);

			file_stat_t st;
			fstat_file(fd, &st);
			part_size = st.file_size; // all parts (except last) must be the same size of size of .iso.0

			// is_multipart -> count parts
			for (is_multipart = 1; is_multipart < 64; is_multipart++)
			{
				sprintf(filepath.get() + flen + 4, ".%i", is_multipart);

				fp[is_multipart] = open_file(filepath.get(), flags);

				if (!FD_OK(fp[is_multipart]))
					break;
			}

			fp[0] = fd;
		}

		return SUCCEEDED; // is multipart or non-PS3ISO
#ifndef NOSSL
	}

	/////////////////////////////////////////////////
	///// encrypted-3k3yredump-isos by NvrBst ///////
	/////////////////////////////////////////////////

	file_t key_fd;
	char *key_path = new char[path_ext_loc - path + 5 + 1];
	strncpy(key_path, path, path_ext_loc - path + 5);

	// Check for redump encrypted mode by looking for the ".dkey" is the same path of the ".iso".
	key_path[path_ext_loc - path + 1] = 'd';
	key_path[path_ext_loc - path + 2] = 'k';
	key_path[path_ext_loc - path + 3] = 'e';
	key_path[path_ext_loc - path + 4] = 'y';
	key_path[path_ext_loc - path + 5] = '\0';

	key_fd = open_file(key_path, flags);
	if (!FD_OK(key_fd))
	{
		// Check for redump encrypted mode by looking for the ".dkey" file in the "REDKEY" folder.
		key_path[path_ps3iso_loc - path + 0] = 'R';
		key_path[path_ps3iso_loc - path + 1] = 'E';
		key_path[path_ps3iso_loc - path + 2] = 'D';
		key_path[path_ps3iso_loc - path + 3] = 'K';
		key_path[path_ps3iso_loc - path + 4] = 'E';
		key_path[path_ps3iso_loc - path + 5] = 'Y';

		key_fd = open_file(key_path, flags);
	}

	delete [] key_path;

	// Check if key_path exists, and create the aes_dec_ context if so.
	if (FD_OK(key_fd))
	{
		char keystr[32];
		if (read_file(key_fd, keystr, sizeof(keystr)) == sizeof(keystr))
		{
			unsigned char key[16];
			keystr_to_keyarr(keystr, key);
#ifdef POLARSSL
			if (aes_setkey_dec(&aes_dec_, key, 128) == SUCCEEDED)
				enc_type_ = kDiscTypeRedump; // SET ENCRYPTION TYPE: Redump
#else
			if (mbedtls_aes_setkey_dec(&aes_dec_, key, 128) == SUCCEEDED)
				enc_type_ = kDiscTypeRedump; // SET ENCRYPTION TYPE: Redump
#endif
		}
		close_file(key_fd);
	}

	// Store the ps3 information sectors.
	file_stat_t st;
	unsigned char sec0sec1[kSectorSize * 2] = {0};
	if ((fstat(&st) >= 0) && (st.file_size >= kSectorSize * 2))
	{
		// Restore the original position even-though we just opened, it should be at 0... (can be safe here, no performance worries for open).
		int64_t initial_read_position = seek(0, SEEK_CUR);
		if (initial_read_position != 0)
		{
			if (seek(0, SEEK_SET) < 0LL)
				printf("ERROR: open > seek1.\n");
		}

		// Note: It's important to bypass our "read(...)" overload because encryption flag could already be set, and, then it'll try to read the region_info_.
		if (read_file(fd, sec0sec1, sizeof(sec0sec1)) == sizeof(sec0sec1))
		{
			// Populate the region information.
			if (region_info_ != NULL)
				delete [] region_info_;

			region_count_ = char_arr_BE_to_uint(sec0sec1) * 2 - 1;
			region_info_ = new PS3RegionInfo[region_count_];

			for (size_t i = 0; i < region_count_; ++i)
			{
				// Store the region information in address format.
				region_info_[i].encrypted = (i % 2 == 1);
				region_info_[i].regions_first_addr = (i == 0 ? 0LL : region_info_[i - 1].regions_last_addr + 1LL);
				region_info_[i].regions_last_addr = static_cast<int64_t>(char_arr_BE_to_uint(sec0sec1 + 12 + (i * 4)) - (i % 2 == 1 ? 1LL : 0LL)) * kSectorSize + kSectorSize - 1LL;
			}
		}

		// Restore to the initial read position (probably 0).
		if (seek(initial_read_position, SEEK_SET) < 0LL)
			printf("ERROR: open > seek2.\n");
	}

	// If encryption is still set to none (sec0sec1 will be zeroed out if it didn't succeed above).
	if (enc_type_ == kDiscTypeNone)
	{
		// The 3k3y watermarks located at 0xF70: (D|E)ncrypted 3K BLD.
		static const unsigned char k3k3yEnWatermark[16]  = {0x45, 0x6E, 0x63, 0x72, 0x79, 0x70, 0x74, 0x65, 0x64, 0x20, 0x33, 0x4B, 0x20, 0x42, 0x4C, 0x44};
		static const unsigned char k3k3yDecWatermark[16] = {0x44, 0x6E, 0x63, 0x72, 0x79, 0x70, 0x74, 0x65, 0x64, 0x20, 0x33, 0x4B, 0x20, 0x42, 0x4C, 0x44};

		if (memcmp(&k3k3yEnWatermark[0], &sec0sec1[0xF70], sizeof(k3k3yEnWatermark)) == SUCCEEDED)
		{
			// Grab the D1 from the 3k3y sector.
			unsigned char key[16];
			memcpy(key, &sec0sec1[0xF80], 0x10);

			// Convert D1 to KEY and generate the aes_dec_ context.
			unsigned char key_d1[] = {0x38, 11, 0xcf, 11, 0x53, 0x45, 0x5b, 60, 120, 0x17, 0xab, 0x4f, 0xa3, 0xba, 0x90, 0xed};
			unsigned char iv_d1[] = {0x69, 0x47, 0x47, 0x72, 0xaf, 0x6f, 0xda, 0xb3, 0x42, 0x74, 0x3a, 0xef, 170, 0x18, 0x62, 0x87};

#ifdef POLARSSL
			aes_context aes_d1;
			if (aes_setkey_enc(&aes_d1, key_d1, 128) == SUCCEEDED)
				if (aes_crypt_cbc(&aes_d1, AES_ENCRYPT, 16, &iv_d1[0], key, key) == SUCCEEDED)
					if (aes_setkey_dec(&aes_dec_, key, 128) == SUCCEEDED)
						enc_type_ = kDiscType3k3yEnc; // SET ENCRYPTION TYPE: 3K3YEnc
#else
			mbedtls_aes_context aes_d1;
			if (mbedtls_aes_setkey_enc(&aes_d1, key_d1, 128) == SUCCEEDED)
				if (mbedtls_aes_crypt_cbc(&aes_d1, MBEDTLS_AES_ENCRYPT, 16, &iv_d1[0], key, key) == SUCCEEDED)
					if (mbedtls_aes_setkey_dec(&aes_dec_, key, 128) == SUCCEEDED)
						enc_type_ = kDiscType3k3yEnc; // SET ENCRYPTION TYPE: 3K3YEnc
#endif
		}
		else if (memcmp(&k3k3yDecWatermark[0], &sec0sec1[0xF70], sizeof(k3k3yDecWatermark)) == SUCCEEDED)
		{
			enc_type_ = kDiscType3k3yDec; // SET ENCRYPTION TYPE: 3K3YDec
		}
	}

	return SUCCEEDED;
#endif
}

int File::close(void)
{
	init_region_info();

	int ret = (FD_OK(fd)) ? close_file(fd) : FAILED;

	fd = INVALID_FD;

	set_last_seek(0);

	if (!is_multipart)
		return ret;

	// close multi part isos (2015 AV)
	for (uint8_t i = 1; i < 64; i++)
	{
		if (FD_OK(fp[i]))
			close_file(fp[i]);

		fp[i] = INVALID_FD;
	}

	is_multipart = 0;
	index = 0;

	return ret;
}

ssize_t File::read(void *buf, size_t nbyte)
{
	if (!is_multipart)
	{
#ifdef NOSSL
		ssize_t ret = read_file(fd, buf, nbyte);

		add_last_seek(ret);
		return ret;
#else
		// In non-encrypted mode just do what is normally done.
		if ((enc_type_ == kDiscTypeNone) || (region_count_ == 0) || (region_info_ == NULL))
		{
			ssize_t ret = read_file(fd, buf, nbyte);

			add_last_seek(ret);
			return ret;
		}

		// read encrypted-3k3yredump-isos by NvrBst //
		int64_t read_position = seek(0, SEEK_CUR);
		ssize_t ret = read_file(fd, buf, nbyte);

		add_last_seek(ret);

		if (read_position < 0LL)
		{
			printf("ERROR: read > enc > seek: Returning decrypted data.\n");
			return ret;
		}

		// If this is a 3k3y iso, and the 0xF70 data is being requests by ps3, we should null it out.
		if ((enc_type_ == kDiscType3k3yDec) || (enc_type_ == kDiscType3k3yEnc))
		{
			if ((read_position + ret >= 0xF70LL) && (read_position <= 0x1070LL))
			{
				// Zero out the 0xF70 - 0x1070 overlap.
				unsigned char *bufb = reinterpret_cast<unsigned char *>(buf);
				unsigned char *buf_overlap_start = read_position < 0xF70LL ? bufb + 0xF70LL - read_position : bufb;
				memset(buf_overlap_start, 0x00, read_position + ret < 0x1070LL ? ret - (buf_overlap_start - bufb) : 0x100 - (buf_overlap_start - bufb));
			}

			// If it's a dec image then return, else go on to the decryption logic.
			if (enc_type_ == kDiscType3k3yDec)
				return ret;
		}

		// Encrypted mode, check if the request lies in an encrypted range.
		for (size_t i = 0; i < region_count_; ++i)
		{
			if ((read_position >= region_info_[i].regions_first_addr) && (read_position <= region_info_[i].regions_last_addr))
			{
				// We found the region, decrypt if needed.
				if (!region_info_[i].encrypted)
					return ret;

				// Decrypted the region before sending it back.
				decrypt_data(aes_dec_, reinterpret_cast<unsigned char *>(buf), ret / kSectorSize, read_position / kSectorSize);

				// TODO(Temporary): Sanity check, we are assuming that reads always start at a beginning of a sector. And that all reads will be multiples of kSectorSize.
				//				  Note: Both are easy fixes, but, code can be more simple + efficient if these two conditions are true (which they look to be from initial testing).
				if ((nbyte % kSectorSize != 0) || (ret % kSectorSize != 0) || (read_position % kSectorSize != 0))
				{
					printf("ERROR: encryption assumptions were not met, code needs to be updated, your game is probably about to crash: nbyte 0x%lx, ret 0x%lx, read_position 0x%lx.\n",
						static_cast<unsigned long int>(nbyte),
						static_cast<unsigned long int>(ret),
						static_cast<unsigned long int>(read_position)
					);
				}
				return ret;
			}
		}

		printf("ERROR: LBA request wasn't in the region_info_ for an encrypted iso? RP: 0x%lx, RC: 0x%lx, LR (0x%016llx-0x%016llx).\n",
			static_cast<unsigned long int>(read_position),
			static_cast<unsigned long int>(region_count_),
			static_cast<unsigned long long int>((region_count_ > 0) ? region_info_[region_count_ - 1].regions_first_addr : 0),
			static_cast<unsigned long long int>((region_count_ > 0) ? region_info_[region_count_ - 1].regions_last_addr  : 0)
		);
		return ret;
#endif
	}

	// read multi part iso (2015 AV)
	ssize_t ret2 = 0, ret = read_file(fp[index], buf, nbyte);

	// check if data is split between 2 parts
	if ((ret < static_cast<ssize_t>(nbyte)) && ((index + 1) < is_multipart))
	{
		index += 1;                                     // change default index to the next part
		seek_file(fp[index], 0, SEEK_SET);              // reset seek pointer to beginning of the next part
		void *buf2 = static_cast<int8_t*>(buf) + ret;   // complete filling the buffer
		ret2 = read_file(fp[index], buf2, nbyte - ret);
	}

	return (ret + ret2);
}

ssize_t File::write(void *buf, size_t nbyte)
{
	if (!is_multipart)
	{
		add_last_seek(nbyte);
		return write_file(fd, buf, nbyte);
	}

	// write multi part iso (2015 AV) - may have issues
	return write_file(fp[index], buf, nbyte);
}

int64_t File::seek(int64_t offset, int whence)
{
	if ((!is_multipart) || (!part_size))
	{
		if (is_last_seek(offset))
			return SUCCEEDED;

		set_last_seek(offset);
		return seek_file(fd, offset, whence);
	}

	// seek multi part iso (2015 AV)
	index = static_cast<int>(offset / part_size);

	return seek_file(fp[index], (offset % part_size), whence);
}

int File::fstat(file_stat_t *fs)
{
	if (!FD_OK(fd))
		return FAILED;

	if (!is_multipart)
		return fstat_file(fd, fs);

	// fstat multi part (2015 AV) - return sum of size of iso parts
	int64_t size = 0;
	file_stat_t statbuf;

	for (uint8_t i = 0; i < is_multipart; i++)
	{
		fstat_file(fp[i], &statbuf);
		size += statbuf.file_size;
	}

	int ret = fstat_file(fd, fs);
	fs->file_size = size;
	return ret;
}

#ifndef NOSSL
///// encrypted-3k3yredump-isos by NvrBst ///////

// keystr_to_keyarr (&& asciischar_to_byte): Convert a hex string into a byte array.
unsigned char File::asciischar_to_byte(char input)
{
	if ((input >= '0') && (input <= '9'))
		return input - '0';
	if ((input >= 'A') && (input <= 'F'))
		return input - 'A' + 10;
	if ((input >= 'a') && (input <= 'f'))
		return input - 'a' + 10;

	printf("ERROR: asciischar_to_byte.\n");
	return 0x00;
}

void File::keystr_to_keyarr(const char (&str)[32], unsigned char (&arr)[16])
{
	for (size_t i = 0; i < sizeof(arr); ++i)
		arr[i] = (asciischar_to_byte(str[i * 2]) * 16) + asciischar_to_byte(str[(i * 2) + 1]);
}

// Convert 4 bytes in big-endian format, to an unsigned integer.
unsigned int File::char_arr_BE_to_uint(unsigned char *arr)
{
	return arr[3] + 256 * (arr[2] + 256 * (arr[1] + (256 * arr[0])));
}

// Reset the iv to a particular lba.
void File::reset_iv(unsigned char (&iv)[16], unsigned int lba)
{
	memset(iv, 0, 12);

	iv[12] = (lba & 0xFF000000) >> 24;
	iv[13] = (lba & 0x00FF0000) >> 16;
	iv[14] = (lba & 0x0000FF00) >> 8;
	iv[15] = (lba & 0x000000FF) >> 0;
}

// Main function that will decrypt the sector(s) (needs to be a multiple of 2048).
#ifdef POLARSSL
void File::decrypt_data(aes_context &aes, unsigned char *data, int sector_count, unsigned int start_lba)
{
	for (int i = 0; i < sector_count; ++i)
	{
		reset_iv(iv_, start_lba + i);
		if (aes_crypt_cbc(&aes, AES_DECRYPT, kSectorSize, &iv_[0], &data[kSectorSize * i], &data[kSectorSize * i]) != SUCCEEDED)
		{
			printf("ERROR: decrypt_data > aes_crypt_cbc.\n");
			return;
		}
	}
}
#else
void File::decrypt_data(mbedtls_aes_context &aes, unsigned char *data, int sector_count, unsigned int start_lba)
{
	for (int i = 0; i < sector_count; ++i)
	{
		reset_iv(iv_, start_lba + i);
		if (mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, kSectorSize, &iv_[0], &data[kSectorSize * i], &data[kSectorSize * i]) != SUCCEEDED)
		{
			printf("ERROR: decrypt_data > aes_crypt_cbc.\n");
			return;
		}
	}
}
#endif
#endif
