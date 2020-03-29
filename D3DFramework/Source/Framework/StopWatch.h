#pragma once

#include <chrono>

/*
�ð��� �����ϴ� Ŭ����
*/
class StopWatch
{
public:
	// �ð� ������ �����Ѵ�.
	void Start() { start = std::chrono::system_clock::now(); }
	// �ð� ������ ����ģ��.
	void End() { end = std::chrono::system_clock::now(); }

	// ������ �ð��� �� ������ ��ȯ�Ѵ�.
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