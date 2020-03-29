#pragma once

#include <chrono>

/*
시간을 측정하는 클래스
*/
class StopWatch
{
public:
	// 시간 측정을 시작한다.
	void Start() { start = std::chrono::system_clock::now(); }
	// 시간 측정을 끝마친다.
	void End() { end = std::chrono::system_clock::now(); }

	// 측정된 시간을 각 단위로 변환한다.
	long long Nanosecond() const { return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count(); }
	long long Microsecond() const { return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count(); }
	long long Millisecond() const { return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(); }
	long long Second() const { return std::chrono::duration_cast<std::chrono::seconds>(end - start).count(); }
	long long Minute() const { return std::chrono::duration_cast<std::chrono::minutes>(end - start).count(); }
	long long Hour() const { return std::chrono::duration_cast<std::chrono::hours>(end - start).count(); }

private:
	std::chrono::system_clock::time_point start;
	std::chrono::system_clock::time_point end;
};