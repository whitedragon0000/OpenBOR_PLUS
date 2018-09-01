/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <string.h>
#include <io/pad.h>
#include "video.h"
#include "globals.h"
#include "control.h"
#include "stristr.h"
#include "sblaster.h"
#include "openbor.h"

#define	PAD_START			1

#define	PAD_END				(BTN_NUM*MAX_BOR_PADS)
#define ANAG_STAND          0x80
#define PAD_STICK_DEADZONE  0x70

static int usejoy;
static int lastkey[MAX_BOR_PADS];
static padData paddata[MAX_BOR_PADS];

static const char *padnames[PAD_END+1+1] = {
	"...",
#define CONTROLNAMES(x) \
	x" Up",             \
	x" Right",          \
	x" Down",           \
	x" Left",           \
	x" Square",         \
	x" Triangle",       \
	x" Cross",          \
	x" Circle",         \
	x" L1",             \
	x" R1",             \
	x" Start",          \
	x" Select",         \
	x" L2",             \
	x" R2",             \
	x" L3",             \
	x" R3",
	CONTROLNAMES("P3 1")
	CONTROLNAMES("P3 2")
	CONTROLNAMES("P3 3")
	CONTROLNAMES("P3 4")
	"undefined"
};

static int flag_to_index(unsigned int flag)
{
	int index = 0;
	unsigned int bit = 1;
	while (!((bit<<index)&flag) && index<31) ++index;
	return index;
}

void control_exit()
{
	usejoy = 0;
	ioPadEnd();
}

void control_init(int joy_enable)
{
	usejoy = joy_enable;
	ioPadInit(MAX_BOR_PADS);
}

int control_usejoy(int enable)
{
	usejoy = enable;
	return 0;
}

int control_getjoyenabled()
{
	return usejoy;
}

int keyboard_getlastkey(void)
{
	int i, ret=0;
	for (i=0; i<MAX_BOR_PADS; i++)
	{
		ret |= lastkey[i];
		lastkey[i] = 0;
	}
	return ret;
}

void control_setkey(s_playercontrols * pcontrols, unsigned int flag, int key)
{
	if (!pcontrols) return;
	pcontrols->settings[flag_to_index(flag)] = key;
	pcontrols->keyflags = pcontrols->newkeyflags = 0;
}

// Scan input for newly-pressed keys.
// Return value:
// 0  = no key was pressed
// >0 = key code for pressed key
// <0 = error
int control_scankey()
{
	static unsigned ready = 0;
	unsigned i, k=0;

	for (i=0; i<MAX_BOR_PADS; i++)
	{
		if (lastkey[i])
		{
			k = 1 + i*BTN_NUM + flag_to_index(lastkey[i]);
			break;
		}
	}

	if (ready && k)
	{
		ready = 0;
		return k;
	}
	ready = (!k);
	return 0;
}

char * control_getkeyname(unsigned keycode)
{
	if (keycode >= PAD_START && keycode <= PAD_END) return (char*)padnames[keycode];
	return "...";
}

void control_update(s_playercontrols ** playercontrols, int numplayers)
{
	unsigned int k;
	unsigned int i;
	int player;
	int t;
	s_playercontrols * pcontrols;
	unsigned port[MAX_BOR_PADS];
	padInfo padinfo;

	ioPadGetInfo (&padinfo);
	for (i = 0; i < MAX_BOR_PADS; i++)
    {
        port[i] = 0;
        if (padinfo.status[i])
        {
            port[i] = getPad(i);
        }
    }

	for (player = 0; player < numplayers; player++)
	{
		pcontrols = playercontrols[player];
		k = 0;
		for (i = 0; i < MAX_INPUT; i++)
		{
			t = pcontrols->settings[i];
			if (t >= PAD_START && t <= PAD_END)
			{
				int portnum = (t-1) / BTN_NUM;
				int shiftby = (t-1) % BTN_NUM;
				if (portnum >= 0 && portnum < MAX_BOR_PADS)
				{
					if ((port[portnum] >> shiftby) & 1) k |= (1<<i);
				}
			}
		}
		pcontrols->kb_break = 0;
		pcontrols->newkeyflags = k & (~pcontrols->keyflags);
		pcontrols->keyflags = k;
	}
}

void control_rumble(int port, int ratio, int msec)
{
    padActParam actparam;
    actparam.small_motor = 1;
    actparam.large_motor = (255 * ratio) / 100;
    ioPadSetActDirect(port, &actparam);
}

unsigned int getPad(int port)
{
	unsigned int btns = 0;

    ioPadGetData(port, &paddata[port]);

    if (control_getjoyenabled())
    {
             if (paddata[port].ANA_L_V > ANAG_STAND + PAD_STICK_DEADZONE)   btns |= PS3_DPAD_DOWN;
        else if (paddata[port].ANA_L_V < ANAG_STAND - PAD_STICK_DEADZONE)   btns |= PS3_DPAD_UP;
             if (paddata[port].ANA_L_H < ANAG_STAND - PAD_STICK_DEADZONE)   btns |= PS3_DPAD_LEFT;
        else if (paddata[port].ANA_L_H > ANAG_STAND + PAD_STICK_DEADZONE)   btns |= PS3_DPAD_RIGHT;
    }

    if (paddata[port].BTN_SELECT)   btns |= PS3_SELECT;
    if (paddata[port].BTN_START)    btns |= PS3_START;
    if (paddata[port].BTN_UP)	    btns |= PS3_DPAD_UP;
    if (paddata[port].BTN_RIGHT)    btns |= PS3_DPAD_RIGHT;
    if (paddata[port].BTN_DOWN)     btns |= PS3_DPAD_DOWN;
    if (paddata[port].BTN_LEFT)     btns |= PS3_DPAD_LEFT;
    if (paddata[port].BTN_TRIANGLE) btns |= PS3_TRIANGLE;
    if (paddata[port].BTN_CIRCLE)   btns |= PS3_CIRCLE;
    if (paddata[port].BTN_CROSS)	btns |= PS3_CROSS;
    if (paddata[port].BTN_SQUARE)   btns |= PS3_SQUARE;
    if (paddata[port].BTN_L2)       btns |= PS3_L2;
    if (paddata[port].BTN_R2)       btns |= PS3_R2;
    if (paddata[port].BTN_L1)       btns |= PS3_L1;
    if (paddata[port].BTN_R1)       btns |= PS3_R1;
    if (paddata[port].BTN_L3)       btns |= PS3_L3;
    if (paddata[port].BTN_R3)       btns |= PS3_R3;

	return lastkey[port] = btns;
}
