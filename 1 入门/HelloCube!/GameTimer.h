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
    double dSecondPerCount; // 时间转换因子
    double dDeltaTime;      // 上一次 Tick 与当前 Tick 的时间间隔

    INT64 i64PausedTime;    // 暂停的总时长
    INT64 i64BaseFrame;     // Reset 计时器的时间帧
    INT64 i64StopFrame;     // Stop 计时器的时间帧
    INT64 i64PrevFrame;     // 倒数第二次 Tick 的时间帧
    INT64 i64CurrFrame;     // 最后一次 Tick 的时间帧

    bool bStopped;          // 计数器状态
};
