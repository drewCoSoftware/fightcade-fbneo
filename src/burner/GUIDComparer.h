#ifndef GUIDCOMPARER_H
#define GUIDCOMPARER_H

// #include <InitGuid.h>
#include "guiddef.h"

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

#endif
