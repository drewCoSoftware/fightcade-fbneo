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

#define MAX_NAME 32			// NOTE: Includes null terminating character.
#define MAX_ENTRY 64


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

	wchar_t Platform[MAX_NAME];
};


#endif