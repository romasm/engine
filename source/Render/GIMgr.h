#pragma once

#include "Common.h"
#include "LightBuffers.h"
#include "MeshMgr.h"
#include "DebugDrawer.h"

#define OCTREE_DEPTH 7
#define DEFAULT_OCTREE_VOXEL_SIZE 0.2f

#define BKICK_RESOLUTION 3

#define DEBUG_MATERIAL_PROBES "$" PATH_SHADERS "objects/editor/debug_probes"

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

	// !!! Use only with SArray or RArray
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
		BoundingBox bbox;
		uint32_t levelInv;
	};

	struct Prob
	{
		Vector3 pos;
		bool interpolated;
	};

	class GIMgr
	{
	public:
		enum DebugState
		{
			DS_NONE = 0,
			DS_OCTREE,
			DS_PROBES
		};

		GIMgr(class BaseWorld* wrd);
		~GIMgr();
				
		bool ReloadGIData();
		void DropGIData();

		ID3D11ShaderResourceView* GetGIVolumeSRV();
		ID3D11Buffer* GetGISampleData() { return sampleDataGPU; }

		bool BuildVoxelOctree();
		
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

		bool SceneBoxIntersect(DArray<VoxelizeSceneItem>& staticScene, BoundingBox& bbox);
		void ProcessOctreeBranch(Octree& octree, DArray<VoxelizeSceneItem>& staticScene, BoundingBox& bbox, int32_t octreeDepth, 
			Vector3& octreeHelper, Vector3& octreeCorner);


		float voxelSize;
		float chunkSize;

		BoxCornerSize worldBox;

		RArray<Octree> octreeArray;
		RArray<RArray<RArray<int32_t>>> chunks;

		DArray<Brick> bricks;

		unordered_map<uint64_t, uint32_t> probesLookup;
		DArray<Prob> probesArray;
		
#ifdef _DEV

	public:
		void DebugSetState(DebugState state);

	private:
		int32_t debugGeomHandle;

#endif

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

		inline uint64_t PosToUint(const Vector3 pos)
		{
			int32_t x = (int32_t)roundf(pos.x * 1000.0f);
			int32_t y = (int32_t)roundf(pos.y * 1000.0f);
			int32_t z = (int32_t)roundf(pos.z * 1000.0f);

			return (((uint64_t)z) << 32) + (((uint64_t)y) << 16) + ((uint64_t)x);
		}
	};

}