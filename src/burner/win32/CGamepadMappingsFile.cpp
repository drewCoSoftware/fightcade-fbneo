#include "CGamepadMappingsFile.h"
#include <Windows.h>

const GUID XBONE = { 0x02FF045E, 0x0000, 0x0000, { 0x00,0x00, 0x50, 0x49, 0x44, 0x56, 0x49, 0x44 } }; //0x50, 0x49, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda } };
const GUID LOGITECH_510 = { 0xC218046D, 0x0000, 0x0000,{0x00,0x00, 0x50, 0x49, 0x44, 0x56, 0x49, 0x44 } };


// ----------------------------------------------------------------------------------------
void CGamepadMappingsFile::GetGamepadMapping(GUID& productGuid, CGamepadButtonMapping& mapping)
{
	ZeroMemory(&mapping, sizeof(CGamepadButtonMapping));

	// TODO: We could open a file here.
	// For now, let's just stick with some hard-coded versions.


	//index.FoundMatch = true;
	/*if (IsEqualGUID(productGuid, XBONE)) {
		int x = 10;
	}*/
	GetDefault(mapping);

	if (IsEqualGUID(productGuid, LOGITECH_510)) {


		// Load the mapping.
		// Apply it to the default (patch) or copy it over.

		//index.FoundMatch = true;
		//index.ButtonCount = 8;

		//index.Buttons[0] = 1;
		//index.Buttons[1] = 2;
		//index.Buttons[2] = 0;
		//index.Buttons[3] = 3;
		//index.Buttons[4] = 4;		// unknown
		//index.Buttons[5] = 5;
		//index.Buttons[6] = 8;
		//index.Buttons[7] = 9;

		//index.DPadCount = 4;
		//index.DPads[0] = 0;
		//index.DPads[1] = 1;
		//index.DPads[2] = 2;
		//index.DPads[3] = 3;

		//index.TriggerCount = 0;

		//// Need some kind of way to represent the trigger as a simple button!
		//// For now, we just won't be able to remap the HK button......
		//// We can't remap the game input as that would mean a mapping per-controller, per-game, which is insane.
		//// we really just want to remap the (type + index)....

		////		index.Buttons[0] = 1;
		//int y = 10;
	}


}

// ----------------------------------------------------------------------------------------
void CGamepadMappingsFile::GetDefault(CGamepadButtonMapping& mapping) {

	ZeroMemory(&mapping, sizeof(CGamepadButtonMapping));
	
	mapping.Entries[0] =  { GPINPUT_LSTICK_UP, ITYPE_STICK, 0x02 };
	mapping.Entries[1] =  { GPINPUT_LSTICK_DOWN, ITYPE_STICK, 0x03 };
	mapping.Entries[2] =  { GPINPUT_LSTICK_LEFT, ITYPE_STICK, 0x00 };
	mapping.Entries[3] =  { GPINPUT_LSTICK_RIGHT, ITYPE_STICK, 0x01 };

	// NOTE: These may be incorrect.
	mapping.Entries[4] =  { GPINPUT_RSTICK_UP, ITYPE_STICK, 0x02 };
	mapping.Entries[5] =  { GPINPUT_RSTICK_DOWN, ITYPE_STICK, 0x03 };
	mapping.Entries[6] =  { GPINPUT_RSTICK_LEFT, ITYPE_STICK, 0x00 };
	mapping.Entries[7] =  { GPINPUT_RSTICK_RIGHT, ITYPE_STICK, 0x01 };

	mapping.Entries[8] =  { GPINPUT_DPAD_UP, ITYPE_DPAD, 0x02 };
	mapping.Entries[9] =  { GPINPUT_DPAD_DOWN, ITYPE_DPAD, 0x03 };
	mapping.Entries[10] = { GPINPUT_DPAD_LEFT, ITYPE_DPAD, 0x00 };
	mapping.Entries[11] = { GPINPUT_DPAD_RIGHT, ITYPE_DPAD, 0x01 };

	mapping.Entries[12] = { GPINPUT_BACK, ITYPE_BUTTON, 0x06 };
	mapping.Entries[13] = { GPINPUT_START, ITYPE_BUTTON, 0x07 };

	mapping.Entries[14] = { GPINPUT_X, ITYPE_BUTTON, 0x02 };
	mapping.Entries[15] = { GPINPUT_Y, ITYPE_BUTTON, 0x03 };
	mapping.Entries[16] = { GPINPUT_A, ITYPE_BUTTON, 0x00 };
	mapping.Entries[17] = { GPINPUT_B, ITYPE_BUTTON, 0x01 };

	// ?? Maybe......
	mapping.Entries[18] = { GPINPUT_LEFT_TRIGGER, ITYPE_TRIGGER, 0x05 };
	mapping.Entries[19] = { GPINPUT_LEFT_BUMPER, ITYPE_BUTTON, 0x04 };

	mapping.Entries[20] = { GPINPUT_RIGHT_TRIGGER, ITYPE_TRIGGER, 0x04 };
	mapping.Entries[21] = { GPINPUT_RIGHT_BUMPER, ITYPE_BUTTON, 0x05 };

	mapping.EntryCount = 22;
	mapping.IsDefault = true;
	mapping.IsPatch = false;

	// mapping->Entries = x;

	//	{ GPINPUT_LSTICK_UP, ITYPE_STICK, 0x02 }
	//};

	//gpp.inputCount = 12;
	//// gpp.driverName = _T("sfiii3nr1");			// NAME		-- GP2040 -- SDL
	//gpp.useAutoDirections = true;
	//gpp.inputs[0] = GamepadInputEx(ITYPE_BUTTON, 0x06);		// Coin		-- S1		-- BACK
	//gpp.inputs[1] = GamepadInputEx(ITYPE_BUTTON, 0x07);		// Start	-- S2		-- START

	//// POV HAT (0x10)
	//gpp.inputs[2] = GamepadInputEx(ITYPE_DPAD, 0x02);		// Up		-- -		-- h0.1
	//gpp.inputs[3] = GamepadInputEx(ITYPE_DPAD, 0x03);		// Down					-- h0.4
	//gpp.inputs[4] = GamepadInputEx(ITYPE_DPAD, 0x00);		// Left					-- h0.2
	//gpp.inputs[5] = GamepadInputEx(ITYPE_DPAD, 0x01);		// Right				-- h0.8

	////// ANALOG STICK (0x0)
	////gpp.inputs[2] = GamepadInputEx(0x00 | 0x02);		// Up		
	////gpp.inputs[3] = GamepadInputEx(0x00 | 0x03);		// Down
	////gpp.inputs[4] = GamepadInputEx(0x00 | 0x00);		// Left
	////gpp.inputs[5] = GamepadInputEx(0x00 | 0x01);		// Right

	//gpp.inputs[6] = GamepadInputEx(ITYPE_BUTTON, 0x02);		// LP		-- B3
	//gpp.inputs[7] = GamepadInputEx(ITYPE_BUTTON, 0x03);		// MP		-- B4
	//gpp.inputs[8] = GamepadInputEx(ITYPE_BUTTON, 0x05);		// HP		-- LB
	//gpp.inputs[9] = GamepadInputEx(ITYPE_BUTTON, 0x00);		// LK		-- B1
	//gpp.inputs[10] = GamepadInputEx(ITYPE_BUTTON, 0x01);		// MK		-- B2

	//// ANALOG - AXIS (z-neg)
	//gpp.inputs[11] = GamepadInputEx(ITYPE_TRIGGER, 0x00 | 0x04);		// HK -- LT

}

// ----------------------------------------------------------------------------------------
const CGamepadMappingEntry* CGamepadButtonMapping::GetMappingFor(const EGamepadInput& gpInput)
{
	for (size_t i = 0; i < EntryCount; i++)
	{
		if (Entries[i].Input == gpInput) {
			return &Entries[i];
		}
	}

	return nullptr;
}
