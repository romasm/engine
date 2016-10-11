#pragma once

#include "Common.h"
#include "Render.h"
#include "Material.h"
#include "RenderTarget.h"
#include "RenderState.h"
#include "LightBuffers.h"
#include "ECS/Entity.h"

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

	struct ShadowMap
	{
		float x;
		float y;
		uint8_t dsv;

		float res;

		float size;

		uint64_t id;
		uint32_t next;
		uint32_t prev;

		ShadowMap()
		{
			x = 0;
			y = 0;
			dsv = 0;
			res = 0;
			size = 0;
			id = 0;
			next = 0;
			prev = 0;
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

		void Close()
		{
			ClearAll();
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

		void Close()
		{
			ClearAll();

		}

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

#define SHADOWS_BUF_RES 4096
#define SHADOWS_BUF_RES_RCP 1.0f / SHADOWS_BUF_RES
#define SHADOWS_BUF_SIZE 4
#define SHADOWMAPS_COUNT 1024

#define SHADOWS_MAXRES 2048
#define SHADOWS_MINRES 64
#define SHADOWS_STRINGS_NUM 6 // 1 + (log(SHADOWS_MAXRES) - log(SHADOWS_MINRES)) / log(2)

#define SHADOWS_LOD_0_SIZE 1.0f
#define SHADOWS_LOD_1_SIZE 0.8f
#define SHADOWS_LOD_2_SIZE 0.5f
#define SHADOWS_LOD_3_SIZE 0.3f
#define SHADOWS_LOD_4_SIZE 0.13f
#define SHADOWS_LOD_5_SIZE 0.05f

	// Dir light configs
#define SHADOWS_DIR_RES SHADOWS_BUF_RES / 2
#define SHADOWS_DIR_DEPTH 10000.0f

	struct CameraComponent;

	class SceneRenderMgr: public BaseRenderMgr
	{
	public:
		SceneRenderMgr();

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
		
		bool RegSpotCaster(XMFLOAT4 color, float range, XMFLOAT2 cone, XMFLOAT3 pos, XMFLOAT3 dir, CXMMATRIX vp, CXMMATRIX proj, UINT id);
		bool RegSpotCasterDisk(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT2 cone, XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 virtpos, float nearclip, 
			CXMMATRIX vp, CXMMATRIX proj, UINT id);
		bool RegSpotCasterRect(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT2 cone, XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 up, XMFLOAT3 side, XMFLOAT3 virtpos, float nearclip,
			CXMMATRIX vp, CXMMATRIX proj, UINT id);

		bool RegPointCaster(XMFLOAT4 color, float range, XMFLOAT3 pos, CXMMATRIX proj, UINT id);
		bool RegPointCasterSphere(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT3 pos, CXMMATRIX proj, UINT id);
		bool RegPointCasterTube(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT3 pos, XMFLOAT3 dir, CXMMATRIX proj, CXMMATRIX view, UINT id);

		void ResolveShadowMaps();
		ID3D11ShaderResourceView* GetShadowBuffer() const {return shadowsBufferSRV;}
		void GenerateShadowHiZ();

		static bool CompareShadows(ShadowMap& first, ShadowMap& second);
		static void SwapShadows(ShadowMap* first, ShadowMap* second, RArray<ShadowMap>* arr);

		bool RegShadowMap(uint id,  float size);

		void RenderShadow(uint id, uchar num, ShadowRenderMgr* shadow_mgr, ID3D11Buffer* vp);
		
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

		void Close()
		{
			ClearAll();

			_DELETE(lightSpot_array);
			_DELETE(lightSpotDisk_array);
			_DELETE(lightSpotRect_array);
			_DELETE(lightPoint_array);
			_DELETE(lightPointSphere_array);
			_DELETE(lightPointTube_array);
			_DELETE(lightDir_array);

			_DELETE(casterSpot_array);
			_DELETE(casterSpotDisk_array);
			_DELETE(casterSpotRect_array);
			_DELETE(casterPoint_array);
			_DELETE(casterPointSphere_array);
			_DELETE(casterPointTube_array);

			shadowmap_array.destroy();

			_RELEASE(shadowsBuffer);
			_RELEASE(shadowsBufferSRV);
			for(int i=0; i<SHADOWS_BUF_SIZE; i++)
				_RELEASE(shadowsBufferDSV[i]);
		}

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

		inline uint16_t GetShadowCascadeRes() {return cascadeShadowRes;}

		ALIGNED_ALLOCATION

	private:
		bool regToDraw(uint32_t index_count, ID3D11Buffer* vertex_buffer, ID3D11Buffer* index_buffer, 
			ID3D11Buffer* constant_buffer, uint32_t vertex_size, Material* material, XMVECTOR center, IA_TOPOLOGY topo);
		bool regToDraw(uint32_t* index_count, ID3D11Buffer** vertex_buffer, 
			ID3D11Buffer** index_buffer, ID3D11Buffer* constant_buffer, uint32_t vertex_size, RArray<Material*>& material, XMVECTOR center, IA_TOPOLOGY topo);

		bool initShadowBuffer();
		void PlaceShadowMaps();

		void cleanRenderArrayHud();
		void cleanRenderArrayEnvProbs() {skyEP = distEP();}

		void cleanRenderArrayLights()
		{
			lightSpot_count = 0;
			lightSpotDisk_count = 0;
			lightSpotRect_count = 0;
			lightPoint_count = 0;
			lightPointSphere_count = 0;
			lightPointTube_count = 0;
			lightDir_count = 0;

			casterSpot_count = 0;
			casterSpotDisk_count = 0;
			casterSpotRect_count = 0;
			casterPoint_count = 0;
			casterPointSphere_count = 0;
			casterPointTube_count = 0;

			for(auto& it: shadowmap_array)
				castersIdx[it.id] = ENTITY_COUNT;		
			shadowmap_array.clear();
		}

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

		SArray<uint32_t, ENTITY_COUNT> castersIdx;

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

		RArray<ShadowMap> shadowmap_array;

		uint16_t cascadeShadowRes;

		ID3D11Texture2D* shadowsBuffer;
		ID3D11ShaderResourceView* shadowsBufferSRV;
		ID3D11DepthStencilView* shadowsBufferDSV[SHADOWS_BUF_SIZE];
		ID3D11RenderTargetView* shadowsBufferRTV[SHADOWS_BUF_SIZE];

		CameraComponent* current_cam;

		struct{
			float size;
			uint16_t res;
		} shadows_sizes[SHADOWS_STRINGS_NUM];
	};
}