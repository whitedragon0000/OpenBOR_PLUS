/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef VIDEO_H
#define VIDEO_H

#include "gfxtypes.h"
#include "types.h"
#include "yuv.h"
#include "SDL.h"

extern u8 pDeltaBuffer[480 * 2592];
extern int opengl;

int SetVideoMode(int, int, int, bool);
void FramerateDelay();

// Frees all VESA shit when returning to textmode
int video_set_mode(s_videomodes);
int video_copy_screen(s_screen*);
void video_clearscreen();
void video_fullscreen_flip();
void video_stretch(int);
void video_set_window_title(const char*);
void video_set_color_correction(int, int);

// for WebM video playback
int video_setup_yuv_overlay(const yuv_video_mode*);
int video_prepare_yuv_frame(yuv_frame*);
int video_display_yuv_frame(void);


int video_current_refresh_rate();
void set_native_screen_size(int);
void on_system_ui_visibility_change_event(int);
void reset_touchstates(void);
#endif


