#ifndef __ISO9660_H__
#define __ISO9660_H__

#include <stdint.h>

#define MAX_ISONAME     207
#define MAX_ISODIR      254

#define ISO_FILE        0   /* Not really a flag...                 */
#define ISO_EXISTENCE   1   /* Do not make existence known (hidden) */
#define ISO_DIRECTORY   2   /* This file is a directory             */
#define ISO_ASSOCIATED  4   /* This file is an assiciated file      */
#define ISO_RECORD      8   /* Record format in extended attr. != 0 */
#define ISO_PROTECTION  16  /* No read/execute perm. in ext. attr.  */
#define ISO_DRESERVED1  32  /* Reserved bit 5                       */
#define ISO_DRESERVED2  64  /* Reserved bit 6                       */
#define ISO_MULTIEXTENT 128 /* Not final entry of a mult. ext. file */

typedef struct _Iso9660PVD
{
	uint8_t   VDType;                          // (00) Must be 1 for primary volume descriptor.
	char      VSStdId[5];                      // (01) Must be "CD001".
	uint8_t   VSStdVersion;                    // (06) Must be 1.
	uint8_t   volumeFlags;                     // (07) 0 in primary volume descriptor.
	char      systemIdentifier[32];            // (08) What system this CD-ROM is meant for.
	char      volumeIdentifier[32];            // (28) The volume name.
	char      Reserved2[8];                    // (48) Must be 0's.
	uint32_t  lsbVolumeSpaceSize;              // (50) Volume size, least-significant-byte order.
	uint32_t  msbVolumeSpaceSize;              // (54) Volume size, most-significant-byte order.
	char      escapeSequences[32];             // (58) 0's in primary volume descriptor
	uint16_t  lsbVolumeSetSize;                // (78) Number of volumes in volume set (must be 1).
	uint16_t  msbVolumeSetSize;                // (7A)
	uint16_t  lsbVolumeSetSequenceNumber;      // (7C) Which volume in volume set (not used).
	uint16_t  msbVolumeSetSequenceNumber;      // (7E)
	uint16_t  lsbLogicalBlockSize;             // (80) We'll assume 2048 for block size.
	uint16_t  msbLogicalBlockSize;             // (82)
	uint32_t  lsbPathTableSize;                // (84) How many bytes in path table.
	uint32_t  msbPathTableSize;                // (88)
	uint32_t  lsbPathTable1;                   // (8C) Mandatory occurrence.
	uint32_t  lsbPathTable2;                   // (90) Optional occurrence.
	uint32_t  msbPathTable1;                   // (94) Mandatory occurrence.
	uint32_t  msbPathTable2;                   // (98) Optional occurrence.
	uint8_t   rootDirectoryRecord[34];         // (9C) Duplicate root directory entry.
	char      volumeSetIdentifier[128];        // (BE) Various copyright and control fields follow. */
	char      publisherIdentifier[128];        // (13E)
	char      dataPreparerIdentifier[128];     // (1BE)
	char      applicationIdentifier[128];      // (23E)
	char      copyrightFileIdentifier[37];     // (2BE)
	char      abstractFileIdentifier[37];      // (2E3)
	char      bibliographicFileIdentifier[37]; // (308)
	char      volumeCreation[17];              // (32D)
	char      volumeModification[17];          // (33E)
	char      volumeExpiration[17];            // (34F)
	char      volumeEffective[17];             // (360)
	uint8_t   FileStructureStandardVersion;    // (371)
	char      Reserved4;                       // (372) Must be 0.
	char      ApplicationUse[512];             // (373)
	char      FutureStandardization[653];      // (573)
} __attribute__((packed)) Iso9660PVD;

typedef struct _Iso9660DirectoryRecord
{
	uint8_t   len_dr;          // (00) Directory record length.
	uint8_t   XARlength;       // (01) Extended attribute record length.
	uint32_t  lsbStart;        // (02) First logical block where file starts.
	uint32_t  msbStart;        // (06)
	uint32_t  lsbDataLength;   // (0A) Number of bytes in file.
	uint32_t  msbDataLength;   // (0E)
	uint8_t   year;            // (12) Since 1900.
	uint8_t   month;           // (13)
	uint8_t   day;             // (14)
	uint8_t   hour;            // (15)
	uint8_t   minute;          // (16)
	uint8_t   second;          // (17)
	uint8_t   gmtOffset;       // (18) 15-minute offset from Universal Time.
	uint8_t   fileFlags;       // (19) Attributes of a file or directory.
	uint8_t   interleaveSize;  // (1A) Used for interleaved files.
	uint8_t   interleaveSkip;  // (1B) Used for interleaved files.
	uint16_t  lsbVolSetSeqNum; // (1C) Which volume in volume set contains this file.
	uint16_t  msbVolSetSeqNum; // (1E)
	uint8_t   len_fi;          // (20) Length of file identifier that follows.
	char      fi;              // (21) File identifier (variable length)
} __attribute__((packed)) Iso9660DirectoryRecord;

typedef struct
{
	uint8_t   len_di;          // (00) Length of directory identifier.
	uint8_t   XARlength;       // (01) Extended attribute record length.
	uint32_t  dirLocation;     // (02) First logical block where directory is stored.
	uint16_t  parentDN;        // (06) Parent directory number.
	char      dirID;           // (08) Directory identifier: actual length is len_di; there is an extra blank byte if len_di is odd.
} __attribute__((packed)) Iso9660PathTable;

#endif /* __ISO9660_H__ */
