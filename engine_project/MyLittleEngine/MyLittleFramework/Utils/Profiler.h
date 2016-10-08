#pragma once
#include "Log.h"
#include "macros.h"
#include "Util.h"
#include "Timer.h"
#include "DataAlloc\Arrays.h"
#include "JobSystem.h"
#include "Render.h"

#include "ProfilerIDs.h"

#define PERF_FRAMES_DUMP 60 * 60 * 1 // 1 minute
#define PERF_FRAMES_OFFSET 5
#define PERF_PARAMS_DEPTH 32

#define PROFILER Profiler::Get()

#ifdef _DEV
	#define PERF_CPU_FRAME_BEGIN Profiler::Get()->CPU_BeginFrame()
	#define PERF_CPU_FRAME_END Profiler::Get()->CPU_EndFrame()
	#define PERF_CPU_BEGIN(param) Profiler::CPU_TimeBegin(PERF_CPU::PERF_CPU##param##)
	#define PERF_CPU_END(param) Profiler::CPU_TimeEnd(PERF_CPU::PERF_CPU##param##)

	#define PERF_GPU_FRAME_BEGIN Profiler::Get()->GPU_BeginFrame()
	#define PERF_GPU_FRAME_END Profiler::Get()->GPU_EndFrame()
	#define PERF_GPU_TIMESTAMP(param) Profiler::GPU_Timestamp(PERF_GPU::PERF_GPU##param##)
	#define PERF_GPU_GRABDATA Profiler::Get()->GPU_GrabData()
#else
	#define PERF_CPU_FRAME_BEGIN
	#define PERF_CPU_FRAME_END
	#define PERF_CPU_BEGIN(param)
	#define PERF_CPU_END(param)

	#define PERF_GPU_FRAME_BEGIN
	#define PERF_GPU_FRAME_END
	#define PERF_GPU_TIMESTAMP(param)
	#define PERF_GPU_GRABDATA
#endif

namespace EngineCore
{
	class Profiler
	{

#undef max

	public:
		Profiler();
		~Profiler();

		bool InitQueries();
		void ReleaseQueries();

		static Profiler* Get(){return instance;}

		void CPU_BeginFrame();
		void CPU_EndFrame();
		
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

		void GPU_BeginFrame();
		void GPU_EndFrame();
		inline static void GPU_Timestamp( uint32_t id )
		{
			if(!instance || !instance->started)
				return;

			CONTEXT->End(instance->timestamps[id][instance->querySwitch]);
		}

		void GPU_GrabData();

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

		uint32_t GetIDsCount() const {return PERF_CPU::PERF_CPU_COUNT;}
		uint32_t GetThreadsCount() const {return (uint32_t)thread_map.size();}
		string GetIDName(uint32_t id) const {return ids_name[id].name;}
		uint32_t GetIDDepth(uint32_t id) const {return (uint32_t)ids_name[id].depth;}
		
		uint32_t GetGpuIDsCount() const {return PERF_GPU::PERF_GPU_COUNT;}
		string GetGpuIDName(uint32_t id) const {return gpu_ids_name[id].name;}
		uint32_t GetGpuIDDepth(uint32_t id) const {return gpu_ids_name[id].depth;}

		XMFLOAT2 GetCurrentTimeSlice(uint32_t id, uint32_t thread);
		XMFLOAT2 GetGpuCurrentTimeSlice(uint32_t id);

		void Dump();

		static void RegLuaFunctions();

	private:
		void InitParams();
		
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
		SArray<name_depth, PERF_CPU::PERF_CPU_COUNT> ids_name;

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
		/*params*/ perf_data; // 8 bytes * threads(9) per frame(3600) per param

		SArray<ID3D11Query*, 2> disjoints;
		RArray<SArray<ID3D11Query*, 2>/*params*/> timestamps;

		RArray<
			SArray<frame_perf, PERF_FRAMES_DUMP>
			/*frames*/>
		/*params*/ gpu_perf_data; // 8 bytes per frame(3600) per param
		
		SArray<name_depth, PERF_GPU::PERF_GPU_COUNT> gpu_ids_name;

		SArray<int32_t, PERF_PARAMS_DEPTH> gpu_prev_id;

		uint8_t querySwitch;
		int8_t collectSwitch;
	};
}