#pragma once
#include "stdafx.h"

namespace EngineCore
{
//------------------------------------------------------------------

	class Timer
	{
	public:
		Timer();
		~Timer();

		inline static Timer* Get(){return m_instance;}

		bool Init();
		void Frame();

		inline static double ForcedGetCurrentTime()
		{
			INT64 currentTime;
			QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
			return double(currentTime) / m_instance->m_ticksPerMs;
		}

		inline static INT64 ForcedGetCurrentTick()
		{
			INT64 currentTime;
			QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
			return currentTime;
		}

		float dt_ms;
		double TimeSinceStart_ms;

		INT64 dt_tick;
		INT64 TimeSinceStart_tick;

		static void RegLuaClass();

	private:
		static Timer* m_instance;

		INT64 m_frequency;
		INT64 m_startTime;
		float m_ticksPerMs;
	};

//------------------------------------------------------------------
}