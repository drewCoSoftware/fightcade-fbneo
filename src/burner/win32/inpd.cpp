// Burner Input Editor Dialog module
// NOTE: This is the <strike>really lousy</strike> war crime of a mapping + preset dialog that comes with the emulator.
// This is where we will make the UI less stupid.
#include "burner.h"

HWND hInpdDlg = NULL;							// Handle to the Input Dialog
static HWND hInpdList = NULL;
static HWND hGamepadList = NULL;
static HWND hProfileList = NULL;

static unsigned short* LastVal = NULL;			// Last input values/defined
static int bLastValDefined = 0;					//

static HWND hInpdGi = NULL, hInpdPci = NULL, hInpdAnalog = NULL;	// Combo boxes
static HWND hP1Select;
static HWND hP2Select;

// Get the currently plugged gamepad data:
// NOTE:  We should just get a pointer to this so that we can set the alias directly.
#define MAX_GAMEPAD 8			// Should match the version in inp_dinput.cpp
static GamepadFileEntry* padInfos[MAX_GAMEPAD];
static INT32 nPadCount;


static InputProfileEntry* inputProfiles[MAX_PROFILE_LEN];
static INT32 nProfileCount;
static int nSelectedProfileIndex = -1;
static HWND hActivePlayerCombo;

// Text buffer for the gamepad alias + other text fields.
// We don't really need this here, it's just where it landed and there hasn't been a great need
// to use in-function buffers yet.
static TCHAR aliasBuffer[MAX_ALIAS_CHARS];

static playerInputs sfiii3nPlayerInputs;

enum EDialogState {
	DIALOGSTATE_NORMAL = 0,
	DIALOGSTATE_SET_PLAYER,			// Set pad index for player.
	DIALOGSTATE_CHOOSE_PROFILE
};
int _DetectPlayerIndex = -1;
int _SelectedPadIndex = -1;

// Choose profile stuff (placeholder)
int _ProfileIndex = -1;
int _LastUpDown = 0;

// The last button code that we detected as pressed.
// The input system has a makework system of checking for button presses,
// so we just keep track of this to help us determine when a button is released....
//bool AllReleased = true;		
//UINT16 CurPressed = 0;		// Currently pressed button (for update call)
INT32 LastPressed = -1;


EDialogState _CurState = DIALOGSTATE_NORMAL;

#define ALIAS_INDEX 0
#define STATE_INDEX 1
#define GUID_INDEX 2

#define DIR_UP -1
#define DIR_DOWN 1
#define DIR_NONE 0
#define Y_AXIS_CODE 1


// ---------------------------------------------------------------------------------------------------------
static INT32 SetComboIndex(HWND handle, int index) {

	SendMessage(handle, CB_SETCURSEL, index, 0);
	//if (_SelectedPadIndex == 0) {

	//}
	//else if (_SelectedPadIndex == 1) {

	//}
	//else {
	//	// Invalid Id...
	//	return -1;
	//}


	// NOTE: We don't really need combo boxes.  We could just update text in a textbox....
	// I want to keep the combos around tho in case we need them later......
	// In theory one could still use them to make a selection, and then hit a pad button.....


	// FROM CHATGPT:
	// With a bit of poking, we can certainly figure out how to get the selected index of
	// the combo box to use when setting the profile..........
	//void ChangeComboBoxSelection(HWND hWndComboBox, int selectedIndex)
	//{
	//	if (hWndComboBox && ::IsWindow(hWndComboBox) && selectedIndex >= 0)
	//	{
	//		::SendMessage(hWndComboBox, CB_SETCURSEL, static_cast<WPARAM>(selectedIndex), 0);
	//	}
	//}

	return 0;
}

// ---------------------------------------------------------------------------------------------------------
// Enable / disable a control based on its ID.
static void SetEnabled(int id, BOOL bEnable) {
	EnableWindow(GetDlgItem(hInpdDlg, id), bEnable);
}


// -------------------------------------------------------------------------------------
static int GetIndexFromButton(UINT16 button) {
	if (button >= 0x4000 && button < 0x8000)
	{
		int index = (button >> 8) & 0x3F;
		return index;
	}
	return -1;
}

// -------------------------------------------------------------------------------------
// From the given button, set the pad index and button code.
// Returns 0 when the values are correctly set.
static int GetIndexAndCodeFromButton(UINT16 button, int& index, UINT16& code) {

	if (button >= 0x4000 && button < 0x8000)
	{
		UINT16 codePart = button & 0xFF;
		int indexPart = (button >> 8) & 0x3F;

		code = codePart;
		index = indexPart;

		return 0;
	}
	else
	{
		// Not a gamepad input.
		return -1;
	}
}

// --------------------------------------------------------------------------------------------------
// For the given pad index, returns an int that is -1 for up, 1 for down, and zero
// if an up or down button is not pressed.
// TODO: These need to be able to detect up/down keys from the keyboard too!
static int GetUpOrDownButton(int padIndex, UINT16 buttons) {


	int index = 0;
	UINT16 nCode = 0;


	if (GetIndexAndCodeFromButton(buttons, index, nCode) == 0)
	{
		if (index != padIndex) { return DIR_NONE; }

		// NOTE: The commented blocks come from the code that formats the labels
		// when printing out hte button lists:
		// This is how I determined what is up and what is down.
		if (nCode < 0x10) {
			//TCHAR szAxis[8][3] = { _T("X"), _T("Y"), _T("Z"), _T("rX"), _T("rY"), _T("rZ"), _T("s0"), _T("s1") };
			//TCHAR szDir[6][16] = { _T("negative"), _T("positive"), _T("Left"), _T("Right"), _T("Up"), _T("Down") };
			//if (nCode < 4) {
			//	_stprintf(szString, _T("%s %s (%s %s)"), gpName, szDir[nCode + 2], szAxis[nCode >> 1], szDir[nCode & 1]);
			//}
			//else {
			//	_stprintf(szString, _T("%s %s %s"), gpName, szAxis[nCode >> 1], szDir[nCode & 1]);
			//}
			//return szString;
			if (nCode < 4) {
				UINT16 axisCode = nCode >> 1;
				UINT16 dirCode = nCode + 2;
				if (axisCode != Y_AXIS_CODE) { return DIR_NONE; }
				if (dirCode == 4) { return DIR_UP; }
				if (dirCode == 5) { return DIR_DOWN; }
			}
			else {
			}
		}
		if (nCode < 0x20) {
			UINT16 dirCode = nCode & 3;
			if (dirCode == 2) { return DIR_UP; }
			if (dirCode == 3) { return DIR_DOWN; }
			//TCHAR szDir[4][16] = { _T("Left"), _T("Right"), _T("Up"), _T("Down") };
			//_stprintf(szString, _T("%s POV-hat %d %s"), gpName, (nCode & 0x0F) >> 2, szDir[nCode & 3]);
			//return szString;
		}

		int xxxxxxx = 10;
	}


	return DIR_NONE;
}

// ------------------------------------------------------------------------------------------
static void SetState(EDialogState newState) {
	TCHAR buffer[256];

	switch (newState) {
	case DIALOGSTATE_NORMAL:
	{
		// Clear the text.
		SetDlgItemText(hInpdDlg, IDC_SET_PLAYER_MESSAGE, 0);
		SetEnabled(ID_SET_PLAYER1, true);
		SetEnabled(ID_SET_PLAYER2, true);

		_SelectedPadIndex = -1;
		_DetectPlayerIndex = -1;
	}
	break;

	case DIALOGSTATE_SET_PLAYER:
	{
		int label = _DetectPlayerIndex + 1;

		swprintf(buffer, _T("Press button for player %d"), label);
		SetDlgItemText(hInpdDlg, IDC_SET_PLAYER_MESSAGE, buffer);

		// Disable the correct button....
		if (_DetectPlayerIndex == 0) {
			SetEnabled(ID_SET_PLAYER1, true);
			SetEnabled(ID_SET_PLAYER2, false);
		}
		else {
			SetEnabled(ID_SET_PLAYER1, false);
			SetEnabled(ID_SET_PLAYER2, true);
		}
	}
	break;


	case DIALOGSTATE_CHOOSE_PROFILE:
	{

		if (_SelectedPadIndex < 0 || _SelectedPadIndex > 1 || nProfileCount < 1)
		{
			// Invalid ID or no profiles to choose from.
			SetState(DIALOGSTATE_NORMAL);
			return;
		}

		// We will set a selection in the combo box.
		hActivePlayerCombo = 0;
		if (_SelectedPadIndex == 0) {
			hActivePlayerCombo = hP1Select;
		}
		else if (_SelectedPadIndex == 1) {
			hActivePlayerCombo = hP2Select;
		}

		SetComboIndex(hActivePlayerCombo, 0);


		SetDlgItemText(hInpdDlg, IDC_SET_PLAYER_MESSAGE, _T("Choose profile and press a button"));
		_ProfileIndex = -1;
		_LastUpDown = -2;
	}
	break;


	default:
		break;
	}

	_CurState = newState;
}

// --------------------------------------------------------------------------------------------------
static void ProcessState() {

	// We only want to update the pressed button code the first time it is pushed.
	// If we still have a pressed button on the next cycle we will ignore it.
	INT32 pressed = InputFind(8);
	int padIndex = GetIndexFromButton(pressed); //   GetGamepadIndex(pressedPadButtons);

	// We only care about pad buttons for this stuff.....
	INT32 usePressed = -1;
	if (padIndex != -1)
	{
		usePressed = LastPressed == -1 ? pressed : -1;
		LastPressed = pressed;
	}

	if (pressed == -1)
	{
		// Clear the pressed button....
		LastPressed = -1;
	}



	// Only use 
	//UINT16 usePressed = AllReleased ? 0 : CurPressed;

	if (_CurState == DIALOGSTATE_SET_PLAYER) {

		// We have to iterate through inputs and select those that are from gamepads.
		// From there we can determine the index of that gamepad....
	//	UINT16 pressed = InputFind(8);
		if (padIndex != -1)
		{
			// Now that we have a pad index, we can associate it with the currently selected
			// input profile!
			_SelectedPadIndex = padIndex;
			SetState(DIALOGSTATE_CHOOSE_PROFILE);
		}

	}
	else if (_CurState == DIALOGSTATE_CHOOSE_PROFILE) {

		//	UINT16 pressed = InputFind(8);
	//		int padIndex = GetIndexFromButton(CurP);

			// Here we want to detect the up/down buttons so that
			// we can cycle through the available profiles.
			// TODO: Make sure that this works with keyboard buttons too!
		int dirDelta = GetUpOrDownButton(_SelectedPadIndex, usePressed);

		bool dirChanged = dirDelta != _LastUpDown;
		if (dirChanged) {
			_LastUpDown = dirDelta;

			int curIndex = SendMessage(hActivePlayerCombo, CB_GETCURSEL, 0, 0);
			int newIndex = curIndex + dirDelta;

			int maxIndex = nProfileCount - 1;

			if (newIndex < 0) { newIndex = maxIndex; }
			if (newIndex > maxIndex) { newIndex = 0; }

			SetComboIndex(hActivePlayerCombo, newIndex);

			// NOTE: This is where we will change the selected profile....
			//TCHAR buffer[256];
			//swprintf(buffer, _T("Up/Down?: %i"), upDownDir);
			//SetDlgItemText(hInpdDlg, IDC_SET_PLAYER_MESSAGE, buffer);

		}
		else if (usePressed != -1 && dirDelta == 0 && padIndex == _SelectedPadIndex) {

			// User didn't press a direction button, but did press something
			// else.  They must be happy with their profile selection....
			throw std::exception("NOT IMPLEMENTED!");
		}
	}

	//// Only update last pressed if it is zero.
	//// We can only really track that NOTHING is pressed, not that a certain button has been released.
	//if (LastPressed == 0)
	//{
	//}

}


// -------------------------------------------------------------------------------------
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

		pszVal = InpToDesc(pgi, padInfos);

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

// ---------------------------------------------------------------------------------------------
// This is where we can wait for p1/p2 to check in and what-not....
int InpdUpdate()
{
	//std::vector<UINT16> pressedPadButtons;
	//GetPressedGamepadButtons(pressedPadButtons);

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



	ProcessState();


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

	return 0;
}



// ---------------------------------------------------------------------------------------------------------
static int ProfileListBegin()
{
	auto hList = hProfileList;
	if (hList == NULL) {
		return 1;
	}

	// Full row select style:
	SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);


	// Make column headers
	LVCOLUMN LvCol;
	memset(&LvCol, 0, sizeof(LvCol));
	LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;


	LvCol.cx = 0x95;		// Column Width.
	LvCol.pszText = _T("Name"); //TODO: Localize  // FBALoadStringEx(hAppInst, IDS_GAMEPAD_ALIAS, true);
	SendMessage(hList, LVM_INSERTCOLUMN, ALIAS_INDEX, (LPARAM)&LvCol);

	return 0;
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


// ---------------------------------------------------------------------------------------------------------
static int ProfileListMake(int bBuild) {

	HWND& hList = hProfileList;
	if (hList == NULL) {
		return 1;
	}

	if (bBuild) {
		SendMessage(hList, LVM_DELETEALLITEMS, 0, 0);
	}


	// Populate the list:
	for (unsigned int i = 0; i < nProfileCount; i++) {
		InputProfileEntry* profile = inputProfiles[i];

		SetStrBuffer(aliasBuffer, MAX_ALIAS_CHARS, profile->Name);

		LVITEM LvItem;

		// Populate the name column.
		memset(&LvItem, 0, sizeof(LvItem));
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = i;
		LvItem.iSubItem = 0;
		LvItem.pszText = profile->Name; // aliasBuffer; // _T(aliasBuffer);
		SendMessage(hList, LVM_INSERTITEM, 0, (LPARAM)&LvItem);
	}


	return 0;
}

// ------------------------------------------------------------------------------------------------------
static int OnProfileListDeselect() {
	SendDlgItemMessage(hInpdDlg, IDC_ALIAS_EDIT, WM_SETTEXT, (WPARAM)0, (LPARAM)NULL);
	nSelectedProfileIndex = -1;
	SetEnabled(ID_REMOVE_PROFILE, FALSE);
	SetEnabled(ID_SAVE_MAPPINGS, FALSE);
	return 0;
}

//
//// ------------------------------------------------------------------------------------------------------
//static int OnGamepadListDeselect() {
//	// Clear the alias input box.
//	SendDlgItemMessage(hInpdDlg, IDC_ALIAS_EDIT, WM_SETTEXT, (WPARAM)0, (LPARAM)NULL);
//	nSelectedPadIndex = -1;
//	selectedPadEntry = NULL;
//	SetEnabled(IDSAVEALIAS, FALSE);
//	SetEnabled(ID_SAVE_MAPPINGS, FALSE);
//	return 0;
//}

//// ------------------------------------------------------------------------------------------------------
//static int SelectGamepadListItem()
//{
//	HWND& list = hGamepadList;
//	LVITEM LvItem;
//	memset(&LvItem, 0, sizeof(LvItem));
//	int nSel = SendMessage(list, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
//	if (nSel < 0) {
//		OnGamepadListDeselect();
//		return 1;
//	}
//
//	// Get the corresponding input
//	nSelectedPadIndex = nSel;
//	getListItemData(list, nSel, LvItem);
//
//	if (nSel >= nPadCount) {
//		OnGamepadListDeselect();
//		return 1;
//	}
//
//	// Now we can set the text in the alias text box!
//	// Set the edit control to current value
//
//	// Enable the save alias button....
//	HWND hBtn = GetDlgItem(hInpdDlg, IDSAVEALIAS);
//	SetEnabled(IDSAVEALIAS, TRUE);
//	SetEnabled(ID_SAVE_MAPPINGS, TRUE);
//
//	TCHAR* useBuffer = wcscmp(aliasBuffer, _T("<not set>")) == 0 ? _T("") : aliasBuffer;
//	SendDlgItemMessage(hInpdDlg, IDC_ALIAS_EDIT, WM_SETTEXT, (WPARAM)0, (LPARAM)useBuffer);
//
//
//}


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
		if (code >= 0x4000 && code < 0x8000)
		{
			// This is a gamepad input.  We need to translate its index in order to set the code correctly.
			code = code | (padIndex << 8);

			int checkIndex = (code >> 8) & 0x3F;
			int x = 10;
		}

		// Now we can set this on the pgi input....
		pgi->nInput = pi.nInput;
		if (pi.nInput == GIT_SWITCH) {
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


// Removed, we don't really need this information.....
// We don't even really need combos for that matter.....
//// ------------------------------------------------------------------------------------------------------
//static int OnPlayerSelectionChanged() {
//
//	// TODO: Enable/disable the 'set' button that is under the combos....
//	int p1Index = SendMessage(hP1Select, CB_GETCURSEL, 0, 0);
//	int p2Index = SendMessage(hP2Select, CB_GETCURSEL, 0, 0);
//
//	bool p1Enabled = p1Index != CB_ERR && p1Index > -1;
//	bool p2Enabled = p2Index != CB_ERR && p2Index > -1;
//	SetEnabled(ID_SET_PLAYER1, p1Enabled);
//	SetEnabled(ID_SET_PLAYER2, p2Enabled);
//
//	return 0;
//}

// ------------------------------------------------------------------------------------------------------
static int SelectProfileListItem()
{
	HWND& list = hProfileList;

	LVITEM LvItem;
	memset(&LvItem, 0, sizeof(LvItem));
	int nSel = SendMessage(list, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
	if (nSel < 0) {
		OnProfileListDeselect();
		return 1;
	}

	// Get the corresponding input
	nSelectedProfileIndex = nSel;
	getListItemData(list, nSel, LvItem);

	if (nSel >= nProfileCount) {
		OnProfileListDeselect();
		return 1;
	}

	// Activate certain buttons.
	SetEnabled(ID_REMOVE_PROFILE, true);
	SetEnabled(ID_SAVE_MAPPINGS, true);

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

// ------------------------------------------------------------------------------------------
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

	for (size_t i = 0; i < nProfileCount; i++)
	{
		TCHAR* padAlias = inputProfiles[i]->Name;
		SendMessage(hP1Select, CB_ADDSTRING, 0, (LPARAM)padAlias);
		SendMessage(hP2Select, CB_ADDSTRING, 0, (LPARAM)padAlias);
	}

	//// We should only be enabled if one or more selections are currently made.....
	//SetEnabled(ID_SET_PLAYER_MAPPINGS, false);
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

// ------------------------------------------------------------------------------------------------------------
static void RefreshProfileData() {
	InputGetProfiles(inputProfiles, &nProfileCount);
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
	hProfileList = GetDlgItem(hInpdDlg, IDC_PROFILE_LIST);

	// Allocate a last val array for the last input values
	nMemLen = nGameInpCount * sizeof(char);
	LastVal = (unsigned short*)malloc(nMemLen * sizeof(unsigned short));
	if (LastVal == NULL) {
		return 1;
	}
	memset(LastVal, 0, nMemLen * sizeof(unsigned short));


	InputGetGamepads(padInfos, &nPadCount);
	RefreshProfileData();

	InpdListBegin();
	InpdListMake(1);
	GamepadListBegin();
	GamepadListMake(1);

	ProfileListBegin();
	ProfileListMake(1);

	// Init the Combo boxes
	hInpdGi = GetDlgItem(hInpdDlg, IDC_INPD_GI);
	hInpdPci = GetDlgItem(hInpdDlg, IDC_INPD_PCI);
	hInpdAnalog = GetDlgItem(hInpdDlg, IDC_INPD_ANALOG);
	hP1Select = GetDlgItem(hInpdDlg, IDC_INDP_P1SELECT);
	hP2Select = GetDlgItem(hInpdDlg, IDC_INDP_P2SELECT);

	InitComboboxes();

	DisablePresets();


	SetState(DIALOGSTATE_NORMAL);
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
	// NOTE: It makes more sense to just keep a mapping profile laying around.....
	if (nSelectedProfileIndex == -1) { return; }
	InputProfileEntry* profile = inputProfiles[nSelectedProfileIndex];

	// We will copy + translate the values from the p1 inputs.....
	unsigned int i, j = 0;
	struct GameInp* pgi = NULL;
	for (i = 0, pgi = GameInp; i < sfiii3nPlayerInputs.buttonCount; i++, pgi++) {
		if (pgi->Input.pVal == NULL) {
			continue;
		}

		GamepadInput& pi = profile->Inputs[i];
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

	InputSaveProfiles();
	// InputSaveGamepadMappings();
}

// ---------------------------------------------------------------------------------------------------------
// Add a new input mapping profile into the system.
static void addNewProfile() {

	memset(aliasBuffer, 0, MAX_ALIAS_CHARS);
	SendDlgItemMessage(hInpdDlg, IDC_PROFILE_NAME, WM_GETTEXT, (WPARAM)MAX_ALIAS_CHARS, (LPARAM)aliasBuffer);

	// No text, bail.
	// TODO: We can track text events to disable the button, check for dupes, etc.
	if (aliasBuffer == 0 || wcslen(aliasBuffer) == 0)
	{
		return;
	}

	INT32 added = InputAddInputProfile(aliasBuffer);
	if (added == 0)
	{
		// Repopulate the alias list.
		RefreshProfileData();
		ProfileListMake(1);

		RefreshPlayerSelectComboBoxes();
	}

}

// ---------------------------------------------------------------------------------------------------------
static void removeProfile() {

	if (nSelectedProfileIndex == -1) { return; }
	InputRemoveProfile(nSelectedProfileIndex);

	OnProfileListDeselect();

	RefreshPlayerSelectComboBoxes();
	RefreshProfileData();
	ProfileListMake(1);
}

//
//// ---------------------------------------------------------------------------------------------------------
//static void saveAliasInfo() {
//	if (nSelectedPadIndex == -1) { return; }
//
//	selectedPadEntry = padInfos[nSelectedPadIndex];
//
//	memset(aliasBuffer, 0, MAX_ALIAS_CHARS);
//	SendDlgItemMessage(hInpdDlg, IDC_ALIAS_EDIT, WM_GETTEXT, (WPARAM)MAX_ALIAS_CHARS, (LPARAM)aliasBuffer);
//
//	// Update the list box item....
//	updateListboxAlias(hGamepadList, nSelectedPadIndex, aliasBuffer, MAX_ALIAS_CHARS, false);
//
//	SetStrBuffer(selectedPadEntry->info.Alias, MAX_ALIAS_CHARS, aliasBuffer);
//
//	// Write out the data to disk....
//	// InputSaveGamepadMappings();
//
//	// Refresh the gamepad names in the mapper....
//	InpdUseUpdate();
//}

// ---------------------------------------------------------------------------------------------------------
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
			//			SelectGamepadListItem();
			return 0;
		}
		if (Id == IDCANCEL && Notify == BN_CLICKED) {
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			return 0;
		}

		//if (Id == IDSAVEALIAS && Notify == BN_CLICKED) {
		//	saveAliasInfo();
		//	return 0;
		//}

		if (Id == ID_ADDPROFILE && Notify == BN_CLICKED) {
			addNewProfile();
			return 0;
		}

		if (Id == ID_REMOVE_PROFILE && Notify == BN_CLICKED) {
			removeProfile();
			return 0;
		}

		if (Id == ID_SAVE_MAPPINGS && Notify == BN_CLICKED) {
			saveMappingsInfo();
			return 0;
		}

		if (Id == ID_REFRESH_PADS && Notify == BN_CLICKED) {
			SetEnabled(ID_REFRESH_PADS, FALSE);
			// OnGamepadListDeselect();
			InputInit();

			// Repopulate the gamepad list....
			InputGetGamepads(padInfos, &nPadCount);
			GamepadListMake(1);

			// Update the labels in the UI.
			InpdUseUpdate();

			SetEnabled(ID_REFRESH_PADS, TRUE);
			return 0;
		}

		if (Id == ID_SET_PLAYER1 && Notify == BN_CLICKED)
		{
			_DetectPlayerIndex = 0;
			SetState(DIALOGSTATE_SET_PLAYER);
			return 0;
		}
		if (Id == ID_SET_PLAYER2 && Notify == BN_CLICKED)
		{
			_DetectPlayerIndex = 1;
			SetState(DIALOGSTATE_SET_PLAYER);
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

		//if ((Id == IDC_INDP_P1SELECT || Id == IDC_INDP_P2SELECT) && Notify == CBN_SELCHANGE) {
		//	OnPlayerSelectionChanged();
		//	return 0;
		//}

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
		//if (Id == IDC_GAMEPAD_LIST && pnm->code == LVN_ITEMCHANGED) {
		//	SelectGamepadListItem();
		//}

		if (Id == IDC_PROFILE_LIST && pnm->code == LVN_ITEMCHANGED) {
			SelectProfileListItem();
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

// ---------------------------------------------------------------------------------------------------------
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


