/****************************************************************************
*
*   Copyright (c) 2010 Dave Hylands	 <dhylands@gmail.com>
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
*   @file   dirent.c
*
*   @brief  This file contains an implementation on readdir and friends
*		   which works under Win32.
*
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "dirent2.h"

#include <windows.h>

static const int FAILED		= -1;
static const int SUCCEEDED	=  0;

//***************************************************************************
/**
*   The opendir() function opens a directory stream corresponding to
*   the directory name, and returns a pointer to the directory stream.
*   The stream is positioned at the first entry in the directory.
*
*   On error, NULL is returned and errno is set appropriately.
*/

DIR2 *opendir2( const char *dirName )
{
	DIR2  *dir;
	DWORD fileAttr;

	// Validate parameters

	if ( dirName == NULL )
	{
		errno = EFAULT;
		return NULL;
	}
	if ( dirName[0] == '\0' )
	{
		errno = ENOENT;
		return NULL;
	}

	// Make sure that dirName is really a directory

	if (( fileAttr = GetFileAttributes( dirName )) == INVALID_FILE_ATTRIBUTES )
	{
		errno = ENOTDIR;
		return NULL;
	}

	if (( dir = calloc( 1, sizeof( *dir ))) == NULL )
	{
		errno = ENOMEM;
		return NULL;
	}
	if (( dir->dd_finddata = malloc( sizeof( WIN32_FIND_DATA ))) == NULL )
	{
		errno = ENOMEM;
		free( dir );
		return NULL;
	}

	// We need to pass in a pattern to FindFirstFile, so we allocate
	// memory to contain the directory passed in followed by /*

	if (( dir->dd_dirpattern = malloc( strlen( dirName ) + 3 )) == NULL )
	{
		errno = ENOMEM;
		free( dir->dd_finddata );
		free( dir );
		return NULL;
	}
	strcpy( dir->dd_dirpattern, dirName );
	strcat( dir->dd_dirpattern, "/*" );

	dir->dd_handle = 0;

	return dir;
}

//***************************************************************************
/**
*   The readdir() function returns a pointer to a dirent structure
*   representing the next directory entry in the directory stream
*   pointed to by dir. It returns NULL on reaching the end-of-file
*   or if an error occurred.
*/

struct dirent2 *readdir2( DIR2 *dir )
{
	WIN32_FIND_DATA	*fd;

	if ( dir->dd_handle == INVALID_HANDLE_VALUE )
	{
		errno = EBADF;
		return NULL;
	}

	if ( dir->dd_handle == 0 )
	{
		dir->dd_handle = FindFirstFile( dir->dd_dirpattern, dir->dd_finddata );
		if ( dir->dd_handle == INVALID_HANDLE_VALUE )
		{
			errno = SUCCEEDED;
			return NULL;
		}
	}
	else
	{
		if ( !FindNextFile( dir->dd_handle, dir->dd_finddata ))
		{
			if ( GetLastError() == ERROR_NO_MORE_FILES )
			{
				errno = SUCCEEDED;
				return NULL;
			}
		}
	}

	fd = dir->dd_finddata;

	strncpy( dir->dd_dirent.d_name, fd->cFileName, sizeof( dir->dd_dirent.d_name ));
	dir->dd_dirent.d_name[ sizeof( dir->dd_dirent.d_name ) - 1 ] = '\0';

	dir->dd_dirent.d_type = 0;

	if ( fd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
	{
		dir->dd_dirent.d_type |= DT_DIR;
	}
	else
	{
		dir->dd_dirent.d_type |= DT_REG;
	}

	return &dir->dd_dirent;
}

//***************************************************************************
/**
*   The closedir() function closes the directory stream associated with dir.
*   The directory stream descriptor dir is not available after this call.
*/

int closedir2( DIR2 *dir )
{
	int rc = SUCCEEDED;

	if (( dir->dd_handle == 0 ) || ( dir->dd_handle != INVALID_HANDLE_VALUE ))
	{
		if ( !FindClose( dir->dd_handle ))
		{
			errno = EBADF;
			rc = FAILED;
		}
		dir->dd_handle = 0;
	}

	free( dir->dd_dirpattern );
	free( dir->dd_finddata );
	free( dir );

	return rc;
}
