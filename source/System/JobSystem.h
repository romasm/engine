#pragma once

#include "stdafx.h"
#include "Common.h"

#define JOBS_MAX_QUEUE 4096
#define JOBS_MAX_PERIODICAL 256
#define MAX_SYNCS 1024
#define SYNC_INVALID 16535

#define JOBS_FRAME_PER_BG 8

#define JOBSYSTEM JobSystem::Get()

using namespace EngineCore;

enum JobPriority
{
	BACKGROUND = 0,
	FRAME = 1,
	IMMEDIATE = 2
};

class JobSystem
{

/* without overhead: 

	void func(void* nothing){...}
	jsys.addJob( func, nullptr );

	void func(void* arg){process arg;}
	jsys.addJob( func, &something );
*/

#define JOB_F_VOID(func) [](void* obj){func();}, nullptr
#define JOB_F_ARG(func, arg_type, arg_ptr) [](void* obj){func(reinterpret_cast<arg_type*>(obj));}, reinterpret_cast<void*>(arg_ptr)
#define JOB_F_MEMBER(class_type, object_ptr, func) [](void* obj){reinterpret_cast<class_type*>(obj)->func();}, reinterpret_cast<void*>(object_ptr)

private:
	typedef void (*jobMain)(void*);
	struct Job
	{
		jobMain func;
		void* obj;

		//uint16_t id;
		uint16_t sync;
		Job()
		{
			func = nullptr;
			obj = nullptr;
			//id = 0;
			sync = MAX_SYNCS;
		}
	};

	struct JobPeriodical
	{
		jobMain func;
		void* obj;
		JobPriority priority;
		float period;
		float timer;
		JobPeriodical()
		{
			func = nullptr;
			obj = nullptr;
			priority = JobPriority::BACKGROUND;
			period = 1000;
			timer = 0;
		}
	};

public:
	JobSystem(uint32_t tcount = 0);
	~JobSystem(){}

	void Tick(float dt);
	void Close();

#ifdef _DEV
	void RegThreadsForProfiler();
#endif

	void addJob(jobMain func, void* obj = nullptr, uint16_t sync = MAX_SYNCS, JobPriority priority = JobPriority::FRAME);
	
	// TODO: callback system for periodical jobs for main thread(lua), dependent job system for others
	void addPeriodicalJob(string name, jobMain func, void* obj = nullptr, float period = 1000, JobPriority priority = JobPriority::FRAME);
	void deletePeriodicalJob(string name);
	bool periodicalJobFillTimer(string name);

	uint16_t createSync();
	void waitSync(uint16_t& id);
	void dropSync(uint16_t& id);

	void treadFunc();

	inline static JobSystem* Get() {return instance;}

	inline static uint32_t GetThreadID()
	{
		std::stringstream ss;
		ss << std::this_thread::get_id();
		return uint32_t(std::stoull(ss.str()));
	}
	inline static size_t GetThreadHash()
	{
		return std::this_thread::get_id().hash();
	}
	
	inline static uint32_t GetHWCount() {return instance->HW_count;}

private:
	static JobSystem *instance;

	Job takeJob();
	
	SDeque<Job, JOBS_MAX_QUEUE> jobsBackground;
	SDeque<Job, JOBS_MAX_QUEUE> jobsFrame;
	SDeque<Job, JOBS_MAX_QUEUE> jobsImmediate;

	unordered_map<string, JobPeriodical> jobsPeriodically;

	SArray<uint16_t, MAX_SYNCS> syncs;
	SDeque<uint16_t, MAX_SYNCS> syncs_freeid;

	thread* workers;

	mutex jobGraber;
	mutex syncProtect;

	condition_variable v_hasJobs;
	condition_variable v_syncCheck;

	uint32_t HW_count;
	bool running;
	uint16_t background_ignore;
};