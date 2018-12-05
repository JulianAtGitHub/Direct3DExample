#include "pch.h"
#include "Utils.h"

void LogOutput(const char *fmt, ...) {
    static char out[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(out, 512, fmt, ap);
    va_end(ap);

#ifdef _DEBUG
    OutputDebugString(out);
#endif
    printf("%s", out);
}