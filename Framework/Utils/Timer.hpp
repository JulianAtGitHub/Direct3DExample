#pragma once

namespace Utils {

class Timer {
public:
    Timer(void)
    : mElapsedTicks(0)
    , mTotalTicks(0)
    {
        QueryPerformanceFrequency(&mFrequency);
        QueryPerformanceCounter(&mLastTime);
        mMaxDelta.QuadPart = mFrequency.QuadPart / 10;
    }

    ~Timer(void) {
    }

    // Get elapsed time since the previous Update call.
    INLINE uint64_t GetElapsedTicks(void) const { return mElapsedTicks; }
    INLINE double GetElapsedSeconds(void) const { return static_cast<double>(mElapsedTicks) / TicksPerSecond; }

    // Get total time since the start of the program.
    INLINE uint64_t GetTotalTicks(void) const { return mTotalTicks; }
    INLINE double GetTotalSeconds(void) const { return static_cast<double>(mTotalTicks) / TicksPerSecond; }

    INLINE void Reset(void) {
        QueryPerformanceCounter(&mLastTime);
        mTotalTicks = 0;
    }

    void Tick(void) {
        // Query the current time.
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        int64_t timeDelta = currentTime.QuadPart - mLastTime.QuadPart;

        mLastTime = currentTime;

        // Clamp excessively large time deltas (e.g. after paused in the debugger).
        if (timeDelta > mMaxDelta.QuadPart) {
            timeDelta = mMaxDelta.QuadPart;
        }

        // Convert QPC units into a canonical tick format. This cannot overflow due to the previous clamp.
        timeDelta *= TicksPerSecond;
        timeDelta /= mFrequency.QuadPart;

        mElapsedTicks = timeDelta;
        mTotalTicks += timeDelta;
    }

private:
    // Integer format represents time using 1,000,000 ticks per second.
    static const int64_t TicksPerSecond = 1000000;

    // Source timing data uses QPC units.
    LARGE_INTEGER   mFrequency;
    LARGE_INTEGER   mLastTime;
    LARGE_INTEGER   mMaxDelta;

    // Derived timing data uses a canonical tick format.
    uint64_t        mElapsedTicks;
    uint64_t        mTotalTicks;
};

}