#pragma once

#include "Common.h"
#include "Render.h"
#include "Material.h"
#include "RenderTarget.h"
#include "RenderState.h"
#include "LightBuffers.h"
#include "Compute.h"
#include "ShadowsRenderer.h"
#include "Entity.h"
#include "MeshLoader.h"
#include "ECS/EnvProbSystem.h"

namespace EngineCore
{
	class BaseRenderMgr
	{
	public:
		struct RenderMesh
		{
			uint32_t indexCount; 
			ID3D11Buffer* vertexBuffer; 
			ID3D11Buffer* indexBuffer; 
			void* gpuMatrixBuffer;
			uint32_t isSkinned;
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
				isSkinned = 0;
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
	class EnvProbMgr;

	class SceneRenderMgr: public BaseRenderMgr
	{
	public:
		SceneRenderMgr(bool lightweight);
		~SceneRenderMgr();

		bool RegMesh(uint32_t indexCount, ID3D11Buffer* indexBuffer, ID3D11Buffer* vertexBuffer, 
			uint32_t vertexSize, bool isSkinned, void* gpuMatrixBuffer, Material* material, Vector3& center, IA_TOPOLOGY topo = IA_TOPOLOGY::TRISLIST);
		bool RegMesh(uint32_t indexCount, ID3D11Buffer* indexBuffer, ID3D11Buffer* vertexBuffer, 
			uint32_t vertexSize, bool isSkinned, void* gpuMatrixBuffer, Material* material, IA_TOPOLOGY topo = IA_TOPOLOGY::TRISLIST);
		
		bool RegSpotLight(uint8_t type, Vector4& color, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir, 
			Vector3& up, Vector3& side, Vector3& virtpos);
		bool RegPointLight(uint8_t type, Vector4& color, float range, Vector3& area, Vector3& pos, Vector3& dir);

		bool RegDirLight(Vector4& color, Vector2& area, Vector3& dir, XMMATRIX* view_proj, Vector3* pos, uint64_t id);

		bool RegSpotCaster(uint8_t type, Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector2& cone, Vector3& pos, 
			Vector3& dir, Vector3& up, Vector3& side, Vector3& virtpos, Vector4& farNear, CXMMATRIX vp, CXMMATRIX proj, uint64_t id);
		bool RegPointCaster(uint8_t type, Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector3& pos, Vector3& dir, 
			Vector4& farNear, CXMMATRIX proj, CXMMATRIX view, uint64_t id);
		
		void RegEnvProb(const EnvProbData& data);

		void PrepassOpaque();
		void DrawOpaque();

		void PrepassTransparent();
		void DrawTransparent();

		//void DrawAlpha(); // TODO

		void DrawHud();
		void DrawOvHud();

		void ClearAll()
		{
			BaseRenderMgr::ClearAll();
			ovhud_array.clear();
			hud_array.clear();

			cleanRenderArrayLights();
		}

		void UpdateCamera(CameraComponent* cam);
		
		inline CameraComponent* GetCurrentCamera() const {return currentCamera;} 
		
		inline SpotLightBuffer* GetSpotLightDataPtr(size_t* size) 
		{*size = lightSpot_count; return lightSpot_array;}

		inline PointLightBuffer* GetPointLightDataPtr(size_t* size) 
		{*size = lightPoint_count; return lightPoint_array;}

		inline DirLightBuffer* GetDirLightDataPtr(size_t* size) 
		{*size = lightDir_count; return lightDir_array;}

		inline SpotCasterBuffer* GetSpotCasterDataPtr(size_t* size) 
		{*size = casterSpot_count; return casterSpot_array;}
		
		inline PointCasterBuffer* GetPointCasterDataPtr(size_t* size) 
		{*size = casterPoint_count; return casterPoint_array;}
		
		ShadowsRenderer* shadowsRenderer;
		EnvProbMgr* envProbMgr;

	private:
		void cleanRenderArrayLights();

		RArray<RenderMesh> hud_array;
		RArray<RenderMesh> ovhud_array;
		
		SpotLightBuffer* lightSpot_array;
		size_t lightSpot_count;
		PointLightBuffer* lightPoint_array;
		size_t lightPoint_count;

		DirLightBuffer* lightDir_array;
		size_t lightDir_count;

		SpotCasterBuffer* casterSpot_array;
		size_t casterSpot_count;
		PointCasterBuffer* casterPoint_array;
		size_t casterPoint_count;
		
		CameraComponent* currentCamera;
	};
}