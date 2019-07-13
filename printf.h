/*
 * Copyright (c) 1995 Patrick Powell.
 *
 * This code is based on code written by Patrick Powell <papowell@astart.com>.
 * It may be used for any purpose as long as this notice remains intact on all
 * source code distributions.
 */

/*
 * Copyright (c) 2008 Holger Weiss.
 *
 * This version of the code is maintained by Holger Weiss <holger@jhweiss.de>.
 * My changes to the code may freely be used, modified and/or redistributed for
 * any purpose.  It would be nice if additions and fixes to this file (including
 * trivial code cleanups) would be sent back in order to let me include them in
 * the version available at <http://www.jhweiss.de/software/snprintf.html>.
 * However, this is not a requirement for using or redistributing (possibly
 * modified) versions of this file, nor is leaving this notice intact mandatory.
 */
#ifndef __VSPRINTF_H__
#define __VSPRINTF_H__

#include <stdarg.h>

//int vsprintf(char *buf, const char *fmt, va_list args);
//int vsnprintf(char *str, size_t size, const char *format, va_list args);
int sprintf(char *buffer, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
int snprintf(char *buffer, size_t len, const char *fmt, ...);
int printf(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
#endif

