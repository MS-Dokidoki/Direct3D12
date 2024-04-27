#include "GameTimer.h"

GameTimer::GameTimer() : mSecondPerCount(0), mDeltaTime(-1), mBaseFrame(0),
mPausedTime(0), mPrevFrame(0), mCurrFrame(0)
{
    INT64 countsPerSec;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
    mSecondPerCount = 1.0 / (double)countsPerSec;
}

void GameTimer::Tick()
{
    if(bStopped)
    {
        mDeltaTime = 0;
        return;
    }
    INT64 currFrame;
    QueryPerformanceCounter((LARGE_INTEGER*)&currFrame);
    mCurrFrame = currFrame;

    mDeltaTime = (mCurrFrame - mPrevFrame) *  mSecondPerCount;

    mPrevFrame = mCurrFrame;
    if(mDeltaTime < 0.0)
        mDeltaTime = 0;
}

float GameTimer::DeltaTime() const
{
    return (float)mDeltaTime;
}

void GameTimer::Reset()
{
    INT64 currFrame;
    QueryPerformanceCounter((LARGE_INTEGER*)&currFrame);

    mBaseFrame = currFrame;
    mPrevFrame = currFrame;
    mStopFrame = 0;
    bStopped = 0;
}

void GameTimer::Stop()
{
    if(!bStopped)
    {
        INT64 currFrame;
        QueryPerformanceCounter((LARGE_INTEGER*)&currFrame);

        mStopFrame = currFrame;
        bStopped = 1;
    }
}

void GameTimer::Start()
{
    INT64 startFrame;
    QueryPerformanceCounter((LARGE_INTEGER*)&startFrame);

    if(bStopped)
    {
        mPausedTime += (startFrame - mStopFrame);
        mPrevFrame = startFrame;
        mStopFrame = 0;
        bStopped = 0;
    }
}

float GameTimer::TotalTime() const
{
    if(bStopped)
    {
        return (float)(((mStopFrame - mPausedTime) - mBaseFrame) * mSecondPerCount);
    }
    else
    {
        return (float)(((mCurrFrame - mPausedTime) - mBaseFrame) * mSecondPerCount);
    }
}