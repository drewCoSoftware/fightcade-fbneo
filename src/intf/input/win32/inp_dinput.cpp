// DirectInput module
// updated to dinput8 by regret

#include "burner.h"
#include "inp_keys.h"

#include <InitGuid.h>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "dinput_core.h"

#pragma comment(lib, "dinput8")

#define MAX_KEYBOARD		(1)
#define MAX_GAMEPAD			(8)
#define MAX_GAMEPAD_INFOS	(32)
#define MAX_JOYAXIS			(8)
#define MAX_MOUSE			(4)
#define MAX_MOUSEAXIS		(3)

static BOOL CALLBACK gamepadEnumAxesCallback(LPCDIDEVICEOBJECTINSTANCE, LPVOID);
static BOOL CALLBACK mouseEnumAxesCallback(LPCDIDEVICEOBJECTINSTANCE, LPVOID);
static BOOL CALLBACK gamepadEnumCallback(LPCDIDEVICEINSTANCE, LPVOID);
static BOOL CALLBACK mouseEnumCallback(LPCDIDEVICEINSTANCE, LPVOID);

struct keyboardData {
	IDirectInputDevice8W* lpdid;
	unsigned char state[256];
	unsigned char readStatus;
} keyboardProperties[MAX_KEYBOARD];

struct gamepadData {
	IDirectInputDevice8W* lpdid;
	GUID guidInstance;								// Per-Machine identifier for the gamepad.
	DIJOYSTATE2 dijs;
	DWORD dwAxisType[MAX_JOYAXIS];
	DWORD dwAxisBaseline[MAX_JOYAXIS];
	DWORD dwAxes;
	DWORD dwPOVs;
	DWORD dwButtons;
	unsigned char readStatus;
} gamepadProperties[MAX_GAMEPAD];


struct GamepadFileData {
	GamepadFileEntry entries[MAX_GAMEPAD_INFOS];
	UINT16 entryCount;

} gamepadFile;

// Convenience:
// Thanks internet!
// https://stackoverflow.com/questions/29436835/guid-as-stdmap-key
struct GUIDComparer
{
	bool operator()(const GUID& Left, const GUID& Right) const
	{
		// comparison logic goes here
		return memcmp(&Left, &Right, sizeof(Right)) < 0;
	}
};
std::map<GUID, GamepadFileEntry*, GUIDComparer> _entryMap;


struct mouseData {
	IDirectInputDevice8W* lpdid;
	DIMOUSESTATE2 dims;
	DWORD dwAxisType[MAX_MOUSEAXIS];
	DWORD dwAxes;
	DWORD dwButtons;
	unsigned char readStatus;
} mouseProperties[MAX_MOUSE];

int keyboardCount;		// Number of keyboards connected to this machine
int gamepadCount;		// Number of gamepads connected to this machine
int mouseCount;			// Number of mice connected to this machine
bool keyboardCooperativeModeForeground = false;

IDirectInput8W* pDI;
HWND hDinpWnd;

int gamepadInitSingle(LPCDIDEVICEINSTANCE instance)
{
	gamepadData* gamepad = &gamepadProperties[gamepadCount];
	if (gamepad->lpdid == NULL) {
		return 1;
	}

	if (FAILED(gamepad->lpdid->SetDataFormat(&c_dfDIJoystick2))) {
		return 1;
	}

	DIDEVCAPS didcl;
	memset(&didcl, 0, sizeof(didcl));
	didcl.dwSize = sizeof(DIDEVCAPS);
	if (FAILED(gamepad->lpdid->GetCapabilities(&didcl))) {
		return 1;
	}

	gamepad->guidInstance = instance->guidInstance;
	gamepad->dwAxes = didcl.dwAxes;
	gamepad->dwPOVs = didcl.dwPOVs;
	gamepad->dwButtons = didcl.dwButtons;

	gamepad->lpdid->SetCooperativeLevel(hDinpWnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
	gamepad->lpdid->EnumObjects(gamepadEnumAxesCallback, /*(void*)this*/ gamepad->lpdid, DIDFT_AXIS);
	gamepad->lpdid->Acquire();

	return 0;
}

int mouseInitSingle()
{
	mouseData* mouse = &mouseProperties[mouseCount];
	if (mouse->lpdid == NULL) {
		return 1;
	}

	if (FAILED(mouse->lpdid->SetDataFormat(&c_dfDIMouse2))) {
		return 1;
	}

	DIDEVCAPS didcl;
	memset(&didcl, 0, sizeof(didcl));
	didcl.dwSize = sizeof(didcl);
	mouse->lpdid->GetCapabilities(&didcl);

	mouse->dwAxes = didcl.dwAxes;
	mouse->dwButtons = didcl.dwButtons;

	mouse->lpdid->SetCooperativeLevel(hDinpWnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
	mouse->lpdid->EnumObjects(mouseEnumAxesCallback, /*(void*)this*/mouse->lpdid, DIDFT_AXIS);
	mouse->lpdid->Acquire();

	return 0;
}

bool mouseEnumDevice(LPCDIDEVICEINSTANCE instance)
{
	if (!instance) {
		return DIENUM_CONTINUE;
	}

	if (mouseCount >= MAX_MOUSE) {
		return DIENUM_STOP;
	}

	// Create the DirectInput interface
	if (FAILED(pDI->CreateDevice(instance->guidInstance, &mouseProperties[mouseCount].lpdid, NULL))) {
		return DIENUM_CONTINUE;
	}

	if (!mouseInitSingle()) {
		mouseCount++;
	}

	return DIENUM_CONTINUE;
}

bool gamepadEnumDevice(LPCDIDEVICEINSTANCE instance)
{
	if (!instance) {
		return DIENUM_CONTINUE;
	}

	if (gamepadCount >= MAX_GAMEPAD) {
		return DIENUM_STOP;
	}

	// Create the DirectInput interface
	if (FAILED(pDI->CreateDevice(instance->guidInstance, &gamepadProperties[gamepadCount].lpdid, NULL))) {
		return DIENUM_CONTINUE;
	}

	// This function initializes the gamepad using the 'gamePadCount' variable below.  Not sloppy at all.
	if (gamepadInitSingle(instance) == 0) {
		gamepadCount++;
	}

	return DIENUM_CONTINUE;
}

bool mouseEnumObject(LPCDIDEVICEOBJECTINSTANCE instance)
{
	mouseData* p = &mouseProperties[mouseCount];

	// Store the flags for later use
	((mouseData*)p)->dwAxisType[DIDFT_GETINSTANCE(instance->dwType)] = instance->dwType;
	return DIENUM_CONTINUE;
}

bool gamepadEnumObject(LPCDIDEVICEOBJECTINSTANCE instance)
{
	if (!(DIDFT_GETTYPE(instance->dwType) & DIDFT_AXIS)) {
		return DIENUM_CONTINUE;
	}

	gamepadData* p = &gamepadProperties[gamepadCount];
	p->dwAxisType[DIDFT_GETINSTANCE(instance->dwType)] = instance->dwType;

	// Set axis range
	DIPROPRANGE range;
	range.diph.dwSize = sizeof(range);
	range.diph.dwHeaderSize = sizeof(range.diph);
	range.diph.dwHow = DIPH_BYID;
	range.diph.dwObj = instance->dwType;
	range.lMin = -32768;
	range.lMax = +32767;
	if (FAILED(p->lpdid->SetProperty(DIPROP_RANGE, &range.diph))) {
		if (FAILED(p->lpdid->GetProperty(DIPROP_RANGE, &range.diph)) || range.lMin >= 0 || range.lMax <= 0) {
			return DIENUM_CONTINUE;
		}
	}

	return DIENUM_CONTINUE;
}

int setCooperativeLevel(bool exclusive, bool foreGround)
{
	if (keyboardProperties[0].lpdid && keyboardCooperativeModeForeground != foreGround) {
		keyboardCooperativeModeForeground = foreGround;
		keyboardProperties[0].lpdid->Unacquire();
		if (foreGround) {
			keyboardProperties[0].lpdid->SetCooperativeLevel(hDinpWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND | (nVidFullscreen ? DISCL_NOWINKEY : 0));
		}
		else {
			keyboardProperties[0].lpdid->SetCooperativeLevel(hDinpWnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND | (nVidFullscreen ? DISCL_NOWINKEY : 0));
		}
		keyboardProperties[0].lpdid->Acquire();
	}

	// mouse 0 is the system mouse
	if (mouseProperties[0].lpdid) {
		mouseProperties[0].lpdid->Unacquire();
		if (exclusive) {
			POINT point = { 0, 0 };
			RECT rect;

			// Ensure the cursor is centered on the input window
			ClientToScreen(hDinpWnd, &point);
			GetClientRect(hDinpWnd, &rect);
			SetCursorPos(point.x + rect.right / 2, point.y + rect.bottom / 2);

			SetForegroundWindow(hDinpWnd);
			SetWindowPos(hDinpWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

			mouseProperties[0].lpdid->SetCooperativeLevel(hDinpWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
		}
		else {
			mouseProperties[0].lpdid->SetCooperativeLevel(hDinpWnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
		}
		mouseProperties[0].lpdid->Acquire();
	}

	// Windows/DirectInput don't always hide the cursor for us
	if (bDrvOkay && (exclusive || nVidFullscreen)) {
		while (ShowCursor(FALSE) >= 0) {}
	}
	else {
		while (ShowCursor(TRUE) < 0) {}
	}

	return 0;
}

// NOTE: This releases all devices + cleans up DX interfaces.
int exit()
{
	// Release the keyboard interface
	for (int i = 0; i < MAX_KEYBOARD; i++) {
		RELEASE(keyboardProperties[i].lpdid)
	}
	keyboardCount = 0;

	// Release the gamepad interfaces
	for (int i = 0; i < MAX_GAMEPAD; i++) {
		RELEASE(gamepadProperties[i].lpdid)
	}
	gamepadCount = 0;

	// Release the mouse interface
	for (int i = 0; i < MAX_MOUSE; i++) {
		RELEASE(mouseProperties[i].lpdid)
	}
	mouseCount = 0;

	// Release the DirectInput interface
	RELEASE(pDI)

		return 0;
}

// ---------------------------------------------------------------------------------------------------------
// IDK a good name for this.....
template <typename T>
void WriteData_G(FILE* f, T data)  {
	fwrite(&data, sizeof(T), 1, f);
}

// ---------------------------------------------------------------------------------------------------------
// IDK a good name for this.....
template <typename T>
T ReadData_G(FILE* f) {
	T res;
	fread(&res, sizeof(T), 1, f);
	return res;
}
//
//
//// ---------------------------------------------------------------------------------------------------------
//void WriteData(FILE* f, GUID g) {
//	WriteData_G(f, g.Data1);
//	WriteData_G(f, g.Data2);
//	WriteData_G(f, g.Data3);
//	WriteData_G(f, g.Data4);
//}
//
//// ---------------------------------------------------------------------------------------------------------
//GUID ReadGUID(FILE* f) {
//	GUID res;
//	res.Data1 = 
//}

// ---------------------------------------------------------------------------------------------------------
void WriteData(FILE* f, UINT16 data) {
	fwrite(&data, sizeof(UINT16), 1, f);
}

// ---------------------------------------------------------------------------------------------------------
void WriteTCHAR(FILE* f, TCHAR* data, int len) {
	fwrite(data, sizeof(TCHAR), len, f);
}

// ---------------------------------------------------------------------------------------------------------
void WriteData(FILE* f, CHAR* data, int len) {
	fwrite(data, sizeof(CHAR), len, f);
}

// ---------------------------------------------------------------------------------------------------------
void ReadData(FILE* f, CHAR* data, int len) {
	fread(data, sizeof(CHAR), len, f);
}

// ---------------------------------------------------------------------------------------------------------
void ReadTCHAR(FILE* f, TCHAR* data, int len) {
	fread(data, sizeof(TCHAR), len, f);
}


// ---------------------------------------------------------------------------------------------------------
UINT16 ReadUint16(FILE* f) {
	UINT16 buf;
	fread(&buf, sizeof(UINT16), 1, f);
	return buf;
}


#define GUID_BUFFER_SIZE 40

// ---------------------------------------------------------------------------------------------------------
void WriteButtonData(FILE* fp, GamepadInputProfile& profile) {

	for (size_t i = 0; i < MAX_INPUTS; i++)
	{
		GamepadInput& input = profile.inputs[i];

		// NOTE: I am not supporting macros at this time.
		WriteData(fp, input.nType);
		WriteData(fp, input.nCode);
	}

}

// ---------------------------------------------------------------------------------------------------------
void ReadButtonData(FILE* fp, GamepadInputProfile& profile) {

	for (size_t i = 0; i < MAX_INPUTS; i++)
	{
		GamepadInput& input = profile.inputs[i];

		// NOTE: I am not supporting macros at this time.
		input.nType = ReadUint16(fp);
		input.nCode = ReadUint16(fp);
	}

}

// ---------------------------------------------------------------------------------------------------------
void clearGamepadMappingData() {
	// Compose the mappings data.  This will end up being our source of data for 'getGamepadInfos'....
	memset(&gamepadFile, 0, sizeof(GamepadFileData));
}

// ---------------------------------------------------------------------------------------------------------
void createGamepadMappingData() {

	// NOTE: This should happen during the merge step.....	
	gamepadFile.entryCount = gamepadCount;
	for (int i = 0; i < gamepadCount; i++)
	{
		GamepadFileEntry& e = gamepadFile.entries[i];
		e.info.guidInstance = gamepadProperties[i].guidInstance;

		// Set our nice, internal alias.
		_entryMap[e.info.guidInstance] = &e;
	}
}

// ---------------------------------------------------------------------------------------------------------
// Save all of the gamepad data to disk.  Alias + button mappings.....
INT32 saveGamepadMappings() {

	if (gamepadFile.entryCount == 0 || gamepadCount == 0) { return 0; }

	TCHAR* szFileName = _T("config/gamepads.dat");
	FILE* fp = _tfopen(szFileName, _T("wb"));
	if (!fp)
	{
		// TODO: Handle this better!
		throw ("File save failed!");
	}
	else
	{
		// Write the file header..
		UINT16 version = 1;
		WriteData(fp, "GP-MAP", 6);
		WriteData(fp, version);

		// Entry Count
		WriteData(fp, gamepadFile.entryCount);

		// Write all entries.
		//CHAR guidBuffer[GUID_BUFFER_SIZE];
		for (size_t i = 0; i < gamepadFile.entryCount; i++)
		{
			GamepadFileEntry& e = gamepadFile.entries[i];
		//	FormatGUID(&e.info.guidInstance, guidBuffer, GUID_BUFFER_SIZE);

			// Write out the info....
			WriteData_G(fp, e.info.guidInstance);
			WriteTCHAR(fp, e.info.Alias, MAX_ALIAS_CHARS);

			// Now we can include the profile data....
			UINT16 profileCount = 1; //e.profileCount;
			WriteData(fp, profileCount);

			// NOTE: We are only serializing one profile at this time....
			//for (size_t j = 0; j < profileCount; j++)
			//{
			GamepadInputProfile& p = e.profile; //[j];
			// }

			WriteTCHAR(fp, p.driverName, MAX_NAME);
			WriteTCHAR(fp, p.profileName, MAX_NAME);

			WriteButtonData(fp, p);
		}
	}

	// TEST: We are only going to save aliases 
	// getGamepadInfos(

	return 0;
}


// ---------------------------------------------------------------------------------------------------------
void addGamepadEntry(gamepadData& props){
	
	UINT16 index = gamepadFile.entryCount;
	if (index >= MAX_GAMEPAD_INFOS) {
		throw "TOO MANY PROFILES!";
	}
	gamepadFile.entryCount += 1;

	// This is all we need, just to add the guid....
	GamepadFileEntry& e = gamepadFile.entries[index];
	e.info.guidInstance = props.guidInstance;

	// This is refreshed in a subsequent step....
	// _entryMap[e.info.guidInstance] = &e;

	//strcpy(e.info.Alias, _T("
	//e.info.
	// Set our nice, internal alias.

}


// ---------------------------------------------------------------------------------------------------------
void refreshEntryMap() {
	// Create our convenient guid map..
	_entryMap.clear();
	size_t len = gamepadFile.entryCount;
	for (size_t i = 0; i < len; i++)
	{
		auto& e = gamepadFile.entries[i];
		_entryMap[e.info.guidInstance] = &e;
	}
}

// ---------------------------------------------------------------------------------------------------------
// Merge any *new* gamepads that have been detected with the current data file.
void mergeGamepadMappings() {

	// NOTE: This should happen during the merge step.....	
	bool saveData = false;
	//gamepadFile.entryCount = gamepadCount;
	for (int i = 0; i < gamepadCount; i++)
	{
		// Check for entry.  If there isn't one, we will create something new!
		auto match = _entryMap.find(gamepadProperties[i].guidInstance);
		if (match == _entryMap.end())
		{
			// This is a new entry!
			addGamepadEntry(gamepadProperties[i]);
			saveData = true;
		}
	}
	if (saveData)
	{
		saveGamepadMappings();
		refreshEntryMap();
	}

}


// ---------------------------------------------------------------------------------------------------------
INT32 loadGamepadMappings() {
	TCHAR* szFileName = _T("config/gamepads.dat");

	FILE* fp = _tfopen(szFileName, _T("r"));
	if (fp) {
		clearGamepadMappingData();

		// LOAD + CHECK HEADER:
		CHAR header[7];
		ReadData(fp, header, 6);
		header[6] = 0;	// Null terminate!
		UINT16 version = ReadUint16(fp);

		if (strcmp(header, "GP-MAP") != 0 || version != 1){
			throw "INVALID HEADER DATA!";
		}
		
		UINT16 entryCount = ReadUint16(fp);
		gamepadFile.entryCount = entryCount;
		for (size_t i = 0; i < entryCount; i++)
		{
			GamepadFileEntry& e = gamepadFile.entries[i];
			e.info.guidInstance = ReadData_G<GUID>(fp);
			ReadTCHAR(fp, e.info.Alias, MAX_ALIAS_CHARS);

			UINT16 profileCount = ReadUint16(fp);
			if (profileCount != 1) { throw "INVALID PROFILE COUNT!"; }

			// Read all of the profiles (in a loop (future))
			GamepadInputProfile& p = e.profile; //[j];
			ReadTCHAR(fp, p.driverName, MAX_NAME);
			ReadTCHAR(fp, p.profileName, MAX_NAME);

			ReadButtonData(fp, p);
		}

	}
	else
	{
		// File doesn't exist?
		// We will set some default data.  Any new gamepads will be added during the merge step!
		clearGamepadMappingData();
	}

	refreshEntryMap();

	return 0;
}

// ---------------------------------------------------------------------------------------------------------
INT32 getGamepadInfos(GamepadFileEntry** ppPadInfos, INT32* nPadCount)
{
	// This is where we will load our mapping information.
	// loadGamepadMappings();

	// NOTE: We only want data for plugged controllers, NOT all of them!
	for (int i = 0; i < gamepadCount; i++)
	{
		GUID& guid = gamepadProperties[i].guidInstance;
		auto match = _entryMap.find(guid);
		if (match == _entryMap.end())
		{
			// This is wrong!  Figure out what the normal error handler is?
			throw "FAIL!";
		}

		// Set the pointer.
		ppPadInfos[i] = match->second;
	}
	*nPadCount = gamepadCount;

	return 0;
}

// ---------------------------------------------------------------------------------------------------------
int init()
{
	hDinpWnd = hScrnWnd;

	exit();

	memset(&keyboardProperties, 0, sizeof(keyboardProperties));
	memset(&gamepadProperties, 0, sizeof(gamepadProperties));
	memset(&mouseProperties, 0, sizeof(mouseProperties));

	if (FAILED(DirectInput8Create(hAppInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&pDI, NULL))) {
		return 1;
	}

	// keyboard
	if (FAILED(pDI->CreateDevice(GUID_SysKeyboard, &keyboardProperties[0].lpdid, NULL))) {
		return 1;
	}
	keyboardProperties[0].lpdid->SetDataFormat(&c_dfDIKeyboard);
	keyboardProperties[0].lpdid->SetCooperativeLevel(hDinpWnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
	keyboardProperties[0].lpdid->Acquire();
	keyboardCount = 1;
	keyboardCooperativeModeForeground = false;

	// Enumerate and set up the mice connected to the system
	// Note that under Win2K/WinXP only one mouse device will be enumerated
	if (FAILED(pDI->EnumDevices(DI8DEVTYPE_MOUSE, mouseEnumCallback, /*(void*)this*/pDI, DIEDFL_ATTACHEDONLY))) {
		return 1;
	}

	// Enumerate and set up the gamepads connected to the system
	if (FAILED(pDI->EnumDevices(DI8DEVCLASS_GAMECTRL, gamepadEnumCallback, /*(void*)this*/pDI, DIEDFL_ATTACHEDONLY))) {
		return 1;
	}

	// TEMP: We will compose and write our gamepad data to disk....
	loadGamepadMappings();
	mergeGamepadMappings();
	// saveGamepadMappings();

	// NOTE: This might be a good place to save data about our input devices.
	// Tracking the guids + allowing player association in the UI would be useful I think... something like that.....

	return 0;
}

// Call before checking for Input in a frame
int newFrame()
{
	// No gamepads have been read for this frame
	for (int i = 0; i < gamepadCount; i++) {
		gamepadProperties[i].readStatus = 0;
	}

	// mouse not read this frame
	for (int i = 0; i < mouseCount; i++) {
		mouseProperties[i].readStatus = 0;
	}

	// keyboard not read this frame
	for (int i = 0; i < keyboardCount; i++) {
		keyboardProperties[i].readStatus = 0;
	}

	return 0;
}

// Read the keyboard
int readKeyboard(keyboardData* keyboard)
{
	if (keyboard->readStatus) {			// Already read this frame - ready to go
		return 0;
	}
	if (!keyboard->lpdid) {
		return 1;
	}

	// Read keyboard, reacquiring if neccesary
	HRESULT ret = DI_OK;
	for (int j = 0; j < 2; j++) {
		ret = keyboard->lpdid->GetDeviceState(sizeof(keyboard->state), (void*)keyboard->state);
		if (ret != DIERR_INPUTLOST && ret != DIERR_NOTACQUIRED) {
			break;
		}
		keyboard->lpdid->Acquire();		// If DIERR_INPUTLOST, reacquire and try again
	}
	if (FAILED(ret)) {					// Failed to read the keyboard
		return 1;
	}

	// The keyboard has been successfully Read this frame
	keyboard->readStatus = 1;

	return 0;
}

// Read one of the gamepads
int readGamepad(gamepadData* gamepad)
{
	if (gamepad->readStatus == 1) {		// Already read this frame - ready to go
		return 0;
	}
	if (gamepad->readStatus == 2) {		// Error reading gamepad
		return 1;
	}
	if (!gamepad->lpdid) {
		return 1;
	}

	gamepad->readStatus = 2;			// Mark gamepad as trying to read

	// Poll + read gamepad, reacquiring if neccesary
	HRESULT ret = DI_OK;
	for (int j = 0; j < 2; j++) {
		gamepad->lpdid->Poll();
		ret = gamepad->lpdid->GetDeviceState(sizeof(gamepad->dijs), &gamepad->dijs);
		if (ret != DIERR_INPUTLOST && ret != DIERR_NOTACQUIRED) {
			break;
		}
		gamepad->lpdid->Acquire();		// If DIERR_INPUTLOST, reacquire and try again
	}
	if (FAILED(ret)) {
		return 1;
	}

	// This gamepad had been successfully Read this frame
	gamepad->readStatus = 1;

	return 0;
}

// Check a subcode (the 40xx bit in 4001, 4102 etc) for a gamepad input code
int gamepadState(gamepadData* gamepad, unsigned int subCode)
{
	if (subCode < 0x10) {										// gamepad directions
		// Some drivers don't assign unused axes a value of 0
		if (!(gamepad->dwAxisType[subCode >> 1] & DIDFT_AXIS)) {
			return 0;
		}

		const int DEADZONE = 0x4000;

		switch (subCode) {
		case 0x00: return gamepad->dijs.lX < -DEADZONE;		// Left
		case 0x01: return gamepad->dijs.lX > DEADZONE;		// Right
		case 0x02: return gamepad->dijs.lY < -DEADZONE;		// Up
		case 0x03: return gamepad->dijs.lY > DEADZONE;		// Down
		case 0x04: return gamepad->dijs.lZ < -DEADZONE;
		case 0x05: return gamepad->dijs.lZ > DEADZONE;
		case 0x06: return gamepad->dijs.lRx < -DEADZONE;
		case 0x07: return gamepad->dijs.lRx > DEADZONE;
		case 0x08: return gamepad->dijs.lRy < -DEADZONE;
		case 0x09: return gamepad->dijs.lRy > DEADZONE;
		case 0x0A: return gamepad->dijs.lRz < -DEADZONE;
		case 0x0B: return gamepad->dijs.lRz > DEADZONE;
		case 0x0C: return gamepad->dijs.rglSlider[0] < -DEADZONE;
		case 0x0D: return gamepad->dijs.rglSlider[0] > DEADZONE;
		case 0x0E: return gamepad->dijs.rglSlider[1] < -DEADZONE;
		case 0x0F: return gamepad->dijs.rglSlider[1] > DEADZONE;
		}
	}

	if (subCode < 0x10 + (gamepad->dwPOVs << 2)) {		// POV hat controls
		DWORD hatDirection = gamepad->dijs.rgdwPOV[(subCode & 0x0F) >> 2];
		if ((LOWORD(hatDirection) != 0xFFFF)) {
			switch (subCode & 3) {
			case 0:									// Left
				return (hatDirection >= 22500 && hatDirection <= 31500);
			case 1:									// Right
				return (hatDirection >= 4500 && hatDirection <= 13500);
			case 2:									// Up
				return (hatDirection >= 31500 || hatDirection <= 4500);
			case 3:									// Down
				return (hatDirection >= 13500 && hatDirection <= 22500);
			}
		}
		return 0;
	}
	if (subCode < 0x80) {								// Undefined
		return 0;
	}
	if (subCode < 0x80 + gamepad->dwButtons) {			// gamepad buttons
		return (gamepad->dijs.rgbButtons[subCode & 0x7F] & 0x80) ? 1 : 0;
	}

	return 0;
}

// Read the mouse
int readMouse(mouseData* mouse)
{
	if (mouse->readStatus) {			// Already read this frame - ready to go
		return 0;
	}
	if (!mouse->lpdid) {
		return 1;
	}

	// Read mouse, reacquiring if neccesary
	HRESULT ret = DI_OK;
	for (int j = 0; j < 2; j++) {
		ret = mouse->lpdid->GetDeviceState(sizeof(mouse->dims), (void*)&mouse->dims);
		if (ret != DIERR_INPUTLOST) {
			break;
		}
		mouse->lpdid->Acquire();		// If DIERR_INPUTLOST, reacquire and try again
	}
	if (FAILED(ret)) {					// Failed to read the mouse
		return 1;
	}

	// The mouse has been successfully Read this frame
	mouse->readStatus = 1;

	return 0;
}

// Check a subcode (the 80xx bit in 8001, 8102 etc) for a mouse input code
int checkMouseState(mouseData* mouse, unsigned int subCode)
{
	if (subCode < 0x80) {							// Undefined
		return 0;
	}
	if (subCode < 0x80 + mouse->dwButtons) {		// mouse buttons
		return (mouse->dims.rgbButtons[subCode & 0x7F] & 0x80) ? 1 : 0;
	}

	return 0;
}

// Get the state (pressed = 1, not pressed = 0) of a particular input code
int getState(int code)
{
	if (code < 0) {
		return 0;
	}

	if (code < 256) {
		if (readKeyboard(&keyboardProperties[0])) {		// Check keyboard has been read - return not pressed on error
			return 0;
		}
		return (keyboardProperties[0].state[code] & 0x80) ? 1 : 0;
	}

	if (code < 0x4000) {
		return 0;
	}

	// Codes 4000-8000 = Joysticks
	if (code < 0x8000) {
		int i = (code - 0x4000) >> 8;
		if (i >= gamepadCount) {					// This gamepad number isn't connected
			return 0;
		}
		if (readGamepad(&gamepadProperties[i])) {	// Error polling the gamepad
			return 0;
		}

		// Find the gamepad state in our array
		return gamepadState(&gamepadProperties[i], code & 0xFF);
	}

	// Codes 8000-C000 = mouse
	if (code < 0xC000) {
		int i = (code - 0x8000) >> 8;
		if (i >= mouseCount) {						// This mouse number isn't connected
			return 0;
		}
		if (readMouse(&mouseProperties[i])) {		// Error Reading the mouse
			return 0;
		}
		return checkMouseState(&mouseProperties[i], code & 0xFF);
	}

	return 0;
}

// Read one gamepad axis
int readGamepadAxis(int i, int axis)
{
	gamepadData* gamepad = &gamepadProperties[i];

	if (i < 0 || i >= gamepadCount) {		// This gamepad isn't connected
		return 0;
	}
	if (readGamepad(gamepad)) {				// Error polling the gamepad
		return 0;
	}

	// Some drivers don't assign unused axes a value of 0
	if (!(gamepad->dwAxisType[axis & (MAX_JOYAXIS - 1)] & DIDFT_AXIS)) {
		return 0;
	}

	switch (axis) {
	case 0:
		return gamepad->dijs.lX;
	case 1:
		return gamepad->dijs.lY;
	case 2:
		return gamepad->dijs.lZ;
	case 3:
		return gamepad->dijs.lRx;
	case 4:
		return gamepad->dijs.lRy;
	case 5:
		return gamepad->dijs.lRz;
	case 6:
		return gamepad->dijs.rglSlider[0];
	case 7:
		return gamepad->dijs.rglSlider[1];
	}

	return 0;
}

// Read one mouse axis
int readMouseAxis(int i, int axis)
{
	mouseData* mouse = &mouseProperties[i];

	if (i < 0 || i >= mouseCount) {		// This mouse isn't connected
		return 0;
	}
	if (readMouse(mouse) != 0) {		// Error polling the mouse
		return 0;
	}

	// Some drivers don't assign unused axes a value of 0
	if (!(mouse->dwAxisType[axis & (MAX_MOUSEAXIS - 1)] & DIDFT_AXIS)) {
		return 0;
	}

	switch (axis) {
	case 0:
		return mouse->dims.lX;
	case 1:
		return mouse->dims.lY;
	case 2:
		return mouse->dims.lZ / WHEEL_DELTA;
	}

	return 0;
}

// This function finds which key is pressed
// To ensure analog gamepad axes are handled correctly, call with createBaseline = true the 1st time
int find(bool createBaseline)
{
	int retVal = -1;									// assume nothing pressed

	// check if any keyboard keys are pressed
	if (!readKeyboard(&keyboardProperties[0])) {
		for (int i = 0; i < 256; i++) {
			if (keyboardProperties[0].state[i] & 0x80) {
				retVal = i;
				goto End;
			}
		}
	}

	// Now check all the connected gamepads
	for (int i = 0; i < gamepadCount; i++) {
		gamepadData* gamepad = &gamepadProperties[i];
		if (readGamepad(gamepad)) {						// There was an error polling the gamepad
			continue;
		}

		for (unsigned int j = 0; j < 0x10; j++) {								// Axes
			int delta = gamepad->dwAxisBaseline[j >> 1] - readGamepadAxis(i, (j >> 1));
			if (delta < -0x4000 || delta > 0x4000) {
				if (gamepadState(gamepad, j)) {
					retVal = 0x4000 | (i << 8) | j;
					goto End;
				}
			}
		}

		for (unsigned int j = 0x10; j < 0x10 + (gamepad->dwPOVs << 2); j++) {	// POV hats
			if (gamepadState(gamepad, j)) {
				retVal = 0x4000 | (i << 8) | j;
				goto End;
			}
		}

		for (unsigned int j = 0x80; j < 0x80 + gamepad->dwButtons; j++) {		// Buttons
			if (gamepadState(gamepad, j)) {
				retVal = 0x4000 | (i << 8) | j;
				goto End;
			}
		}
	}

	// Now check all the connected mice
	for (int i = 0; i < mouseCount; i++) {
		mouseData* mouse = &mouseProperties[i];
		if (readMouse(mouse)) {
			continue;
		}

		for (unsigned int j = 0x80; j < 0x80 + mouse->dwButtons; j++) {
			if (checkMouseState(mouse, j)) {
				retVal = 0x8000 | (i << 8) | j;
				goto End;
			}
		}

		int maxDelta = 0, maxAxis = 0;
		for (unsigned int j = 0; j < MAX_MOUSEAXIS; j++) {
			int delta = readMouseAxis(i, j);
			if (abs(maxDelta) < abs(delta)) {
				maxDelta = delta;
				maxAxis = j;
			}
		}
		if (maxDelta < 0) {
			return 0x8000 | (i << 8) | (maxAxis << 1) | 0;
		}
		if (maxDelta > 0) {
			return 0x8000 | (i << 8) | (maxAxis << 1) | 1;
		}
	}

End:

	if (createBaseline) {
		for (int i = 0; i < gamepadCount; i++) {
			for (int j = 0; j < MAX_JOYAXIS; j++) {
				gamepadProperties[i].dwAxisBaseline[j] = readGamepadAxis(i, j);
			}
		}
	}

	return retVal;
}

int getControlName(int code, wchar_t* deviceName, wchar_t* controlName)
{
	IDirectInputDevice8W* lpdid = NULL;
	DWORD* pdwAxisType = NULL;
	DWORD dwMouseAxes = 0, dwPOVs = 0, dwButtons = 0;
	DWORD dwObj = 0;
	int deviceType = code & 0xC000;

	if (deviceName) {
		deviceName[0] = '\0';
	}
	if (controlName) {
		controlName[0] = '\0';
	}

	switch (deviceType) {
	case 0x0000: {
		int i = (code >> 8) & 0x3F;

		if (i >= keyboardCount) {		// This keyboard isn't connected
			return 0;
		}
		if (keyboardProperties[i].lpdid == NULL) {
			return 1;
		}

		if (deviceName) {
			DIDEVICEINSTANCE didi;

			memset(&didi, 0, sizeof(didi));
			didi.dwSize = sizeof(didi);

			keyboardProperties[i].lpdid->GetDeviceInfo(&didi);
			_snwprintf(deviceName, MAX_PATH, L"System keyboard: %s", didi.tszInstanceName);
		}
		if (controlName) {
			DIDEVICEOBJECTINSTANCE didoi;

			memset(&didoi, 0, sizeof(didoi));
			didoi.dwSize = sizeof(didoi);

			if (SUCCEEDED(keyboardProperties[i].lpdid->GetObjectInfo(&didoi, DIDFT_MAKEINSTANCE(code & 0xFF) | DIDFT_PSHBUTTON, DIPH_BYID))) {
				wcsncpy(controlName, didoi.tszName, MAX_PATH);
			}
		}

		return 0;
	}
	case 0x4000: {
		int i = (code >> 8) & 0x3F;

		if (i >= gamepadCount) {		// This gamepad isn't connected
			return 0;
		}
		lpdid = gamepadProperties[i].lpdid;
		pdwAxisType = gamepadProperties[i].dwAxisType;
		dwPOVs = gamepadProperties[i].dwPOVs;
		dwButtons = gamepadProperties[i].dwButtons;
		break;
	}
	case 0x8000: {
		int i = (code >> 8) & 0x3F;

		if (i >= mouseCount) {			// This mouse isn't connected
			return 0;
		}
		lpdid = mouseProperties[i].lpdid;
		pdwAxisType = mouseProperties[i].dwAxisType;
		dwMouseAxes = mouseProperties[i].dwAxes;
		dwButtons = mouseProperties[i].dwButtons;
		break;
	}
	}

	if (lpdid == NULL) {
		return 1;
	}

	if (deviceName) {
		DIDEVICEINSTANCE didi;

		memset(&didi, 0, sizeof(didi));
		didi.dwSize = sizeof(didi);

		if (SUCCEEDED(lpdid->GetDeviceInfo(&didi))) {

			// Special treatment for the system mouse
			if ((code & 0xFF00) == 0x8000) {
				_snwprintf(deviceName, MAX_PATH, L"System mouse: %s", didi.tszInstanceName);
			}
			else {
				wcsncpy(deviceName, didi.tszInstanceName, MAX_PATH);
			}
		}
	}
	if (controlName) {
		DIDEVICEOBJECTINSTANCE didoi;

		unsigned int subCode = code & 0xFF;

		if ((subCode >> 1) < ((deviceType == 0x4000) ? MAX_JOYAXIS : MAX_MOUSEAXIS)) {
			if (pdwAxisType[subCode >> 1] & DIDFT_AXIS) {
				dwObj = pdwAxisType[subCode >> 1];
			}
		}
		if (subCode >= 0x10 && subCode < 0x10 + (dwPOVs << 2)) {
			dwObj = DIDFT_POV | DIDFT_MAKEINSTANCE((subCode & 0x0F) >> 2);
		}
		if (subCode >= 0x80 && subCode < 0x80 + dwButtons) {
			dwObj = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE((subCode & 0x7F) + dwMouseAxes);
		}

		memset(&didoi, 0, sizeof(didoi));
		didoi.dwSize = sizeof(didoi);

		if (SUCCEEDED(lpdid->GetObjectInfo(&didoi, dwObj, DIPH_BYID))) {
			wcsncpy(controlName, didoi.tszName, MAX_PATH);
		}
	}

	return 0;
}

// Callback that evaluates and sets up each gamepad axis
static BOOL CALLBACK gamepadEnumAxesCallback(LPCDIDEVICEOBJECTINSTANCE instance, LPVOID /*p*/)
{
	//	return ((InputDI*)p)->gamepadEnumObject(instance);
	return gamepadEnumObject(instance);
}

// Callback that evaluates and sets up each mouse axis
static BOOL CALLBACK mouseEnumAxesCallback(LPCDIDEVICEOBJECTINSTANCE instance, LPVOID /*p*/)
{
	//	return ((InputDI*)p)->mouseEnumObject(instance);
	return mouseEnumObject(instance);
}

// Callback that evaluates each gamepad DirectInput device
static BOOL CALLBACK gamepadEnumCallback(LPCDIDEVICEINSTANCE instance, LPVOID /*p*/)
{
	//	return ((InputDI*)p)->gamepadEnumDevice(instance);
	return gamepadEnumDevice(instance);
}

// Callback that evaluates each mouse DirectInput device
static BOOL CALLBACK mouseEnumCallback(LPCDIDEVICEINSTANCE instance, LPVOID /*p*/)
{
	//	return ((InputDI*)p)->mouseEnumDevice(instance);
	return mouseEnumDevice(instance);
}

struct InputInOut InputInOutDInput = { init, exit, setCooperativeLevel, newFrame, getState, readGamepadAxis, readMouseAxis, find, getControlName, NULL, getGamepadInfos, _T("DirectInput8 input") };
