#include "stdafx.h"
#include "Profiler.h"
#include "FileIO.h"

using namespace EngineCore;

Profiler* Profiler::instance = nullptr;

void Profiler::InitParams()
{
	ids_name[PERF_CPU::PERF_CPU_FRAME] = name_depth("Frame", 0);
	ids_name[PERF_CPU::PERF_CPU_GUIUPDATE] = name_depth("GuiUpdate", 1);
	ids_name[PERF_CPU::PERF_CPU_SCENE] = name_depth("Scene", 1);
	ids_name[PERF_CPU::PERF_CPU_SCENE_UPDATE] = name_depth("SceneUpdate", 2);
	ids_name[PERF_CPU::PERF_CPU_SCENE_DRAW] = name_depth("SceneDraw", 2);
	ids_name[PERF_CPU::PERF_CPU_GUIDRAW] = name_depth("GuiDraw & Present", 1);

	gpu_ids_name[PERF_GPU::PERF_GPU_FRAME] = name_depth("Frame", 0);
	gpu_ids_name[PERF_GPU::PERF_GPU_SCENE] = name_depth("Scene", 1);
	gpu_ids_name[PERF_GPU::PERF_GPU_GUI] = name_depth("Gui", 1);
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

XMFLOAT2 Profiler::GetCurrentTimeSlice(uint32_t id, uint32_t thread) 
{
	int32_t frame_id = (int32_t)currentFrameID - PERF_FRAMES_OFFSET;
	if(frame_id < 0)
		frame_id = PERF_FRAMES_DUMP + frame_id;
	auto& id_slice = perf_data[id][thread][frame_id];
	return XMFLOAT2(id_slice.begin, id_slice.length);
}

XMFLOAT2 Profiler::GetGpuCurrentTimeSlice(uint32_t id)
{
	int32_t frame_id = (int32_t)currentFrameID - PERF_FRAMES_OFFSET;
	if(frame_id < 0)
		frame_id = PERF_FRAMES_DUMP + frame_id;
	auto& id_slice = gpu_perf_data[id][frame_id];
	return XMFLOAT2(id_slice.begin, id_slice.length);
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
	for (uint32_t i = 0; i <= PERF_GPU::PERF_GPU_COUNT; i++)
	{
		uint64_t time;
		if( CONTEXT->GetData(timestamps[i][prevCollect], &time, sizeof(uint64_t), 0) != S_OK )
		{
			ERR("Cant get profiler GPU timestamp query data for id: %u !", i);
			return;
		}

		if(i == 0)
			timeBegin = time;

		if(i < PERF_GPU::PERF_GPU_COUNT)
			gpu_perf_data[i][prevFrameID].begin = float(time - timeBegin) / timestampDisjoint.Frequency;

		uint16_t depth = 0;
		if(i < PERF_GPU::PERF_GPU_COUNT)
			depth = gpu_ids_name[i].depth;
		
		int32_t prevID = gpu_prev_id[depth];
		if(prevID >= 0)
			gpu_perf_data[prevID][prevFrameID].length = float(time - gpu_perf_data[prevID][prevFrameID].begin) / timestampDisjoint.Frequency;
		else
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

	stream.close();

	LOG("Performance for last %i frames dumped in %s", PERF_FRAMES_DUMP, filename.c_str());
}
