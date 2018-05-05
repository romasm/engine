#pragma once

#include "Common.h"
#include "LightBuffers.h"
#include "MeshMgr.h"
#include "DebugDrawer.h"

#define DEFAULT_VOXEL_SIZE 0.6f

namespace EngineCore
{
	struct GISampleData
	{
		Vector3 minCorner;
		float worldSizeRcp;
	};

	struct VoxelizeSceneItem
	{
		BoundingBox bbox;
		Matrix transform;
		uint32_t meshID;
		
		VoxelizeSceneItem()
		{
			meshID = MeshMgr::nullres;
		}
	};

	struct OctreeBranch
	{
		uint32_t leaf[8];
	};

	class GIMgr
	{
	public:
		GIMgr(class BaseWorld* wrd);
		~GIMgr();
				
		bool ReloadGIData();
		void DropGIData();

		ID3D11ShaderResourceView* GetGIVolumeSRV();
		ID3D11Buffer* GetGISampleData() { return sampleDataGPU; }

		bool BuildVoxelOctree();

		void DebugDrawOctree(DebugDrawer* dbgDrawer);

	private:
		bool InitBuffers();

		BaseWorld* world;

		ID3D11Texture3D* giVolume;
		ID3D11ShaderResourceView* giVolumeSRV;
		ID3D11UnorderedAccessView* giVolumeUAV;
		
		uint32_t sgVolume;

		GISampleData sampleData;
		ID3D11Buffer* sampleDataGPU;

		float voxelSize;

		BoundingBox worldBBPow2;
		DArray<OctreeBranch> octree;

		DArray<BoundingBox> debugOctreeVisuals;
		bool bDebugOctree;

		bool SceneBoxIntersect(DArray<VoxelizeSceneItem>& staticScene, BoundingBox& bbox);
		void ProcessOctreeBranch(DArray<OctreeBranch>& octree, DArray<VoxelizeSceneItem>& staticScene, uint32_t branchID, BoundingBox& bbox, int32_t octreeDepth);

		inline void MarkAsLeaf(uint32_t& leaf)
		{
			leaf = leaf | 0x80000000;
		}
		
		inline void MarkAsBranch(uint32_t& leaf)
		{
			leaf = leaf & 0x7fffffff;
		}

		inline bool IsLeaf(uint32_t& leaf)
		{
			return (leaf >> 31) > 0;
		}
	};

}