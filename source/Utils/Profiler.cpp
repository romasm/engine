#include "stdafx.h"
#include "Profiler.h"
#include "FileIO.h"

using namespace EngineCore;

Profiler* Profiler::instance = nullptr;

void Profiler::InitParams()
{
	ids_name[PERF_CPU::PERF_CPU_FRAME] = name_depth("Frame", 0);
	ids_name[PERF_CPU::PERF_CPU_LUA_TICK] = name_depth("LuaTick", 1);
	ids_name[PERF_CPU::PERF_CPU_WIN_MSG] = name_depth("Win Msg", 1);
	ids_name[PERF_CPU::PERF_CPU_GUI_UPDATE] = name_depth("GuiUpdate", 1);
	ids_name[PERF_CPU::PERF_CPU_SCENE] = name_depth("Scene", 1);
		ids_name[PERF_CPU::PERF_CPU_SCENE_UPDATE] = name_depth("SceneUpdate", 2);
		ids_name[PERF_CPU::PERF_CPU_SCENE_DRAW] = name_depth("SceneDraw", 2);
			ids_name[PERF_CPU::PERF_CPU_VOXELIZATION] = name_depth("Voxelization", 3);
			ids_name[PERF_CPU::PERF_CPU_VOXELLIGHT] = name_depth("VxLight", 3);
	ids_name[PERF_CPU::PERF_CPU_GUI_DRAW] = name_depth("GuiDraw", 1);
	ids_name[PERF_CPU::PERF_CPU_PRESENT] = name_depth("Present", 1);

	gpu_ids_name[PERF_GPU::PERF_GPU_FRAME] = name_depth("Frame", 0);
	gpu_ids_name[PERF_GPU::PERF_GPU_SCENE] = name_depth("Scene", 1);
		gpu_ids_name[PERF_GPU::PERF_GPU_SCENE_SHADOWS] = name_depth("Shadows", 2); 
		gpu_ids_name[PERF_GPU::PERF_GPU_SCENE_FORWARD] = name_depth("Forward", 2);
			gpu_ids_name[PERF_GPU::PERF_GPU_VOXELIZATION] = name_depth("Voxelization", 3);
			gpu_ids_name[PERF_GPU::PERF_GPU_VOXELLIGHT] = name_depth("VxLight", 3);
				gpu_ids_name[PERF_GPU::PERF_GPU_PROPAGATE] = name_depth("Propagation", 4);
				gpu_ids_name[PERF_GPU::PERF_GPU_VOXELDOWNSAMPLE] = name_depth("Downsample", 4);
			gpu_ids_name[PERF_GPU::PERF_GPU_GEOMETRY] = name_depth("Geometry", 3);
			gpu_ids_name[PERF_GPU::PERF_GPU_DEPTH_COPY] = name_depth("DepthCopy", 3);
		gpu_ids_name[PERF_GPU::PERF_GPU_SCENE_DEFFERED] = name_depth("Deffered", 2);
			gpu_ids_name[PERF_GPU::PERF_GPU_HIZ_GEN] = name_depth("HiZ", 3);
			gpu_ids_name[PERF_GPU::PERF_GPU_SSR] = name_depth("SSR", 3);
			gpu_ids_name[PERF_GPU::PERF_GPU_AO] = name_depth("AO", 3);
			gpu_ids_name[PERF_GPU::PERF_GPU_OPAQUE_MAIN] = name_depth("OpaqueMain", 3);
			gpu_ids_name[PERF_GPU::PERF_GPU_OPAQUE_FINAL] = name_depth("OpaqueFinal", 3);
			gpu_ids_name[PERF_GPU::PERF_GPU_HDR_BLOOM] = name_depth("BloomHDR", 3);
		gpu_ids_name[PERF_GPU::PERF_GPU_SCENE_APLHA] = name_depth("Transparent", 2);
		gpu_ids_name[PERF_GPU::PERF_GPU_SCENE_UI] = name_depth("UI", 2);
		gpu_ids_name[PERF_GPU::PERF_GPU_SCENE_LDR] = name_depth("LDR", 2);
			gpu_ids_name[PERF_GPU::PERF_GPU_COMBINE] = name_depth("Combine", 3);
			gpu_ids_name[PERF_GPU::PERF_GPU_AA] = name_depth("SMAA", 3);
	gpu_ids_name[PERF_GPU::PERF_GPU_GUI] = name_depth("Gui", 1);
	gpu_ids_name[PERF_GPU::PERF_GPU_PRESENT] = name_depth("Present", 1);
}

Profiler::Profiler()
{
	frame_perf null_perf;
	null_perf.begin = 0;
	null_perf.length = 0;

	// CPU
	perf_data.create(PERF_CPU::PERF_CPU_COUNT);
	perf_data.resize(PERF_CPU::PERF_CPU_COUNT);
	for(uint32_t i = 0; i < PERF_CPU::PERF_CPU_COUNT; i++)
	{
		perf_data[i].create(JobSystem::GetHWCount() + 1);
		perf_data[i].resize(perf_data[i].capacity());
		for(uint32_t j = 0; j < perf_data[i].capacity(); j++)
		{
			perf_data[i][j].resize(PERF_FRAMES_DUMP);
			perf_data[i][j].assign(null_perf);
		}
	}

	frameBegin.resize(PERF_FRAMES_DUMP);
	frameBegin.assign(0);

	started = false;
	dumping = false;
	currentFrameID = 0;

	thread_map.reserve(JobSystem::GetHWCount() + 1);
	thread_count = 0;

	ids_name.resize(PERF_CPU::PERF_CPU_COUNT);

	InitParams();

	// GPU
	disjoints.resize(2);
	disjoints.assign(nullptr);

	timestamps.create(PERF_GPU::PERF_GPU_COUNT + 1);
	timestamps.resize(PERF_GPU::PERF_GPU_COUNT + 1);
	for(uint32_t i = 0; i <= PERF_GPU::PERF_GPU_COUNT; i++)
	{
		timestamps[i].resize(2);
		timestamps[i].assign(nullptr);
	}

	gpu_perf_data.create(PERF_GPU::PERF_GPU_COUNT);
	gpu_perf_data.resize(PERF_GPU::PERF_GPU_COUNT);
	for(uint32_t i = 0; i < PERF_GPU::PERF_GPU_COUNT; i++)
	{
		gpu_perf_data[i].resize(PERF_FRAMES_DUMP);
		gpu_perf_data[i].assign(null_perf);
	}

	querySwitch = 0;
	collectSwitch = -1;

	gpu_ids_name.resize(PERF_GPU::PERF_GPU_COUNT);

	gpu_prev_id.resize(PERF_PARAMS_DEPTH);
	gpu_prev_id.assign(-1);

	if(instance)
		ERR("Only one instance of Profiler is allowed!");
	instance = this;
}

Profiler::~Profiler()
{
	instance = nullptr;
}

bool Profiler::InitQueries()
{
	D3D11_QUERY_DESC queryDesc = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };

	for(uint32_t i = 0; i < 2; i++)
		if ( FAILED( DEVICE->CreateQuery(&queryDesc, &disjoints[i]) ) )
			return false;

	queryDesc.Query = D3D11_QUERY_TIMESTAMP;

	for(uint32_t i = 0; i <= PERF_GPU::PERF_GPU_COUNT; i++)
		for(uint32_t j = 0; j < 2; j++)
			if ( FAILED( DEVICE->CreateQuery(&queryDesc, &timestamps[i][j]) ) )
				return false;
	
	return true;
}

void Profiler::ReleaseQueries()
{
	for(uint32_t i = 0; i < 2; i++)
		_RELEASE(disjoints[i]);

	for(uint32_t i = 0; i <= PERF_GPU::PERF_GPU_COUNT; i++)
		for(uint32_t j = 0; j < 2; j++)
			_RELEASE(timestamps[i][j]);
}

void Profiler::CPU_BeginFrame()
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

	for(uint32_t i = 0; i < PERF_CPU::PERF_CPU_COUNT; i++)
		for(uint32_t j = 0; j < perf_data[i].capacity(); j++)
		{
			perf_data[i][j][currentFrameID].begin = 0;
			perf_data[i][j][currentFrameID].length = 0;
		}

	perf_data[PERF_CPU::PERF_CPU_FRAME][0][currentFrameID].begin = 
		(float)(Timer::ForcedGetCurrentTime() - frameBegin[currentFrameID]);
}

void Profiler::CPU_EndFrame()
{
	if(!started)
		return;

	perf_data[PERF_CPU::PERF_CPU_FRAME][0][currentFrameID].length = 
		(float)(Timer::ForcedGetCurrentTime() - frameBegin[currentFrameID] - perf_data[PERF_CPU::PERF_CPU_FRAME][0][currentFrameID].begin);
}

void Profiler::GPU_BeginFrame()
{
	if(!started)
		return;

	CONTEXT->Begin(disjoints[querySwitch]);
	CONTEXT->End(timestamps[PERF_GPU::PERF_GPU_FRAME][querySwitch]);
}
		
void Profiler::GPU_EndFrame()
{
	if(!started)
		return;
	
	CONTEXT->End(timestamps[PERF_GPU::PERF_GPU_COUNT][querySwitch]);
	CONTEXT->End(disjoints[querySwitch]);
	++querySwitch &= 1;
}

Vector2 Profiler::GetCurrentTimeSlice(uint32_t id, uint32_t thread) 
{
	int32_t frame_id = (int32_t)currentFrameID - PERF_FRAMES_OFFSET;
	if(frame_id < 0)
		frame_id = PERF_FRAMES_DUMP + frame_id;
	auto& id_slice = perf_data[id][thread][frame_id];
	return Vector2(id_slice.begin, id_slice.length);
}

Vector2 Profiler::GetGpuCurrentTimeSlice(uint32_t id)
{
	int32_t frame_id = (int32_t)currentFrameID - PERF_FRAMES_OFFSET;
	if(frame_id < 0)
		frame_id = PERF_FRAMES_DUMP + frame_id;
	auto& id_slice = gpu_perf_data[id][frame_id];
	return Vector2(id_slice.begin, id_slice.length);
}

void Profiler::GPU_GrabData()
{
	if(!started)
		return;

	if(collectSwitch < 0)
	{
		collectSwitch = 0;
		return;
	}

	bool msg = false;
	while( CONTEXT->GetData(disjoints[collectSwitch], NULL, 0, 0) == S_FALSE )
	{
		if(!msg)
		{
			WRN("Profiler wait for GPU data!");
			msg = true;
		}
		Sleep(1);
	}

	int8_t prevCollect = collectSwitch;
	++collectSwitch &= 1;

	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT timestampDisjoint;
	if( CONTEXT->GetData(disjoints[prevCollect], &timestampDisjoint, sizeof(timestampDisjoint), 0) != S_OK )
	{
		ERR("Cant get profiler GPU timestamp disjoint query data!");
		return;
	}

	if (timestampDisjoint.Disjoint)
	{
		WRN("Profiler GPU timestamps disjoint!");
		return;
	}

	int32_t prevFrameID = (int32_t)currentFrameID - 1;
	if(prevFrameID < 0)
		prevFrameID = PERF_FRAMES_DUMP + prevFrameID;

	gpu_prev_id.assign(-1);

	uint64_t timeBegin = 0;
	uint16_t maxDepth = 0;
	for (uint32_t i = 0; i <= PERF_GPU::PERF_GPU_COUNT; i++)
	{
		uint64_t time;
		if( CONTEXT->GetData(timestamps[i][prevCollect], &time, sizeof(uint64_t), 0) != S_OK )
		{
			//ERR("Cant get profiler GPU timestamp query data for id: %u !", i);
			if(i < PERF_GPU::PERF_GPU_COUNT)
			{
				gpu_perf_data[i][prevFrameID].begin = 0;
				gpu_perf_data[i][prevFrameID].length = 0;
			}
			continue;
		}

		if(i == 0)
			timeBegin = time;

		float timeslice = 1000.0f * float(time - timeBegin) / timestampDisjoint.Frequency;

		if(i < PERF_GPU::PERF_GPU_COUNT)
			gpu_perf_data[i][prevFrameID].begin = timeslice;

		uint16_t depth = 0;
		if(i < PERF_GPU::PERF_GPU_COUNT)
			depth = gpu_ids_name[i].depth;
		
		maxDepth = max(maxDepth, depth);

		int32_t prevID = gpu_prev_id[depth];
		if(prevID >= 0)
		{
			gpu_perf_data[prevID][prevFrameID].length = timeslice - gpu_perf_data[prevID][prevFrameID].begin;
			for(uint16_t j = depth + 1; j <= maxDepth; j++)
			{
				int32_t prevDepthID = gpu_prev_id[j];
				if(prevDepthID < 0)
					continue;
				gpu_perf_data[prevDepthID][prevFrameID].length = timeslice - gpu_perf_data[prevDepthID][prevFrameID].begin;
				gpu_prev_id[j] = -1;
			}
		}
		
		gpu_prev_id[depth] = i;
	}
}

void Profiler::RegLuaFunctions()
{
	getGlobalNamespace(LSTATE)
		.beginNamespace("Util")
			.addFunction("GetProfiler", &Profiler::Get)
			.beginClass<Profiler>("Profiler")
				.addFunction("Stop", &Profiler::Stop)
				.addFunction("Start", &Profiler::Start)
				.addFunction("IsRunning", &Profiler::IsRunning)
				.addProperty("dump", &Profiler::GetDumping, &Profiler::SetDumping)
				.addFunction("GetIDsCount", &Profiler::GetIDsCount)
				.addFunction("GetThreadsCount", &Profiler::GetThreadsCount)
				.addFunction("GetIDName", &Profiler::GetIDName)
				.addFunction("GetIDDepth", &Profiler::GetIDDepth)
				.addFunction("GetCurrentTimeSlice", &Profiler::GetCurrentTimeSlice)

				.addFunction("GetGpuIDsCount", &Profiler::GetGpuIDsCount)
				.addFunction("GetGpuIDName", &Profiler::GetGpuIDName)
				.addFunction("GetGpuIDDepth", &Profiler::GetGpuIDDepth)
				.addFunction("GetGpuCurrentTimeSlice", &Profiler::GetGpuCurrentTimeSlice)
			.endClass()
		.endNamespace();
}

void Profiler::Dump()
{
	if(!dumping)
		return;

	GET_DATETIME

	string filename = PATH_RUNTIME_STATS "perf_cpu_" + datetime + ".prf";

	ofstream stream;
	stream.open(filename, ofstream::out | ofstream::trunc | ofstream::binary);
	if(!stream.good())
		return;
	
	uint32_t paramsCount = PERF_CPU::PERF_CPU_COUNT;
	stream.write((char*)&paramsCount, sizeof(uint32_t));
	uint32_t threadsCount = (uint32_t)thread_map.size();
	stream.write((char*)&threadsCount, sizeof(uint32_t));

	for(uint32_t i = 0; i < PERF_CPU::PERF_CPU_COUNT; i++)
		for(uint32_t j = 0; j < perf_data[i].capacity(); j++)
			stream.write((char*)perf_data[i][j].data(), sizeof(double) * perf_data[i][j].size());

	uint32_t gpuParamsCount = PERF_GPU::PERF_GPU_COUNT;
	stream.write((char*)&gpuParamsCount, sizeof(uint32_t));
	for(uint32_t i = 0; i < PERF_GPU::PERF_GPU_COUNT; i++)
		stream.write((char*)gpu_perf_data[i].data(), sizeof(double) * gpu_perf_data[i].size());

	stream.close();

	LOG("Performance for last %i frames dumped in %s", PERF_FRAMES_DUMP, filename.c_str());
}
