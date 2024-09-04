#include "CGamepadMappingsFile.h"
#include <Windows.h>


// productGuid = { C218046D - 0000 - 0000 - 0000 - 504944564944 }

const GUID XBONE = { 0x02FF045E, 0x0000, 0x0000, { 0x00,0x00, 0x50, 0x49, 0x44, 0x56, 0x49, 0x44 } }; //0x50, 0x49, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda } };
const GUID LOGITECH_510 = { 0xC218046D, 0x0000, 0x0000,{0x00,0x00, 0x50, 0x49, 0x44, 0x56, 0x49, 0x44 } };


// ----------------------------------------------------------------------------------------
void CGamepadMappingsFile::GetButtonIndex(GUID& productGuid, CGamepadButtonIndex& index)
{
	ZeroMemory(&index, sizeof(index));


	// TODO: We could open a file here.
	// For now, let's just stick with some hard-coded versions.


	index.FoundMatch = true;
	if (IsEqualGUID(productGuid, XBONE)) {
		int x = 10;
	}
	else if (IsEqualGUID(productGuid, LOGITECH_510)) {
		int y = 10;
	}
	else {
		index.FoundMatch = false;
	}


}