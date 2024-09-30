// Burner Game Input
#include "burner.h"
#include "CGamepadDatabase.h"

// Player Default Controls
// These defaults are created / saved when the emulator is closed or something... they don't come pre-installed.
// soo.... this is the mechanism that is used to 'remember' the last set of inputs for a given driver / platform....
// I'm guessing that we can also come up with a better system, esp. since it saves joy indexes and stuff like that.....
// --> Yes, we can have a better system, after looking at the cona.cpp code, we certainly can...
INT32 nPlayerDefaultControls[5] = { 0, 1, 2, 3, 4 };
TCHAR szPlayerDefaultIni[5][MAX_PATH] = { _T(""), _T(""), _T(""), _T(""), _T("") };

// New mapping of PC inputs to game inputs.
// GameInp.pcInput will not be used in the future.
// Inputs are broken into logical sets, i.e. 'player', 'system', etc.
CGameInputSet GameInputSet;

// Flag to let us know that we should read from here to get the game inputs vs.
// using the ones that are defined in GameInp.....
// When this is set to true, we will process GameInputSet FIRST, and then copy those
// values over to GameInp.pcInput.  This is how (at least for now) we will achieve proper
// double mapping for directional inputs.
bool UseGameInputSetForPCInputs = false;

// Mapping of PC inputs to game inputs
struct GameInp* GameInp = NULL;
UINT32 nGameInpCount = 0;							// The number of buttons/analog/other inputs.
UINT32 nMacroCount = 0;
UINT32 nMaxMacro = 0;

INT32 nAnalogSpeed;

INT32 nFireButtons = 0;

bool bStreetFighterLayout = false;
bool bLeftAltkeyMapped = false;

enum {
	UP,
	DOWN,
	LEFT,
	RIGHT,
	COUNT
};

static INT32 InpDirections[2][COUNT] = {};
static INT32 InpDataPrev[2][COUNT] = {};
static INT32 InpDataNext[2][COUNT] = {};
static bool bClearOpposites = false;

static CGamepadDatabase GamepadDatabase;

// ------------------------------------------------------------------------------------------------------------------------------
INT32 LoadGamepadDatabase() {
	// NOTE: The origninal gamepad database file came from here:
	// https://github.com/mdqinc/SDL_GameControllerDB/tree/master
	// It would be prudent to update it once in a while, I think.
	if (!GamepadDatabase.IsInitialized()) {
		GamepadDatabase.Init(L"config\\gamecontrollerdb.txt", L"Windows");
	}
	return 0;
}

static int GetInpFrame(INT32 player, INT32 dir)
{
	return GameInp[InpDirections[player][dir]].Input.nVal;
}

static void SetInpFrame(INT32 player, INT32 dir, INT32 val, BOOL bCopy)
{
	GameInp[InpDirections[player][dir]].Input.nVal = val;
	if (bCopy) {
		*(GameInp[InpDirections[player][dir]].Input.pVal) = val;
	}
}

static int GetInpPrev(INT32 player, INT32 dir)
{
	return InpDataPrev[player][dir];
}

static void SetInpPrev(INT32 player, INT32 dir, INT32 val)
{
	InpDataPrev[player][dir] = val;
}

static int GetInpNext(INT32 player, INT32 dir)
{
	return InpDataNext[player][dir];
}

static void SetInpNext(INT32 player, INT32 dir, INT32 val)
{
	InpDataNext[player][dir] = val;
}

// These are mappable global macros for mapping Pause/FFWD etc to controls in the input mapping dialogue. -dink
UINT8 macroSystemPause = 0;
UINT8 macroSystemFFWD = 0;
UINT8 macroSystemFrame = 0;
UINT8 macroSystemSaveState = 0;
UINT8 macroSystemLoadState = 0;
UINT8 macroSystemUNDOState = 0;
UINT8 macroSystemLuaHotkey1 = 0;
UINT8 macroSystemLuaHotkey2 = 0;
UINT8 macroSystemLuaHotkey3 = 0;
UINT8 macroSystemLuaHotkey4 = 0;
UINT8 macroSystemLuaHotkey5 = 0;
UINT8 macroSystemLuaHotkey6 = 0;
UINT8 macroSystemLuaHotkey7 = 0;
UINT8 macroSystemLuaHotkey8 = 0;
UINT8 macroSystemLuaHotkey9 = 0;

std::vector<UINT8*> lua_hotkeys = {
	&macroSystemLuaHotkey1,
	&macroSystemLuaHotkey2,
	&macroSystemLuaHotkey3,
	&macroSystemLuaHotkey4,
	&macroSystemLuaHotkey5,
	&macroSystemLuaHotkey6,
	&macroSystemLuaHotkey7,
	&macroSystemLuaHotkey8,
	&macroSystemLuaHotkey9
};

#define HW_NEOGEO ( ((BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK) == HARDWARE_SNK_NEOGEO) || ((BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK) == HARDWARE_SNK_NEOCD) )

// ---------------------------------------------------------------------------

// Check if the left alt (menu) key is mapped
#ifndef __LIBRETRO__
void GameInpCheckLeftAlt()
{
	struct GameInp* pgi;
	UINT32 i;

	bLeftAltkeyMapped = false;

	for (i = 0, pgi = GameInp; i < (nGameInpCount + nMacroCount); i++, pgi++) {

		if (bLeftAltkeyMapped) {
			break;
		}

		switch (pgi->pcInput) {
		case GIT_SWITCH:
			if (pgi->Input.Switch.nCode == FBK_LALT) {
				bLeftAltkeyMapped = true;
			}
			break;
		case GIT_MACRO_AUTO:
		case GIT_MACRO_CUSTOM:
			if (pgi->Macro.nMode) {
				if (pgi->Macro.Switch.nCode == FBK_LALT) {
					bLeftAltkeyMapped = true;
				}
			}
			break;

		default:
			continue;
		}
	}
}

// Check if the sytem mouse is mapped and set the cooperative level apropriately
void GameInpCheckMouse()
{
	bool bMouseMapped = false;
	struct GameInp* pgi;
	UINT32 i;

	for (i = 0, pgi = GameInp; i < (nGameInpCount + nMacroCount); i++, pgi++) {

		if (bMouseMapped) {
			break;
		}

		switch (pgi->pcInput) {
		case GIT_SWITCH:
			if ((pgi->Input.Switch.nCode & 0xFF00) == 0x8000) {
				bMouseMapped = true;
			}
			break;
		case GIT_MOUSEAXIS:
			if (pgi->Input.MouseAxis.nMouse == 0) {
				bMouseMapped = true;
			}
			break;
		case GIT_MACRO_AUTO:
		case GIT_MACRO_CUSTOM:
			if (pgi->Macro.nMode) {
				if ((pgi->Macro.Switch.nCode & 0xFF00) == 0x8000) {
					bMouseMapped = true;
				}
			}
			break;

		default:
			continue;
		}
	}

	if (bDrvOkay) {
		if (!bRunPause) {
			InputSetCooperativeLevel(bMouseMapped, bAlwaysProcessKeyboardInput);
		}
		else {
			InputSetCooperativeLevel(false, bAlwaysProcessKeyboardInput);
		}
	}
	else {
		InputSetCooperativeLevel(false, false);
	}
}
#endif

// ---------------------------------------------------------------------------
INT32 ResetGameInputs(bool resetDipSwitches)
{
	UINT32 i = 0;
	struct GameInp* pgi = NULL;

	// Reset all inputs to undefined (even dip switches, if bDipSwitch==1)
	if (GameInp == NULL) {
		return 1;
	}

	// Get the targets in the library for the Input Values
	for (i = 0, pgi = GameInp; i < nGameInpCount; i++, pgi++) {
		struct BurnInputInfo bii;
		memset(&bii, 0, sizeof(bii));
		BurnDrvGetInputInfo(&bii, i);
		if (!resetDipSwitches && (bii.nType & BIT_GROUP_CONSTANT)) {		// Don't blank the dip switches
			continue;
		}

		memset(pgi, 0, sizeof(*pgi));									// Clear input

		pgi->nType = bii.nType;											// store input type
		pgi->Input.pVal = bii.pVal;										// store input pointer to value

		if (bii.nType & BIT_GROUP_CONSTANT) {							// Further initialisation for constants/DIPs
			pgi->pcInput = GIT_CONSTANT;
			pgi->Input.Constant.nConst = *bii.pVal;
		}
	}

	for (i = 0; i < nMacroCount; i++, pgi++) {
		pgi->Macro.nMode = 0;
		if (pgi->pcInput == GIT_MACRO_CUSTOM) {
			pgi->pcInput = 0;
		}
	}

	bLeftAltkeyMapped = false;

	return 0;
}

// OBSOLETE: Macro system will be removed + replaced.  It is not good.
static void GameInpInitMacros()
{
	struct GameInp* pgi;
	struct BurnInputInfo bii;

	INT32 nPunchx3[5] = { 0, 0, 0, 0, 0 };
	INT32 nPunchInputs[5][3];
	INT32 nKickx3[5] = { 0, 0, 0, 0, 0 };
	INT32 nKickInputs[5][3];
	INT32 nAttackx3[5] = { 0, 0, 0, 0, 0 };
	INT32 nAttackInputs[5][3];

	INT32 nNeogeoButtons[5][4];
	INT32 nPgmButtons[10][16];

	bStreetFighterLayout = false;
	nMacroCount = 0;

	nFireButtons = 0;

	memset(&nNeogeoButtons, 0, sizeof(nNeogeoButtons));
	memset(&nPgmButtons, 0, sizeof(nPgmButtons));

	for (UINT32 i = 0; i < nGameInpCount; i++) {
		bii.szName = NULL;
		BurnDrvGetInputInfo(&bii, i);
		if (bii.szName == NULL) {
			bii.szName = "";
		}

		bool bPlayerInInfo = (toupper(bii.szInfo[0]) == 'P' && bii.szInfo[1] >= '1' && bii.szInfo[1] <= '4'); // Because some of the older drivers don't use the standard input naming.
		bool bPlayerInName = (bii.szName[0] == 'P' && bii.szName[1] >= '1' && bii.szName[1] <= '4');

		if (bPlayerInInfo || bPlayerInName) {
			INT32 nPlayer = 0;

			if (bPlayerInName)
				nPlayer = bii.szName[1] - '1';
			if (bPlayerInInfo && nPlayer == 0)
				nPlayer = bii.szInfo[1] - '1';

			if (nPlayer == 0) {
				if (strncmp(" fire", bii.szInfo + 2, 5) == 0) {
					nFireButtons++;
				}
			}

			// 3x punch
			if (_stricmp(" Weak Punch", bii.szName + 2) == 0) {
				nPunchx3[nPlayer] |= 1;
				nPunchInputs[nPlayer][0] = i;
			}
			if (_stricmp(" Medium Punch", bii.szName + 2) == 0) {
				nPunchx3[nPlayer] |= 2;
				nPunchInputs[nPlayer][1] = i;
			}
			if (_stricmp(" Strong Punch", bii.szName + 2) == 0) {
				nPunchx3[nPlayer] |= 4;
				nPunchInputs[nPlayer][2] = i;
			}

			// 3x kick
			if (_stricmp(" Weak Kick", bii.szName + 2) == 0) {
				nKickx3[nPlayer] |= 1;
				nKickInputs[nPlayer][0] = i;
			}
			if (_stricmp(" Medium Kick", bii.szName + 2) == 0) {
				nKickx3[nPlayer] |= 2;
				nKickInputs[nPlayer][1] = i;
			}
			if (_stricmp(" Strong Kick", bii.szName + 2) == 0) {
				nKickx3[nPlayer] |= 4;
				nKickInputs[nPlayer][2] = i;
			}

			// 3x attack
			if (_stricmp(" Weak Attack", bii.szName + 2) == 0) {
				nAttackx3[nPlayer] |= 1;
				nAttackInputs[nPlayer][0] = i;
			}
			if (_stricmp(" Medium Attack", bii.szName + 2) == 0) {
				nAttackx3[nPlayer] |= 2;
				nAttackInputs[nPlayer][1] = i;
			}
			if (_stricmp(" Strong Attack", bii.szName + 2) == 0) {
				nAttackx3[nPlayer] |= 4;
				nAttackInputs[nPlayer][2] = i;
			}

			if (HW_NEOGEO) {
				if (_stricmp(" Button A", bii.szName + 2) == 0) {
					nNeogeoButtons[nPlayer][0] = i;
				}
				if (_stricmp(" Button B", bii.szName + 2) == 0) {
					nNeogeoButtons[nPlayer][1] = i;
				}
				if (_stricmp(" Button C", bii.szName + 2) == 0) {
					nNeogeoButtons[nPlayer][2] = i;
				}
				if (_stricmp(" Button D", bii.szName + 2) == 0) {
					nNeogeoButtons[nPlayer][3] = i;
				}
			}

			//if ((BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK) == HARDWARE_IGS_PGM) {
			{ // Use nPgmButtons for Autofire too -dink
				if ((_stricmp(" Button 1", bii.szName + 2) == 0) || (_stricmp(" fire 1", bii.szInfo + 2) == 0)) {
					nPgmButtons[nPlayer][0] = i;
				}
				if ((_stricmp(" Button 2", bii.szName + 2) == 0) || (_stricmp(" fire 2", bii.szInfo + 2) == 0)) {
					nPgmButtons[nPlayer][1] = i;
				}
				if ((_stricmp(" Button 3", bii.szName + 2) == 0) || (_stricmp(" fire 3", bii.szInfo + 2) == 0)) {
					nPgmButtons[nPlayer][2] = i;
				}
				if ((_stricmp(" Button 4", bii.szName + 2) == 0) || (_stricmp(" fire 4", bii.szInfo + 2) == 0)) {
					nPgmButtons[nPlayer][3] = i;
				}
				if ((_stricmp(" Button 5", bii.szName + 2) == 0) || (_stricmp(" fire 5", bii.szInfo + 2) == 0)) {
					nPgmButtons[nPlayer][4] = i;
				}
				if ((_stricmp(" Button 6", bii.szName + 2) == 0) || (_stricmp(" fire 6", bii.szInfo + 2) == 0)) {
					nPgmButtons[nPlayer][5] = i;
				}
			}
		}
	}

	pgi = GameInp + nGameInpCount;

	{ // Mappable system macros -dink
		pgi->pcInput = GIT_MACRO_AUTO;
		pgi->nType = BIT_DIGITAL;
		pgi->Macro.nMode = 0;
		pgi->Macro.nSysMacro = 1;
		sprintf(pgi->Macro.szName, "System Pause");
		pgi->Macro.pVal[0] = &macroSystemPause;
		pgi->Macro.nVal[0] = 1;
		nMacroCount++;
		pgi++;

		pgi->pcInput = GIT_MACRO_AUTO;
		pgi->nType = BIT_DIGITAL;
		pgi->Macro.nMode = 0;
		pgi->Macro.nSysMacro = 1;
		sprintf(pgi->Macro.szName, "System FFWD");
		pgi->Macro.pVal[0] = &macroSystemFFWD;
		pgi->Macro.nVal[0] = 1;
		nMacroCount++;
		pgi++;

		pgi->pcInput = GIT_MACRO_AUTO;
		pgi->nType = BIT_DIGITAL;
		pgi->Macro.nMode = 0;
		pgi->Macro.nSysMacro = 1;
		sprintf(pgi->Macro.szName, "System Frame");
		pgi->Macro.pVal[0] = &macroSystemFrame;
		pgi->Macro.nVal[0] = 1;
		nMacroCount++;
		pgi++;

		pgi->pcInput = GIT_MACRO_AUTO;
		pgi->nType = BIT_DIGITAL;
		pgi->Macro.nMode = 0;
		pgi->Macro.nSysMacro = 1;
		sprintf(pgi->Macro.szName, "System Load State");
		pgi->Macro.pVal[0] = &macroSystemLoadState;
		pgi->Macro.nVal[0] = 1;
		nMacroCount++;
		pgi++;

		pgi->pcInput = GIT_MACRO_AUTO;
		pgi->nType = BIT_DIGITAL;
		pgi->Macro.nMode = 0;
		pgi->Macro.nSysMacro = 1;
		sprintf(pgi->Macro.szName, "System Save State");
		pgi->Macro.pVal[0] = &macroSystemSaveState;
		pgi->Macro.nVal[0] = 1;
		nMacroCount++;
		pgi++;

		pgi->pcInput = GIT_MACRO_AUTO;
		pgi->nType = BIT_DIGITAL;
		pgi->Macro.nMode = 0;
		pgi->Macro.nSysMacro = 1;
		sprintf(pgi->Macro.szName, "System UNDO State");
		pgi->Macro.pVal[0] = &macroSystemUNDOState;
		pgi->Macro.nVal[0] = 1;
		nMacroCount++;
		pgi++;

		for (UINT32 hotkey_num = 0; hotkey_num < lua_hotkeys.size(); hotkey_num++) {
			char hotkey_name[20];
			sprintf(hotkey_name, "Lua Hotkey %d", (hotkey_num + 1));
			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			pgi->Macro.nSysMacro = 1;
			sprintf(pgi->Macro.szName, hotkey_name);
			pgi->Macro.pVal[0] = lua_hotkeys[hotkey_num];
			pgi->Macro.nVal[0] = 1;
			nMacroCount++;
			pgi++;
		}
	}

	if (!kNetGame)
	{ // Autofire!!!
		for (INT32 nPlayer = 0; nPlayer < nMaxPlayers; nPlayer++) {
			for (INT32 i = 0; i < nFireButtons; i++) {
				pgi->pcInput = GIT_MACRO_AUTO;
				pgi->nType = BIT_DIGITAL;
				pgi->Macro.nMode = 0;
				pgi->Macro.nSysMacro = 15; // 15 = Auto-Fire mode
				if ((BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK) == HARDWARE_SEGA_MEGADRIVE) {
					if (i < 3) {
						sprintf(pgi->Macro.szName, "P%d Auto-Fire Button %c", nPlayer + 1, i + 'A'); // A,B,C
					}
					else {
						sprintf(pgi->Macro.szName, "P%d Auto-Fire Button %c", nPlayer + 1, i + 'X' - 3); // X,Y,Z
					}
				}
				else {
					sprintf(pgi->Macro.szName, "P%d Auto-Fire Button %d", nPlayer + 1, i + 1);
				}
				if (HW_NEOGEO) {
					BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][i]);
				}
				else {
					BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][i]);
				}
				pgi->Macro.pVal[0] = bii.pVal;
				pgi->Macro.nVal[0] = 1;
				nMacroCount++;
				pgi++;
			}
		}
	}

	for (INT32 nPlayer = 0; nPlayer < nMaxPlayers; nPlayer++) {
		if (nPunchx3[nPlayer] == 7) {		// Create a 3x punch macro
			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;

			sprintf(pgi->Macro.szName, "P%i 3x Punch", nPlayer + 1);
			for (INT32 j = 0; j < 3; j++) {
				BurnDrvGetInputInfo(&bii, nPunchInputs[nPlayer][j]);
				pgi->Macro.pVal[j] = bii.pVal;
				pgi->Macro.nVal[j] = 1;
			}

			nMacroCount++;
			pgi++;
		}

		if (nKickx3[nPlayer] == 7) {		// Create a 3x kick macro
			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;

			sprintf(pgi->Macro.szName, "P%i 3x Kick", nPlayer + 1);
			for (INT32 j = 0; j < 3; j++) {
				BurnDrvGetInputInfo(&bii, nKickInputs[nPlayer][j]);
				pgi->Macro.pVal[j] = bii.pVal;
				pgi->Macro.nVal[j] = 1;
			}

			nMacroCount++;
			pgi++;
		}

		if (nAttackx3[nPlayer] == 7) {		// Create a 3x attack macro
			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;

			sprintf(pgi->Macro.szName, "P%i 3x Attack", nPlayer + 1);
			for (INT32 j = 0; j < 3; j++) {
				BurnDrvGetInputInfo(&bii, nAttackInputs[nPlayer][j]);
				pgi->Macro.pVal[j] = bii.pVal;
				pgi->Macro.nVal[j] = 1;
			}

			nMacroCount++;
			pgi++;
		}

		if (nFireButtons == 4 && HW_NEOGEO) {
			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons AB", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][0]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][1]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons AC", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][0]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][2]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons AD", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][0]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][3]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons BC", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][1]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][2]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons BD", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][1]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][3]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons CD", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][2]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][3]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons ABC", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][0]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][1]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][2]);
			pgi->Macro.pVal[2] = bii.pVal;
			pgi->Macro.nVal[2] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons ABD", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][0]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][1]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][3]);
			pgi->Macro.pVal[2] = bii.pVal;
			pgi->Macro.nVal[2] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons ACD", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][0]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][2]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][3]);
			pgi->Macro.pVal[2] = bii.pVal;
			pgi->Macro.nVal[2] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons BCD", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][1]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][2]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][3]);
			pgi->Macro.pVal[2] = bii.pVal;
			pgi->Macro.nVal[2] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons ABCD", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][0]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][1]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][2]);
			pgi->Macro.pVal[2] = bii.pVal;
			pgi->Macro.nVal[2] = 1;
			BurnDrvGetInputInfo(&bii, nNeogeoButtons[nPlayer][3]);
			pgi->Macro.pVal[3] = bii.pVal;
			pgi->Macro.nVal[3] = 1;
			nMacroCount++;
			pgi++;
		}

		if (nFireButtons == 4 && (BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK) == HARDWARE_IGS_PGM) {
			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons 12", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][0]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][1]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons 13", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][0]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][2]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons 14", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][0]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][3]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons 23", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][1]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][2]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons 24", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][1]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][3]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons 34", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][2]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][3]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons 123", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][0]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][1]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][2]);
			pgi->Macro.pVal[2] = bii.pVal;
			pgi->Macro.nVal[2] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons 124", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][0]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][1]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][3]);
			pgi->Macro.pVal[2] = bii.pVal;
			pgi->Macro.nVal[2] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons 134", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][0]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][2]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][3]);
			pgi->Macro.pVal[2] = bii.pVal;
			pgi->Macro.nVal[2] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons 234", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][1]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][2]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][3]);
			pgi->Macro.pVal[2] = bii.pVal;
			pgi->Macro.nVal[2] = 1;
			nMacroCount++;
			pgi++;

			pgi->pcInput = GIT_MACRO_AUTO;
			pgi->nType = BIT_DIGITAL;
			pgi->Macro.nMode = 0;
			sprintf(pgi->Macro.szName, "P%i Buttons 1234", nPlayer + 1);
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][0]);
			pgi->Macro.pVal[0] = bii.pVal;
			pgi->Macro.nVal[0] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][1]);
			pgi->Macro.pVal[1] = bii.pVal;
			pgi->Macro.nVal[1] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][2]);
			pgi->Macro.pVal[2] = bii.pVal;
			pgi->Macro.nVal[2] = 1;
			BurnDrvGetInputInfo(&bii, nPgmButtons[nPlayer][3]);
			pgi->Macro.pVal[3] = bii.pVal;
			pgi->Macro.nVal[3] = 1;
			nMacroCount++;
			pgi++;
		}
	}

	if ((nPunchx3[0] == 7) && (nKickx3[0] == 7)) {
		bStreetFighterLayout = true;
	}

	if (nFireButtons >= 7 && (BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK) == HARDWARE_CAPCOM_CPS2) {
		// used to be 5 buttons in the above check - now we have volume buttons it is 7
		bStreetFighterLayout = true;
	}
}

// --------------------------------------------------------------------------------------------------------
INT32 GameInpInit()
{
	INT32 nRet = 0;
	// Count the number of inputs
	nGameInpCount = 0;
	nMacroCount = 0;
	nMaxMacro = nMaxPlayers * 52;

	// This loop literally just counts up to the size of the input list.
	// Yep, don't bother to just define the size, loop over it till you overflow :``D
	// Anyway.... the driver defines the total number of unique inputs....
	for (UINT32 i = 0; i < 0x1000; i++) {
		nRet = BurnDrvGetInputInfo(NULL, i);
		if (nRet) {														// end of input list
			nGameInpCount = i;
			break;
		}
	}

	// Allocate space for all the inputs
	INT32 nSize = (nGameInpCount + nMaxMacro) * sizeof(struct GameInp);
	GameInp = (struct GameInp*)malloc(nSize);
	if (GameInp == NULL) {
		return 1;
	}
	memset(GameInp, 0, nSize);


	//	// This is where we will / can define a default CGameInputSet instance!
	//	// So the idea is that we will always use a CGameInputSet to map onto GameImp for now.
	//	// Other code that we have... (and eventually a user UI) can change this default, BUT
	//	// creating it in the first place kind of gives us the best of both worlds...  we can
	//	// always assume that there is a CGameInputSet instance that is actually controlling the action...
	//	// --> NOTE: This still may/probably break compatibility with the input config dialog... figuring out a 
	//	//      way to keep all of that consistent during the transition will be a big challenge....
	//	//-- --> Maybe add a button or something on the dialog as a HACK?
	//	// We can also detect the directional inputs while we are here....
	//	CGameInputSet dSet;
	//	dSet.GroupCount = 1;
	//
	//	GameInputGroup& dGroup = dSet.InputGroups[0];
	//	dGroup.BurnInputStartIndex = 0;
	//	dGroup.GroupType = IGROUP_NOT_USED;
	//
	//	{
	//		struct BurnInputInfo bii;
	//		for (UINT32 i = 0; i < nGameInpCount; i++) {
	//			BurnDrvGetInputInfo(&bii, i);
	//
	//			dGroup.InputCount++;
	//			dGroup.Inputs[i] = { GP
	////			bii.nType
	//		}
	//	}
	//	throw std::exception("Create a default CGameInputSet instance from the GameInpArray");


		// cache input directions for clearing opposites
		// TODO: Input system needs to be reworked so that it can be aware of directional inputs in a sane way.
		// NOTE: This is a great place to make note of the directional inputs for a game so that we can look into mapping stick / dpad inputs to directions.
	memset(InpDirections[0], 0, 4 * sizeof(INT32));
	memset(InpDirections[1], 0, 4 * sizeof(INT32));
	struct BurnInputInfo bii;
	for (UINT32 i = 0; i < nGameInpCount; i++) {
		BurnDrvGetInputInfo(&bii, i);
		if (!_stricmp(bii.szName, "p1 up")) InpDirections[0][UP] = i; else
			if (!_stricmp(bii.szName, "p1 down")) InpDirections[0][DOWN] = i; else
				if (!_stricmp(bii.szName, "p1 left")) InpDirections[0][LEFT] = i; else
					if (!_stricmp(bii.szName, "p1 right")) InpDirections[0][RIGHT] = i; else
						if (!_stricmp(bii.szName, "p2 up")) InpDirections[1][UP] = i; else
							if (!_stricmp(bii.szName, "p2 down")) InpDirections[1][DOWN] = i; else
								if (!_stricmp(bii.szName, "p2 left")) InpDirections[1][LEFT] = i; else
									if (!_stricmp(bii.szName, "p2 right")) InpDirections[1][RIGHT] = i;
	}

	ResetGameInputs(true);

	InpDIPSWResetDIPs();

	GameInpInitMacros();

	nAnalogSpeed = 0x0100;

	// NOTE: This data should be contained in the driver data....
	// check if game needs clear opposites (SOCD)
	// --> Better yet, just assume that EVERY game needs to clear opposites.
	const char* clearOppositesGameList[] = {
		"umk3", "umk3p", "umk3uc", "umk3uk", "umk3te",
		"mk2", "mk2p", "mk2ute",
		"dbz2", "jojo", "jchan", "jchan2", "dankuga", "kaiserkn",
		NULL
	};
	const char* gameName = BurnDrvGetTextA(DRV_NAME);
	UINT32 i = 0;
	while (clearOppositesGameList[i]) {
		if (strstr(gameName, clearOppositesGameList[i]) == gameName) {
			bClearOpposites = true;
			break;
		}
		i++;
	}

	return 0;
}

INT32 GameInpExit()
{
	if (GameInp) {
		free(GameInp);
		GameInp = NULL;
	}

	nGameInpCount = 0;
	nMacroCount = 0;

	nFireButtons = 0;

	bStreetFighterLayout = false;
	bLeftAltkeyMapped = false;

	return 0;
}

// ---------------------------------------------------------------------------
// Convert a string from a config file to an input

static TCHAR* SliderInfo(struct GameInp* pgi, TCHAR* s)
{
	TCHAR* szRet = NULL;
	pgi->Input.Slider.nSliderSpeed = 0x700;				// defaults
	pgi->Input.Slider.nSliderCenter = 0;
	pgi->Input.Slider.nSliderValue = 0x8000;

	szRet = LabelCheck(s, _T("speed"));
	s = szRet;
	if (s == NULL) {
		return s;
	}
	pgi->Input.Slider.nSliderSpeed = (INT16)_tcstol(s, &szRet, 0);
	s = szRet;
	if (s == NULL) {
		return s;
	}
	szRet = LabelCheck(s, _T("center"));
	s = szRet;
	if (s == NULL) {
		return s;
	}
	pgi->Input.Slider.nSliderCenter = (INT16)_tcstol(s, &szRet, 0);
	s = szRet;
	if (s == NULL) {
		return s;
	}

	return szRet;
}

static INT32 StringToJoyAxis(struct GameInp* pgi, TCHAR* s)
{
	TCHAR* szRet = s;

	pgi->Input.JoyAxis.nJoy = (UINT8)_tcstol(s, &szRet, 0);
	if (szRet == NULL) {
		return 1;
	}
	s = szRet;
	pgi->Input.JoyAxis.nAxis = (UINT8)_tcstol(s, &szRet, 0);
	if (szRet == NULL) {
		return 1;
	}

	return 0;
}

static INT32 StringToMouseAxis(struct GameInp* pgi, TCHAR* s)
{
	TCHAR* szRet = s;

	pgi->Input.MouseAxis.nAxis = (UINT8)_tcstol(s, &szRet, 0);
	if (szRet == NULL) {
		return 1;
	}

	return 0;
}

static INT32 StringToMacro(struct GameInp* pgi, TCHAR* s)
{
	TCHAR* szRet = NULL;

	szRet = LabelCheck(s, _T("switch"));
	if (szRet) {
		s = szRet;
		pgi->Macro.nMode = 0x01;
		pgi->Macro.Switch.nCode = (UINT16)_tcstol(s, &szRet, 0);
		return 0;
	}

	return 1;
}

static INT32 StringToInp(struct GameInp* pgi, TCHAR* s)
{
	TCHAR* szRet = NULL;

	SKIP_WS(s);											// skip whitespace
	szRet = LabelCheck(s, _T("undefined"));
	if (szRet) {
		pgi->pcInput = 0;
		return 0;
	}

	szRet = LabelCheck(s, _T("constant"));
	if (szRet) {
		pgi->pcInput = GIT_CONSTANT;
		s = szRet;
		pgi->Input.Constant.nConst = (UINT8)_tcstol(s, &szRet, 0);
		*(pgi->Input.pVal) = pgi->Input.Constant.nConst;
		return 0;
	}

	szRet = LabelCheck(s, _T("switch"));
	if (szRet) {
		pgi->pcInput = GIT_SWITCH;
		s = szRet;
		pgi->Input.Switch.nCode = (UINT16)_tcstol(s, &szRet, 0);
		return 0;
	}

	// Analog using mouse axis:
	szRet = LabelCheck(s, _T("mouseaxis"));
	if (szRet) {
		pgi->pcInput = GIT_MOUSEAXIS;
		return StringToMouseAxis(pgi, szRet);
	}
	// Analog using joystick axis:
	szRet = LabelCheck(s, _T("joyaxis-neg"));
	if (szRet) {
		pgi->pcInput = GIT_JOYAXIS_NEG;
		return StringToJoyAxis(pgi, szRet);
	}
	szRet = LabelCheck(s, _T("joyaxis-pos"));
	if (szRet) {
		pgi->pcInput = GIT_JOYAXIS_POS;
		return StringToJoyAxis(pgi, szRet);
	}
	szRet = LabelCheck(s, _T("joyaxis"));
	if (szRet) {
		pgi->pcInput = GIT_JOYAXIS_FULL;
		return StringToJoyAxis(pgi, szRet);
	}

	// Analog using keyboard slider
	szRet = LabelCheck(s, _T("slider"));
	if (szRet) {
		s = szRet;
		pgi->pcInput = GIT_KEYSLIDER;
		pgi->Input.Slider.SliderAxis.nSlider[0] = 0;	// defaults
		pgi->Input.Slider.SliderAxis.nSlider[1] = 0;	//

		pgi->Input.Slider.SliderAxis.nSlider[0] = (UINT16)_tcstol(s, &szRet, 0);
		s = szRet;
		if (s == NULL) {
			return 1;
		}
		pgi->Input.Slider.SliderAxis.nSlider[1] = (UINT16)_tcstol(s, &szRet, 0);
		s = szRet;
		if (s == NULL) {
			return 1;
		}
		szRet = SliderInfo(pgi, s);
		s = szRet;
		if (s == NULL) {								// Get remaining slider info
			return 1;
		}
		return 0;
	}

	// Analog using joystick slider
	szRet = LabelCheck(s, _T("joyslider"));
	if (szRet) {
		s = szRet;
		pgi->pcInput = GIT_JOYSLIDER;
		pgi->Input.Slider.JoyAxis.nJoy = 0;				// defaults
		pgi->Input.Slider.JoyAxis.nAxis = 0;			//

		pgi->Input.Slider.JoyAxis.nJoy = (UINT8)_tcstol(s, &szRet, 0);
		s = szRet;
		if (s == NULL) {
			return 1;
		}
		pgi->Input.Slider.JoyAxis.nAxis = (UINT8)_tcstol(s, &szRet, 0);
		s = szRet;
		if (s == NULL) {
			return 1;
		}
		szRet = SliderInfo(pgi, s);						// Get remaining slider info
		s = szRet;
		if (s == NULL) {
			return 1;
		}
		return 0;
	}

	return 1;
}

// ---------------------------------------------------------------------------
// Convert an input to a string for config files

static TCHAR* InpToString(struct GameInp* pgi)
{
	static TCHAR szString[80];

	if (pgi->pcInput == 0) {
		return _T("undefined");
	}
	if (pgi->pcInput == GIT_CONSTANT) {
		_stprintf(szString, _T("constant 0x%.2X"), pgi->Input.Constant.nConst);
		return szString;
	}
	if (pgi->pcInput == GIT_SWITCH) {
		_stprintf(szString, _T("switch 0x%.2X"), pgi->Input.Switch.nCode);
		return szString;
	}
	if (pgi->pcInput == GIT_KEYSLIDER) {
		_stprintf(szString, _T("slider 0x%.2x 0x%.2x speed 0x%x center %d"), pgi->Input.Slider.SliderAxis.nSlider[0], pgi->Input.Slider.SliderAxis.nSlider[1], pgi->Input.Slider.nSliderSpeed, pgi->Input.Slider.nSliderCenter);
		return szString;
	}
	if (pgi->pcInput == GIT_JOYSLIDER) {
		_stprintf(szString, _T("joyslider %d %d speed 0x%x center %d"), pgi->Input.Slider.JoyAxis.nJoy, pgi->Input.Slider.JoyAxis.nAxis, pgi->Input.Slider.nSliderSpeed, pgi->Input.Slider.nSliderCenter);
		return szString;
	}
	if (pgi->pcInput == GIT_MOUSEAXIS) {
		_stprintf(szString, _T("mouseaxis %d"), pgi->Input.MouseAxis.nAxis);
		return szString;
	}
	if (pgi->pcInput == GIT_JOYAXIS_FULL) {
		_stprintf(szString, _T("joyaxis %d %d"), pgi->Input.JoyAxis.nJoy, pgi->Input.JoyAxis.nAxis);
		return szString;
	}
	if (pgi->pcInput == GIT_JOYAXIS_NEG) {
		_stprintf(szString, _T("joyaxis-neg %d %d"), pgi->Input.JoyAxis.nJoy, pgi->Input.JoyAxis.nAxis);
		return szString;
	}
	if (pgi->pcInput == GIT_JOYAXIS_POS) {
		_stprintf(szString, _T("joyaxis-pos %d %d"), pgi->Input.JoyAxis.nJoy, pgi->Input.JoyAxis.nAxis);
		return szString;
	}

	return _T("unknown");
}

static TCHAR* InpMacroToString(struct GameInp* pgi)
{
	static TCHAR szString[256];

	if (pgi->pcInput == GIT_MACRO_AUTO) {
		if (pgi->Macro.nMode) {
			_stprintf(szString, _T("switch 0x%.2X"), pgi->Macro.Switch.nCode);
			return szString;
		}
	}

	if (pgi->pcInput == GIT_MACRO_CUSTOM) {
		struct BurnInputInfo bii;

		if (pgi->Macro.nMode) {
			_stprintf(szString, _T("switch 0x%.2X"), pgi->Macro.Switch.nCode);
		}
		else {
			_stprintf(szString, _T("undefined"));
		}

		for (INT32 i = 0; i < 4; i++) {
			if (pgi->Macro.pVal[i]) {
				BurnDrvGetInputInfo(&bii, pgi->Macro.nInput[i]);
				_stprintf(szString + _tcslen(szString), _T(" \"%hs\" 0x%02X"), bii.szName, pgi->Macro.nVal[i]);
			}
		}

		return szString;
	}

	return _T("undefined");
}

// ---------------------------------------------------------------------------
// Generate a user-friendly name for a control (PC-side)
static struct { INT32 nCode; TCHAR* szName; } KeyNames[] = {

#define FBK_DEFNAME(k) k, _T(#k)

	{ FBK_ESCAPE,				_T("ESCAPE") },
	{ FBK_1,					_T("1") },
	{ FBK_2,					_T("2") },
	{ FBK_3,					_T("3") },
	{ FBK_4,					_T("4") },
	{ FBK_5,					_T("5") },
	{ FBK_6,					_T("6") },
	{ FBK_7,					_T("7") },
	{ FBK_8,					_T("8") },
	{ FBK_9,					_T("9") },
	{ FBK_0,					_T("0") },
	{ FBK_MINUS,				_T("MINUS") },
	{ FBK_EQUALS,				_T("EQUALS") },
	{ FBK_BACK,					_T("BACKSPACE") },
	{ FBK_TAB,					_T("TAB") },
	{ FBK_Q,					_T("Q") },
	{ FBK_W,					_T("W") },
	{ FBK_E,					_T("E") },
	{ FBK_R,					_T("R") },
	{ FBK_T,					_T("T") },
	{ FBK_Y,					_T("Y") },
	{ FBK_U,					_T("U") },
	{ FBK_I,					_T("I") },
	{ FBK_O,					_T("O") },
	{ FBK_P,					_T("P") },
	{ FBK_LBRACKET,				_T("OPENING BRACKET") },
	{ FBK_RBRACKET,				_T("CLOSING BRACKET") },
	{ FBK_RETURN,				_T("ENTER") },
	{ FBK_LCONTROL,				_T("LEFT CONTROL") },
	{ FBK_A,					_T("A") },
	{ FBK_S,					_T("S") },
	{ FBK_D,					_T("D") },
	{ FBK_F,					_T("F") },
	{ FBK_G,					_T("G") },
	{ FBK_H,					_T("H") },
	{ FBK_J,					_T("J") },
	{ FBK_K,					_T("K") },
	{ FBK_L,					_T("L") },
	{ FBK_SEMICOLON,			_T("SEMICOLON") },
	{ FBK_APOSTROPHE,			_T("APOSTROPHE") },
	{ FBK_GRAVE,				_T("ACCENT GRAVE") },
	{ FBK_LSHIFT,				_T("LEFT SHIFT") },
	{ FBK_BACKSLASH,			_T("BACKSLASH") },
	{ FBK_Z,					_T("Z") },
	{ FBK_X,					_T("X") },
	{ FBK_C,					_T("C") },
	{ FBK_V,					_T("V") },
	{ FBK_B,					_T("B") },
	{ FBK_N,					_T("N") },
	{ FBK_M,					_T("M") },
	{ FBK_COMMA,				_T("COMMA") },
	{ FBK_PERIOD,				_T("PERIOD") },
	{ FBK_SLASH,				_T("SLASH") },
	{ FBK_RSHIFT,				_T("RIGHT SHIFT") },
	{ FBK_MULTIPLY,				_T("NUMPAD MULTIPLY") },
	{ FBK_LALT,					_T("LEFT MENU") },
	{ FBK_SPACE,				_T("SPACE") },
	{ FBK_CAPITAL,				_T("CAPSLOCK") },
	{ FBK_F1,					_T("F1") },
	{ FBK_F2,					_T("F2") },
	{ FBK_F3,					_T("F3") },
	{ FBK_F4,					_T("F4") },
	{ FBK_F5,					_T("F5") },
	{ FBK_F6,					_T("F6") },
	{ FBK_F7,					_T("F7") },
	{ FBK_F8,					_T("F8") },
	{ FBK_F9,					_T("F9") },
	{ FBK_F10,					_T("F10") },
	{ FBK_NUMLOCK,				_T("NUMLOCK") },
	{ FBK_SCROLL,				_T("SCROLLLOCK") },
	{ FBK_NUMPAD7,				_T("NUMPAD 7") },
	{ FBK_NUMPAD8,				_T("NUMPAD 8") },
	{ FBK_NUMPAD9,				_T("NUMPAD 9") },
	{ FBK_SUBTRACT,				_T("NUMPAD SUBTRACT") },
	{ FBK_NUMPAD4,				_T("NUMPAD 4") },
	{ FBK_NUMPAD5,				_T("NUMPAD 5") },
	{ FBK_NUMPAD6,				_T("NUMPAD 6") },
	{ FBK_ADD,					_T("NUMPAD ADD") },
	{ FBK_NUMPAD1,				_T("NUMPAD 1") },
	{ FBK_NUMPAD2,				_T("NUMPAD 2") },
	{ FBK_NUMPAD3,				_T("NUMPAD 3") },
	{ FBK_NUMPAD0,				_T("NUMPAD 0") },
	{ FBK_DECIMAL,				_T("NUMPAD DECIMAL POINT") },
	{ FBK_DEFNAME(FBK_OEM_102) },
	{ FBK_F11,					_T("F11") },
	{ FBK_F12,					_T("F12") },
	{ FBK_F13,					_T("F13") },
	{ FBK_F14,					_T("F14") },
	{ FBK_F15,					_T("F15") },
	{ FBK_DEFNAME(FBK_KANA) },
	{ FBK_DEFNAME(FBK_ABNT_C1) },
	{ FBK_DEFNAME(FBK_CONVERT) },
	{ FBK_DEFNAME(FBK_NOCONVERT) },
	{ FBK_DEFNAME(FBK_YEN) },
	{ FBK_DEFNAME(FBK_ABNT_C2) },
	{ FBK_NUMPADEQUALS,			_T("NUMPAD EQUALS") },
	{ FBK_DEFNAME(FBK_PREVTRACK) },
	{ FBK_DEFNAME(FBK_AT) },
	{ FBK_COLON,				_T("COLON") },
	{ FBK_UNDERLINE,			_T("UNDERSCORE") },
	{ FBK_DEFNAME(FBK_KANJI) },
	{ FBK_DEFNAME(FBK_STOP) },
	{ FBK_DEFNAME(FBK_AX) },
	{ FBK_DEFNAME(FBK_UNLABELED) },
	{ FBK_DEFNAME(FBK_NEXTTRACK) },
	{ FBK_NUMPADENTER,			_T("NUMPAD ENTER") },
	{ FBK_RCONTROL,				_T("RIGHT CONTROL") },
	{ FBK_DEFNAME(FBK_MUTE) },
	{ FBK_DEFNAME(FBK_CALCULATOR) },
	{ FBK_DEFNAME(FBK_PLAYPAUSE) },
	{ FBK_DEFNAME(FBK_MEDIASTOP) },
	{ FBK_DEFNAME(FBK_VOLUMEDOWN) },
	{ FBK_DEFNAME(FBK_VOLUMEUP) },
	{ FBK_DEFNAME(FBK_WEBHOME) },
	{ FBK_DEFNAME(FBK_NUMPADCOMMA) },
	{ FBK_DIVIDE,				_T("NUMPAD DIVIDE") },
	{ FBK_SYSRQ,				_T("PRINTSCREEN") },
	{ FBK_RALT,					_T("RIGHT MENU") },
	{ FBK_PAUSE,				_T("PAUSE") },
	{ FBK_HOME,					_T("HOME") },
	{ FBK_UPARROW,				_T("ARROW UP") },
	{ FBK_PRIOR,				_T("PAGE UP") },
	{ FBK_LEFTARROW,			_T("ARROW LEFT") },
	{ FBK_RIGHTARROW,			_T("ARROW RIGHT") },
	{ FBK_END,					_T("END") },
	{ FBK_DOWNARROW,			_T("ARROW DOWN") },
	{ FBK_NEXT,					_T("PAGE DOWN") },
	{ FBK_INSERT,				_T("INSERT") },
	{ FBK_DELETE,				_T("DELETE") },
	{ FBK_LWIN,					_T("LEFT WINDOWS") },
	{ FBK_RWIN,					_T("RIGHT WINDOWS") },
	{ FBK_DEFNAME(FBK_APPS) },
	{ FBK_DEFNAME(FBK_POWER) },
	{ FBK_DEFNAME(FBK_SLEEP) },
	{ FBK_DEFNAME(FBK_WAKE) },
	{ FBK_DEFNAME(FBK_WEBSEARCH) },
	{ FBK_DEFNAME(FBK_WEBFAVORITES) },
	{ FBK_DEFNAME(FBK_WEBREFRESH) },
	{ FBK_DEFNAME(FBK_WEBSTOP) },
	{ FBK_DEFNAME(FBK_WEBFORWARD) },
	{ FBK_DEFNAME(FBK_WEBBACK) },
	{ FBK_DEFNAME(FBK_MYCOMPUTER) },
	{ FBK_DEFNAME(FBK_MAIL) },
	{ FBK_DEFNAME(FBK_MEDIASELECT) },

#undef FBK_DEFNAME

	{ 0,				NULL }
};

// ---------------------------------------------------------------------------
// NOTE: padInfos should be contained in gami.cpp!  Not in the indp.cpp file!
TCHAR* InputCodeDesc(INT32 c, GamepadFileEntry** padInfos)
{
	static TCHAR szString[64];
	TCHAR* szName = _T("");

	// Mouse
	if (c >= MOUSE_LOWER) {
		INT32 nMouse = (c >> 8) & 0x3F;
		INT32 nCode = c & 0xFF;
		if (nCode >= 0x80) {
			_stprintf(szString, _T("Mouse %d Button %d"), nMouse, nCode & 0x7F);
			return szString;
		}
		if (nCode < 0x06) {
			TCHAR szAxis[3][3] = { _T("X"), _T("Y"), _T("Z") };
			TCHAR szDir[6][16] = { _T("negative"), _T("positive"), _T("Left"), _T("Right"), _T("Up"), _T("Down") };
			if (nCode < 4) {
				_stprintf(szString, _T("Mouse %d %s (%s %s)"), nMouse, szDir[nCode + 2], szAxis[nCode >> 1], szDir[nCode & 1]);
			}
			else {
				_stprintf(szString, _T("Mouse %d %s %s"), nMouse, szAxis[nCode >> 1], szDir[nCode & 1]);
			}
			return szString;
		}
	}

	// Joystick
	// NOTE: If formatting for aliases, we would replace 'Joy' below with the alias name!
	if (c >= JOYSTICK_LOWER && c < JOYSTICK_UPPER) {

		INT32 nJoy = (c >> 8) & 0x3F;			// Translate the index of the joystick.
		INT32 nCode = c & 0xFF;

		// Determine the name to use for the input.
		TCHAR gamepadName[MAX_ALIAS_CHARS];
		_stprintf(gamepadName, _T("Joy % d"), nJoy);

		if (nCode >= 0x80) {
			_stprintf(szString, _T("%s Button %d"), gamepadName, nCode & 0x7F);
			return szString;
		}

		// Directional Axes:
		if (nCode < 0x10) {
			TCHAR szAxis[8][3] = { _T("X"), _T("Y"), _T("Z"), _T("rX"), _T("rY"), _T("rZ"), _T("s0"), _T("s1") };
			TCHAR szDir[6][16] = { _T("negative"), _T("positive"), _T("Left"), _T("Right"), _T("Up"), _T("Down") };
			if (nCode < 4) {
				_stprintf(szString, _T("%s %s (%s %s)"), gamepadName, szDir[nCode + 2], szAxis[nCode >> 1], szDir[nCode & 1]);
			}
			else {
				_stprintf(szString, _T("%s %s %s"), gamepadName, szAxis[nCode >> 1], szDir[nCode & 1]);
			}
			return szString;
		}

		// D-PAD
		if (nCode < 0x20) {
			TCHAR szDir[4][16] = { _T("Left"), _T("Right"), _T("Up"), _T("Down") };
			_stprintf(szString, _T("%s POV-hat %d %s"), gamepadName, (nCode & 0x0F) >> 2, szDir[nCode & 3]);
			return szString;
		}
	}

	// Keyboard
	for (INT32 i = 0; KeyNames[i].nCode; i++) {
		if (c == KeyNames[i].nCode) {
			if (KeyNames[i].szName) {
				szName = KeyNames[i].szName;
			}
			break;
		}
	}

	if (szName[0]) {
		_stprintf(szString, _T("%s"), szName);
	}
	else {
		_stprintf(szString, _T("code 0x%.2X"), c);
	}

	return szString;
}

// ---------------------------------------------------------------------------
// NOTE: This could be updated to reflected aliased gamepad names, if we wanted.
TCHAR* InpToDesc(struct GameInp* pgi, GamepadFileEntry** padInfos)
{
	static TCHAR szInputName[64] = _T("");

	if (pgi->pcInput == 0) {
		return _T("");
	}
	if (pgi->pcInput == GIT_CONSTANT) {
		if (pgi->nType & BIT_GROUP_CONSTANT) {
			for (INT32 i = 0; i < 8; i++) {
				szInputName[7 - i] = pgi->Input.Constant.nConst & (1 << i) ? _T('1') : _T('0');
			}
			szInputName[8] = 0;

			return szInputName;
		}

		if (pgi->Input.Constant.nConst == 0) {
			return _T("-");
		}
	}
	if (pgi->pcInput == GIT_SWITCH) {
		return InputCodeDesc(pgi->Input.Switch.nCode, padInfos);
	}
	if (pgi->pcInput == GIT_MOUSEAXIS) {
		TCHAR nAxis = _T('?');
		switch (pgi->Input.MouseAxis.nAxis) {
		case 0:
			nAxis = _T('X');
			break;
		case 1:
			nAxis = _T('Y');
			break;
		case 2:
			nAxis = _T('Z');
			break;
		}
		_stprintf(szInputName, _T("Mouse %i %c axis"), pgi->Input.MouseAxis.nMouse, nAxis);
		return szInputName;
	}

	if (pgi->pcInput & GIT_GROUP_JOYSTICK) {
		TCHAR szAxis[8][3] = { _T("X"), _T("Y"), _T("Z"), _T("rX"), _T("rY"), _T("rZ"), _T("s0"), _T("s1") };
		TCHAR szRange[4][16] = { _T("unknown"), _T("full"), _T("negative"), _T("positive") };
		INT32 nRange = 0;
		switch (pgi->pcInput) {
		case GIT_JOYAXIS_FULL:
			nRange = 1;
			break;
		case GIT_JOYAXIS_NEG:
			nRange = 2;
			break;
		case GIT_JOYAXIS_POS:
			nRange = 3;
			break;
		}

		// Get the name of our input first....
		TCHAR gamepadName[MAX_ALIAS_CHARS];
		_stprintf(gamepadName, _T("Joy % d"), pgi->Input.JoyAxis.nJoy);

		_stprintf(szInputName, _T("%s %s axis (%s range)"), gamepadName, szAxis[pgi->Input.JoyAxis.nAxis], szRange[nRange]);
		return szInputName;
	}

	return InpToString(pgi);							// Just do the rest as they are in the config file
}

TCHAR* InpMacroToDesc(struct GameInp* pgi)
{
	if (pgi->pcInput & GIT_GROUP_MACRO) {
		if (pgi->Macro.nMode) {
			return InputCodeDesc(pgi->Macro.Switch.nCode, NULL);
		}
	}

	return _T("");
}

// ---------------------------------------------------------------------------
// Find the input number by info
static UINT32 InputInfoToNum(TCHAR* szName)
{
	for (UINT32 i = 0; i < nGameInpCount; i++) {
		struct BurnInputInfo bii;
		BurnDrvGetInputInfo(&bii, i);
		if (bii.pVal == NULL) {
			continue;
		}

		if (_tcsicmp(szName, ANSIToTCHAR(bii.szInfo, NULL, 0)) == 0) {
			return i;
		}
	}
	return ~0U;
}

// Find the input number by name
static UINT32 InputNameToNum(TCHAR* szName)
{
	for (UINT32 i = 0; i < nGameInpCount; i++) {
		struct BurnInputInfo bii;
		BurnDrvGetInputInfo(&bii, i);
		if (bii.pVal == NULL) {
			continue;
		}

		if (_tcsicmp(szName, ANSIToTCHAR(bii.szName, NULL, 0)) == 0) {
			return i;
		}
	}
	return ~0U;
}

static TCHAR* InputNumToName(UINT32 i)
{
	struct BurnInputInfo bii;
	bii.szName = NULL;
	BurnDrvGetInputInfo(&bii, i);
	if (bii.szName == NULL) {
		return _T("unknown");
	}
	return ANSIToTCHAR(bii.szName, NULL, 0);
}

static UINT32 MacroNameToNum(TCHAR* szName)
{
	struct GameInp* pgi = GameInp + nGameInpCount;
	for (UINT32 i = 0; i < nMacroCount; i++, pgi++) {
		if (pgi->pcInput & GIT_GROUP_MACRO) {
			if (_tcsicmp(szName, ANSIToTCHAR(pgi->Macro.szName, NULL, 0)) == 0) {
				return i;
			}
		}
	}
	return ~0U;
}

// ---------------------------------------------------------------------------
// Set a reasonable default input for the corresponding PGI.
static INT32 GameInpAutoOne(struct GameInp* pgi, char* szi)
{
	for (INT32 i = 0; i < nMaxPlayers; i++) {
		INT32 nSlide = nPlayerDefaultControls[i] >> 4;
		switch (nPlayerDefaultControls[i] & 0x0F) {
		case 0:										// Keyboard
			GamcAnalogKey(pgi, szi, i, nSlide);
			GamcPlayer(pgi, szi, i, -1);
			GamcMisc(pgi, szi, i);
			break;
		case 1:										// Joystick 1
			GamcAnalogJoy(pgi, szi, i, 0, nSlide);
			GamcPlayer(pgi, szi, i, 0);
			GamcMisc(pgi, szi, i);
			break;
		case 2:										// Joystick 2
			GamcAnalogJoy(pgi, szi, i, 1, nSlide);
			GamcPlayer(pgi, szi, i, 1);
			GamcMisc(pgi, szi, i);
			break;
		case 3:										// Joystick 3
			GamcAnalogJoy(pgi, szi, i, 2, nSlide);
			GamcPlayer(pgi, szi, i, 2);
			GamcMisc(pgi, szi, i);
			break;
		case 4:										// X-Arcade left side
			GamcMisc(pgi, szi, i);
			GamcPlayerHotRod(pgi, szi, i, 0x10, nSlide);
			break;
		case 5:										// X-Arcade right side
			GamcMisc(pgi, szi, i);
			GamcPlayerHotRod(pgi, szi, i, 0x11, nSlide);
			break;
		case 6:										// Hot Rod left side
			GamcMisc(pgi, szi, i);
			GamcPlayerHotRod(pgi, szi, i, 0x00, nSlide);
			break;
		case 7:										// Hot Rod right side
			GamcMisc(pgi, szi, i);
			GamcPlayerHotRod(pgi, szi, i, 0x01, nSlide);
			break;
		default:
			GamcMisc(pgi, szi, i);
		}
	}

	return 0;
}

static INT32 AddCustomMacro(TCHAR* szValue, bool bOverWrite)
{
	TCHAR* szQuote = NULL;
	TCHAR* szEnd = NULL;

	if (QuoteRead(&szQuote, &szEnd, szValue)) {
		return 1;
	}

	INT32 nMode = -1;
	INT32 nInput = -1;
	bool bCreateNew = false;
	struct BurnInputInfo bii;

	for (UINT32 j = nGameInpCount; j < nGameInpCount + nMacroCount; j++) {
		if (GameInp[j].pcInput == GIT_MACRO_CUSTOM) {
			if (LabelCheck(szQuote, ANSIToTCHAR(GameInp[j].Macro.szName, NULL, 0))) {
				nInput = j;
				break;
			}
		}
	}

	if (nInput == -1) {
		if (nMacroCount + 1 == nMaxMacro) {
			return 1;
		}
		nInput = nGameInpCount + nMacroCount;
		bCreateNew = true;
	}

	_tcscpy(szQuote, ANSIToTCHAR(GameInp[nInput].Macro.szName, NULL, 0));

	if ((szValue = LabelCheck(szEnd, _T("undefined"))) != NULL) {
		nMode = 0;
	}
	else {
		if ((szValue = LabelCheck(szEnd, _T("switch"))) != NULL) {

			if (bOverWrite || GameInp[nInput].Macro.nMode == 0) {
				GameInp[nInput].Macro.Switch.nCode = (UINT16)_tcstol(szValue, &szValue, 0);
			}

			nMode = 1;
		}
	}

	if (nMode >= 0) {
		INT32 nFound = 0;

		for (INT32 i = 0; i < 4; i++) {
			GameInp[nInput].Macro.pVal[i] = NULL;
			GameInp[nInput].Macro.nVal[i] = 0;
			GameInp[nInput].Macro.nInput[i] = 0;

			if (szValue == NULL) {
				break;
			}

			if (QuoteRead(&szQuote, &szEnd, szValue)) {
				break;
			}

			for (UINT32 j = 0; j < nGameInpCount; j++) {
				bii.szName = NULL;
				BurnDrvGetInputInfo(&bii, j);
				if (bii.pVal == NULL) {
					continue;
				}

				TCHAR* szString = LabelCheck(szQuote, ANSIToTCHAR(bii.szName, NULL, 0));
				if (szString && szEnd) {
					GameInp[nInput].Macro.pVal[i] = bii.pVal;
					GameInp[nInput].Macro.nInput[i] = j;

					GameInp[nInput].Macro.nVal[i] = (UINT8)_tcstol(szEnd, &szValue, 0);

					nFound++;

					break;
				}
			}
		}

		if (nFound) {
			if (GameInp[nInput].Macro.pVal[nFound - 1]) {
				GameInp[nInput].pcInput = GIT_MACRO_CUSTOM;
				GameInp[nInput].Macro.nMode = nMode;
				if (bCreateNew) {
					nMacroCount++;
				}
				return 0;
			}
		}
	}

	return 1;
}

// Read game input settings from an ini file.
// --> This function will probably be removed.
INT32 GameInputAutoIni(INT32 nPlayer, TCHAR* lpszFile, bool bOverWrite)
{
	TCHAR szLine[1024];
	INT32 nFileVersion = 0;
	UINT32 i;

	//nAnalogSpeed = 0x0100; /* this clobbers the setting read at the beginning of the file. */

	FILE* h = _tfopen(lpszFile, _T("rt"));
	if (h == NULL) {
		return 1;
	}

	// Go through each line of the config file and process inputs
	while (_fgetts(szLine, sizeof(szLine), h)) {
		TCHAR* szValue;
		INT32 nLen = _tcslen(szLine);

		// Get rid of the linefeed at the end
		if (szLine[nLen - 1] == 10) {
			szLine[nLen - 1] = 0;
			nLen--;
		}

		szValue = LabelCheck(szLine, _T("version"));
		if (szValue) {
			nFileVersion = _tcstol(szValue, NULL, 0);
		}
		szValue = LabelCheck(szLine, _T("analog"));
		if (szValue) {
			nAnalogSpeed = _tcstol(szValue, NULL, 0);
		}

		if (nConfigMinVersion <= nFileVersion && nFileVersion <= nBurnVer) {
			szValue = LabelCheck(szLine, _T("input"));
			if (szValue) {
				TCHAR* szQuote = NULL;
				TCHAR* szEnd = NULL;
				if (QuoteRead(&szQuote, &szEnd, szValue)) {
					continue;
				}

				if ((szQuote[0] == _T('p') || szQuote[0] == _T('P')) && szQuote[1] >= _T('1') && szQuote[1] <= _T('0') + nMaxPlayers && szQuote[2] == _T(' ')) {
					if (szQuote[1] != _T('1') + nPlayer) {
						continue;
					}
				}
				else {
					if (nPlayer != 0) {
						continue;
					}
				}

				// Find which input number this refers to
				i = InputNameToNum(szQuote);
				if (i == ~0U) {
					i = InputInfoToNum(szQuote);
					if (i == ~0U) {
						continue;
					}
				}

				if (GameInp[i].pcInput == 0 || bOverWrite) {				// Undefined - assign mapping
					StringToInp(GameInp + i, szEnd);
				}
			}

			szValue = LabelCheck(szLine, _T("macro"));
			if (szValue) {
				TCHAR* szQuote = NULL;
				TCHAR* szEnd = NULL;
				if (QuoteRead(&szQuote, &szEnd, szValue)) {
					continue;
				}

				i = MacroNameToNum(szQuote);
				if (i != ~0U) {
					i += nGameInpCount;
					if (GameInp[i].Macro.nMode == 0 || bOverWrite) {	// Undefined - assign mapping
						StringToMacro(GameInp + i, szEnd);
					}
				}
			}

			szValue = LabelCheck(szLine, _T("custom"));
			if (szValue) {
				AddCustomMacro(szValue, bOverWrite);
			}
		}
	}

	fclose(h);

	return 0;
}

INT32 ConfigGameLoadHardwareDefaults()
{
	TCHAR* szDefaultCpsFile = _T("config/presets/cps.ini");
	TCHAR* szDefaultNeogeoFile = _T("config/presets/neogeo.ini");
	TCHAR* szDefaultNESFile = _T("config/presets/nes.ini");
	TCHAR* szDefaultFDSFile = _T("config/presets/fds.ini");
	TCHAR* szDefaultPgmFile = _T("config/presets/pgm.ini");
	TCHAR* szFileName = _T("");
	INT32 nApplyHardwareDefaults = 0;

	INT32 nHardwareFlag = (BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK);

	if (nHardwareFlag == HARDWARE_CAPCOM_CPS1 || nHardwareFlag == HARDWARE_CAPCOM_CPS1_QSOUND || nHardwareFlag == HARDWARE_CAPCOM_CPS1_GENERIC || nHardwareFlag == HARDWARE_CAPCOM_CPSCHANGER || nHardwareFlag == HARDWARE_CAPCOM_CPS2 || nHardwareFlag == HARDWARE_CAPCOM_CPS3) {
		szFileName = szDefaultCpsFile;
		nApplyHardwareDefaults = 1;
	}

	if (nHardwareFlag == HARDWARE_SNK_NEOGEO || nHardwareFlag == HARDWARE_SNK_NEOCD) {
		szFileName = szDefaultNeogeoFile;
		nApplyHardwareDefaults = 1;
	}

	if (nHardwareFlag == HARDWARE_NES) {
		szFileName = szDefaultNESFile;
		nApplyHardwareDefaults = 1;
	}

	if (nHardwareFlag == HARDWARE_FDS) {
		szFileName = szDefaultFDSFile;
		nApplyHardwareDefaults = 1;
	}

	if (nHardwareFlag == HARDWARE_IGS_PGM) {
		szFileName = szDefaultPgmFile;
		nApplyHardwareDefaults = 1;
	}

	if (nApplyHardwareDefaults) {
		for (INT32 nPlayer = 0; nPlayer < nMaxPlayers; nPlayer++) {
			GameInputAutoIni(nPlayer, szFileName, true);
		}
	}

	return 0;
}


// --------------------------------------------------------------------------------
INT32 GetDefaultPlayerMappingsForGame(GameInputGroup& playerInputs) {

	// These are describing the default inputs for the game, NOT the gamepad!
	// NOTE: This is for sfiii3nr1 ONLY!
	ZeroMemory(&playerInputs, sizeof(GameInputGroup));

	// NOTE: The inputs in this case map 1:1 to the indexes for the player inputs
	// in the driver....
	// See the defintion of 'cps3InputList' in 'd_cps3.cpp' for an example.

	// playerInputs.inputCount = 16;
	playerInputs.Inputs[0] = { GPINPUT_BACK, 0 };
	playerInputs.Inputs[1] = { GPINPUT_START, 1 };

	// The directinal inputs need to be mapped onto the DRIVER inputs somehow....
	// The code that does the SOCD stuff just looks for the BurnInputInfo.szInfo value to decide
	// what is up/down/left/right..  IMO, not the most robust way, but it can work....
	// POV HAT (0x10) (NOTE: This is where we would find a way to map multiple inputs.)
	playerInputs.Inputs[2] = { GPINPUT_DPAD_UP, 2 };			// Up			
	playerInputs.Inputs[3] = { GPINPUT_DPAD_DOWN, 3 };			// Down		
	playerInputs.Inputs[4] = { GPINPUT_DPAD_LEFT, 4 };			// Left		
	playerInputs.Inputs[5] = { GPINPUT_DPAD_RIGHT, 5 };			// Right		

	// ANALOG STICK (0x0)
	playerInputs.Inputs[6] = { GPINPUT_LSTICK_UP, 2 };			// Up			
	playerInputs.Inputs[7] = { GPINPUT_LSTICK_DOWN, 3 };			// Down		
	playerInputs.Inputs[8] = { GPINPUT_LSTICK_LEFT, 4 };			// Left		
	playerInputs.Inputs[9] = { GPINPUT_LSTICK_RIGHT, 5 };		// Right		

	playerInputs.Inputs[10] = { GPINPUT_X, 6 };
	playerInputs.Inputs[11] = { GPINPUT_Y, 7 };
	playerInputs.Inputs[12] = { GPINPUT_RIGHT_BUMPER, 8 };
	playerInputs.Inputs[13] = { GPINPUT_A, 9 };
	playerInputs.Inputs[14] = { GPINPUT_B, 10 };
	playerInputs.Inputs[15] = { GPINPUT_RIGHT_TRIGGER, 11 };

	playerInputs.InputCount = 16;

	return 0;
}

// --------------------------------------------------------------------------------
// Given a producxt guid, this will lookup alternate mappings tables.
INT32 GetGamepadMapping(GUID& productGuid, GamepadInputProfileEx& gpp) {

	// NOTE: The mappings that we create a game dependent.  A check
	// for this might need to take place at some point.
	GameInputGroup playerInputs;
	GetDefaultPlayerMappingsForGame(playerInputs);

	// Get the device specific mappings.
	CGamepadButtonMapping mapping;
	if (!GamepadDatabase.TryGetMapping(productGuid, mapping))
	{
		GamepadDatabase.GetDefaultMapping(mapping);
	}


	// Now, with the set of inputs, and the gamepad mappings, we can populate the input profile!
	ZeroMemory(&gpp, sizeof(GamepadInputProfileEx));
	gpp.inputCount = playerInputs.InputCount;
	for (size_t i = 0; i < gpp.inputCount; i++)
	{
		GamepadInputDesc inputDesc = playerInputs.Inputs[i];

		const CGamepadMappingEntry* e = mapping.GetMappingFor(inputDesc.Input);
		if (e) {
			gpp.inputs[i] = { e->Type, e->Index };
		}
		else {
			gpp.inputs[i] = { ITYPE_UNSET, 0 };
		}
	}

	return 0;
}

// --------------------------------------------------------------------------------
// Since we almost always play on gamepads / joysticks, we will auto detect +
// set those inputs.  This should happen as controllers are plugged in / out.
// Because of the janky input system, this isn't as easy as it may seem....
// In reality, inputs shouldn't be a loose collection of 'buttons', rather there
// should be a device associated with each player....  A 'mixed-mode' device could even
// exist that could be mouse + keyboard + etc.
INT32 SetDefaultGamepadInputs() {
	UseGameInputSetForPCInputs = false;

	// NOTE: Check for specific game that supports this notion.
	// TODO: Some way to list the games that support this feature.  Probably
	// something to add to the drivers, or metadriver system.
	char* drvName = 0;
	if (nBurnDrvActive != NOT_ACTIVE) {
		drvName = BurnDrvGetTextA(DRV_NAME);
	}
	if (drvName == nullptr || strcmp(drvName, "sfiii3nr1"))
	{
		return 0;
	}

	// NOTE: Players should be listed in order.
	// So first GroupType == IGROUP_PLAYER = Player 1, etc.
	GameInputGroup p1;
	GetDefaultPlayerMappingsForGame(p1);
	p1.BurnInputStartIndex = 0;
	p1.GroupType = IGROUP_PLAYER;

	GameInputGroup p2;
	GetDefaultPlayerMappingsForGame(p2);
	p2.BurnInputStartIndex = 12;
	p2.GroupType = IGROUP_PLAYER;

	// TODO: Finish mappings for system.  This means that we have to also support
	// keyboard inputs, properly.  ATM we are really only caring about gamepad stuff,
	// which is OK, but won't work long term.
	GameInputGroup sys;
	sys.BurnInputStartIndex = 24;
	sys.InputCount = 5;
	sys.Inputs[0] = { (EGamepadInput)(GPINPUT_KEYB | FBK_F2), 0 };
	sys.Inputs[1] = { (EGamepadInput)(GPINPUT_KEYB | FBK_F3), 1 };
	sys.Inputs[2] = { (EGamepadInput)(GPINPUT_KEYB | FBK_9), 2 };
	sys.Inputs[3] = { GPINPUT_UNSUPPORTED, 3 };
	sys.Inputs[4] = { GPINPUT_UNSUPPORTED, 4 };

	// Let's make a input set for third strike!
	CGameInputSet iSet;
	iSet.MaxPlayerCount = 2;
	iSet.GroupCount = 3;
	iSet.InputGroups[0] = p1;
	iSet.InputGroups[1] = p2;
	iSet.InputGroups[2] = sys;

	// Assign the datas...
	GameInputSet = iSet;
	UseGameInputSetForPCInputs = true;

	// We grab all of the currently plugged gamepads, and use those, up to max
	// players for the game to get the inputs mapped.
	GamepadFileEntry* pads[MAX_GAMEPADS];
	UINT32 nPadCount = 0;
	InputGetGamepads(pads, &nPadCount);

	UINT32 usePlayerCount = nPadCount;
	if (GameInputSet.MaxPlayerCount < usePlayerCount) {
		usePlayerCount = GameInputSet.MaxPlayerCount;
	}


	UINT32 padsSet = 0;
	for (size_t i = 0; i < usePlayerCount; i++)
	{
		// NOTE: Based on the product guid, we need to add some mapping data.. (indexes are not 1:1 across all pads)
		// In this case we would directly modify gpp to have the correct offsets.....
		// We will start with a hard-coded set, and then later pull it from disk...
		GUID& prodGuid = pads[i]->info.guidProduct;

		GamepadInputProfileEx gpp;
		GetGamepadMapping(prodGuid, gpp);


		// NOTE: This is where we can check the system guid, or whatever... to choose a user-specific mapping.
		// CopyPadInputsToGameInputs(i, gpp);

		++padsSet;
	}

	if (padsSet < usePlayerCount) {
		// TODO:
		// Clear or otherwise set the other player inputs p.x... to default keyboard inputs.
		int abc = 10;
	}
	//pInputInOut[nInputSelect]

	return 0;
}

// --------------------------------------------------------------------------------
INT32 CopyPadInputsToGameInputs(int playerIndex, GamepadInputProfileEx& gpp) {

	// OK, this is the heart of the multi-direction update.
	// FBNEO drivers don't let us multi-map inputs, so it is up to us to figure
	// out how we are going to do this....
	// The big issue, is that they are double-dipping the game inputs, and use them as the PC inputs,
	// in the same structure / memory area.  This is what needs to be fixed.
	// PC inputs can be more exotic, and then get mapped onto the games that we are emulating...
	//throw (std::exception("figure out how we are going to do multi-inputs for directions...."));

	if (!GameInp) { return 0; }


	if (UseGameInputSetForPCInputs) {
		// Run through all of the GameInput sets, and merge / copy the values over to the GameInp.pcInput stuf.....
		int x = 10;
	}


	auto offset = playerIndex * gpp.inputCount;

	auto pGameInput = GameInp + offset;
	for (size_t i = 0; i < gpp.inputCount; i++)
	{
		auto& ci = gpp.inputs[i];
		if (ci.type == ITYPE_UNSET) { continue; }

		UINT16 code = ci.GetBurnerCode();
		code = code | (playerIndex << 8);

		// Ensure this is interpreted as joystick code....
		code |= JOYSTICK_LOWER;

		auto useInp = (pGameInput + i);
		UINT8 pcInput = useInp->pcInput;

		if (pcInput == GIT_SWITCH) {
			useInp->Input.Switch.nCode = code;
		}
		else if (pcInput & GIT_GROUP_JOYSTICK) {
			// Joysticks / analogs don't have a code, it is all encoded in the nType data....
			int x = 10;
		}

	}

	return 0;
}

// --------------------------------------------------------------------------------
// From the BurnInputInfo.szInfo member, determine the default group index.
// typically, system = 0, p1 = 1, p2 = 2, etc.
INT32 GetGroupIndex(char* szInfo) {
	if (szInfo[0] == 'p') {
		// If the next value is a number.....
		char index = szInfo[1];
		if (index >= 48 && index <= 57) {
			return index - 48;
		}
	}

	// Default / system.
	return 0;
}

// --------------------------------------------------------------------------------
EGamepadInput TranslatePCInput(struct GameInp* pgi) {

	UINT8 i = pgi->pcInput;


	switch (pgi->pcInput) {
	case GIT_SWITCH:
	{
		UINT16 code = pgi->Input.Switch.nCode;


		if (code >= MOUSE_LOWER) {
			// TEMP: We haven't considered a mouse yet.....
			return EGamepadInput::GPINPUT_UNSUPPORTED;
		}

		else if (code >= JOYSTICK_LOWER && code < JOYSTICK_UPPER) {
			// return EGamepadInput::
			UINT16 gpCode = code & 0xFF;
			UINT16 joyIndex = (code >> 8) & 0x3F;

			if ((gpCode & BURNER_BUTTON) == BURNER_BUTTON) {
				// NOTE: We can't really say what button is what...
				// because of gamepad mappings, so maybe just punt?
				size_t useCode = gpCode ^ BURNER_BUTTON;
				EGamepadInput res = (EGamepadInput)(GPINPUT_PAD_BUTTONS | (useCode + 1));
				return res;
			}
			else if ((gpCode & BURNER_DPAD) == BURNER_DPAD) {
				size_t useCode = gpCode ^ BURNER_DPAD;
				EGamepadInput res = (EGamepadInput)(GPINPUT_PAD_DPAD | (useCode + 1));
				return res;
			}
			else if (gpCode < BURNER_DPAD) {
				// Analog inputs.
				EGamepadInput res = (EGamepadInput)(GPINPUT_PAD_ANALOG | (gpCode + 1));
				return res;
			 }


			int z = 10;
		}

		else {

			// NOTE: We shouldn't have to scan every frikkin key....
			// Like maybe organize the key indexes by nCode?
			for (INT32 i = 0; KeyNames[i].nCode; i++) {
				if (code == KeyNames[i].nCode) {
					EGamepadInput res = (EGamepadInput)(GPINPUT_KEYB | code);
					return res;
				}
			}

		}

		// NOT FOUND!
		break;
	}
	default:
		return EGamepadInput::GPINPUT_UNSUPPORTED;
		break;
	}


	return EGamepadInput::GPINPUT_UNSUPPORTED;
}

// --------------------------------------------------------------------------------
INT32 CreateDefaultInputSet() {

	struct GameInp* pgi;
	struct BurnInputInfo bii;
	UINT32 i;

	// nMax
	ZeroMemory(&GameInputSet, sizeof(CGameInputSet));
	GameInputSet.GroupCount = nMaxPlayers + 1;

	if (nMaxPlayers > MAX_PLAYERS) {
		throw std::exception("Max players exceeded!");
	}

	auto& sysGroup = GameInputSet.InputGroups[0];
	sysGroup.GroupType = EInputGroupType::IGROUP_SYSTEM;

	for (size_t i = 0; i < nMaxPlayers; i++)
	{
		GameInputSet.InputGroups[i + 1].GroupType = IGROUP_PLAYER;
	}

	// NOTE: In a better, brighter world, we would have a driver system that would
	// just allow us to assing the groups along with the other defs.
	for (i = 0, pgi = GameInp; i < nGameInpCount; i++, pgi++) {

		// Get the extra info about the input
		bii.szInfo = NULL;
		BurnDrvGetInputInfo(&bii, i);
		if (bii.pVal == NULL) {
			continue;
		}

		int groupIndex = GetGroupIndex(bii.szInfo);
		auto& set = GameInputSet.InputGroups[groupIndex];

		GamepadInputDesc gpDesc;
		gpDesc.GameInputIndex = i;
		gpDesc.Input = TranslatePCInput(pgi);

		set.Inputs[set.InputCount] = gpDesc;
		set.InputCount += 1;

	}

	return 0;
}


// --------------------------------------------------------------------------------
// Auto-configure any undefined inputs to defaults
// NOTE: This is where we could properly apply default mappings for joysticks / assigned players.
INT32 SetDefaultGameInputs()
{
	struct GameInp* pgi;
	struct BurnInputInfo bii;
	UINT32 i;

	//for (INT32 nPlayer = 0; nPlayer < nMaxPlayers; nPlayer++) {

	//	if ((nPlayerDefaultControls[nPlayer] & 0x0F) != 0x0F) {
	//		continue;
	//	}

	//	GameInputAutoIni(nPlayer, szPlayerDefaultIni[nPlayer], false);
	//}

	// Fill all inputs still undefined
	for (i = 0, pgi = GameInp; i < nGameInpCount; i++, pgi++) {
		if (pgi->pcInput) {											// Already defined - leave it alone
			continue;
		}

		// Get the extra info about the input
		bii.szInfo = NULL;
		BurnDrvGetInputInfo(&bii, i);
		if (bii.pVal == NULL) {
			continue;
		}
		if (bii.szInfo == NULL) {
			bii.szInfo = "";
		}

		// Dip switches - set to constant
		if (bii.nType & BIT_GROUP_CONSTANT) {
			pgi->pcInput = GIT_CONSTANT;
			continue;
		}

		GameInpAutoOne(pgi, bii.szInfo);
	}

	// Fill in macros still undefined
	for (i = 0; i < nMacroCount; i++, pgi++) {
		if (pgi->pcInput != GIT_MACRO_AUTO || pgi->Macro.nMode) {	// Already defined - leave it alone
			continue;
		}

		GameInpAutoOne(pgi, pgi->Macro.szName);
	}


	// Now that we have our defaults, we can populate the input set.
	// NOTE: The code above, where the calls to GameInpAutoOne are made,
	// can eventually be folded into the code for creating the deafault input set.  We
	// can probably find a way to greatly reduce its complexity at that time as well.
	CreateDefaultInputSet();


	return 0;
}

// ---------------------------------------------------------------------------
// Write all the GameInps out to config file 'h'

INT32 GameInpWrite(FILE* h)
{
	// Write input types
	for (UINT32 i = 0; i < nGameInpCount; i++) {
		TCHAR* szName = NULL;
		INT32 nPad = 0;
		szName = InputNumToName(i);
		_ftprintf(h, _T("input  \"%s\" "), szName);
		nPad = 16 - _tcslen(szName);
		for (INT32 j = 0; j < nPad; j++) {
			_ftprintf(h, _T(" "));
		}
		_ftprintf(h, _T("%s\n"), InpToString(GameInp + i));
	}

	_ftprintf(h, _T("\n"));

	struct GameInp* pgi = GameInp + nGameInpCount;
	for (UINT32 i = 0; i < nMacroCount; i++, pgi++) {
		INT32 nPad = 0;

		if (pgi->pcInput & GIT_GROUP_MACRO) {
			switch (pgi->pcInput) {
			case GIT_MACRO_AUTO:									// Auto-assigned macros
				_ftprintf(h, _T("macro  \"%hs\" "), pgi->Macro.szName);
				break;
			case GIT_MACRO_CUSTOM:									// Custom macros
				_ftprintf(h, _T("custom \"%hs\" "), pgi->Macro.szName);
				break;
			default:												// Unknown -- ignore
				continue;
			}

			nPad = 16 - strlen(pgi->Macro.szName);
			for (INT32 j = 0; j < nPad; j++) {
				_ftprintf(h, _T(" "));
			}
			_ftprintf(h, _T("%s\n"), InpMacroToString(pgi));
		}
	}

	return 0;
}

// ---------------------------------------------------------------------------

// Read a GameInp in
INT32 GameInpRead(TCHAR* szVal, bool bOverWrite)
{
	INT32 nRet;
	TCHAR* szQuote = NULL;
	TCHAR* szEnd = NULL;
	UINT32 i = 0;

	nRet = QuoteRead(&szQuote, &szEnd, szVal);
	if (nRet) {
		return 1;
	}

	// Find which input number this refers to
	i = InputNameToNum(szQuote);
	if (i == ~0U) {
		return 1;
	}

	if (bOverWrite || GameInp[i].pcInput == 0) {
		// Parse the input description into the GameInp structure
		StringToInp(GameInp + i, szEnd);
	}

	return 0;
}

INT32 GameInpMacroRead(TCHAR* szVal, bool bOverWrite)
{
	INT32 nRet;
	TCHAR* szQuote = NULL;
	TCHAR* szEnd = NULL;
	UINT32 i = 0;

	nRet = QuoteRead(&szQuote, &szEnd, szVal);
	if (nRet) {
		return 1;
	}

	i = MacroNameToNum(szQuote);
	if (i != ~0U) {
		i += nGameInpCount;
		if (GameInp[i].Macro.nMode == 0 || bOverWrite) {
			StringToMacro(GameInp + i, szEnd);
		}
	}

	return 0;
}

INT32 GameInpCustomRead(TCHAR* szVal, bool bOverWrite)
{
	return AddCustomMacro(szVal, bOverWrite);
}

void GameInpUpdatePrev(bool bCopy)
{
	// Set prev data
	for (INT32 i = 0; i < 2; i++) {
		for (INT32 j = 0; j < 4; j++) {
			SetInpPrev(i, j, GetInpFrame(i, j));
		}
	}
}


void GameInpUpdateNext(bool bCopy)
{
	// Fix from next frame
	for (INT32 i = 0; i < 2; i++) {
		for (INT32 j = 0; j < 4; j++) {
			if (GetInpNext(i, j)) {
				SetInpFrame(i, j, 1, bCopy);
			}
			SetInpNext(i, j, 0);
		}
	}
}

void GameInpClearOpposites(bool bCopy)
{
	if (kNetVersion >= NET_VERSION_SOCD && (bClearOpposites || nEnableSOCD > 0)) {
		if (nEnableSOCD == 2) {
			// Hitbox SOCD cleaner
			for (INT32 i = 0; i < 2; i++) {
				// D + U = U || (no matter the state of L and R)
				if (GetInpFrame(i, UP) && GetInpFrame(i, DOWN)) {
					SetInpFrame(i, DOWN, 0, bCopy);
				}
				// L + R = neutral
				if (GetInpFrame(i, LEFT) && GetInpFrame(i, RIGHT)) {
					SetInpFrame(i, LEFT, 0, bCopy);
					SetInpFrame(i, RIGHT, 0, bCopy);
				}
			}
		}
		else {
			// Regular SOCD cleaner
			for (INT32 i = 0; i < 2; i++) {
				// D + U = neutral
				if (GetInpFrame(i, UP) && GetInpFrame(i, DOWN)) {
					SetInpFrame(i, UP, 0, bCopy);
					SetInpFrame(i, DOWN, 0, bCopy);
				}
				// L + R = neutral
				if (GetInpFrame(i, LEFT) && GetInpFrame(i, RIGHT)) {
					SetInpFrame(i, LEFT, 0, bCopy);
					SetInpFrame(i, RIGHT, 0, bCopy);
				}
			}
		}
	}
}

void GameInpFixDiagonals(bool bCopy)
{
	if (kNetVersion >= NET_VERSION_FIX_DIAGONALS && bFixDiagonals) {
		for (INT32 i = 0; i < 2; i++) {
			// D + L
			if (GetInpFrame(i, DOWN) && GetInpFrame(i, LEFT)) {

				if (GetInpPrev(i, DOWN) && GetInpPrev(i, RIGHT)) {
					SetInpFrame(i, LEFT, 0, bCopy);
					SetInpNext(i, DOWN, 1);
					SetInpNext(i, LEFT, 1);
					//VidOverlayAddChatLine(_T("System"), _T("DR -> (D) -> DL"));
				}
				else if (GetInpPrev(i, UP) && GetInpPrev(i, LEFT)) {
					SetInpFrame(i, DOWN, 0, bCopy);
					SetInpNext(i, LEFT, 1);
					SetInpNext(i, DOWN, 1);
					//VidOverlayAddChatLine(_T("System"), _T("UL -> (L) -> DL"));
				}

			}
			// D + R
			else if (GetInpFrame(i, DOWN) && GetInpFrame(i, RIGHT)) {
				if (GetInpPrev(i, DOWN) && GetInpPrev(i, LEFT)) {
					SetInpFrame(i, RIGHT, 0, bCopy);
					SetInpNext(i, DOWN, 1);
					SetInpNext(i, RIGHT, 1);
					//VidOverlayAddChatLine(_T("System"), _T("DL -> (D) -> DR"));
				}
				else if (GetInpPrev(i, UP) && GetInpPrev(i, RIGHT)) {
					SetInpFrame(i, DOWN, 0, bCopy);
					SetInpNext(i, RIGHT, 1);
					SetInpNext(i, DOWN, 1);
					//VidOverlayAddChatLine(_T("System"), _T("UR -> (R) -> DR"));
				}

			}
			// U + L
			else if (GetInpFrame(i, UP) && GetInpFrame(i, LEFT)) {
				if (GetInpPrev(i, UP) && GetInpPrev(i, RIGHT)) {
					SetInpFrame(i, LEFT, 0, bCopy);
					SetInpNext(i, UP, 1);
					SetInpNext(i, LEFT, 1);
					//VidOverlayAddChatLine(_T("System"), _T("UR -> (U) -> UL"));
				}
				else if (GetInpPrev(i, DOWN) && GetInpPrev(i, LEFT)) {
					SetInpFrame(i, UP, 0, bCopy);
					SetInpNext(i, LEFT, 1);
					SetInpNext(i, UP, 1);
					//VidOverlayAddChatLine(_T("System"), _T("DL -> (L) -> UL"));
				}
			}
			// D + R
			else if (GetInpFrame(i, UP) && GetInpFrame(i, RIGHT)) {
				if (GetInpPrev(i, UP) && GetInpPrev(i, LEFT)) {
					SetInpFrame(i, RIGHT, 0, bCopy);
					SetInpNext(i, UP, 1);
					SetInpNext(i, RIGHT, 1);
					//VidOverlayAddChatLine(_T("System"), _T("UL -> (U) -> UR"));
				}
				else if (GetInpPrev(i, DOWN) && GetInpPrev(i, RIGHT)) {
					SetInpFrame(i, UP, 0, bCopy);
					SetInpNext(i, RIGHT, 1);
					SetInpNext(i, UP, 1);
					//VidOverlayAddChatLine(_T("System"), _T("DR -> (R) -> UR"));
				}
			}
		}
	}
}
