/*
 * OpenBOR - https://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */
// Adapted from sdl/menu.c.  Uses s_screen images instead of SDL surfaces.

#include <dirent.h>
#include <unistd.h>
#include <ogcsys.h>
#include <wiiuse/wpad.h>
#include <wupc/wupc.h>
#include "wiiport.h"
#include "video.h"
#include "control.h"
#include "packfile.h"
#include "stristr.h"
#include "image.h"

#include "pngdec.h"
#include "../resources/OpenBOR_Logo_480x272_png.h"
#include "../resources/OpenBOR_Logo_320x240_png.h"
#include "../resources/OpenBOR_Menu_480x272_png.h"
#include "../resources/OpenBOR_Menu_320x240_png.h"

#include "openbor.h"


extern int videoMode;

#define RGB32(R,G,B) ((R << 16) | ((G) << 8) | (B))
#define RGB16(R,G,B) ((B&0xF8)<<8) | ((G&0xFC)<<3) | (R>>3)
#define RGB(R,G,B)   (bpp==16?RGB16(R,G,B):RGB32(R,G,B))

#define BLACK		RGB(  0,   0,   0)
#define WHITE		RGB(255, 255, 255)
#define RED			RGB(255,   0,   0)
#define	GREEN		RGB(  0, 255,   0)
#define BLUE		RGB(  0,   0, 255)
#define YELLOW		RGB(255, 255,   0)
#define PURPLE		RGB(255,   0, 255)
#define ORANGE		RGB(255, 128,   0)
#define GRAY		RGB(112, 128, 144)
#define LIGHT_GRAY  RGB(223, 223, 223)
#define DARK_RED	RGB(128,   0,   0)
#define DARK_GREEN	RGB(  0, 128,   0)
#define DARK_BLUE	RGB(  0,   0, 128)

#define LOG_SCREEN_TOP 2
#define LOG_SCREEN_END (isWide ? 26 : 23)

#define FIRST_KEYPRESS      1
#define IMPULSE_TIME        0.12
#define FIRST_IMPULSE_TIME  1.2

#define DIR_UP			0x00000001
#define DIR_RIGHT		0x00000002
#define DIR_DOWN		0x00000004
#define DIR_LEFT		0x00000008
#define WIIMOTE_A		0x00000010
#define WIIMOTE_B		0x00000020
#define WIIMOTE_1		0x00000040
#define WIIMOTE_2		0x00000080
#define WIIMOTE_PLUS	0x00000100
#define WIIMOTE_MINUS	0x00000200
#define WIIMOTE_HOME	0x00000400
#define NUNCHUK_C		0x00000800
#define NUNCHUK_Z		0x00001000
#define CC_A			0x00002000
#define CC_B			0x00004000
#define CC_X			0x00008000
#define CC_Y			0x00010000
#define CC_L			0x00020000
#define CC_R			0x00040000
#define CC_ZL			0x00080000
#define CC_ZR			0x00100000
#define CC_PLUS			0x00200000
#define CC_MINUS		0x00400000
#define CC_HOME			0x00800000
#define GC_A			0x01000000
#define GC_B			0x02000000
#define GC_X			0x04000000
#define GC_Y			0x08000000
#define GC_L			0x10000000
#define GC_R			0x20000000
#define GC_Z			0x40000000
#define GC_START		0x80000000

#define PREVIEW_SCREENSHOTS 1

s_screen *Source = NULL;
s_screen *Scaler = NULL;
s_screen *Screen = NULL;
int bpp = 32;
int factor = 1;
int isFull = 0;
int isWide = 0;
int flags;
int dListTotal;
int dListCurrentPosition;
int dListScrollPosition;
int which_logfile = OPENBOR_LOG;
int buttonsHeld = 0;
int buttonsPressed = 0;
FILE *bgmFile = NULL;
fileliststruct *filelist;
static s_videomodes videomodes;
extern u64 bothkeys, bothnewkeys;

typedef struct{
	stringptr *buf;
	int *pos;
	int line;
	int rows;
	char ready;
}s_logfile;
s_logfile logfile[2];

typedef struct{
	int x;
	int y;
	int width;
	int height;
}Rect;

typedef int (*ControlInput)();

static int ControlMenu();
void PlayBGM();
void StopBGM();
void fillRect(s_screen* dest, Rect* rect, u32 color);
static ControlInput pControl;

static int Control()
{
	return pControl();
}

void refreshInput()
{
	unsigned long btns = 0;
	unsigned short gcbtns;
	WPADData *wpad;
	struct WUPCData *wupc;

	PAD_Init();
	WUPC_Init();
	PAD_ScanPads();
	gcbtns = PAD_ButtonsDown(0) | PAD_ButtonsHeld(0);
	WUPC_UpdateButtonStats();
	WPAD_ScanPads();
	wpad = WPAD_Data(0);
	wupc = WUPC_Data(0);

	if(wpad->exp.type == WPAD_EXP_CLASSIC)
	{
		// Left thumb stick
		if(wpad->exp.classic.ljs.mag >= 0.3)
		{
			if (wpad->exp.classic.ljs.ang >= 310 || wpad->exp.classic.ljs.ang <= 50)   btns |= DIR_UP;
			if (wpad->exp.classic.ljs.ang >= 130 && wpad->exp.classic.ljs.ang <= 230)  btns |= DIR_DOWN;
			if (wpad->exp.classic.ljs.ang >= 220 && wpad->exp.classic.ljs.ang <= 320)  btns |= DIR_LEFT;
			if (wpad->exp.classic.ljs.ang >= 40 && wpad->exp.classic.ljs.ang <= 140)   btns |= DIR_RIGHT;
		}
		// D-pad
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_UP)         btns |= DIR_UP;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_DOWN)       btns |= DIR_DOWN;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_LEFT)       btns |= DIR_LEFT;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_RIGHT)      btns |= DIR_RIGHT;
	}
	else if (wupc != NULL) // Pro Controller
	{
		if(wupc->button & WPAD_CLASSIC_BUTTON_UP)		  btns |= DIR_UP;
		if(wupc->button & WPAD_CLASSIC_BUTTON_DOWN) 	  btns |= DIR_DOWN;
		if(wupc->button & WPAD_CLASSIC_BUTTON_LEFT) 	  btns |= DIR_LEFT;
		if(wupc->button & WPAD_CLASSIC_BUTTON_RIGHT) 	  btns |= DIR_RIGHT;
		if(wupc->button & WPAD_CLASSIC_BUTTON_PLUS) 	  btns |= CC_PLUS;
		if(wupc->button & WPAD_CLASSIC_BUTTON_MINUS)	  btns |= CC_MINUS;
		if(wupc->button & WPAD_CLASSIC_BUTTON_HOME)       btns |= CC_HOME;
		if(wupc->button & WPAD_CLASSIC_BUTTON_A) 		  btns |= CC_A;
		if(wupc->button & WPAD_CLASSIC_BUTTON_B)		  btns |= CC_B;
		if(wupc->button & WPAD_CLASSIC_BUTTON_Y)          btns |= CC_Y;
		if(wupc->button & WPAD_CLASSIC_BUTTON_X)          btns |= CC_X;
		if(wupc->button & WPAD_CLASSIC_BUTTON_FULL_R)     btns |= CC_R;
		if(wupc->button & WPAD_CLASSIC_BUTTON_FULL_L)     btns |= CC_L;
		if(wupc->button & WPAD_CLASSIC_BUTTON_ZL)         btns |= CC_ZL;
		if(wupc->button & WPAD_CLASSIC_BUTTON_ZR)         btns |= CC_ZR;

		//analog stick
		if(wupc->yAxisL > 200)							  btns |= DIR_UP;
		if(wupc->yAxisL < -200)							  btns |= DIR_DOWN;
		if(wupc->xAxisL > 200)							  btns |= DIR_RIGHT;
		if(wupc->xAxisL < -200)							  btns |= DIR_LEFT;

	}
	else if(wpad->exp.type == WPAD_EXP_NUNCHUK) // Wiimote + Nunchuk
	{
		if(wpad->exp.nunchuk.js.pos.y >= 0xB0)            btns |= DIR_UP;
		if(wpad->exp.nunchuk.js.pos.y <= 0x40)            btns |= DIR_DOWN;
		if(wpad->exp.nunchuk.js.pos.x <= 0x40)            btns |= DIR_LEFT;
		if(wpad->exp.nunchuk.js.pos.x >= 0xB0)            btns |= DIR_RIGHT;
		if(wpad->btns_h & WPAD_BUTTON_UP)                 btns |= DIR_UP;
		if(wpad->btns_h & WPAD_BUTTON_DOWN)               btns |= DIR_DOWN;
		if(wpad->btns_h & WPAD_BUTTON_LEFT)               btns |= DIR_LEFT;
		if(wpad->btns_h & WPAD_BUTTON_RIGHT)              btns |= DIR_RIGHT;
	}
	else // Wiimote held sideways
	{
		if(wpad->btns_h & WPAD_BUTTON_UP)                 btns |= DIR_LEFT;
		if(wpad->btns_h & WPAD_BUTTON_DOWN)               btns |= DIR_RIGHT;
		if(wpad->btns_h & WPAD_BUTTON_LEFT)               btns |= DIR_DOWN;
		if(wpad->btns_h & WPAD_BUTTON_RIGHT)              btns |= DIR_UP;
	}

	// GameCube analog stick and D-pad
	if(PAD_StickY(0) > 18)                                btns |= DIR_UP;
	if(PAD_StickY(0) < -18)                               btns |= DIR_DOWN;
	if(PAD_StickX(0) < -18)                               btns |= DIR_LEFT;
	if(PAD_StickX(0) > 18)                                btns |= DIR_RIGHT;
	if(gcbtns & PAD_BUTTON_UP)                            btns |= DIR_UP;
	if(gcbtns & PAD_BUTTON_DOWN)                          btns |= DIR_DOWN;
	if(gcbtns & PAD_BUTTON_LEFT)                          btns |= DIR_LEFT;
	if(gcbtns & PAD_BUTTON_RIGHT)                         btns |= DIR_RIGHT;

	// Controller buttons
	if(wpad->exp.type <= WPAD_EXP_NUNCHUK)
	{
		if(wpad->btns_h & WPAD_BUTTON_1)                      btns |= WIIMOTE_1;
		if(wpad->btns_h & WPAD_BUTTON_2)                      btns |= WIIMOTE_2;
		if(wpad->btns_h & WPAD_BUTTON_A)                      btns |= WIIMOTE_A;
		if(wpad->btns_h & WPAD_BUTTON_B)                      btns |= WIIMOTE_B;
		if(wpad->btns_h & WPAD_BUTTON_MINUS)                  btns |= WIIMOTE_MINUS;
		if(wpad->btns_h & WPAD_BUTTON_PLUS)                   btns |= WIIMOTE_PLUS;
		if(wpad->btns_h & WPAD_BUTTON_HOME)                   btns |= WIIMOTE_HOME;
		if(wpad->btns_h & WPAD_NUNCHUK_BUTTON_Z)              btns |= NUNCHUK_Z;
		if(wpad->btns_h & WPAD_NUNCHUK_BUTTON_C)              btns |= NUNCHUK_C;
	}
	else if(wpad->exp.type == WPAD_EXP_CLASSIC)
	{
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_A)              btns |= CC_A;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_B)              btns |= CC_B;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_Y)              btns |= CC_Y;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_X)              btns |= CC_X;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_MINUS)          btns |= CC_MINUS;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_PLUS)           btns |= CC_PLUS;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_HOME)           btns |= CC_HOME;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_FULL_R)         btns |= CC_R;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_FULL_L)         btns |= CC_L;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_ZL)             btns |= CC_ZL;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_ZR)             btns |= CC_ZR;
	}
	else
	{
		if(gcbtns & PAD_BUTTON_X)                             btns |= GC_X;
		if(gcbtns & PAD_BUTTON_Y)                             btns |= GC_Y;
		if(gcbtns & PAD_BUTTON_A)                             btns |= GC_A;
		if(gcbtns & PAD_BUTTON_B)                             btns |= GC_B;
		if(gcbtns & PAD_TRIGGER_R)                            btns |= GC_R;
		if(gcbtns & PAD_TRIGGER_L)                            btns |= GC_L;
		if(gcbtns & PAD_TRIGGER_Z)                            btns |= GC_Z;
		if(gcbtns & PAD_BUTTON_START)                         btns |= GC_START;
	}

	// update buttons pressed (not held)
	buttonsPressed = btns & ~buttonsHeld;
	buttonsHeld = btns;
}

void getAllLogs()
{
	int i, j, k;
	for(i=0; i<2; i++)
	{
		logfile[i].buf = readFromLogFile(i);
		if(logfile[i].buf != NULL)
		{
			logfile[i].pos = malloc(++logfile[i].rows * sizeof(int));
			if(logfile[i].pos == NULL) return;
			memset(logfile[i].pos, 0, logfile[i].rows * sizeof(int));

			for(k=0, j=0; j<logfile[i].buf->size; j++)
			{
				if(!k)
				{
					logfile[i].pos[logfile[i].rows - 1] = j;
					k = 1;
				}
				if(logfile[i].buf->ptr[j]=='\n')
				{
					int *_pos = malloc(++logfile[i].rows * sizeof(int));
					if(_pos == NULL) return;
					memcpy(_pos, logfile[i].pos, (logfile[i].rows - 1) * sizeof(int));
					_pos[logfile[i].rows - 1] = 0;
					free(logfile[i].pos);
					logfile[i].pos = NULL;
					logfile[i].pos = malloc(logfile[i].rows * sizeof(int));
					if(logfile[i].pos == NULL) return;
					memcpy(logfile[i].pos, _pos, logfile[i].rows * sizeof(int));
					free(_pos);
					_pos = NULL;
					logfile[i].buf->ptr[j] = 0;
					k = 0;
				}
				if(logfile[i].buf->ptr[j]=='\r') logfile[i].buf->ptr[j] = 0;
				if(logfile[i].rows>0xFFFFFFFE) break;
			}
			logfile[i].ready = 1;
		}
	}
}

void freeAllLogs()
{
	int i;
	for(i=0; i<2; i++)
	{
		if(logfile[i].ready)
		{
			free(logfile[i].buf);
			logfile[i].buf = NULL;
			free(logfile[i].pos);
			logfile[i].pos = NULL;
		}
	}
}

void sortList()
{
	int i, j;
	fileliststruct temp;
	if(dListTotal<2) return;
	for(j=dListTotal-1; j>0; j--)
	{
		for(i=0; i<j; i++)
		{
			if(stricmp(filelist[i].filename, filelist[i+1].filename)>0)
			{
				temp = filelist[i];
				filelist[i] = filelist[i+1];
				filelist[i+1] = temp;
			}
		}
	}
}

static int findPaks(void)
{
	int i = 0;
	DIR* dp = NULL;
	struct dirent* ds;
	dp = opendir(paksDir);

	if(dp != NULL)
   	{
   	    filelist = NULL;
		while((ds = readdir(dp)) != NULL)
		{
			if(packfile_supported(ds->d_name))
			{
				if(filelist == NULL) filelist = malloc(sizeof(struct fileliststruct));
				else
				{
				    filelist = (fileliststruct *)realloc(filelist, (i+1) * sizeof(struct fileliststruct));
				    /*fileliststruct *copy = NULL;
					copy = malloc((i + 1) * sizeof(struct fileliststruct));
					memcpy(copy, filelist, (i + 1) * sizeof(struct fileliststruct));
					free(filelist);
					filelist = malloc((i + 1) * sizeof(struct fileliststruct));
					memcpy(filelist, copy, (i + 1) * sizeof(struct fileliststruct));
					free(copy); copy = NULL;*/
				}
				memset(&filelist[i], 0, sizeof(struct fileliststruct));
				strcpy(filelist[i].filename, ds->d_name);
				i++;
			}
		}
		closedir(dp);
   	}
	return i;
}

void copyScreens(s_screen *Image)
{
	// Copy Logo or Menu from Source to Scaler to give us a background
	// prior to printing to this s_screen.
	copyscreen_o(Scaler, Image, 0, 0);
}

void writeToScreen(s_screen* src)
{
	copyscreen(Screen, src);
}

void drawScreens(s_screen *Image, int x, int y)
{
	if (!PREVIEW_SCREENSHOTS)
    {
        if(Image) copyscreen_o(Scaler, Image, x, y);
    }
    else
    {
        if(Image)
        {
            putscreen(Scaler, Image, x, y, NULL);
            freescreen(&Image);
            Image = NULL;
        }
    }

	writeToScreen(Scaler);
	video_copy_screen(Screen);
}

/*static Image *getPreview(char *filename)
{
	int width = factor == 4 ? 640 : (factor == 2 ? 320 : 160);
	int height = factor == 4 ? 480 : (factor == 2 ? 240 : 120);
	s_screen *title = NULL;
	s_screen *scale = NULL;
	Image *preview = NULL;
	unsigned char *sp;
	int x, y, i, pos = 0;
	unsigned int palette[256];
	unsigned char *pal_ptr;
	Color *dp;

	return NULL;

	// Grab current path and filename
	//getBasePath(packfile, filename, 1);
	strncpy(packfile, paksDir, MAX_FILENAME_LEN);
	strcat(packfile, "/");
	strcat(packfile, filename);

	// Create & Load & Scale Image
	if(!loadscreen("data/bgs/title", packfile, NULL, PIXEL_x8, &title)) return NULL;
	if((scale = allocscreen(width, height, title->pixelformat)) == NULL) return NULL;
	if((preview = createImage(width, height)) == NULL) return NULL;
	scalescreen(scale, title);
	//memcpy(scale->palette, title->palette, PAL_BYTES);

	pal_ptr = title->palette + 1;
	pos = 0;
   	for (i = 0; i < 256; i++)
    {
        palette[i] = RGB(pal_ptr[pos], pal_ptr[pos+1], pal_ptr[pos+2]);
        pos += 4;
    }

    for(i=0; i<256; i++) printf("0x%x ", palette[i]);

	sp = scale->data;
	dp = (void*)preview->data;
	for(y=0; y<height; y++)
	{
	    for(x=0; x<width; x++)
	    {
            dp[x] = palette[((int)(sp[x])) & 0xFF];
	    }
   		sp += scale->width;
   		dp += preview->imageWidth;
   	}

	// ScreenShots within Menu will be saved as "Menu"
	strncpy(packfile,"Menu.ext",MAX_FILENAME_LEN);

	// Free Images and Terminate FileCaching
	freescreen(&title);
	freescreen(&scale);
	return preview;
}*/

static s_screen *getPreviewScreen(char *filename)
{
	int width = factor == 4 ? 640 : (factor == 2 ? 320 : 160);
	int height = factor == 4 ? 480 : (factor == 2 ? 240 : 120);
	s_screen *title = NULL;
	s_screen *scale = NULL;
    FILE *preview = NULL;
	char ssPath[MAX_FILENAME_LEN] = "";

	// Grab current path and filename
	getBasePath(ssPath,"ScreenShots/",0); //get screenshots directory from base path
	strncat(ssPath, filename, strrchr(filename, '.') - filename); //remove extension from pak filename
	strcat(ssPath, " - 0000.png"); //add to end of pak filename
	preview = fopen(ssPath, "r"); //open preview image

	if(preview) //if preview image found
	{
		fclose(preview); //close preview image
		strcpy(packfile,"null.file"); //dummy pak file since we are loading outside a pak file

		//Create & Load & Scale Image
		if(!loadscreen32(ssPath, packfile, &title)) return NULL; //exit if image screen not loaded
		if((scale = allocscreen(width, height, title->pixelformat)) == NULL) return NULL; //exit if scaled screen not
		scalescreen32(scale, title); //copy image to scaled down screen

	} else { return NULL; }

	// ScreenShots within Menu will be saved as "Menu"
	strncpy(packfile,"Menu.ext",MAX_FILENAME_LEN);

	// Free Images and Terminate FileCaching
    if(title) freescreen(&title); //free image screen
    return scale; // return scaled down screen
}

/*static void getAllPreviews()
{
	int i;
	for(i=0; i<dListTotal; i++)
	{
		filelist[i].preview = getPreview(filelist[i].filename);
	}
}

static void freeAllPreviews()
{
	int i;
	for(i=0; i<dListTotal; i++)
	{
		if(filelist[i].preview != NULL) freeImage(filelist[i].preview);
	}
}

static void freeAllImages()
{
    freeAllLogs();
	if (!PREVIEW_SCREENSHOTS) freeAllPreviews();
}*/

/* PARAMS:
 * key: pressed key flag
 * time_range: time between 2 key impulses
 * start_press_flag: 1 == press the first time too, 0 == no first time key press
 * start_time_eta: wait time after the first key press (time between 1st and 2nd impulse)
 */
static int hold_key_impulse(int key, float time_range, int start_press_flag, float start_time_eta) {
    static int hold_time[64];
    static int first_keypress[64];
    static int second_keypress[64];
    int key_index = 0, tmp_key = key;

    while (tmp_key >>= 1) key_index++;

    if ( buttonsHeld & key ) {
        unsigned time = timer_gettick();

        time_range *= GAME_SPEED;
        start_time_eta *= GAME_SPEED;
        if ( !hold_time[key_index] ) {
            hold_time[key_index] = time;

            if ( start_press_flag > 0 && !first_keypress[key_index] ) {
                first_keypress[key_index] = 1;
                return key;
            }
        } else if ( time - hold_time[key_index] >= time_range ) {
            if ( start_time_eta > 0 && !second_keypress[key_index] ) {
                if ( time - hold_time[key_index] < start_time_eta ) return 0;
            }

            // simulate hold press
            if ( !second_keypress[key_index] ) second_keypress[key_index] = 1;
            hold_time[key_index] = 0;
            return key;
        }
    } else {
        hold_time[key_index] = 0;
        first_keypress[key_index] = 0;
        second_keypress[key_index] = 0;
    }

    return 0;
}

static int ControlMenu()
{
	int status = -1;
	int dListMaxDisplay = MAX_PAGE_MODS_LENGTH - 1;

	//bothnewkeys = 0;
	//inputrefresh(0);
	refreshInput();

	buttonsPressed |= hold_key_impulse(DIR_DOWN, IMPULSE_TIME, FIRST_KEYPRESS, FIRST_IMPULSE_TIME);
	buttonsPressed |= hold_key_impulse(DIR_LEFT, IMPULSE_TIME, FIRST_KEYPRESS, FIRST_IMPULSE_TIME);
	buttonsPressed |= hold_key_impulse(DIR_UP, IMPULSE_TIME, FIRST_KEYPRESS, FIRST_IMPULSE_TIME);
	buttonsPressed |= hold_key_impulse(DIR_RIGHT, IMPULSE_TIME, FIRST_KEYPRESS, FIRST_IMPULSE_TIME);

	switch(buttonsPressed)
	{
		case DIR_UP:
			dListScrollPosition--;
			if(dListScrollPosition < 0)
			{
				dListScrollPosition = 0;
				dListCurrentPosition--;
			}
			if(dListCurrentPosition < 0) dListCurrentPosition = 0;
			break;

		case DIR_DOWN:
			dListCurrentPosition++;
			if(dListCurrentPosition > dListTotal - 1) dListCurrentPosition = dListTotal - 1;
			if(dListCurrentPosition > dListMaxDisplay)
	        {
		        if((dListCurrentPosition+dListScrollPosition) < dListTotal) dListScrollPosition++;
			    dListCurrentPosition = dListMaxDisplay;
			}
			break;

		case DIR_LEFT:
			dListScrollPosition -= MAX_PAGE_MODS_FAST_FORWARD;
			if(dListScrollPosition < 0)
			{
				dListScrollPosition = 0;
				dListCurrentPosition -= MAX_PAGE_MODS_FAST_FORWARD;
			}
			if(dListCurrentPosition < 0) dListCurrentPosition = 0;
			break;

		case DIR_RIGHT:
			dListCurrentPosition += MAX_PAGE_MODS_FAST_FORWARD;
			if(dListCurrentPosition > dListTotal - 1) dListCurrentPosition = dListTotal - 1;
			if(dListCurrentPosition > dListMaxDisplay)
	        {
		        //if((dListCurrentPosition + dListScrollPosition) < dListTotal)
                    dListScrollPosition += MAX_PAGE_MODS_FAST_FORWARD;
		        if((dListCurrentPosition + dListScrollPosition) > dListTotal - 1) dListScrollPosition = dListTotal - MAX_PAGE_MODS_LENGTH;
			    dListCurrentPosition = dListMaxDisplay;
			}
			break;

		case WIIMOTE_PLUS:
		case WIIMOTE_2:
		case CC_PLUS:
		case CC_A:
		case GC_START:
		case GC_A:
			// Start Engine!
			status = 1;
			break;

		case WIIMOTE_HOME: // TODO? make a nice-looking Home menu
		case CC_HOME:
		case GC_Z:
			// Exit Engine!
			status = 2;
			break;

		case WIIMOTE_1:
		case CC_X:
		case GC_X:
			status = 3;
			break;

		default:
			// No Update Needed!
			status = 0;
			break;
	}
	return status;
}

void initMenu(int type)
{
	// Read Logo or Menu from Array.
	if(!type) Source = pngToScreen(isWide ? (void*) openbor_logo_480x272_png.data : (void*) openbor_logo_320x240_png.data);
	else Source = pngToScreen(isWide ? (void*) openbor_menu_480x272_png.data : (void*) openbor_menu_320x240_png.data);

	// Depending on which mode we are in (WideScreen/FullScreen)
	// allocate proper size for final screen.
	Screen = allocscreen(Source->width, Source->height, PIXEL_32);

	// Allocate Scaler.
	Scaler = allocscreen(Screen->width, Screen->height, PIXEL_32);

	control_init(2);
	apply_controls();
}

void termMenu()
{
	freescreen(&Source);
	Source = NULL;
	freescreen(&Scaler);
	Scaler = NULL;
	freescreen(&Screen);
	Screen = NULL;
	control_exit();
}

void draw_vscrollbar() {
    int offset_x = (isWide ? 30 : 7)    - 3;
    int offset_y = (isWide ? 33 : 22)   + 4;
    int box_width = 144;
    int box_height = 194;
    int min_vscrollbar_height = 2;
    int vbar_height = box_height;
    int vbar_width = 4;
    float vbar_ratio;
    int vspace = 0;
    int vbar_y = 0;

    if (dListTotal <= MAX_PAGE_MODS_LENGTH) return;

    // set v scroll bar height
    vbar_ratio = ((MAX_PAGE_MODS_LENGTH * 100.0f) / dListTotal) / 100.0f;
    vbar_height = box_height * vbar_ratio;
    if (vbar_height < min_vscrollbar_height) vbar_height = min_vscrollbar_height;

    // set v scroll bar position
    vspace = box_height - vbar_height;
    vbar_y = (int)(((dListScrollPosition) * vspace) / (dListTotal - MAX_PAGE_MODS_LENGTH));

    // draw v scroll bar
    printBox( (offset_x + box_width - vbar_width), offset_y, vbar_width, box_height, LIGHT_GRAY, 0, Scaler);
    printBox( (offset_x + box_width - vbar_width), (offset_y + vbar_y), vbar_width, vbar_height, GRAY, 0, Scaler);
    //printText(Scaler, 10,220, BLACK, 0, 0, "%d/%d space: %d, vbar_y: %d vbar_height: %d", (dListCurrentPosition + dListScrollPosition), dListTotal, vspace, vbar_y, vbar_height);
}

void drawMenu()
{
	char listing[45] = {""};
	int list = 0;
	int shift = 0;
	int colors = 0;
	int imageX = 0, imageY = 0;
	int imageW = factor == 4 ? 640 : (factor == 2 ? 320 : 160);
	int imageH = factor == 4 ? 480 : (factor == 2 ? 240 : 120);

	copyScreens(Source);
	if(dListTotal < 1) printText(Scaler, (isWide ? 30 : 8), (isWide ? 33 : 24), RED, 0, 0, "No Mods In Paks Folder!");
	for(list=0; list<dListTotal; list++)
	{
		if(list < MAX_PAGE_MODS_LENGTH)
		{
			shift = 0;
			colors = GRAY;
			strncpy(listing, "", (isWide ? 44 : 28));
			if(strlen(filelist[list+dListScrollPosition].filename)-4 < (isWide ? 44 : 28))
				safe_strncpy(listing, filelist[list+dListScrollPosition].filename, strlen(filelist[list+dListScrollPosition].filename)-4);
			if(strlen(filelist[list+dListScrollPosition].filename)-4 > (isWide ? 44 : 28))
				safe_strncpy(listing, filelist[list+dListScrollPosition].filename, (isWide ? 44 : 28));
			if(list == dListCurrentPosition)
			{
				shift = 2;
				colors = RED;
				if (!PREVIEW_SCREENSHOTS)
                {
                    if(filelist[list].preview != NULL)
                    {
                        imageX = factor * (isWide ? 286 : 155);
                        imageY = factor * (isWide ? (factor == 4 ? (s16)32.5 : 32) : (factor == 4 ? (s16)21.5 : 21));
                        printScreen(filelist[list].preview, imageX, imageY, imageW, imageH, 0, Scaler);

                        //drawScreens(filelist[list].preview, imageX, imageY);
                        //freescreen(&Image);
                        //Image = NULL;
                    }
                    //else printText(Scaler, (isWide ? 288 : 157), (isWide ? 141 : 130), RED, 0, 0, "No Preview Available!");
                }
                else
                {
                    s_screen* Image = getPreviewScreen(filelist[list+dListScrollPosition].filename);
                    if(Image)
                    {
                        //draw screen with the preview image
                        drawScreens(Image, imageX, imageY);
                    }
                }
			}
			printText(Scaler, (isWide ? 30 : 7) + shift, (isWide ? 33 : 22)+(11*list) , colors, 0, 0, "%s", listing);
			draw_vscrollbar();
		}
	}

	printText(Scaler, (isWide ? 26 : 5), (isWide ? 11 : 4), WHITE, 0, 0, "OpenBoR %s", VERSION);
	printText(Scaler, (isWide ? 392 : 261),(isWide ? 11 : 4), WHITE, 0, 0, __DATE__);
	printText(Scaler, (isWide ? 23 : 4),(isWide ? 251 : 226), WHITE, 0, 0, "%s: Start Game", control_getkeyname(savedata.keys[0][SDID_ATTACK]));
	printText(Scaler, (isWide ? 150 : 84),(isWide ? 251 : 226), WHITE, 0, 0, "%s: BGM Player", control_getkeyname(savedata.keys[0][SDID_ATTACK2]));
	printText(Scaler, (isWide ? 270 : 164),(isWide ? 251 : 226), WHITE, 0, 0, "%s: View Logs", control_getkeyname(savedata.keys[0][SDID_JUMP]));
	printText(Scaler, (isWide ? 390 : 244),(isWide ? 251 : 226), WHITE, 0, 0, "%s: Quit Game", control_getkeyname(savedata.keys[0][SDID_SPECIAL]));
   	printText(Scaler, (isWide ? 330 : 197),(isWide ? 170 : 155), BLACK, 0, 0, "www.ChronoCrash.com");
	printText(Scaler, (isWide ? 322 : 190),(isWide ? 180 : 165), BLACK, 0, 0, "www.SenileTeam.com");

#ifdef SPK_SUPPORTED
	printText(Scaler, (isWide ? 324 : 192),(isWide ? 191 : 176), DARK_RED, 0, 0, "SecurePAK Edition");
#endif

	drawScreens(NULL, 0, 0);
}

void drawLogs()
{
	int i=which_logfile, j, k, l, done=0;
	bothkeys = bothnewkeys = 0;
	s_screen *Viewer = NULL;

	bothkeys = bothnewkeys = 0;
	Viewer = allocscreen(Source->width, Source->height, Source->pixelformat);
	clearscreen(Viewer);
	bothkeys = bothnewkeys = 0;

	while(!done)
	{
	    copyScreens(Viewer);
	    //inputrefresh(0);
	    refreshInput();
	    printText(Scaler, (isWide ? 410 : 250), 3, RED, 0, 0, "Quit : 1/B");
		if(buttonsPressed & (WIIMOTE_1|CC_B|GC_B)) done = 1;

		if(logfile[i].ready)
		{
			printText(Scaler, 5, 3, RED, 0, 0, "OpenBorLog.txt");
			if(buttonsHeld & DIR_UP) --logfile[i].line;
	        if(buttonsHeld & DIR_DOWN) ++logfile[i].line;
			if(buttonsHeld & DIR_LEFT) logfile[i].line = 0;
			if(buttonsHeld & DIR_RIGHT) logfile[i].line = logfile[i].rows - (LOG_SCREEN_END - LOG_SCREEN_TOP);
			if(logfile[i].line > logfile[i].rows - (LOG_SCREEN_END - LOG_SCREEN_TOP) - 1) logfile[i].line = logfile[i].rows - (LOG_SCREEN_END - LOG_SCREEN_TOP) - 1;
			if(logfile[i].line < 0) logfile[i].line = 0;
			for(l=LOG_SCREEN_TOP, j=logfile[i].line; j<logfile[i].rows-1; l++, j++)
			{
				if(l<LOG_SCREEN_END)
				{
					char textpad[480] = {""};
					for(k=0; k<480; k++)
					{
						if(!logfile[i].buf->ptr[logfile[i].pos[j]+k]) break;
						textpad[k] = logfile[i].buf->ptr[logfile[i].pos[j]+k];
					}
					if(logfile[i].rows>0xFFFF)
						printText(Scaler, 5, l*10, WHITE, 0, 0, "0x%08x:  %s", j, textpad);
					else
						printText(Scaler, 5, l*10, WHITE, 0, 0, "0x%04x:  %s", j, textpad);
				}
				else break;
			}
		}
		else if(i == SCRIPT_LOG) printText(Scaler, 5, 3, RED, 0, 0, "Log NOT Found: ScriptLog.txt");
		else                     printText(Scaler, 5, 3, RED, 0, 0, "Log NOT Found: OpenBorLog.txt");

	    drawScreens(NULL, 0, 0);
	}
	freescreen(&Viewer);
	Viewer = NULL;
	drawMenu();
}

void drawLogo()
{
	unsigned startTime;
	initMenu(0);
	copyScreens(Source);
	drawScreens(NULL, 0, 0);
	vga_vwait();
	startTime = timer_gettick();

	// The logo displays for 2 seconds.  Let's put that time to good use.
	dListTotal = findPaks();

	while(1) { // display logo for remainder of time
		if(timer_gettick() - startTime >= 2000) break;
	}
	termMenu();
}

void fillRect(s_screen *dest, Rect *rect, u32 color)
{
	u32 *data = (u32*)dest->data;
	int x, y, width=dest->width;
	for(y=rect->y; y<rect->y+rect->height; y++)
	{
		for(x=rect->x; x<rect->x+rect->width; x++)
		{
			data[x+y*width] = color;
		}
	}
}

void setVideoMode()
{
	if(isWide) // 480x272
	{
		videomodes.mode    = savedata.screen[videoMode][0];
		videomodes.filter  = savedata.screen[videoMode][1];
		videomodes.hRes    = 480;
		videomodes.vRes    = 272;
		videomodes.hScale  = (float)1.5;
		videomodes.vScale  = (float)1.13;
		videomodes.hShift  = 80;
		videomodes.vShift  = 20;
		videomodes.dOffset = 263;
		videomodes.pixel   = 4;
	}
	else // 320x240
	{
		videomodes.mode    = savedata.screen[videoMode][0];
		videomodes.filter  = savedata.screen[videoMode][1];
		videomodes.hRes    = 320;
		videomodes.vRes    = 240;
		videomodes.hScale  = 1;
		videomodes.vScale  = 1;
		videomodes.hShift  = 0;
		videomodes.vShift  = 0;
		videomodes.dOffset = 231;
		videomodes.pixel   = 4;
	}

	video_set_mode(videomodes);
}

void Menu()
{
	int done = 0;
	int ctrl = 0;

	// Set video mode based on aspect ratio
	if(CONF_GetAspectRatio() == CONF_ASPECT_16_9) isWide = 1;
	setVideoMode();
	drawLogo();

	// Skips menu if we already have a .pak to load
	int quicklaunch = (packfile[0] == '\0') ? 0 : 1;

	if(!quicklaunch)
	{
		dListTotal = findPaks();
		dListCurrentPosition = 0;
		if(dListTotal != 1)
		{
			sortList();
			getAllLogs();
			initMenu(1);
			drawMenu();
			pControl = ControlMenu;

			while(!done)
			{
				ctrl = Control();
				switch(ctrl)
				{
					case 1:
						if (dListTotal > 0) done = 1;
						break;

					case 2:
						done = 1;
						break;

					case 3:
						drawLogs();
						break;

					case -1:
						drawMenu();
						break;

					case -2:
						// BGM player isn't supported
						break;

		            default:
						break;
				}
			}
			freeAllLogs();
			termMenu();
			if(ctrl == 2)
			{
				if (filelist)
				{
					free(filelist);
					filelist = NULL;
				}
				borExit(0);
			}
		}
		getBasePath(packfile, filelist[dListCurrentPosition+dListScrollPosition].filename, 1);
	}
	free(filelist);
}

