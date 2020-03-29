#pragma once

/*
������ �ð��� �Լ��� �����ϴ� Ŭ����
*/
class Timer
{
public:
	Timer() = default;
	~Timer() = default;

public:
	// �ش� �ð��� �����ϸ� �Լ��� �����ϴ� Ÿ�̸Ӹ� �����Ѵ�.
	template<typename function>
	void SetTimeout(function function, int delay); // milisecond ����

	// interval �ð� ���� ���� �Լ��� �ݺ������� �����ϴ�
	// Ÿ�̸Ӹ� �����Ѵ�.
	template<typename function>
	void SetInterval(function function, int interval); // milisecond ����

	void Stop();
	bool IsClear();

private:
	// Ÿ�̸Ӱ� ��� ������ ������ �Ǵ��Ѵ�.
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