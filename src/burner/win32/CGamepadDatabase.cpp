#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "Windows.h"

#include "CGamepadDatabase.h"

static std::map<size_t, EGamepadInput> SDLButtonsToGamepadInputs;
static std::map<size_t, EInputType> TypeCodesToInputTypes;

// ------------------------------------------------------------------------------------------------------------------------------------
// TODO: Translate other debug messages.
void DebugMsg(const char* msg) {
	printf(msg);
}

// ------------------------------------------------------------------------------------------------------------------------------------
// NOTE: Copied from FC + modified for our purposes.
size_t GetHash(const wchar_t* str)
{
	size_t len = wcslen(str);
	size_t hash = 1315423911;
	for (size_t i = 0; i < len; i++) {
		hash ^= ((hash << 5) + str[i] + (hash >> 2));
	}
	return hash;
}

// https://github.com/libsdl-org/SDL/blob/main/src/joystick/SDL_joystick.c#L2503
// ------------------------------------------------------------------------------------------------------------------------------------
std::vector<std::wstring> Split(const std::wstring input, const std::wstring delimiter, bool includeEmpties)
{
	std::vector<std::wstring> res;

	size_t start = 0;
	size_t inputLen = input.length();


	while (true)
	{
		size_t pos = input.find(delimiter, start);
		if (pos != -1)
		{
			size_t subLen = pos - start;
			std::wstring toAdd = input.substr(start, subLen);
			res.push_back(toAdd);

			start = pos + delimiter.length();
		}
		else {
			// We didn't find the delimiter!  Just copy in the rest of the data...
			std::wstring toAdd = input.substr(start);
			if (toAdd.length() > 0 || includeEmpties) {
				res.push_back(toAdd);
			}

			break;
		}
	}

	return res;
}


// ------------------------------------------------------------------------------------------------------------------------------------
bool TranslateMappingEntry(const std::vector<std::wstring> parts, CGamepadMappingEntry& entry) {

	ZeroMemory(&entry, sizeof(CGamepadMappingEntry));

	if (parts.size() != 2) {
		DebugMsg("Incorrect part count for entry");
		return false;
	}

	auto inputName = parts[0].data();
	auto inputVal = parts[1].data();

	size_t hash = GetHash(inputName);
	auto it = SDLButtonsToGamepadInputs.find(hash);
	if (it != SDLButtonsToGamepadInputs.end()) {
		entry.Input = it->second;

		// NOTE: This is wrong because the typecode is everything up to the index / first number!
		int x = 10;
		wchar_t codeString[8] = {};
		wchar_t typeCode = inputVal[0];

		size_t codeSize = 1;
		codeString[0] = typeCode;
		if (typeCode == '+' || typeCode == '-') {
			codeString[1] = inputVal[1];
			codeSize = 2;
		}

		auto inputIt = TypeCodesToInputTypes.find(GetHash(codeString));
		if (inputIt == TypeCodesToInputTypes.end()) {
			// VERBOSE:
			printf("type code: %c is not recognized!", typeCode);
			return false;
		}

		entry.Type = inputIt->second;

		// Now we have to determine the index....
		const int MAX_INDEX_CHARS = 8;
		wchar_t indexString[MAX_INDEX_CHARS + 1];
		ZeroMemory(indexString, sizeof(wchar_t) * MAX_INDEX_CHARS + 1);

		if (typeCode == 'h') {
			// 'hat' inputs (dpad) have the form: hat-index:button-index.
			// At this time, we assume that there is only one dpad on the controller.  Others will not be supported!
			int dotPos = std::wstring(inputVal).find('.');
			if (dotPos == -1) {
				// VERBOSE:
				DebugMsg("unsupported 'hat' format!");
				return false;
			}

			memcpy(indexString, inputVal + (dotPos + 1), wcslen(inputVal) - (dotPos + 1));
			int abc = 10;

		}
		else {
			// Single digit type code.
			memcpy(indexString, inputVal + codeSize, wcslen(inputVal) - codeSize);
			int def = 10;
		}

		// Safety first!
		indexString[MAX_INDEX_CHARS] = 0;
		entry.Index = _wtoi(indexString);


		return true;
	}
	else {
		printf("The button: '%zs' is not currently supported!", inputName);
		return false;
	}


	return false;

}

// ------------------------------------------------------------------------------------------------------------------------------------
void BuildInputTypeMap() {
	TypeCodesToInputTypes[GetHash(L"b")] = ITYPE_GAMEPAD_BUTTON;
	TypeCodesToInputTypes[GetHash(L"h")] = ITYPE_DPAD;
	TypeCodesToInputTypes[GetHash(L"a")] = ITYPE_FULL_ANALOG;       // a = full axis?
	TypeCodesToInputTypes[GetHash(L"+a")] = ITYPE_HALF_ANALOG;      // +a = single (+) axis? trigger?
	TypeCodesToInputTypes[GetHash(L"-a")] = ITYPE_HALF_ANALOG;      // -a = single (-) axis? trigger?

	//  TypeCodesToInputTypes[GetHash(L"b")] = ITYPE_GAMEPAD_BUTTON;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void BuildSDLButtonMap() {

	SDLButtonsToGamepadInputs[GetHash(L"a")] = GPINPUT_A;
	SDLButtonsToGamepadInputs[GetHash(L"b")] = GPINPUT_B;
	SDLButtonsToGamepadInputs[GetHash(L"x")] = GPINPUT_X;
	SDLButtonsToGamepadInputs[GetHash(L"y")] = GPINPUT_Y;

	SDLButtonsToGamepadInputs[GetHash(L"back")] = GPINPUT_BACK;
	SDLButtonsToGamepadInputs[GetHash(L"start")] = GPINPUT_START;

	SDLButtonsToGamepadInputs[GetHash(L"dpup")] = GPINPUT_DPAD_DOWN;
	SDLButtonsToGamepadInputs[GetHash(L"dpdown")] = GPINPUT_DPAD_UP;
	SDLButtonsToGamepadInputs[GetHash(L"dpleft")] = GPINPUT_DPAD_LEFT;
	SDLButtonsToGamepadInputs[GetHash(L"dpright")] = GPINPUT_DPAD_RIGHT;

	SDLButtonsToGamepadInputs[GetHash(L"leftshoulder")] = GPINPUT_LEFT_BUMPER;
	SDLButtonsToGamepadInputs[GetHash(L"rightshoulder")] = GPINPUT_RIGHT_BUMPER;

	SDLButtonsToGamepadInputs[GetHash(L"lefttrigger")] = GPINPUT_LEFT_TRIGGER;
	SDLButtonsToGamepadInputs[GetHash(L"righttrigger")] = GPINPUT_RIGHT_TRIGGER;

	SDLButtonsToGamepadInputs[GetHash(L"leftstick")] = GPINPUT_LSTICK_BUTTON;
	SDLButtonsToGamepadInputs[GetHash(L"rightstick")] = GPINPUT_RSTICK_BUTTON;

	SDLButtonsToGamepadInputs[GetHash(L"leftx")] = GPINPUT_LSTICK_X;
	SDLButtonsToGamepadInputs[GetHash(L"lefty")] = GPINPUT_LSTICK_Y;
	SDLButtonsToGamepadInputs[GetHash(L"rightx")] = GPINPUT_RSTICK_X;
	SDLButtonsToGamepadInputs[GetHash(L"righty")] = GPINPUT_RSTICK_Y;

	SDLButtonsToGamepadInputs[GetHash(L"guide")] = GPINPUT_GUIDE;

	// These are the 'camera' buttons on an n64.  we don't support these at the moment.
	SDLButtonsToGamepadInputs[GetHash(L"+rightx")] = GPINPUT_DPAD_RIGHT;
	SDLButtonsToGamepadInputs[GetHash(L"-rightx")] = GPINPUT_DPAD_LEFT;
	SDLButtonsToGamepadInputs[GetHash(L"+righty")] = GPINPUT_DPAD_UP;
	SDLButtonsToGamepadInputs[GetHash(L"-righty")] = GPINPUT_DPAD_DOWN;

	// These are apparently unique to an 8bit-do controller...?
	// https://app.8bitdo.com/  Just regular buttons, but they are on the back of the controller....
	SDLButtonsToGamepadInputs[GetHash(L"paddle1")] = GPINPUT_MISC_BUTTON;
	SDLButtonsToGamepadInputs[GetHash(L"paddle2")] = GPINPUT_MISC_BUTTON;
	SDLButtonsToGamepadInputs[GetHash(L"paddle3")] = GPINPUT_MISC_BUTTON;
	SDLButtonsToGamepadInputs[GetHash(L"paddle4")] = GPINPUT_MISC_BUTTON;

	SDLButtonsToGamepadInputs[GetHash(L"misc1")] = GPINPUT_MISC_BUTTON;
	SDLButtonsToGamepadInputs[GetHash(L"misc2")] = GPINPUT_MISC_BUTTON;
	SDLButtonsToGamepadInputs[GetHash(L"misc3")] = GPINPUT_MISC_BUTTON;

	SDLButtonsToGamepadInputs[GetHash(L"touchpad")] = GPINPUT_TOUCHPAD;

	// Not sure if this is the correct way to handle this, but this seems to be specific to the 'Gravis Destroyer' / 'JoyCon'
	// It is just a dpad, but has a weird entry in the SDL DB.
	SDLButtonsToGamepadInputs[GetHash(L"+leftx")] = GPINPUT_DPAD_RIGHT;
	SDLButtonsToGamepadInputs[GetHash(L"-leftx")] = GPINPUT_DPAD_LEFT;
	SDLButtonsToGamepadInputs[GetHash(L"+lefty")] = GPINPUT_DPAD_UP;
	SDLButtonsToGamepadInputs[GetHash(L"-lefty")] = GPINPUT_DPAD_DOWN;


	// TODO: The rest of them....
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
void SwapBytes(UINT16& data) {

	char b1 = data & 0x00FF;
	char b2 = (data & 0xFF00) >> 8;

	data = (b1 << 8) | b2;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// Given the SDL guid, we will compute a guid that is compatible with DX8Input / DirectX
GUID TranslateSDLGuid(const std::wstring sdlGuid) {

	auto vid = sdlGuid.substr(8, 4);
	auto pid = sdlGuid.substr(16, 4);

	//if (wcscmp(sdlGuid.data(), L"030000006d0400001ec2000000000000") == 0) {
	//    int acbd = 10;
	//}

	UINT16 vidVal = (UINT16)wcstol(vid.data(), nullptr, 16);
	UINT16 pidVal = (UINT16)wcstol(pid.data(), nullptr, 16);

	SwapBytes(vidVal);
	SwapBytes(pidVal);

	unsigned long pidvid = 0;
	pidvid |= vidVal;
	pidvid |= pidVal << 16;

	GUID dxGuid;
	dxGuid.Data1 = pidvid;
	dxGuid.Data2 = 0;
	dxGuid.Data3 = 0;

	const char pidvidPart[8] = { 0x00, 0x00, 0x50, 0x49, 0x44, 0x56, 0x49, 0x44 };
	memcpy(dxGuid.Data4, pidvidPart, sizeof(char) * 8);

	return dxGuid;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
size_t CGamepadDatabase::ParseLine(std::wstring& curLine, CGamepadButtonMapping& mapping, GUID& mappingGuid) {

	// Trim excessing newline characters.
	size_t newLine = curLine.find('\n');
	if (newLine != -1)
	{
		curLine.erase(newLine);
	}

	// Transfer contents + clear the current line.
	// This is just a convenience for downstream processing.
	std::wstring useLine;
	useLine.append(curLine);
	curLine.clear();

	// No data, or a comment.
	// NOTE: In a future iteration we may want to add some kind of version check...?
	wchar_t firstChar = useLine.data()[0];
	if (useLine.length() == 0 || firstChar == '#' || firstChar == '\n') {
		useLine.clear();
		return CANT_PARSE;
	}

	// Now we can parse the line.  We can start by splitting it into an array of strings.....
	auto cols = Split(useLine, L",", false);
	size_t colCount = cols.size();
	if (colCount < 4) {
		printf("Column count of: %zu is not valid.... skipping...\n", colCount);
		return CANT_PARSE;
	}


	const size_t GUID_LEN = 32;
	auto sdlGuid = cols[0];
	size_t guidLen = sdlGuid.length();

	if (guidLen != GUID_LEN) {
		printf("Expected guid length is: %zu but we expected a length of: %zu... skipping...\n", guidLen, GUID_LEN);
		return CANT_PARSE;
	}

	// TODO: Extra, unnecessary data copies here.  Update the code to cut down on them.
	GUID mGuid = TranslateSDLGuid(sdlGuid);
	memcpy(&mappingGuid, &mGuid, sizeof(GUID));

	// We don't parse lines for platfomrs we don't care about.
	std::wstring platform = cols[colCount - 1];
	if (!platform._Equal(this->Platform.data())) {
		return WRONG_PLATFORM;
	}

	// We want to loop through all of the listed inputs + parse them as needed.
	size_t inputCount = colCount - 3;
	if (inputCount > MAX_ENTRY) {
		// VERBOSE:
		printf("The number of inputs for the device: %zu exceeds the max: %u, some inputs may not work correctly!  Consider altering the MAX_ENTRY define!\n", inputCount, MAX_ENTRY);
		inputCount = MAX_ENTRY;
	}

	int entryIndex = 0;
	bool readOK = true;
	for (size_t i = 0; i < inputCount; i++)
	{
		std::wstring input = cols[2 + i];
		auto parts = Split(input, L":", true);

		CGamepadMappingEntry entry;
		bool mappingOK = TranslateMappingEntry(parts, entry);
		if (mappingOK) {

			// Expand 'full analog' inputs as needed.
			bool isStickType = entry.Input == GPINPUT_LSTICK_X || entry.Input == GPINPUT_LSTICK_Y || entry.Input == GPINPUT_RSTICK_X || entry.Input == GPINPUT_RSTICK_Y;

			if (entry.Type == ITYPE_FULL_ANALOG && isStickType)
			{
				switch (entry.Input)
				{
				case GPINPUT_LSTICK_X:
				{
					CGamepadMappingEntry e1;
					e1.Input = GPINPUT_LSTICK_LEFT;
					e1.Type = ITYPE_HALF_ANALOG;
					e1.Index = 0;

					CGamepadMappingEntry e2;
					e2.Input = GPINPUT_LSTICK_RIGHT;
					e2.Type = ITYPE_HALF_ANALOG;
					e2.Index = 1;

					mapping.Entries[entryIndex] = e1;
					mapping.Entries[entryIndex + 1] = e2;

					entryIndex += 2;
					break;
				}
				case GPINPUT_LSTICK_Y:
				{
					CGamepadMappingEntry e1;
					e1.Input = GPINPUT_LSTICK_UP;
					e1.Type = ITYPE_HALF_ANALOG;
					e1.Index = 2;

					CGamepadMappingEntry e2;
					e2.Input = GPINPUT_LSTICK_DOWN;
					e2.Type = ITYPE_HALF_ANALOG;
					e2.Index = 3;

					mapping.Entries[entryIndex] = e1;
					mapping.Entries[entryIndex + 1] = e2;

					entryIndex += 2;
					break;
				}

				case GPINPUT_RSTICK_X:
				{
					CGamepadMappingEntry e1;
					e1.Input = GPINPUT_RSTICK_LEFT;
					e1.Type = ITYPE_HALF_ANALOG;
					e1.Index = 0;

					CGamepadMappingEntry e2;
					e2.Input = GPINPUT_RSTICK_RIGHT;
					e2.Type = ITYPE_HALF_ANALOG;
					e2.Index = 1;

					mapping.Entries[entryIndex] = e1;
					mapping.Entries[entryIndex + 1] = e2;

					entryIndex += 2;
					break;
				}
				case GPINPUT_RSTICK_Y:
				{
					CGamepadMappingEntry e1;
					e1.Input = GPINPUT_RSTICK_UP;
					e1.Type = ITYPE_HALF_ANALOG;
					e1.Index = 0;

					CGamepadMappingEntry e2;
					e2.Input = GPINPUT_RSTICK_DOWN;
					e2.Type = ITYPE_HALF_ANALOG;
					e2.Index = 3;

					mapping.Entries[entryIndex] = e1;
					mapping.Entries[entryIndex + 1] = e2;

					entryIndex += 2;
					break;
				}

				// NOTE: We also have to deal with full analog triggers....which is a thing somehow.... :(

				//case GPINPUT_LEFT_BUMPER:
				//case GPINPUT_RIGHT_BUMPER:
				//case GPINPUT_LEFT_TRIGGER:
				//case GPINPUT_RIGHT_TRIGGER:
				//{
				//	// Normal
				//	entry.Type = ITYPE_HALF_ANALOG;
				//	mapping.Entries[entryIndex] = entry;
				//	++entryIndex;
				//	break;
				//}

				default:
					throw std::exception("unexpected input for full analog type1");
				}
				//e1.Input = GP_INPUT
			}
			else {
				// Normal
				mapping.Entries[entryIndex] = entry;
				++entryIndex;
			}

		}
		else {
			printf("could not translate mapping: %s", input);
		}
	}
	if (!readOK) {
		return PARSE_FAILED;
	}

	mapping.EntryCount = entryIndex;

	return PARSE_OK;
}



// ---------------------------------------------------------------------------------------------------------------------------------------
size_t CGamepadDatabase::ReadFile(const wchar_t* sdldbPath, const wchar_t* platform) {

	FILE* fs = nullptr;

	std::wstring curLine;

	const int BUFFER_SIZE = 0x800;
	wchar_t readBuffer[BUFFER_SIZE];

	bool inWindowsSection = false;

	try {
		errno_t err = _wfopen_s(&fs, sdldbPath, L"r");
		if (err == 0) {

			// Now we can pull in the data, parsing it line by line...
			int lineCount = 0;
			while (auto bufferData = fgetws(readBuffer, BUFFER_SIZE, fs) != nullptr)
			{
				size_t readLen = wcslen(readBuffer);
				if (readLen == 0) { continue; }

				bool lineComplete = false;
				curLine.append(readBuffer);

				wchar_t check = readBuffer[readLen - 1];
				if (check == '\n') {
					// The current line is complete!
					lineComplete = true;
				}

				if (lineComplete) {

					// TODO: We only care about reading the windows section.
					// I want to update the code so that it reflects this and stops / doesn't process anything not in the '# Windows' section.
					//if (wcscmp(curLine.data(), L"# Windows\n")) {
					//  inWindowsSection = true;
					//}
					//else if (wcscmp(curLine.data(), L"# Mac

					GUID dxGuid;

					++lineCount;
					CGamepadButtonMapping mapping = {};
					int code = ParseLine(curLine, mapping, dxGuid);
					if (code == PARSE_OK)
					{
						_Mappings[dxGuid] = mapping;
					}
					else {
						if (code == PARSE_FAILED) {
							DebugMsg("Parsing failed!\n");
							continue;
						}
						else {
							DebugMsg("Could not parse the input for some reason...\n");
							continue;
						}
					}
				}

			}

			if (feof(fs)) {
				// VERBOSE:
				DebugMsg("File read complete!\n");
			}
			else if (ferror(fs)) {
				// VERBOSE:
				printf("There was an error reading the file at line: %u\n", lineCount);
			}
		}
		else {
			printf("Could not open the database file!");
		}
	}
	catch (const std::exception& ex)
	{
		fclose(fs);
		fs = nullptr;
		return 2;
	}
	if (fs) { fclose(fs); }

	return 0;
}


// ---------------------------------------------------------------------------------------------------------------------------------------
size_t CGamepadDatabase::Init(const wchar_t* sdldbPath, const wchar_t* platform)
{
	if (_IsInitialized) {
		// VERBOSE:
		printf("Gamepad database has already been initialized!");
		return 1;
	}

	this->Platform.clear();
	this->Platform.append(L"platform:");
	this->Platform.append(platform);

	// Read in the DB file, and build the map with it.
	BuildSDLButtonMap();
	BuildInputTypeMap();

	CreateDefaultMapping();

	size_t readRes = ReadFile(sdldbPath, platform);
	if (readRes != 0) {
		return readRes;
	}


	_IsInitialized = true;
	return 0;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
void CGamepadDatabase::CreateDefaultMapping() {

	CGamepadButtonMapping mapping;
	ZeroMemory(&mapping, sizeof(CGamepadButtonMapping));

	// NOTE: These inputs technically make sense, but they should be expanded so that they are more compatible with FBNEO.
	mapping.Entries[0] = { GPINPUT_LSTICK_UP, ITYPE_HALF_ANALOG, 0x02 };
	mapping.Entries[1] = { GPINPUT_LSTICK_DOWN, ITYPE_HALF_ANALOG, 0x03 };
	mapping.Entries[2] = { GPINPUT_LSTICK_LEFT, ITYPE_HALF_ANALOG, 0x00 };
	mapping.Entries[3] = { GPINPUT_LSTICK_RIGHT, ITYPE_HALF_ANALOG, 0x01 };

	// NOTE: These may be incorrect.
	mapping.Entries[4] = { GPINPUT_RSTICK_UP, ITYPE_HALF_ANALOG, 0x02 };
	mapping.Entries[5] = { GPINPUT_RSTICK_DOWN, ITYPE_HALF_ANALOG, 0x03 };
	mapping.Entries[6] = { GPINPUT_RSTICK_LEFT, ITYPE_HALF_ANALOG, 0x00 };
	mapping.Entries[7] = { GPINPUT_RSTICK_RIGHT, ITYPE_HALF_ANALOG, 0x01 };


	mapping.Entries[8] = { GPINPUT_DPAD_UP, ITYPE_DPAD, 0x02 };
	mapping.Entries[9] = { GPINPUT_DPAD_DOWN, ITYPE_DPAD, 0x03 };
	mapping.Entries[10] = { GPINPUT_DPAD_LEFT, ITYPE_DPAD, 0x00 };
	mapping.Entries[11] = { GPINPUT_DPAD_RIGHT, ITYPE_DPAD, 0x01 };

	mapping.Entries[12] = { GPINPUT_BACK, ITYPE_GAMEPAD_BUTTON, 0x06 };
	mapping.Entries[13] = { GPINPUT_START, ITYPE_GAMEPAD_BUTTON, 0x07 };

	mapping.Entries[14] = { GPINPUT_X, ITYPE_GAMEPAD_BUTTON, 0x02 };
	mapping.Entries[15] = { GPINPUT_Y, ITYPE_GAMEPAD_BUTTON, 0x03 };
	mapping.Entries[16] = { GPINPUT_A, ITYPE_GAMEPAD_BUTTON, 0x00 };
	mapping.Entries[17] = { GPINPUT_B, ITYPE_GAMEPAD_BUTTON, 0x01 };

	// ?? Maybe......
	mapping.Entries[18] = { GPINPUT_LEFT_BUMPER, ITYPE_GAMEPAD_BUTTON, 0x04 };
	mapping.Entries[19] = { GPINPUT_LEFT_TRIGGER, 	ITYPE_HALF_ANALOG, 0x05 };

	mapping.Entries[20] = { GPINPUT_RIGHT_BUMPER, ITYPE_GAMEPAD_BUTTON, 0x05 };
	mapping.Entries[21] = { GPINPUT_RIGHT_TRIGGER, ITYPE_HALF_ANALOG, 0x04 };

	mapping.EntryCount = 22;
	mapping.IsDefault = true;
	mapping.IsPatch = false;

	SetDefaultMapping(mapping);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
bool CGamepadDatabase::TryGetMapping(const GUID productGuid, CGamepadButtonMapping& mapping) {

	auto it = _Mappings.find(productGuid);
	if (it == _Mappings.end()) {
		return false;
	}

	memcpy(&mapping, &(it->second), sizeof(CGamepadButtonMapping));
	return true;
}


// --------------------------------------------------------------------------------------------------------------------------------------
void CGamepadDatabase::SetDefaultMapping(const CGamepadButtonMapping& mapping) {
	memcpy(&_DefaultMapping, &mapping, sizeof(CGamepadButtonMapping));
}

// --------------------------------------------------------------------------------------------------------------------------------------
void CGamepadDatabase::GetDefaultMapping(CGamepadButtonMapping& mapping) {
	memcpy(&mapping, &_DefaultMapping, sizeof(CGamepadButtonMapping));
}