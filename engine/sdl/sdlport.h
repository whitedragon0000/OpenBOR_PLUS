/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef SDLPORT_H
#define SDLPORT_H

#include <SDL.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#ifdef PS3
#include <sys/unistd.h>
#else
#include <unistd.h>
#endif

#include "globals.h"

#if LINUX
#define stricmp  strcasecmp
#define strnicmp strncasecmp
#endif

#if DARWIN || ANDROID
#define SKIP_CODE
#endif

#ifdef ANDROID
#define MAXTOUCHB 13
#endif

//#define MEMTEST 1

#if _POSIX_C_SOURCE >= 199309L
void _usleep(u32 usec);
#define usleep _usleep
#endif

void initSDL();

extern char rootDir[MAX_BUFFER_LEN];

char* getRootPath(char *relPath);

#ifdef ANDROID
#elif PS3
#else
extern int argFullscreen;
extern int argKeepAspectRatio;
#endif

void borExit(int reset);
void openborMain(int argc, char** argv);

extern char packfile[MAX_FILENAME_LEN];
extern char paksDir[MAX_FILENAME_LEN];
extern char savesDir[MAX_FILENAME_LEN];
extern char logsDir[MAX_FILENAME_LEN];
extern char screenShotsDir[MAX_FILENAME_LEN];

#endif
