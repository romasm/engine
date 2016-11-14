#pragma once

#include "Common.h"
#include "Compute.h"
#include "LightBuffers.h"

#define COMPUTE_VOXEL_INJECT_LIGHT PATH_SHADERS "system/voxel_light_inject", "InjectLightToVolume"
#define COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE PATH_SHADERS "system/voxel_downsample", "DownsampleEmittance"
#define COMPUTE_VOXEL_DOWNSAMPLE_MOVE PATH_SHADERS "system/voxel_downsample", "DownsampleMove"

#define VOXEL_VOLUME_RES 64
#define VOXEL_VOLUME_CLIPMAP_COUNT 6
#define VOXEL_VOLUME_SUBSAMPLES 8
#define VOXEL_VOLUME_SIZE 10.0f

namespace EngineCore
{
	struct VolumeData
	{
		XMMATRIX volumeVP[3];

		XMFLOAT3 cornerOffset;
		float worldSize;
		
		float scaleHelper;
		uint32_t volumeRes;
		uint32_t volumeDoubleRes;
		float voxelSize;

		float voxelDiag;
		float _padding0;
		float _padding1;
		float _padding2;
	};

	class SceneRenderMgr;

	class VoxelRenderer
	{
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

		inline ID3D11Buffer* GetVolumeBuffer() const {return volumeBuffer;}

	private:
		bool initVoxelBuffers();

		SArray<SpotVoxelBuffer, SPOT_VOXEL_FRAME_MAX> spotVoxel_array;
		SArray<PointVoxelBuffer, POINT_VOXEL_FRAME_MAX> pointVoxel_array;
		SArray<DirVoxelBuffer, LIGHT_DIR_FRAME_MAX> dirVoxel_array;

		StructBuf spotLightInjectBuffer;
		StructBuf pointLightInjectBuffer;
		StructBuf dirLightInjectBuffer;

		Compute* voxelInjectLight;
		Compute* voxelDownsample;
		Compute* voxelDownsampleMove;

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

		ID3D11Buffer* volumeBuffer;
		ID3D11Buffer* volumeInfo;
		ID3D11Buffer* volumeDownsampleBuffer;

		SceneRenderMgr* render_mgr;
	};

}