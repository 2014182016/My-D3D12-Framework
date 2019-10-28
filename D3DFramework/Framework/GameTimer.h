//***************************************************************************************
// GameTimer.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#ifndef GAMETIMER_H
#define GAMETIMER_H

class GameTimer
{
public:
	GameTimer();

	float GetTotalTime()const; // 초 단위
	float GetDeltaTime()const; // 초 단위

	void Reset(); // Loop 이전에 부른다.
	void Start(); // 시작할 때, 부른다.
	void Stop();  // 정지할 때, 부른다.
	void Tick();  // 매 프레임마다 부른다.

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