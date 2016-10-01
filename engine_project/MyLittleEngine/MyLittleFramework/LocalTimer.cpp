#include "stdafx.h"
#include "LocalTimer.h"
#include "LuaVM.h"

using namespace EngineCore;

void LocalTimer::RegLuaClass()
{
	getGlobalNamespace(LSTATE)
		.beginClass<LocalTimer>("LTimer")
			.addConstructor<void (*)(void)>()
			.addProperty("dt", &LocalTimer::dt)
			.addProperty("time", &LocalTimer::GetTime)
			.addFunction("IsRun", &LocalTimer::IsRun)
			.addFunction("Start", &LocalTimer::Start)
			.addFunction("Stop", &LocalTimer::Stop)
			.addFunction("SetAutoStop", &LocalTimer::SetAutoStop)
			.addFunction("SetScale", &LocalTimer::SetScale)
		.endClass();
}

LocalTimer::LocalTimer()
{
	m_scale = 1.0f;
	m_time = 0;
	m_stoptime = -1;
	b_run = false;
}

float LocalTimer::dt() const
{
	return Timer::Get()->dt_ms * m_scale;
}

void LocalTimer::Frame()
{
	if(!b_run)
		return;
	m_time += Timer::Get()->dt_ms * m_scale;
	if(m_time >= m_stoptime && m_stoptime != -1)
	{
		Stop();
	}
}

void LocalTimer::Start()
{
	m_time = 0;
	b_run = true;
}

void LocalTimer::Stop()
{
	b_run = false;
}

void LocalTimer::SetAutoStop(float time)
{
	m_stoptime = time;
}

bool LocalTimer::IsRun() const
{
	return b_run;
}

float LocalTimer::GetTime() const
{
	return m_time;
}