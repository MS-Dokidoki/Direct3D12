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
    double dSecondPerCount; // ʱ��ת������
    double dDeltaTime;      // ��һ�� Tick �뵱ǰ Tick ��ʱ����

    INT64 i64PausedTime;    // ��ͣ����ʱ��
    INT64 i64BaseFrame;     // Reset ��ʱ����ʱ��֡
    INT64 i64StopFrame;     // Stop ��ʱ����ʱ��֡
    INT64 i64PrevFrame;     // �����ڶ��� Tick ��ʱ��֡
    INT64 i64CurrFrame;     // ���һ�� Tick ��ʱ��֡

    bool bStopped;          // ������״̬
};
