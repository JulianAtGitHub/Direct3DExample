#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>
#include <shellapi.h>

#include <d3d12.h>
#include <dxgi1_4.h>
#if defined(_DEBUG)
#include <dxgidebug.h>
#endif
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "Common/d3dx12.h"

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cassert>

#include "Common/Utils.h"

#include "Common/CUtils.hpp"
#include "Common/CString.hpp"
#include "Common/CList.hpp"
#include "Common/CStack.hpp"
#include "Common/CTree.hpp"
