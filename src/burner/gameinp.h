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
	PCI_NOT_SET = 0x00,
	PCI_UNSUPPORTED = 0x01,
	PCI_CONSTANT = 0x02,			// An input with a constant value.

	// This value is meant to be used as a mask.
	PCI_GAMEPAD = GAMEPAD_MASK,

	// Analog stick directions.
	// NOTE: We do a bit of extra translation for these when we parse out the SDL database.
	PCI_PAD_ANALOG = GAMEPAD_ANALOG_MASK,

	PIC_PAD_LSTICK_LEFT,
	PIC_PAD_LSTICK_RIGHT,
	PIC_PAD_LSTICK_UP,
	PIC_PAD_LSTICK_DOWN,

	// TODO: Figure out the indexes for this....
	PIC_PAD_RSTICK_LEFT,
	PIC_PAD_RSTICK_RIGHT,
	PIC_PAD_RSTICK_UP,
	PIC_PAD_RSTICK_DOWN,

	PCI_PAD_DPAD = GAMEPAD_DPAD_MASK,
	PIC_PAD_DPAD_LEFT,
	PIC_PAD_DPAD_RIGHT,
	PIC_PAD_DPAD_UP,
	PIC_PAD_DPAD_DOWN,



	PCI_PAD_BUTTONS = GAMEPAD_BUTTONS_MASK,
	PCI_PAD_A,				// PS - cross / x	: SWITCH - B
	PCI_PAD_B,				// PS - circle		: SWITCH - A
	PCI_PAD_X,				// PS - square		: SWITCH - Y
	PCI_PAD_Y,				// PS - triangle	: SWITCH - X
	PCI_PAD_LEFT_TRIGGER,
	PCI_PAD_RIGHT_TRIGGER,
	PCI_PAD_BACK,
	PCI_PAD_START,

	// L/R stick 'click' buttons.
	PIC_PAD_LSTICK_BUTTON,
	PIC_PAD_RSTICK_BUTTON,

	// Analog triggers....
	PCI_PAD_LEFT_BUMPER,
	PCI_PAD_RIGHT_BUMPER,

	PCI_PAD_TOUCHPAD,       // This is some kind of PS specific thing.

	PCI_PAD_GUIDE,          // This is the main x-box button.
	PCI_PAD_MISC_BUTTON,	// Just some random, extra button....

	// This value is meant to be masked with the FBK_* defs in inp_keys.h
	// Their values should match the FBK_* version.
	PCI_KEYB = KEYBOARD_MASK,

	PCI_KEYB_ESCAPE,
	PCI_KEYB_1,
	PCI_KEYB_2,
	PCI_KEYB_3,
	PCI_KEYB_4,
	PCI_KEYB_5,
	PCI_KEYB_6,
	PCI_KEYB_7,
	PCI_KEYB_8,
	PCI_KEYB_9,
	PCI_KEYB_0,
	PCI_KEYB_MINUS,
	PCI_KEYB_EQUALS,
	PCI_KEYB_BACK,
	PCI_KEYB_TAB,
	PCI_KEYB_Q,
	PCI_KEYB_W,
	PCI_KEYB_E,
	PCI_KEYB_R,
	PCI_KEYB_T,
	PCI_KEYB_Y,
	PCI_KEYB_U,
	PCI_KEYB_I,
	PCI_KEYB_O,
	PCI_KEYB_P,
	PCI_KEYB_LBRACKET,
	PCI_KEYB_RBRACKET,
	PCI_KEYB_RETURN,
	PCI_KEYB_LCONTROL,
	PCI_KEYB_A,
	PCI_KEYB_S,
	PCI_KEYB_D,
	PCI_KEYB_F,
	PCI_KEYB_G,
	PCI_KEYB_H,
	PCI_KEYB_J,
	PCI_KEYB_K,
	PCI_KEYB_L,
	PCI_KEYB_SEMICOLON,
	PCI_KEYB_APOSTROPH,
	PCI_KEYB_GRAVE,
	PCI_KEYB_LSHIFT,
	PCI_KEYB_BACKSLASH,
	PCI_KEYB_Z,
	PCI_KEYB_X,
	PCI_KEYB_C,
	PCI_KEYB_V,
	PCI_KEYB_B,
	PCI_KEYB_N,
	PCI_KEYB_M,
	PCI_KEYB_COMMA,
	PCI_KEYB_PERIOD,
	PCI_KEYB_SLASH,
	PCI_KEYB_RSHIFT,
	PCI_KEYB_MULTIPLY,
	PCI_KEYB_LALT,
	PCI_KEYB_SPACE,
	PCI_KEYB_CAPITAL,
	PCI_KEYB_F1,
	PCI_KEYB_F2,
	PCI_KEYB_F3,
	PCI_KEYB_F4,
	PCI_KEYB_F5,
	PCI_KEYB_F6,
	PCI_KEYB_F7,
	PCI_KEYB_F8,
	PCI_KEYB_F9,
	PCI_KEYB_F10,
	PCI_KEYB_NUMLOCK,
	PCI_KEYB_SCROLL,
	PCI_KEYB_NUMPAD7,
	PCI_KEYB_NUMPAD8,
	PCI_KEYB_NUMPAD9,
	PCI_KEYB_SUBTRACT,
	PCI_KEYB_NUMPAD4,
	PCI_KEYB_NUMPAD5,
	PCI_KEYB_NUMPAD6,
	PCI_KEYB_ADD,
	PCI_KEYB_NUMPAD1,
	PCI_KEYB_NUMPAD2,
	PCI_KEYB_NUMPAD3,
	PCI_KEYB_NUMPAD0,
	PCI_KEYB_DECIMAL,
	PCI_KEYB_OEM_102,
	PCI_KEYB_F11,
	PCI_KEYB_F12,

	PCI_F13 = PCI_KEYB | 0x64,
	PCI_F14 = PCI_KEYB | 0x65,
	PCI_F15 = PCI_KEYB | 0x66,

	PCI_KANA = PCI_KEYB | 0x70,
	PCI_ABNT_C1 = PCI_KEYB | 0x73,
	PCI_CONVERT = PCI_KEYB | 0x79,
	PCI_NOCONVERT = PCI_KEYB | 0x7B,
	PCI_YEN = PCI_KEYB | 0x7D,
	PCI_ABNT_C2 = PCI_KEYB | 0x7E,
	PCI_NUMPADEQUALS = PCI_KEYB | 0x8D,
	PCI_PREVTRACK = PCI_KEYB | 0x90,
	PCI_AT = PCI_KEYB | 0x91,
	PCI_COLON = PCI_KEYB | 0x92,
	PCI_UNDERLINE = PCI_KEYB | 0x93,
	PCI_KANJI = PCI_KEYB | 0x94,
	PCI_STOP = PCI_KEYB | 0x95,
	PCI_AX = PCI_KEYB | 0x96,
	PCI_UNLABELED = PCI_KEYB | 0x97,
	PCI_NEXTTRACK = PCI_KEYB | 0x99,
	PCI_NUMPADENTER = PCI_KEYB | 0x9C,
	PCI_RCONTROL = PCI_KEYB | 0x9D,
	PCI_MUTE = PCI_KEYB | 0xA0,
	PCI_CALCULATOR = PCI_KEYB | 0xA1,
	PCI_PLAYPAUSE = PCI_KEYB | 0xA2,
	PCI_MEDIASTOP = PCI_KEYB | 0xA4,
	PCI_VOLUMEDOWN = PCI_KEYB | 0xAE,
	PCI_VOLUMEUP = PCI_KEYB | 0xB0,
	PCI_WEBHOME = PCI_KEYB | 0xB2,
	PCI_NUMPADCOMMA = PCI_KEYB | 0xB3,
	PCI_DIVIDE = PCI_KEYB | 0xB5,
	PCI_SYSRQ = PCI_KEYB | 0xB7,
	PCI_RALT = PCI_KEYB | 0xB8,
	PCI_PAUSE = PCI_KEYB | 0xC5,
	PCI_HOME = PCI_KEYB | 0xC7,
	PCI_UPARROW = PCI_KEYB | 0xC8,
	PCI_PRIOR = PCI_KEYB | 0xC9,
	PCI_LEFTARROW = PCI_KEYB | 0xCB,
	PCI_RIGHTARROW = PCI_KEYB | 0xCD,
	PCI_END = PCI_KEYB | 0xCF,
	PCI_DOWNARROW = PCI_KEYB | 0xD0,
	PCI_NEXT = PCI_KEYB | 0xD1,
	PCI_INSERT = PCI_KEYB | 0xD2,
	PCI_DELETE = PCI_KEYB | 0xD3,
	PCI_LWIN = PCI_KEYB | 0xDB,
	PCI_RWIN = PCI_KEYB | 0xDC,
	PCI_APPS = PCI_KEYB | 0xDD,
	PCI_POWER = PCI_KEYB | 0xDE,
	PCI_SLEEP = PCI_KEYB | 0xDF,
	PCI_WAKE = PCI_KEYB | 0xE3,
	PCI_WEBSEARCH = PCI_KEYB | 0xE5,
	PCI_WEBFAVORITES = PCI_KEYB | 0xE6,
	PCI_WEBREFRESH = PCI_KEYB | 0xE7,
	PCI_WEBSTOP = PCI_KEYB | 0xE8,
	PCI_WEBFORWARD = PCI_KEYB | 0xE9,
	PCI_WEBBACK = PCI_KEYB | 0xEA,
	PCI_MYCOMPUTER = PCI_KEYB | 0xEB,
	PCI_MAIL = PCI_KEYB | 0xEC,
	PCI_MEDIASELECT = PCI_KEYB | 0xED,

	// Mouse inputs.
	PCI_MOUSE = MOUSE_MASK,

	// TODO: Add mouse inputs, later.

	// Special inputs, that have extra / special meaning?
	// Maybe only used when we read in the gamepad database?
	PCI_XTRA = 0x8000,

	// NOTE: These are kind of special...  they describe two values (+/-) for a single axis.
	// Also, this is more of a thing that we use when reading the pad database file....
	PIC_PAD_LSTICK_X,         // x-axis (+/-)
	PIC_PAD_LSTICK_Y,         // y-axis (+/-)

	// Analog stick directions.
	PIC_PAD_RSTICK_X,         // x-axis (+/-)
	PIC_PAD_RSTICK_Y,         // y-axis (+/-)


};


// REFACTOR: This can describe other inputs, not just gamepads.
// This type is used as a way to describe a unique button,etc. and how it maps onto the driver input.
struct GamepadInputDesc {
	PCInputs Input;				// The type of input.  This is really just a unique identifier for the input....
	UINT32 DriverInputIndex;		// The corresponding input index (defined in the driver).  NOTE: DriverInputIndex can be defined more than once to allow for analog stick + dpad inputs for directions.
};

#endif
