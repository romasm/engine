#pragma once

#include "Common.h"
#include "LightBuffers.h"
#include "MeshMgr.h"
#include "DebugDrawer.h"
#include "Compute.h"

#define OCTREE_DEPTH 7
#define DEFAULT_OCTREE_VOXEL_SIZE 0.1f
#define OCTREE_INTERSECT_TOLERANCE 0.1f
#define PROB_INTERPOLATION_OFFSET_SIZE 0.0001f

#define BRICK_RESOLUTION 3
#define BRICK_COEF_COUNT 9

#define PROB_CAPTURE_NEARCLIP 0.005f
#define PROB_CAPTURE_FARCLIP 10000.0f

#define PROB_CAPTURE_OFFSET_VOXEL_RES 11

#define PROB_CAPTURE_BOUNCES 4
#define PROB_CAPTURE_AA 8
#define PROB_CAPTURE_AA_OFFSET 0.5f

#define PROB_CAPTURE_RESOLUTION_B0 128
#define PROB_CAPTURE_RESOLUTION_B1 128
#define PROB_CAPTURE_RESOLUTION_B2 128
#define PROB_CAPTURE_RESOLUTION_B3 256

#define DEBUG_MATERIAL_PROBES "$" PATH_SHADERS "objects/editor/debug_probes"
#define SHADER_CUBEMAP_TO_SH PATH_SHADERS "offline/cubemap_to_sh", "ComputeSH"
#define SHADER_BRICKS_COPY PATH_SHADERS "offline/bricks_copy", "Copy3D"
#define SHADER_INTERPOLATE_PROBES PATH_SHADERS "offline/interpolate_probes", "Interpolate"

#define GI_BRICKS_ATLAS_PATH "/gi_bricks" EXT_TEXTURE
#define GI_BRICKS_LOOKUP_PATH "/gi_lookups" EXT_TEXTURE
#define GI_CHUNKS_PATH "/gi_chunks" EXT_TEXTURE

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
		float pixelCountRcp;
		float _padding0;
		float _padding1;
		float _padding2;
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

		static void Swap(Octree& first, Octree& second)
		{
			swap(first.depth, second.depth);
			swap(first.bbox, second.bbox);
			swap(first.lookupRes, second.lookupRes);
			swap(first.parentChunk, second.parentChunk);
			swap(first.lookupAdress, second.lookupAdress);

			int32_t* temp = first.lookup;
			first.lookup = second.lookup;
			second.lookup = temp;
		}

		~Octree()
		{
			_DELETE_ARRAY(lookup);
		}
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
		Vector3 posAA[PROB_CAPTURE_AA];
		bool bake;
		bool geomCloseProbe;
		uint8_t minDepth;
		uint8_t maxDepth;
		uint8_t neighborFlags;
		DArray<Vector3Uint32> adresses;
	};

	struct ProbInterpolation
	{
		int32_t probID;
		uint8_t minDepth;
		Vector3 lerpOffset;
	};

	struct ProbInterpolationGPU
	{
		Vector4 pos;
		Vector4 offset;
		Vector4 targetAdresses[48]; // targetAdresses[0].w == count
	};

	class GIMgr
	{
	public:
		enum DebugState
		{
			DS_NONE = 0,
			DS_OCTREE,
			DS_PROBES,
			DS_VOXELS
		};

		GIMgr(class BaseWorld* wrd);
		~GIMgr();

		void LoadGIData(GISampleData& giData);
		GISampleData* SaveGIData();

		ID3D11ShaderResourceView* GetGIBricksSRV();
		ID3D11ShaderResourceView* GetGIChunksSRV();
		ID3D11ShaderResourceView* GetGILookupsSRV();

		ID3D11Buffer* GetGISampleData() { return sampleDataGPU; }

		bool BakeGI();
		
		static bool CompareOctrees(Octree& first, Octree& second);
		static void SwapOctrees(Octree* first, Octree* second, RArray<RArray<RArray<int32_t>>>* arr);

		static bool CompareInterpolationLinks(ProbInterpolation& first, ProbInterpolation& second);

	private:
		bool InitBuffers();
		bool RecreateResources();
		void DeleteResources();

		BaseWorld* world;

		uint32_t bricksAtlasResource;
		uint32_t bricksLookupsResource;
		uint32_t chunksResource;

		GISampleData sampleData;
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

		bool SceneBoxIntersect(BoundingBox& bbox);
		void ProcessOctreeBranch(Octree& octree, BoundingBox& bbox, int32_t octreeDepth,
			Vector3& octreeHelper, Vector3& octreeCorner);

		template<class voxelGrid>
		void AdjustProbPos(Prob& prob, voxelGrid& voxels, voxelGrid& meshVoxels);
		
		void InterpolateProbes(RArray<ProbInterpolation>& interpolationArray);
		uint8_t GetNeighborFlag(int32_t i);
		Vector3 GetOutterVectorFromNeighbors(uint8_t flags);
		void CopyBricks();
		
		Compute* cubemapToSH;
		ID3D11Buffer* adressBuffer;

		Compute* copyBricks;
		Compute* interpolateProbes;

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

		RArray<Vector3> probOffsetsArray;
		DArray<VoxelizeSceneItem> staticScene;

#ifdef _DEV

	public:
		void DebugSetState(DebugState state);

	private:
		int32_t debugGeomHandleOctree;
		int32_t debugGeomHandleProbes;
		int32_t debugGeomHandleVoxel;

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

#define PROB_MIDDLE_ID 13

#define PROB_FACE_ID_Zm 4
#define PROB_FACE_ID_Ym 10
#define PROB_FACE_ID_Xm 12
#define PROB_FACE_ID_Xp 14
#define PROB_FACE_ID_Yp 16
#define PROB_FACE_ID_Zp 22

#define PROB_SIDE_ID_Ym_Zm 1
#define PROB_SIDE_ID_Xm_Zm 3
#define PROB_SIDE_ID_Xp_Zm 5
#define PROB_SIDE_ID_Yp_Zm 7
#define PROB_SIDE_ID_Xm_Ym 9
#define PROB_SIDE_ID_Xp_Ym 11
#define PROB_SIDE_ID_Xm_Yp 15
#define PROB_SIDE_ID_Xp_Yp 17
#define PROB_SIDE_ID_Ym_Zp 19
#define PROB_SIDE_ID_Xm_Zp 21
#define PROB_SIDE_ID_Xp_Zp 23
#define PROB_SIDE_ID_Yp_Zp 25

#define PROB_CORNER_ID_Xm_Ym_Zm 0
#define PROB_CORNER_ID_Xp_Ym_Zm 2
#define PROB_CORNER_ID_Xm_Yp_Zm 6
#define PROB_CORNER_ID_Xp_Yp_Zm 8
#define PROB_CORNER_ID_Xm_Ym_Zp 18
#define PROB_CORNER_ID_Xp_Ym_Zp 20
#define PROB_CORNER_ID_Xm_Yp_Zp 24
#define PROB_CORNER_ID_Xp_Yp_Zp 26
	};
}