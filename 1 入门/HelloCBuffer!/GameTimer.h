#pragma once
#include <windows.h>

class GameTimer
{
public:
    GameTimer();

    float TotalTime() const;
    float DeltaTime() const;

    void Reset();
    void Start();
    void Stop();
    void Tick();
private:
    double mSecondPerCount;
    double mDeltaTime;

    INT64 mBaseFrame;
    INT64 mPausedTime;
    INT64 mStopFrame;
    INT64 mPrevFrame;
    INT64 mCurrFrame;

    bool bStopped;
};
