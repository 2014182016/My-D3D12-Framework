//***************************************************************************************
// GameTimer.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include <Framework/GameTimer.h>
#include <Windows.h>

GameTimer::GameTimer()
: mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), 
  mPausedTime(0), mPrevTime(0), mCurrentTime(0), mStopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

// Reset�� ȣ��� ���� �帥�ð����� �Ͻ� ������ �ð��� ������ �ð��� �����ش�.
float GameTimer::GetTotalTime()const
{
	// Ÿ�̸Ӱ� ���� �����̸�, ������ �������� �帥 �ð��� ������� ���ƾ� �Ѵ�.
	// ����, ������ �̹� �Ͻ� ������ ���� �ִٸ� �ð��� mStopTime - mBasetime����
	// �Ͻ� ���� ���� �ð��� ���ԵǾ� �մµ�, �� ���� �ð��� ��ü �ð��� ��������
	// ���ƾ� �Ѵ�. �̸� �ٷ���� ����, mStopTime���� �Ͻ� ���� ���� �ð��� ����.
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime

	if( mStopped )
	{
		return (float)(((mStopTime - mPausedTime)-mBaseTime)*mSecondsPerCount);
	}

	// �ð��� mCurrentTime - mBaseTime���� �Ͻ� ���� ���� �ð��� ���ԵǾ� �ִ�.
	// �̸� ��ü �ð��� �����ϸ� �� �ǹǷ�, �� �ð��� mCurrTime���� ����.
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


	// ����(�Ͻ� ����)�� ����(�簳) ���̿� �帥 �ð��� �����Ѵ�.
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     

	// ���� ���¿��� Ÿ�̸Ӹ� �簳�ϴ� ���
	if( mStopped )
	{
		// �Ͻ� ������ �ð��� �����Ѵ�.
		mPausedTime += (startTime - mStopTime);	

		// Ÿ�̸Ӹ� �ٽ� �����ϴ� ���̹Ƿ�, ������ mPrevtime(���� �ð�)��
		// ��ȿ���� �ʴ�(�Ͻ� ���� ���߿� ���ŵǾ��� ���̹Ƿ�).
		// ���� ���� �ð����� �ٽ� �����Ѵ�.
		mPrevTime = startTime;

		// ������ ���� ���°� �ƴϹǷ� ���� ������� �����Ѵ�.
		mStopTime = 0;
		mStopped  = false;
	}
}

void GameTimer::Stop()
{
	// ���� �����̸� �ƹ� �ϵ� ���� �ʴ´�.
	if( !mStopped )
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		// �׷��� �ʴٸ� ���� �ð��� Ÿ�̸� ���� ���� �ð����� �����ϰ�,
		// Ÿ�̸Ӱ� �����Ǿ����� ���ϴ� �ο� �÷��׸� �����Ѵ�.
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

	// �̹� �������� �ð��� ��´�.
	INT64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrentTime = currTime;

	// �̹� �������� �ð��� ���� �������� �ð��� ���̸� ���Ѵ�.
	mDeltaTime = (mCurrentTime - mPrevTime)*mSecondsPerCount;

	// ���� �������� �غ��Ѵ�.
	mPrevTime = mCurrentTime;

	// ������ ���� �ʰ� �Ѵ�. SDK ����ȭ�� CDXUTTimer �׸� ������,
	// ���μ����� ���� ��忡 ���ų� ������ �Ǵ� �ٸ� ���μ�����
	// ��Ű�� ��� mDeltaTime�� ������ �� �� �ִ�.
	if(mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}

