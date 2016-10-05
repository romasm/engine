#include "stdafx.h"
#include "Profiler.h"
#include "FileIO.h"

using namespace EngineCore;

Profiler* Profiler::instance = nullptr;

Profiler::Profiler()
{
	frame_perf null_perf;
	null_perf.begin = 0;
	null_perf.length = 0;

	perf_data.create(PERF_IDS_COUNT);
	perf_data.resize(PERF_IDS_COUNT);
	for(uint32_t i = 0; i < PERF_IDS_COUNT; i++)
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

	ids_name.resize(PERF_IDS_COUNT);

	ids_name[PERF_CPU_FRAME_ID] = name_depth("Frame", 0);
	ids_name[PERF_CPU_GUIUPDATE_ID] = name_depth("GuiUpdate", 1);
	ids_name[PERF_CPU_SCENE_ID] = name_depth("Scene", 1);
	ids_name[PERF_CPU_SCENE_UPDATE_ID] = name_depth("SceneUpdate", 2);
	ids_name[PERF_CPU_SCENE_DRAW_ID] = name_depth("SceneDraw", 2);
	ids_name[PERF_CPU_GUIDRAW_ID] = name_depth("GuiDraw & Present", 1);

	if(instance)
		ERR("Only one instance of Profiler is allowed!");
	instance = this;
}

Profiler::~Profiler()
{
	instance = nullptr;
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
	
	uint32_t paramsCount = PERF_IDS_COUNT;
	stream.write((char*)&paramsCount, sizeof(uint32_t));
	uint32_t threadsCount = (uint32_t)thread_map.size();
	stream.write((char*)&threadsCount, sizeof(uint32_t));

	for(uint32_t i = 0; i < PERF_IDS_COUNT; i++)
		for(uint32_t j = 0; j < perf_data[i].capacity(); j++)
			stream.write((char*)perf_data[i][j].data(), sizeof(double) * perf_data[i][j].size());

	stream.close();

	LOG("Performance for last %i frames dumped in %s", PERF_FRAMES_DUMP, filename.c_str());
}
