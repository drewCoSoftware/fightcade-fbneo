// Burner Input Editor Dialog module
// NOTE: This is the <strike>really lousy</strike> war crime of a mapping + preset dialog that comes with the emulator.
// This is where we will make the UI less difficult/cumbersome to use.
#include "burner.h"
#include "GUIDComparer.h"
#include "CGamepadState.h"


#define CB_ITEM_SIZE	20			// TBD

HWND hInpdDlg = NULL;							// Handle to the Input Dialog
static HWND hInpdList = NULL;
static HWND hGamepadList = NULL;
static HWND hProfileList = NULL;

static unsigned short* LastVal = NULL;			// Last input values/defined
static int bLastValDefined = 0;					//

// Combo boxes
static HWND hInpdGi = NULL;
static HWND hInpdPci = NULL;
static HWND hInpdAnalog = NULL;
static HWND hP1Profile = NULL;
static HWND hP2Profile = NULL;

static HWND hP1DeviceList = NULL;
static HWND hP2DeviceList = NULL;

// Get the currently plugged gamepad data:
// NOTE:  We should just get a pointer to this so that we can set the alias directly.
// NOTE: This is just the max number of plugged in devices....
#define MAX_GAMEPAD 8			// Should match the version in inp_dinput.cpp
static GamepadFileEntry* padInfos[MAX_GAMEPAD];
static UINT32 nPadCount = 0;

static CGamepadState PadStates[MAX_GAMEPAD];


// NOTE: These would be game dependent....
static InputProfileEntry DefaultInputProfile;


static InputProfileEntry* inputProfiles[MAX_PROFILE_LEN];
static UINT32 nProfileCount = 0;
static int nSelectedProfileIndex = -1;
static HWND hActivePlayerCombo;

static playerInputs sfiii3nPlayerInputs;

enum EDialogState {
	QUICKPICK_STATE_NONE = 0
	, QUICKPICK_STATE_SET_DEVICE_INDEX			// Set pad index for player.
	, QUICKPICK_STATE_CHOOSE_PROFILE
	, QUICKPICK_STATE_QUICKSETUP_COMPLETE
};
int _TargetPlayerIndex = -1;
int _SelectedPadIndex = -1;			// NOTE: This can and should be replaced with the index of the combobox that lists the selected device!

// Choose profile stuff (placeholder)
int _ProfileIndex = -1;
int _LastDirDelta = 0;

// The last button code that we detected as pressed.
// The input system has a makework system of checking for button presses,
// so we just keep track of this to help us determine when all buttons are released...
INT32 LastPressed = -1;
bool _IsQuickSetupMode = false;
INT32 _QuickSetupIndex = 0;

EDialogState _CurState = QUICKPICK_STATE_NONE;

// #define ALIAS_INDEX 0
#define STATE_INDEX 1
#define GUID_INDEX 0


// Keyboard arrow key codes.
#define KEYB_LEFT 203
#define KEYB_UP 200
#define KEYB_RIGHT 205
#define KEYB_DOWN 208

#define BORDER_WIDTH 4

// This is used to help suggest / find a profile that is associated with
// a certain game pad.  This is just a convenience feature so the combo boxes
// don't always have to start at index zero.
static std::map<GUID, TCHAR[MAX_ALIAS_CHARS], GUIDComparer> _ProfileHints;


static void SetSetupState(EDialogState newState);
static void RefreshProfileData();

// ---------------------------------------------------------------------------------------------------------
static INT32 GetComboIndex(HWND handle) {
	INT32 res = SendMessage(handle, CB_GETCURSEL, 0, 0);
	return res;
}

// ---------------------------------------------------------------------------------------------------------
static INT32 SetComboIndex(HWND handle, int index) {

	SendMessage(handle, CB_SETCURSEL, index, 0);
	return 0;
}

// ---------------------------------------------------------------------------------------------------------
// Enable / disable a control based on its ID.
static void SetEnabled(int id, BOOL bEnable) {
	EnableWindow(GetDlgItem(hInpdDlg, id), bEnable);
}

// -------------------------------------------------------------------------------------
static int GetIndexFromButton(UINT16 button) {
	if (button >= JOYSTICK_LOWER && button < JOYSTICK_UPPER)
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

	if (button >= JOYSTICK_LOWER && button < JOYSTICK_UPPER)
	{
		UINT16 codePart = button & 0xFF;
		int indexPart = (button >> 8) & 0x3F;

		code = codePart;
		index = indexPart;

		return 1;
	}
	else
	{
		// Not a gamepad input.
		return 0;
	}
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

// ------------------------------------------------------------------------------------------------------
// REFACTOR: This should probably be moved over to gami.cpp!
static int SetInputMappings(int padIndex, const InputProfileEntry* profile, int inputIndexOffset)
{
	if (padIndex == -1 || padIndex >= nPadCount) {
		// Do nothing.
		return 1;
	}

	// Otherwise we are going to iterate over the game input + mapped inputs and set the data accordingly.
	int i = 0;
	struct GameInp* pGameInput;
	// Set the correct mem location...
	for (i = 0, pGameInput = GameInp + inputIndexOffset; i < sfiii3nPlayerInputs.buttonCount; i++, pGameInput++) {
		if (pGameInput->Input.pVal == NULL) {
			continue;
		}

		const GamepadInput& pi = profile->Inputs[i];
		if (pi.type == ITYPE_UNSET) { continue; }		// Not set.

		UINT16 code = pi.nCode;
		if (code >= JOYSTICK_LOWER && code < JOYSTICK_UPPER)
		{
			// This is a gamepad input.  We need to translate its index in order to set the code correctly.
			// PS. Making the input codes dependent on the gamepad index is dumb.
			code = code | (padIndex << 8);

			//int checkIndex = (code >> 8) & 0x3F;
			//int x = 10;
		}

		// Now we can set this on the pgi input....
		UINT8 nInput = pi.GetBurnInput();
		if (nInput == GIT_SWITCH) {
			pGameInput->Input.Switch.nCode = code;
		}
		else if (nInput & GIT_GROUP_JOYSTICK) {
			code = 0;		// Joysticks don't have a code, it is all encoded in the nType data....
		}

	}

	// I think that we need to update the description in the main select box too......
	InpdUseUpdate();

	return 0;
}

// ------------------------------------------------------------------------------------------------------
// Returns 0 if the mapping was set correctly.
static int SetPlayerMappings(int playerIndex, int padIndex, int profileIndex)
{
	if (playerIndex == -1 || profileIndex == -1) { return 1; }

	InputProfileEntry* useProfile = inputProfiles[profileIndex];
	int buttonOffset = playerIndex == 0 ? 0 : sfiii3nPlayerInputs.buttonCount;
	SetInputMappings(padIndex, useProfile, buttonOffset);


	// Update our hints map...
	GUID& padGuid = padInfos[_SelectedPadIndex]->info.guidInstance;

	// Copy the name!
	TCHAR* name = _ProfileHints[padGuid];
	wcscpy(name, inputProfiles[profileIndex]->Name);

	// SUCCESS!
	return 0;
}


// --------------------------------------------------------------------------------------------------
static int ResolveHintIndex() {
	int res = 0;
	if (_SelectedPadIndex > 0 && _SelectedPadIndex < nPadCount)
	{
		GUID& padGuid = padInfos[_SelectedPadIndex]->info.guidInstance;
		auto match = _ProfileHints.find(padGuid);
		if (match != _ProfileHints.end())
		{
			// We just need to get the index of the profile now!
			for (size_t i = 0; i < nProfileCount; i++)
			{
				bool areSame = wcscmp(match->second, inputProfiles[i]->Name) == 0;
				if (areSame)
				{
					res = i;
					break;
				}
			}
		}
	}

	return res;
}

// --------------------------------------------------------------------------------------------------
static void InitProfileSelect(int playerIndex)
{

	if (_SelectedPadIndex < 0 || _SelectedPadIndex >= nPadCount || nProfileCount < 1)
	{
		// Invalid ID or no profiles to choose from.
		SetSetupState(QUICKPICK_STATE_NONE);
		return;
	}

	// We will set a selection in the combo box.
	hActivePlayerCombo = 0;
	if (playerIndex == 0) {
		hActivePlayerCombo = hP1Profile;
	}
	else if (playerIndex == 1) {
		hActivePlayerCombo = hP2Profile;
	}

	// Resolve the pad hint..
	int useComboIndex = ResolveHintIndex();

	SetComboIndex(hActivePlayerCombo, useComboIndex);

	TCHAR textBuffer[256];
	swprintf(textBuffer, _T("Choose profile for player %d and press a button"), playerIndex + 1);
	SetDlgItemText(hInpdDlg, IDC_SET_PLAYER_MESSAGE, textBuffer);
	_ProfileIndex = -1;
	_LastDirDelta = -2;
}

// --------------------------------------------------------------------------------------------------
static void InitChooseProfile(int padIndex) {
	_SelectedPadIndex = padIndex;
	SetSetupState(QUICKPICK_STATE_CHOOSE_PROFILE);
}

// ------------------------------------------------------------------------------------------
static void InitSetPlayer(int playerIndex) {

	_TargetPlayerIndex = playerIndex;
	SetSetupState(QUICKPICK_STATE_SET_DEVICE_INDEX);
}

// ------------------------------------------------------------------------------------------
static void SetSetupState(EDialogState newState) {
	TCHAR buffer[256];

	switch (newState) {
	case QUICKPICK_STATE_NONE:
	{
		// Clear the text.
		SetDlgItemText(hInpdDlg, IDC_SET_PLAYER_MESSAGE, _T(""));
		SetEnabled(ID_QUICK_SETUP1, true);
		SetEnabled(ID_QUICK_SETUP2, true);

		_SelectedPadIndex = -1;
		_TargetPlayerIndex = -1;
	}
	break;

	case QUICKPICK_STATE_SET_DEVICE_INDEX:
	{
		TCHAR buffer[256];

		int label = _TargetPlayerIndex + 1;
		swprintf(buffer, _T("Press button for player %d"), label);
		SetDlgItemText(hInpdDlg, IDC_SET_PLAYER_MESSAGE, buffer);
	}
	break;


	case QUICKPICK_STATE_CHOOSE_PROFILE:
	{
		InitProfileSelect(_TargetPlayerIndex);
	}
	break;

	case QUICKPICK_STATE_QUICKSETUP_COMPLETE:
	{
		SetDlgItemText(hInpdDlg, IDC_SET_PLAYER_MESSAGE, _T("Quick setup complete! Press Escape!"));
	}
	break;

	default:
		break;
	}

	_CurState = newState;
}

// --------------------------------------------------------------------------------------------------
static INT32 BeginQuickSetup() {

	if (nPadCount == 0) {
		_IsQuickSetupMode = false;
		return 1;
	}

	// Quick setup puts us in a state where we do profile input for p1, then p2.....
	_QuickSetupIndex = 0;
	InitSetPlayer(_QuickSetupIndex);

	for (size_t i = 0; i < nPadCount; i++)
	{
		PadStates[i].Reset();
	}

	return 0;
}

// --------------------------------------------------------------------------------------------------
static void UpdateGamepadState(int padIndex) {
	UINT16 dirs[MAX_DIRS];
	UINT16 btns[MAX_GAMEPAD_BUTTONS];

	// NOTE: We should just be able to pass in the struct (by pointer) vs. constantly
	// remapping all of the values + doing these stack allocs....
	DWORD btnCount = 0;
	InputGetGamepadState(padIndex, dirs, btns, &btnCount);

	PadStates[padIndex].Update(dirs, btns, btnCount);
}


// --------------------------------------------------------------------------------------------------
static void UpdateGamepadStatuses() {

	LVITEM LvItem;
	TCHAR szVal[16];

	memset(&LvItem, 0, sizeof(LvItem));
	LvItem.mask = LVIF_TEXT;
	//	LvItem.iItem = j;
	LvItem.iSubItem = 1;
	LvItem.pszText = szVal;


	for (size_t i = 0; i < nPadCount; i++)
	{
		bool isActive = PadStates[i].AnyDown() || PadStates[i].AnyDir();
		if (isActive) {
			_tcscpy(szVal, _T("CHECK!"));
		}
		else {
			szVal[0] = 0;
		}

		LvItem.iItem = i;
		SendMessage(hGamepadList, LVM_SETITEM, 0, (LPARAM)&LvItem);
	}

}

// --------------------------------------------------------------------------------------------------
static void ProcessQuickpickState() {

	INT32 pressed = -1;
	int padIndex = -1;

	if (_CurState == QUICKPICK_STATE_NONE)
	{
		// Check for quick setup mode and handle accordingly......
		if (_IsQuickSetupMode) {
			BeginQuickSetup();
			return;
		}
	}
	else if (_CurState == QUICKPICK_STATE_SET_DEVICE_INDEX) {

		for (size_t i = 0; i < nPadCount; i++) {
			if (PadStates[i].AnyJustDown()) {
				padIndex = i;
				break;
			}
		}

		if (padIndex != -1) {
			HWND deviceCombo = _TargetPlayerIndex == 0 ? hP1DeviceList : hP2DeviceList;
			SetComboIndex(deviceCombo, padIndex);
			InitChooseProfile(padIndex);
		}

	}
	else if (_CurState == QUICKPICK_STATE_CHOOSE_PROFILE) {

		// We only care about the active gamepad.
		// This lets the spazz next to you bang away on their hitbox like a coked up monkey while you
		// make your pick.
		bool buttonPressed = PadStates[_SelectedPadIndex].AnyJustDown();

		// Here we want to detect the up/down buttons so that
		// we can cycle through the available profiles.
		int dirDelta = PadStates[_SelectedPadIndex].GetUpOrDownDelta();



		// Maybe left or right?
		if (dirDelta == 0)
		{
			dirDelta = PadStates[_SelectedPadIndex].GetLeftOrRightDelta();
		}
		//else{
		//	// TEMP: release mode debugging.....
		//	// We are getting weird values for the deltas, and even when the mouse is
		//	// moved.  This indicates a memory problem
		//  // Indeed, the calls for getting the direction deltas didn't always return a value.
		//	TCHAR textBuffer[256];
		//	swprintf(textBuffer, _T("DELTA IS: %i"), dirDelta);
		//	SetDlgItemText(hInpdDlg, IDC_SET_PLAYER_MESSAGE, textBuffer);
		//}


		bool dirChanged = dirDelta != _LastDirDelta;
		if (dirChanged) {
			_LastDirDelta = dirDelta;

			int curIndex = GetComboIndex(hActivePlayerCombo);
			int newIndex = curIndex + dirDelta;

			int maxIndex = nProfileCount - 1;

			if (newIndex < 0) { newIndex = maxIndex; }
			if (newIndex > maxIndex) { newIndex = 0; }

			SetComboIndex(hActivePlayerCombo, newIndex);
		}
		else if (buttonPressed) {

			// User didn't press a direction button, but did press something
			// else.  They must be happy with their profile selection....
			// This is where we will copy the profile buttons over to the input interface.....
			int profileIndex = GetComboIndex(hActivePlayerCombo);
			SetPlayerMappings(_TargetPlayerIndex, _SelectedPadIndex, profileIndex);

			// Disable both combos when the players are set.
			//SetEnabled(IDC_INDP_P1PROFILE, false);
			//SetEnabled(IDC_INDP_P2PROFILE, false);

			if (_IsQuickSetupMode) {
				_QuickSetupIndex++;
				if (_QuickSetupIndex == sfiii3nPlayerInputs.maxPlayers) {

					// We are done with setup, so let's close the dialog ?? 
					_IsQuickSetupMode = false;
					SetSetupState(QUICKPICK_STATE_QUICKSETUP_COMPLETE);

					// TODO: Figure out how we close the dialog....
					// Actually, I think we want to consider it done!
					return;
				}

				InitSetPlayer(_QuickSetupIndex);
			}
			else {
				SetSetupState(QUICKPICK_STATE_NONE);
			}
		}
	}

}


// ---------------------------------------------------------------------------------------------
// This is where we can wait for p1/p2 to check in and what-not....
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

	// Update all states so we can signal activity in the gamepad list.
	for (int i = 0; i < nPadCount; i++) {
		UpdateGamepadState(i);
	}

	UpdateGamepadStatuses();
	ProcessQuickpickState();


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
	LvCol.mask = LVCF_TEXT | LVCF_WIDTH;

	RECT r;
	GetWindowRect(hGamepadList, &r);
	LvCol.cx = (r.right - r.left) - BORDER_WIDTH;
	LvCol.pszText = _T("Name"); //TODO: Localize  // FBALoadStringEx(hAppInst, IDS_GAMEPAD_ALIAS, true);
	SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);

	return 0;
}


// ---------------------------------------------------------------------------------------------------------
static int GamepadListBegin()
{
	if (hGamepadList == NULL) {
		return 1;
	}


	RECT r;
	GetWindowRect(hGamepadList, &r);

	const int GUID_WIDTH = 200;
	int stateWidth = (r.right - r.left) - GUID_WIDTH - BORDER_WIDTH;


	// Full row select style:
	SendMessage(hGamepadList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);


	// Make column headers
	LVCOLUMN LvCol;
	memset(&LvCol, 0, sizeof(LvCol));
	LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

	LvCol.cx = GUID_WIDTH;// 0xa5;		// Column Width.
	LvCol.pszText = FBALoadStringEx(hAppInst, IDS_GAMEPAD_GUID, true);
	SendMessage(hGamepadList, LVM_INSERTCOLUMN, GUID_INDEX, (LPARAM)&LvCol);

	LvCol.cx = stateWidth; // 100; //0x95;
	LvCol.pszText = FBALoadStringEx(hAppInst, IDS_INPUT_STATE, true);
	SendMessage(hGamepadList, LVM_INSERTCOLUMN, STATE_INDEX, (LPARAM)&LvCol);


	return 0;
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

	TCHAR buffer[MAX_ALIAS_CHARS];


	// Populate the list:
	for (unsigned int i = 0; i < nPadCount; i++) {
		GamepadFileEntry* pad = padInfos[i];

		LVITEM LvItem;

		// Set the name of the gamepad....
		// memset(buffer, 0, 1);
		wsprintf(buffer, _T("Joy %u"), i);

		// Populate the GUID column
		memset(&LvItem, 0, sizeof(LvItem));
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = i;
		LvItem.iSubItem = GUID_INDEX;
		LvItem.pszText = buffer; //GUIDToTCHAR(&pad->info.guidInstance);
		SendMessage(list, LVM_INSERTITEM, 0, (LPARAM)&LvItem);

		// Populate the STATE column (empty is fine!)
		// This gets set when we detect input from an attached gamepad!
		memset(&LvItem, 0, sizeof(LvItem));
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = i;
		LvItem.iSubItem = STATE_INDEX;
		LvItem.pszText = _T("");
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

	TCHAR textBuffer[MAX_ALIAS_CHARS];

	// Populate the list:
	for (unsigned int i = 0; i < nProfileCount; i++) {
		InputProfileEntry* profile = inputProfiles[i];

		SetStrBuffer(textBuffer, MAX_ALIAS_CHARS, profile->Name);

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

// ------------------------------------------------------------------------------------------------------
void getListItemData(HWND& list, int index, LVITEM& item, TCHAR* textBuffer) {

	item.mask = LVIF_TEXT;
	item.iItem = index;
	item.iSubItem = 0;
	item.pszText = textBuffer;
	item.cchTextMax = MAX_ALIAS_CHARS;
	SendMessage(list, LVM_GETITEM, 0, (LPARAM)&item);
}

// ------------------------------------------------------------------------------------------------------
// Get the text from an input control.
void getText(int textId, TCHAR* textBuffer) {
	SendDlgItemMessage(hInpdDlg, textId, WM_GETTEXT, (WPARAM)MAX_ALIAS_CHARS, (LPARAM)textBuffer);
}


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

	TCHAR textBuffer[MAX_ALIAS_CHARS];

	// Get the corresponding input
	nSelectedProfileIndex = nSel;
	getListItemData(list, nSel, LvItem, textBuffer);

	if (nSel >= nProfileCount) {
		OnProfileListDeselect();
		return 1;
	}

	// Activate certain buttons.
	SetEnabled(ID_REMOVE_PROFILE, true);
	SetEnabled(ID_SAVE_MAPPINGS, true);

	return 0;
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
static void SetComboBoxDropdownSize(HWND cb, int itemCount, int itemSize) {
	if (cb)
	{
		RECT r;
		GetWindowRect(cb, &r);
		LONG useWidth = r.right - r.left;

		// In theory we would use code like this to automatically size the content.
		// Not getting it to work reliably, but our heuristic is good enough so NBD.
		//auto selSize=  SendMessage(cb, CB_GETITEMHEIGHT, -1, NULL);
		//auto itemSize = SendMessage(cb, CB_GETITEMHEIGHT, 0, NULL);		// NOTE: This sometimes returns zero....

		int totalSize = (itemCount + 1) * itemSize;
		SetWindowPos(cb, NULL, 0, 0, useWidth, totalSize, SWP_NOMOVE);
	}
}

// ------------------------------------------------------------------------------------------
static void RefreshPlayerSelectComboBoxes() {
	// TODO: Keep track of current selections?
	// This would be kind of part of some feature where we track the pad guid on the background
	// and just make associations as we go.  Actually, that would be retty easy to do....

	// Get the list of active gamepads, and populate each combox box with that data.
	SendMessage(hP1Profile, CB_RESETCONTENT, 0, 0);
	SendMessage(hP2Profile, CB_RESETCONTENT, 0, 0);

	for (size_t i = 0; i < nProfileCount; i++)
	{
		TCHAR* padAlias = inputProfiles[i]->Name;
		SendMessage(hP1Profile, CB_ADDSTRING, 0, (LPARAM)padAlias);
		SendMessage(hP2Profile, CB_ADDSTRING, 0, (LPARAM)padAlias);
	}

	// Set an appropriate size for the combos.  Note that we can base this on the number of items.
	UINT32 useCount = (std::max)(4u, nProfileCount);
	useCount = (std::min)(16u, useCount);
	useCount = (std::min)(16u, useCount);
	SetComboBoxDropdownSize(hP1Profile, useCount, CB_ITEM_SIZE);
	SetComboBoxDropdownSize(hP2Profile, useCount, CB_ITEM_SIZE);
}

// ------------------------------------------------------------------------------------------
static void RefreshPlayerDeviceComboBoxes() {
	// TODO: Keep track of current selections?
	// This would be kind of part of some feature where we track the pad guid on the background
	// and just make associations as we go.  Actually, that would be retty easy to do....

	// Get the list of active gamepads, and populate each combox box with that data.
	SendMessage(hP1DeviceList, CB_RESETCONTENT, 0, 0);
	SendMessage(hP2DeviceList, CB_RESETCONTENT, 0, 0);

	TCHAR buffer[MAX_ALIAS_CHARS];
	for (size_t i = 0; i < nPadCount; i++)
	{
		// NOTE: We should either read this from the actual populated gamepad list,
		// or we should create a function to compute the string!
		wsprintf(buffer, _T("Joy %u"), i);
		//		TCHAR* padAlias = inputProfiles[i]->Name;
		SendMessage(hP1DeviceList, CB_ADDSTRING, 0, (LPARAM)buffer);
		SendMessage(hP2DeviceList, CB_ADDSTRING, 0, (LPARAM)buffer);
	}

	UINT32 useCount = (std::max)(4u, nPadCount);
	SetComboBoxDropdownSize(hP1DeviceList, useCount, CB_ITEM_SIZE);
	SetComboBoxDropdownSize(hP2DeviceList, useCount, CB_ITEM_SIZE);

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

	// We want to always leave these comobos on for manual setting!
	//SetEnabled(IDC_INDP_P1PROF"),ILE, false);
	//SetEnabled(IDC_INDP_P2PROFILE, false);

	RefreshPlayerDeviceComboBoxes();
}


// ------------------------------------------------------------------------------------------------------------
// Changes in pads / profiles means some of the data in our hints map may be stale/invalid.
// We will update it at this time to fix....
static void RefreshProfileHints() {

	// For every hint that we have, make sure that both the profile (by name) and gamepad exist.
	// If they don't, we will remove it.....
	std::vector<GUID> toRemove;
	for (auto it = _ProfileHints.begin(); it != _ProfileHints.end(); it++) {

		bool foundPad = false;
		const GUID& findGuid = it->first;
		for (size_t i = 0; i < nPadCount; i++) {
			if (findGuid == padInfos[i]->info.guidInstance) {
				foundPad = true;
				break;
			}
		}

		if (!foundPad) {
			toRemove.push_back(findGuid);
		}

		// Let's see if the profile exists too.....
		bool foundProfile = false;
		TCHAR* findName = it->second;
		for (size_t i = 0; i < nProfileCount; i++)
		{
			bool areSame = wcscmp(findName, inputProfiles[i]->Name) == 0;
			if (areSame) {
				foundProfile = true;
				break;
			}
		}
		if (!foundProfile) {
			toRemove.push_back(findGuid);
		}
	}

	// Remove all hints where data could not be found.
	int len = toRemove.size();
	for (size_t i = 0; i < len; i++)
	{
		_ProfileHints.erase(toRemove[i]);
	}

}

// ------------------------------------------------------------------------------------------------------------
static void RefreshGamepads() {
	InputGetGamepads(padInfos, &nPadCount);
	RefreshProfileData();
	RefreshPlayerDeviceComboBoxes();
}

// ------------------------------------------------------------------------------------------------------------
static void RefreshProfileData() {
	InputGetProfiles(inputProfiles, &nProfileCount);

	// Refresh / update our hint map.....
	RefreshProfileHints();
}

// ------------------------------------------------------------------------------------------------------------
static int InpdInit()
{
	// We are just going to hard-code this mapping for now.
	// All of this is for playing 3rd strike in a group setting, so it will be OK for now.
	auto& pi = sfiii3nPlayerInputs;
	pi.p1Index = 0;
	pi.p2Index = 12;
	pi.buttonCount = 12;
	pi.maxPlayers = 2;

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

	RefreshGamepads();

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
	hP1Profile = GetDlgItem(hInpdDlg, IDC_INDP_P1PROFILE);
	hP2Profile = GetDlgItem(hInpdDlg, IDC_INDP_P2PROFILE);

	hP1DeviceList = GetDlgItem(hInpdDlg, IDC_INDP_P1DEVICES);
	hP2DeviceList = GetDlgItem(hInpdDlg, IDC_INDP_P2DEVICES);

	InitComboboxes();

	DisablePresets();


	SetSetupState(QUICKPICK_STATE_NONE);
	return 0;
}

// ------------------------------------------------------------------------------------------------------------
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
		UINT8 nInput = pgi->nInput;

		//pi.SetInputType(pgi->nInput);
		//pi.nInput = pgi->nInput;

		UINT16 code = 0; //pgi->Input.nVal;


		if (nInput == GIT_SWITCH) {
			code = pgi->Input.Switch.nCode;
		}
		else if (nInput & GIT_GROUP_JOYSTICK) {
			code = 0;		// Joysticks don't have a code, it is all encoded in the nType data....
		}
		else {
			// Not supported?
			continue;
		}

		// NOTE: This is where we need to translated gamepad codes....
		if (code >= JOYSTICK_LOWER && code < JOYSTICK_UPPER) {
			INT32 index = (code >> 8) & 0x3F;
			code = code & 0xFF;

			// We are going to put in a gamepad bit as this is how the input coding system works...
			code |= JOYSTICK_LOWER;
		}
		pi.nCode = code;
	}

	InputSaveProfiles();
}

// ---------------------------------------------------------------------------------------------------------
// Add a new input mapping profile into the system.
static void addNewProfile() {

	TCHAR textBuffer[MAX_ALIAS_CHARS];
	getText(IDC_PROFILE_NAME, textBuffer);

	// No text, bail.
	// TODO: We can track text events to disable the button, check for dupes, etc.
	if (textBuffer == 0 || wcslen(textBuffer) == 0)
	{
		return;
	}

	INT32 added = InputAddInputProfile(textBuffer);
	if (added == 0)
	{
		// Repopulate the alias list.
		RefreshProfileData();
		ProfileListMake(1);

		RefreshPlayerSelectComboBoxes();
		RefreshPlayerDeviceComboBoxes();
	}

}

// ---------------------------------------------------------------------------------------------------------
static void removeProfile() {

	if (nSelectedProfileIndex == -1) { return; }
	InputRemoveProfile(nSelectedProfileIndex);

	OnProfileListDeselect();

	RefreshPlayerSelectComboBoxes();
	RefreshPlayerDeviceComboBoxes();
	RefreshProfileData();
	ProfileListMake(1);
}

HFONT hMsgFont = 0;

// ---------------------------------------------------------------------------------------------------------
static void OnRefreshPadsClicked(bool reinitializeInput) {
	SetEnabled(ID_REFRESH_PADS, FALSE);

	if (reinitializeInput) {
		InputInit();
	}

	// Repopulate the gamepad list....
	RefreshGamepads();
	GamepadListMake(1);

	// Update the labels in the UI.
	InpdUseUpdate();

	SetEnabled(ID_REFRESH_PADS, TRUE);
}

// ---------------------------------------------------------------------------------------------------------
static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == UM_INPUT_CHANGE) {
		if (InSendMessage()) {
			ReplyMessage(true);
		}

		//// NOTE: The wParam can be inspected for the exact change...
		//if (wParam == DBT_DEVICEARRIVAL) { added... }
		//if (wParam == DBT_DEVICEREMOVECOMPLETE) { removed... }

		OnRefreshPadsClicked(false);
	}

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
			return 0;
		}
		if (Id == IDCANCEL && Notify == BN_CLICKED) {
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			return 0;
		}

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
			OnRefreshPadsClicked(true);
			return 0;
		}

		if (Id == ID_QUICK_SETUP1 && Notify == BN_CLICKED)
		{
			_TargetPlayerIndex = 0;
			SetSetupState(QUICKPICK_STATE_SET_DEVICE_INDEX);
			return 0;
		}
		if (Id == ID_QUICK_SETUP2 && Notify == BN_CLICKED)
		{
			_TargetPlayerIndex = 1;
			SetSetupState(QUICKPICK_STATE_SET_DEVICE_INDEX);
			return 0;
		}

		// The manual 'Set!' buttons....
		// Player 1
		if (Id == ID_SET_INPUT1 && Notify == BN_CLICKED) {
			UINT32 deviceIndex = GetComboIndex(hP1DeviceList);
			// HACK: This value should be passed to the associatd functions!
			UINT32 profileIndex = GetComboIndex(hP1Profile);

			_SelectedPadIndex = deviceIndex;
			SetPlayerMappings(0, deviceIndex, profileIndex);
			return 0;
		}
		// Player 2
		if (Id == ID_SET_INPUT2 && Notify == BN_CLICKED) {
			UINT32 deviceIndex = GetComboIndex(hP2DeviceList);
			UINT32 profileIndex = GetComboIndex(hP2Profile);

			_SelectedPadIndex = deviceIndex;
			SetPlayerMappings(1, deviceIndex, profileIndex);
			return 0;
		}

		if (Id == IDC_INPD_NEWMACRO && Notify == BN_CLICKED) {
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
int InpdCreate(bool quickSetup)
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


	// NOTE: We can't really send this data to the FBACreateDialog function (and then to the WM_INITDIALOG code)
	// So we will just set the flag here.
	_IsQuickSetupMode = quickSetup;
	if (_IsQuickSetupMode) {
		SetWindowText(hInpdDlg, _T("Quick Setup Mode!"));
	}
	return 0;
}


