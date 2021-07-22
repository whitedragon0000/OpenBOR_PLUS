/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef	CONTROL_H
#define	CONTROL_H

#include <stdint.h>

#define PSP_DPAD_UP         0x00000001
#define PSP_DPAD_RIGHT      0x00000002
#define PSP_DPAD_DOWN       0x00000004
#define PSP_DPAD_LEFT       0x00000008
#define PSP_CROSS           0x00000010
#define PSP_CIRCLE          0x00000020
#define PSP_SQUARE          0x00000040
#define PSP_TRIANGLE        0x00000080
#define PSP_LEFT_TRIGGER    0x00000100
#define PSP_RIGHT_TRIGGER   0x00000200
#define PSP_START           0x00000400
#define PSP_SELECT          0x00000800
#define PSP_NOTE			0x00001000
#define PSP_HOME			0x00002000
#define PSP_HOLD			0x00004000
#define PSP_SCREEN			0x00008000
#define PSP_VOLUP			0x00010000
#define PSP_VOLDOWN			0x00020000

#define	CONTROL_ESC					14

#define PAD_LOW_BOUND 0x30
#define PAD_HIGH_BOUND 0xC0

// 32 is an arbitrary number larger than the number of input devices that will ever be available
#define MAX_DEVICES                4
#define CONTROL_DEVICE_NAME_SIZE   64

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

