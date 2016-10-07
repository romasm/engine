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
	#define PERF_CPU_BEGIN(param) Profiler::CPU_TimeBegin(PERF_CPU##param##_ID)
	#define PERF_CPU_END(param) Profiler::CPU_TimeEnd(PERF_CPU##param##_ID)
#else
	#define PERF_CPU_BEGIN(param)
	#define PERF_CPU_END(param)
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
		
		inline static void CPU_TimeBegin( uint32_t id )
		{
			if(!instance || !instance->started)
				return;

			uint32_t& th = instance->thread_map[JobSystem::GetThreadHash()];
			instance->perf_data[id][th][instance->currentFrameID].begin = 
				(float)(Timer::ForcedGetCurrentTime() - instance->frameBegin[instance->currentFrameID]);
		}
		inline static void CPU_TimeEnd( uint32_t id )
		{
			if(!instance || !instance->started)
				return;

			uint32_t& th = instance->thread_map[JobSystem::GetThreadHash()];
			instance->perf_data[id][th][instance->currentFrameID].length = 
				(float)(Timer::ForcedGetCurrentTime() - instance->frameBegin[instance->currentFrameID] - 
					instance->perf_data[id][th][instance->currentFrameID].begin);
		}

		void Stop() 
		{
			started = false;
			LOG("Profiler stopped!");
		}
		void Start()
		{
			started = true;
			LOG("Profiler started!");
		}
		inline bool IsRunning() {return started;}

		void RegThread(size_t thread_hash)
		{
			instance->thread_map.insert(make_pair(thread_hash, thread_count));
			thread_count++;
		}

		void SetDumping(bool dump) {dumping = dump;}
		bool GetDumping() const {return dumping;}

		uint32_t GetIDsCount() const {return PERF_IDS_COUNT;}
		uint32_t GetThreadsCount() const {return (uint32_t)thread_map.size();}
		string GetIDName(uint32_t id) const {return ids_name[id].name;}
		uint32_t GetIDDepth(uint32_t id) const {return (uint32_t)ids_name[id].depth;}

		XMFLOAT2 GetCurrentTimeSlice(uint32_t id, uint32_t thread) 
		{
			int32_t frame_id = (int32_t)currentFrameID - PERF_FRAMES_OFFSET;
			if(frame_id < 0)
				frame_id = PERF_FRAMES_DUMP + frame_id;
			auto& id_slice = perf_data[id][thread][frame_id];
			return XMFLOAT2(id_slice.begin, id_slice.length);
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
		uint32_t thread_count;

		SArray<double, PERF_FRAMES_DUMP> frameBegin;

		struct frame_perf
		{
			float begin;
			float length;
		};
		RArray<
			RArray<
				SArray<frame_perf, PERF_FRAMES_DUMP>
				/*frames*/>
			/*threads*/>
		/*params*/ perf_data; // 8 bytes * threads(9) per frame per param
	};
}