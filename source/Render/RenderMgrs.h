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

namespace EngineCore
{
	template <typename MeshClass=void> struct MeshGroup
	{
		unsigned int ID;
		XMVECTOR center;
		
		bool checked;
		bool visible;
		
		MeshClass** meshes;
		unsigned int mesh_count;
		
		MeshGroup()
		{
			ID = 0;
			meshes = nullptr;
			center = XMVectorZero();
			checked = false;
			visible = false;
			mesh_count = 0;
		}
	};

	struct RenderMesh
	{
		uint32_t index_count; 
		ID3D11Buffer* vertex_buffer; 
		ID3D11Buffer* index_buffer; 
		ID3D11Buffer* constant_buffer;
		uint32_t vertex_size;
		Material* material;
		IA_TOPOLOGY topo;

		MeshGroup<RenderMesh>* group;

		RenderMesh()
		{
			index_count = 0;
			vertex_buffer = nullptr;
			index_buffer = nullptr;
			constant_buffer = nullptr;
			vertex_size = 0;
			material = nullptr;
			group = nullptr;
		}
	};

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
		BaseRenderMgr();
		~BaseRenderMgr()
		{ClearAll();}

		inline bool IsShadow() const {return b_shadow;}

		static bool CompareMeshes(RenderMesh* first, RenderMesh* second);
		static bool InvCompareMeshes(RenderMesh* first, RenderMesh* second);

		void ZeroMeshgroups()
		{
			meshgroup_count = 0;
		}

		void ClearAll()
		{
			cleanRenderArrayOpaque();
			cleanRenderArrayAlphatest();
			cleanRenderArrayTransparenty();
		}

	protected:
		bool b_shadow;
		
		void cleanRenderArrayTransparenty();
		void cleanRenderArrayAlphatest();
		void cleanRenderArrayOpaque();

		RArray<RenderMesh*> opaque_array;
		RArray<RenderMesh*> alphatest_array;
		RArray<RenderMesh*> transparent_array;

		XMVECTOR cameraPosition;
		
		unsigned int meshgroup_count;
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

		bool RegMesh(uint32_t index_count, 
			ID3D11Buffer* vertex_buffer, ID3D11Buffer* index_buffer, ID3D11Buffer* constant_buffer,
			uint32_t vertex_size, Material* material, XMVECTOR center);
		bool RegMultiMesh(uint32_t* index_count, 
			ID3D11Buffer** vertex_buffer, ID3D11Buffer** index_buffer, ID3D11Buffer* constant_buffer,
			uint32_t vertex_size, RArray<Material*>& material, XMVECTOR center);
		
		void UpdateCamera(XMVECTOR pos) {cameraPosition = pos;}
		
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

		bool RegMesh(uint32_t index_count, ID3D11Buffer* vertex_buffer, ID3D11Buffer* index_buffer, 
			ID3D11Buffer* constant_buffer, uint32_t vertex_size, Material* material, IA_TOPOLOGY topo = IA_TOPOLOGY::TRISLIST);
		bool RegMesh(uint32_t index_count, ID3D11Buffer* vertex_buffer, ID3D11Buffer* index_buffer, 
			ID3D11Buffer* constant_buffer, uint32_t vertex_size, Material* material, XMVECTOR center, IA_TOPOLOGY topo = IA_TOPOLOGY::TRISLIST);
		
		bool RegMultiMesh(uint32_t* index_count, ID3D11Buffer** vertex_buffer, 
			ID3D11Buffer** index_buffer, ID3D11Buffer* constant_buffer, uint32_t vertex_size, RArray<Material*>& material, IA_TOPOLOGY topo = IA_TOPOLOGY::TRISLIST);
		bool RegMultiMesh(uint32_t* index_count, ID3D11Buffer** vertex_buffer, 
			ID3D11Buffer** index_buffer, ID3D11Buffer* constant_buffer, uint32_t vertex_size, RArray<Material*>& material, XMVECTOR center, IA_TOPOLOGY topo = IA_TOPOLOGY::TRISLIST);

		void RegDistEnvProb(ID3D11ShaderResourceView* specCube, ID3D11ShaderResourceView* diffCube, UINT mipsCount, CXMMATRIX envRot)
		{
			skyEP.specCube = specCube;
			skyEP.diffCube = diffCube;
			skyEP.mipsCount = mipsCount;
			skyEP.matrix = envRot;
		}

		bool RegSpotLight(XMFLOAT4 color, float range, XMFLOAT2 cone, XMFLOAT3 pos, XMFLOAT3 dir);
		bool RegSpotLightDisk(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT2 cone, XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 virtpos);
		bool RegSpotLightRect(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT2 cone, XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 up, XMFLOAT3 side, XMFLOAT3 virtpos);

		bool RegPointLight(XMFLOAT4 color, float range, XMFLOAT3 pos);
		bool RegPointLightSphere(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT3 pos);
		bool RegPointLightTube(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT3 pos, XMFLOAT3 dir);

		bool RegDirLight(XMFLOAT4 color, XMFLOAT2 area, XMFLOAT3 dir, XMMATRIX* view_proj, XMFLOAT3* pos, uint64_t id);
		
		bool RegSpotCaster(XMFLOAT4 color, float range, XMFLOAT2 cone, XMFLOAT3 pos, XMFLOAT3 dir, float nearclip, CXMMATRIX vp, CXMMATRIX proj, uint64_t id);
		bool RegSpotCasterDisk(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT2 cone, XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 virtpos, float nearclip, 
			CXMMATRIX vp, CXMMATRIX proj, uint64_t id);
		bool RegSpotCasterRect(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT2 cone, XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 up, XMFLOAT3 side, XMFLOAT3 virtpos, float nearclip,
			CXMMATRIX vp, CXMMATRIX proj, uint64_t id);

		bool RegPointCaster(XMFLOAT4 color, float range, XMFLOAT3 pos, CXMMATRIX proj, uint64_t id);
		bool RegPointCasterSphere(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT3 pos, CXMMATRIX proj, uint64_t id);
		bool RegPointCasterTube(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT3 pos, XMFLOAT3 dir, CXMMATRIX proj, CXMMATRIX view, uint64_t id);

		void DrawOpaque(ScenePipeline* scene);
		void DrawAlphatest(ScenePipeline* scene);
		void DrawTransparent(ScenePipeline* scene);
		void DrawHud();
		void DrawOvHud();

		void ClearAll()
		{
			BaseRenderMgr::ClearAll();
			cleanRenderArrayHud();
			cleanRenderArrayEnvProbs();
			cleanRenderArrayLights();
		}

		void UpdateCamera(CameraComponent* cam);
		
		inline CameraComponent* GetCurrentCamera() const {return current_cam;} 

		inline const distEP& GetDistEnvProb() const {return skyEP;}

		inline SpotLightBuffer* GetSpotLightDataPtr(size_t* size) 
		{*size = lightSpot_count; return lightSpot_array;}
		inline SpotLightDiskBuffer* GetSpotLightDiskDataPtr(size_t* size) 
		{*size = lightSpotDisk_count; return lightSpotDisk_array;}
		inline SpotLightRectBuffer* GetSpotLightRectDataPtr(size_t* size) 
		{*size = lightSpotRect_count; return lightSpotRect_array;}

		inline PointLightBuffer* GetPointLightDataPtr(size_t* size) 
		{*size = lightPoint_count; return lightPoint_array;}
		inline PointLightSphereBuffer* GetPointLightSphereDataPtr(size_t* size) 
		{*size = lightPointSphere_count; return lightPointSphere_array;}
		inline PointLightTubeBuffer* GetPointLightTubeDataPtr(size_t* size) 
		{*size = lightPointTube_count; return lightPointTube_array;}

		inline DirLightBuffer* GetDirLightDataPtr(size_t* size) 
		{*size = lightDir_count; return lightDir_array;}

		inline SpotCasterBuffer* GetSpotCasterDataPtr(size_t* size) 
		{*size = casterSpot_count; return casterSpot_array;}
		inline SpotCasterDiskBuffer* GetSpotCasterDiskDataPtr(size_t* size) 
		{*size = casterSpotDisk_count; return casterSpotDisk_array;}
		inline SpotCasterRectBuffer* GetSpotCasterRectDataPtr(size_t* size) 
		{*size = casterSpotRect_count; return casterSpotRect_array;}
		
		inline PointCasterBuffer* GetPointCasterDataPtr(size_t* size) 
		{*size = casterPoint_count; return casterPoint_array;}
		inline PointCasterSphereBuffer* GetPointCasterSphereDataPtr(size_t* size) 
		{*size = casterPointSphere_count; return casterPointSphere_array;}
		inline PointCasterTubeBuffer* GetPointCasterTubeDataPtr(size_t* size) 
		{*size = casterPointTube_count; return casterPointTube_array;}
		
		// temp
		inline RArray<RenderMesh*>& GetOpaqueArray() {return opaque_array;}

		VoxelRenderer* voxelRenderer;
		ShadowsRenderer* shadowsRenderer;

		ALIGNED_ALLOCATION

	private:
		bool regToDraw(uint32_t index_count, ID3D11Buffer* vertex_buffer, ID3D11Buffer* index_buffer, 
			ID3D11Buffer* constant_buffer, uint32_t vertex_size, Material* material, XMVECTOR center, IA_TOPOLOGY topo);
		bool regToDraw(uint32_t* index_count, ID3D11Buffer** vertex_buffer, 
			ID3D11Buffer** index_buffer, ID3D11Buffer* constant_buffer, uint32_t vertex_size, RArray<Material*>& material, XMVECTOR center, IA_TOPOLOGY topo);

		void cleanRenderArrayHud();
		void cleanRenderArrayEnvProbs() {skyEP = distEP();}

		void cleanRenderArrayLights();

		RArray<RenderMesh*> hud_array;
		RArray<RenderMesh*> ovhud_array;

		distEP skyEP;

		SpotLightBuffer* lightSpot_array;
		size_t lightSpot_count;
		SpotLightDiskBuffer* lightSpotDisk_array;
		size_t lightSpotDisk_count;
		SpotLightRectBuffer* lightSpotRect_array;
		size_t lightSpotRect_count;

		PointLightBuffer* lightPoint_array;
		size_t lightPoint_count;
		PointLightSphereBuffer* lightPointSphere_array;
		size_t lightPointSphere_count;
		PointLightTubeBuffer* lightPointTube_array;
		size_t lightPointTube_count;

		DirLightBuffer* lightDir_array;
		size_t lightDir_count;

		SpotCasterBuffer* casterSpot_array;
		size_t casterSpot_count;
		SpotCasterDiskBuffer* casterSpotDisk_array;
		size_t casterSpotDisk_count;
		SpotCasterRectBuffer* casterSpotRect_array;
		size_t casterSpotRect_count;

		PointCasterBuffer* casterPoint_array;
		size_t casterPoint_count;
		PointCasterSphereBuffer* casterPointSphere_array;
		size_t casterPointSphere_count;
		PointCasterTubeBuffer* casterPointTube_array;
		size_t casterPointTube_count;
		
		CameraComponent* current_cam;
	};
}