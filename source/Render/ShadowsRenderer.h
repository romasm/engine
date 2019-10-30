#pragma once

#include "Common.h"
#include "LightBuffers.h"
#include "ECS\Entity.h"

#define SHADOWS_BUF_RES 4096
#define SHADOWS_BUF_RES_RCP 1.0f / SHADOWS_BUF_RES
#define SHADOWS_BUF_MIPS 8 // min - 16

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

namespace EngineCore
{
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

	class SceneRenderMgr;
	class ShadowRenderMgr;

	class ShadowsRenderer
	{
	public:
		ShadowsRenderer(SceneRenderMgr* rndm);
		~ShadowsRenderer();

		void ClearPerFrame();
		
		void ResolveShadowMaps();
		inline ID3D11ShaderResourceView* GetShadowBuffer() const {return shadowsBufferSRV;}

		static bool CompareShadows(ShadowMap& first, ShadowMap& second);
		static void SwapShadows(ShadowMap* first, ShadowMap* second, SArray<ShadowMap, SHADOWMAPS_COUNT>* arr);

		bool RegShadowMap(uint32_t id,  float size);

		void RenderShadow(uint32_t id, uint8_t num, ShadowRenderMgr* shadow_mgr, ID3D11Buffer* vp);
		
		inline ShadowMap& GetShadowAdress(uint64_t id) {return shadowmap_array[castersIdx[id]];}
		inline ShadowMap& GetShadowAdressNext(ShadowMap& prev) {return shadowmap_array[prev.next];}

		inline uint16_t GetShadowCascadeRes() {return cascadeShadowRes;}

	private:
			
		bool initShadowBuffer();
		void PlaceShadowMaps();

		SArray<uint32_t, ENTITY_COUNT> castersIdx;

		SArray<ShadowMap, SHADOWMAPS_COUNT> shadowmap_array;

		uint16_t cascadeShadowRes;

		ID3D11Texture2D* shadowsBuffer;
		ID3D11ShaderResourceView* shadowsBufferSRV;
		ID3D11DepthStencilView* shadowsBufferDSV[SHADOWS_BUF_SIZE];

		struct{
			float size;
			uint16_t res;
		} shadows_sizes[SHADOWS_STRINGS_NUM];

		SceneRenderMgr* render_mgr;
	};

}