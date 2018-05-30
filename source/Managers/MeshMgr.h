#pragma once
#include "stdafx.h"
#include "Common.h"
#include "BaseMgr.h"
#include "MeshLoader.h"

namespace EngineCore
{
	enum TriClipping
	{
		TC_BOTH = 0,
		TC_FRONT,
		TC_BACK
	};

	class MeshMgr : public BaseMgr<MeshData, RESOURCE_MAX_COUNT>
	{
	public:
		MeshMgr();

		void OnLoad(uint32_t id, MeshData* data, ImportInfo& info, uint32_t& date);

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

		static bool MeshBoxOverlap(uint32_t meshID, const Matrix& transform, const BoundingBox& bbox);
		static float MeshRayIntersect(uint32_t meshID, const Matrix& transform, const Vector3& origin, const Vector3& dirNormal, float maxDist, TriClipping triClipping);
	#endif

		inline static MeshMgr* Get(){return (MeshMgr*)BaseMgr<MeshData, RESOURCE_MAX_COUNT>::Get();}

	private:

#ifdef _EDITOR
		SArray<uint32_t, RESOURCE_MAX_COUNT> mesh_reloaded;
		bool something_reloaded;
#endif
	};

	class SkeletonMgr : public BaseMgr<SkeletonData, RESOURCE_MAX_COUNT>
	{
	public:
		SkeletonMgr() : BaseMgr<SkeletonData, RESOURCE_MAX_COUNT>()
		{
			null_resource = MeshLoader::LoadSkeleton(string(PATH_SKELETON_NULL));
			resType = ResourceType::SKELETON;
			resExt = EXT_SKELETON;
		}
		inline static SkeletonMgr* Get(){return (SkeletonMgr*)BaseMgr<SkeletonData, RESOURCE_MAX_COUNT>::Get();}
	};

	class AnimationMgr : public BaseMgr<AnimationData, RESOURCE_MAX_COUNT>
	{
	public:
		AnimationMgr() : BaseMgr<AnimationData, RESOURCE_MAX_COUNT>()
		{
			null_resource = MeshLoader::LoadAnimation(string(PATH_ANIMATION_NULL));
			resType = ResourceType::ANIMATION;
			resExt = EXT_ANIMATION;
		}
		inline static AnimationMgr* Get(){return (AnimationMgr*)BaseMgr<AnimationData, RESOURCE_MAX_COUNT>::Get();}
	};
}