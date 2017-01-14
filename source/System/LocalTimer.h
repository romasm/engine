#pragma once
#include "stdafx.h"
#include "Timer.h"

namespace EngineCore
{
//------------------------------------------------------------------

	class LocalTimer
	{
	public:
		LocalTimer();

		inline float dt() const;

		inline void SetScale(float scale)
		{
			m_scale = scale;
		}

		void Frame();

		inline void Start();
		inline void Stop();
		// ������������� - ��������� ���������
		inline void SetAutoStop(float time);

		inline bool IsRun() const;
		inline float GetTime() const;

		static void RegLuaClass();

	private:
		float m_time;
		bool b_run;
		float m_stoptime;
	
		float m_scale;
	};

//------------------------------------------------------------------
}