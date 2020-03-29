//***************************************************************************************
// GameTimer.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#pragma once

#include <basetsd.h>

/*
게임의 시간을 측정한다.
게임이 시작될 때부터 시간 측정을 시작하며 프레임 간 시간 차,
게임이 시작한 후의 시간, 게임 정지 등의 역할을 한다.
*/
class GameTimer
{
public:
	GameTimer();

	float GetTotalTime() const; // 초 단위
	float GetDeltaTime() const; // 초 단위

	void Reset(); // Loop 이전에 부른다.
	void Start(); // 시작할 때, 부른다.
	void Stop();  // 정지할 때, 부른다.
	void Tick();  // 매 프레임마다 부른다.

private:
	double mSecondsPerCount;
	double mDeltaTime;

	INT64 mBaseTime;
	INT64 mPausedTime;
	INT64 mStopTime;
	INT64 mPrevTime;
	INT64 mCurrentTime;

	bool mStopped;
};