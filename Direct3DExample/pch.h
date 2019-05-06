#pragma once

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include <windows.h>
#include <shellapi.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <wchar.h>

#include <exception>

#include <wrl.h>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <dxgi1_5.h>
#include <dxgi1_6.h>

#if defined(_DEBUG)
    #include <dxgidebug.h>
#endif

#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "DirectX/d3dx12.h"

#ifdef __cplusplus
    using namespace Microsoft;
    using namespace DirectX;
#endif

#include "Common/Utils.hpp"

#include "Common/CUtils.hpp"
#include "Common/CString.hpp"
#include "Common/CList.hpp"
#include "Common/CQueue.hpp"
#include "Common/CStack.hpp"
#include "Common/CTree.hpp"

