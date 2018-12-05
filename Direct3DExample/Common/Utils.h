#pragma once

#include <exception>

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

extern void LogOutput(const char *fmt, ...);

