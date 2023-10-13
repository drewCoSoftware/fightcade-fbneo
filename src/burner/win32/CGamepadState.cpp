#include "burner.h"
#include "CGamepadState.h"

// ------------------------------------------------------------------------------------------
void CButtonState::Update(UINT16 val) {
	bool down = val == 1;

	JustDown = !IsDown & down;
	JustUp = IsDown & !down;
	IsDown = down;
}
// ------------------------------------------------------------------------------------------
void CButtonState::Reset() {
	JustDown = false;
	JustUp = false;
	IsDown = false;
}

// ------------------------------------------------------------------------------------------
CGamepadState::CGamepadState() {
	memset(&Dirs, 0, sizeof(CButtonState) * MAX_DIRS);
	memset(&Buttons, 0, sizeof(CButtonState) * MAX_GAMEPAD_BUTTONS);
}

// ------------------------------------------------------------------------------------------
void CGamepadState::Update(UINT16* dirStates, UINT16* btnStates, DWORD btnCount) {
	for (size_t i = 0; i < MAX_DIRS; i++)
	{
		Dirs[i].Update(dirStates[i]);
	}

	int maxbuttons = (std::min)((int)btnCount, MAX_GAMEPAD_BUTTONS);
	for (size_t i = 0; i < maxbuttons; i++)
	{
		Buttons[i].Update(btnStates[i]);
	}
}

// ------------------------------------------------------------------------------------------
int CGamepadState::GetUpOrDownDelta() {
	if (Dirs[DIR_UP].JustDown) { return DELTA_UP; }
	if (Dirs[DIR_DOWN].JustDown) { return DELTA_DOWN; }
	return DELTA_NONE;
}

// ------------------------------------------------------------------------------------------
int CGamepadState::GetLeftOrRightDelta() {
	if (Dirs[DIR_LEFT].JustDown) { return DELTA_LEFT; }
	if (Dirs[DIR_RIGHT].JustDown) { return DELTA_RIGHT; }
	return DELTA_NONE;
}

// ------------------------------------------------------------------------------------------
/// <summary>
/// Are any of the buttons just down?
/// </summary>
bool CGamepadState::AnyJustDown() {
	for (size_t i = 0; i < MAX_GAMEPAD_BUTTONS; i++)
	{
		if (Buttons[i].JustDown) {
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------
/// <summary>
/// Are any of the buttons just up?
/// </summary>
bool CGamepadState::AnyJustUp() {
	for (size_t i = 0; i < MAX_GAMEPAD_BUTTONS; i++)
	{
		if (Buttons[i].JustUp) {
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------
/// <summary>
/// Are any of the buttons down?
/// </summary>
bool CGamepadState::AnyDown() {
	for (size_t i = 0; i < MAX_GAMEPAD_BUTTONS; i++)
	{
		if (Buttons[i].IsDown) {
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------
/// <summary>
/// Is any direction being pressed?
/// </summary>
/// <returns></returns>
bool CGamepadState::AnyDir() {
	for (size_t i = 0; i < MAX_DIRS; i++)
	{
		if (Dirs[i].IsDown) {
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------
void CGamepadState::Reset() {
	for (size_t i = 0; i < MAX_DIRS; i++) {
		Dirs[i].Reset();
	}
	for (size_t i = 0; i < MAX_GAMEPAD_BUTTONS; i++) {
		Buttons[i].Reset();
	}
}

