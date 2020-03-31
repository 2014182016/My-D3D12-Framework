//***************************************************************************************
// GameTimer.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "GameTimer.h"
#include <Windows.h>

GameTimer::GameTimer()
: mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), 
  mPausedTime(0), mPrevTime(0), mCurrentTime(0), mStopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

// Reset이 호출된 이후 흐른시간에서 일시 정지된 시간을 제외한 시간을 돌려준다.
float GameTimer::GetTotalTime()const
{
	// 타이머가 정지 상태이면, 정지된 시점부터 흐른 시간은 계산하지 말아야 한다.
	// 또한, 이전에 이미 일시 정지된 적이 있다면 시간차 mStopTime - mBasetime에는
	// 일시 정지 누적 시간이 포함되어 잇는데, 그 누적 시간은 전체 시간에 포함하지
	// 말아야 한다. 이를 바로잡기 위해, mStopTime에서 일시 정지 누적 시간을 뺀다.
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime

	if( mStopped )
	{
		return (float)(((mStopTime - mPausedTime)-mBaseTime)*mSecondsPerCount);
	}

	// 시간차 mCurrentTime - mBaseTime에는 일시 정지 누적 시간이 포함되어 있다.
	// 이를 전체 시간에 포함하면 안 되므로, 그 시간을 mCurrTime에서 뺀다.
	//  (mCurrTime - mPausedTime) - mBaseTime 
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mCurrTime
	
	else
	{
		return (float)(((mCurrentTime -mPausedTime)-mBaseTime)*mSecondsPerCount);
	}
}

float GameTimer::GetDeltaTime()const
{
	return (float)mDeltaTime;
}

void GameTimer::Reset()
{
	INT64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped  = false;
}

void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);


	// 정지(일시 정지)와 시작(재개) 사이에 흐른 시간을 누적한다.
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     

	// 정지 상태에서 타이머를 재개하는 경우
	if( mStopped )
	{
		// 일시 정지된 시간을 누적한다.
		mPausedTime += (startTime - mStopTime);	

		// 타이머를 다시 시작하는 것이므로, 현재의 mPrevtime(이전 시간)은
		// 유효하지 않다(일시 정지 도중에 갱신되었을 것이므로).
		// 따라서 현재 시간으로 다시 설정한다.
		mPrevTime = startTime;

		// 이제는 정지 상태가 아니므로 관련 멤버들을 갱신한다.
		mStopTime = 0;
		mStopped  = false;
	}
}

void GameTimer::Stop()
{
	// 정지 상태이면 아무 일도 하지 않는다.
	if( !mStopped )
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		// 그렇지 않다면 현재 시간을 타이머 정지 시점 시간으로 저장하고,
		// 타이머가 정지되었음을 뜻하는 부울 플래그를 설정한다.
		mStopTime = currTime;
		mStopped  = true;
	}
}

void GameTimer::Tick()
{
	if( mStopped )
	{
		mDeltaTime = 0.0;
		return;
	}

	// 이번 프레임의 시간을 얻는다.
	INT64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrentTime = currTime;

	// 이번 프레임의 시간과 이전 프레임의 시간의 차이를 구한다.
	mDeltaTime = (mCurrentTime - mPrevTime)*mSecondsPerCount;

	// 다음 프레임을 준비한다.
	mPrevTime = mCurrentTime;

	// 음수가 되지 않게 한다. SDK 문서화이 CDXUTTimer 항목에 따르면,
	// 프로세서가 절전 모드에 들어가거나 실행이 되는 다른 프로세서와
	// 엉키는 경우 mDeltaTime이 음수가 될 수 있다.
	if(mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}

