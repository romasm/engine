#pragma once

#include "ECS_defines.h"

#define ENTITY_COUNT				MAX_ENTITY_COUNT
#define MINIMUM_FREE_INDICES		1024

namespace EngineCore
{
	class EntityMgr 
	{
	public:
		EntityMgr(uint32_t maxCount)
		{
			maxCount = min(ENTITY_COUNT, maxCount);
			minFreeIdx = min(MINIMUM_FREE_INDICES, maxCount / 32);

			generation.create(maxCount);
			generation.resize(maxCount);
			generation.assign(0);

			free_id.create(maxCount);
			free_id.resize(maxCount);
			for(uint32_t i=0; i<maxCount; i++)
				free_id[i] = uint32_t(i);

			entity_alive.create(maxCount);
			entity_alive.resize(maxCount);
			entity_alive.assign(false);
		}
		~EntityMgr() {}

		Entity CreateEntity()
		{
			Entity res;
			if(free_id.size() <= minFreeIdx)
			{
				res.setnull();
				return res;
			}

			UINT idx = free_id.front();
			free_id.pop_front();
			res.set(idx, generation[idx]);

			entity_alive[idx] = true;

			return res;
		}

		Entity RestoreEntity() // remove???
		{
			Entity res;
			if(free_id.size() <= minFreeIdx)
			{
				res.setnull();
				return res;
			}

			UINT idx = free_id.back();
			free_id.pop_back();

			generation[idx] = generation[idx] > 0 ? generation[idx] - 1 : 0;
			res.set(idx, generation[idx]);
			
			entity_alive[idx] = true;

			return res;
		}

		inline bool IsAlive(Entity e)
		{
			return generation[e.index()] == e.generation();
		}

		void Destroy(Entity e)
		{
			if(!IsAlive(e)) return;
			UINT idx = e.index();
			generation[idx]++;
			if(generation[idx] >= MAX_ENTITY_GENERATION)
				generation[idx] = 0;

			free_id.push_back(idx);
			
			entity_alive[idx] = false;
		}

		inline uint32_t GetEntityCount() { return (uint32_t)(free_id.capacity() - free_id.size()); }

		Entity GetNextEntity(Entity e)
		{
			uint32_t index = 0;
			if(!e.isnull())
				index = e.index() + 1;

			Entity res;
			if(index >= generation.capacity())
			{
				res.setnull();
				return res;
			}

			while(!entity_alive[index])
			{
				index++;
				if(index >= generation.capacity())
				{
					res.setnull();
					return res;
				}
			}
			res.set(index, generation[index]);
			return res;
		}

	private:
		RDeque<uint32_t> free_id;
		RArray<uint32_t> generation;

		RArray<bool> entity_alive;

		uint32_t minFreeIdx;
	};
}
