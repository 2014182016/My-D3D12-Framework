#pragma once

#include <chrono>

class StopWatch
{
public:
	void Start() { start = std::chrono::system_clock::now(); }
	void End() { end = std::chrono::system_clock::now(); }

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