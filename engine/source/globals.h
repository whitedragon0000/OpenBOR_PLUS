/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

/////////////////////////////////////////////////////////////////////////////

#ifndef GLOBALS_H
#define GLOBALS_H

/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <ctype.h>

#include "omath.h"
#include "utils.h"
#include "safealloc.h"

#define		MAX_BUFFER_LEN		512
#define		MAX_FILENAME_LEN	256
#define		MAX_LABEL_LEN		128

#define		MAX_PAGE_MODS_LENGTH            18
#define		MAX_PAGE_MODS_FAST_FORWARD      10
#define     MAX_MODS_NUM                    100

#ifdef SDL
#include "sdlport.h"
#endif

#ifdef WII
#include <gctypes.h>
#include <ogc/conf.h>
#include "wiiport.h"
// For devkitPPC r29+
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

/*
#ifndef stricmp
    #define stricmp safe_stricmp
#endif
#ifndef strnicmp
    #define strnicmp safe_strnicmp
#endif
*/


#ifdef PS3
#include <ppu-types.h>
#include "sdlport.h"
#ifndef stricmp
    #define stricmp safe_stricmp
#endif
#ifndef strnicmp
    #define strnicmp safe_strnicmp
#endif
#ifndef strncmp
    #define strncmp safe_strncmp
#endif
#endif

#include "packfile.h"

/////////////////////////////////////////////////////////////////////////////

#ifndef PP_TEST
#define printf writeToLogFile

// redefine assert to write to the log file and exit nicely instead of aborting
#undef assert
#define assert(x)    exitIfFalse((x)?1:0, #x, __func__, __FILE__, __LINE__)
#define sysassert(x) abortIfFalse((x)?1:0, #x, __func__, __FILE__, __LINE__)
#endif

/////////////////////////////////////////////////////////////////////////////

extern int int_assert[sizeof(int) == 4 ? 1 : -1];

#define MIN_INT (int)0x80000000
#define MAX_INT	(int)0x7fffffff

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define __realloc(p, n) \
p = realloc((p), sizeof(*(p))*((n)+1));\
memset((p)+(n), 0, sizeof(*(p)));

#define __reallocto(p, n, s) \
p = realloc((p), sizeof(*(p))*(s));\
memset((p)+(n), 0, sizeof(*(p))*((s)-(n)));

#endif
