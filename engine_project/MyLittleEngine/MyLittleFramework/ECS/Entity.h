#pragma once

#include "ECS_defines.h"

#define ENTITY_COUNT				MAX_ENTITY_COUNT
#define MINIMUM_FREE_INDICES		1024

namespace EngineCore
{
	class EntityMgr 
	{
	public:
		EntityMgr()
		{
			generation.resize(ENTITY_COUNT);
			generation.assign(0);
			free_id.resize(ENTITY_COUNT);
			for(uint32_t i=0; i<ENTITY_COUNT; i++)
				free_id[i] = uint32_t(i);
			entity_alive.resize(ENTITY_COUNT);
			entity_alive.assign(false);
		}
		~EntityMgr() {}

		Entity CreateEntity()
		{
			Entity res;
			if(free_id.size() <= MINIMUM_FREE_INDICES)
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
			if(free_id.size() <= MINIMUM_FREE_INDICES)
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

		inline uint32_t GetEntityCount() { return (uint32_t)(ENTITY_COUNT - free_id.size()); }

		Entity GetNextEntity(Entity e)
		{
			uint32_t index = 0;
			if(!e.isnull())
				index = e.index() + 1;

			Entity res;
			if(index >= ENTITY_COUNT)
			{
				res.setnull();
				return res;
			}

			while(!entity_alive[index])
			{
				index++;
				if(index >= ENTITY_COUNT)
				{
					res.setnull();
					return res;
				}
			}
			res.set(index, generation[index]);
			return res;
		}

	private:
		SDeque<uint32_t, ENTITY_COUNT> free_id;
		SArray<uint32_t, ENTITY_COUNT> generation;

		SArray<bool, ENTITY_COUNT> entity_alive;
	};
}
