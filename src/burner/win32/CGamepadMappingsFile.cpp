#include "CGamepadMappingsFile.h"
#include <Windows.h>

const GUID XBONE = { 0x02FF045E, 0x0000, 0x0000, { 0x00,0x00, 0x50, 0x49, 0x44, 0x56, 0x49, 0x44 } }; //0x50, 0x49, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda } };
const GUID LOGITECH_510 = { 0xC218046D, 0x0000, 0x0000, {0x00,0x00, 0x50, 0x49, 0x44, 0x56, 0x49, 0x44 } };

// ----------------------------------------------------------------------------------------
const CGamepadMappingEntry* CGamepadButtonMapping::GetMappingFor(const PCInputs& gpInput)
{
	for (size_t i = 0; i < EntryCount; i++)
	{
		if (Entries[i].Input == gpInput) {
			return &Entries[i];
		}
	}

	return nullptr;
}
