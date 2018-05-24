#pragma once

#include "Common.h"
#include "LightBuffers.h"
#include "MeshMgr.h"
#include "DebugDrawer.h"
#include "Compute.h"

#define OCTREE_DEPTH 7
#define DEFAULT_OCTREE_VOXEL_SIZE 0.2f
#define OCTREE_INTERSECT_TOLERANCE 0.5f

#define BRICK_RESOLUTION 3
#define BRICK_COEF_COUNT 9

#define PROB_CAPTURE_NEARCLIP 0.01f
#define PROB_CAPTURE_FARCLIP 10000.0f

#define DEBUG_MATERIAL_PROBES "$" PATH_SHADERS "objects/editor/debug_probes"
#define SHADER_CUBEMAP_TO_SH PATH_SHADERS "offline/cubemap_to_sh", "ComputeSH"
#define SHADER_BRICKS_COPY PATH_SHADERS "offline/bricks_copy", "Copy3D"

namespace EngineCore
{
	struct GISampleData
	{
		Vector3 minCorner;
		float chunkSizeRcp;
		Vector3Uint32 chunksCount;
		float minHalfVoxelSize;
		Vector3 brickAtlasOffset;
		float _padding0;
		Vector3 halfBrickVoxelSize;
		float _padding1;
		Vector3 brickSampleSize;
		float _padding2;
	};

	struct SHAdresses
	{
		Vector4 adresses[48]; // 6 depth levels * 8 corner neighbors, adresses[0].w == count
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
		int32_t* lookup;

		Vector3Uint32 parentChunk;
		Vector4Uint16 lookupAdress;

		Octree()
		{
			depth = 0;
			lookupRes = 0;
			lookup = nullptr;
		}

		~Octree()
		{
			_DELETE_ARRAY(lookup);
		}
	};

	struct BoxCornerSize
	{
		Vector3 corner;
		Vector3 size;
	};

	struct Brick
	{
		uint32_t probes[BRICK_RESOLUTION * BRICK_RESOLUTION * BRICK_RESOLUTION];
		BoundingBox bbox;
		uint32_t depth;
	};

	struct Prob
	{
		Vector3 pos;
		bool bake;
		uint8_t minDepth;
		uint8_t copyCount;
		DArray<Vector3Uint32> adresses;
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

		ID3D11ShaderResourceView* GetGIBricksSRV();
		ID3D11ShaderResourceView* GetGIChunksSRV();
		ID3D11ShaderResourceView* GetGILookupsSRV();

		ID3D11Buffer* GetGISampleData() { return sampleDataGPU; }

		bool BuildVoxelOctree();
		
		static bool CompareOctrees(Octree& first, Octree& second);
		static void SwapOctrees(Octree* first, Octree* second, RArray<RArray<RArray<int32_t>>>* arr);

	private:
		bool InitBuffers();
		bool RecreateResources();
		void DeleteResources();

		BaseWorld* world;

		uint32_t sgVolume;

		ID3D11Buffer* sampleDataGPU;

		// editor gi resources

		ID3D11Texture3D* bricksAtlas;
		ID3D11ShaderResourceView* bricksAtlasSRV;
		ID3D11UnorderedAccessView* bricksAtlasUAV;

		ID3D11Texture3D* bricksTempAtlas;
		ID3D11ShaderResourceView* bricksTempAtlasSRV;
		ID3D11UnorderedAccessView* bricksTempAtlasUAV;

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
		Vector3 AdjustProbPos(Vector3& pos);

		Compute* cubemapToSH;
		ID3D11Buffer* adressBuffer;

		Compute* copyBricks;

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
		#define BRICK_X_MASK 0x00fff000
		#define BRICK_Y_MASK 0x00000fff
		#define BRICK_DEPTH_MASK 0xff000000

		// lookup uint32: 8 bit = pow(2, depth), 12 bit = x, 12 bit = y
		inline uint32_t SetLookupNode(uint32_t x, uint32_t y, uint32_t depth)
		{
			uint32_t powDepth = (uint32_t)roundf(powf(2.0f, (float)depth));
			return ((x << BRICK_XY_BITS) & BRICK_X_MASK) + (y & BRICK_Y_MASK) + ((powDepth << BRICK_ADRESS_BITS) & BRICK_DEPTH_MASK);
		}

		inline void GetBrickAdress(uint32_t node, uint32_t& x, uint32_t& y)
		{
			x = ((node & BRICK_X_MASK) >> BRICK_XY_BITS);
			y = (node & BRICK_Y_MASK);
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