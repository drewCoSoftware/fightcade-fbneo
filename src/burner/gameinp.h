#ifndef GAMEINP_H
#define GAMEINP_H


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

struct giForce {
	UINT8 nInput;				// The input to apply force feedback effects to
	UINT8 nEffect;				// The effect to use
};

struct giMacro {
	UINT8 nMode;				// 0 = Unused, 1 = used

	UINT8* pVal[4];				// Destination for the Input Value
	UINT8 nVal[4];				// The Input Value
	UINT8 nInput[4];			// Which inputs are mapped

	struct giSwitch Switch;

	char szName[33];			// Maximum name length 16 chars
	UINT8 nSysMacro;			// mappable system macro (1) or Auto-Fire (15)
};

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




struct GameInp {
	UINT8 nInput;				// PC side: see above
	UINT8 nType;				// game side: see burn.h

	union {
		struct giInput Input;
		struct giForce Force;
		struct giMacro Macro;
	};
};

// Input types for gamepads.  This is part of an effort to get things
// a bit more standard, a bit more SDL like...
enum EInputType {
	ITYPE_UNSET = 0,
	ITYPE_BUTTON,				// on/off
	ITYPE_DPAD,

	// NOTE: We will have to revisit this as the indexes may not be full compatible with FBNEO.
	ITYPE_FULL_ANALOG,    // The full +/- minus range.  Something like a stick.
	ITYPE_HALF_ANALOG   // Half axis range, + or -.  Something like a trigger.
};

//
//// Input types for gamepads.  This is part of an effort to get things
//// a bit more standard, a bit more SDL like...
//enum EInputType {
//	ITYPE_UNSET = 0,
//	ITYPE_BUTTON,				// on/off
//	ITYPE_DPAD,
//	ITYPE_ANALOG					// Axes, sticks, triggers, etc.
//	//ITYPE_STICK,				// analog stick
//	//ITYPE_TRIGGER				// triggers, like LT/RT on gamepad
//};

// =============================================================================================
// Standardized names for gamepad inputs.
enum EGamepadInput {
	GPINPUT_UNSUPPORTED = 0x00,

	// Analog stick directions.
	// NOTE: This was my first approach where each analog direction (+/-) has its own designation.  This was to make it compatible with the single direction triggers...
	// Clearly some extra mapping needs to take place....
	//GPINPUT_LSTICK_UP,
	//GPINPUT_LSTICK_DOWN,
	//GPINPUT_LSTICK_LEFT,
	//GPINPUT_LSTICK_RIGHT,
	//GPINPUT_RSTICK_UP,
	//GPINPUT_RSTICK_DOWN,
	//GPINPUT_RSTICK_LEFT,
	//GPINPUT_RSTICK_RIGHT,

	GPINPUT_LSTICK_X,         // x-axis (+/-)
	GPINPUT_LSTICK_Y,         // y-axis (+/-)

	// Analog stick directions.
	GPINPUT_RSTICK_X,         // x-axis (+/-)
	GPINPUT_RSTICK_Y,         // y-axis (+/-)

	// L/R stick 'click' buttons.
	GPINPUT_LSTICK_BUTTON,
	GPINPUT_RSTICK_BUTTON,


	// NOTE: I think that a single 'DPAD' type will be OK.
	GPINPUT_DPAD_UP,
	GPINPUT_DPAD_DOWN,
	GPINPUT_DPAD_LEFT,
	GPINPUT_DPAD_RIGHT,

	GPINPUT_BACK,				// back / select button.
	GPINPUT_START,

	GPINPUT_X,				// PS - square		: SWITCH - Y
	GPINPUT_Y,				// PS - triangle	: SWITCH - X
	GPINPUT_A,				// PS - cross / x	: SWITCH - B
	GPINPUT_B,				// PS - circle		: SWITCH - A

	// Analog triggers....
	GPINPUT_LEFT_TRIGGER,
	GPINPUT_LEFT_BUMPER,

	GPINPUT_RIGHT_TRIGGER,
	GPINPUT_RIGHT_BUMPER,

	GPINPUT_TOUCHPAD,       // This is some kind of PS specific thing.

	GPINPUT_GUIDE,          // This is the main x-box button, etc.
	GPINPUT_MISC_BUTTON,   // Just some random, extra button....


	// TODO: Add more as needed.
};

//
//// =============================================================================================
//// Standardized names for gamepad inputs.
//enum EGamepadInput {
//	GPINPUT_UNKNOWN = 0x00,
//
//	GPINPUT_LSTICK_UP,
//	GPINPUT_LSTICK_DOWN,
//	GPINPUT_LSTICK_LEFT,
//	GPINPUT_LSTICK_RIGHT,
//
//	GPINPUT_RSTICK_UP,
//	GPINPUT_RSTICK_DOWN,
//	GPINPUT_RSTICK_LEFT,
//	GPINPUT_RSTICK_RIGHT,
//
//	GPINPUT_DPAD_UP,
//	GPINPUT_DPAD_DOWN,
//	GPINPUT_DPAD_LEFT,
//	GPINPUT_DPAD_RIGHT,
//
//	GPINPUT_BACK,				// back / select button.
//	GPINPUT_START,
//
//	GPINPUT_X,				// PS - square		: SWITCH - Y
//	GPINPUT_Y,				// PS - triangle	: SWITCH - X
//	GPINPUT_A,				// PS - cross / x	: SWITCH - B
//	GPINPUT_B,				// PS - circle		: SWITCH - A
//
//	GPINPUT_LEFT_TRIGGER,
//	GPINPUT_LEFT_BUMPER,
//
//	GPINPUT_RIGHT_TRIGGER,
//	GPINPUT_RIGHT_BUMPER,
//
//	// TODO: Add more as needed.
//};


#endif
