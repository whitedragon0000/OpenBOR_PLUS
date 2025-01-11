/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include "sdlport.h"
#include "packfile.h"
#include "ram.h"
#include "video.h"
#include "menu.h"
#ifdef PS3
#include <lv2/sysfs.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/process.h>
#else
#include <time.h>
#endif
#ifdef ANDROID
#include "jniutils.h"
#endif
#include <unistd.h>

#undef usleep

#ifdef DARWIN
#include <CoreFoundation/CoreFoundation.h>
#elif WIN
#undef main
#endif

#ifdef PS3
#define SYS_PROCESS_PARAM_OPENBOR(prio,stacksize) \
	sys_process_param_t __sys_process_param __attribute__((aligned(8), section(".sys_proc_param"), unused)) = { \
		sizeof(sys_process_param_t), \
		SYS_PROCESS_SPAWN_MAGIC, \
		SYS_PROCESS_SPAWN_VERSION_330, \
		SYS_PROCESS_SPAWN_FW_VERSION_330, \
		prio, \
		stacksize, \
		SYS_PROCESS_SPAWN_MALLOC_PAGE_SIZE_1M, \
		SYS_PROCESS_SPAWN_PPC_SEG_DEFAULT\
	};

SYS_PROCESS_PARAM_OPENBOR(1001, 0x00100000)
#endif

char packfile[MAX_FILENAME_LEN] = {"bor.pak"};
//#if ANDROID || PS3
char rootDir[MAX_BUFFER_LEN] = {""};
//#endif
char paksDir[MAX_FILENAME_LEN] = {"Paks"};
char savesDir[MAX_FILENAME_LEN] = {"Saves"};
char logsDir[MAX_FILENAME_LEN] = {"Logs"};
char screenShotsDir[MAX_FILENAME_LEN] = {"ScreenShots"};

// sleeps for the given number of microseconds
#if _POSIX_C_SOURCE >= 199309L
void _usleep(u32 usec)
{
    struct timespec sleeptime;
    sleeptime.tv_sec = usec / 1000000LL;
    sleeptime.tv_nsec = (usec % 1000000LL) * 1000;
    nanosleep(&sleeptime, NULL);
}
#endif

char* getRootPath(char *relPath)
{
	static char filename[MAX_FILENAME_LEN];
	strcpy(filename, rootDir);
	strcat(filename, relPath);
	return filename;
}

#if ANDROID
#elif PS3
#elif WII
#elif VITA
#elif PSP
#else
int argFullscreen = 0;
int argKeepAspectRatio = 0;
#endif

void borExit(int reset)
{
	//SDL_Delay(1000);
	SDL_Quit(); // Call this instead of atexit(SDL_Quit); It's best practice!
#ifdef DMALLOC_MODE
PRINT_DMALLOC_INFO;
#endif

    exit(reset);
}

int main(int argc, char *argv[])
{
#ifndef SKIP_CODE
	char pakname[MAX_FILENAME_LEN] = {""};
#endif
#ifdef CUSTOM_SIGNAL_HANDLER
	struct sigaction sigact;
#endif
#ifdef PS3
    int retry = 0, dir_exists_flag = 1;
#endif

#ifdef DARWIN
	char resourcePath[PATH_MAX] = {""};
	CFBundleRef mainBundle;
	CFURLRef resourcesDirectoryURL;
	mainBundle = CFBundleGetMainBundle();
	resourcesDirectoryURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
	if(!CFURLGetFileSystemRepresentation(resourcesDirectoryURL, true, (UInt8 *) resourcePath, PATH_MAX))
	{
		borExit(0);
	}
	CFRelease(resourcesDirectoryURL);
	chdir(resourcePath);
#endif

#ifdef CUSTOM_SIGNAL_HANDLER
	sigact.sa_sigaction = handleFatalSignal;
	sigact.sa_flags = SA_RESTART | SA_SIGINFO;

	if(sigaction(SIGSEGV, &sigact, NULL) != 0)
	{
		printf("Error setting signal handler for %d (%s)\n", SIGSEGV, strsignal(SIGSEGV));
		borExit(EXIT_FAILURE);
	}
#endif

	setSystemRam();
	initSDL();

	packfile_mode(0);

#ifdef ANDROID
    //const char *path = SDL_AndroidGetExternalStoragePath();
    char *path = (char*) malloc(MAX_FILENAME_LEN);
    jniutils_get_storage_path(path);

    //if(strstr(SDL_AndroidGetExternalStoragePath(), "org.openbor.engine"))
    if(!strcmp(SDL_AndroidGetExternalStoragePath(), "org.openbor.engine"))
    {
        strcpy(rootDir, "/mnt/sdcard/OpenBOR/");
        strcpy(paksDir, "/mnt/sdcard/OpenBOR/Paks");
        strcpy(savesDir, "/mnt/sdcard/OpenBOR/Saves");
        strcpy(logsDir, "/mnt/sdcard/OpenBOR/Logs");
        strcpy(screenShotsDir, "/mnt/sdcard/OpenBOR/ScreenShots");
    }
    else
    {
        strcpy(rootDir, path);
        strcat(rootDir, "/OpenBOR/");
        strcpy(paksDir, path);
        strcat(paksDir, "/OpenBOR/Paks");
        strcpy(savesDir, path);
        strcat(savesDir, "/OpenBOR/Saves");
        strcpy(logsDir, path);
        strcat(logsDir, "/OpenBOR/Logs");
        strcpy(screenShotsDir, path);
        strcat(screenShotsDir, "/OpenBOR/ScreenShots");
    }

    free((char*) path);
	dirExists(rootDir, 1);
    chdir(rootDir);
#elif PS3
    strcpy(rootDir, "/dev_hdd0/OpenBOR");
    strcpy(paksDir, "/dev_hdd0/OpenBOR/Paks");
    strcpy(savesDir, "/dev_hdd0/OpenBOR/Saves");
    strcpy(logsDir, "/dev_hdd0/OpenBOR/Logs");
    strcpy(screenShotsDir, "/dev_hdd0/OpenBOR/ScreenShots");

	dirExists(rootDir, 1);
#else
    char pathname[MAX_FILENAME_LEN] = {""};
    for (int i = strlen(argv[0]); i >= 0; i--) {
        if (argv[0][i] == '\\' || argv[0][i] == '/') {
            strncpy(pathname, argv[0], i);
            break;
        }
    }
    if (strcmp(getcwd(NULL, 0), pathname) != 0 && strlen(pathname) > 0)
    {
        strcpy(rootDir, pathname);
        strcat(rootDir, PATH_SLASH);
        strcpy(paksDir, pathname);
        strcat(paksDir, PATH_SLASH);
        strcat(paksDir, "Paks");
        strcpy(savesDir, pathname);
        strcat(savesDir, PATH_SLASH);
        strcat(savesDir, "Saves");
        strcpy(logsDir, pathname);
        strcat(logsDir, PATH_SLASH);
        strcat(logsDir, "Logs");
        strcpy(screenShotsDir, pathname);
        strcat(screenShotsDir, PATH_SLASH);
        strcat(screenShotsDir, "ScreenShots");
        strcpy(packfile, pathname);
        strcat(packfile, PATH_SLASH);
        strcat(packfile, "bor.pak");
    }
#endif

	dirExists(paksDir, 1);
	dirExists(savesDir, 1);
	dirExists(logsDir, 1);
	dirExists(screenShotsDir, 1);

#ifdef PS3
	dir_exists_flag &= dirExists(paksDir, 1);
	dir_exists_flag &= dirExists(savesDir, 1);
	dir_exists_flag &= dirExists(logsDir, 1);
	dir_exists_flag &= dirExists(screenShotsDir, 1);

	do
	{
	    usleep(250000);
	    retry++;
	    dir_exists_flag = 1;
	}
	while (!dir_exists_flag && retry <= 12);
#endif

#ifdef ANDROID
    Menu();
#elif PS3
    Menu();
#else
    if (argc > 1)
    {
        char game_absolute_filename[MAX_FILENAME_LEN] = {""};
        char game_filename[MAX_FILENAME_LEN] = {""};
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-fullscreen") != 0 &&
                strcmp(argv[i], "-keepaspectratio") != 0)
            {
                strcpy(game_absolute_filename, argv[i]);
            }
            else
            {
                if (strcmp(argv[i], "-fullscreen") == 0)
                {
                    argFullscreen = 1;
                }
                else if (strcmp(argv[i], "-keepaspectratio") == 0)
                {
                    argKeepAspectRatio = 1;
                }
            }
        }

        if (strlen(game_absolute_filename) > 0)
        {
            for (int i = strlen(argv[0]); i >= 0; i--) {
                if (game_absolute_filename[i] == '\\' || game_absolute_filename[i] == '/') {
                    strncpy(game_filename, game_absolute_filename + i + 1, strlen(game_absolute_filename) - 1);
                    break;
                }
            }
            if (strlen(game_filename) <= 0)
            {
               strcpy(game_filename, game_absolute_filename);
            }

            //getBasePath(packfile, game_filename, 1);
			strcpy(packfile, game_absolute_filename);
            // Restore pixelformat default value.
            pixelformat = PIXEL_x8;
        }
    }
    else
    {
        Menu();
    }
#endif

#ifndef SKIP_CODE
	getPakName(pakname, -1);
    video_set_window_title(pakname);
#endif
	openborMain(argc, argv);
	borExit(0);
	return 0;
}

