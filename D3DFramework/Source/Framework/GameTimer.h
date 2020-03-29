//***************************************************************************************
// GameTimer.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#pragma once

#include <basetsd.h>

/*
������ �ð��� �����Ѵ�.
������ ���۵� ������ �ð� ������ �����ϸ� ������ �� �ð� ��,
������ ������ ���� �ð�, ���� ���� ���� ������ �Ѵ�.
*/
class GameTimer
{
public:
	GameTimer();

	float GetTotalTime() const; // �� ����
	float GetDeltaTime() const; // �� ����

	void Reset(); // Loop ������ �θ���.
	void Start(); // ������ ��, �θ���.
	void Stop();  // ������ ��, �θ���.
	void Tick();  // �� �����Ӹ��� �θ���.

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