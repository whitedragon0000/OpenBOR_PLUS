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

#define	MAX_BOR_PADS		        4
#define MAX_INPUT                   32
#define BTN_NUM                     16

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

#define	CONTROL_DEFAULT1_UP			1
#define	CONTROL_DEFAULT1_RIGHT		2
#define	CONTROL_DEFAULT1_DOWN		3
#define	CONTROL_DEFAULT1_LEFT		4
#define CONTROL_DEFAULT1_FIRE1		5
#define CONTROL_DEFAULT1_FIRE2		6
#define	CONTROL_DEFAULT1_FIRE3		13
#define	CONTROL_DEFAULT1_FIRE4		14
#define	CONTROL_DEFAULT1_FIRE5		7
#define	CONTROL_DEFAULT1_FIRE6		8
#define CONTROL_DEFAULT1_START		11
#define CONTROL_DEFAULT1_SCREENSHOT 16

#define	CONTROL_DEFAULT2_UP			(1+BTN_NUM)
#define	CONTROL_DEFAULT2_RIGHT		(2+BTN_NUM)
#define	CONTROL_DEFAULT2_DOWN		(3+BTN_NUM)
#define	CONTROL_DEFAULT2_LEFT		(4+BTN_NUM)
#define CONTROL_DEFAULT2_FIRE1		(5+BTN_NUM)
#define CONTROL_DEFAULT2_FIRE2		(6+BTN_NUM)
#define	CONTROL_DEFAULT2_FIRE3		(13+BTN_NUM)
#define	CONTROL_DEFAULT2_FIRE4		(14+BTN_NUM)
#define	CONTROL_DEFAULT2_FIRE5		(7+BTN_NUM)
#define	CONTROL_DEFAULT2_FIRE6		(8+BTN_NUM)
#define CONTROL_DEFAULT2_START		(11+BTN_NUM)
#define CONTROL_DEFAULT2_SCREENSHOT (16+BTN_NUM)

#define	CONTROL_DEFAULT3_UP			(1+(BTN_NUM*2))
#define	CONTROL_DEFAULT3_RIGHT		(2+(BTN_NUM*2))
#define	CONTROL_DEFAULT3_DOWN		(3+(BTN_NUM*2))
#define	CONTROL_DEFAULT3_LEFT		(4+(BTN_NUM*2))
#define CONTROL_DEFAULT3_FIRE1		(5+(BTN_NUM*2))
#define CONTROL_DEFAULT3_FIRE2		(6+(BTN_NUM*2))
#define	CONTROL_DEFAULT3_FIRE3		(13+(BTN_NUM*2))
#define	CONTROL_DEFAULT3_FIRE4		(14+(BTN_NUM*2))
#define	CONTROL_DEFAULT3_FIRE5		(7+(BTN_NUM*2))
#define	CONTROL_DEFAULT3_FIRE6		(8+(BTN_NUM*2))
#define CONTROL_DEFAULT3_START		(11+(BTN_NUM*2))
#define CONTROL_DEFAULT3_SCREENSHOT (16+(BTN_NUM*2))

#define	CONTROL_DEFAULT4_UP			(1+(BTN_NUM*3))
#define	CONTROL_DEFAULT4_RIGHT		(2+(BTN_NUM*3))
#define	CONTROL_DEFAULT4_DOWN		(3+(BTN_NUM*3))
#define	CONTROL_DEFAULT4_LEFT		(4+(BTN_NUM*3))
#define CONTROL_DEFAULT4_FIRE1		(5+(BTN_NUM*3))
#define CONTROL_DEFAULT4_FIRE2		(6+(BTN_NUM*3))
#define	CONTROL_DEFAULT4_FIRE3		(13+(BTN_NUM*3))
#define	CONTROL_DEFAULT4_FIRE4		(14+(BTN_NUM*3))
#define	CONTROL_DEFAULT4_FIRE5		(7+(BTN_NUM*3))
#define	CONTROL_DEFAULT4_FIRE6		(8+(BTN_NUM*3))
#define CONTROL_DEFAULT4_START		(11+(BTN_NUM*3))
#define CONTROL_DEFAULT4_SCREENSHOT (16+(BTN_NUM*3))

typedef struct
{
	int	settings[MAX_INPUT];
	unsigned int keyflags, newkeyflags;
	int kb_break;
}
s_playercontrols;

void control_exit();
void control_init(int joy_enable);
int control_usejoy(int enable);
int control_getjoyenabled();
int keyboard_getlastkey();
void control_setkey(s_playercontrols * pcontrols, unsigned int flag, int key);
int control_scankey();
char* control_getkeyname(unsigned int keycode);
void control_update(s_playercontrols ** playercontrols, int numplayers);
void control_rumble(int port, int ratio, int msec);
unsigned int getPad(int port);

#endif
