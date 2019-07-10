#pragma once

#ifdef _MSC_VER 
#define GCONST extern __declspec(selectany) const
#else
#define GCONST const
#endif

GCONST float    gfMaxPerspective = 1000000.f;
GCONST float    gfMinPerspective = 100.f;

GCONST std::wstring gsBingAPIKey = L"{your Bing API key here}";

GCONST GLfloat gfRectMatrix[] = {
	0.0f,  0.0f,
	0.0f,  1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f
};

#define SHADER_CODE __declspec(selectany) extern char* const

SHADER_CODE gwTileVS =
#include "tile.vs"
"";

SHADER_CODE gwTileFS =
#include "tile.fs"
"";