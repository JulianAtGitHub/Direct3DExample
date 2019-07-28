#pragma once

class CTimer {
public:
    CTimer(void)
    : mElapsedTicks(0)
    , mTotalTicks(0)
    , mLeftOverTicks(0)
    , mIsFixedTimeStep(false)
    , mTargetElapsedTicks(TicksPerSecond / 60)
    , mFrameCount(0)
    , mFramesPerSecond(0)
    , mFramesThisSecond(0)
    , mSecondCounter(0)
    {
        QueryPerformanceFrequency(&mFrequency);
        QueryPerformanceCounter(&mLastTime);
        mMaxDelta.QuadPart = mFrequency.QuadPart / 10;
    }

    ~CTimer(void) {
    }

    // Get elapsed time since the previous Update call.
    INLINE uint64_t GetElapsedTicks(void) const { return mElapsedTicks; }
    INLINE double GetElapsedSeconds(void) const { return static_cast<double>(mElapsedTicks) / TicksPerSecond; }

    // Get total time since the start of the program.
    INLINE uint64_t GetTotalTicks(void) const { return mTotalTicks; }
    INLINE double GetTotalSeconds(void) const { return static_cast<double>(mTotalTicks) / TicksPerSecond; }

    // Get total number of updates since start of the program.
    INLINE uint32_t GetFrameCount(void) const { return mFrameCount; }

    // Get the current framerate.
    INLINE uint32_t GetFramesPerSecond() const { return mFramesPerSecond; }

    // Set whether to use fixed or variable timestep mode.
    INLINE void SetFixedTimeStep(bool isFixedTimestep) { mIsFixedTimeStep = isFixedTimestep; }

    // Set how often to call Update when in fixed timestep mode.
    INLINE void SetTargetElapsedTicks(uint64_t targetElapsed) { mTargetElapsedTicks = targetElapsed; }

    INLINE void Reset(void) {
        QueryPerformanceCounter(&mLastTime);
        mLeftOverTicks = 0;
        mFramesPerSecond = 0;
        mFramesThisSecond = 0;
        mSecondCounter = 0;
    }

    void Tick(void) {
        // Query the current time.
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        int64_t timeDelta = currentTime.QuadPart - mLastTime.QuadPart;

        mLastTime = currentTime;
        mSecondCounter += timeDelta;

        // Clamp excessively large time deltas (e.g. after paused in the debugger).
        if (timeDelta > mMaxDelta.QuadPart) {
            timeDelta = mMaxDelta.QuadPart;
        }

        // Convert QPC units into a canonical tick format. This cannot overflow due to the previous clamp.
        timeDelta *= TicksPerSecond;
        timeDelta /= mFrequency.QuadPart;

        uint32_t lastFrameCount = mFrameCount;

        if (mIsFixedTimeStep) {
            // If the app is running very close to the target elapsed time (within 1/4 of a millisecond) just clamp
            // the clock to exactly match the target value. This prevents tiny and irrelevant errors
            // from accumulating over time. Without this clamping, a game that requested a 60 fps
            // fixed update, running with vsync enabled on a 59.94 NTSC display, would eventually
            // accumulate enough tiny errors that it would drop a frame. It is better to just round 
            // small deviations down to zero to leave things running smoothly.

            if (_abs64(timeDelta - mTargetElapsedTicks) < TicksPerSecond / 4000) {
                timeDelta = mTargetElapsedTicks;
            }

            mLeftOverTicks += timeDelta;

            while (mLeftOverTicks >= mTargetElapsedTicks) {
                mElapsedTicks = mTargetElapsedTicks;
                mTotalTicks += mTargetElapsedTicks;
                mLeftOverTicks -= mTargetElapsedTicks;
                ++ mFrameCount;
            }
        } else {
            // Variable timestep update logic.
            mElapsedTicks = timeDelta;
            mTotalTicks += timeDelta;
            mLeftOverTicks = 0;
            ++ mFrameCount;
        }

        // Track the current framerate.
        if (mFrameCount != lastFrameCount) {
            ++ mFramesThisSecond;
        }

        if (mSecondCounter >= mFrequency.QuadPart)
        {
            mFramesPerSecond = mFramesThisSecond;
            mFramesThisSecond = 0;
            mSecondCounter %= mFrequency.QuadPart;
        }
    }

private:
    // Integer format represents time using 10,000,000 ticks per second.
    static const int64_t TicksPerSecond = 10000000;

    // Source timing data uses QPC units.
    LARGE_INTEGER   mFrequency;
    LARGE_INTEGER   mLastTime;
    LARGE_INTEGER   mMaxDelta;

    // Derived timing data uses a canonical tick format.
    uint64_t        mElapsedTicks;
    uint64_t        mTotalTicks;
    int64_t         mLeftOverTicks;

    // Members for configuring fixed timestep mode.
    bool            mIsFixedTimeStep;
    int64_t         mTargetElapsedTicks;

    // Members for tracking the framerate.
    uint32_t        mFrameCount;
    uint32_t        mFramesPerSecond;
    uint32_t        mFramesThisSecond;
    int64_t         mSecondCounter;
};