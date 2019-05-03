#pragma once

inline char * WStr2Str(const wchar_t *wstr) {
    if (!wstr) { return nullptr; }
    size_t len = wcslen(wstr) * 2 + 1;
    char *str = (char *)malloc(len);
    size_t converted = 0;
    wcstombs_s(&converted, str, len, wstr, _TRUNCATE );
    return str;
}

struct CHException : public std::exception {
    CHException(HRESULT hr) {
        sprintf(mStr, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    }
    char mStr[64];
};

inline void ThrowIfFailed(HRESULT hr) {
    if (FAILED(hr)) {
        throw CHException(hr);
    }
}

inline void ReleaseIfNotNull(IUnknown *obj) {
    if (obj) {
        obj->Release();
    }
}

inline void Printf(const char *str) {
#ifdef _DEBUG
    OutputDebugString(str);
#else
    printf("%s", str);
#endif
}

#define MAX_CHAR_A_LINE 256 

inline void Print(const char *fmt, ...) {
    char line[MAX_CHAR_A_LINE];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, MAX_CHAR_A_LINE, fmt, ap);
    va_end(ap);
    Printf(line);
}

inline void PrintSub(const char* fmt, ...)
{
    Printf("--> ");
    char line[MAX_CHAR_A_LINE];
    va_list ap;
    va_start(ap, fmt);
    vsprintf_s(line, MAX_CHAR_A_LINE, fmt, ap);
    Printf(line);
    Printf("\n");
}

inline void PrintSub(void) {

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

#define BreakIfFailed( hr ) if (FAILED(hr)) __debugbreak()

