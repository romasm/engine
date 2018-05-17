#pragma once

#include "Common.h"
#include "LightBuffers.h"
#include "MeshMgr.h"
#include "DebugDrawer.h"

#define OCTREE_DEPTH 7
#define DEFAULT_OCTREE_VOXEL_SIZE 0.2f

#define BKICK_RESOLUTION 3
#define BKICK_F4_COUNT 4

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

		uint32_t parentChunk[3];

		Octree()
		{
			depth = 0;
			lookupRes = 0;
			lookup = nullptr;

			parentChunk[0] = 0;
			parentChunk[1] = 0;
			parentChunk[2] = 0;
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

		static void Swap(Octree& a, Octree& b)
		{
			swap(a.depth, b.depth);
			swap(a.bbox, b.bbox);
			swap(a.lookupRes, b.lookupRes);
			swap(a.lookup, b.lookup);
			swap(a.parentChunk[0], b.parentChunk[0]);
			swap(a.parentChunk[1], b.parentChunk[1]);
			swap(a.parentChunk[2], b.parentChunk[2]);
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
		Vector3 adress;
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
		
		static bool CompareOctrees(Octree& first, Octree& second);
		static void SwapOctrees(Octree* first, Octree* second, RArray<RArray<RArray<int32_t>>>* arr);

	private:
		bool InitBuffers();
		bool RecreateResources();
		bool DeleteResources();

		BaseWorld* world;

		uint32_t sgVolume;

		GISampleData sampleData;
		ID3D11Buffer* sampleDataGPU;

		// editor gi resources

		ID3D11Texture3D* bricksAtlas;
		ID3D11ShaderResourceView* bricksAtlasSRV;
		ID3D11UnorderedAccessView* bricksAtlasUAV;

		ID3D11Texture3D* chunksLookup;
		ID3D11ShaderResourceView* chunksLookupSRV;

		ID3D11Texture3D* bricksLookup;
		ID3D11ShaderResourceView* bricksLookupSRV;
		
		uint32_t bricksTexX;
		uint32_t bricksTexY;

		// baking

		bool SceneBoxIntersect(DArray<VoxelizeSceneItem>& staticScene, BoundingBox& bbox);
		void ProcessOctreeBranch(Octree& octree, DArray<VoxelizeSceneItem>& staticScene, BoundingBox& bbox, int32_t octreeDepth, 
			Vector3& octreeHelper, Vector3& octreeCorner);


		float voxelSize;
		float chunkSize;
		uint32_t lookupMaxSize;

		BoxCornerSize worldBox;

		RArray<Octree> octreeArray;
		RArray<RArray<RArray<int32_t>>> chunks;
		DArray<Brick> bricks;
		DArray<Prob> probesArray;

		// global temp data
		unordered_map<uint64_t, uint32_t> probesLookup;
		DArray<uint32_t> bricksLinks;

#ifdef _DEV

	public:
		void DebugSetState(DebugState state);

	private:
		int32_t debugGeomHandle;

#endif

		#define BRICK_ADRESS_BITS 24
		#define BRICK_XY_BITS 12
		#define BRICK_ADRESS_MASK 0x00ffffff
		#define BRICK_X_MASK 0x00fff000
		#define BRICK_Y_MASK 0x00000fff
		#define BRICK_LEVEL_MASK 0xff000000

		inline uint32_t SetLookupNode(uint32_t id, uint32_t level)
		{
			return (id & BRICK_ADRESS_MASK) + ((level << BRICK_ADRESS_BITS) & BRICK_LEVEL_MASK);
		}

		// lookup uint32: 8 bit - level, 12 bit - x, 12 bit - y
		inline uint32_t SetLookupNode(uint32_t x, uint32_t y, uint32_t level)
		{
			return ((x << BRICK_XY_BITS) & BRICK_X_MASK) + (y & BRICK_Y_MASK) + ((level << BRICK_ADRESS_BITS) & BRICK_LEVEL_MASK);
		}

		inline uint32_t GetBrickID(uint32_t node)
		{
			return (node & BRICK_ADRESS_MASK);
		}

		inline void GetBrickAdress(uint32_t node, uint32_t& x, uint32_t& y)
		{
			x = ((node & BRICK_X_MASK) >> BRICK_XY_BITS);
			y = (node & BRICK_Y_MASK);
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