#include "Time.h"
#include <chrono>



static f32 _dt;

static std::chrono::steady_clock::time_point _prev = std::chrono::high_resolution_clock::now();

void Time::Update()
{
	auto now = std::chrono::high_resolution_clock::now();
	 _dt = std::chrono::duration<float>(now - _prev).count();
	_prev = now;
}

f64 Time::GetDeltaTime()
{
	return _dt;
}
