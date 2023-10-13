#include "burn.h"

// ==============================================================================================================================
// A simple way to track when a certain button is up/down/etc.
struct CButtonState {

	void Update(UINT16 val);
	void Reset();

	bool JustDown = false;
	bool JustUp = false;
	bool IsDown = false;

};

// ==============================================================================================================================
// Simple way to track the directions + buttons on a gamepad and their up/down state,etc.
class CGamepadState {

public:
	CGamepadState();
	void Update(UINT16* dirStates, UINT16* btnStates, DWORD btnCount);

	int GetUpOrDownDelta();

	int GetLeftOrRightDelta();
	/// <summary>
	/// Are any of the buttons just down?
	/// </summary>
	bool AnyJustDown();

	/// <summary>
	/// Are any of the buttons just up?
	/// </summary>
	bool AnyJustUp();

	/// <summary>
	/// Are any of the buttons down?
	/// </summary>
	bool AnyDown();

	/// <summary>
	/// Is any direction being pressed?
	/// </summary>
	/// <returns></returns>
	bool AnyDir();

	void Reset();

private:
	CButtonState Dirs[MAX_DIRS];
	CButtonState Buttons[MAX_GAMEPAD_BUTTONS];
};
