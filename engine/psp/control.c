/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <pspkernel.h>
#include <pspctrl.h>
#include "pspport.h"
#include "control.h"
#include "control/control.h"
#include "openbor.h"

#define	MAX_PADS			4

typedef enum {
    DEVICE_TYPE_NONE,
    DEVICE_TYPE_STANDARD_CONTROLLER,
} DeviceType;

typedef struct {
    DeviceType deviceType;
    char name[CONTROL_DEVICE_NAME_SIZE];
    int mappings[SDID_COUNT];
    int port;
} InputDevice;

static InputDevice devices[MAX_DEVICES];
static bool controlInited = false;

static int controllerIDs[4] = {-1, -1, -1, -1};

// if non-null, device is being remapped in the input settings menu
static InputDevice *remapDevice = NULL;
static int remapKeycode = -1;

// each list member is an array of SDID_COUNT ints, dynamically allocated
static List savedMappings;
static bool savedMappingsInited = false;

static const char *deviceTypeNames[] = {
    "None",
    "Standard Controller",
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
    memset(controllerIDs, 0xff, sizeof(controllerIDs));

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
    }

    remapDevice = NULL;
    remapKeycode = -1;
    controlInited = false;
}

static void set_default_standard_controller_mappings(InputDevice *device)
{
    device->mappings[SDID_MOVEUP]     = PSP_DPAD_UP;
    device->mappings[SDID_MOVEDOWN]   = PSP_DPAD_DOWN;
    device->mappings[SDID_MOVELEFT]   = PSP_DPAD_LEFT;
    device->mappings[SDID_MOVERIGHT]  = PSP_DPAD_RIGHT;
    device->mappings[SDID_ATTACK]     = PSP_SQUARE;
    device->mappings[SDID_ATTACK2]    = PSP_CIRCLE;
    device->mappings[SDID_ATTACK3]    = PSP_LEFT_TRIGGER;
    device->mappings[SDID_ATTACK4]    = PSP_RIGHT_TRIGGER;
    device->mappings[SDID_JUMP]       = PSP_CROSS;
    device->mappings[SDID_SPECIAL]    = PSP_TRIANGLE;
    device->mappings[SDID_START]      = PSP_START;
    device->mappings[SDID_SCREENSHOT] = PSP_SELECT;
    device->mappings[SDID_ESC]        = PSP_HOME;
}

void control_resetmappings(int deviceID)
{
    if (deviceID < 0) return;

    InputDevice *device = &devices[deviceID];
    switch (device->deviceType)
    {
        case DEVICE_TYPE_STANDARD_CONTROLLER:
            set_default_standard_controller_mappings(device);
            break;
        default:
            memset(device->mappings, 0, sizeof(device->mappings));
            break;
    }
}

static void handle_events()
{
	if (!controlInited)
	{
		int port = 0;
		DeviceType newType = DEVICE_TYPE_STANDARD_CONTROLLER;
		
		if (controllerIDs[port] == -1)
		{
			for (size_t i = 0; i < MAX_DEVICES; i++)
			{
				if (devices[i].deviceType == DEVICE_TYPE_NONE)
				{
					controllerIDs[port] = i;
					break;
				}
			}

			// MAX_DEVICES is 32 and there are a maximum of 12 devices supported, so this should be safe
			assert(controllerIDs[port] != -1);
		}
		
		if (newType != devices[controllerIDs[port]].deviceType) // controller connected or expansion type changed
		{
			setup_device(controllerIDs[port], newType, deviceTypeNames[newType], port);
		}
	}
}

unsigned int getPad(int port)
{
	unsigned long btns = 0;
	SceCtrlData data;
	getCtrlData(&data);

	if (port != 0) return 0;
	   
	if (data.Ly >= PAD_HIGH_BOUND)	 btns |= PSP_DPAD_DOWN;
	if (data.Ly <= PAD_LOW_BOUND)    btns |= PSP_DPAD_UP;
	if (data.Lx <= PAD_LOW_BOUND)    btns |= PSP_DPAD_LEFT;
	if (data.Lx >= PAD_HIGH_BOUND)   btns |= PSP_DPAD_RIGHT;
	   
	if (data.Buttons & PSP_CTRL_SELECT)   btns |= PSP_SELECT;
	if (data.Buttons & PSP_CTRL_START)    btns |= PSP_START;
	if (data.Buttons & PSP_CTRL_UP)	      btns |= PSP_DPAD_UP;
	if (data.Buttons & PSP_CTRL_RIGHT)    btns |= PSP_DPAD_RIGHT;
	if (data.Buttons & PSP_CTRL_DOWN)     btns |= PSP_DPAD_DOWN;
	if (data.Buttons & PSP_CTRL_LEFT)     btns |= PSP_DPAD_LEFT;
	if (data.Buttons & PSP_CTRL_LTRIGGER) btns |= PSP_LEFT_TRIGGER;
	if (data.Buttons & PSP_CTRL_RTRIGGER) btns |= PSP_RIGHT_TRIGGER;
	if (data.Buttons & PSP_CTRL_TRIANGLE) btns |= PSP_TRIANGLE;
	if (data.Buttons & PSP_CTRL_CIRCLE)   btns |= PSP_CIRCLE;
	if (data.Buttons & PSP_CTRL_CROSS)	  btns |= PSP_CROSS;
	if (data.Buttons & PSP_CTRL_SQUARE)   btns |= PSP_SQUARE;
	if (data.Buttons & PSP_CTRL_NOTE)     btns |= PSP_NOTE;
	if (data.Buttons & PSP_CTRL_HOME)     btns |= PSP_HOME;
	if (data.Buttons & PSP_CTRL_HOLD)     btns |= PSP_HOLD;
	if (data.Buttons & PSP_CTRL_SCREEN)   btns |= PSP_SCREEN;
	if (data.Buttons & PSP_CTRL_VOLUP)    btns |= PSP_VOLUP;
	if (data.Buttons & PSP_CTRL_VOLDOWN)  btns |= PSP_VOLDOWN;
	   
	if (btns & PSP_HOME && btns & PSP_START) borExit(-1);

	return btns;
}

// Returns 1 if key is pressed, 0 if not
static unsigned int is_key_pressed(InputDevice *device, int keycode)
{
	int port = device->port;
	
	if(port != 0) return 0;
	
	if (device->deviceType == DEVICE_TYPE_STANDARD_CONTROLLER)
    {
		SceCtrlData data;
		getCtrlData(&data);
		
		switch (keycode)
		{
			case PSP_DPAD_UP:    		return (data.Ly <= PAD_LOW_BOUND)  | !!(data.Buttons & PSP_CTRL_UP);
			case PSP_DPAD_DOWN:  		return (data.Ly >= PAD_HIGH_BOUND) | !!(data.Buttons & PSP_CTRL_DOWN);
			case PSP_DPAD_LEFT:  		return (data.Lx <= PAD_LOW_BOUND)  | !!(data.Buttons & PSP_CTRL_LEFT);
			case PSP_DPAD_RIGHT: 		return (data.Lx >= PAD_HIGH_BOUND) | !!(data.Buttons & PSP_CTRL_RIGHT);
			case PSP_SELECT: 			return !!(data.Buttons & PSP_CTRL_SELECT);
			case PSP_START: 			return !!(data.Buttons & PSP_CTRL_START);
			case PSP_SQUARE: 			return !!(data.Buttons & PSP_CTRL_SQUARE);
			case PSP_TRIANGLE: 			return !!(data.Buttons & PSP_CTRL_TRIANGLE);
			case PSP_CROSS: 			return !!(data.Buttons & PSP_CTRL_CROSS);
			case PSP_CIRCLE: 			return !!(data.Buttons & PSP_CTRL_CIRCLE);
			case PSP_LEFT_TRIGGER: 		return !!(data.Buttons & PSP_CTRL_LTRIGGER);
			case PSP_RIGHT_TRIGGER: 	return !!(data.Buttons & PSP_CTRL_RTRIGGER);
			case PSP_NOTE: 				return !!(data.Buttons & PSP_CTRL_NOTE);
			case PSP_HOME: 				return !!(data.Buttons & PSP_CTRL_HOME);
			case PSP_HOLD: 				return !!(data.Buttons & PSP_CTRL_HOLD);
			case PSP_SCREEN: 			return !!(data.Buttons & PSP_CTRL_SCREEN);
			case PSP_VOLUP: 			return !!(data.Buttons & PSP_CTRL_VOLUP);
			case PSP_VOLDOWN: 			return !!(data.Buttons & PSP_CTRL_VOLDOWN);
			default:             		return 0;
		}
    }

    return 0;
}

const char *control_getkeyname(int deviceID, int keycode)
{
    if (deviceID < 0) return "None";

	switch (keycode)
	{
		case PSP_DPAD_UP:    		return "Up";
		case PSP_DPAD_DOWN:  		return "Down";
		case PSP_DPAD_LEFT:  		return "Left";
		case PSP_DPAD_RIGHT: 		return "Right";
		case PSP_SELECT: 			return "Select";
		case PSP_START: 			return "Start";
		case PSP_SQUARE: 			return "[]";
		case PSP_TRIANGLE: 			return "/\\";
		case PSP_CROSS: 			return "X";
		case PSP_CIRCLE: 			return "O";
		case PSP_LEFT_TRIGGER: 		return "L-Trigger";
		case PSP_RIGHT_TRIGGER: 	return "R-Trigger";
		case PSP_NOTE: 				return "Note";
		case PSP_HOME: 				return "Home";
		case PSP_HOLD: 				return "Hold";
		case PSP_SCREEN: 			return "Screen";
		case PSP_VOLUP: 			return "Volume Up";
		case PSP_VOLDOWN: 			return "Volume Down";
	}

    return "None";
}

void control_update_player(s_playercontrols *playerControls)
{
    uint32_t keyflags = 0;
    InputDevice *device = &devices[playerControls->deviceID];

    for (unsigned int i = 0; i < SDID_COUNT; i++)
    {
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

