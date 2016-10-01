#pragma once
#include "Log.h"
#include "macros.h"
#include "Util.h"
#include "Timer.h"
#include "DataAlloc\Arrays.h"
#include "JobSystem.h"

#include "ProfilerIDs.h"

#define PERF_FRAMES_DUMP 60 * 60 * 1 // 1 minute

#define PROFILER Profiler::Get()

#ifdef _DEV
	#define PERF_CPU(param) Profiler::CPU_TimeSlice(PERF_CPU##param##_ID)
#else
	#define PERF_CPU(param)
#endif

namespace EngineCore
{
	class Profiler
	{

#undef max

	public:
		Profiler();
		~Profiler();

		static Profiler* Get(){return instance;}

		inline void CPU_BeginFrame()
		{
			if(!started)
				return;

			currentFrameID++;

			if(currentFrameID >= PERF_FRAMES_DUMP)
			{
				Dump();
				currentFrameID = 0;
			}
		}
		
		inline static void CPU_TimeSlice( uint32_t id )
		{
			if(!instance || !instance->started)
				return;

			uint32_t& th = instance->thread_map[JobSystem::GetThreadHash()];
			instance->perf_data[id][th][instance->currentFrameID] = Timer::ForcedGetCurrentTime();
		}

		inline void Stop() {started = false;}
		inline void Start() {started = true;}
		inline bool IsRunning() {return started;}

		void Dump();

	private:
		static Profiler* instance;
		bool started;

		uint32_t currentFrameID;

		unordered_map<size_t, uint32_t> thread_map;

		RArray<
			RArray<
				SArray<double, PERF_FRAMES_DUMP>
				/*frames*/>
			/*threads*/>
		/*params*/ perf_data; // 8 bytes * threads(9) per frame per param
	};
}