//***************************************************************************************
// GameTimer.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#ifndef GAMETIMER_H
#define GAMETIMER_H

class GameTimer
{
public:
	GameTimer();

	float GetTotalTime()const; // �� ����
	float GetDeltaTime()const; // �� ����

	void Reset(); // Loop ������ �θ���.
	void Start(); // ������ ��, �θ���.
	void Stop();  // ������ ��, �θ���.
	void Tick();  // �� �����Ӹ��� �θ���.

private:
	double mSecondsPerCount;
	double mDeltaTime;

	__int64 mBaseTime;
	__int64 mPausedTime;
	__int64 mStopTime;
	__int64 mPrevTime;
	__int64 mCurrentTime;

	bool mStopped;
};

#endif // GAMETIMER_H