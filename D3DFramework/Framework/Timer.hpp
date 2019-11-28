#pragma once

#include "pch.h"

class Timer
{
public:
	Timer() = default;
	~Timer() = default;

public:
	template<typename function>
	void SetTimeout(function function, int delay); // milisecond 단위

	template<typename function>
	void SetInterval(function function, int interval); // milisecond 단위

	void Stop() { mClear = true; }

public:
	inline bool GetIsClear() const { return mClear; }

private:
	bool mClear = false;
};

template<typename function>
void Timer::SetTimeout(function function, int delay)
{
	mClear = false;

	std::thread t([=]() {
		if (this->GetIsClear()) return;
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
		if (this->GetIsClear()) return;
		function();
	});
	t.detach();
}


template<typename function>
void Timer::SetInterval(function function, int interval)
{
	mClear = false;

	std::thread t([=]() {
		while (true) {
			if (this->GetIsClear()) return;
			std::this_thread::sleep_for(std::chrono::milliseconds(interval));
			if (this->GetIsClear()) return;
			function();
		}
	});
	t.detach();
}