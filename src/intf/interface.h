#pragma once
#ifdef FBNEO_DEBUG
#define PRINT_DEBUG_INFO
#endif
#include <vector>

// GameInp structure
#include "gameinp.h"
// Key codes
#include "inp_keys.h"

#define MAX_ALIAS_CHARS 32

// Interface info (used for all modules)
struct InterfaceInfo {
	const TCHAR* pszModuleName;
	TCHAR** ppszInterfaceSettings;
	TCHAR** ppszModuleSettings;
};

INT32 IntInfoFree(InterfaceInfo* pInfo);
INT32 IntInfoInit(InterfaceInfo* pInfo);
INT32 IntInfoAddStringInterface(InterfaceInfo* pInfo, TCHAR* szString);
INT32 IntInfoAddStringModule(InterfaceInfo* pInfo, TCHAR* szString);

struct GamepadInfo
{
	GUID guidInstance;
	GUID guidProduct;
	TCHAR Alias[MAX_ALIAS_CHARS];
};

// Gamepad listing info.....
// Legacy type for keeping track of older input mappings files.
struct GamepadInput {
	EInputType type;			// Type of input.  Switch, analog, etc.
	UINT16 nCode;				// Code for input.  NOTE: These are translated codes.  Internally the system will add extra bits to identify the gamepad index.
	//UINT8 nAxis;
	//UINT8 nRange;
	// See gami.cpp:1386 to see where gamepad index and code are tranlated.
};

#define BURNER_BUTTON	0x80
#define BURNER_DPAD		0x10
#define BURNER_ANALOG	0x00

// Gamepad listing info.....
// New type that we are using to properly abstract a gamepad for use in the emulator.
// NOTE: I fully intend to do a full rewrite of the input system as the current version
// is super janky IMO.
struct GamepadInputEx {
	EInputType type;			// Type of input.  Button, stick, etc.
	UINT16 index;				// Code for input.  NOTE: These are translated codes.  Internally the system will add extra bits to identify the gamepad index.

	// -------------------------------------------------------------
	// Return an input code (nCode) that is compatible with the emulator.
	// NOTE: 'burner/burn' is a term used often as fbNeo = 'final burn neo'
	UINT32 GetBurnerCode() const {
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

		default:
			throw std::exception("NOT SUPPORTTED!");
		}
	}

};

// These are guesses....
#define MAX_NAME 32
#define MAX_INPUTS 16
struct GamepadInputProfile {
	TCHAR driverName[MAX_NAME];			// PLACEHOLDER: The game that this profile belongs to.
	TCHAR profileName[MAX_NAME];		// PLACEHOLDER: The name of the profile.  In theory, there could be many.

	GamepadInput inputs[MAX_INPUTS];
	UINT16 inputCount;
	bool useAutoDirections;				// PLACEHOLDER: Auto map the directional inputs?
};

// These are guesses....
#define MAX_NAME 32
#define MAX_INPUTS 16

// Describes a set of inputs (player, system, etc.) for a game.
// For all reasonable purposes, the set for p1/p2/px should be the same.
struct GameInputSet {
	GamepadInputDesc Inputs[MAX_INPUTS];
	UINT16 InputCount;
	//wchar_t* Description;		// 'player', 'system', etc. NOTE: This can be used later when we get deeper into the input system... 
};

// NOTE: This is different than 'GameInputSet' as the inputs have specific types + indexes that are later
// used by the input system to poll devices and so on.
struct GamepadInputProfileEx {
	GamepadInputEx inputs[MAX_INPUTS];
	UINT16 inputCount;
};

// There can be many entries per file, each of which would map to a game, and any number of input profiles
// from there.
struct GamepadFileEntry {
	GamepadInfo info;

	// NOTE: We just assume a single profile for now.  NBD.
	UINT16 profileCount = 1;
	GamepadInputProfile profile;
};

// Stores data about a single input profile.
// This maps a name to some buttons.
struct InputProfileEntry {
	TCHAR Name[MAX_ALIAS_CHARS];
	GamepadInput Inputs[MAX_INPUTS];
};

// Other Input Related Defines:
#define MAX_GAMEPAD_BUTTONS	(16)

// TODO: These defines would be right at home in the rest of the code!
// They describe address ranges for input codes!
// Let's make an effort to remove magic values!
#define JOYSTICK_LOWER 0x4000
#define JOYSTICK_UPPER 0x8000
#define MOUSE_LOWER JOYSTICK_UPPER
#define MOUSE_UPPER 0xC000

#define MAX_DIRS 4

#define DELTA_UP -1
#define DELTA_DOWN 1

#define DELTA_LEFT -1
#define DELTA_RIGHT 1

#define DELTA_NONE 0
#define Y_AXIS_CODE 1
#define X_AXIS_CODE 0

//#define DIR_NONE -1
#define DIR_UP 0
#define DIR_DOWN 1
#define DIR_LEFT 2
#define DIR_RIGHT 3

// NOTE: The above defs come from an enum def in gami.cpp
//enum {
//	UP,
//	DOWN,
//	LEFT,
//	RIGHT,
//	COUNT
//};


// Input plugin:
struct InputInOut {
	INT32(*Init)();
	INT32(*Exit)();
	INT32(*SetCooperativeLevel)(bool bExclusive, bool bForeground);
	// Setup new frame
	INT32(*NewFrame)();
	// Read digital
	INT32(*ReadSwitch)(INT32 nCode);
	// Read analog
	INT32(*ReadJoyAxis)(INT32 i, INT32 nAxis);
	INT32(*ReadMouseAxis)(INT32 i, INT32 nAxis);
	// Find out which control is activated
	INT32(*Find)(bool CreateBaseline);
	// Get the name of a control
	INT32(*GetControlName)(INT32 nCode, TCHAR* pszDeviceName, TCHAR* pszControlName);
	// Get plugin info
	INT32(*GetPluginSettings)(InterfaceInfo* pInfo);

	// Get Gamepads...
	INT32(*GetGamepadList)(GamepadFileEntry** ppPadInfos, UINT32* nPadCount);
	INT32(*GetGamepadState)(int padIndex, UINT16* dirStates, UINT16* btnStates, DWORD* btnCount);

	// We detected that a gamepad was added to the system.
	INT32(*OnGamepadAdded)(bool isGamepad);

	// We detected that a gamepad was removed from the system...
	INT32(*OnGamepadRemoved)(bool isGamepad);

	// Save the current set of mapping data!
	INT32(*SaveGamepadMappings)();

	// Get Input profiles...
	INT32(*GetProfileList)(InputProfileEntry** ppProfiles, UINT32* nProfileCount);

	// Save the current set of input profiles!
	INT32(*SaveInputProfiles)();
	INT32(*AddInputProfile)(const TCHAR* name);
	INT32(*RemoveInputProfile)(size_t index);

	const TCHAR* szModuleName;
};

INT32 LoadGamepadDatabase();		// defined in gami.cpp

INT32 InputInit();
INT32 InputExit();
INT32 InputSetCooperativeLevel(const bool bExclusive, const bool bForeGround);
INT32 InputMake(bool bCopy);
INT32 InputFind(const INT32 nFlags);
INT32 InputGetControlName(INT32 nCode, TCHAR* pszDeviceName, TCHAR* pszControlName);
InterfaceInfo* InputGetInfo();
std::vector<const InputInOut*> InputGetInterfaces();
INT32 InputSaveGamepadMappings();

INT32 InputGetGamepads(GamepadFileEntry** ppPadInfos, UINT32* nPadCount);
INT32 InputGetGamepadState(int padIndex, UINT16* dirStates, UINT16* btnStates, DWORD* btnCount);

INT32 InputOnInputDeviceAdded(bool isGamepad);
INT32 InputOnInputDeviceRemoved(bool isGamepad);

INT32 InputGetProfiles(InputProfileEntry** ppProfiles, UINT32* nProfileCount);
INT32 InputSaveProfiles();
INT32 InputAddInputProfile(const TCHAR* name);
INT32 InputRemoveProfile(size_t index);

extern bool bInputOkay;
extern UINT32 nInputSelect;

// CD emulation module

struct CDEmuDo {
	INT32(*CDEmuExit)();
	INT32(*CDEmuInit)();
	INT32(*CDEmuStop)();
	INT32(*CDEmuPlay)(UINT8 M, UINT8 S, UINT8 F);
	INT32(*CDEmuLoadSector)(INT32 LBA, char* pBuffer);
	UINT8* (*CDEmuReadTOC)(INT32 track);
	UINT8* (*CDEmuReadQChannel)();
	INT32(*CDEmuGetSoundBuffer)(INT16* buffer, INT32 samples);
	INT32(*CDEmuScan)(INT32 nAction, INT32* pnMin);
	// Get plugin info
	INT32(*GetPluginSettings)(InterfaceInfo* pInfo);
	const TCHAR* szModuleName;
};

#include "cd_interface.h"

InterfaceInfo* CDEmuGetInfo();

extern bool bCDEmuOkay;
extern UINT32 nCDEmuSelect;

extern CDEmuStatusValue CDEmuStatus;

// Profiling plugin
struct ProfileDo {
	INT32(*ProfileExit)();
	INT32(*ProfileInit)();
	INT32(*ProfileStart)(INT32 nSubSystem);
	INT32(*ProfileEnd)(INT32 nSubSystem);
	double (*ProfileReadLast)(INT32 nSubSystem);
	double (*ProfileReadAverage)(INT32 nSubSystem);
	// Get plugin info
	INT32(*GetPluginSettings)(InterfaceInfo* pInfo);
	const  TCHAR* szModuleName;
};

extern bool bProfileOkay;
extern UINT32 nProfileSelect;

INT32 ProfileInit();
INT32 ProfileExit();
INT32 ProfileProfileStart(INT32 nSubSystem);
INT32 ProfileProfileEnd(INT32 nSubSustem);
double ProfileProfileReadLast(INT32 nSubSustem);
double ProfileProfileReadAverage(INT32 nSubSustem);
InterfaceInfo* ProfileGetInfo();

// Audio Output plugin
struct AudOut {
	INT32(*BlankSound)();
	INT32(*SoundInit)();
	INT32(*SoundExit)();
	INT32(*SoundCheck)();
	INT32(*SoundFrame)();
	INT32(*SoundPlay)();
	INT32(*SoundStop)();
	INT32(*SoundSetVolume)();
	// Get plugin info
	INT32(*GetPluginSettings)(InterfaceInfo* pInfo);
	const TCHAR* szModuleName;
};

INT32 AudSelect(UINT32 nPlugIn);
INT32 AudBlankSound();
INT32 AudSoundInit();
INT32 AudSoundExit();
INT32 AudSoundCheck();
INT32 AudSoundFrame();
INT32 AudSoundPlay();
INT32 AudSoundStop();
INT32 AudSoundSetVolume();
INT32 AudSoundGetSampleRate();
INT32 AudSoundGetSegLen();
InterfaceInfo* AudGetInfo();
void AudWriteSilence();

extern INT32 nAudSampleRate[8];     // sample rate
extern INT32 nAudVolume;			// Sound volume (% * 100)
extern INT32 nAudSegCount[8];       // Segs in the pdsbLoop buffer
extern INT32 nAudSegLen;            // Seg length in samples (calculated from Rate/Fps)
extern INT32 nAudExclusive;			// Exclusive mode
extern INT32 nAudAllocSegLen;
extern INT16* nAudNextSound;       	// The next sound seg we will add to the sample loop
extern UINT8 bAudOkay;    			// True if DSound was initted okay
extern UINT8 bAudPlaying;			// True if the Loop buffer is playing
extern INT32 nAudDSPModule[8];		// DSP module to use: 0 = none, 1 = low-pass filter
extern UINT32 nAudSelect;

// Video Output plugin:
struct VidOut {
	INT32(*Init)();
	INT32(*Exit)();
	INT32(*Frame)(bool bRedraw);
	INT32(*Paint)(INT32 bValidate);
	INT32(*ImageSize)(RECT* pRect, INT32 nGameWidth, INT32 nGameHeight);
	// Get plugin info
	INT32(*GetPluginSettings)(InterfaceInfo* pInfo);
	const TCHAR* szModuleName;
};

INT32 VidSelect(UINT32 nPlugin);
INT32 VidInit();
INT32 VidExit();
INT32 VidReInitialise();
INT32 VidFrame();
extern void (*pVidTransCallback)(void);
INT32 VidRedraw();
INT32 VidRecalcPal();
INT32 VidPaint(INT32 bValidate);
INT32 VidImageSize(RECT* pRect, INT32 nGameWidth, INT32 nGameHeight);
const TCHAR* VidGetModuleName();
InterfaceInfo* VidGetInfo();

#ifdef BUILD_WIN32
extern HWND hVidWnd;
#endif

#if defined (_XBOX)
extern HWND hVidWnd;
#endif

extern bool bVidOkay;
extern UINT32 nVidSelect;
extern INT32 nVidWidth, nVidHeight, nVidDepth, nVidRefresh;

extern INT32 nVidHorWidth, nVidHorHeight;
extern INT32 nVidVerWidth, nVidVerHeight;

extern INT32 nVidFullscreen;
extern INT32 bVidBilinear;
extern INT32 bVidScanlines;
extern INT32 bVidScanRotate;
extern INT32 bVidScanBilinear;
extern INT32 nVidScanIntensity;
extern INT32 bVidScanHalf;
extern INT32 bVidScanDelay;
extern INT32 nVidFeedbackIntensity;
extern INT32 nVidFeedbackOverSaturation;
extern INT32 bVidCorrectAspect;
extern INT32 bVidArcaderes;

extern INT32 bVidArcaderesHor;
extern INT32 bVidArcaderesVer;

extern INT32 nVidRotationAdjust;
extern INT32 bVidUseHardwareGamma;
extern INT32 bVidAutoSwitchFull;
extern INT32 bVidAutoSwitchFullDisable;
extern INT32 bVidForce16bit;
extern INT32 bVidForce16bitDx9Alt;
extern INT32 bVidForceFlip;
extern INT32 nVidTransferMethod;
extern float fVidScreenAngle;
extern float fVidScreenCurvature;
extern INT64 nVidBlitterOpt[];
extern INT32 bVidFullStretch;
extern INT32 bVidTripleBuffer;
extern INT32 bVidVSync;
extern double dVidCubicB;
extern double dVidCubicC;
extern INT32 bVidDX9Bilinear;
extern INT32 bVidHardwareVertex;
extern INT32 bVidMotionBlur;
extern INT32 bVidDX9Scanlines;
extern INT32 bVidDX9WinFullscreen;
extern INT32 bVidDX9LegacyRenderer;
extern INT32 nVidDX9HardFX;
extern INT32 bVidOverlay;
extern INT32 bVidBigOverlay;
extern INT32 bVidShowInputs;
extern INT32 bVidUnrankedScores;
extern INT32 bVidSaveOverlayFiles;
extern INT32 bVidSaveChatHistory;
extern INT32 bVidMuteChat;
extern INT32 nVidRunahead;
extern wchar_t HorScreen[32];
extern wchar_t VerScreen[32];
extern INT32 nVidScrnWidth, nVidScrnHeight;
extern INT32 nVidScrnDepth;

extern INT32 nVidScrnAspectX, nVidScrnAspectY;
extern INT32 nVidVerScrnAspectX, nVidVerScrnAspectY;

extern UINT8* pVidImage;
extern INT32 nVidImageWidth, nVidImageHeight;
extern INT32 nVidImageLeft, nVidImageTop;
extern INT32 nVidImagePitch, nVidImageBPP;
extern INT32 nVidImageDepth;

extern "C" UINT32(__cdecl* VidHighCol) (INT32 r, INT32 g, INT32 b, INT32 i);

extern TCHAR szPlaceHolder[MAX_PATH];

// vid_directx_support.cpp

INT32 VidSNewTinyMsg(const TCHAR* pText, INT32 nRGB = 0, INT32 nDuration = 0, INT32 nPriority = 5);
INT32 VidSNewJoystickMsg(const TCHAR* pText, INT32 nRGB = 0, INT32 nDuration = 0, INT32 nLineNo = 0);
INT32 VidSNewShortMsg(const TCHAR* pText, INT32 nRGB = 0, INT32 nDuration = 0, INT32 nPriority = 5);
void VidSKillShortMsg();
void VidSKillTinyMsg();

INT32 VidSUpdate();
INT32 VidSSetGameInfo(const TCHAR* p1, const TCHAR* p2, INT32 spectator, INT32 ranked, INT32 player);
INT32 VidSSetGameScores(INT32 score1, INT32 score2);
INT32 VidSSetGameSpectators(INT32 num);
INT32 VidSSetSystemMessage(TCHAR* status);
INT32 VidSSetStats(double fps, INT32 ping, INT32 delay);
INT32 VidSShowStats(INT32 show);
INT32 VidSAddChatLine(const TCHAR* pID, INT32 nIDRGB, const TCHAR* pMain, INT32 nMainRGB);

#define MAX_CHAT_SIZE (128)

extern INT32 nVidSDisplayStatus;
extern INT32 nMaxChatFontSize;
extern INT32 nMinChatFontSize;
extern bool bEditActive;
extern bool bEditTextChanged;
extern TCHAR EditText[MAX_CHAT_SIZE + 1];

// osd text display for dx9
extern TCHAR OSDMsg[MAX_PATH];
extern UINT32 nOSDTimer;
void VidSKillOSDMsg();

// overlay for dx9Alt
#include "vid_overlay.h"



// These map onto the 'BurnInputInfo' lists that are used to populate the input dialog.
// For example, see cps3InputList[] in d_cps3.cpp:32
struct playerInputs {
	UINT16 p1Index;			// Starting index of the player1 buttons.
	UINT16 p2Index;			// Starting index of the player2 buttons.

	//GamepadInput buttons[MAX_INPUTS];
	UINT16 buttonCount;				// Total number of mapped buttons.
	UINT16 maxPlayers;				// Max number of players for the game.
};
