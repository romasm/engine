#pragma once

#include "Common.h"
#include "LightBuffers.h"
#include "MeshMgr.h"
#include "DebugDrawer.h"

#define DEFAULT_OCTREE_DEPTH 7
#define DEFAULT_OCTREE_VOXEL_SIZE 0.2f

#define BKICK_RESOLUTION 3

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

	struct Octree
	{
		int32_t depth;
		BoundingBox bbox;

		int32_t lookupRes;
		uint32_t*** lookup;

		Octree()
		{
			depth = 0;
			lookupRes = 0;
			lookup = nullptr;
		}

		~Octree()
		{
			for (int32_t x = 0; x < lookupRes; x++)
			{
				for (int32_t y = 0; y < lookupRes; y++)
					delete[] lookup[x][y];
				delete[] lookup[x];
			}
			delete[] lookup;
		}
	};

	struct BoxCornerSize
	{
		Vector3 corner;
		Vector3 size;
	};

	struct Brick
	{
		uint32_t probes[BKICK_RESOLUTION * BKICK_RESOLUTION * BKICK_RESOLUTION];
	};

	struct Prob
	{
		Vector3 pos;
		bool interpolated;
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

		// baking

		float voxelSize;
		float chunkSize;
		int32_t maxOctreeDepth;

		BoxCornerSize worldBox;

		DArray<Octree> octreeArray;
		RArray<RArray<RArray<int32_t>>> chunks;

		DArray<Brick> bricks;

		struct Int3Pos
		{
			int32_t x;
			int32_t y;
			int32_t z;

			Int3Pos()
			{
				x = 0;
				y = 0;
				z = 0;
			}

			Int3Pos(const Vector3 pos)
			{
				x = (int32_t)roundf(pos.x * 1000.0f);
				y = (int32_t)roundf(pos.y * 1000.0f);
				z = (int32_t)roundf(pos.z * 1000.0f);
			}
		};
		unordered_map<Int3Pos, uint32_t> probesLookup;
		DArray<Prob> probesArray;

		DArray<BoundingBox> debugOctreeVisuals;
		bool bDebugOctree;

		bool SceneBoxIntersect(DArray<VoxelizeSceneItem>& staticScene, BoundingBox& bbox);
		void ProcessOctreeBranch(Octree& octree, DArray<VoxelizeSceneItem>& staticScene, BoundingBox& bbox, int32_t octreeDepth, Vector3& octreeHelper);
	
		#define BRICK_ADRESS_BITS 24
		#define BRICK_ADRESS_MASK 0x00ffffff
		#define BRICK_LEVEL_MASK 0xff000000

		// lookup uint32: 8 bit - level, 12 bit - x, 12 bit - y
		inline uint32_t SetLookupNode(uint32_t id, uint32_t level)
		{
			return (id & BRICK_ADRESS_MASK) + ((level << BRICK_ADRESS_BITS) & BRICK_LEVEL_MASK);
		}

		inline uint32_t GetBrickID(uint32_t node)
		{
			return (node & BRICK_ADRESS_MASK);
		}

		inline uint32_t GetBrickLevel(uint32_t node)
		{
			return ((node & BRICK_LEVEL_MASK) >> BRICK_ADRESS_BITS);
		}
	};

}