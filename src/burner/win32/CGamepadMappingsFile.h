#ifndef CGAMEPADMAPPINGSFILE_H
#define CGAMEPADMAPPINGSFILE_H

// From 'burn.h'
// Note that these are kind of cut-pasted all over the place.
typedef unsigned char						UINT8;
typedef signed char 						INT8;
typedef unsigned short						UINT16;
typedef signed short						INT16;
typedef unsigned int						UINT32;
typedef signed int							INT32;


#include "GUIDComparer.h"
#include "gameinp.h"

#define MAX_ENTRY 32
//
//#define MAX_BUTTONS			16
//#define MAX_DPAD_INPUTS		4		// 4 inputs, i.e. up, down, left, right
//#define MAX_STICK_INPUTS	8		// 8 inputs = 2 analog sticks, common to most gamepads.
//#define MAX_TRIGGERS		2		// 2 analog triggers, common to most gamepads.



// =============================================================================================
struct CGamepadMappingEntry {
	EGamepadInput Input;
	EInputType Type;
	UINT16 Index;
};


// =============================================================================================
struct CGamepadButtonMapping {
	
	CGamepadMappingEntry Entries[MAX_ENTRY];
	UINT16 EntryCount;
	bool IsDefault;
	
	// Indicates that the mapping is a 'patch'.  The entries within are meant to replace matching entries in the default mapping.
	// If false, the mapping should be considered complete.
	bool IsPatch;

	// Get the mapping for the corresponding input.
	// Returns null if there is no matching entry.
	const CGamepadMappingEntry* GetMappingFor(const EGamepadInput& i);
};

// =============================================================================================
// Represents access to a database of gamepad mappings.
// Because the indexes, etc. of the buttons that are represented in DirectInput/XInput
// are not the same across all devices, we need a way to choose a consistent layout
// for any old device that we might want to plug in.
class CGamepadMappingsFile {

public:

	// Populates 'index' with the button index for the device with a matching 'productGuid' entry.  If
	// productGuid doesn't match no index data will be populated. 
	void GetGamepadMapping(GUID& productGuid, CGamepadButtonMapping& index);

	void GetDefault(CGamepadButtonMapping& mapping);


private:



};
















#endif