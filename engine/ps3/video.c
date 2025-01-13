/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 *
 * Video.c - adjunct to the main build's video.c.
 * Made by White Dragon (000whitedragon000@gmail.com).
 * Modifications by CRxTRDude, White Dragon and msmalik681.
 */

#include <math.h>
#include "types.h"
#include "video.h"
#include "vga.h"
#include "screen.h"
#include "sdlport.h"
#include "openbor.h"
#include "gfxtypes.h"
#include "gfx.h"
#include "videocommon.h"
#include "pngdec.h"
#include <sysutil/video.h>

#define NATIVE_WIDTH        640
#define NATIVE_HEIGHT       480
#define W_DEFAULT_MARGIN    0
#define H_DEFAULT_MARGIN    0

extern int videoMode;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
SDL_Texture *texture_base = NULL;

s_videomodes stored_videomodes;

#ifdef WEBM
yuv_video_mode stored_yuv_mode;
#endif

char windowTitle[MAX_LABEL_LEN] = {"OpenBOR PLUS"};

int stretch = 1;
int opengl = 0;

int nativeWidth, nativeHeight;           // resolution of device screen
static int textureWidth, textureHeight;  // dimensions of game screen and texture

int brightness = 0;

void initSDL()
{
    SDL_DisplayMode mode;
    int init_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;// | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER; // | SDL_INIT_HAPTIC

#ifdef CUSTOM_SIGNAL_HANDLER
    init_flags |= SDL_INIT_NOPARACHUTE;
#endif

    //printf("TEST: %d %d \n",mode.format,mode.refresh_rate);

    //SDL_setenv("SDL_VIDEODRIVER", "psl1ght", 1);

    if(SDL_Init(init_flags) < 0)
    {
        printf("SDL Failed to Init!!!! (%s)\n", SDL_GetError());
        borExit(0);
    }
    SDL_ShowCursor(SDL_DISABLE);
    //atexit(SDL_Quit); //White Dragon: use SDL_Quit() into sdlport.c it's best practice!

    mode.refresh_rate = 0;

    // Store the monitor's current resolution before setting the video mode for the first time

    /*videoState state;
    videoResolution res;
    videoGetState(0, 0, &state);
    if (videoGetState(0, 0, &state) == 0 && videoGetResolution(state.displayMode.resolution, &res) == 0) {

        videoConfiguration vconfig;
        memset(&vconfig, 0, sizeof(videoConfiguration));
        vconfig.resolution = state.displayMode.resolution;
        vconfig.format = VIDEO_BUFFER_FORMAT_XRGB;
        vconfig.pitch = res.width * sizeof(u32);
        vconfig.aspect = state.displayMode.aspect;//VIDEO_ASPECT_AUTO;
        
        if (videoConfigure(0, &vconfig, NULL, 0) != 0) return;
        if (videoGetState(0, 0, &state) != 0) return;

        nativeWidth = res.width;
        nativeHeight = res.height;
        printf("PS3 Screen Size: %i %i %i\n", res.width, res.height, state.displayMode.resolution);
    }
    else */if(SDL_GetDesktopDisplayMode(0, &mode) == 0)
    {
        nativeWidth = mode.w;
        nativeHeight = mode.h;
        printf("SDL Screen Size: %i %i\n", mode.w, mode.h);
        
    }
    else
    {
        nativeWidth = NATIVE_WIDTH;
        nativeHeight = NATIVE_HEIGHT;
    }

		//Hardcode full screen mode
    savedata.fullscreen = 1;

}

//Start of touch control UI code



/*
Start of video code. Unlike the original code for video, everything is
incorporated in video_set_mode, since this is isolated from the main
code.
*/

static unsigned pixelformats[4] = {SDL_PIXELFORMAT_INDEX8, SDL_PIXELFORMAT_RGB565, SDL_PIXELFORMAT_RGB888, SDL_PIXELFORMAT_RGBA8888};

int video_set_mode(s_videomodes videomodes)
{
    stored_videomodes = videomodes;

    //hardcode flags
    int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;

    savedata.fullscreen = 1;

    videomodes = setupPreBlitProcessing(videomodes);

    // 8-bit color should be transparently converted to 32-bit
    assert(videomodes.pixel == 2 || videomodes.pixel == 4);

    //destroy all
    /*if(texture_base)
    {
        SDL_DestroyTexture(texture_base);
        texture_base = NULL;
    }*/
    if(texture)
    {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }

    if(videomodes.hRes == 0 && videomodes.vRes == 0)
    {
        Term_Gfx();
        return 0;
    }

    if(!window && !(window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, nativeWidth, nativeHeight, flags)))
    {
        printf("error: %s\n", SDL_GetError());
        return 0;
    }

    if(!renderer && !(renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED)))
    {
        printf("error: %s\n", SDL_GetError());
        return 0;
    }

    //For status
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);

    printf("SDL video Renderer: %s \n", info.name);

    SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, savedata.hwfilter ? "nearest" : "linear", SDL_HINT_DEFAULT);

    // now create a texture

  	textureWidth = videomodes.hRes;
  	textureHeight = videomodes.vRes;

    if(!texture_base)
    {
        SDL_Surface *tmp_surface = SDL_CreateRGBSurface(0, 800, 480, 32, 0, 0, 0, 0);
        if(!tmp_surface)
        {
            printf("error: %s\n", SDL_GetError());
            return 0;
        }
        SDL_FillRect(tmp_surface, NULL, SDL_MapRGB(tmp_surface->format, 0, 0, 0));
        SDL_SetSurfaceBlendMode(tmp_surface, SDL_BLENDMODE_NONE);
        if(!(texture_base = SDL_CreateTextureFromSurface(renderer, tmp_surface)))
        {
            printf("error: %s\n", SDL_GetError());
            return 0;
        }
        SDL_FreeSurface(tmp_surface);
        tmp_surface = NULL;
    }

    if(!(texture = SDL_CreateTexture(renderer,  pixelformats[videomodes.pixel-1], SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight)))
    {
        printf("error: %s\n", SDL_GetError());
        return 0;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    video_clearscreen();

    return 1;
}

void video_fullscreen_flip()
{
}

void blit()
{
    //SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    //SDL_SetRenderTarget(renderer, NULL);
    //SDL_RenderSetLogicalSize(renderer, 0, 0);
    SDL_SetTextureBlendMode(texture_base, SDL_BLENDMODE_NONE);
    SDL_RenderCopy(renderer, texture_base, NULL, NULL);

    if(stretch)
    {
        //SDL_RenderSetLogicalSize(renderer, 0, 0);
        //SDL_RenderCopy(renderer, texture, NULL, NULL);
        unsigned scaledWidth  = nativeWidth  - W_DEFAULT_MARGIN;
        unsigned scaledHeight = ((scaledWidth * nativeHeight) / nativeWidth) + H_DEFAULT_MARGIN;

        SDL_Rect d_rect = {(int)(nativeWidth/2.0f - scaledWidth/2.0f), (int)(nativeHeight/2.0f - scaledHeight/2.0f), scaledWidth, scaledHeight};
        SDL_RenderCopy(renderer, texture, NULL, &d_rect);
    }
    else
    {
        //SDL_RenderSetLogicalSize(renderer, textureWidth, textureHeight);
        float aspectRatio = (float)textureWidth / (float)textureHeight;
        float newWidth = nativeHeight * aspectRatio;
        unsigned scaledWidth  = newWidth  - W_DEFAULT_MARGIN;
        unsigned scaledHeight = ((scaledWidth * nativeHeight) / newWidth) + H_DEFAULT_MARGIN;

        if (newWidth > nativeWidth) {
          float newHeight;

          newWidth = nativeWidth - W_DEFAULT_MARGIN;
          newHeight = (newWidth / aspectRatio) + H_DEFAULT_MARGIN;
          scaledWidth  = (unsigned)newWidth;
          scaledHeight = (unsigned)newHeight;
        }

        //SDL_Log("aspect: from %d/%d con %f, orig: %d/%d -> %d",textureWidth,textureHeight,aspectRatio,nativeWidth,nativeHeight,(int)newWidth);
        //SDL_Rect d_rect = {(int)(nativeWidth/2.0f - newWidth/2.0f), 0, (int)newWidth, nativeHeight};
        SDL_Rect d_rect = {(int)(nativeWidth/2.0f - scaledWidth/2.0f), (int)(nativeHeight/2.0f - scaledHeight/2.0f), scaledWidth, scaledHeight};
        SDL_RenderCopy(renderer, texture, NULL, &d_rect);
    }

    /*if(brightness > 0)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, brightness-1);
    }
    else if(brightness < 0)
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, (-brightness)-1);
    }
    SDL_RenderFillRect(renderer, NULL);*/

    //SDL_RenderSetLogicalSize(renderer, 0, 0);
    SDL_RenderPresent(renderer);
}

int video_copy_screen(s_screen *src)
{
    s_videosurface *surface = getVideoSurface(src);
   	SDL_UpdateTexture(texture, NULL, surface->data, surface->pitch);
    blit();

    //SDL_Delay(1);
    usleep(5);
    return 1;
}

void video_clearscreen()
{
    //SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    //SDL_RenderPresent(renderer);
}

void video_stretch(int enable)
{
    stretch = enable;
}

void vga_vwait(void)
{
	static int prevtick = 0;
	int now = timer_gettick();
	int wait = 1000/60 - (now - prevtick);
	if (wait>0)
	{
		SDL_Delay(wait);
	}
	else
    {
        SDL_Delay(1);
    }
	prevtick = now;
}

void video_set_color_correction(int gm, int br)
{
	brightness = br;
}

#ifdef WEBM
int video_setup_yuv_overlay(const yuv_video_mode *mode)
{
	stored_yuv_mode = *mode;
	if (texture) SDL_DestroyTexture(texture);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	texture = SDL_CreateTexture(renderer,
	                            SDL_PIXELFORMAT_YV12,
	                            SDL_TEXTUREACCESS_STREAMING,
	                            mode->width, mode->height);
	textureWidth = mode->display_width;
    textureHeight = mode->display_height;
	return texture ? 1 : 0;
}

int video_prepare_yuv_frame(yuv_frame *src)
{
	if (texture)
    {
        SDL_UpdateYUVTexture(texture, NULL, src->lum, stored_yuv_mode.width,
                             src->cr, stored_yuv_mode.width/2, src->cb,
                             stored_yuv_mode.width/2);
    }
	return 1;
}

int video_display_yuv_frame(void)
{
	blit();
	return 1;
}

int video_current_refresh_rate()
{
    SDL_DisplayMode display_mode;
    if (SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &display_mode) != 0)
        return 60;
    return display_mode.refresh_rate;
}

void video_set_window_title(const char* title)
{
	//if(window) SDL_SetWindowTitle(window, title);
	strncpy(windowTitle, title, sizeof(windowTitle)-1);
}
#endif
