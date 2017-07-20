#pragma once
#include "stdafx.h"
#include "Common.h"
#include "BaseMgr.h"
#include "MeshLoader.h"

namespace EngineCore
{
	class MeshMgr : public BaseMgr<MeshData>
	{
	public:
		MeshMgr();

		void OnLoad(uint32_t id, MeshData* data);

	#ifdef _EDITOR
		inline bool IsJustReloaded(uint32_t id) 
		{
			if(id == RESOURCE_NULL)
				return false;
			bool res = mesh_reloaded[id] > 0;
			if(res)
				mesh_reloaded[id]--;
			return res;
		}

		inline bool IsBBoxesDirty()
		{
			bool res = something_reloaded;
			something_reloaded = false;
			return res;
		}
	#endif

	private:

#ifdef _EDITOR
		SArray<uint32_t, RESOURCE_MAX_COUNT> mesh_reloaded;
		bool something_reloaded;
#endif
	};
}