#pragma once
#include "stdafx.h"
#include "Common.h"
#include "BaseMgr.h"
#include "MeshLoader.h"

namespace EngineCore
{
	class MeshMgr : public BaseMgr<MeshData, RESOURCE_MAX_COUNT>
	{
	public:
		MeshMgr();

		void OnLoad(uint32_t id, MeshData* data);

	#ifdef _EDITOR
		inline bool IsJustReloaded(uint32_t id) 
		{
			if(id == nullres)
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

		inline static MeshMgr* Get(){return (MeshMgr*)BaseMgr<MeshData, RESOURCE_MAX_COUNT>::Get();}
		
	private:

#ifdef _EDITOR
		SArray<uint32_t, RESOURCE_MAX_COUNT> mesh_reloaded;
		bool something_reloaded;
#endif
	};
}