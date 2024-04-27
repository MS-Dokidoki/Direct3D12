#include "GameTimer.h"

GameTimer::GameTimer() : 
dSecondPerCount(0.0), dDeltaTime(-1.0), i64BaseFrame(0),
i64PausedTime(0), i64CurrFrame(0), i64PrevFrame(0),
i64StopFrame(0), bStopped(0)
{
    INT64 countsPerSec;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
    dSecondPerCount = 1.0 / (double)countsPerSec;
}

void GameTimer::Tick()
{
    if(bStopped)
    {
        dDeltaTime = 0;
        return;
    }
    INT64 currFrame;
    QueryPerformanceCounter((LARGE_INTEGER*)&currFrame);
    i64CurrFrame = currFrame;

    dDeltaTime = (i64CurrFrame - i64PrevFrame) *  dSecondPerCount;

    i64PrevFrame = i64CurrFrame;
    if(dDeltaTime < 0.0)
        dDeltaTime = 0;
}

float GameTimer::DeltaTime() const
{
    return (float)dDeltaTime;
}

void GameTimer::Reset()
{
    INT64 currFrame;
    QueryPerformanceCounter((LARGE_INTEGER*)&currFrame);

    i64BaseFrame = currFrame;
    i64PrevFrame = currFrame;
    i64StopFrame = 0;
    bStopped = 0;
}

void GameTimer::Stop()
{
    if(!bStopped)
    {
        INT64 currFrame;
        QueryPerformanceCounter((LARGE_INTEGER*)&currFrame);

        i64StopFrame = currFrame;
        bStopped = 1;
    }
}

void GameTimer::Start()
{

    if(bStopped)
    {
        INT64 startFrame;
        QueryPerformanceCounter((LARGE_INTEGER*)&startFrame);
        
        i64PausedTime += (startFrame - i64StopFrame);
        i64PrevFrame = startFrame;
        i64CurrFrame = startFrame;
        i64StopFrame = 0;
        bStopped = 0;
    }
}

float GameTimer::TotalTime() const
{
    if(bStopped)
    {
        return (float)(((i64StopFrame - i64PausedTime) - i64BaseFrame) * dSecondPerCount);
    }
    else
    {
        return (float)(((i64CurrFrame - i64PausedTime) - i64BaseFrame) * dSecondPerCount);
    }
}