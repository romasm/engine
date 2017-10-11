#pragma once

#include "Common.h"
#include "Render.h"
#include "Material.h"
#include "RenderTarget.h"
#include "RenderState.h"
#include "LightBuffers.h"
#include "Compute.h"
#include "VoxelRenderer.h"
#include "ShadowsRenderer.h"
#include "Entity.h"
#include "MeshLoader.h"

namespace EngineCore
{
	struct distEP
	{
		ID3D11ShaderResourceView* specCube;
		ID3D11ShaderResourceView* diffCube;
		UINT mipsCount;
		XMMATRIX matrix;
		
		ALIGNED_ALLOCATION

		distEP()
		{
			specCube = nullptr;
			diffCube = nullptr;
			mipsCount = 0;
		}
	};

	class BaseRenderMgr
	{
	public:
		struct RenderMesh
		{
			uint32_t indexCount; 
			ID3D11Buffer* vertexBuffer; 
			ID3D11Buffer* indexBuffer; 
			void* gpuMatrixBuffer;
			bool isSkinned;
			uint32_t vertexSize;
			Material* material;
			IA_TOPOLOGY topo;
			float distanceSq;

			RenderMesh()
			{
				indexCount = 0;
				vertexBuffer = nullptr;
				indexBuffer = nullptr;
				gpuMatrixBuffer = nullptr;
				isSkinned = false;
				vertexSize = 0;
				material = nullptr;
				topo = IA_TOPOLOGY::TRISLIST;
				distanceSq = 0;
			}
		};

		BaseRenderMgr();
		~BaseRenderMgr()
		{ClearAll();}

		inline bool IsShadow() const {return b_shadow;}

		static bool CompareMeshes(RenderMesh& first, RenderMesh& second);
		static bool InvCompareMeshes(RenderMesh& first, RenderMesh& second);

		void ClearAll()
		{
			opaque_array.clear();
			alphatest_array.clear();
			transparent_array.clear();
		}

	protected:
		bool b_shadow;
		
		RArray<RenderMesh> opaque_array;
		RArray<RenderMesh> alphatest_array;
		RArray<RenderMesh> transparent_array;

		Vector3 cameraPosition;
	};
	
#define OPAQUE_SHADOW_MAX 2048
#define OPAQUE_SHADOW_ALPHATEST_MAX 2048
#define TRANSPARENT_SHADOW_MAX 512

	class ShadowRenderMgr: public BaseRenderMgr
	{
	public:
		ShadowRenderMgr();
		~ShadowRenderMgr()
		{ClearAll();}

		bool RegMesh(uint32_t indexCount, ID3D11Buffer* indexBuffer, ID3D11Buffer* vertexBuffer, 
			uint32_t vertexSize, bool isSkinned, void* gpuMatrixBuffer, Material* material, Vector3& center, IA_TOPOLOGY topo = IA_TOPOLOGY::TRISLIST);
		bool RegMesh(uint32_t indexCount, ID3D11Buffer* indexBuffer, ID3D11Buffer* vertexBuffer, 
			uint32_t vertexSize, bool isSkinned, void* gpuMatrixBuffer, Material* material, IA_TOPOLOGY topo = IA_TOPOLOGY::TRISLIST);
		
		void UpdateCamera(Vector3& pos) {cameraPosition = pos;}
		
		void DrawOpaque();
		void DrawAlphatest();
		void DrawTransparent();

		void ClearAll() {BaseRenderMgr::ClearAll();}

		inline bool IsTranparentShadows() const {return transparent_array.capacity() != 0;}
		void SetTranparentShadows(bool enable)
		{
			transparent_array.destroy();
			if(enable) transparent_array.create(TRANSPARENT_SHADOW_MAX);
		}
	};

#define OPAQUE_FRAME_MAX 8096
#define OPAQUE_FRAME_ALPHATEST_MAX 4096
#define TRANSPARENT_FRAME_MAX 4096
#define HUD_FRAME_MAX 2048
#define OV_HUD_FRAME_MAX 512

#define ENVPROB_FRAME_MAX 32

	struct CameraComponent;

	class SceneRenderMgr: public BaseRenderMgr
	{
	public:
		SceneRenderMgr(bool lightweight);
		~SceneRenderMgr();

		bool RegMesh(uint32_t indexCount, ID3D11Buffer* indexBuffer, ID3D11Buffer* vertexBuffer, 
			uint32_t vertexSize, bool isSkinned, void* gpuMatrixBuffer, Material* material, Vector3& center, IA_TOPOLOGY topo = IA_TOPOLOGY::TRISLIST);
		bool RegMesh(uint32_t indexCount, ID3D11Buffer* indexBuffer, ID3D11Buffer* vertexBuffer, 
			uint32_t vertexSize, bool isSkinned, void* gpuMatrixBuffer, Material* material, IA_TOPOLOGY topo = IA_TOPOLOGY::TRISLIST);

		void RegDistEnvProb(ID3D11ShaderResourceView* specCube, ID3D11ShaderResourceView* diffCube, UINT mipsCount, CXMMATRIX envRot)
		{
			skyEP.specCube = specCube;
			skyEP.diffCube = diffCube;
			skyEP.mipsCount = mipsCount;
			skyEP.matrix = envRot;
		}

		bool RegSpotLight(Vector4& color, float range, Vector2& cone, Vector3& pos, Vector3& dir);
		bool RegSpotLightDisk(Vector4& color, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir, Vector3& virtpos);
		bool RegSpotLightRect(Vector4& color, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir, Vector3& up, Vector3& side, Vector3& virtpos);

		bool RegPointLight(Vector4& color, float range, Vector3& pos);
		bool RegPointLightSphere(Vector4& color, float range, Vector3& area, Vector3& pos);
		bool RegPointLightTube(Vector4& color, float range, Vector3& area, Vector3& pos, Vector3& dir);

		bool RegDirLight(Vector4& color, Vector2& area, Vector3& dir, XMMATRIX* view_proj, Vector3* pos, uint64_t id);
		
		bool RegSpotCaster(Vector4& color, Vector4& nonAreaColor, float range, Vector2& cone, Vector3& pos, Vector3& dir, Vector4& farNear, CXMMATRIX vp, CXMMATRIX proj, uint64_t id);
		bool RegSpotCasterDisk(Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir, Vector3& virtpos, Vector4& farNear, 
			CXMMATRIX vp, CXMMATRIX proj, uint64_t id);
		bool RegSpotCasterRect(Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir, Vector3& up, Vector3& side, Vector3& virtpos, Vector4& farNear,
			CXMMATRIX vp, CXMMATRIX proj, uint64_t id);

		bool RegPointCaster(Vector4& color, Vector4& nonAreaColor, float range, Vector3& pos, Vector4& farNear, CXMMATRIX proj, uint64_t id);
		bool RegPointCasterSphere(Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector3& pos, Vector4& farNear, CXMMATRIX proj, uint64_t id);
		bool RegPointCasterTube(Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector3& pos, Vector3& dir, Vector4& farNear, CXMMATRIX proj, CXMMATRIX view, uint64_t id);

		void DrawOpaque(ScenePipeline* scene);
		void DrawAlphatest(ScenePipeline* scene);
		void PrepassTransparent(ScenePipeline* scene);
		void DrawTransparent(ScenePipeline* scene);
		void DrawHud();
		void DrawOvHud();

		void ClearAll()
		{
			BaseRenderMgr::ClearAll();
			ovhud_array.clear();
			hud_array.clear();
			skyEP = distEP();

			cleanRenderArrayLights();
		}

		void UpdateCamera(CameraComponent* cam);
		
		inline CameraComponent* GetCurrentCamera() const {return current_cam;} 

		inline const distEP& GetDistEnvProb() const {return skyEP;}

		inline SpotLightBuffer* GetSpotLightDataPtr(size_t* size) 
		{*size = lightSpot_count; return lightSpot_array;}
		inline DiskLightBuffer* GetSpotLightDiskDataPtr(size_t* size) 
		{*size = lightSpotDisk_count; return lightSpotDisk_array;}
		inline RectLightBuffer* GetSpotLightRectDataPtr(size_t* size) 
		{*size = lightSpotRect_count; return lightSpotRect_array;}

		inline PointLightBuffer* GetPointLightDataPtr(size_t* size) 
		{*size = lightPoint_count; return lightPoint_array;}
		inline SphereLightBuffer* GetPointLightSphereDataPtr(size_t* size) 
		{*size = lightPointSphere_count; return lightPointSphere_array;}
		inline TubeLightBuffer* GetPointLightTubeDataPtr(size_t* size) 
		{*size = lightPointTube_count; return lightPointTube_array;}

		inline DirLightBuffer* GetDirLightDataPtr(size_t* size) 
		{*size = lightDir_count; return lightDir_array;}

		inline SpotCasterBuffer* GetSpotCasterDataPtr(size_t* size) 
		{*size = casterSpot_count; return casterSpot_array;}
		inline DiskCasterBuffer* GetSpotCasterDiskDataPtr(size_t* size) 
		{*size = casterSpotDisk_count; return casterSpotDisk_array;}
		inline RectCasterBuffer* GetSpotCasterRectDataPtr(size_t* size) 
		{*size = casterSpotRect_count; return casterSpotRect_array;}
		
		inline PointCasterBuffer* GetPointCasterDataPtr(size_t* size) 
		{*size = casterPoint_count; return casterPoint_array;}
		inline SphereCasterBuffer* GetPointCasterSphereDataPtr(size_t* size) 
		{*size = casterPointSphere_count; return casterPointSphere_array;}
		inline TubeCasterBuffer* GetPointCasterTubeDataPtr(size_t* size) 
		{*size = casterPointTube_count; return casterPointTube_array;}
		
		// temp
		inline RArray<RenderMesh>& GetOpaqueArray() {return opaque_array;}

		VoxelRenderer* voxelRenderer;
		ShadowsRenderer* shadowsRenderer;

		ALIGNED_ALLOCATION

	private:
		void cleanRenderArrayLights();

		RArray<RenderMesh> hud_array;
		RArray<RenderMesh> ovhud_array;

		distEP skyEP;

		SpotLightBuffer* lightSpot_array;
		size_t lightSpot_count;
		DiskLightBuffer* lightSpotDisk_array;
		size_t lightSpotDisk_count;
		RectLightBuffer* lightSpotRect_array;
		size_t lightSpotRect_count;

		PointLightBuffer* lightPoint_array;
		size_t lightPoint_count;
		SphereLightBuffer* lightPointSphere_array;
		size_t lightPointSphere_count;
		TubeLightBuffer* lightPointTube_array;
		size_t lightPointTube_count;

		DirLightBuffer* lightDir_array;
		size_t lightDir_count;

		SpotCasterBuffer* casterSpot_array;
		size_t casterSpot_count;
		DiskCasterBuffer* casterSpotDisk_array;
		size_t casterSpotDisk_count;
		RectCasterBuffer* casterSpotRect_array;
		size_t casterSpotRect_count;

		PointCasterBuffer* casterPoint_array;
		size_t casterPoint_count;
		SphereCasterBuffer* casterPointSphere_array;
		size_t casterPointSphere_count;
		TubeCasterBuffer* casterPointTube_array;
		size_t casterPointTube_count;
		
		CameraComponent* current_cam;
	};
}