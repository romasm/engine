#pragma once

#include "ECS_defines.h"

#define ENTITY_COUNT				MAX_ENTITY_COUNT
#define MINIMUM_FREE_INDICES		1024

#define FRUSTUM_MAX_COUNT 256

namespace EngineCore
{
	class EntityMgr 
	{
	public:
		EntityMgr(uint32_t maxCount)
		{
			maxCount = std::min<uint32_t>(ENTITY_COUNT, maxCount);
			minFreeIdx = std::min<uint32_t>(MINIMUM_FREE_INDICES, maxCount / 32);

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

			enable.create(maxCount);
			enable.resize(maxCount);
			enable.assign(true);

		#ifdef _EDITOR
			editor_visible.create(maxCount);
			editor_visible.resize(maxCount);
			editor_visible.assign(true);
		#endif
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

			uint32_t idx = free_id.front();
			free_id.pop_front();
			res.set(idx, generation[idx]);

			entity_alive[idx] = true;
			enable[idx] = true;
		#ifdef _EDITOR
			editor_visible[idx] = true;
		#endif

			return res;
		}

		inline bool IsAlive(Entity e)
		{
			return generation[e.index()] == e.generation();
		}

		inline bool IsNeedProcess(Entity e) const 
		{ 
		#ifdef _EDITOR
			return enable[e.index()] && editor_visible[e.index()];
		#else
			return enable[e.index()];
		#endif
		}

		inline bool IsEnable(Entity e) const 
		{ return enable[e.index()]; }

		inline void SetEnable(Entity e, bool isEnbale)
		{ enable[e.index()] = isEnbale; }

	#ifdef _EDITOR

		inline bool IsEditorVisible(Entity e) const
		{ return editor_visible[e.index()]; }

		inline void SetEditorVisible(Entity e, bool isVisible)
		{ editor_visible[e.index()] = isVisible; }

	#endif

		void Destroy(Entity e)
		{
			if(!IsAlive(e)) return;
			uint32_t idx = e.index();
			generation[idx]++;
			if(generation[idx] >= MAX_ENTITY_GENERATION)
				generation[idx] = 0;

			free_id.push_back(idx);
			
			entity_alive[idx] = false;
			enable[idx] = false;
		#ifdef _EDITOR
			editor_visible[idx] = false;
		#endif
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

		RArray<bool> enable;

	#ifdef _EDITOR
		RArray<bool> editor_visible;
	#endif

		uint32_t minFreeIdx;
	};
}
