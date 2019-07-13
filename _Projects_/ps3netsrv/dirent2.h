/****************************************************************************
*
*   Copyright (c) 2010 Dave Hylands     <dhylands@gmail.com>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License version 2 as
*   published by the Free Software Foundation.
*
*   Alternatively, this software may be distributed under the terms of BSD
*   license.
*
*   See README and COPYING for more details.
*
****************************************************************************/
/**
*
*   @file   dirent.h
*
*   @brief  The mingw version of dirent and friends doesn't include the 
*           the d_type field. So we make our own version.
*
****************************************************************************/

#if !defined( MINGW_DIRENT_H )
#define MINGW_DIRENT_H         /**< Include Guard                          */

/* ---- Include Files ---------------------------------------------------- */

#include <sys/types.h>
#include <io.h>

/* ---- Constants and Types ---------------------------------------------- */

#define DT_DIR      004
#define DT_REG      010

struct dirent2
{
	long            d_ino;      /* Always zero.     */
	off_t           d_off;      /* Always zero.     */
	unsigned short  d_reclen;   /* Always zero      */
	unsigned char   d_type;     /* Type of file     */
	char            d_name[2048]; /* File name. */
};

typedef struct
{
	/* dd_findsata stores the information needed for the
	 * _findfirst/_findnext API
	 */
	void               *dd_finddata;

	/* dd_dirent stores the dirent returned from readdir, which makes
	 * this threadsafe as long as only one thread uses this particular
	 * DIR entry.
	 */
	struct  dirent2      dd_dirent;

	/* dd_dirpattern contains the directory pattern that was passed to 
	 * FindFirstFile.
	 */
	char               *dd_dirpattern;

	/* dd_handle is the HANDLE returned by FindFirstFile
	 */
	void               *dd_handle;

} DIR2;

/* ---- Variable Externs ------------------------------------------------- */

/* ---- Function Prototypes ---------------------------------------------- */

DIR2 *opendir2( const char *dirName );
struct dirent2 *readdir2( DIR2 *dir );
int closedir2( DIR2 *dir );

/** @} */

#endif /* MINGW_DIRENT_H */
