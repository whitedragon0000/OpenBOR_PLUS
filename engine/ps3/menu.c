/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#include <sys/time.h>
#include <sys/unistd.h>
#include <io/pad.h>
#include "SDL.h"
#include "sdlport.h"
#include "video.h"
#include "soundmix.h"
#include "packfile.h"
//#include "gfx.h"
#include "hankaku.h"
#include "stristr.h"
#include "stringptr.h"
#include "control.h"
#include "openbor.h"

#include "pngdec.h"
#include "../resources/OpenBOR_Logo_480x272_png.h"
#include "../resources/OpenBOR_Logo_320x240_png.h"
#include "../resources/openbor_menu_480x272_sony_png.h"
#include "../resources/OpenBOR_Menu_320x240_png.h"
#include "../resources/logviewer_480x272_png.h"
#include "../resources/logviewer_320x240_png.h"

#include <dirent.h>

extern s_videomodes videomodes;
extern s_screen* vscreen;
extern int nativeHeight;
extern int nativeWidth;
static s_screen* bgscreen;
static s_screen* logscreen;

#ifdef SONY_REVERSE_COLOR
#define RGB32(R,G,B) ((B) << 8 | ((G) << 16) | ((R) << 24)) & 0xFFFFFF00
#else
#define RGB32(R,G,B) ((R) | ((G) << 8) | ((B) << 16))
#endif
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

static int bpp = 32;
static int isWide = 0;
static int isFull = 0;
static int dListTotal;
static int dListCurrentPosition;
static int dListScrollPosition;
static int which_logfile = OPENBOR_LOG;
static FILE *bgmFile = NULL;
static unsigned int bgmPlay = 0, bgmLoop = 0, bgmCycle = 0, bgmCurrent = 0, bgmStatus = 0;
static fileliststruct *filelist;
extern u64 bothkeys, bothnewkeys;
extern const s_drawmethod plainmethod;
static unsigned int buttonsHeld = 0;
static unsigned int buttonsPressed = 0;

typedef struct{
	stringptr *buf;
	int *pos;
	int line;
	int rows;
	char ready;
}s_logfile;
static s_logfile logfile[2];

typedef int (*ControlInput)();

static int ControlMenu();
static int ControlBGM();
static void PlayBGM();
static void StopBGM();
static ControlInput pControl;

static int Control()
{
	return pControl();
}

static void refreshInput()
{
    unsigned int buttons = 0;
	padInfo padinfo;
	int i;

	ioPadGetInfo (&padinfo);
	for (i = 0; i < MAX_BOR_PADS; i++)
    {
        if (padinfo.status[i])
        {
            buttons |= getPad(i);
        }
    }

    // update buttons pressed (not held)
    buttonsPressed = buttons & ~buttonsHeld;
    buttonsHeld = buttons;
}

static void getAllLogs()
{
	ptrdiff_t i, j, k;
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

static void freeAllLogs()
{
	int i;
	for(i=0; i<2; i++)
	{
		if(logfile[i].ready)
		{
			free_string(logfile[i].buf);
			logfile[i].buf = NULL;
			free(logfile[i].pos);
			logfile[i].pos = NULL;
		}
	}
}

static void sortList()
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

static void printText(int x, int y, int col, int backcol, int fill, char *format, ...)
{
	int x1, y1, i;
	u32 data;
	u16 *line16 = NULL;
	u32 *line32 = NULL;
	u8 *font;
	u8 ch = 0;
	char buf[128] = {""};
	int pitch = vscreen->width*bpp/8;
	va_list arglist;
		va_start(arglist, format);
		vsprintf(buf, format, arglist);
		va_end(arglist);

	for(i=0; i<sizeof(buf); i++)
	{
		ch = buf[i];
		// mapping
		if (ch<0x20) ch = 0;
		else if (ch<0x80) { ch -= 0x20; }
		else if (ch<0xa0) {	ch = 0;	}
		else ch -= 0x40;
		font = (u8 *)&hankaku_font10[ch*10];
		// draw
		if (bpp == 16) line16 = (u16*)(vscreen->data + x*2 + y * pitch);
		else           line32 = (u32*)(vscreen->data + x*4 + y * pitch);

		for (y1=0; y1<10; y1++)
		{
			data = *font++;
			for (x1=0; x1<5; x1++)
			{
				if (data & 1)
				{
					if (bpp == 16) *line16 = col;
				    else           *line32 = col;
				}
				else if (fill)
				{
					if (bpp == 16) *line16 = backcol;
					else           *line32 = backcol;
				}

				if (bpp == 16) line16++;
				else           line32++;

				data = data >> 1;
			}
			if (bpp == 16) line16 += pitch/2-5;
			else           line32 += pitch/4-5;
		}
		x+=5;
	}
}

static s_screen *getPreview(char *filename)
{
    int width = 160;
    int height = 120;
	s_screen *title = NULL;
	s_screen *scale = NULL;

	// Grab current path and filename
	getBasePath(packfile, filename, 1);

	// Create & Load & Scale Image
	if(!loadscreen("data/bgs/title", packfile, NULL, PIXEL_x8, &title)) return NULL;
	if((scale = allocscreen(width, height, title->pixelformat)) == NULL) return NULL;

	scalescreen(scale, title);
	memcpy(scale->palette, title->palette, PAL_BYTES);

	// ScreenShots within Menu will be saved as "Menu"
	strncpy(packfile,"Menu.xxx",MAX_FILENAME_LEN);

	freescreen(&title);
	return scale;
}

static void StopBGM()
{
	sound_close_music();
	if (bgmFile)
	{
		fclose(bgmFile);
		bgmFile = NULL;
	}
	bgmPlay = 0;
}

static void PlayBGM()
{
	bgmPlay = packfile_music_play(filelist, bgmFile, bgmLoop, dListCurrentPosition, dListScrollPosition);
}

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
        u32 time = timer_gettick();

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

	bothnewkeys = 0;
	//inputrefresh(0);
	refreshInput();

	buttonsPressed |= hold_key_impulse(PS3_DPAD_DOWN,   IMPULSE_TIME, FIRST_KEYPRESS, FIRST_IMPULSE_TIME);
	buttonsPressed |= hold_key_impulse(PS3_DPAD_LEFT,   IMPULSE_TIME, FIRST_KEYPRESS, FIRST_IMPULSE_TIME);
	buttonsPressed |= hold_key_impulse(PS3_DPAD_UP,     IMPULSE_TIME, FIRST_KEYPRESS, FIRST_IMPULSE_TIME);
	buttonsPressed |= hold_key_impulse(PS3_DPAD_RIGHT,  IMPULSE_TIME, FIRST_KEYPRESS, FIRST_IMPULSE_TIME);

	switch(buttonsPressed)
	{
		case PS3_DPAD_UP:
			dListScrollPosition--;
			if(dListScrollPosition < 0)
			{
				dListScrollPosition = 0;
				dListCurrentPosition--;
			}
			if(dListCurrentPosition < 0) dListCurrentPosition = 0;
			break;

        case PS3_DPAD_LEFT:
			dListScrollPosition -= MAX_PAGE_MODS_FAST_FORWARD;
			if(dListScrollPosition < 0)
			{
				dListScrollPosition = 0;
				dListCurrentPosition -= MAX_PAGE_MODS_FAST_FORWARD;
			}
			if(dListCurrentPosition < 0) dListCurrentPosition = 0;
			break;

		case PS3_DPAD_DOWN:
			dListCurrentPosition++;
			if(dListCurrentPosition > dListTotal - 1) dListCurrentPosition = dListTotal - 1;
			if(dListCurrentPosition > dListMaxDisplay)
	        {
		        if((dListCurrentPosition+dListScrollPosition) < dListTotal) dListScrollPosition++;
			    dListCurrentPosition = dListMaxDisplay;
			}
			break;

		case PS3_DPAD_RIGHT:
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

		case PS3_START:
		case PS3_SQUARE:
			// Start Engine!
			status = 1;
			break;

		case PS3_TRIANGLE:
			pControl = ControlBGM;
			status = -2;
			break;

		case PS3_CROSS:
			// Exit Engine!
			status = 2;
			break;

		case PS3_CIRCLE:
			//drawLogs();
			status = 3;
			break;

		default:
			// No Update Needed!
			status = 0;
			break;
	}
	return status;
}

static int ControlBGM()
{
	int status = -2;
	int dListMaxDisplay = MAX_PAGE_MODS_LENGTH - 1;

	bothnewkeys = 0;
	//inputrefresh(0);
	refreshInput();

	buttonsPressed |= hold_key_impulse(PS3_DPAD_DOWN,   IMPULSE_TIME, FIRST_KEYPRESS, FIRST_IMPULSE_TIME);
	buttonsPressed |= hold_key_impulse(PS3_DPAD_UP,     IMPULSE_TIME, FIRST_KEYPRESS, FIRST_IMPULSE_TIME);

	switch(buttonsPressed)
	{
		case PS3_DPAD_UP:
			dListScrollPosition--;
			if(dListScrollPosition < 0)
			{
				dListScrollPosition = 0;
				dListCurrentPosition--;
			}
			if(dListCurrentPosition < 0) dListCurrentPosition = 0;
			break;

		case PS3_DPAD_DOWN:
			dListCurrentPosition++;
			if(dListCurrentPosition > dListTotal - 1) dListCurrentPosition = dListTotal - 1;
			if(dListCurrentPosition > dListMaxDisplay)
	        {
		        if((dListCurrentPosition+dListScrollPosition) < dListTotal) dListScrollPosition++;
			    dListCurrentPosition = dListMaxDisplay;
			}
			break;

		case PS3_L1:
			if(!bgmStatus || (bgmPlay && bgmCurrent == dListCurrentPosition+dListScrollPosition))
			{
				filelist[bgmCurrent].bgmTrack--;
				if(filelist[bgmCurrent].bgmTrack < 0) filelist[bgmCurrent].bgmTrack = filelist[bgmCurrent].nTracks-1;
				if(bgmStatus) PlayBGM();
			}
			break;

		case PS3_R1:
			if(!bgmStatus || (bgmPlay && bgmCurrent == dListCurrentPosition+dListScrollPosition))
			{
				filelist[bgmCurrent].bgmTrack++;
				if(filelist[bgmCurrent].bgmTrack > filelist[bgmCurrent].nTracks - 1) filelist[bgmCurrent].bgmTrack = 0;
				if(bgmStatus) PlayBGM();
			}
			break;

		case PS3_START:
		case PS3_SQUARE:
			if(bgmPlay) StopBGM();
			else PlayBGM();
			break;

		case PS3_TRIANGLE:
			if(!bgmPlay)
			{
				if(bgmLoop) bgmLoop = 0;
				else bgmLoop = 1;
			}
			break;

		case PS3_CIRCLE:
			if(!bgmPlay)
			{
				if(bgmCycle) bgmCycle = 0;
				else bgmCycle = 1;
			}
			break;

		case PS3_CROSS:
			pControl = ControlMenu;
			status = -1;
			break;

		default:
			// No Update Needed!
			status = 0;
			break;
	}
	return status;
}

static void initMenu(int type)
{

	isWide = (float)nativeHeight/(float)nativeWidth < 3.0f/4.0f;
	isFull = 1;
	bpp = 32;
	savedata.hwscale = 0.0f;

	pixelformat = PIXEL_x8;

	savedata.fullscreen = isFull;
	video_stretch(savedata.stretch);
	videomodes.hRes = isWide ? 480 :320;
	videomodes.vRes = isWide ? 272 :240;
	videomodes.pixel = pixelbytes[PIXEL_32];

	videomodes.hScale = 2.0f;
	videomodes.vScale = 2.0f;
	savedata.hwscale = 2.0f;
	savedata.hwfilter = 1;

	vscreen = allocscreen(videomodes.hRes, videomodes.vRes, PIXEL_32);

	video_set_mode(videomodes);

	// Read Logo or Menu from Array.
	if(!type)
		bgscreen = pngToScreen(isWide ? (void*) openbor_logo_480x272_png.data : (void*) openbor_logo_320x240_png.data);
	else
		bgscreen = pngToScreen(isWide ? (void*) openbor_menu_480x272_sony_png.data : (void*) openbor_menu_320x240_png.data);
	// CRxTRDude - Initialize log screen images
	logscreen = pngToScreen(isWide ? (void*) logviewer_480x272_png.data : (void*) logviewer_320x240_png.data);

	control_init(2);
	apply_controls();
	sound_init(12);
	sound_start_playback(savedata.soundbits,savedata.soundrate);
}

static void termMenu()
{
	videomodes.hRes = videomodes.vRes = 0;
	video_set_mode(videomodes);
	if(bgscreen) freescreen(&bgscreen);
	if(logscreen) freescreen(&logscreen);
	if(vscreen) freescreen(&vscreen);
	sound_exit();
	control_exit();
}

static void blit_video_menu(s_screen* vscreen)
{
    video_stretch(1); // set to fullscreen
    video_copy_screen(vscreen);
    video_stretch(savedata.stretch); // reset to saved value
}

static void draw_vscrollbar() {
    int offset_x = (isWide ? 110 : 7)   - 3;
    int offset_y = (isWide ? 33 : 22)   + 2;
    int box_width = 144;
    int box_height = 200;
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
    putbox( (offset_x + box_width - vbar_width), offset_y, vbar_width, box_height, LIGHT_GRAY, vscreen, NULL);
    putbox( (offset_x + box_width - vbar_width), (offset_y + vbar_y), vbar_width, vbar_height, GRAY, vscreen, NULL);
    //printText(10,220, BLACK, 0, 0, "%d/%d space: %d, vbar_y: %d vbar_height: %d", (dListCurrentPosition + dListScrollPosition), dListTotal, vspace, vbar_y, vbar_height);
}

static void drawMenu()
{
	char listing[45] = {""};
	int list = 0;
	int shift = 0;
	int colors = 0;
	s_screen* Image = NULL;

	putscreen(vscreen,bgscreen,0,0,NULL);
	if(dListTotal < 1) printText((isWide ? 30 : 8), (isWide ? 33 : 24), RED, 0, 0, "No Mods In Paks Folder!");
	for(list = 0; list < dListTotal; list++)
	{
		if(list < MAX_PAGE_MODS_LENGTH)
		{
		    int len = strlen(filelist[list+dListScrollPosition].filename)-4;
			shift = 0;
			colors = GRAY;
			strncpy(listing, "", (isWide ? 44 : 28));
			if(len < (isWide ? 44 : 28))
                safe_strncpy(listing, filelist[list+dListScrollPosition].filename, len);
			else
				safe_strncpy(listing, filelist[list+dListScrollPosition].filename, (isWide ? 44 : 28));
			if(list == dListCurrentPosition)
			{
				shift = 2;
				colors = RED;
				Image = getPreview(filelist[list+dListScrollPosition].filename);
			}
			printText((isWide ? 30 : 7) + shift, (isWide ? 33 : 22)+(11*list) , colors, 0, 0, "%s", listing);
			draw_vscrollbar();
		}
	}

	printText((isWide ? 26 : 5), (isWide ? 11 : 4), WHITE, 0, 0, "OpenBoR %s", VERSION);
	printText((isWide ? 392 : 261),(isWide ? 11 : 4), WHITE, 0, 0, __DATE__);
	printText((isWide ? 23 : 4),(isWide ? 251 : 226), WHITE, 0, 0, "Start Game", control_getkeyname(savedata.keys[0][SDID_ATTACK]));
	printText((isWide ? 150 : 84),(isWide ? 251 : 226), WHITE, 0, 0, "BGM Player", control_getkeyname(savedata.keys[0][SDID_ATTACK2]));
	printText((isWide ? 270 : 164),(isWide ? 251 : 226), WHITE, 0, 0, "View Logs", control_getkeyname(savedata.keys[0][SDID_JUMP]));
	printText((isWide ? 390 : 244),(isWide ? 251 : 226), WHITE, 0, 0, "Quit Game", control_getkeyname(savedata.keys[0][SDID_SPECIAL]));
    printText((isWide ? 320 : 188),(isWide ? 175 : 158), BLACK, 0, 0, "www.chronocrash.com");
	printText((isWide ? 322 : 190),(isWide ? 185 : 168), BLACK, 0, 0, "www.SenileTeam.com");

#ifdef SPK_SUPPORTED
	printText((isWide ? 324 : 192),(isWide ? 191 : 176), DARK_RED, 0, 0, "SecurePAK Edition");
#endif

	if(Image)
	{
		putscreen(vscreen, Image, isWide ? 286 : 155, isWide ? 32:21, NULL);
		freescreen(&Image);
	}
	else
		printText((isWide ? 288 : 157), (isWide ? 141 : 130), RED, 0, 0, "No Preview Available!");

	blit_video_menu(vscreen);
}

static void drawBGMPlayer()
{
	char listing[45] = {""}, bgmListing[25] = {""};
	char t1[64] = "", t2[25] = "Unknown";
	char a1[64] = "", a2[25] = "Unknown";
	int list = 0, colors = 0, shift = 0;
	int filename_len = strlen(filelist[bgmCurrent].filename)-4;

	// Allocate Preview Box for Music Text Info.
	putscreen(vscreen,bgscreen,0,0,NULL);
	putbox((isWide ? 286 : 155),(isWide ? 32 : 21),160,120,LIGHT_GRAY,vscreen,NULL);

	for(list = 0; list < dListTotal; list++)
	{
		if(list < MAX_PAGE_MODS_LENGTH)
		{
		    int len = strlen(filelist[list+dListScrollPosition].filename)-4;
			shift = 0;
			colors = GRAY;
			strncpy(listing, "", (isWide ? 44 : 28));
			if(len < (isWide ? 44 : 28))
				safe_strncpy(listing, filelist[list+dListScrollPosition].filename, len);
			else
				safe_strncpy(listing, filelist[list+dListScrollPosition].filename, (isWide ? 44 : 28));
			if(list==dListCurrentPosition) { shift = 2; colors = RED; }
			printText((isWide ? 30 : 7) + shift, (isWide ? 33 : 22)+(11*list) , colors, 0, 0, "%s", listing);
			draw_vscrollbar();
		}
	}

	printText((isWide ? 26 : 5), (isWide ? 11 : 4), WHITE, 0, 0, "OpenBoR %s", VERSION);
	printText((isWide ? 392 : 261),(isWide ? 11 : 4), WHITE, 0, 0, __DATE__);
	printText((isWide ? 23 : 4),(isWide ? 251 : 226), WHITE, 0, 0, "%s", bgmPlay ? "Stop" : "Play");
	printText((isWide ? 150 : 84),(isWide ? 251 : 226), WHITE, 0, 0, "%s", bgmLoop ? "Repeat On" : "Repeat Off");
	printText((isWide ? 270 : 164),(isWide ? 251 : 226), WHITE, 0, 0, "%s", bgmCycle ? "Cycle On" : "Cycle Off");
	printText((isWide ? 390 : 244),(isWide ? 251 : 226), WHITE, 0, 0, "Exit Player");
    printText((isWide ? 320 : 188),(isWide ? 175 : 158), BLACK, 0, 0, "www.chronocrash.com");
	printText((isWide ? 322 : 190),(isWide ? 185 : 168), BLACK, 0, 0, "www.SenileTeam.com");

#ifdef SPK_SUPPORTED
	printText((isWide ? 324 : 192),(isWide ? 191 : 176), DARK_RED, 0, 0, "SecurePAK Edition");
#endif

	if(!bgmPlay) bgmCurrent = dListCurrentPosition+dListScrollPosition;
	if(filename_len < 24)
		safe_strncpy(bgmListing, filelist[bgmCurrent].filename, strlen(filelist[bgmCurrent].filename) - 4);
	else
		safe_strncpy(bgmListing, filelist[bgmCurrent].filename, 24);
	if(!sound_query_music(a1, t1))
	{
		PlayBGM();
		sound_query_music(a1, t1);
		StopBGM();
	}
	if(t1[0]) safe_strncpy(t2, t1, 25);
	if(a1[0]) safe_strncpy(a2, a1, 25);
	printText((isWide ? 288 : 157),(isWide ? 35 : 23) + (11 * 0), DARK_RED, 0, 0, "Game: %s", bgmListing);
	printText((isWide ? 288 : 157),(isWide ? 35 : 23) + (11 * 1), bgmPlay ? DARK_GREEN : DARK_BLUE, 0, 0, "Total Tracks: %d", filelist[bgmCurrent].nTracks-1);
	printText((isWide ? 288 : 157),(isWide ? 35 : 23) + (11 * 2), bgmPlay ? DARK_GREEN : DARK_BLUE, 0, 0, "Current Track: %d", filelist[bgmCurrent].bgmTrack);
	printText((isWide ? 288 : 157),(isWide ? 35 : 23) + (11 * 3), bgmPlay ? DARK_GREEN : DARK_BLUE, 0, 0, "File: %s", filelist[bgmCurrent].bgmFileName[filelist[bgmCurrent].bgmTrack]);
	printText((isWide ? 288 : 157),(isWide ? 35 : 23) + (11 * 4), bgmPlay ? DARK_GREEN : DARK_BLUE, 0, 0, "Track: %s", t2);
	printText((isWide ? 288 : 157),(isWide ? 35 : 23) + (11 * 5), bgmPlay ? DARK_GREEN : DARK_BLUE, 0, 0, "Artist: %s", a2);

	blit_video_menu(vscreen);
}

static void drawLogs()
{
	int i=which_logfile, j, k, l, done=0;
	bothkeys = bothnewkeys = 0;

	while(!done)
	{
		putscreen(vscreen,logscreen,0,0,NULL);

        //inputrefresh(0);
        refreshInput();

	    sound_update_music();
#if OPENDINGUX
	    printText(250, 3, RED, 0, 0, "Quit : Select");
#else
	    printText((isWide ? 410 : 250), 3, RED, 0, 0, "Quit : Escape");
#endif
		if(buttonsPressed & PS3_CROSS) done = 1;

		if(logfile[i].ready)
		{
			printText(5, 3, RED, 0, 0, "OpenBorLog.txt");
			if(buttonsPressed & PS3_DPAD_UP) --logfile[i].line;
	        if(buttonsPressed & PS3_DPAD_DOWN) ++logfile[i].line;
			if(buttonsPressed & PS3_DPAD_LEFT) logfile[i].line = 0;
			if(buttonsPressed & PS3_DPAD_RIGHT) logfile[i].line = logfile[i].rows - (LOG_SCREEN_END - LOG_SCREEN_TOP);
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
						printText(5, l*10, WHITE, 0, 0, "0x%08x:  %s", j, textpad);
					else
						printText(5, l*10, WHITE, 0, 0, "0x%04x:  %s", j, textpad);
				}
				else break;
			}
		}
		else if(i == SCRIPT_LOG) printText(5, 3, RED, 0, 0, "Log NOT Found: ScriptLog.txt");
		else                     printText(5, 3, RED, 0, 0, "Log NOT Found: OpenBorLog.txt");

	    blit_video_menu(vscreen);
	}
	drawMenu();
}

static void drawLogo()
{
    int i;
    int delay;

    delay = 500;

    if(savedata.logo) {
        delay = 1;
        //return;
    }

	initMenu(0);

	for(i = 0; i < delay; i++)
    {
        blit_video_menu(bgscreen);
        SDL_Delay(1);
    }

	termMenu();
}

void Menu()
{
	int done = 0;
	int ctrl = 0;
	loadsettings();
	drawLogo();
	dListCurrentPosition = 0;
	if((dListTotal = findPaks()) != 1)
	{
		sortList();
		getAllLogs();
		packfile_music_read(filelist, dListTotal);
		initMenu(1);
		drawMenu();
		pControl = ControlMenu;

		while(!done)
		{
			sound_update_music();
			bgmStatus = sound_query_music(NULL, NULL);
			if(bgmPlay && !bgmStatus)
			{
				if(bgmCycle)
				{
					filelist[bgmCurrent].bgmTrack++;
					if(filelist[bgmCurrent].bgmTrack > filelist[bgmCurrent].nTracks - 1) filelist[bgmCurrent].bgmTrack = 0;
					PlayBGM();
				}
				else StopBGM();
				drawBGMPlayer();
			}

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
				    if (dListTotal > 0) drawBGMPlayer();
					break;

                default:
					break;
			}

            blit_video_menu(vscreen);
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
			//borExit(0);
			borShutdown(0, DEFAULT_SHUTDOWN_MESSAGE); // or SDL_Quit();
		}
	}
	getBasePath(packfile, filelist[dListCurrentPosition+dListScrollPosition].filename, 1);
	free(filelist);

	// Restore pixelformat default value.
	pixelformat = PIXEL_x8;
}

