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

static padData paddata[MAX_DEVICES];

typedef enum {
    DEVICE_TYPE_NONE,
    DEVICE_TYPE_STANDARD_CONTROLLER,
	DEVICE_TYPE_REMOTE_CONTROLLER,
	DEVICE_TYPE_LDD_CONTROLLER,
} DeviceType;

typedef struct {
    DeviceType deviceType;
    char name[CONTROL_DEVICE_NAME_SIZE];
    int mappings[SDID_COUNT];
    int port;
} InputDevice;

static InputDevice devices[MAX_DEVICES];
static bool controlInited = false;

static int controllerIDs[MAX_PORTS] = {-1, -1, -1, -1};

// if non-null, device is being remapped in the input settings menu
static InputDevice *remapDevice = NULL;
static int remapKeycode = -1;

// each list member is an array of SDID_COUNT ints, dynamically allocated
static List savedMappings;
static bool savedMappingsInited = false;

static const char *deviceTypeNames[] = {
    "None",
    "Standard Controller",
	"Remote Controller",
	"LDD Controller",
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

    ioPadInit(MAX_DEVICES);

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
	
	ioPadEnd();
}

static void set_default_standard_controller_mappings(InputDevice *device)
{
    device->mappings[SDID_MOVEUP]     = PS3_DPAD_UP;
    device->mappings[SDID_MOVEDOWN]   = PS3_DPAD_DOWN;
    device->mappings[SDID_MOVELEFT]   = PS3_DPAD_LEFT;
    device->mappings[SDID_MOVERIGHT]  = PS3_DPAD_RIGHT;
    device->mappings[SDID_ATTACK]     = PS3_SQUARE;
    device->mappings[SDID_ATTACK2]    = PS3_CIRCLE;
    device->mappings[SDID_ATTACK3]    = PS3_L1;
    device->mappings[SDID_ATTACK4]    = PS3_R1;
    device->mappings[SDID_JUMP]       = PS3_CROSS;
    device->mappings[SDID_SPECIAL]    = PS3_TRIANGLE;
    device->mappings[SDID_START]      = PS3_START;
    device->mappings[SDID_SCREENSHOT] = PS3_R3;
    device->mappings[SDID_ESC]        = PS3_L3;
}

static void set_default_remote_controller_mappings(InputDevice *device)
{
    device->mappings[SDID_MOVEUP]     = BTN_BD_UP;
    device->mappings[SDID_MOVEDOWN]   = BTN_BD_DOWN;
    device->mappings[SDID_MOVELEFT]   = BTN_BD_LEFT;
    device->mappings[SDID_MOVERIGHT]  = BTN_BD_RIGHT;
    device->mappings[SDID_ATTACK]     = BTN_BD_SQUARE;
    device->mappings[SDID_ATTACK2]    = BTN_BD_CIRCLE;
    device->mappings[SDID_ATTACK3]    = BTN_BD_L1;
    device->mappings[SDID_ATTACK4]    = BTN_BD_R1;
    device->mappings[SDID_JUMP]       = BTN_BD_CROSS;
    device->mappings[SDID_SPECIAL]    = BTN_BD_TRIANGLE;
    device->mappings[SDID_START]      = BTN_BD_START;
    device->mappings[SDID_SCREENSHOT] = BTN_BD_R3;
    device->mappings[SDID_ESC]        = BTN_BD_L3;
}

static void set_default_ldd_controller_mappings(InputDevice *device)
{
    device->mappings[SDID_MOVEUP]     = PS3_DPAD_UP;
    device->mappings[SDID_MOVEDOWN]   = PS3_DPAD_DOWN;
    device->mappings[SDID_MOVELEFT]   = PS3_DPAD_LEFT;
    device->mappings[SDID_MOVERIGHT]  = PS3_DPAD_RIGHT;
    device->mappings[SDID_ATTACK]     = PS3_SQUARE;
    device->mappings[SDID_ATTACK2]    = PS3_CIRCLE;
    device->mappings[SDID_ATTACK3]    = PS3_L1;
    device->mappings[SDID_ATTACK4]    = PS3_R1;
    device->mappings[SDID_JUMP]       = PS3_CROSS;
    device->mappings[SDID_SPECIAL]    = PS3_TRIANGLE;
    device->mappings[SDID_START]      = PS3_START;
    device->mappings[SDID_SCREENSHOT] = PS3_R3;
    device->mappings[SDID_ESC]        = PS3_L3;
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
        case DEVICE_TYPE_REMOTE_CONTROLLER:
            set_default_remote_controller_mappings(device);
            break;
        case DEVICE_TYPE_LDD_CONTROLLER:
            set_default_ldd_controller_mappings(device);
            break;
        default:
            memset(device->mappings, 0, sizeof(device->mappings));
            break;
    }
}

static DeviceType device_type_for_expansion_type(int expansion)
{
    switch (expansion)
    {
        case PAD_TYPE_STANDARD:
            return DEVICE_TYPE_STANDARD_CONTROLLER;
        case PAD_TYPE_REMOTE:
            return DEVICE_TYPE_REMOTE_CONTROLLER;
        case PAD_TYPE_LDD:
			return DEVICE_TYPE_LDD_CONTROLLER;
        default:
            return DEVICE_TYPE_STANDARD_CONTROLLER;
    }
}

// handle controller connected/disconnected
static void handle_events()
{
    for (size_t port = 0; port < MAX_PORTS; port++)
    {
		padInfo2 padinfo2;
		
		ioPadGetInfo2(&padinfo2);

        if (!padinfo2.port_status[port]) // controller disconnected
        {
            if (devices[port].deviceType != DEVICE_TYPE_NONE)
            if (controllerIDs[port] != -1)
            {
                printf("%s disconnected\n", devices[port].name);
                devices[controllerIDs[port]].deviceType = DEVICE_TYPE_NONE;
                controllerIDs[port] = -1;
            }
        }
        else
        {
			DeviceType newType = device_type_for_expansion_type(padinfo2.device_type[port]);
			
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
}

unsigned int getPad(int port)
{
	unsigned int btns = 0;
	padInfo2 padinfo2;
	
    ioPadGetData(port, &paddata[port]);
	ioPadGetInfo2(&padinfo2);

	u32 type = padinfo2.device_type[port];

            if (paddata[port].ANA_L_V > ANAG_STAND + PAD_STICK_DEADZONE)   btns |= PS3_DPAD_DOWN;
    else if (paddata[port].ANA_L_V < ANAG_STAND - PAD_STICK_DEADZONE)   btns |= PS3_DPAD_UP;
            if (paddata[port].ANA_L_H < ANAG_STAND - PAD_STICK_DEADZONE)   btns |= PS3_DPAD_LEFT;
    else if (paddata[port].ANA_L_H > ANAG_STAND + PAD_STICK_DEADZONE)   btns |= PS3_DPAD_RIGHT;

	if (type == PAD_TYPE_REMOTE)
	{	
		if (paddata[port].BTN_BDCODE & BTN_BD_SELECT)   btns |= PS3_SELECT;
		if (paddata[port].BTN_BDCODE & BTN_BD_START)    btns |= PS3_START;
		if (paddata[port].BTN_BDCODE & BTN_BD_UP)	    btns |= PS3_DPAD_UP;
		if (paddata[port].BTN_BDCODE & BTN_BD_RIGHT)    btns |= PS3_DPAD_RIGHT;
		if (paddata[port].BTN_BDCODE & BTN_BD_DOWN)     btns |= PS3_DPAD_DOWN;
		if (paddata[port].BTN_BDCODE & BTN_BD_LEFT)     btns |= PS3_DPAD_LEFT;
		if (paddata[port].BTN_BDCODE & BTN_BD_TRIANGLE) btns |= PS3_TRIANGLE;
		if (paddata[port].BTN_BDCODE & BTN_BD_CIRCLE)   btns |= PS3_CIRCLE;
		if (paddata[port].BTN_BDCODE & BTN_BD_CROSS)	btns |= PS3_CROSS;
		if (paddata[port].BTN_BDCODE & BTN_BD_SQUARE)   btns |= PS3_SQUARE;
		if (paddata[port].BTN_BDCODE & BTN_BD_L2)       btns |= PS3_L2;
		if (paddata[port].BTN_BDCODE & BTN_BD_R2)       btns |= PS3_R2;
		if (paddata[port].BTN_BDCODE & BTN_BD_L1)       btns |= PS3_L1;
		if (paddata[port].BTN_BDCODE & BTN_BD_R1)       btns |= PS3_R1;
		if (paddata[port].BTN_BDCODE & BTN_BD_L3)       btns |= PS3_L3;
		if (paddata[port].BTN_BDCODE & BTN_BD_R3)       btns |= PS3_R3;
	}
    else// if (type == PAD_TYPE_STANDARD || type == PAD_TYPE_LDD)
	{
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
	}

	return btns;
}

// Returns 1 if key is pressed, 0 if not
static unsigned int is_key_pressed(InputDevice *device, int keycode)
{
    if (device->deviceType == DEVICE_TYPE_STANDARD_CONTROLLER
			|| device->deviceType == DEVICE_TYPE_LDD_CONTROLLER)
    {
		int port = device->port;
        ioPadGetData(port, &paddata[port]);
        switch (keycode)
        {
            case PS3_DPAD_UP:    	return (paddata[port].ANA_L_V < ANAG_STAND - PAD_STICK_DEADZONE) | !!paddata[port].BTN_UP;
            case PS3_DPAD_DOWN:  	return (paddata[port].ANA_L_V > ANAG_STAND + PAD_STICK_DEADZONE) | !!paddata[port].BTN_DOWN;
            case PS3_DPAD_LEFT:  	return (paddata[port].ANA_L_H < ANAG_STAND - PAD_STICK_DEADZONE) | !!paddata[port].BTN_LEFT;
            case PS3_DPAD_RIGHT: 	return (paddata[port].ANA_L_H > ANAG_STAND + PAD_STICK_DEADZONE) | !!paddata[port].BTN_RIGHT;
			case PS3_SELECT: 		return !!paddata[port].BTN_SELECT;
			case PS3_START: 		return !!paddata[port].BTN_START;
			case PS3_SQUARE: 		return !!paddata[port].BTN_SQUARE;
			case PS3_TRIANGLE: 		return !!paddata[port].BTN_TRIANGLE;
			case PS3_CROSS: 		return !!paddata[port].BTN_CROSS;
			case PS3_CIRCLE: 		return !!paddata[port].BTN_CIRCLE;
			case PS3_L1: 			return !!paddata[port].BTN_L1;
			case PS3_L2: 			return !!paddata[port].BTN_L2;
			case PS3_L3: 			return !!paddata[port].BTN_L3;
			case PS3_R1: 			return !!paddata[port].BTN_R1;
			case PS3_R2: 			return !!paddata[port].BTN_R2;
			case PS3_R3: 			return !!paddata[port].BTN_R3;
            default:             	return 0;
        }
    }
	else if (device->deviceType == DEVICE_TYPE_REMOTE_CONTROLLER)
	{
		padData c_paddata = paddata[device->port];
		return !!(c_paddata.BTN_BDCODE & keycode);
	}

    return 0;
}

const char *control_getkeyname(int deviceID, int keycode)
{
    if (deviceID < 0) return "None";

    if (devices[deviceID].deviceType == DEVICE_TYPE_STANDARD_CONTROLLER
			|| devices[deviceID].deviceType == DEVICE_TYPE_LDD_CONTROLLER)
    {
        switch (keycode)
        {
            case PS3_DPAD_UP:    	return "Up";
            case PS3_DPAD_DOWN:  	return "Down";
            case PS3_DPAD_LEFT:  	return "Left";
            case PS3_DPAD_RIGHT: 	return "Right";
			case PS3_SELECT: 		return "Select";
			case PS3_START: 		return "Start";
			case PS3_SQUARE: 		return "Square";
			case PS3_TRIANGLE: 		return "Triangle";
			case PS3_CROSS: 		return "Cross";
			case PS3_CIRCLE: 		return "Circle";
			case PS3_L1: 			return "L1";
			case PS3_L2: 			return "L2";
			case PS3_L3: 			return "L3";
			case PS3_R1: 			return "R1";
			case PS3_R2: 			return "R2";
			case PS3_R3: 			return "R3";
        }
    }
	else if (devices[deviceID].deviceType == DEVICE_TYPE_REMOTE_CONTROLLER)
	{
        switch (keycode)
        {
            case BTN_BD_UP:    		return "Up";
            case BTN_BD_DOWN:  		return "Down";
            case BTN_BD_LEFT:  		return "Left";
            case BTN_BD_RIGHT: 		return "Right";
			case BTN_BD_SELECT: 	return "Select";
			case BTN_BD_START: 		return "Start";
			case BTN_BD_SQUARE: 	return "Square";
			case BTN_BD_TRIANGLE: 	return "Triangle";
			case BTN_BD_CROSS: 		return "Cross";
			case BTN_BD_CIRCLE: 	return "Circle";
			case BTN_BD_L1: 		return "L1";
			case BTN_BD_L2: 		return "L2";
			case BTN_BD_L3: 		return "L3";
			case BTN_BD_R1: 		return "R1";
			case BTN_BD_R2: 		return "R2";
			case BTN_BD_R3: 		return "R3";
        }
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
    padActParam actparam;
    actparam.small_motor = 1;
    actparam.large_motor = (255 * ratio) / 100;
    ioPadSetActDirect(player, &actparam);
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
