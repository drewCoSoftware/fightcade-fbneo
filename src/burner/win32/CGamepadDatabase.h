#pragma once

#include <string>
#include <map>

#include "GUIDComparer.h"
#include "CGamepadMappingsFile.h"

// Return codes...
#define PARSE_OK 0
#define PARSE_FAILED 1
#define CANT_PARSE 2
#define WRONG_PLATFORM 3

// This is the thing that can read an SDL gamepad database file and present it in a way
// that is compatible with FBNeo.  Long term, the best choice would be to use SDL in the windows
// version of FBNeo instead of DirectX.
class CGamepadDatabase {

private:
  std::wstring Platform;      // The platform (windows, linux, etc.) that we want to read entries for.
  bool _IsInitialized = false;

  std::map<GUID, CGamepadButtonMapping, GUIDComparer> _Mappings;
  size_t ReadFile(const wchar_t* sdldbPath, const wchar_t* platform);
  CGamepadButtonMapping _DefaultMapping;
  void CreateDefaultMapping();

public:
  size_t Init(const wchar_t* sdldbPath, const wchar_t* platform_);
  bool TryGetMapping(const GUID productGuid, CGamepadButtonMapping& mapping);

  void SetDefaultMapping(const CGamepadButtonMapping& mapping);
  void GetDefaultMapping(CGamepadButtonMapping& mapping);

  size_t ParseLine(std::wstring& curLine, CGamepadButtonMapping& mapping, GUID& mappingGuid);

  inline const bool IsInitialized() { return _IsInitialized; }
};

