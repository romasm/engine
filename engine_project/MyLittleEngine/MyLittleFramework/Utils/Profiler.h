#pragma once
#include "Log.h"
#include "macros.h"
#include "Util.h"
#include "Timer.h"
#include "DataAlloc\Arrays.h"
#include "JobSystem.h"

#include "ProfilerIDs.h"

#define PERF_FRAMES_DUMP 60 * 60 * 1 // 1 minute
#define PERF_FRAMES_OFFSET 5

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

			frameBegin[currentFrameID] = Timer::ForcedGetCurrentTime();
		}
		
		inline static void CPU_TimeSlice( uint32_t id )
		{
			if(!instance || !instance->started)
				return;

			uint32_t& th = instance->thread_map[JobSystem::GetThreadHash()];
			instance->perf_data[id][th][instance->currentFrameID] = Timer::ForcedGetCurrentTime();
		}

		void Stop() {started = false;}
		void Start() {started = true;}
		inline bool IsRunning() {return started;}

		void SetDumping(bool dump) {dumping = dump;}
		bool GetDumping() const {return dumping;}

		uint32_t GetIDsCount() const {return PERF_IDS_COUNT;}
		string GetIDName(uint32_t id) const {return ids_name[id].name;}
		uint32_t GetIDDepth(uint32_t id) const {return (uint32_t)ids_name[id].depth;}

		float GetCurrentTimeSlice(uint32_t id, uint32_t thread) 
		{
			int32_t frame_id = (int32_t)currentFrameID - PERF_FRAMES_OFFSET;
			if(frame_id < 0)
				frame_id = PERF_FRAMES_DUMP + frame_id;
			double& id_slice = perf_data[id][thread][frame_id];
			double& frame_slice = frameBegin[frame_id];
			return max(0.0f, (float)(id_slice - frame_slice));
		}

		void Dump();

		static void RegLuaFunctions();

	private:
		static Profiler* instance;
		bool started;
		bool dumping;

		uint32_t currentFrameID;

		struct name_depth
		{
			string name;
			uint16_t depth;
			name_depth(string str, uint16_t d) : name(str), depth(d) {}
			name_depth() : depth(0) {}
		};
		SArray<name_depth, PERF_IDS_COUNT> ids_name;

		unordered_map<size_t, uint32_t> thread_map;

		SArray<double, PERF_FRAMES_DUMP> frameBegin;

		RArray<
			RArray<
				SArray<double, PERF_FRAMES_DUMP>
				/*frames*/>
			/*threads*/>
		/*params*/ perf_data; // 8 bytes * threads(9) per frame per param
	};
}