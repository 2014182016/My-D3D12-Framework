#pragma once

/*
설정한 시간에 함수를 수행하는 클래스
*/
class Timer
{
public:
	Timer() = default;
	~Timer() = default;

public:
	// 해당 시간에 도달하면 함수를 수행하는 타이머를 설정한다.
	template<typename function>
	void SetTimeout(function function, int delay); // milisecond 단위

	// interval 시간 간격 동안 함수를 반복적으로 수행하는
	// 타이머를 설정한다.
	template<typename function>
	void SetInterval(function function, int interval); // milisecond 단위

	void Stop();
	bool IsClear();

private:
	// 타이머가 계속 수행할 것인지 판단한다.
	bool clear = false;
};

template<typename function>
void Timer::SetTimeout(function function, int delay)
{
	clear = false;

	std::thread t([=]() {
		if (this->IsClear()) return;
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
		if (this->IsClear()) return;
		function();
	});
	t.detach();
}


template<typename function>
void Timer::SetInterval(function function, int interval)
{
	clear = false;

	std::thread t([=]() {
		while (true) {
			if (this->IsClear()) return;
			std::this_thread::sleep_for(std::chrono::milliseconds(interval));
			if (this->IsClear()) return;
			function();
		}
	});
	t.detach();
}

void Timer::Stop()
{
	clear = false;
}

bool Timer::IsClear()
{
	return clear;
}