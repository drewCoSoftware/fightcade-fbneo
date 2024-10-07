#include "burner.h"

#define INT_INFO_STRINGS (8)

INT32 IntInfoFree(InterfaceInfo* pInfo)
{
	if (pInfo->ppszInterfaceSettings) {
		for (INT32 i = 0; i < INT_INFO_STRINGS; i++) {
			if (pInfo->ppszInterfaceSettings[i]) {
				free(pInfo->ppszInterfaceSettings[i]);
				pInfo->ppszInterfaceSettings[i] = NULL;
			}
		}
	}
	if (pInfo->ppszInterfaceSettings) {
		free(pInfo->ppszInterfaceSettings);
		pInfo->ppszInterfaceSettings = NULL;
	}

	if (pInfo->ppszModuleSettings) {
		for (INT32 i = 0; i < INT_INFO_STRINGS; i++) {
			if (pInfo->ppszModuleSettings[i]) {
				free(pInfo->ppszModuleSettings[i]);
				pInfo->ppszModuleSettings[i] = NULL;
			}
		}
	}
	if (pInfo->ppszModuleSettings) {
		free(pInfo->ppszModuleSettings);
		pInfo->ppszModuleSettings = NULL;
	}

	memset(pInfo, 0, sizeof(InterfaceInfo));

	return 0;
}

INT32 IntInfoInit(InterfaceInfo* pInfo)
{
	IntInfoFree(pInfo);

	pInfo->ppszInterfaceSettings = (TCHAR**)malloc((INT_INFO_STRINGS + 1) * sizeof(TCHAR*));
	if (pInfo->ppszInterfaceSettings == NULL) {
		return 1;
	}
	memset(pInfo->ppszInterfaceSettings, 0, (INT_INFO_STRINGS + 1) * sizeof(TCHAR*));

	pInfo->ppszModuleSettings = (TCHAR**)malloc((INT_INFO_STRINGS + 1) * sizeof(TCHAR*));
	if (pInfo->ppszModuleSettings == NULL) {
		return 1;
	}
	memset(pInfo->ppszModuleSettings, 0, (INT_INFO_STRINGS + 1) * sizeof(TCHAR*));

	return 0;
}

INT32 IntInfoAddStringInterface(InterfaceInfo* pInfo, TCHAR* szString)
{
	INT32 i;

	for (i = 0; pInfo->ppszInterfaceSettings[i] && i < INT_INFO_STRINGS; i++) {}

	if (i >= INT_INFO_STRINGS) {
		return 1;
	}

	pInfo->ppszInterfaceSettings[i] = (TCHAR*)malloc(MAX_PATH * sizeof(TCHAR));
	if (pInfo->ppszInterfaceSettings[i] == NULL) {
		return 1;
	}

	_tcsncpy(pInfo->ppszInterfaceSettings[i], szString, MAX_PATH);

	return 0;
}

INT32 IntInfoAddStringModule(InterfaceInfo* pInfo, TCHAR* szString)
{
	INT32 i;

	for (i = 0; pInfo->ppszModuleSettings[i] && i < INT_INFO_STRINGS; i++) {}

	if (i >= INT_INFO_STRINGS) {
		return 1;
	}

	pInfo->ppszModuleSettings[i] = (TCHAR*)malloc(MAX_PATH * sizeof(TCHAR));
	if (pInfo->ppszModuleSettings[i] == NULL) {
		return 1;
	}

	_tcsncpy(pInfo->ppszModuleSettings[i], szString, MAX_PATH);

	return 0;
}



// ---------------------------------------------------------------------------
// Return an input code (nCode) that is compatible with the emulator.
// NOTE: 'burner/burn' is a term used often as fbNeo = 'final burn neo'
UINT32 GamepadInputEx::GetBurnerCode() const {
	switch (type) {
	case ITYPE_UNSET:
		return 0;

	case ITYPE_BUTTON:
		return BURNER_BUTTON | index;

	case ITYPE_FULL_ANALOG:
	case ITYPE_HALF_ANALOG:
		return BURNER_ANALOG | index;

	case ITYPE_DPAD:
		return BURNER_DPAD | index;

	case ITYPE_KEYBOARD:
		return index;

	default:
		throw std::exception("NOT SUPPORTTED!");
	}
}


// ---------------------------------------------------------------------------
// Get the input for the given player number: 1 = player 1, 2 = player 2, etc.
CInputGroupDesc* CGameInputDescription::GetPlayerGroup(size_t playerNumber) {
	CInputGroupDesc* res = nullptr;
	if (playerNumber <= MaxPlayerCount)
	{
		size_t pi = 0;
		for (size_t i = 0; i < GroupCount; i++)
		{
			if (InputGroups[i].GroupType == IGROUP_PLAYER)
			{
				++pi;
				if (pi == playerNumber) {
					return &InputGroups[i];
				}
			}
		}
	}

	return res;
}
