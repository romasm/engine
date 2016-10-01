#include "stdafx.h"
#include "JobSystem.h"
#include "Timer.h"

JobSystem *JobSystem::instance = nullptr;

JobSystem::JobSystem(uint32_t tcount)
{
	if(instance)
		ERR("Only one instance of JobSystem is allowed!");
	instance = this;

	HW_count = tcount;
	if(HW_count == 0)
	{
		HW_count = std::thread::hardware_concurrency();
		if(!HW_count) 
			ERR("No concurrency!");
		else 
			LOG("Hardware threads count: %u ", HW_count);
	}
	else
		LOG("Force threads count: %u ", HW_count);

	syncs.resize(MAX_SYNCS);
	syncs.assign(SYNC_INVALID);
	syncs_freeid.resize(MAX_SYNCS);
	for(uint32_t i=0; i<MAX_SYNCS; i++)
		syncs_freeid[i] = i;

	running = true;
	background_ignore = 0;

	jobsPeriodically.reserve(JOBS_MAX_PERIODICAL);
	
	workers = new thread[HW_count];
	for(uint32_t i = 0; i < HW_count; i++)
		workers[i] = move( thread(&JobSystem::treadFunc, &(*this)) );
}

void JobSystem::Tick(float dt)
{
	for(auto& it: jobsPeriodically)
	{
		it.second.timer += dt;
		if( it.second.timer >= it.second.period ) // TODO?: long jobs taked multiple times
		{
			addJob(it.second.func, it.second.obj, MAX_SYNCS, it.second.priority);
			it.second.timer = 0;
		}
	}
}

void JobSystem::Close()
{
	running = false;
	v_hasJobs.notify_all();

	for(uint32_t i = 0; i < HW_count; i++)
	{
		if(workers[i].joinable())
			workers[i].join();
	}

	if(workers)
		delete[] workers;

	instance = nullptr;
}

bool JobSystem::periodicalJobFillTimer(string name)
{
	auto it = jobsPeriodically.find(name);
	if(it == jobsPeriodically.end())
		return false;

	it->second.timer = it->second.period + 1.0f;
	return true;
}

void JobSystem::addPeriodicalJob(string name, jobMain func, void* obj, float period, JobPriority priority)
{
	if(jobsPeriodically.size() >= JOBS_MAX_PERIODICAL)
		return;

	JobPeriodical job;
	job.func = func;
	job.obj = obj;
	job.period = period;
	job.priority = priority;
	jobsPeriodically.insert(make_pair(name, job));
}

void JobSystem::deletePeriodicalJob(string name)
{
	jobsPeriodically.erase(name);
}

void JobSystem::addJob(jobMain func, void* obj, uint16_t sync, JobPriority priority)
{
	SDeque<Job, JOBS_MAX_QUEUE>* jobs;
	switch (priority)
	{
	case JobPriority::BACKGROUND:
		jobs = &jobsBackground;
		break;
	case JobPriority::FRAME:
		jobs = &jobsFrame;
		break;
	case JobPriority::IMMEDIATE:
		jobs = &jobsImmediate;
		break;
	}

	{
		lock_guard<mutex> guard(jobGraber);

		if(jobs->full())
		{
			ERR("Jobs queue overflow!");
			return;
		}

		if(sync < MAX_SYNCS)
		{
			lock_guard<mutex> guard(syncProtect);
			if(syncs[sync] != SYNC_INVALID)
				syncs[sync]++;
		}

		Job& j = jobs->push_back();
		j.func = func;
		j.sync = sync;
		j.obj = obj;
		j.id = (uint16_t)priority * JOBS_MAX_QUEUE + (uint16_t)jobs->back_idx();
		
		//if(sync < MAX_SYNCS) LOG("Job %u added with sync %u", j.id, j.sync);
		//else LOG("Job %u added", j.id);
	}
	
	v_hasJobs.notify_one();
}

JobSystem::Job JobSystem::takeJob()
{
	lock_guard<mutex> guard(jobGraber);

	Job res;
	if(!jobsImmediate.empty())
	{
		res = jobsImmediate.front();
		jobsImmediate.pop_front();
	}
	else
	{
		if( !jobsFrame.empty() && (background_ignore < JOBS_FRAME_PER_BG || jobsBackground.empty()) )
		{
			res = jobsFrame.front();
			jobsFrame.pop_front();

			background_ignore++;
		}
		else if(!jobsBackground.empty())
		{
			res = jobsBackground.front();
			jobsBackground.pop_front();

			background_ignore = 0;
		}
	}
	return res;
}

uint16_t JobSystem::createSync()
{
	lock_guard<mutex> guard(syncProtect);

	if(syncs_freeid.size() == 0)
	{
		ERR("Syncs amount overflow!");
		return MAX_SYNCS;
	}

	uint16_t idx = syncs_freeid.front();
	syncs_freeid.pop_front();

	syncs[idx] = 0;
	LOG("Sync %u created", idx);
	return idx;
}

void JobSystem::waitSync(uint16_t& id)
{
	unique_lock<mutex> l(syncProtect);

	if(id >= MAX_SYNCS || syncs[id] == SYNC_INVALID)
		return;				

	if(syncs[id] > 0)
	{
		LOG("Waiting for sync %u", id);
		v_syncCheck.wait(l, [&]()
		{
			return syncs[id] == 0;
		});
	}
	LOG("Sync %u passed", id);

	syncs[id] = SYNC_INVALID;
	syncs_freeid.push_back(id);
	id = MAX_SYNCS;
}

void JobSystem::dropSync(uint16_t& id)
{
	lock_guard<mutex> guard(syncProtect);

	if(id >= MAX_SYNCS || syncs[id] == SYNC_INVALID)
		return;		

	LOG("Sync %u droped", id);

	syncs[id] = SYNC_INVALID;
	syncs_freeid.push_back(id);
	id = MAX_SYNCS;
}

void JobSystem::treadFunc()
{
	LOG("Start tread %u ", GetThreadID());

	while(running)
	{
		Job job = this->takeJob();
		if(job.func == nullptr)
		{
			//LOG("Tread %u go to sleep", GetThreadID());

			mutex m;
			unique_lock<mutex> l(m);
			v_hasJobs.wait(l);
			l.unlock();

			//LOG("Tread %u wake up", GetThreadID());

			continue;
		}

		//LOG("Job %u taken by thread %u ", job.id, GetThreadID());

		job.func(job.obj);

		//LOG("Job %u done by thread %u ", job.id, GetThreadID());

		if(job.sync < MAX_SYNCS)
		{
			lock_guard<mutex> guard(syncProtect);
			if(syncs[job.sync] > 0 && syncs[job.sync] != SYNC_INVALID)
			{
				syncs[job.sync]--;
				if(syncs[job.sync] == 0)
					v_syncCheck.notify_all();
			}
		}
	}

	LOG("End tread %u ", GetThreadID());
}



