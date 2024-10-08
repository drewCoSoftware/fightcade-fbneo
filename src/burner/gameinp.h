#ifndef GAMEINP_H
#define GAMEINP_H

#include <exception>

struct giConstant {
	UINT8 nConst;				// The constant value
};

struct giSwitch {
	UINT16 nCode;				// The input code (for digital)
};

struct giJoyAxis {
	UINT8 nJoy;					// The joystick number
	UINT8 nAxis;	   			// The joystick axis number
};

struct giMouseAxis {
	UINT8 nMouse;				// The mouse number
	UINT8 nAxis;				// The axis number
	UINT16 nOffset;				// Used for absolute axes
};

struct giSliderAxis {
	UINT16 nSlider[2];			// Keys to use for slider
};

struct giSlider {
	union {
		struct giJoyAxis JoyAxis;
		struct giSliderAxis SliderAxis;
	};
	INT16 nSliderSpeed;					// speed with which keys move the slider
	INT16 nSliderCenter;				// Speed the slider should center itself (high value = slow)
	INT32 nSliderValue;					// Current position of the slider
};

struct giInput {
	union {								// Destination for the Input Value
		UINT8* pVal;
		UINT16* pShortVal;
	};
	UINT16 nVal;				// The Input Value

	union {
		struct giConstant Constant;
		struct giSwitch Switch;
		struct giJoyAxis JoyAxis;
		struct giMouseAxis MouseAxis;
		struct giSlider Slider;
	};
};

// @@AAR: I don't think this is actually used anywhere....
struct giForce {
	UINT8 nInput;				// The input to apply force feedback effects to
	UINT8 nEffect;				// The effect to use		// @@AAR: Not used, as far as I can tell.....
};

// OBSOLETE:  Macro system is janky, so we will find a way to repeal and replace it.
// With defineable CGamPCInputs code we can certainly come up with a better system.
struct giMacro {
	UINT8 nMode;				// 0 = Unused, 1 = used

	UINT8* pVal[4];				// Destination for the Input Value
	UINT8 nVal[4];				// The Input Value
	UINT8 nInput[4];			// Which inputs are mapped

	struct giSwitch Switch;

	char szName[33];			// Maximum name length 16 chars
	UINT8 nSysMacro;			// mappable system macro (1) or Auto-Fire (15)
};

static const UINT16 BURNER_BUTTON = 0x80;
static const UINT16 BURNER_DPAD = 0x10;
static const UINT16 BURNER_ANALOG = 0x00;


// TODO: This should be an enum.

#define GIT_UNDEFINED		(0x00)
#define GIT_CONSTANT		(0x01)
#define GIT_SWITCH			(0x02)

#define GIT_GROUP_SLIDER	(0x08)
#define GIT_KEYSLIDER		(0x08)
#define GIT_JOYSLIDER		(0x09)

#define GIT_GROUP_MOUSE		(0x10)
#define GIT_MOUSEAXIS		(0x10)

#define GIT_GROUP_JOYSTICK	(0x20)
#define GIT_JOYAXIS_FULL	(0x20)
#define GIT_JOYAXIS_NEG		(0x21)
#define GIT_JOYAXIS_POS		(0x22)

#define GIT_FORCE			(0x40)

#define GIT_GROUP_MACRO		(0x80)
#define GIT_MACRO_AUTO		(0x80)
#define GIT_MACRO_CUSTOM	(0x81)



// Maybe we just fix this here.....
// So attach an index that matches up to the BurnInputInfos.....
// --> Not really possible b/c of the way that the drivers are defined, and then all of the dumbshit C++ macros that they use
// --> to 'organize' everything....

// So... if we have a fixed number of game inputs (probably unavoidable at this point, but also makes sense) then we need to
// have more than one nInput available per game (nType) input.

struct GameInp {
	UINT8 nInput;				// PC side: see above  --> This is the TYPE of pc input!  Nothing else!
	UINT8 nType;				// game side: see burn.h (BIT_* defs) (BurnInputInfo)

	union {
		struct giInput Input;
		struct giForce Force;
		struct giMacro Macro;
	};
};

enum EInputDeviceType {
	ITYPE_UNSET = 0,

	ITYPE_GAMEPAD_BUTTON,				// on/off
	ITYPE_DPAD,

	// NOTE: These inputs technically make sense, but they should be expanded so that they are more compatible with FBNEO.
	ITYPE_FULL_ANALOG,		// The full +/- minus range.  Something like a stick. (NOTE: We might need to infer / expand in indexes to make sure that this works....)
	ITYPE_HALF_ANALOG,		// Half axis range, + or -.  Something like a trigger.

	// A keyboard!
	ITYPE_KEYBOARD,
	ITYPE_CONSTANT
};


enum ECardinalDir {
	DIR_NONE,
	DIR_UP,
	DIR_DOWN,
	DIR_LEFT,
	DIR_RIGHT
};

//const static UINT32 GAMEPAD_BUTTON_MASK = GAMEPAD_MASK | 0x300;
//const static UINT32 GAMEPAD_ANALOG_MASK = 0x1000;

const static UINT32 GAMEPAD_MASK = 0x1000;
const static UINT32 GAMEPAD_ANALOG_MASK = GAMEPAD_MASK | 0x100;
const static UINT32 GAMEPAD_DPAD_MASK = GAMEPAD_MASK | 0x200;
const static UINT32 GAMEPAD_BUTTONS_MASK = GAMEPAD_MASK | 0x300;

const static UINT32 KEYBOARD_MASK = 0x2000;
const static UINT32 MOUSE_MASK = 0x4000;
const static UINT32 CONSTANT_MASK = 0x5000;

// =============================================================================================
// Standardized names/labels for PC inputs.
// NOTE: I am doing some weird stuff with masks / values.  This is just so that I can take advantage
// of indexes, etc. that are already defined in FBneo vs. having to create a lookup table to translate
// between the two.
// NOTE: Some other kind of struct / def can be set up so that we can assign all of the names to these inputs,
// like that of 'KeyNames' defined in gami.cpp.
enum PCInputs {
	PCINPUT_NOT_SET = 0x00,
	PCINPUT_UNSUPPORTED = 0x01,
	PCINPUT_CONSTANT = 0x02,			// An input with a constant value.

	// This value is meant to be used as a mask.
	PCINPUT_GAMEPAD = GAMEPAD_MASK,

	// Analog stick directions.
	// NOTE: We do a bit of extra translation for these when we parse out the SDL database.
	PCINPUT_PAD_ANALOG = GAMEPAD_ANALOG_MASK,

	PCINPUT_LSTICK_LEFT,
	PCINPUT_LSTICK_RIGHT,
	PCINPUT_LSTICK_UP,
	PCINPUT_LSTICK_DOWN,

	// TODO: Figure out the indexes for this....
	PCINPUT_RSTICK_LEFT,
	PCINPUT_RSTICK_RIGHT,
	PCINPUT_RSTICK_UP,
	PCINPUT_RSTICK_DOWN,

	PCINPUT_PAD_DPAD = GAMEPAD_DPAD_MASK,
	PCINPUT_DPAD_LEFT,
	PCINPUT_DPAD_RIGHT,
	PCINPUT_DPAD_UP,
	PCINPUT_DPAD_DOWN,



	PCINPUT_PAD_BUTTONS = GAMEPAD_BUTTONS_MASK,
	PCINPUT_A,				// PS - cross / x	: SWITCH - B
	PCINPUT_B,				// PS - circle		: SWITCH - A
	PCINPUT_X,				// PS - square		: SWITCH - Y
	PCINPUT_Y,				// PS - triangle	: SWITCH - X
	PCINPUT_LEFT_TRIGGER,
	PCINPUT_RIGHT_TRIGGER,
	PCINPUT_BACK,
	PCINPUT_START,

	// L/R stick 'click' buttons.
	PCINPUT_LSTICK_BUTTON,
	PCINPUT_RSTICK_BUTTON,

	// Analog triggers....
	PCINPUT_LEFT_BUMPER,
	PCINPUT_RIGHT_BUMPER,

	PCINPUT_TOUCHPAD,       // This is some kind of PS specific thing.

	PCINPUT_GUIDE,          // This is the main x-box button.
	PCINPUT_MISC_BUTTON,	// Just some random, extra button....

	// This value is meant to be masked with the FBK_* defs in inp_keys.h
	// Their values should match the FBK_* version.
	PCINPUT_KEYB = KEYBOARD_MASK,

	PCINPUT_KEYB_ESCAPE,
	PCINPUT_KEYB_1,
	PCINPUT_KEYB_2,
	PCINPUT_KEYB_3,
	PCINPUT_KEYB_4,
	PCINPUT_KEYB_5,
	PCINPUT_KEYB_6,
	PCINPUT_KEYB_7,
	PCINPUT_KEYB_8,
	PCINPUT_KEYB_9,
	PCINPUT_KEYB_0,
	PCINPUT_KEYB_MINUS,
	PCINPUT_KEYB_EQUALS,
	PCINPUT_KEYB_BACK,
	PCINPUT_KEYB_TAB,
	PCINPUT_KEYB_Q,
	PCINPUT_KEYB_W,
	PCINPUT_KEYB_E,
	PCINPUT_KEYB_R,
	PCINPUT_KEYB_T,
	PCINPUT_KEYB_Y,
	PCINPUT_KEYB_U,
	PCINPUT_KEYB_I,
	PCINPUT_KEYB_O,
	PCINPUT_KEYB_P,
	PCINPUT_KEYB_LBRACKET,
	PCINPUT_KEYB_RBRACKET,
	PCINPUT_KEYB_RETURN,
	PCINPUT_KEYB_LCONTROL,
	PCINPUT_KEYB_A,
	PCINPUT_KEYB_S,
	PCINPUT_KEYB_D,
	PCINPUT_KEYB_F,
	PCINPUT_KEYB_G,
	PCINPUT_KEYB_H,
	PCINPUT_KEYB_J,
	PCINPUT_KEYB_K,
	PCINPUT_KEYB_L,
	PCINPUT_KEYB_SEMICOLON,
	PCINPUT_KEYB_APOSTROPH,
	PCINPUT_KEYB_GRAVE,
	PCINPUT_KEYB_LSHIFT,
	PCINPUT_KEYB_BACKSLASH,
	PCINPUT_KEYB_Z,
	PCINPUT_KEYB_X,
	PCINPUT_KEYB_C,
	PCINPUT_KEYB_V,
	PCINPUT_KEYB_B,
	PCINPUT_KEYB_N,
	PCINPUT_KEYB_M,
	PCINPUT_KEYB_COMMA,
	PCINPUT_KEYB_PERIOD,
	PCINPUT_KEYB_SLASH,
	PCINPUT_KEYB_RSHIFT,
	PCINPUT_KEYB_MULTIPLY,
	PCINPUT_KEYB_LALT,
	PCINPUT_KEYB_SPACE,
	PCINPUT_KEYB_CAPITAL,
	PCINPUT_KEYB_F1,
	PCINPUT_KEYB_F2,
	PCINPUT_KEYB_F3,
	PCINPUT_KEYB_F4,
	PCINPUT_KEYB_F5,
	PCINPUT_KEYB_F6,
	PCINPUT_KEYB_F7,
	PCINPUT_KEYB_F8,
	PCINPUT_KEYB_F9,
	PCINPUT_KEYB_F10,
	PCINPUT_KEYB_NUMLOCK,
	PCINPUT_KEYB_SCROLL,
	PCINPUT_KEYB_NUMPAD7,
	PCINPUT_KEYB_NUMPAD8,
	PCINPUT_KEYB_NUMPAD9,
	PCINPUT_KEYB_SUBTRACT,
	PCINPUT_KEYB_NUMPAD4,
	PCINPUT_KEYB_NUMPAD5,
	PCINPUT_KEYB_NUMPAD6,
	PCINPUT_KEYB_ADD,
	PCINPUT_KEYB_NUMPAD1,
	PCINPUT_KEYB_NUMPAD2,
	PCINPUT_KEYB_NUMPAD3,
	PCINPUT_KEYB_NUMPAD0,
	PCINPUT_KEYB_DECIMAL,
	PCINPUT_KEYB_OEM_102,
	PCINPUT_KEYB_F11,
	PCINPUT_KEYB_F12,

	PCINPUT_F13 = PCINPUT_KEYB | 0x64,
	PCINPUT_F14 = PCINPUT_KEYB | 0x65,
	PCINPUT_F15 = PCINPUT_KEYB | 0x66,

	PCINPUT_KANA = PCINPUT_KEYB | 0x70,
	PCINPUT_ABNT_C1 = PCINPUT_KEYB | 0x73,
	PCINPUT_CONVERT = PCINPUT_KEYB | 0x79,
	PCINPUT_NOCONVERT = PCINPUT_KEYB | 0x7B,
	PCINPUT_YEN = PCINPUT_KEYB | 0x7D,
	PCINPUT_ABNT_C2 = PCINPUT_KEYB | 0x7E,
	PCINPUT_NUMPADEQUALS = PCINPUT_KEYB | 0x8D,
	PCINPUT_PREVTRACK = PCINPUT_KEYB | 0x90,
	PCINPUT_AT = PCINPUT_KEYB | 0x91,
	PCINPUT_COLON = PCINPUT_KEYB | 0x92,
	PCINPUT_UNDERLINE = PCINPUT_KEYB | 0x93,
	PCINPUT_KANJI = PCINPUT_KEYB | 0x94,
	PCINPUT_STOP = PCINPUT_KEYB | 0x95,
	PCINPUT_AX = PCINPUT_KEYB | 0x96,
	PCINPUT_UNLABELED = PCINPUT_KEYB | 0x97,
	PCINPUT_NEXTTRACK = PCINPUT_KEYB | 0x99,
	PCINPUT_NUMPADENTER = PCINPUT_KEYB | 0x9C,
	PCINPUT_RCONTROL = PCINPUT_KEYB | 0x9D,
	PCINPUT_MUTE = PCINPUT_KEYB | 0xA0,
	PCINPUT_CALCULATOR = PCINPUT_KEYB | 0xA1,
	PCINPUT_PLAYPAUSE = PCINPUT_KEYB | 0xA2,
	PCINPUT_MEDIASTOP = PCINPUT_KEYB | 0xA4,
	PCINPUT_VOLUMEDOWN = PCINPUT_KEYB | 0xAE,
	PCINPUT_VOLUMEUP = PCINPUT_KEYB | 0xB0,
	PCINPUT_WEBHOME = PCINPUT_KEYB | 0xB2,
	PCINPUT_NUMPADCOMMA = PCINPUT_KEYB | 0xB3,
	PCINPUT_DIVIDE = PCINPUT_KEYB | 0xB5,
	PCINPUT_SYSRQ = PCINPUT_KEYB | 0xB7,
	PCINPUT_RALT = PCINPUT_KEYB | 0xB8,
	PCINPUT_PAUSE = PCINPUT_KEYB | 0xC5,
	PCINPUT_HOME = PCINPUT_KEYB | 0xC7,
	PCINPUT_UPARROW = PCINPUT_KEYB | 0xC8,
	PCINPUT_PRIOR = PCINPUT_KEYB | 0xC9,
	PCINPUT_LEFTARROW = PCINPUT_KEYB | 0xCB,
	PCINPUT_RIGHTARROW = PCINPUT_KEYB | 0xCD,
	PCINPUT_END = PCINPUT_KEYB | 0xCF,
	PCINPUT_DOWNARROW = PCINPUT_KEYB | 0xD0,
	PCINPUT_NEXT = PCINPUT_KEYB | 0xD1,
	PCINPUT_INSERT = PCINPUT_KEYB | 0xD2,
	PCINPUT_DELETE = PCINPUT_KEYB | 0xD3,
	PCINPUT_LWIN = PCINPUT_KEYB | 0xDB,
	PCINPUT_RWIN = PCINPUT_KEYB | 0xDC,
	PCINPUT_APPS = PCINPUT_KEYB | 0xDD,
	PCINPUT_POWER = PCINPUT_KEYB | 0xDE,
	PCINPUT_SLEEP = PCINPUT_KEYB | 0xDF,
	PCINPUT_WAKE = PCINPUT_KEYB | 0xE3,
	PCINPUT_WEBSEARCH = PCINPUT_KEYB | 0xE5,
	PCINPUT_WEBFAVORITES = PCINPUT_KEYB | 0xE6,
	PCINPUT_WEBREFRESH = PCINPUT_KEYB | 0xE7,
	PCINPUT_WEBSTOP = PCINPUT_KEYB | 0xE8,
	PCINPUT_WEBFORWARD = PCINPUT_KEYB | 0xE9,
	PCINPUT_WEBBACK = PCINPUT_KEYB | 0xEA,
	PCINPUT_MYCOMPUTER = PCINPUT_KEYB | 0xEB,
	PCINPUT_MAIL = PCINPUT_KEYB | 0xEC,
	PCINPUT_MEDIASELECT = PCINPUT_KEYB | 0xED,

	// Mouse inputs.
	PCINPUT_MOUSE = MOUSE_MASK,

	// TODO: Add mouse inputs, later.

	// Special inputs, that have extra / special meaning?
	// Maybe only used when we read in the gamepad database?
	PCINPUT_XTRA = 0x8000,

	// NOTE: These are kind of special...  they describe two values (+/-) for a single axis.
	// Also, this is more of a thing that we use when reading the pad database file....
	PCINPUT_LSTICK_X,         // x-axis (+/-)
	PCINPUT_LSTICK_Y,         // y-axis (+/-)

	// Analog stick directions.
	PCINPUT_RSTICK_X,         // x-axis (+/-)
	PCINPUT_RSTICK_Y,         // y-axis (+/-)


};


// REFACTOR: This can describe other inputs, not just gamepads.
// This type is used as a way to describe a unique button,etc. and how it maps onto the driver input.
struct GamepadInputDesc {
	PCInputs Input;				// The type of input.  This is really just a unique identifier for the input....
	UINT32 DriverInputIndex;		// The corresponding input index (defined in the driver).  NOTE: DriverInputIndex can be defined more than once to allow for analog stick + dpad inputs for directions.
};

#endif
