#pragma once

#define INLINE __forceinline

extern void * ReadFileData(const char *fileName, size_t &size);

extern void SIMDMemCopy( void* __restrict Dest, const void* __restrict Source, size_t NumQuadwords );
extern void SIMDMemFill( void* __restrict Dest, __m128 FillVector, size_t NumQuadwords );

template <typename T>
INLINE T AlignUpWithMask(T value, size_t mask) {
    return (T)(((size_t)value + mask) & ~mask);
}

template <typename T> 
INLINE T AlignDownWithMask(T value, size_t mask) {
    return (T)((size_t)value & ~mask);
}

template <typename T> 
INLINE T AlignUp(T value, size_t alignment) {
    return AlignUpWithMask(value, alignment - 1);
}

template <typename T> 
INLINE T AlignDown(T value, size_t alignment) {
    return AlignDownWithMask(value, alignment - 1);
}

template <typename T> 
INLINE bool IsAligned( T value, size_t alignment ) {
    return 0 == ((size_t)value & (alignment - 1));
}

template <typename T> 
INLINE bool IsPowerOfTwo(T value) {
    return 0 == (value & (value - 1));
}

template <typename T> 
INLINE T DivideByMultiple( T value, size_t alignment ) {
    return (T)((value + alignment - 1) / alignment);
}

INLINE char * WStr2Str(const wchar_t *wstr) {
    if (!wstr) { return nullptr; }
    size_t len = wcslen(wstr) * 2 + 1;
    char *str = (char *)malloc(len);
    size_t converted = 0;
    wcstombs_s(&converted, str, len, wstr, _TRUNCATE );
    return str;
}

INLINE char * CopyStr(const char *source) {
    if (!source) {
        return nullptr;
    }

    size_t size = strlen(source) + 1;
    char *dest = new char[size];
    strcpy_s(dest, size, source);

    return dest;
}

INLINE wchar_t * CopyWStr(const wchar_t *source) {
    if (!source) {
        return nullptr;
    }

    size_t size = wcslen(source) + 1;
    wchar_t *dest = new wchar_t[size];
    wcscpy_s(dest, size, source);

    return dest;
}

struct CHException : public std::exception {
    CHException(HRESULT hr) {
        sprintf(mStr, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    }
    char mStr[64];
};

INLINE void ThrowIfFailed(HRESULT hr) {
    if (FAILED(hr)) {
        throw CHException(hr);
    }
}

INLINE void Printf(const char *str) {
#ifdef _DEBUG
    OutputDebugStringA(str);
#else
    printf("%s", str);
#endif
}

INLINE void WPrintf(const wchar_t *str) {
#ifdef _DEBUG
    OutputDebugStringW(str);
#else
    wprintf(L"%s", str);
#endif
}

#define MAX_CHAR_A_LINE 256 

INLINE void Print(const char *fmt, ...) {
    char line[MAX_CHAR_A_LINE];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, MAX_CHAR_A_LINE, fmt, ap);
    va_end(ap);
    Printf(line);
}

INLINE void PrintSub(const char* fmt, ...) {
    Printf("--> ");
    char line[MAX_CHAR_A_LINE];
    va_list ap;
    va_start(ap, fmt);
    vsprintf_s(line, MAX_CHAR_A_LINE, fmt, ap);
    Printf(line);
    Printf("\n");
}

INLINE void PrintSub(void) {

}

#ifdef _DEBUG

    #define STRINGIFY(x) #x
    #define STRINGIFY_BUILTIN(x) STRINGIFY(x)
    #define ASSERT_PRINT( isFalse, ... ) \
        if (!(bool)(isFalse)) { \
            Print("\nAssertion failed in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
            PrintSub("\'" #isFalse "\' is false"); \
            PrintSub(__VA_ARGS__); \
            Print("\n"); \
            __debugbreak(); \
        }

    #define ASSERT_SUCCEEDED( hr, ... ) \
        if (FAILED(hr)) { \
            Print("\nHRESULT failed in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
            PrintSub("hr = 0x%08X", hr); \
            PrintSub(__VA_ARGS__); \
            Print("\n"); \
            __debugbreak(); \
        }


    #define WARN_ONCE_IF( isTrue, ... ) \
        { \
            static bool s_TriggeredWarning = false; \
            if ((bool)(isTrue) && !s_TriggeredWarning) { \
                s_TriggeredWarning = true; \
                Print("\nWarning issued in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
                PrintSub("\'" #isTrue "\' is true"); \
                PrintSub(__VA_ARGS__); \
                Print("\n"); \
            } \
        }

    #define WARN_ONCE_IF_NOT( isTrue, ... ) WARN_ONCE_IF(!(isTrue), __VA_ARGS__)

    #define ERROR_PRINT( ... ) \
        Print("\nError reported in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
        PrintSub(__VA_ARGS__); \
        Print("\n");

    #define DEBUG_PRINT( msg, ... ) \
        Print( msg "\n", ##__VA_ARGS__ );

#else

    #define ASSERT_PRINT( isTrue, ... ) (void)(isTrue)
    #define WARN_ONCE_IF( isTrue, ... ) (void)(isTrue)
    #define WARN_ONCE_IF_NOT( isTrue, ... ) (void)(isTrue)
    #define ERROR_PRINT( msg, ... )
    #define DEBUG_PRINT( msg, ... ) do {} while(0)
    #define ASSERT_SUCCEEDED( hr, ... ) (void)(hr)

#endif // _DEBUG

#define DeleteAndSetNull( obj ) if (obj) { delete obj; obj = nullptr; }

#define DeleteArrayAndSetNull( obj ) if (obj) { delete [] obj; obj = nullptr; }

#define DeleteArray( obj ) if (obj) { delete [] obj; }

#define ReleaseAndSetNull( obj ) if (obj) { obj->Release(); obj = nullptr; }

#define BreakIfFailed( hr ) if (FAILED(hr)) __debugbreak()

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
