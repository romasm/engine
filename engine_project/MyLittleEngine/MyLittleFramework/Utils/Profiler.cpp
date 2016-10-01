#include "stdafx.h"
#include "Profiler.h"
#include "FileIO.h"

using namespace EngineCore;

Profiler* Profiler::instance = nullptr;

Profiler::Profiler()
{
	perf_data.create(PERF_IDS_COUNT);
	perf_data.resize(PERF_IDS_COUNT);
	for(uint32_t i = 0; i < PERF_IDS_COUNT; i++)
	{
		perf_data[i].create(JobSystem::GetHWCount() + 1);
		perf_data[i].resize(perf_data[i].capacity());
		for(uint32_t j = 0; j < perf_data[i].capacity(); j++)
		{
			perf_data[i][j].resize(PERF_FRAMES_DUMP);
			perf_data[i][j].assign(0);
		}
	}
	started = false;
	currentFrameID = 0;

	thread_map.reserve(JobSystem::GetHWCount() + 1);

	if(instance)
		ERR("Only one instance of Profiler is allowed!");
	instance = this;
}

Profiler::~Profiler()
{
	instance = nullptr;
}

void Profiler::Dump()
{
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
