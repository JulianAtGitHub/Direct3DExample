
#ifndef PCH_H
#define PCH_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <random>
#include <vector>

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

// random value from [0.0 ~ 1.0)
extern float RandomUnit(void);

// (-1.0 ~ 1.0)
extern XMVECTOR RandomInUnitDisk(void);

// (-1.0 ~ 1.0)
extern XMVECTOR RandomInUnitSphere(void);

#endif //PCH_H
