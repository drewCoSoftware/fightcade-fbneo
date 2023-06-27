// Burner Input Editor Dialog module
// NOTE: This is the <strike>really lousy</strike> war crime of a mapping + preset dialog that comes with the emulator.
// This is where we will make the UI less stupid.
#include "burner.h"

HWND hInpdDlg = NULL;							// Handle to the Input Dialog
static HWND hInpdList = NULL;
static HWND hGamepadList = NULL;

static unsigned short* LastVal = NULL;			// Last input values/defined
static int bLastValDefined = 0;					//

static HWND hInpdGi = NULL, hInpdPci = NULL, hInpdAnalog = NULL;	// Combo boxes
static HWND hP1Select;
static HWND hP2Select;

// Get the currently plugged gamepad data:
// NOTE:  We should just get a pointer to this so that we can set the alias directly.
static GamepadFileEntry* padInfos[8];
static INT32 nPadCount;
static int nSelectedPadIndex = -1;


// Text buffer for the gamepad alias.
static TCHAR aliasBuffer[MAX_ALIAS_CHARS];
GamepadFileEntry* selectedPadEntry = NULL;

static playerInputs sfiii3nPlayerInputs;

// Update which input is using which PC input
static int InpdUseUpdate()
{
	unsigned int i, j = 0;
	struct GameInp* pgi = NULL;
	if (hInpdList == NULL) {
		return 1;
	}

	// Update the values of all the inputs
	for (i = 0, pgi = GameInp; i < nGameInpCount; i++, pgi++) {
		LVITEM LvItem;
		TCHAR* pszVal = NULL;

		if (pgi->Input.pVal == NULL) {
			continue;
		}

		pszVal = InpToDesc(pgi);

		if (_tcscmp(pszVal, _T("code 0x00")) == 0)
			pszVal = _T("Unassigned (locked)");

		memset(&LvItem, 0, sizeof(LvItem));
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = j;
		LvItem.iSubItem = 1;
		LvItem.pszText = pszVal;

		SendMessage(hInpdList, LVM_SETITEM, 0, (LPARAM)&LvItem);

		j++;
	}

	for (i = 0, pgi = GameInp + nGameInpCount; i < nMacroCount; i++, pgi++) {
		LVITEM LvItem;
		TCHAR* pszVal = NULL;

		if (pgi->nInput & GIT_GROUP_MACRO) {
			pszVal = InpMacroToDesc(pgi);

			if (_tcscmp(pszVal, _T("code 0x00")) == 0)
				pszVal = _T("Unassigned (locked)");

			memset(&LvItem, 0, sizeof(LvItem));
			LvItem.mask = LVIF_TEXT;
			LvItem.iItem = j;
			LvItem.iSubItem = 1;
			LvItem.pszText = pszVal;

			SendMessage(hInpdList, LVM_SETITEM, 0, (LPARAM)&LvItem);
		}

		j++;
	}

	return 0;
}

int InpdUpdate()
{
	unsigned int i, j = 0;
	struct GameInp* pGameInput = NULL;			// Pointer to the game input.
	unsigned short* pLastVal = NULL;			// Pointer to the 'last value' data.
	unsigned short nThisVal;
	if (hInpdList == NULL) {
		return 1;
	}
	if (LastVal == NULL) {
		return 1;
	}

	// Update the values of all the inputs.
	// Note that this loop is incrementing the pointer addresses to enumerate.  'i' is only used as a control.
	for (i = 0, pGameInput = GameInp, pLastVal = LastVal; i < nGameInpCount; i++, pGameInput++, pLastVal++) {
		LVITEM LvItem;
		TCHAR szVal[16];

		if (pGameInput->nType == 0) {
			continue;
		}

		if (pGameInput->nType & BIT_GROUP_ANALOG) {
			if (bRunPause) {														// Update LastVal
				nThisVal = pGameInput->Input.nVal;
			}
			else {
				nThisVal = *pGameInput->Input.pShortVal;
			}

			// Continue if input state hasn't changed.
			if (bLastValDefined && (pGameInput->nType != BIT_ANALOG_REL || nThisVal) && pGameInput->Input.nVal == *pLastVal) {
				j++;
				continue;
			}

			*pLastVal = nThisVal;
		}
		else {
			if (bRunPause) {														// Update LastVal
				nThisVal = pGameInput->Input.nVal;
			}
			else {
				nThisVal = *pGameInput->Input.pVal;
			}

			// Continue if input state hasn't changed.
			if (bLastValDefined && pGameInput->Input.nVal == *pLastVal) {
				j++;
				continue;
			}

			*pLastVal = nThisVal;
		}

		switch (pGameInput->nType) {
		case BIT_DIGITAL: {
			if (nThisVal == 0) {
				szVal[0] = 0;
			}
			else {
				if (nThisVal == 1) {
					// NOTE: This is where the input check method appears....
					_tcscpy(szVal, _T("ON"));
				}
				else {
					_stprintf(szVal, _T("0x%02X"), nThisVal);
				}
			}
			break;
		}
		case BIT_ANALOG_ABS: {
			_stprintf(szVal, _T("0x%02X"), nThisVal >> 8);
			break;
		}
		case BIT_ANALOG_REL: {
			if (nThisVal == 0) {
				szVal[0] = 0;
			}
			if ((short)nThisVal < 0) {
				_stprintf(szVal, _T("%d"), ((short)nThisVal) >> 8);
			}
			if ((short)nThisVal > 0) {
				_stprintf(szVal, _T("+%d"), ((short)nThisVal) >> 8);
			}
			break;
		}
		default: {
			_stprintf(szVal, _T("0x%02X"), nThisVal);
		}
		}

		memset(&LvItem, 0, sizeof(LvItem));
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = j;
		LvItem.iSubItem = 2;
		LvItem.pszText = szVal;

		SendMessage(hInpdList, LVM_SETITEM, 0, (LPARAM)&LvItem);

		j++;
	}

	bLastValDefined = 1;										// LastVal is now defined

	return 0;
}

#define ALIAS_INDEX 0
#define STATE_INDEX 1
#define GUID_INDEX 2

// ---------------------------------------------------------------------------------------------------------
static void SetEnabled(int id, BOOL bEnable)
{
	EnableWindow(GetDlgItem(hInpdDlg, id), bEnable);
}

// ---------------------------------------------------------------------------------------------------------
static int GamepadListBegin()
{
	if (hGamepadList == NULL) {
		return 1;
	}

	// Full row select style:
	SendMessage(hGamepadList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);


	// Make column headers
	LVCOLUMN LvCol;
	memset(&LvCol, 0, sizeof(LvCol));
	LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;



	LvCol.cx = 0x95;		// Column Width.
	LvCol.pszText = FBALoadStringEx(hAppInst, IDS_GAMEPAD_ALIAS, true);
	SendMessage(hGamepadList, LVM_INSERTCOLUMN, ALIAS_INDEX, (LPARAM)&LvCol);

	LvCol.cx = 0x38;
	LvCol.pszText = FBALoadStringEx(hAppInst, IDS_INPUT_STATE, true);
	SendMessage(hGamepadList, LVM_INSERTCOLUMN, STATE_INDEX, (LPARAM)&LvCol);

	LvCol.cx = 0xa5;		// Column Width.
	LvCol.pszText = FBALoadStringEx(hAppInst, IDS_GAMEPAD_GUID, true);
	SendMessage(hGamepadList, LVM_INSERTCOLUMN, GUID_INDEX, (LPARAM)&LvCol);


	//InputInit();
	return 0;
}

// ---------------------------------------------------------------------------------------------------------
void SetStrBuffer(TCHAR* buffer, int bufLen, TCHAR* data) {
	int len = wcslen(data);
	memset(buffer, 0, bufLen);
	if (len >= bufLen) {
		len = bufLen - 1;
	}
	for (size_t i = 0; i < len; i++)
	{
		buffer[i] = data[i];
	}
}

// ---------------------------------------------------------------------------------------------------------
static void updateListboxAlias(HWND& list, int index, TCHAR* buffer, int bufSize, bool insertNew) {

	// TODO: Make sure the index exists....
	LVITEM LvItem;
	memset(&LvItem, 0, sizeof(LvItem));
	LvItem.mask = LVIF_TEXT | LVIF_PARAM;
	LvItem.iItem = index;
	LvItem.iSubItem = ALIAS_INDEX;
	LvItem.pszText = buffer; // _T("ALIAS");  // TODO: Alias data will come when we populate the gamepad data when the dialog opens.
	LvItem.lParam = (LPARAM)index;
	SendMessage(list, insertNew ? LVM_INSERTITEM : LVM_SETITEM, 0, (LPARAM)&LvItem);

}

// ---------------------------------------------------------------------------------------------------------
static int GamepadListMake(int bBuild) {
	//	return 0; //

	HWND& list = hGamepadList;
	if (list == NULL) {
		return 1;
	}

	if (bBuild) {
		SendMessage(list, LVM_DELETEALLITEMS, 0, 0);
	}


	// Populate the list:
	for (unsigned int i = 0; i < nPadCount; i++) {
		GamepadFileEntry* pad = padInfos[i];


		if (pad->info.Alias == 0 || wcscmp(_T(""), pad->info.Alias) == 0) {
			SetStrBuffer(aliasBuffer, MAX_ALIAS_CHARS, _T("<not set>"));
		}
		else
		{
			SetStrBuffer(aliasBuffer, MAX_ALIAS_CHARS, pad->info.Alias);
		}

		// Populate the ALIAS column (TODO)
		updateListboxAlias(list, i, aliasBuffer, MAX_ALIAS_CHARS, true);


		LVITEM LvItem;

		// Populate the STATE column (empty is fine!)
		// This gets set when we detect input from an attached gamepad!
		memset(&LvItem, 0, sizeof(LvItem));
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = i;
		LvItem.iSubItem = STATE_INDEX;
		LvItem.pszText = _T("STATE");
		SendMessage(list, LVM_SETITEM, 0, (LPARAM)&LvItem);

		// Populate the GUID column
		memset(&LvItem, 0, sizeof(LvItem));
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = i;
		LvItem.iSubItem = GUID_INDEX;
		LvItem.pszText = GUIDToTCHAR(&pad->info.guidInstance);
		SendMessage(list, LVM_SETITEM, 0, (LPARAM)&LvItem);

	}


	return 0;
}


// ------------------------------------------------------------------------------------------------------
static int OnGamepadListDeselect()
{
	// Clear the alias input box.
	SendDlgItemMessage(hInpdDlg, IDC_ALIAS_EDIT, WM_SETTEXT, (WPARAM)0, (LPARAM)NULL);
	nSelectedPadIndex = -1;
	selectedPadEntry = NULL;
	SetEnabled(IDSAVEALIAS, FALSE);
	SetEnabled(ID_SAVE_MAPPINGS, FALSE);
	return 0;
}

// ------------------------------------------------------------------------------------------------------
void getListItemData(HWND& list, int index, LVITEM& item) {

	item.mask = LVIF_TEXT;
	item.iItem = index;
	item.iSubItem = 0;
	item.pszText = aliasBuffer;
	item.cchTextMax = MAX_ALIAS_CHARS;
	SendMessage(list, LVM_GETITEM, 0, (LPARAM)&item);

}


// ------------------------------------------------------------------------------------------------------
static int SetInputMappings(int padIndex, int inputIndexOffset)
{
	if (padIndex == -1 || padIndex >= nPadCount) {
		// Do nothing.
		return 1;
	}

	auto& profile = padInfos[padIndex]->profile;

	// Otherwise we are going to iterate over the game input + mapped inputs and set the data accordingly.
	int i = 0;
	struct GameInp* pgi;
				// Set the correct mem location...
	for (i = 0, pgi = GameInp + inputIndexOffset; i < sfiii3nPlayerInputs.buttonCount; i++, pgi++) {
		if (pgi->Input.pVal == NULL) {
			continue;
		}

		auto& pi = profile.inputs[i];
		if (pi.nInput == 0) { continue; }		// Not set.

		UINT16 code = pi.nCode;
		if (code & 0x4000)
		{
			// This is a gamepad input.  We need to translate its index in order to set the code correctly.
			code = code | (padIndex << 8);

			int checkIndex = (code  >> 8) &0x3F;
			int x = 10;
		}

		// Now we can set this on the pgi input....
		pgi->nInput = pi.nInput;
		if (pi.nInput == GIT_SWITCH){
			pgi->Input.Switch.nCode = code;
		}
		else if (pi.nInput & GIT_GROUP_JOYSTICK) {
			code = 0;		// Joysticks don't have a code, it is all encoded in the nType data....
		}

	}
	// I think that we need to update the description in the main select box too......
	InpdUseUpdate();

	return 0;
}

// ------------------------------------------------------------------------------------------------------
static int SetPlayerMappings()
{
	int p1Index = SendMessage(hP1Select, CB_GETCURSEL, 0, 0);
	int p2Index = SendMessage(hP2Select, CB_GETCURSEL, 0, 0);

	// NOTE: The indexes should correspond to the gamepads that are currently defined.
	// If they aren't, something bad will happen.  We can care about checking this in the future.
	SetInputMappings(p1Index, 0);
	SetInputMappings(p2Index, sfiii3nPlayerInputs.buttonCount);

	return 0;
}



// ------------------------------------------------------------------------------------------------------
static int OnPlayerSelectionChanged() {

	int p1Index = SendMessage(hP1Select, CB_GETCURSEL, 0, 0);
	int p2Index = SendMessage(hP2Select, CB_GETCURSEL, 0, 0);

	if (p1Index == CB_ERR && p2Index == CB_ERR) {
		// Disable the set button and exit!
		SetEnabled(ID_SET_PLAYER_MAPPINGS, false);
		return 1;
	}

	// If there is at least one valid selection, then we can set the mappings.
	SetEnabled(ID_SET_PLAYER_MAPPINGS, true);

	return 0;
}

// ------------------------------------------------------------------------------------------------------
static int SelectGamepadListItem()
{
	HWND& list = hGamepadList;
	LVITEM LvItem;
	memset(&LvItem, 0, sizeof(LvItem));
	int nSel = SendMessage(list, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
	if (nSel < 0) {
		OnGamepadListDeselect();
		return 1;
	}

	// Get the corresponding input
	nSelectedPadIndex = nSel;
	getListItemData(list, nSel, LvItem);

	if (nSel >= nPadCount) {
		OnGamepadListDeselect();
		return 1;
	}

	// Now we can set the text in the alias text box!
	// Set the edit control to current value

	// Enable the save alias button....
	HWND hBtn = GetDlgItem(hInpdDlg, IDSAVEALIAS);
	SetEnabled(IDSAVEALIAS, TRUE);
	SetEnabled(ID_SAVE_MAPPINGS, TRUE);

	TCHAR* useBuffer = wcscmp(aliasBuffer, _T("<not set>")) == 0 ? _T("") : aliasBuffer;
	SendDlgItemMessage(hInpdDlg, IDC_ALIAS_EDIT, WM_SETTEXT, (WPARAM)0, (LPARAM)useBuffer);


}

// ---------------------------------------------------------------------------------------------------------
static int InpdListBegin()
{
	LVCOLUMN LvCol;
	if (hInpdList == NULL) {
		return 1;
	}

	// Full row select style:
	SendMessage(hInpdList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	// Make column headers
	memset(&LvCol, 0, sizeof(LvCol));
	LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

	LvCol.cx = 0xa0;
	LvCol.pszText = FBALoadStringEx(hAppInst, IDS_INPUT_INPUT, true);
	SendMessage(hInpdList, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);

	LvCol.cx = 0xa0;
	LvCol.pszText = FBALoadStringEx(hAppInst, IDS_INPUT_MAPPING, true);
	SendMessage(hInpdList, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol);

	LvCol.cx = 0x38;
	LvCol.pszText = FBALoadStringEx(hAppInst, IDS_INPUT_STATE, true);
	SendMessage(hInpdList, LVM_INSERTCOLUMN, 2, (LPARAM)&LvCol);

	return 0;
}

// ---------------------------------------------------------------------------------------------------------
// Make a list view of the game inputs.
// These are all of the buttons that one could use.
int InpdListMake(int bBuild)
{
	unsigned int j = 0;

	if (hInpdList == NULL) {
		return 1;
	}

	bLastValDefined = 0;
	if (bBuild) {
		SendMessage(hInpdList, LVM_DELETEALLITEMS, 0, 0);
	}

	// Add all the (normal) input names to the list
	for (unsigned int i = 0; i < nGameInpCount; i++) {
		struct BurnInputInfo bii;
		LVITEM LvItem;

		// Get the name of the input
		bii.szName = NULL;
		BurnDrvGetInputInfo(&bii, i);

		// skip unused inputs
		if (bii.pVal == NULL) {
			continue;
		}
		if (bii.szName == NULL) {
			bii.szName = "";
		}

		memset(&LvItem, 0, sizeof(LvItem));
		LvItem.mask = LVIF_TEXT | LVIF_PARAM;
		LvItem.iItem = j;
		LvItem.iSubItem = 0;
		LvItem.pszText = ANSIToTCHAR(bii.szName, NULL, 0);
		LvItem.lParam = (LPARAM)i;

		SendMessage(hInpdList, bBuild ? LVM_INSERTITEM : LVM_SETITEM, 0, (LPARAM)&LvItem);

		j++;
	}

	// Populate the macro related data.
	struct GameInp* pgi = GameInp + nGameInpCount;
	for (unsigned int i = 0; i < nMacroCount; i++, pgi++) {
		LVITEM LvItem;

		if (pgi->nInput & GIT_GROUP_MACRO) {
			memset(&LvItem, 0, sizeof(LvItem));
			LvItem.mask = LVIF_TEXT | LVIF_PARAM;
			LvItem.iItem = j;
			LvItem.iSubItem = 0;
			LvItem.pszText = ANSIToTCHAR(pgi->Macro.szName, NULL, 0);
			LvItem.lParam = (LPARAM)j;

			SendMessage(hInpdList, bBuild ? LVM_INSERTITEM : LVM_SETITEM, 0, (LPARAM)&LvItem);
		}

		j++;
	}

	InpdUseUpdate();

	return 0;
}

static void DisablePresets()
{
	EnableWindow(hInpdPci, FALSE);
	EnableWindow(hInpdAnalog, FALSE);
	EnableWindow(GetDlgItem(hInpdDlg, IDC_INPD_DEFAULT), FALSE);
	EnableWindow(GetDlgItem(hInpdDlg, IDC_INPD_USE), FALSE);
}


// ------------------------------------------------------------------------------------------
static void RefreshPlayerSelectComboBoxes() {
	// TODO: Keep track of current selections?

	// Get the list of active gamepads, and populate each combox box with that data.
	SendMessage(hP1Select, CB_RESETCONTENT, 0, 0);
	SendMessage(hP2Select, CB_RESETCONTENT, 0, 0);

	for (size_t i = 0; i < nPadCount; i++)
	{
		TCHAR* padAlias = padInfos[i]->info.Alias;
		SendMessage(hP1Select, CB_ADDSTRING, 0, (LPARAM)padAlias);
		SendMessage(hP2Select, CB_ADDSTRING, 0, (LPARAM)padAlias);
	}

	// We should only be enabled if one or more selections are currently made.....
	SetEnabled(ID_SET_PLAYER_MAPPINGS, false);
}

// ------------------------------------------------------------------------------------------
static void InitComboboxes()
{
	TCHAR szLabel[1024];
	HANDLE search;
	WIN32_FIND_DATA findData;

	for (int i = 0; i < 4; i++) {
		_stprintf(szLabel, FBALoadStringEx(hAppInst, IDS_INPUT_INP_PLAYER, true), i + 1);
		SendMessage(hInpdGi, CB_ADDSTRING, 0, (LPARAM)szLabel);
	}

	SendMessage(hInpdPci, CB_ADDSTRING, 0, (LPARAM)FBALoadStringEx(hAppInst, IDS_INPUT_INP_KEYBOARD, true));
	for (int i = 0; i < 3; i++) {
		_stprintf(szLabel, FBALoadStringEx(hAppInst, IDS_INPUT_INP_JOYSTICK, true), i);
		SendMessage(hInpdPci, CB_ADDSTRING, 0, (LPARAM)szLabel);
	}
	SendMessage(hInpdPci, CB_ADDSTRING, 0, (LPARAM)FBALoadStringEx(hAppInst, IDS_INPUT_INP_XARCADEL, true));
	SendMessage(hInpdPci, CB_ADDSTRING, 0, (LPARAM)FBALoadStringEx(hAppInst, IDS_INPUT_INP_XARCADER, true));
	SendMessage(hInpdPci, CB_ADDSTRING, 0, (LPARAM)FBALoadStringEx(hAppInst, IDS_INPUT_INP_HOTRODL, true));
	SendMessage(hInpdPci, CB_ADDSTRING, 0, (LPARAM)FBALoadStringEx(hAppInst, IDS_INPUT_INP_HOTRODR, true));

	// Scan presets directory for .ini files and add them to the list
	if ((search = FindFirstFile(_T("config/presets/*.ini"), &findData)) != INVALID_HANDLE_VALUE) {
		do {
			findData.cFileName[_tcslen(findData.cFileName) - 4] = 0;
			SendMessage(hInpdPci, CB_ADDSTRING, 0, (LPARAM)findData.cFileName);
		} while (FindNextFile(search, &findData) != 0);

		FindClose(search);
	}

	RefreshPlayerSelectComboBoxes();
}

static int InpdInit()
{
	// We are just going to hard-code this mapping for now.
	// All of this is for playing 3rd strike in a group setting, so it will be OK for now.
	auto& pi = sfiii3nPlayerInputs;
	pi.p1Index = 0;
	pi.p2Index = 12;
	pi.buttonCount = 12;

	int nMemLen;

	hInpdList = GetDlgItem(hInpdDlg, IDC_INPD_LIST);
	hGamepadList = GetDlgItem(hInpdDlg, IDC_GAMEPAD_LIST);

	// Allocate a last val array for the last input values
	nMemLen = nGameInpCount * sizeof(char);
	LastVal = (unsigned short*)malloc(nMemLen * sizeof(unsigned short));
	if (LastVal == NULL) {
		return 1;
	}
	memset(LastVal, 0, nMemLen * sizeof(unsigned short));

	InpdListBegin();
	InpdListMake(1);

	InputGetGamepads(padInfos, &nPadCount);
	GamepadListBegin();
	GamepadListMake(1);

	// Init the Combo boxes
	hInpdGi = GetDlgItem(hInpdDlg, IDC_INPD_GI);
	hInpdPci = GetDlgItem(hInpdDlg, IDC_INPD_PCI);
	hInpdAnalog = GetDlgItem(hInpdDlg, IDC_INPD_ANALOG);
	hP1Select = GetDlgItem(hInpdDlg, IDC_INDP_P1SELECT);
	hP2Select = GetDlgItem(hInpdDlg, IDC_INDP_P2SELECT);

	InitComboboxes();

	DisablePresets();

	return 0;
}

static int InpdExit()
{
	// Exit the Combo boxes
	hInpdPci = NULL;
	hInpdGi = NULL;
	hInpdAnalog = NULL;

	if (LastVal != NULL) {
		free(LastVal);
		LastVal = NULL;
	}
	hInpdList = NULL;
	hGamepadList = NULL;
	hInpdDlg = NULL;
	if (!bAltPause && bRunPause) {
		bRunPause = 0;
	}
	GameInpCheckMouse();

	return 0;
}

static void GameInpConfigOne(int nPlayer, int nPcDev, int nAnalog, struct GameInp* pgi, char* szi)
{
	switch (nPcDev) {
	case  0:
		GamcPlayer(pgi, szi, nPlayer, -1);						// Keyboard
		GamcAnalogKey(pgi, szi, nPlayer, nAnalog);
		GamcMisc(pgi, szi, nPlayer);
		break;
	case  1:
		GamcPlayer(pgi, szi, nPlayer, 0);						// Joystick 1
		GamcAnalogJoy(pgi, szi, nPlayer, 0, nAnalog);
		GamcMisc(pgi, szi, nPlayer);
		break;
	case  2:
		GamcPlayer(pgi, szi, nPlayer, 1);						// Joystick 2
		GamcAnalogJoy(pgi, szi, nPlayer, 1, nAnalog);
		GamcMisc(pgi, szi, nPlayer);
		break;
	case  3:
		GamcPlayer(pgi, szi, nPlayer, 2);						// Joystick 3
		GamcAnalogJoy(pgi, szi, nPlayer, 2, nAnalog);
		GamcMisc(pgi, szi, nPlayer);
		break;
	case  4:
		GamcPlayerHotRod(pgi, szi, nPlayer, 0x10, nAnalog);		// X-Arcade left side
		GamcMisc(pgi, szi, -1);
		break;
	case  5:
		GamcPlayerHotRod(pgi, szi, nPlayer, 0x11, nAnalog);		// X-Arcade right side
		GamcMisc(pgi, szi, -1);
		break;
	case  6:
		GamcPlayerHotRod(pgi, szi, nPlayer, 0x00, nAnalog);		// HotRod left side
		GamcMisc(pgi, szi, -1);
		break;
	case  7:
		GamcPlayerHotRod(pgi, szi, nPlayer, 0x01, nAnalog);		// HotRod right size
		GamcMisc(pgi, szi, -1);
		break;
	}
}

// Configure some of the game input
static int GameInpConfig(int nPlayer, int nPcDev, int nAnalog)
{
	struct GameInp* pgi = NULL;
	unsigned int i;

	for (i = 0, pgi = GameInp; i < nGameInpCount; i++, pgi++) {
		struct BurnInputInfo bii;

		// Get the extra info about the input
		bii.szInfo = NULL;
		BurnDrvGetInputInfo(&bii, i);
		if (bii.pVal == NULL) {
			continue;
		}
		if (bii.szInfo == NULL) {
			bii.szInfo = "";
		}
		GameInpConfigOne(nPlayer, nPcDev, nAnalog, pgi, bii.szInfo);
	}

	for (i = 0; i < nMacroCount; i++, pgi++) {
		GameInpConfigOne(nPlayer, nPcDev, nAnalog, pgi, pgi->Macro.szName);
	}

	GameInpCheckLeftAlt();

	return 0;
}

// ------------------------------------------------------------------------------------------------------
// List item activated; find out which one
static int ActivateInputListItem()
{
	HWND& list = hInpdList;

	struct BurnInputInfo bii;
	LVITEM LvItem;

	memset(&LvItem, 0, sizeof(LvItem));
	int nSel = SendMessage(list, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
	if (nSel < 0) {
		return 1;
	}

	// Get the corresponding input
	LvItem.mask = LVIF_PARAM;
	LvItem.iItem = nSel;
	LvItem.iSubItem = 0;
	SendMessage(list, LVM_GETITEM, 0, (LPARAM)&LvItem);
	nSel = LvItem.lParam;

	if (nSel >= (int)(nGameInpCount + nMacroCount)) {	// out of range
		return 1;
	}

	memset(&bii, 0, sizeof(bii));
	bii.nType = 0;
	int rc = BurnDrvGetInputInfo(&bii, nSel);
	if (bii.pVal == NULL && rc != 1) {                  // rc == 1 for a macro or system macro.
		return 1;
	}

	DestroyWindow(hInpsDlg);							// Make sure any existing dialogs are gone
	DestroyWindow(hInpcDlg);							//

	if (bii.nType & BIT_GROUP_CONSTANT) {
		// Dip switch is a constant - change it
		nInpcInput = nSel;
		InpcCreate();
	}
	else {
		if (GameInp[nSel].nInput == GIT_MACRO_CUSTOM) {
#if 0
			InpMacroCreate(nSel);
#endif
	}
		else {
			// Assign to a key
			nInpsInput = nSel;
			InpsCreate();
		}
}

	GameInpCheckLeftAlt();

	return 0;
}

#if 0
static int NewMacroButton()
{
	LVITEM LvItem;
	int nSel;

	DestroyWindow(hInpsDlg);							// Make sure any existing dialogs are gone
	DestroyWindow(hInpcDlg);							//

	nSel = SendMessage(hInpdList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
	if (nSel < 0) {
		nSel = -1;
	}

	// Get the corresponding input
	LvItem.mask = LVIF_PARAM;
	LvItem.iItem = nSel;
	LvItem.iSubItem = 0;
	SendMessage(hInpdList, LVM_GETITEM, 0, (LPARAM)&LvItem);
	nSel = LvItem.lParam;

	if (nSel >= (int)nGameInpCount && nSel < (int)(nGameInpCount + nMacroCount)) {
		if (GameInp[nSel].nInput != GIT_MACRO_CUSTOM) {
			nSel = -1;
		}
	}
	else {
		nSel = -1;
	}

	InpMacroCreate(nSel);

	return 0;
		}
#endif

static int DeleteInput(unsigned int i)
{
	struct BurnInputInfo bii;

	if (i >= nGameInpCount) {

		if (i < nGameInpCount + nMacroCount) {	// Macro
			GameInp[i].Macro.nMode = 0;
		}
		else { 								// out of range
			return 1;
		}
	}
	else {									// "True" input
		bii.nType = BIT_DIGITAL;
		BurnDrvGetInputInfo(&bii, i);
		if (bii.pVal == NULL) {
			return 1;
		}
		if (bii.nType & BIT_GROUP_CONSTANT) {	// Don't delete dip switches
			return 1;
		}

		GameInp[i].nInput = 0;
	}

	GameInpCheckLeftAlt();

	return 0;
}

// List item(s) deleted; find out which one(s)
static int ListItemDelete()
{
	int nStart = -1;
	LVITEM LvItem;
	int nRet;

	while ((nRet = SendMessage(hInpdList, LVM_GETNEXTITEM, (WPARAM)nStart, LVNI_SELECTED)) != -1) {
		nStart = nRet;

		// Get the corresponding input
		LvItem.mask = LVIF_PARAM;
		LvItem.iItem = nRet;
		LvItem.iSubItem = 0;
		SendMessage(hInpdList, LVM_GETITEM, 0, (LPARAM)&LvItem);
		nRet = LvItem.lParam;

		DeleteInput(nRet);
	}

	InpdListMake(0);							// refresh view
	return 0;
}

static int InitAnalogOptions(int nGi, int nPci)
{
	// init analog options dialog
	int nAnalog = -1;
	if (nPci == (nPlayerDefaultControls[nGi] & 0x0F)) {
		nAnalog = nPlayerDefaultControls[nGi] >> 4;
	}

	SendMessage(hInpdAnalog, CB_RESETCONTENT, 0, 0);
	if (nPci >= 1 && nPci <= 3) {
		// Absolute mode only for joysticks
		SendMessage(hInpdAnalog, CB_ADDSTRING, 0, (LPARAM)(LPARAM)FBALoadStringEx(hAppInst, IDS_INPUT_ANALOG_ABS, true));
	}
	else {
		if (nAnalog > 0) {
			nAnalog--;
		}
	}
	SendMessage(hInpdAnalog, CB_ADDSTRING, 0, (LPARAM)(LPARAM)FBALoadStringEx(hAppInst, IDS_INPUT_ANALOG_AUTO, true));
	SendMessage(hInpdAnalog, CB_ADDSTRING, 0, (LPARAM)(LPARAM)FBALoadStringEx(hAppInst, IDS_INPUT_ANALOG_NORMAL, true));

	SendMessage(hInpdAnalog, CB_SETCURSEL, (WPARAM)nAnalog, 0);

	return 0;
}

static void SaveHardwarePreset()
{
	TCHAR* szDefaultCpsFile = _T("config\\presets\\cps.ini");
	TCHAR* szDefaultNeogeoFile = _T("config\\presets\\neogeo.ini");
	TCHAR* szDefaultNESFile = _T("config\\presets\\nes.ini");
	TCHAR* szDefaultFDSFile = _T("config\\presets\\fds.ini");
	TCHAR* szDefaultPgmFile = _T("config\\presets\\pgm.ini");
	TCHAR* szFileName = _T("config\\presets\\preset.ini");
	TCHAR* szHardwareString = _T("Generic hardware");

	int nHardwareFlag = (BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK);

	if (nHardwareFlag == HARDWARE_CAPCOM_CPS1 || nHardwareFlag == HARDWARE_CAPCOM_CPS1_QSOUND || nHardwareFlag == HARDWARE_CAPCOM_CPS1_GENERIC || nHardwareFlag == HARDWARE_CAPCOM_CPSCHANGER || nHardwareFlag == HARDWARE_CAPCOM_CPS2 || nHardwareFlag == HARDWARE_CAPCOM_CPS3) {
		szFileName = szDefaultCpsFile;
		szHardwareString = _T("CPS-1/CPS-2/CPS-3 hardware");
	}

	if (nHardwareFlag == HARDWARE_SNK_NEOGEO) {
		szFileName = szDefaultNeogeoFile;
		szHardwareString = _T("Neo-Geo hardware");
	}

	if (nHardwareFlag == HARDWARE_NES) {
		szFileName = szDefaultNESFile;
		szHardwareString = _T("NES hardware");
	}

	if (nHardwareFlag == HARDWARE_FDS) {
		szFileName = szDefaultFDSFile;
		szHardwareString = _T("FDS hardware");
	}

	if (nHardwareFlag == HARDWARE_IGS_PGM) {
		szFileName = szDefaultPgmFile;
		szHardwareString = _T("PGM hardware");
	}

	FILE* fp = _tfopen(szFileName, _T("wt"));
	if (fp) {
		_ftprintf(fp, _T(APP_TITLE) _T(" - Hardware Default Preset\n\n"));
		_ftprintf(fp, _T("%s\n\n"), szHardwareString);
		_ftprintf(fp, _T("version 0x%06X\n\n"), nBurnVer);
		GameInpWrite(fp);
		fclose(fp);
	}

	// add to dropdown (if not already there)
	TCHAR szPresetName[MAX_PATH] = _T("");
	int iCBItem = 0;

	memcpy(szPresetName, szFileName + 15, (_tcslen(szFileName) - 19) * sizeof(TCHAR));
	iCBItem = SendMessage(hInpdPci, CB_FINDSTRING, -1, (LPARAM)szPresetName);
	if (iCBItem == -1) SendMessage(hInpdPci, CB_ADDSTRING, 0, (LPARAM)szPresetName);

	// confirm to user
	FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_PRESET_SAVED), szFileName);
	FBAPopupDisplay(PUF_TYPE_INFO);
}

int UsePreset(bool bMakeDefault)
{
	int nGi, nPci, nAnalog = 0;
	TCHAR szFilename[MAX_PATH] = _T("config\\presets\\");

	nGi = SendMessage(hInpdGi, CB_GETCURSEL, 0, 0);
	if (nGi == CB_ERR) {
		return 1;
	}
	nPci = SendMessage(hInpdPci, CB_GETCURSEL, 0, 0);
	if (nPci == CB_ERR) {
		return 1;
	}
	if (nPci <= 7) {
		// Determine analog option
		nAnalog = SendMessage(hInpdAnalog, CB_GETCURSEL, 0, 0);
		if (nAnalog == CB_ERR) {
			return 1;
		}

		if (nPci == 0 || nPci > 3) {				// No "Absolute" option for keyboard or X-Arcade/HotRod controls
			nAnalog++;
		}

		GameInpConfig(nGi, nPci, nAnalog);			// Re-configure inputs
	}
	else {
		// Find out the filename of the preset ini
		SendMessage(hInpdPci, CB_GETLBTEXT, nPci, (LPARAM)(szFilename + _tcslen(szFilename)));
		_tcscat(szFilename, _T(".ini"));

		GameInputAutoIni(nGi, szFilename, true);	// Read inputs from file

		// Make sure all inputs are defined
		for (unsigned int i = 0, j = 0; i < nGameInpCount; i++) {
			if (GameInp[i].Input.pVal == NULL) {
				continue;
			}

			if (GameInp[i].nInput == 0) {
				DeleteInput(j);
			}

			j++;
		}

		nPci = 0x0F;
	}

	SendMessage(hInpdAnalog, CB_SETCURSEL, (WPARAM)-1, 0);
	SendMessage(hInpdPci, CB_SETCURSEL, (WPARAM)-1, 0);
	SendMessage(hInpdGi, CB_SETCURSEL, (WPARAM)-1, 0);

	DisablePresets();

	if (bMakeDefault) {
		nPlayerDefaultControls[nGi] = nPci | (nAnalog << 4);
		_tcscpy(szPlayerDefaultIni[nGi], szFilename);
	}

	GameInpCheckLeftAlt();

	return 0;
}

static void SliderInit() // Analog sensitivity slider
{
	// Initialise slider
	SendDlgItemMessage(hInpdDlg, IDC_INPD_ANSLIDER, TBM_SETRANGE, (WPARAM)0, (LPARAM)MAKELONG(0x40, 0x0400));
	SendDlgItemMessage(hInpdDlg, IDC_INPD_ANSLIDER, TBM_SETLINESIZE, (WPARAM)0, (LPARAM)0x05);
	SendDlgItemMessage(hInpdDlg, IDC_INPD_ANSLIDER, TBM_SETPAGESIZE, (WPARAM)0, (LPARAM)0x10);
	SendDlgItemMessage(hInpdDlg, IDC_INPD_ANSLIDER, TBM_SETTIC, (WPARAM)0, (LPARAM)0x0100);
	SendDlgItemMessage(hInpdDlg, IDC_INPD_ANSLIDER, TBM_SETTIC, (WPARAM)0, (LPARAM)0x0200);
	SendDlgItemMessage(hInpdDlg, IDC_INPD_ANSLIDER, TBM_SETTIC, (WPARAM)0, (LPARAM)0x0300);
	SendDlgItemMessage(hInpdDlg, IDC_INPD_ANSLIDER, TBM_SETTIC, (WPARAM)0, (LPARAM)0x0400);

	// Set slider to current value
	SendDlgItemMessage(hInpdDlg, IDC_INPD_ANSLIDER, TBM_SETPOS, (WPARAM)true, (LPARAM)nAnalogSpeed);

	// Set the edit control to current value
	TCHAR szText[16];
	_stprintf(szText, _T("%i"), nAnalogSpeed * 100 / 256);
	SendDlgItemMessage(hInpdDlg, IDC_INPD_ANEDIT, WM_SETTEXT, (WPARAM)0, (LPARAM)szText);
}

static void SliderUpdate()
{
	TCHAR szText[16] = _T("");
	bool bValid = 1;
	int nValue;

	if (SendDlgItemMessage(hInpdDlg, IDC_INPD_ANEDIT, WM_GETTEXTLENGTH, (WPARAM)0, (LPARAM)0) < 16) {
		SendDlgItemMessage(hInpdDlg, IDC_INPD_ANEDIT, WM_GETTEXT, (WPARAM)16, (LPARAM)szText);
	}

	// Scan string in the edit control for illegal characters
	for (int i = 0; szText[i]; i++) {
		if (!_istdigit(szText[i])) {
			bValid = 0;
			break;
		}
	}

	if (bValid) {
		nValue = _tcstol(szText, NULL, 0);
		if (nValue < 25) {
			nValue = 25;
		}
		else {
			if (nValue > 400) {
				nValue = 400;
			}
		}

		nValue = (int)((double)nValue * 256.0 / 100.0 + 0.5);

		// Set slider to current value
		SendDlgItemMessage(hInpdDlg, IDC_INPD_ANSLIDER, TBM_SETPOS, (WPARAM)true, (LPARAM)nValue);
	}
}

static void SliderExit()
{
	TCHAR szText[16] = _T("");
	INT32 nVal = 0;

	SendDlgItemMessage(hInpdDlg, IDC_INPD_ANEDIT, WM_GETTEXT, (WPARAM)16, (LPARAM)szText);
	nVal = _tcstol(szText, NULL, 0);
	if (nVal < 25) {
		nVal = 25;
	}
	else {
		if (nVal > 400) {
			nVal = 400;
		}
	}

	nAnalogSpeed = (int)((double)nVal * 256.0 / 100.0 + 0.5);
	bprintf(0, _T("  * Analog Speed: %X\n"), nAnalogSpeed);
}

// ---------------------------------------------------------------------------------------------------------
// Given the current set of input mappings for p1, we will save them to the selected gamepad.
// This is a bit weird in that if you don't currently have the buttons correctly mapped on the p1
// inputs, you may not get what you expect.
// Long term we will come up with a way for the user to be able to set all of the buttons at one time,
// probably by borrowing functionality from the inps.cpp dialog....
static void saveMappingsInfo() {
	if (nSelectedPadIndex == -1) { return; }

	GamepadFileEntry* pad = padInfos[nSelectedPadIndex];

	// We will copy + translate the values from the p1 inputs.....
	unsigned int i, j = 0;
	struct GameInp* pgi = NULL;
	for (i = 0, pgi = GameInp; i < sfiii3nPlayerInputs.buttonCount; i++, pgi++) {
		if (pgi->Input.pVal == NULL) {
			continue;
		}

		GamepadInput& pi = pad->profile.inputs[i];
		pi.nInput = pgi->nInput;

		UINT16 code = 0; //pgi->Input.nVal;


		if (pi.nInput == GIT_SWITCH) {
			code = pgi->Input.Switch.nCode;
		}
		else if (pi.nInput & GIT_GROUP_JOYSTICK) {
			code = 0;		// Joysticks don't have a code, it is all encoded in the nType data....
		}
		else {
			// Not supported?
			continue;
		}

		// NOTE: This is where we need to translated gamepad codes....
		if (code >= 0x4000 && code < 0x8000) {
			INT32 index = (code >> 8) & 0x3F;
			code = code & 0xFF;

			// We are going to put in a gamepad bit as this is how the input coding system works...
			code |= 0x4000;
		}
		pi.nCode = code;


	}


	InputSaveGamepadMappings();
}

// ---------------------------------------------------------------------------------------------------------
static void saveAliasInfo() {
	if (nSelectedPadIndex == -1) { return; }

	selectedPadEntry = padInfos[nSelectedPadIndex];

	memset(aliasBuffer, 0, MAX_ALIAS_CHARS);
	SendDlgItemMessage(hInpdDlg, IDC_ALIAS_EDIT, WM_GETTEXT, (WPARAM)MAX_ALIAS_CHARS, (LPARAM)aliasBuffer);

	// Update the list box item....
	updateListboxAlias(hGamepadList, nSelectedPadIndex, aliasBuffer, MAX_ALIAS_CHARS, false);

	SetStrBuffer(selectedPadEntry->info.Alias, MAX_ALIAS_CHARS, aliasBuffer);

	// Write out the data to disk....
	InputSaveGamepadMappings();
}

static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_INITDIALOG) {
		hInpdDlg = hDlg;
		InpdInit();
		SliderInit();
		if (!kNetGame && bAutoPause) {
			bRunPause = 1;
		}

		return TRUE;
	}

	if (Msg == WM_CLOSE) {
		SliderExit();
		EnableWindow(hScrnWnd, TRUE);
		DestroyWindow(hInpdDlg);
		return 0;
	}

	if (Msg == WM_DESTROY) {
		InpdExit();
		return 0;
	}

	if (Msg == WM_COMMAND) {
		int Id = LOWORD(wParam);
		int Notify = HIWORD(wParam);

		if (Id == IDOK && Notify == BN_CLICKED) {
			ActivateInputListItem();
			SelectGamepadListItem();
			return 0;
		}
		if (Id == IDCANCEL && Notify == BN_CLICKED) {
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			return 0;
		}

		if (Id == IDSAVEALIAS && Notify == BN_CLICKED) {
			saveAliasInfo();
			return 0;
		}

		if (Id == ID_SAVE_MAPPINGS && Notify == BN_CLICKED) {
			saveMappingsInfo();
			return 0;
		}

		if (Id == ID_REFRESH_PADS && Notify == BN_CLICKED) {
			SetEnabled(ID_REFRESH_PADS, FALSE);
			OnGamepadListDeselect();
			InputInit();

			// Repopulate the gamepad list....
			InputGetGamepads(padInfos, &nPadCount);
			GamepadListMake(1);

			RefreshPlayerSelectComboBoxes();
			SetEnabled(ID_REFRESH_PADS, TRUE);
			return 0;
		}

		if (Id == ID_SET_PLAYER_MAPPINGS && Notify == BN_CLICKED) {
			SetPlayerMappings();
			return 0;
		}



		if (Id == IDC_INPD_NEWMACRO && Notify == BN_CLICKED) {

			//			NewMacroButton();

			return 0;
		}

		if (Id == IDC_INPD_SAVE_AS_PRESET && Notify == BN_CLICKED) {
			SaveHardwarePreset();
			return 0;
		}

		if (Id == IDC_INPD_USE && Notify == BN_CLICKED) {

			UsePreset(false);

			InpdListMake(0);								// refresh view

			return 0;
		}

		if (Id == IDC_INPD_DEFAULT && Notify == BN_CLICKED) {

			UsePreset(true);

			InpdListMake(0);								// refresh view

			return 0;
		}

		if (Notify == EN_UPDATE) {                          // analog slider update
			SliderUpdate();

			return 0;
		}

		if ((Id == IDC_INDP_P1SELECT || Id == IDC_INDP_P2SELECT) && Notify == CBN_SELCHANGE) {
			OnPlayerSelectionChanged();
			return 0;
		}

		if (Id == IDC_INPD_GI && Notify == CBN_SELCHANGE) {
			int nGi;
			nGi = SendMessage(hInpdGi, CB_GETCURSEL, 0, 0);
			if (nGi == CB_ERR) {
				SendMessage(hInpdPci, CB_SETCURSEL, (WPARAM)-1, 0);
				SendMessage(hInpdAnalog, CB_SETCURSEL, (WPARAM)-1, 0);

				DisablePresets();

				return 0;
			}
			int nPci = nPlayerDefaultControls[nGi] & 0x0F;
			SendMessage(hInpdPci, CB_SETCURSEL, nPci, 0);
			EnableWindow(hInpdPci, TRUE);

			if (nPci > 5) {
				SendMessage(hInpdAnalog, CB_SETCURSEL, (WPARAM)-1, 0);
				EnableWindow(hInpdAnalog, FALSE);
			}
			else {
				InitAnalogOptions(nGi, nPci);
				EnableWindow(hInpdAnalog, TRUE);
			}

			EnableWindow(GetDlgItem(hInpdDlg, IDC_INPD_DEFAULT), TRUE);
			EnableWindow(GetDlgItem(hInpdDlg, IDC_INPD_USE), TRUE);

			return 0;
		}

		if (Id == IDC_INPD_PCI && Notify == CBN_SELCHANGE) {
			int nGi, nPci;
			nGi = SendMessage(hInpdGi, CB_GETCURSEL, 0, 0);
			if (nGi == CB_ERR) {
				return 0;
			}
			nPci = SendMessage(hInpdPci, CB_GETCURSEL, 0, 0);
			if (nPci == CB_ERR) {
				return 0;
			}

			if (nPci > 7) {
				EnableWindow(GetDlgItem(hInpdDlg, IDC_INPD_DEFAULT), TRUE);
				EnableWindow(GetDlgItem(hInpdDlg, IDC_INPD_USE), TRUE);

				SendMessage(hInpdAnalog, CB_SETCURSEL, (WPARAM)-1, 0);
				EnableWindow(hInpdAnalog, FALSE);
			}
			else {
				EnableWindow(hInpdAnalog, TRUE);
				InitAnalogOptions(nGi, nPci);

				if (SendMessage(hInpdAnalog, CB_GETCURSEL, 0, 0) != CB_ERR) {
					EnableWindow(GetDlgItem(hInpdDlg, IDC_INPD_DEFAULT), TRUE);
					EnableWindow(GetDlgItem(hInpdDlg, IDC_INPD_USE), TRUE);
				}
				else {
					EnableWindow(GetDlgItem(hInpdDlg, IDC_INPD_DEFAULT), FALSE);
					EnableWindow(GetDlgItem(hInpdDlg, IDC_INPD_USE), FALSE);
				}
			}

			return 0;
		}

		if (Id == IDC_INPD_ANALOG && Notify == CBN_SELCHANGE) {
			if (SendMessage(hInpdAnalog, CB_GETCURSEL, 0, 0) != CB_ERR) {
				EnableWindow(GetDlgItem(hInpdDlg, IDC_INPD_DEFAULT), TRUE);
				EnableWindow(GetDlgItem(hInpdDlg, IDC_INPD_USE), TRUE);
			}

			return 0;
		}

	}

	if (Msg == WM_HSCROLL) { // Analog Slider updates
		switch (LOWORD(wParam)) {
		case TB_BOTTOM:
		case TB_ENDTRACK:
		case TB_LINEDOWN:
		case TB_LINEUP:
		case TB_PAGEDOWN:
		case TB_PAGEUP:
		case TB_THUMBPOSITION:
		case TB_THUMBTRACK:
		case TB_TOP: {
			TCHAR szText[16] = _T("");
			int nValue;

			// Update the contents of the edit control
			nValue = SendDlgItemMessage(hDlg, IDC_INPD_ANSLIDER, TBM_GETPOS, (WPARAM)0, (LPARAM)0);
			nValue = (int)((double)nValue * 100.0 / 256.0 + 0.5);
			_stprintf(szText, _T("%i"), nValue);
			SendDlgItemMessage(hDlg, IDC_INPD_ANEDIT, WM_SETTEXT, (WPARAM)0, (LPARAM)szText);
			break;
		}
		}

		return 0;
	}

	if (Msg == WM_NOTIFY && lParam != 0) {
		int Id = LOWORD(wParam);
		NMHDR* pnm = (NMHDR*)lParam;

		if (Id == IDC_INPD_LIST && pnm->code == LVN_ITEMACTIVATE) {
			ActivateInputListItem();
		}
		if (Id == IDC_GAMEPAD_LIST && pnm->code == LVN_ITEMCHANGED) {
			SelectGamepadListItem();
		}

		if (Id == IDC_INPD_LIST && pnm->code == LVN_KEYDOWN) {
			NMLVKEYDOWN* pnmkd = (NMLVKEYDOWN*)lParam;
			if (pnmkd->wVKey == VK_DELETE) {
				ListItemDelete();
			}
		}

		if (Id == IDC_INPD_LIST && pnm->code == NM_CUSTOMDRAW) {
			NMLVCUSTOMDRAW* plvcd = (NMLVCUSTOMDRAW*)lParam;

			switch (plvcd->nmcd.dwDrawStage) {
			case CDDS_PREPAINT:
				SetWindowLongPtr(hInpdDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
				return 1;
			case CDDS_ITEMPREPAINT:
				if (plvcd->nmcd.dwItemSpec < nGameInpCount) {
					if (GameInp[plvcd->nmcd.dwItemSpec].nType & BIT_GROUP_CONSTANT) {

						if (GameInp[plvcd->nmcd.dwItemSpec].nInput == 0) {
							plvcd->clrTextBk = RGB(0xDF, 0xDF, 0xDF);

							SetWindowLongPtr(hInpdDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
							return 1;
						}

						if (GameInp[plvcd->nmcd.dwItemSpec].nType == BIT_DIPSWITCH) {
							plvcd->clrTextBk = RGB(0xFF, 0xEF, 0xD7);

							SetWindowLongPtr(hInpdDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
							return 1;
						}
					}
				}

				if (plvcd->nmcd.dwItemSpec >= nGameInpCount) {
					if (GameInp[plvcd->nmcd.dwItemSpec].Macro.nMode) {
						plvcd->clrTextBk = RGB(0xFF, 0xCF, 0xCF);
					}
					else {
						plvcd->clrTextBk = RGB(0xFF, 0xEF, 0xEF);
					}

					SetWindowLongPtr(hInpdDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
					return 1;
				}
				return 1;
			}
		}
		return 0;
	}

	return 0;
}

int InpdCreate()
{
	if (bDrvOkay == 0) {
		return 1;
	}

	DestroyWindow(hInpdDlg);										// Make sure exitted

	hInpdDlg = FBACreateDialog(hAppInst, MAKEINTRESOURCE(IDD_INPD), hScrnWnd, (DLGPROC)DialogProc);
	if (hInpdDlg == NULL) {
		return 1;
	}

	WndInMid(hInpdDlg, hScrnWnd);
	ShowWindow(hInpdDlg, SW_NORMAL);

	return 0;
}


