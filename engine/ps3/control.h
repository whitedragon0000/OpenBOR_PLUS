/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under a BSD-style license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

#ifndef	CONTROL_H
#define	CONTROL_H

#include <stdint.h>

#define MAX_INPUT                   32
#define BTN_NUM                     16

#define ANAG_STAND          0x80
#define PAD_STICK_DEADZONE  0x70

#define PS3_DPAD_UP         0x00000001 //1..
#define PS3_DPAD_RIGHT      0x00000002
#define PS3_DPAD_DOWN       0x00000004
#define PS3_DPAD_LEFT       0x00000008
#define PS3_SQUARE          0x00000010
#define PS3_TRIANGLE        0x00000020
#define PS3_CROSS           0x00000040
#define PS3_CIRCLE          0x00000080
#define PS3_L2              0x00000100
#define PS3_R2              0x00000200
#define PS3_START           0x00000400
#define PS3_SELECT          0x00000800
#define PS3_L1              0x00001000
#define PS3_R1              0x00002000
#define PS3_L3              0x00004000
#define PS3_R3              0x00008000 //..16

#define	CONTROL_ESC			        15

// 32 is an arbitrary number larger than the number of input devices that will ever be available
#define MAX_DEVICES                4
#define CONTROL_DEVICE_NAME_SIZE   64

#define	CONTROL_NONE				(1+(BTN_NUM*99)) //Kratus (20-04-21) value used to clear all keys

typedef struct {
    int deviceID;
    uint32_t keyflags;
    uint32_t newkeyflags;
} s_playercontrols;

void control_init();
void control_exit();

/* Listen to input from deviceID. The first input from deviceID will be returned by the next call to
   control_getremappedkey(). Call with deviceID=-1 to finish remapping. */
void control_remapdevice(int deviceID);

// Returns the keycode of the first key pressed on the device being remapped, or -1 if nothing has been pressed yet
int control_getremappedkey();

// Returns an array of size SDID_COUNT
int *control_getmappings(int deviceID);

// Resets mappings for device to default
void control_resetmappings(int deviceID);
void control_update(s_playercontrols **allPlayerControls, int numPlayers);
void control_update_keyboard(s_playercontrols *keyboardControls);
const char *control_getkeyname(int deviceID, int keycode);
bool control_isvaliddevice(int deviceID);
const char *control_getdevicename(int deviceID);

bool control_loadmappings(const char *filename);
bool control_savemappings(const char *filename);
void control_rumble(s_playercontrols ** playercontrols, int player, int ratio, int msec);

// clears saved mappings and resets every device's mappings to defaults
void control_clearmappings();

unsigned int getPad(int port);

#define control_getmappedkeyname(deviceID, key) control_getkeyname(deviceID, control_getmappings(deviceID)[key])

#endif
