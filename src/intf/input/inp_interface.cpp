// Burner Input module
#include "burner.h"
#include <vector>

UINT32 nInputSelect = 0;
bool bInputOkay = false;			// Inidcates that the input system has been initialized.

static bool bEnableKeyboardInputs;			// Are we OK to process inputs?

#if defined (BUILD_WIN32)
extern struct InputInOut InputInOutDInput;
#elif defined (BUILD_MACOS)
extern struct InputInOut InputInOutMacOS;
#elif defined (BUILD_SDL)
extern struct InputInOut InputInOutSDL;
#elif defined (BUILD_SDL2)
extern struct InputInOut InputInOutSDL2;
#elif defined (_XBOX)
extern struct InputInOut InputInOutXInput2;
#elif defined (BUILD_QT)
extern struct InputInOut InputInOutQt;
#endif

// Why is this an array?
// TODO: Make this not an array.
static struct InputInOut* pInputInOut[] =
{
#if defined (BUILD_WIN32)
	&InputInOutDInput,
#elif defined (BUILD_MACOS)
	&InputInOutMacOS,
#elif defined (BUILD_SDL2)
		&InputInOutSDL2,
#elif defined (BUILD_SDL)
	&InputInOutSDL,
#elif defined (_XBOX)
	&InputInOutXInput2,
#elif defined (BUILD_QT)
	&InputInOutQt,
#endif
};

#define INPUT_LEN (sizeof(pInputInOut) / sizeof(pInputInOut[0]))
#define INPUT_MERGE(x) (x != 0 ? x : pgi->Input.nVal * nCurrentFrameMerge)

static InterfaceInfo InpInfo = { NULL, NULL, NULL };
static INT32 nCurrentFrameMerge = 0;
static INT32 nCurrentFrameInput = 0;

// --------------------------------------------------------------------------------------------------------------
std::vector<const InputInOut*> InputGetInterfaces()
{
	std::vector<const InputInOut*> list;
	for (unsigned int i = 0; i < INPUT_LEN; i++)
		list.push_back(pInputInOut[i]);
	return list;
}


inline INT32 CinpState(const INT32 nCode)
{
	// Return off for keyboard inputs if current input is turned off
	if (nCode < 0x4000 && !bEnableKeyboardInputs) {
		return 0;
	}

	// Read from Direct Input
	return pInputInOut[nInputSelect]->ReadSwitch(nCode);
}

// Read an axis of a joystick
inline INT32 CinpJoyAxis(const INT32 i, const INT32 nAxis)
{
	// Read from Direct Input
	return pInputInOut[nInputSelect]->ReadJoyAxis(i, nAxis);
}

// Read an axis of a mouse
inline INT32 CinpMouseAxis(const INT32 i, const INT32 nAxis)
{
	// Read from Direct Input
	return pInputInOut[nInputSelect]->ReadMouseAxis(i, nAxis);
}

// ----------------------------------------------------------------------------------------------------------
// Do one frames worth of input sliders.
// 'Sliders' are a way to emulate analog inputs (like a roller ball) with digital (i.e. keyboard) inputs.
static INT32 ProcessSliders()
{
	struct GameInp* pgi;
	UINT32 i;

	for (i = 0, pgi = GameInp; i < nGameInpCount; i++, pgi++) {
		INT32 nAdd = 0;
		INT32 bGotKey = 0;

		if ((pgi->nInput & GIT_GROUP_SLIDER) == 0) {				// not a slider
			continue;
		}

		if (pgi->nInput == GIT_KEYSLIDER) {
			// Get states of the two keys
			if (CinpState(pgi->Input.Slider.SliderAxis.nSlider[0])) {
				bGotKey = 1;
				nAdd -= 0x100;
			}
			if (CinpState(pgi->Input.Slider.SliderAxis.nSlider[1])) {
				bGotKey = 1;
				nAdd += 0x100;
			}
		}

		if (pgi->nInput == GIT_JOYSLIDER) {
			// Get state of the axis
			nAdd = CinpJoyAxis(pgi->Input.Slider.JoyAxis.nJoy, pgi->Input.Slider.JoyAxis.nAxis);

			if (nAdd != 0) { bGotKey = 1; }

			nAdd /= 0x80;
			// May 30, 2019 -dink
			// Was "nAdd /= 0x100;" - Current gamepads w/ thumbsticks
			// register 0x3f <- 0x80  -> 0xbe, so we must account for that
			// to be able to get the same range as GIT_KEYSLIDER.
		}

		// nAdd is now -0x100 to +0x100

		// Change to slider speed
		nAdd *= pgi->Input.Slider.nSliderSpeed;
		nAdd /= 0x100;

		if (pgi->Input.Slider.nSliderCenter && !bGotKey) {						// Attact to center
			if (pgi->Input.Slider.nSliderCenter == 1) {
				// Fastest Auto-Center speed, center immediately when key/button is released
				pgi->Input.Slider.nSliderValue = 0x8000;
			}
			else {
				INT32 v = pgi->Input.Slider.nSliderValue - 0x8000;
				v *= (pgi->Input.Slider.nSliderCenter - 1);
				v /= pgi->Input.Slider.nSliderCenter;
				v += 0x8000;
				pgi->Input.Slider.nSliderValue = v;
			}
		}

		pgi->Input.Slider.nSliderValue += nAdd;
		// Limit slider
		if (pgi->Input.Slider.nSliderValue < 0x0000) {
			pgi->Input.Slider.nSliderValue = 0x0000;
		}
		if (pgi->Input.Slider.nSliderValue > 0xFFFF) {
			pgi->Input.Slider.nSliderValue = 0xFFFF;
		}
	}
	return 0;
}

// ------------------------------------------------------------------------------------------------------------------------
INT32 InputGetProfiles(InputProfileEntry** ppProfiles, UINT32* nProfileCount) {
	INT32 nRet = 0;

	// Fail on index mismatch.
	if (nInputSelect >= INPUT_LEN) {
		return 1;
	}

	nRet = pInputInOut[nInputSelect]->GetProfileList(ppProfiles, nProfileCount);
	return nRet;
}

// ------------------------------------------------------------------------------------------------------------------------
INT32 InputGetGamepadState(int padIndex, UINT16* dirStates, UINT16* btnStates, DWORD* btnCount) {
	INT32 res = pInputInOut[nInputSelect]->GetGamepadState(padIndex, dirStates, btnStates, btnCount);
	return res;
}

// ------------------------------------------------------------------------------------------------------------------------
INT32 InputOnInputDeviceAdded(bool isGamepad) {
	INT32 res = pInputInOut[nInputSelect]->OnGamepadAdded(isGamepad);
	return res;
}

// ------------------------------------------------------------------------------------------------------------------------
INT32 InputOnInputDeviceRemoved(bool isGamepad) {
	INT32 res = pInputInOut[nInputSelect]->OnGamepadRemoved(isGamepad);
	return res;
}


// ------------------------------------------------------------------------------------------------------------------------
INT32 InputGetGamepads(GamepadFileEntry** ppPadInfos, UINT32* nPadCount) {
	INT32 nRet = 0;

	// Fail on index mismatch.
	if (nInputSelect >= INPUT_LEN) {
		return 1;
	}

	nRet = pInputInOut[nInputSelect]->GetGamepadList(ppPadInfos, nPadCount);
	return nRet;
}

// ------------------------------------------------------------------------------------------------------------------------
INT32 InputRemoveProfile(size_t index) {
	INT32 nRet = pInputInOut[nInputSelect]->RemoveInputProfile(index);
	return nRet;
}

// ------------------------------------------------------------------------------------------------------------------------
INT32 InputAddInputProfile(const TCHAR* name) {
	INT32 nRet = pInputInOut[nInputSelect]->AddInputProfile(name);
	return nRet;
}

// ------------------------------------------------------------------------------------------------------------------------
INT32 InputSaveProfiles() {
	INT32 nRet = 0;

	// Fail on index mismatch.
	if (nInputSelect >= INPUT_LEN) {
		return 1;
	}

	nRet = pInputInOut[nInputSelect]->SaveInputProfiles();
	return nRet;
}

// ------------------------------------------------------------------------------------------------------------------------
INT32 InputSaveGamepadMappings() {
	// We don't save mappings per-pad, but we might care about aliasing again....
	throw std::exception("NOT SUPPORTED!");
	//INT32 nRet = 0;

	//// Fail on index mismatch.
	//if (nInputSelect >= INPUT_LEN) {
	//	return 1;
	//}

	//nRet = pInputInOut[nInputSelect]->SaveGamepadMappings();
	//return nRet;
}

// ------------------------------------------------------------------------------------------------------------------------
INT32 InputInit() {
	INT32 nRet;

	LoadGamepadDatabase();

	bInputOkay = false;

	if (nInputSelect >= INPUT_LEN) {
		return 1;
	}

	if ((nRet = pInputInOut[nInputSelect]->Init()) == 0) {
		bInputOkay = true;
	}

	return nRet;
}

// ------------------------------------------------------------------------------------------------------------------------
INT32 InputExit()
{
	IntInfoFree(&InpInfo);

	if (nInputSelect >= INPUT_LEN) {
		return 1;
	}

	bInputOkay = false;

	return pInputInOut[nInputSelect]->Exit();
}

// This will request exclusive access for mouse and/or request input processing if the application is in the foreground only
//  - bExcusive = 1   - Request exclusive access to inputs (this may apply to only the mouse or all input, depending on the API)
//                      This function will show or hide the mouse cursor as appropriate
//  - bForeground = 1 - Request input processing only if application is in foreground (this may not be supported by all APIs)
INT32 InputSetCooperativeLevel(const bool bExclusive, const bool bForeground)
{
	if (!bInputOkay || nInputSelect >= INPUT_LEN) {
		return 1;
	}
	if (pInputInOut[nInputSelect]->SetCooperativeLevel == NULL) {
		return 0;
	}

	return pInputInOut[nInputSelect]->SetCooperativeLevel(bExclusive, bForeground);
}

static bool bLastAF[1000];
INT32 nAutoFireRate = 12;

// --------------------------------------------------------------------------------------------------------------
static inline INT32 AutofirePick() {
	return ((nCurrentFrame % nAutoFireRate) > nAutoFireRate - 4);
}


// --------------------------------------------------------------------------------------------------------------
// This will process all PC-side inputs and optionally update the emulated game side.
INT32 InputMake(bool bCopy)
{
	nCurrentFrameMerge = nCurrentFrameInput == nCurrentFrame ? 1 : 0;
	nCurrentFrameInput = nCurrentFrame;

	struct GameInp* pgi;
	UINT32 i;

	if (!bInputOkay || nInputSelect >= INPUT_LEN) {
		return 1;
	}


	// Clear existing input states.
	pInputInOut[nInputSelect]->NewFrame();
	bEnableKeyboardInputs = !IsEditActive();

	ProcessSliders();

	// NOTE: We can actually set this up when we first create the GameInp (or whatever) data.
	// For now, we will hang out....
	std::map<UINT32, UINT32> driverIndexToValue;


	// This is where we will run the inputs for GameInputSet, and copy them over to
	// 'GameInp'.  The most important thing here is that we have some way to run
	// / merge our multi-inputs and map them to the correct pgi index.
	for (size_t gIndex = 0; gIndex < GameInputSet.GroupCount; gIndex++)
	{
		CGameInputGroup& g = GameInputSet.Groups[gIndex];

		for (size_t i = 0; i < g.InputCount; i++)
		{
			auto& input = g.Inputs[i];
			UINT32 code = input.BurnerCode;




			// Whatever the last detected input was, we will assign it.
			pgi = (GameInp + input.DriverInputIndex);
			if (pgi->Input.pVal == NULL) {
				continue;
			}

			switch (pgi->nInput) {
			case GIT_UNDEFINED:						// Undefined
				pgi->Input.nVal = 0;
				break;

			case GIT_CONSTANT:						// Constant value
				pgi->Input.nVal = INPUT_MERGE(pgi->Input.Constant.nConst);
				if (bCopy) {
					*(pgi->Input.pVal) = pgi->Input.nVal;
				}
				break;

			case GIT_SWITCH: {						// Digital input

				// Old Way... use the nCode on the input....
				INT32 s = CinpState(code);

				UINT32 useS = s;

				// This is crappy, but it will do for a POC!
				if (useS == 0) {
					auto f = driverIndexToValue.find(input.DriverInputIndex);
					if (f != driverIndexToValue.end()) {
						useS = f->second;
					}
				}
				else {
					driverIndexToValue[input.DriverInputIndex] = useS;
				}
				s = useS;


				// TODO: We need to have a way to detect when things are double mapped and merge the inputs
				// accordingly....  Basically, use the last non-zero value....

				if (pgi->nType & BIT_GROUP_ANALOG) {
					// Set analog controls to full
					if (s) {
						pgi->Input.nVal = INPUT_MERGE(0xFFFF);
					}
					else {
						pgi->Input.nVal = INPUT_MERGE(0x0001);
					}
					if (bCopy) {
						*(pgi->Input.pShortVal) = pgi->Input.nVal;
					}
				}
				else {
					// Binary controls
					if (s) {
						pgi->Input.nVal = INPUT_MERGE(1);
					}
					else {
						pgi->Input.nVal = INPUT_MERGE(0);
					}
					if (bCopy) {
						*(pgi->Input.pVal) = pgi->Input.nVal;
					}
				}

				break;
			}

						   // TODO: Shouldn't the sliders be handled in the 'ProcessSliders' function?
			case GIT_KEYSLIDER:						// Keyboard slider
			case GIT_JOYSLIDER: {					// Joystick slider
				INT32 nSlider = pgi->Input.Slider.nSliderValue;
				if (pgi->nType == BIT_ANALOG_REL) {
					nSlider -= 0x8000;
					nSlider >>= 4;
				}

				nSlider *= nAnalogSpeed;
				nSlider >>= 8;

				// Clip axis to 16 bits (signed)
				if (nSlider < -32768) {
					nSlider = -32768;
				}
				if (nSlider > 32767) {
					nSlider = 32767;
				}

				pgi->Input.nVal = (UINT16)nSlider;
				if (bCopy) {
					*(pgi->Input.pShortVal) = pgi->Input.nVal;
				}
				break;
			}
			case GIT_MOUSEAXIS: {					// Mouse axis
				INT32 nMouse = CinpMouseAxis(pgi->Input.MouseAxis.nMouse, pgi->Input.MouseAxis.nAxis) * nAnalogSpeed;
				// Clip axis to 16 bits (signed)
				if (nMouse < -32768) {
					nMouse = -32768;
				}
				if (nMouse > 32767) {
					nMouse = 32767;
				}
				pgi->Input.nVal = (UINT16)nMouse;
				if (bCopy) {
					*(pgi->Input.pShortVal) = pgi->Input.nVal;
				}
				break;
			}
			case GIT_JOYAXIS_FULL: {				// Joystick axis
				INT32 nJoy = CinpJoyAxis(pgi->Input.JoyAxis.nJoy, pgi->Input.JoyAxis.nAxis);

				if (pgi->nType == BIT_ANALOG_REL) {
					nJoy *= nAnalogSpeed;
					nJoy >>= 13;

					// Clip axis to 16 bits (signed)
					if (nJoy < -32768) {
						nJoy = -32768;
					}
					if (nJoy > 32767) {
						nJoy = 32767;
					}
				}
				else {
					nJoy >>= 1;
					nJoy += 0x8000;

					// Clip axis to 16 bits
					if (nJoy < 0x0001) {
						nJoy = 0x0001;
					}
					if (nJoy > 0xFFFF) {
						nJoy = 0xFFFF;
					}
				}

				pgi->Input.nVal = (UINT16)nJoy;
				if (bCopy) {
					*(pgi->Input.pShortVal) = pgi->Input.nVal;
				}
				break;
			}
			case GIT_JOYAXIS_NEG: {				// Joystick axis Lo
				INT32 nJoy = CinpJoyAxis(pgi->Input.JoyAxis.nJoy, pgi->Input.JoyAxis.nAxis);
				if (nJoy < 32767) {
					nJoy = -nJoy;

					if (nJoy < 0x0000) {
						nJoy = 0x0000;
					}
					if (nJoy > 0xFFFF) {
						nJoy = 0xFFFF;
					}

					pgi->Input.nVal = INPUT_MERGE((UINT16)nJoy);
				}
				else {
					pgi->Input.nVal = INPUT_MERGE(0);
				}

				if (bCopy) {
					*(pgi->Input.pShortVal) = pgi->Input.nVal;
				}
				break;
			}
			case GIT_JOYAXIS_POS: {				// Joystick axis Hi
				INT32 nJoy = CinpJoyAxis(pgi->Input.JoyAxis.nJoy, pgi->Input.JoyAxis.nAxis);
				if (nJoy > 32767) {

					if (nJoy < 0x0000) {
						nJoy = 0x0000;
					}
					if (nJoy > 0xFFFF) {
						nJoy = 0xFFFF;
					}

					pgi->Input.nVal = (UINT16)nJoy;
				}
				else {
					pgi->Input.nVal = 0;
				}

				if (bCopy) {
					*(pgi->Input.pShortVal) = pgi->Input.nVal;
				}
				break;
			}
			}



			int x = 10;
		}
	}



	//// LEGACY:
	//// Use the codes that are assigned to pgi->Inputs.
	//for (i = 0, pgi = GameInp; i < nGameInpCount; i++, pgi++) {
	//	if (pgi->Input.pVal == NULL) {
	//		continue;
	//	}

	//	switch (pgi->nInput) {
	//	case GIT_UNDEFINED:						// Undefined
	//		pgi->Input.nVal = 0;
	//		break;

	//	case GIT_CONSTANT:						// Constant value
	//		pgi->Input.nVal = INPUT_MERGE(pgi->Input.Constant.nConst);
	//		if (bCopy) {
	//			*(pgi->Input.pVal) = pgi->Input.nVal;
	//		}
	//		break;

	//	case GIT_SWITCH: {						// Digital input
	//		INT32 s = CinpState(pgi->Input.Switch.nCode);

	//		if (pgi->nType & BIT_GROUP_ANALOG) {
	//			// Set analog controls to full
	//			if (s) {
	//				pgi->Input.nVal = INPUT_MERGE(0xFFFF);
	//			}
	//			else {
	//				pgi->Input.nVal = INPUT_MERGE(0x0001);
	//			}
	//			if (bCopy) {
	//				*(pgi->Input.pShortVal) = pgi->Input.nVal;
	//			}
	//		}
	//		else {
	//			// Binary controls
	//			if (s) {
	//				pgi->Input.nVal = INPUT_MERGE(1);
	//			}
	//			else {
	//				pgi->Input.nVal = INPUT_MERGE(0);
	//			}
	//			if (bCopy) {
	//				*(pgi->Input.pVal) = pgi->Input.nVal;
	//			}
	//		}

	//		break;
	//	}

	//				   // TODO: Shouldn't the sliders be handled in the 'ProcessSliders' function?
	//	case GIT_KEYSLIDER:						// Keyboard slider
	//	case GIT_JOYSLIDER: {					// Joystick slider
	//		INT32 nSlider = pgi->Input.Slider.nSliderValue;
	//		if (pgi->nType == BIT_ANALOG_REL) {
	//			nSlider -= 0x8000;
	//			nSlider >>= 4;
	//		}

	//		nSlider *= nAnalogSpeed;
	//		nSlider >>= 8;

	//		// Clip axis to 16 bits (signed)
	//		if (nSlider < -32768) {
	//			nSlider = -32768;
	//		}
	//		if (nSlider > 32767) {
	//			nSlider = 32767;
	//		}

	//		pgi->Input.nVal = (UINT16)nSlider;
	//		if (bCopy) {
	//			*(pgi->Input.pShortVal) = pgi->Input.nVal;
	//		}
	//		break;
	//	}
	//	case GIT_MOUSEAXIS: {					// Mouse axis
	//		INT32 nMouse = CinpMouseAxis(pgi->Input.MouseAxis.nMouse, pgi->Input.MouseAxis.nAxis) * nAnalogSpeed;
	//		// Clip axis to 16 bits (signed)
	//		if (nMouse < -32768) {
	//			nMouse = -32768;
	//		}
	//		if (nMouse > 32767) {
	//			nMouse = 32767;
	//		}
	//		pgi->Input.nVal = (UINT16)nMouse;
	//		if (bCopy) {
	//			*(pgi->Input.pShortVal) = pgi->Input.nVal;
	//		}
	//		break;
	//	}
	//	case GIT_JOYAXIS_FULL: {				// Joystick axis
	//		INT32 nJoy = CinpJoyAxis(pgi->Input.JoyAxis.nJoy, pgi->Input.JoyAxis.nAxis);

	//		if (pgi->nType == BIT_ANALOG_REL) {
	//			nJoy *= nAnalogSpeed;
	//			nJoy >>= 13;

	//			// Clip axis to 16 bits (signed)
	//			if (nJoy < -32768) {
	//				nJoy = -32768;
	//			}
	//			if (nJoy > 32767) {
	//				nJoy = 32767;
	//			}
	//		}
	//		else {
	//			nJoy >>= 1;
	//			nJoy += 0x8000;

	//			// Clip axis to 16 bits
	//			if (nJoy < 0x0001) {
	//				nJoy = 0x0001;
	//			}
	//			if (nJoy > 0xFFFF) {
	//				nJoy = 0xFFFF;
	//			}
	//		}

	//		pgi->Input.nVal = (UINT16)nJoy;
	//		if (bCopy) {
	//			*(pgi->Input.pShortVal) = pgi->Input.nVal;
	//		}
	//		break;
	//	}
	//	case GIT_JOYAXIS_NEG: {				// Joystick axis Lo
	//		INT32 nJoy = CinpJoyAxis(pgi->Input.JoyAxis.nJoy, pgi->Input.JoyAxis.nAxis);
	//		if (nJoy < 32767) {
	//			nJoy = -nJoy;

	//			if (nJoy < 0x0000) {
	//				nJoy = 0x0000;
	//			}
	//			if (nJoy > 0xFFFF) {
	//				nJoy = 0xFFFF;
	//			}

	//			pgi->Input.nVal = INPUT_MERGE((UINT16)nJoy);
	//		}
	//		else {
	//			pgi->Input.nVal = INPUT_MERGE(0);
	//		}

	//		if (bCopy) {
	//			*(pgi->Input.pShortVal) = pgi->Input.nVal;
	//		}
	//		break;
	//	}
	//	case GIT_JOYAXIS_POS: {				// Joystick axis Hi
	//		INT32 nJoy = CinpJoyAxis(pgi->Input.JoyAxis.nJoy, pgi->Input.JoyAxis.nAxis);
	//		if (nJoy > 32767) {

	//			if (nJoy < 0x0000) {
	//				nJoy = 0x0000;
	//			}
	//			if (nJoy > 0xFFFF) {
	//				nJoy = 0xFFFF;
	//			}

	//			pgi->Input.nVal = (UINT16)nJoy;
	//		}
	//		else {
	//			pgi->Input.nVal = 0;
	//		}

	//		if (bCopy) {
	//			*(pgi->Input.pShortVal) = pgi->Input.nVal;
	//		}
	//		break;
	//	}
	//	}
	//}

	//for (i = 0; i < nMacroCount; i++, pgi++) {
	//	if (pgi->Macro.nMode == 1 && pgi->Macro.nSysMacro == 0) { // Macro is defined
	//		if (bCopy && CinpState(pgi->Macro.Switch.nCode)) {
	//			for (INT32 j = 0; j < 4; j++) {
	//				if (pgi->Macro.pVal[j]) {
	//					*(pgi->Macro.pVal[j]) = pgi->Macro.nVal[j];
	//				}
	//			}
	//		}
	//	}
	//	if (pgi->Macro.nSysMacro) { // System-Macro is defined -dink
	//		if (CinpState(pgi->Macro.Switch.nCode)) {
	//			if (pgi->Macro.pVal[0]) {
	//				*(pgi->Macro.pVal[0]) = pgi->Macro.nVal[0];
	//				if (pgi->Macro.nSysMacro == 15) { //Auto-Fire mode!
	//					if (AutofirePick() || bLastAF[i] == 0)
	//						*(pgi->Macro.pVal[0]) = pgi->Macro.nVal[0];
	//					else
	//						*(pgi->Macro.pVal[0]) = 0;
	//					bLastAF[i] = 1;
	//				}
	//			}
	//		}
	//		else { // Disable System-Macro when key up
	//			if (pgi->Macro.pVal[0] && pgi->Macro.nSysMacro == 1) {
	//				*(pgi->Macro.pVal[0]) = 0;
	//			}
	//			else {
	//				if (pgi->Macro.nSysMacro == 15)
	//					bLastAF[i] = 0;
	//			}
	//		}
	//	}
	//}

	return 0;
}

// Use this function as follows:
//  - First call with nFlags =  2
//  -  Then call with nFlags =  4 until all controls are released
//  -  Then call with nFlags =  8 until a control is activated
//  -  Then call with nFlags = 16 until all controls are released again
//     It will continue to return a control (reflecting the direction an axis is moved in)
//     Use this to handle analog controls correctly
//
// Call with nFlags & 1 to indicate that controls need to be polled (when normal input processing is disabled)
INT32 InputFind(const INT32 nFlags)
{
	static INT32 nInputCode, nDelay, nJoyPrevPos;

	INT32 nFind;

	if (nInputSelect >= INPUT_LEN) {
		return 1;
	}

	if (nFlags & 1) {
		pInputInOut[nInputSelect]->NewFrame();
	}

	nFind = pInputInOut[nInputSelect]->Find(nFlags & 2);

	switch (nFlags) {
	case  4: {
		return nFind;
	}
	case  8: {
		if (nFind >= 0) {
			nInputCode = nFind;
			if ((nInputCode & 0x4000) && (nInputCode & 0xFF) < 0x10) {
				nJoyPrevPos = CinpJoyAxis((nInputCode >> 8) & 0x3F, (nInputCode >> 1) & 0x07);
			}
			nDelay = 0;
		}

		return nFind;
	}
	case 16: {

		// Treat joystick axes specially
		// Wait until the axis reports no movement for some time
		if ((nInputCode & 0x4000) && (nInputCode & 0xFF) < 0x10) {
			INT32 nJoyPos = CinpJoyAxis((nInputCode >> 8) & 0x3F, (nInputCode >> 1) & 0x07);
			INT32 nJoyDelta = nJoyPrevPos - nJoyPos;

			nJoyPrevPos = nJoyPos;

			if (nFind != -1) {
				nInputCode = nFind;
			}

			// While the movement is within the threshold, treat it as no movement
			if (nJoyDelta > -0x0100 && nJoyDelta < 0x0100) {
				nDelay++;
				if (nDelay > 64) {
					return -1;
				}
			}
			else {
				nDelay = 0;
			}

			return nInputCode;
		}

		// Treat mouse axes specially
		// Wait until the axis reports no movement/movement in the same direction for some time
		if ((nInputCode & 0x8000) && (nInputCode & 0xFF) < 0x06) {
			INT32 nMouseDelta = CinpMouseAxis((nInputCode >> 8) & 0x3F, (nInputCode >> 1) & 0x07);
			if (nFind == -1 || ((nInputCode & 1) ? nMouseDelta > 0 : nMouseDelta < 0)) {
				nDelay++;
				if (nDelay > 128) {
					return -1;
				}
			}
			else {
				nDelay = 0;
				nInputCode = nFind;
			}

			return nInputCode;
		}

		return nFind;
	}
	}

	return -1;
}

// Get the name of a control and/or the device it's on (not all API's may support this)
// Either parameter can be passed as NULL
INT32 InputGetControlName(INT32 nCode, TCHAR* pszDeviceName, TCHAR* pszControlName)
{
	if (!bInputOkay || nInputSelect >= INPUT_LEN) {
		return 1;
	}
	if (pInputInOut[nInputSelect]->GetControlName == NULL) {
		return 1;
	}

	return pInputInOut[nInputSelect]->GetControlName(nCode, pszDeviceName, pszControlName);
}

InterfaceInfo* InputGetInfo()
{
	if (IntInfoInit(&InpInfo)) {
		IntInfoFree(&InpInfo);
		return NULL;
	}

	if (bInputOkay) {
		InpInfo.pszModuleName = pInputInOut[nInputSelect]->szModuleName;

		if (pInputInOut[nInputSelect]->GetPluginSettings) {
			pInputInOut[nInputSelect]->GetPluginSettings(&InpInfo);
		}

		for (INT32 nType = 0; nType < 3; nType++) {
			INT32 nDeviceTypes[] = { 0x0000, 0x8000, 0x4000 };
			TCHAR nDeviceTypeNames[][16] = { _T("keyboard"), _T("mouse   "), _T("joystick") };
			TCHAR nDeviceName[MAX_PATH] = _T("");
			INT32 nActiveDevice = 0;

			while (nActiveDevice < 16 && pInputInOut[nInputSelect]->GetControlName(nDeviceTypes[nType] | (nActiveDevice << 8), nDeviceName, NULL) == 0 && nDeviceName[0]) {
				TCHAR szString[MAX_PATH] = _T("");

				_sntprintf(szString, MAX_PATH, _T("%s %d %s"), nDeviceTypeNames[nType], nActiveDevice, nDeviceName);

				if (IntInfoAddStringInterface(&InpInfo, szString)) {
					break;
				}

				nActiveDevice++;
			}
		}
	}
	else {
		IntInfoAddStringInterface(&InpInfo, _T("Input plugin not initialised"));
	}

	return &InpInfo;
}
