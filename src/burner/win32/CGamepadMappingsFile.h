#ifndef CGAMEPADMAPPINGSFILE_H
#define CGAMEPADMAPPINGSFILE_H

// #include "burn.h"
#include "GUIDComparer.h"

#define MAX_BUTTONS 16
#define MAX_DPAD	4
#define MAX_STICKS	2
#define MAX_TRIGGERS 2

// From 'burn.h'
typedef unsigned int						UINT32;

struct CGamepadButtonIndex {
	bool FoundMatch;

	UINT32 Buttons[MAX_BUTTONS];
	UINT32 ButtonCount = 0;

	UINT32 Sticks[MAX_STICKS];
	UINT32 StickCount = 0;

	UINT32 Triggers[MAX_TRIGGERS];
	UINT32 TriggerCount = 0;
};

// Represents access to a database of gamepad mappings.
// Because the indexes, etc. of the buttons that are represented in DirectInput/XInput
// are not the same across all devices, we need a way to choose a consistent layout
// for any old device that we might want to plug in.
class CGamepadMappingsFile {

public:
	
	// Populates 'index' with the button index for the device with a matching 'productGuid' entry.  If
	// productGuid doesn't match no index data will be populated. 
	void GetButtonIndex(GUID& productGuid, CGamepadButtonIndex& index);


private:



};
















#endif