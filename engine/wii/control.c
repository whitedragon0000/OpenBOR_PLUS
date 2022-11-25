/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2019 OpenBOR Team
 */

// Generic control stuff (keyboard+joystick)

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ogc/lwp_watchdog.h>
#include <wiiuse/wpad.h>
#include <ogc/pad.h>
#include <ogc/system.h>
#include <wupc/wupc.h>
#include "globals.h"
#include "control.h"
#include "openbor.h"
#include "List.h"

/*#define NUNCHUK_STICK_UP       (0x0004 << 16)
#define NUNCHUK_STICK_DOWN     (0x0008 << 16)
#define NUNCHUK_STICK_LEFT     (0x0010 << 16)
#define NUNCHUK_STICK_RIGHT    (0x0020 << 16)*/
#define LEFT_STICK_UP    	-1
#define LEFT_STICK_DOWN  	-2
#define LEFT_STICK_LEFT  	-3
#define LEFT_STICK_RIGHT 	-4
#define LEFT_SUBSTICK_UP    -5
#define LEFT_SUBSTICK_DOWN  -6
#define LEFT_SUBSTICK_LEFT  -7
#define LEFT_SUBSTICK_RIGHT -8

typedef enum {
    DEVICE_TYPE_NONE,
    DEVICE_TYPE_WII_REMOTE,
    DEVICE_TYPE_WIIMOTE_NUNCHUK,
    DEVICE_TYPE_CLASSIC_CONTROLLER,
    DEVICE_TYPE_PRO_CONTROLLER,
    DEVICE_TYPE_GAMECUBE_CONTROLLER,
} DeviceType;

typedef struct {
    DeviceType deviceType;
    char name[CONTROL_DEVICE_NAME_SIZE];
    int mappings[SDID_COUNT];
	int gc_mappings[SDID_COUNT];
    int port;
	int is_gc;
	int rumbling;
	long long unsigned rumble_msec;
	long long unsigned time2rumble;
} InputDevice;

static InputDevice devices[MAX_DEVICES];
static bool controlInited = false;

static int deviceIDs[MAX_PORTS] = {-1, -1, -1, -1};

// if non-null, device is being remapped in the input settings menu
static InputDevice *remapDevice = NULL;
static int remapKeycode = -1;

// each list member is an array of SDID_COUNT ints, dynamically allocated
static List savedMappings;
static bool savedMappingsInited = false;

static const char *deviceTypeNames[] = {
    "None",
    "Wii Remote",
    "Remote+Nunchuk",
    "Classic Controller",
    "Wii U Pro Controller",
    "GameCube Controller",
};

static void handle_events();

// update the mappings for a device in the save data
static void update_saved_mapping(int deviceID)
{
    InputDevice *device = &devices[deviceID];
    if (device->deviceType == DEVICE_TYPE_NONE) return;

    if (List_FindByName(&savedMappings, device->name))
    {
        memcpy(List_Retrieve(&savedMappings), device->mappings, SDID_COUNT * sizeof(int));
    }
    else
    {
        int *mappings = malloc(SDID_COUNT * sizeof(int));
        memcpy(mappings, device->mappings, SDID_COUNT * sizeof(int));
        List_InsertAfter(&savedMappings, mappings, device->name);
    }
}

// set the mappings for a device to the saved settings
static void load_from_saved_mapping(int deviceID)
{
    InputDevice *device = &devices[deviceID];
    if (device->deviceType == DEVICE_TYPE_NONE) return;

    if (List_FindByName(&savedMappings, device->name))
    {
        memcpy(device->mappings, List_Retrieve(&savedMappings), SDID_COUNT * sizeof(int));
    }
    else
    {
        control_resetmappings(deviceID);
    }
}

static void clear_saved_mappings()
{
    if (!savedMappingsInited)
    {
        List_Init(&savedMappings);
        savedMappingsInited = true;
    }

    int numMappings = List_GetSize(&savedMappings);
	List_Reset(&savedMappings);
	for (int i = 0; i < numMappings; i++)
	{
	    free(List_Retrieve(&savedMappings));
        List_GotoNext(&savedMappings);
    }
    List_Clear(&savedMappings);
}

static void setup_device(int deviceID, DeviceType type, const char *name, int port)
{
    devices[deviceID].deviceType = type;
    devices[deviceID].port = port;
    snprintf(devices[deviceID].name, sizeof(devices[deviceID].name), "%s #%i", name, port+1);
    load_from_saved_mapping(deviceID);
    printf("Set up device: %s\n", devices[deviceID].name);
}

void control_init()
{
    if (controlInited) return;

    if (!savedMappingsInited)
    {
        List_Init(&savedMappings);
        savedMappingsInited = true;
    }

    // initialize all devices to DEVICE_TYPE_NONE and all device IDs to -1
    memset(devices, 0, sizeof(devices));
    memset(deviceIDs, 0xff, sizeof(deviceIDs));

    PAD_Init();
	WUPC_Init();
	WPAD_Init();

    handle_events();

    controlInited = true;
}

void control_exit()
{
    if (!controlInited) return;

    clear_saved_mappings();

    for (int i = 0; i < MAX_DEVICES; i++)
    {
        InputDevice *device = &devices[i];
        device->deviceType = DEVICE_TYPE_NONE;
		device->rumbling = 0;
		device->rumble_msec = 0;
    }

    remapDevice = NULL;
    remapKeycode = -1;
    controlInited = false;
}

static void set_default_wiimote_mappings(InputDevice *device)
{
    // up/down/left/right are rotated because the remote is held sideways
    device->mappings[SDID_MOVEUP]     = WPAD_BUTTON_RIGHT;
    device->mappings[SDID_MOVEDOWN]   = WPAD_BUTTON_LEFT;
    device->mappings[SDID_MOVELEFT]   = WPAD_BUTTON_UP;
    device->mappings[SDID_MOVERIGHT]  = WPAD_BUTTON_DOWN;
    device->mappings[SDID_ATTACK]     = WPAD_BUTTON_1;
    device->mappings[SDID_ATTACK2]    = WPAD_BUTTON_A;
    device->mappings[SDID_ATTACK3]    = WPAD_BUTTON_HOME;
    device->mappings[SDID_ATTACK4]    = 0;
    device->mappings[SDID_JUMP]       = WPAD_BUTTON_2;
    device->mappings[SDID_SPECIAL]    = WPAD_BUTTON_B;
    device->mappings[SDID_START]      = WPAD_BUTTON_PLUS;
    device->mappings[SDID_SCREENSHOT] = WPAD_BUTTON_MINUS;
    device->mappings[SDID_ESC]        = WPAD_BUTTON_1;
}

static void set_default_wiimote_nunchuk_mappings(InputDevice *device)
{
    device->mappings[SDID_MOVEUP]     = LEFT_STICK_UP;
    device->mappings[SDID_MOVEDOWN]   = LEFT_STICK_DOWN;
    device->mappings[SDID_MOVELEFT]   = LEFT_STICK_LEFT;
    device->mappings[SDID_MOVERIGHT]  = LEFT_STICK_RIGHT;
    device->mappings[SDID_ATTACK]     = WPAD_BUTTON_A;
    device->mappings[SDID_ATTACK2]    = WPAD_NUNCHUK_BUTTON_C;
    device->mappings[SDID_ATTACK3]    = WPAD_BUTTON_1;
    device->mappings[SDID_ATTACK4]    = WPAD_BUTTON_2;
    device->mappings[SDID_JUMP]       = WPAD_BUTTON_B;
    device->mappings[SDID_SPECIAL]    = WPAD_NUNCHUK_BUTTON_Z;
    device->mappings[SDID_START]      = WPAD_BUTTON_PLUS;
    device->mappings[SDID_SCREENSHOT] = WPAD_BUTTON_MINUS;
    device->mappings[SDID_ESC]        = WPAD_BUTTON_B;
}

static void set_default_classic_controller_mappings(InputDevice *device)
{
    device->mappings[SDID_MOVEUP]     = WPAD_CLASSIC_BUTTON_UP;
    device->mappings[SDID_MOVEDOWN]   = WPAD_CLASSIC_BUTTON_DOWN;
    device->mappings[SDID_MOVELEFT]   = WPAD_CLASSIC_BUTTON_LEFT;
    device->mappings[SDID_MOVERIGHT]  = WPAD_CLASSIC_BUTTON_RIGHT;
    device->mappings[SDID_ATTACK]     = WPAD_CLASSIC_BUTTON_A;
    device->mappings[SDID_ATTACK2]    = WPAD_CLASSIC_BUTTON_Y;
    device->mappings[SDID_ATTACK3]    = WPAD_CLASSIC_BUTTON_FULL_R;
    device->mappings[SDID_ATTACK4]    = WPAD_CLASSIC_BUTTON_FULL_L;
    device->mappings[SDID_JUMP]       = WPAD_CLASSIC_BUTTON_B;
    device->mappings[SDID_SPECIAL]    = WPAD_CLASSIC_BUTTON_X;
    device->mappings[SDID_START]      = WPAD_CLASSIC_BUTTON_PLUS;
    device->mappings[SDID_SCREENSHOT] = WPAD_CLASSIC_BUTTON_MINUS;
    device->mappings[SDID_ESC]        = WPAD_CLASSIC_BUTTON_B;
}

static void set_default_gamecube_controller_mappings(InputDevice *device)
{
    device->gc_mappings[SDID_MOVEUP]     = LEFT_STICK_UP; //PAD_BUTTON_UP;
    device->gc_mappings[SDID_MOVEDOWN]   = LEFT_STICK_DOWN; //PAD_BUTTON_DOWN;
    device->gc_mappings[SDID_MOVELEFT]   = LEFT_STICK_LEFT; //PAD_BUTTON_LEFT;
    device->gc_mappings[SDID_MOVERIGHT]  = LEFT_STICK_RIGHT; //PAD_BUTTON_RIGHT;
    device->gc_mappings[SDID_ATTACK]     = PAD_BUTTON_A;
    device->gc_mappings[SDID_ATTACK2]    = PAD_BUTTON_Y;
    device->gc_mappings[SDID_ATTACK3]    = PAD_TRIGGER_R;
    device->gc_mappings[SDID_ATTACK4]    = PAD_TRIGGER_L;
    device->gc_mappings[SDID_JUMP]       = PAD_BUTTON_B;
    device->gc_mappings[SDID_SPECIAL]    = PAD_BUTTON_X;
    device->gc_mappings[SDID_START]      = PAD_BUTTON_START;
    device->gc_mappings[SDID_SCREENSHOT] = PAD_BUTTON_DOWN; //0x2000;
    device->gc_mappings[SDID_ESC]        = PAD_TRIGGER_Z;
}

void control_resetmappings(int deviceID)
{
    if (deviceID < 0) return;

    InputDevice *device = &devices[deviceID];
	// Controller GC always loaded
	memset(device->gc_mappings, 0, sizeof(device->gc_mappings));
	set_default_gamecube_controller_mappings(device);
	
    switch (device->deviceType)
    {
        /*case DEVICE_TYPE_GAMECUBE_CONTROLLER:
            set_default_gamecube_controller_mappings(device);
			//break;*/
        case DEVICE_TYPE_WII_REMOTE:
            set_default_wiimote_mappings(device);
            break;
        case DEVICE_TYPE_WIIMOTE_NUNCHUK:
            set_default_wiimote_nunchuk_mappings(device);
            break;
        case DEVICE_TYPE_CLASSIC_CONTROLLER:
		case DEVICE_TYPE_PRO_CONTROLLER:
            set_default_classic_controller_mappings(device);
            break;
        default:
            memset(device->mappings, 0, sizeof(device->mappings));
            break;
    }
}

static s32 get_gc_chan(int port)
{
	switch (port)
	{
		case 0: return PAD_CHAN0_BIT;
		case 1: return PAD_CHAN1_BIT;
		case 2: return PAD_CHAN2_BIT;
		case 3: return PAD_CHAN3_BIT;
		default: return PAD_CHAN0_BIT;
	}
}

/*static int is_gc_connected(int port)
{
	PADStatus status[MAX_PORTS];
	PAD_Read(status);
	if (status[port].err == PAD_ERR_NO_CONTROLLER) return 0;
	return 1;
}*/

static DeviceType get_device_type(int expansion, size_t port)
{
    switch (expansion)
    {
        case WPAD_EXP_NUNCHUK:
            return DEVICE_TYPE_WIIMOTE_NUNCHUK;
        case WPAD_EXP_CLASSIC:
            return DEVICE_TYPE_CLASSIC_CONTROLLER;
        case WPAD_EXP_NONE:
        default:
		{
			struct WUPCData *wupc = WUPC_Data(port);
			if (wupc != NULL) return DEVICE_TYPE_PRO_CONTROLLER;
			// Controller GC always loaded
			//else if (is_gc_connected(port)) return DEVICE_TYPE_GAMECUBE_CONTROLLER;
            return DEVICE_TYPE_WII_REMOTE;
		}
    }
}

// handle controller connected/disconnected or Wiimote expansion plugged/unplugged
static void handle_events()
{
    WPAD_ScanPads();
	//PAD_ScanPads(); // Controller GC always loaded

    for (size_t port = 0; port < MAX_PORTS; port++)
    {
        u32 type;
		s32 wii_probe = WPAD_Probe(port, &type);
		
        if (wii_probe == WPAD_ERR_NO_CONTROLLER/* && !is_gc_connected(port)*/) // pad disconnected
        {
            if (devices[port].deviceType != DEVICE_TYPE_NONE)
            if (deviceIDs[port] != -1)
            {
                printf("%s disconnected\n", devices[port].name);
                devices[deviceIDs[port]].deviceType = DEVICE_TYPE_NONE;
                deviceIDs[port] = -1;
            }
        }
        else
        {
            WPADData *wpad = WPAD_Data(port);
            DeviceType newType = get_device_type(wpad->exp.type, port);
            if (deviceIDs[port] == -1) // pad connected
            {
                for (size_t i = 0; i < MAX_DEVICES; i++)
                {
                    if (devices[i].deviceType == DEVICE_TYPE_NONE)
                    {
                        deviceIDs[port] = i;
                        break;
                    }
                }

                // MAX_DEVICES is 32 and there are a maximum of 12 devices supported, so this should be safe
                assert(deviceIDs[port] != -1);
            }

			InputDevice device = devices[deviceIDs[port]];
			if (newType != device.deviceType) // pad connected or expansion type changed
			{
				//device.is_gc = (newType == DEVICE_TYPE_GAMECUBE_CONTROLLER) ? 1 : 0;
				setup_device(deviceIDs[port], newType, deviceTypeNames[newType], port);
			}
        }
    }
}

// Returns 1 if key is pressed, 0 if not
static unsigned int is_key_pressed(InputDevice *device, int keycode)
{
		
    if (device->deviceType == DEVICE_TYPE_WII_REMOTE)
    {
        WPADData *wpad = WPAD_Data(device->port);
        return !!(wpad->btns_h & keycode);
    }
    else if (device->deviceType == DEVICE_TYPE_WIIMOTE_NUNCHUK)
    {
        WPADData *wpad = WPAD_Data(device->port);
        switch (keycode)
        {
            case LEFT_STICK_UP:    return (wpad->exp.nunchuk.js.pos.y >= 0xB0);
            case LEFT_STICK_DOWN:  return (wpad->exp.nunchuk.js.pos.y <= 0x40);
            case LEFT_STICK_LEFT:  return (wpad->exp.nunchuk.js.pos.x <= 0x40);
            case LEFT_STICK_RIGHT: return (wpad->exp.nunchuk.js.pos.x >= 0xB0);
            default:               return !!(wpad->btns_h & keycode);
        }
    }
    else if (device->deviceType == DEVICE_TYPE_CLASSIC_CONTROLLER)
    {
		WPADData *wpad = WPAD_Data(device->port);
		return !!(wpad->btns_h & keycode);
    }
    else if (device->deviceType == DEVICE_TYPE_PRO_CONTROLLER)
    {
		WUPC_UpdateButtonStats();
		struct WUPCData *wupc;
		wupc = WUPC_Data(device->port);
		switch (keycode)
		{
			case LEFT_STICK_UP:    		return (wupc->yAxisL > 200);
			case LEFT_STICK_DOWN:  		return (wupc->yAxisL < -200);
			case LEFT_STICK_LEFT:  		return (wupc->xAxisL < -200);
			case LEFT_STICK_RIGHT: 		return (wupc->xAxisL > 200);
			case LEFT_SUBSTICK_UP:    	return (wupc->yAxisR > 200);
			case LEFT_SUBSTICK_DOWN:  	return (wupc->yAxisR < -200);
			case LEFT_SUBSTICK_LEFT:  	return (wupc->xAxisR < -200);
			case LEFT_SUBSTICK_RIGHT: 	return (wupc->xAxisR > 200);
			default:               		return !!(wupc->button & keycode);
		}
	}
    else if (device->deviceType == DEVICE_TYPE_GAMECUBE_CONTROLLER)
    {
		PAD_ScanPads();
		u16 btns_down = PAD_ButtonsDown(device->port) | PAD_ButtonsHeld(device->port);
		s8 pad_x = PAD_StickX(device->port);
		s8 pad_y = PAD_StickY(device->port);
		s8 spad_x = PAD_SubStickX(device->port);
		s8 spad_y = PAD_SubStickY(device->port);
		
		if (btns_down ||
			(pad_y > 18) || (pad_y < -18) || (pad_x < -18) || (pad_x > 18) ||
			(spad_y > 18) || (spad_y < -18) || (spad_x < -18) || (spad_x > 18))  	device->is_gc = 1;
		else																		device->is_gc = 0;
		
		switch (keycode)
		{
			case LEFT_STICK_UP:    		return (pad_y > 18);
			case LEFT_STICK_DOWN:  		return (pad_y < -18);
			case LEFT_STICK_LEFT:  		return (pad_x < -18);
			case LEFT_STICK_RIGHT: 		return (pad_x > 18);
			case LEFT_SUBSTICK_UP:    	return (spad_y > 18);
			case LEFT_SUBSTICK_DOWN:  	return (spad_y < -18);
			case LEFT_SUBSTICK_LEFT:  	return (spad_x < -18);
			case LEFT_SUBSTICK_RIGHT: 	return (spad_x > 18);
			default:               		return !!(btns_down & keycode);
		}
    }

    return 0;
}

void control_update_player(s_playercontrols *playerControls)
{
    uint32_t keyflags = 0;
    InputDevice *device = &devices[playerControls->deviceID];

	if(device->rumbling)
	{
		long long unsigned msec = 0;
		if(!msec) msec = ticks_to_millisecs(gettime());
		if(msec > device->time2rumble + device->rumble_msec)
		{
			device->rumbling = 0;
			WPAD_Rumble(device->port, 0);
			WUPC_Rumble(device->port, false);
			PAD_ControlMotor(get_gc_chan(device->port), 0);
		}
	}

    for (unsigned int i = 0; i < SDID_COUNT; i++)
    {
		DeviceType tmp_type = device->deviceType;
		device->deviceType = DEVICE_TYPE_GAMECUBE_CONTROLLER; // Controller GC always loaded
		keyflags |= (is_key_pressed(device, device->gc_mappings[i]) << i);
		device->deviceType = tmp_type;
        keyflags |= (is_key_pressed(device, device->mappings[i]) << i);
    }
    playerControls->newkeyflags = keyflags & (~playerControls->keyflags);
    playerControls->keyflags = keyflags;
}

void control_update(s_playercontrols **playerControls, int numPlayers)
{
    handle_events();

    for (int i = 0; i < numPlayers; i++)
    {
        control_update_player(playerControls[i]);
    }
}

void control_remapdevice(int deviceID)
{
    if (deviceID < 0)
    {
        // done remapping; reset globals to default values
        remapDevice = NULL;
        remapKeycode = -1;
    }
    else
    {
        assert(devices[deviceID].deviceType != DEVICE_TYPE_NONE);
        remapDevice = &devices[deviceID];
        remapKeycode = -1;
    }
}

int control_getremappedkey()
{
    return remapKeycode;
}

int *control_getmappings(int deviceID)
{
    return devices[deviceID].mappings;
}

const char *control_getkeyname(int deviceID, int keycode)
{
    if (deviceID < 0) return "None";

    if (devices[deviceID].deviceType == DEVICE_TYPE_WII_REMOTE)
    {
        const char *buttonNames[] = {
            "2",
            "1",
            "B",
            "A",
            "-",
            "???",
            "???",
            "Home",
            "Down",
            "Up",
            "Left",
            "Right",
            "+",
        };

        for (size_t i = 0; i < sizeof(buttonNames) / sizeof(buttonNames[0]); i++)
        {
            if (keycode == (1 << i))
            {
                return buttonNames[i];
            }
        }
    }
    else if (devices[deviceID].deviceType == DEVICE_TYPE_WIIMOTE_NUNCHUK)
    {
        const char *buttonNames[] = {
            "2",
            "1",
            "B",
            "A",
            "-",
            "Home",
            "D-Pad Left",
            "D-Pad Right",
            "D-Pad Down",
            "D-Pad Up",
            "+",
            "???",
            "???",
            "???",
            "Z",
            "C",
        };

        for (size_t i = 0; i < sizeof(buttonNames) / sizeof(buttonNames[0]); i++)
        {
            if (keycode == (1 << i))
            {
                return buttonNames[i];
            }
        }

        // if it's not a button, it's an analog stick direction
        switch (keycode)
        {
            case LEFT_STICK_UP:    return "Analog Stick Up";
            case LEFT_STICK_DOWN:  return "Analog Stick Down";
            case LEFT_STICK_LEFT:  return "Analog Stick Left";
            case LEFT_STICK_RIGHT: return "Analog Stick Right";
        }
    }
    else if (devices[deviceID].deviceType == DEVICE_TYPE_CLASSIC_CONTROLLER || devices[deviceID].deviceType == DEVICE_TYPE_PRO_CONTROLLER)
    {
        const char *buttonNames[] = {
            "D-Pad Up",
            "D-Pad Left",
            "ZR",
            "X",
            "A",
            "Y",
            "B",
            "ZL",
            "R",
            "+",
            "Home",
            "-",
            "L",
            "Down",
            "Right",
        };

        for (size_t i = 0; i < sizeof(buttonNames) / sizeof(buttonNames[0]); i++)
        {
            if (keycode == (0x10000 << i))
            {
                return buttonNames[i];
            }
        }
		
		if (devices[deviceID].deviceType == DEVICE_TYPE_PRO_CONTROLLER)
		{
			// if it's not a button, it's an analog stick direction
			switch (keycode)
			{
				case LEFT_STICK_UP:    return "Analog L-Stick Up";
				case LEFT_STICK_DOWN:  return "Analog L-Stick Down";
				case LEFT_STICK_LEFT:  return "Analog L-Stick Left";
				case LEFT_STICK_RIGHT: return "Analog L-Stick Right";
				case LEFT_SUBSTICK_UP:    return "Analog R-Stick Up";
				case LEFT_SUBSTICK_DOWN:  return "Analog R-Stick Down";
				case LEFT_SUBSTICK_LEFT:  return "Analog R-Stick Left";
				case LEFT_SUBSTICK_RIGHT: return "Analog R-Stick Right";
			}
		}
    }
	else if (devices[deviceID].deviceType == DEVICE_TYPE_GAMECUBE_CONTROLLER)
    {
        const char *buttonNames[] = {
            "D-Pad Left",
            "D-Pad Right",
            "D-Pad Down",
            "D-Pad Up",
            "Trigger Z",
            "Trigger R",
            "Trigger L",
            "A",
            "B",
            "X",
            "Y",
            "Menu",
            "Start",
            "???",
            "???",
            "???",
        };

        for (size_t i = 0; i < sizeof(buttonNames) / sizeof(buttonNames[0]); i++)
        {
            if (keycode == (1 << i))
            {
                return buttonNames[i];
            }
        }

        // if it's not a button, it's an analog stick direction
        switch (keycode)
        {
            case LEFT_STICK_UP:    return "Analog Stick Up";
            case LEFT_STICK_DOWN:  return "Analog Stick Down";
            case LEFT_STICK_LEFT:  return "Analog Stick Left";
            case LEFT_STICK_RIGHT: return "Analog Stick Right";
			case LEFT_SUBSTICK_UP:    return "Analog C-Stick Up";
            case LEFT_SUBSTICK_DOWN:  return "Analog C-Stick Down";
            case LEFT_SUBSTICK_LEFT:  return "Analog C-Stick Left";
            case LEFT_SUBSTICK_RIGHT: return "Analog C-Stick Right";
        }
    }

    return "None";
}

bool control_isvaliddevice(int deviceID)
{
    return deviceID >= 0 && devices[deviceID].deviceType != DEVICE_TYPE_NONE;
}

const char *control_getdevicename(int deviceID)
{
    return devices[deviceID].deviceType == DEVICE_TYPE_NONE ? "None" : devices[deviceID].name;
}

void control_rumble(s_playercontrols ** playercontrols, int player, int ratio, int msec)
{
	s_playercontrols * pcontrols = playercontrols[player];
	int deviceID = pcontrols->deviceID;
	InputDevice *device = &devices[deviceID];
	
	WPADData *wpad;
	struct WUPCData *wupc;
	wpad = WPAD_Data(device->port);
	wupc = WUPC_Data(device->port);

	device->rumbling = 1;
	device->rumble_msec = msec * 3;
	device->time2rumble = ticks_to_millisecs(gettime());

	if (device->is_gc)                     			PAD_ControlMotor(get_gc_chan(device->port), 1);
	else if(wupc != NULL)							WUPC_Rumble(device->port, true);
	else if (wpad->exp.type != WPAD_EXP_CLASSIC)    WPAD_Rumble(device->port, 1);
}

#define MAPPINGS_FILE_SENTINEL 0x9cf232d4

bool control_loadmappings(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        return false;
    }

    clear_saved_mappings();

    while (!feof(fp) && !ferror(fp))
    {
        char name[CONTROL_DEVICE_NAME_SIZE];
		int *mapping = malloc(SDID_COUNT * sizeof(int));
		int sentinel;
        if (fread(name, 1, sizeof(name), fp) != sizeof(name) ||
            fread(mapping, sizeof(int), SDID_COUNT, fp) != SDID_COUNT ||
            fread(&sentinel, sizeof(int), 1, fp) != 1)
        {
            free(mapping);
            break;
        }
        else if (sentinel != MAPPINGS_FILE_SENTINEL)
        {
            free(mapping);
            fclose(fp);
            return false;
        }

        name[sizeof(name)-1] = '\0'; // just in case
        printf("Loaded mapping for %s\n", name);
        List_InsertAfter(&savedMappings, mapping, name);
    }

    fclose(fp);

    // update all current device mappings with the newly loaded mappings
    for (int i = 0; i < MAX_DEVICES; i++)
    {
        if (devices[i].deviceType != DEVICE_TYPE_NONE)
        {
            load_from_saved_mapping(i);
        }
    }

    return true;
}

bool control_savemappings(const char *filename)
{
    // update savedMappings with all current device mappings
    for (int i = 0; i < MAX_DEVICES; i++)
    {
        if (devices[i].deviceType != DEVICE_TYPE_NONE)
        {
            update_saved_mapping(i);
        }
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        return false;
    }

    int numMappings = List_GetSize(&savedMappings);
	List_Reset(&savedMappings);
	for (int i = 0; i < numMappings; i++)
	{
		char name[CONTROL_DEVICE_NAME_SIZE];
		snprintf(name, sizeof(name), "%s", List_GetName(&savedMappings));
		int *mapping = List_Retrieve(&savedMappings);
		const int sentinel = MAPPINGS_FILE_SENTINEL;
        if (fwrite(name, 1, sizeof(name), fp) != sizeof(name) ||
            fwrite(mapping, sizeof(int), SDID_COUNT, fp) != SDID_COUNT ||
            fwrite(&sentinel, sizeof(int), 1, fp) != 1)
        {
            fclose(fp);
            return false;
        }

        List_GotoNext(&savedMappings);
    }

    fclose(fp);
    return true;
}

void control_clearmappings()
{
    clear_saved_mappings();

    for (int i = 0; i < MAX_DEVICES; i++)
    {
        if (devices[i].deviceType != DEVICE_TYPE_NONE)
        {
            control_resetmappings(i);
        }
    }
}

