
#ifndef PCH_H
#define PCH_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <random>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <DirectXMath.h>

using namespace DirectX;

#define INLINE __forceinline

#endif //PCH_H
