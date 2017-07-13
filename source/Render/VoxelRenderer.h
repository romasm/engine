#pragma once

#include "Common.h"
#include "Compute.h"
#include "LightBuffers.h"

#define COMPUTE_VOXEL_INJECT_LIGHT PATH_SHADERS "system/voxel_light_inject", "InjectLightToVolume"
#define COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE PATH_SHADERS "system/voxel_downsample", "DownsampleEmittance"
#define COMPUTE_VOXEL_DOWNSAMPLE_MOVE PATH_SHADERS "system/voxel_downsample", "DownsampleMove"

#define VCT_VOLUME_RES 64
#define VCT_CLIPMAP_COUNT 6
#define VCT_SUBSAMPLES 8
#define VCT_VOLUME_SIZE 8.0f
#define VCT_BACK_VOXEL_COUNT 2

#define VCT_VOLUME_RES 64
#define VCT_CLIPMAP_COUNT 6
#define VCT_MIPMAP_COUNT 6
#define VCT_MAX_COUNT 20
#define VCT_SUBSAMPLES 8
#define VCT_VOLUME_SIZE 8.0f

#define VCT_MESH_MAX_COUNT 4096
#define VCT_MESH_MAX_INSTANCE 128

namespace EngineCore
{
	struct VolumeMatrix
	{
		XMMATRIX volumeVP[VCT_MAX_COUNT][3];
	};

	struct VolumeData
	{
		Vector3 cornerOffset;
		float worldSize;
		
		float scaleHelper;
		uint32_t volumeRes;
		float voxelSize;
		float voxelSizeRcp;

		float voxelDiag;
		float voxelDiagRcp;
		Vector2 levelOffset;
		
		Vector3 volumeOffset;
		float worldSizeRcp;

		Vector2 levelOffsetTex;
		float _padding0;
		float _padding1;
	};

	struct VolumeTraceData
	{
		uint32_t maxLevel;
		uint32_t levelsCount;
		float xVolumeSizeRcp;
		uint32_t clipmapCount;
	};

	struct VolumeDownsample
	{
		Vector3 writeOffset;
		uint32_t currentLevel;

		uint32_t currentRes;
		Vector3 isShifted;
	};

	class SceneRenderMgr;

	class VoxelRenderer
	{
		struct VCTRenderMesh
		{
			uint32_t index_count; 
			ID3D11Buffer* vertex_buffer; 
			ID3D11Buffer* index_buffer; 
			uint32_t vertex_size;
			Material* material;

			uint32_t meshHash;

			uint32_t arrayID;

			VCTRenderMesh()
			{
				index_count = 0;
				vertex_buffer = nullptr;
				index_buffer = nullptr;
				vertex_size = 0;
				material = nullptr;
				meshHash = 0;
				arrayID = 0;
			}
		};

		struct VCTInstanceGroup
		{
			VCTRenderMesh* meshData;
			uint32_t matrixStart;
			uint32_t instanceCount;

			VCTInstanceGroup()
			{
				meshData = nullptr;
				matrixStart = 0;
				instanceCount = 0;
			}
		};

	public:
		VoxelRenderer(SceneRenderMgr* rndm);
		~VoxelRenderer();

		void ClearPerFrame();

		void VoxelizeScene();
		void ProcessEmittance();

		inline void RegisterSpotCaster(SpotVoxelBuffer& data)
		{spotVoxel_array.push_back(data);}
		inline void RegisterPointCaster(PointVoxelBuffer& data)
		{pointVoxel_array.push_back(data);}
		inline void RegisterDirCaster(DirVoxelBuffer& data)
		{dirVoxel_array.push_back(data);}
		
		inline ID3D11ShaderResourceView* GetVoxelSRV() const {return voxelSceneSRV;}
		inline ID3D11ShaderResourceView* GetVoxelColor0SRV() const {return voxelSceneColor0SRV;}
		inline ID3D11ShaderResourceView* GetVoxelColor1SRV() const {return voxelSceneColor1SRV;}
		inline ID3D11ShaderResourceView* GetVoxelNormalSRV() const {return voxelSceneNormalSRV;}

		inline ID3D11ShaderResourceView* GetVoxelEmittanceSRV() const {return voxelEmittanceSRV;}

		inline ID3D11Buffer* GetVolumeBuffer() const {return volumeDataBuffer;}
		inline ID3D11Buffer* GetVolumeTraceBuffer() const {return volumeTraceDataBuffer;}

		void RegMeshForVCT(uint32_t& index_count, uint32_t&& vertex_size, ID3D11Buffer* index_buffer, ID3D11Buffer* vertex_buffer, Material* material, StmMatrixBuffer& matrixData, BoundingOrientedBox& bbox);

		void CalcVolumeBox(XMVECTOR& camPos, XMVECTOR& camDir);
		inline BoundingOrientedBox& GetBigVolumeBox() {return volumesConfig[clipmapCount - 1].volumeBox;}

	private:
		bool initVoxelBuffers();
		void prepareMeshData();
		void updateBuffers();

		static bool CompareMeshes(VCTRenderMesh& a, VCTRenderMesh& b);
		static void SwapMeshes(VCTRenderMesh* first, VCTRenderMesh* second, SArray<VCTRenderMesh, VCT_MESH_MAX_COUNT>* meshArr, 
			SArray<StmMatrixBuffer, VCT_MESH_MAX_COUNT>* matrixArr);
		
		inline uint32_t calcMeshHash(VCTRenderMesh* meshPtr)
		{
			uint64_t meshHash = reinterpret_cast<uint64_t>( meshPtr->index_buffer ) + 
				reinterpret_cast<uint64_t>( meshPtr->vertex_buffer ) + 
				reinterpret_cast<uint64_t>( meshPtr->material );
			return static_cast<uint32_t>( meshHash ^ (meshHash >> 32));
		}

		void calcVolumesConfigs();

		RArray<SArray<VCTRenderMesh, VCT_MESH_MAX_COUNT>> meshesToRender;
		RArray<SArray<StmMatrixBuffer, VCT_MESH_MAX_COUNT>> matrixPerMesh;
		RArray<SArray<VCTInstanceGroup, VCT_MESH_MAX_COUNT>> meshInstanceGroups;

		SArray<SpotVoxelBuffer, SPOT_VOXEL_FRAME_MAX> spotVoxel_array;
		SArray<PointVoxelBuffer, POINT_VOXEL_FRAME_MAX> pointVoxel_array;
		SArray<DirVoxelBuffer, LIGHT_DIR_FRAME_MAX> dirVoxel_array;

		StructBuf spotLightInjectBuffer;
		StructBuf pointLightInjectBuffer;
		StructBuf dirLightInjectBuffer;

		Compute* voxelInjectLight;
		Compute* voxelDownsample[4];
		Compute* voxelDownsampleMove[4];

		ID3D11Texture2D* voxelizationDumb;
		ID3D11RenderTargetView* voxelizationDumbRTV;

		ID3D11Texture3D* voxelScene;
		ID3D11UnorderedAccessView* voxelSceneUAV;
		ID3D11ShaderResourceView* voxelSceneSRV;

		ID3D11Texture3D* voxelSceneColor0;
		ID3D11UnorderedAccessView* voxelSceneColor0UAV;
		ID3D11ShaderResourceView* voxelSceneColor0SRV;
		ID3D11Texture3D* voxelSceneColor1;
		ID3D11UnorderedAccessView* voxelSceneColor1UAV;
		ID3D11ShaderResourceView* voxelSceneColor1SRV;

		ID3D11Texture3D* voxelSceneNormal;
		ID3D11UnorderedAccessView* voxelSceneNormalUAV;
		ID3D11ShaderResourceView* voxelSceneNormalSRV;
		
		ID3D11Texture3D* voxelEmittance;
		ID3D11UnorderedAccessView* voxelEmittanceUAV;
		ID3D11ShaderResourceView* voxelEmittanceSRV;

		ID3D11Texture3D* voxelDownsampleTemp;
		ID3D11UnorderedAccessView* voxelDownsampleTempUAV;
		ID3D11ShaderResourceView* voxelDownsampleTempSRV;
		
		ID3D11Buffer* volumeMatBuffer;
		ID3D11Buffer* volumeDataBuffer;
		ID3D11Buffer* volumeTraceDataBuffer;
		ID3D11Buffer* levelBuffer;

		ID3D11Buffer* volumeLightInfo;
		ID3D11Buffer* volumeDownsampleBuffer;

		D3D11_VIEWPORT viewport;

		VolumeData volumeData[VCT_MAX_COUNT];

		ID3D11Buffer* instanceMatrixBuffer;
		
		struct VolumeConfig
		{
			Vector3 corner;
			float worldSize;
			float voxelSize;
			BoundingOrientedBox volumeBox;
			VolumeConfig() : corner(0,0,0), worldSize(0), voxelSize(0) 
			{}
		};
		RArray<VolumeConfig> volumesConfig;

		uint16_t volumeResolution;
		float volumeSize;
		uint16_t clipmapCount;
		uint16_t mipmapCount;
		uint16_t AAquality;

		uint16_t injectGroupsCount[3];

		SceneRenderMgr* render_mgr;
	};

}