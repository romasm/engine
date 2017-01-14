#include "stdafx.h"
#include "Timer.h"
#include "Log.h"
#include "LuaVM.h"

using namespace EngineCore;

Timer* Timer::m_instance = nullptr;

Timer::Timer()
{
	if(!m_instance)
	{
		if(Init())
			m_instance = this;
	}
	else
		ERR("Попытка повторного создания глобального таймера");
}

Timer::~Timer()
{
	m_instance = nullptr;
}

static float Get_dt(){return Timer::Get()->dt_ms;}
static double Get_time(){return Timer::Get()->TimeSinceStart_ms;}

void Timer::RegLuaClass()
{
	getGlobalNamespace(LSTATE)
		.addFunction("Get_dt", &Get_dt)
		.addFunction("Get_time", &Get_time);
}

bool Timer::Init()
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&m_frequency);
	if(m_frequency == 0)
		return false;

	m_ticksPerMs = (float)m_frequency / 1000;

	QueryPerformanceCounter((LARGE_INTEGER*)&m_startTime);
	TimeSinceStart_ms = 0;
	TimeSinceStart_tick = 0;
	dt_ms = 0;
	dt_tick = 0;

	return true;
}

void Timer::Frame()
{
	INT64 currentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);

	dt_tick = currentTime - m_startTime;
	dt_ms = float(dt_tick) / m_ticksPerMs;

	TimeSinceStart_ms += double(dt_ms);
	TimeSinceStart_tick += dt_tick;

	m_startTime = currentTime;
}