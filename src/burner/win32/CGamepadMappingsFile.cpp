#include "CGamepadMappingsFile.h"


const GUID XBONE = { 0x745a17a0, 0x74d3, 0x11d0, { 0xb6, 0xfe, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda } };
const GUID LOGITECH_510= { 0x4D1E55B2L, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };


void CGamepadMappingsFile::GetButtonIndex(GUID& productGuid, CGamepadButtonIndex& index)
{
	if (IsEqualGUID(productGuid, XBONE)){
	}
	else if (IsEqualGUID(productGuid, LOGITECH_510)) {
	}
	// TODO: We could open a file here.
	// For now, let's just stick with some hard-coded versions.

}