#pragma once

#include "Common.h"
#include "Render.h"
#include "RenderTarget.h"
#include "ScreenPlane.h"
#include "LocalTimer.h"
#include "GaussianBlur.h"
#include "Compute.h"
#include "ECS\CameraSystem.h"

namespace EngineCore
{

#define AO_TYPE 1
#define AO_FILTER

#define RENDERING_TRANSPARENT_CHANNEL_COUNT 3

#define SP_MATERIAL_DEPTH_OPAC_DIR PATH_SHADERS "system/depth_copy_hiz"

#define SP_MATERIAL_DEFFERED_OPAC_DIR PATH_SHADERS "system/deffered_opaque"
#define SP_MATERIAL_DEFFERED_OPAC_SIMPLE PATH_SHADERS "system/deffered_opaque_simple"

#define TEX_NOISE2D PATH_SYS_TEXTURES "noise2d" EXT_TEXTURE
#define TEX_PBSENVLUT PATH_SYS_TEXTURES "pbs_env_lut" EXT_TEXTURE

#define SP_MATERIAL_HBAO PATH_SHADERS "system/hbao"
#define SP_MATERIAL_HBAO_PERPECTIVE_CORRECT PATH_SHADERS "system/hbao_perspective_correct"
#define TEX_HBAO_DITHER PATH_SYS_TEXTURES "hbaoDither" EXT_TEXTURE

#if AO_TYPE == 0
	#define SP_MATERIAL_AO SP_MATERIAL_HBAO
#elif AO_TYPE == 1
	#define SP_MATERIAL_AO SP_MATERIAL_HBAO_PERPECTIVE_CORRECT
#endif

#define SP_MATERIAL_HDR PATH_SHADERS "system/hdr" 
#define SP_MATERIAL_COMBINE PATH_SHADERS "system/combine"

#define SP_MATERIAL_HIZ_DEPTH PATH_SHADERS "system/hiz_depth" 
#define SP_MATERIAL_OPAQUE_BLUR PATH_SHADERS "system/opaque_blur" 

#define SP_MATERIAL_AVGLUM PATH_SHADERS "system/avglum" 
#define SP_MATERIAL_BLOOM_FIND PATH_SHADERS "system/bloom_find" 

#define SP_MATERIAL_AA_EDGE PATH_SHADERS "system/aa_edge" 
#define SP_MATERIAL_AA_BLEND PATH_SHADERS "system/aa_blend" 
#define SP_MATERIAL_AA PATH_SHADERS "system/aa_final" 
#define TEX_SMAA_AREA PATH_SYS_TEXTURES "aa/smaa_area" EXT_TEXTURE
#define TEX_SMAA_SEARCH PATH_SYS_TEXTURES "aa/smaa_search" EXT_TEXTURE

#define SP_SHADER_SSR PATH_SHADERS "system/ssr_compute"

#define SP_SHADER_SCREENSHOT PATH_SHADERS "system/scene_to_color_alpha"

	class ScenePipeline
	{
		struct SharedBuffer
		{
			XMMATRIX viewProjection;
			XMMATRIX invViewProjection;
			XMMATRIX view;
			XMMATRIX projection;

			XMFLOAT3 CamPos;
			int screenW;

			int screenH;
			XMFLOAT3 CamDir;

			float time; 
			float dt; 
			XMFLOAT2 PixSize; 

			XMFLOAT4 maxScreenCoords;	

			XMFLOAT3 CamTangent; 
			float mipCount; 

			XMFLOAT3 CamBinormal; 
			float perspParam;

			XMFLOAT2 uvCorrectionForPow2;
			XMFLOAT2 padding0;

			// 0 ---- 1
			// |      |
			// 2 ---- 3
			XMFLOAT3 g_CamFrust0;
			float padding1;
			XMFLOAT3 g_CamFrust1;
			float padding2;
			XMFLOAT3 g_CamFrust2;
			float padding3;
			XMFLOAT3 g_CamFrust3;
			float padding4;
		};
		
	public:
		ScenePipeline();
		~ScenePipeline();

		void ResolveShadowmaps() {render_mgr->shadowsRenderer->ResolveShadowMaps();}
		
		void HudStage();

		void OpaqueForwardStage();
		void TransparentForwardStage();

		void OpaqueDefferedStage();
		void HDRtoLDRStage();
		//void AllCombineStage();

		void LinearAndDepthToRT(RenderTarget* rt, ScreenPlane* sp);

		luaSRV GetSRV()
		{
			luaSRV res;
			res.srv = rt_Antialiased->GetShaderResourceView(1);
			return res;
		}		
		luaSRV GetShadowSRV()
		{
			luaSRV res;
			res.srv = render_mgr->shadowsRenderer->GetShadowBuffer();
			return res;
		}
		luaSRV GetAOSRV() // todo: bend ao
		{
			luaSRV res;
			res.srv = rt_AO->GetShaderResourceView(0);
			return res;
		}
		luaSRV GetAlbedoSRV()
		{
			luaSRV res;
			res.srv = rt_OpaqueForward->GetShaderResourceView(0);
			return res;
		}
		luaSRV GetNormalSRV()
		{
			luaSRV res;
			res.srv = rt_OpaqueForward->GetShaderResourceView(1);
			return res;
		}
		luaSRV GetSpecularSRV()
		{
			luaSRV res;
			res.srv = rt_OpaqueForward->GetShaderResourceView(3);
			return res;
		}
		luaSRV GetEmissiveSRV()
		{
			luaSRV res;
			res.srv = rt_OpaqueForward->GetShaderResourceView(4);
			return res;
		}
		luaSRV GetSubsurfaceSRV()
		{
			luaSRV res;
			res.srv = rt_OpaqueForward->GetShaderResourceView(6);
			return res;
		}

		void SetMode(int mode)
		{
			render_mode = (uint8_t)mode;
			sp_HDRtoLDR->SetFloat((float)render_mode, 12);
		}
		int GetMode() const {return (int)render_mode;}

		void SetExposure(bool adapt, float exp)
		{
			cameraAdapt = adapt;
			cameraExposure = exp;

			if(!cameraAdapt)
			{
				rt_AvgLum->ClearRenderTargets(cameraExposure,cameraExposure,cameraExposure,cameraExposure);
				rt_AvgLumCurrent->ClearRenderTargets(cameraExposure,cameraExposure,cameraExposure,cameraExposure);
			}
		}

		void Close();

		bool Init(int t_width, int t_height, bool lightweight);

		bool Resize(int t_width, int t_height);

		ID3D11ShaderResourceView* GetOpaqueSceneDepth() const 
		{return rt_OpaqueForward->GetShaderResourceView(4);}

		inline CameraLink GetSceneCamera() const {return camera;}
		void SetCamera(CameraLink cam);

		inline SceneRenderMgr* GetRenderMgr() const {return render_mgr;}

		void SetComponents(bool dirSpec, bool indirSpec, bool dirDiff, bool indirDiff) // TODO
		{
			b_directSpec = dirSpec;
			b_indirectSpec = indirSpec;
			b_directDiff = dirDiff;
			b_indirectDiff = indirDiff;
		}

		void SetHud(bool draw)
		{
			b_renderHud = draw;
			if(!b_renderHud)
				rt_3DHud->ClearRenderTargets(false);
		}

		bool StartFrame(LocalTimer* timer);
		void EndFrame();

		void LoadEnvProbs();
		//static bool CompareEnvProbs(EnvProb* first, EnvProb* second);

		uint8_t LoadLights();

		bool SaveScreenshot(string path, float ssX, float ssY);

		RenderTarget *rt_OpaqueForward;
		RenderTarget *rt_AO;
		RenderTarget *rt_OpaqueDefferedDirect;
		RenderTarget *rt_OpaqueFinal;
		RenderTarget *rt_HiZDepth;
		RenderTarget *rt_TransparentRecursive;
		RenderTarget *rt_TransparentForward;
		RenderTarget *rt_FinalLDR;
		RenderTarget *rt_Antialiased;
		//RenderTarget *rt_LDRandHud;
		RenderTarget *rt_3DHud;
		RenderTarget *rt_AvgLum;
		RenderTarget *rt_AvgLumCurrent;

		RenderTarget *rt_SSR;

		RenderTarget *rt_Bloom;
		GaussianBlur *g_Bloom;
#ifdef AO_FILTER
		GaussianBlur *g_AO;
#endif

		ScreenPlane *sp_OpaqueDepthCopy;

		ScreenPlane *sp_OpaqueDefferedDirect;
		ScreenPlane *sp_AO;
		ScreenPlane *sp_FinalOpaque;
		ScreenPlane *sp_HDRtoLDR;
		//ScreenPlane *sp_3DHud;

		ScreenPlane *sp_HiZ;

		ScreenPlane *sp_AvgLum;
		ScreenPlane *sp_Bloom;

		ScreenPlane *sp_SSR;
		GaussianBlur *g_SSR;

		ScreenPlane *sp_Antialiased[3];
		
		MaterialParamsStructBuffer Materials[MATERIALS_COUNT];
		uint32_t Materials_Count;

		SceneRenderMgr* render_mgr;

		ID3D11Buffer* m_CamMoveBuffer;

		ID3D11Buffer* m_AOBuffer;
		//AOBuffer aoBuffer;

		ID3D11Buffer* m_SharedBuffer;
		SharedBuffer sharedconst;

		ID3D11Buffer* lightSpotBuffer;
		ID3D11Buffer* lightPointBuffer;
		ID3D11Buffer* lightDirBuffer;

		ID3D11Buffer* casterSpotBuffer;
		ID3D11Buffer* casterDiskBuffer;
		ID3D11Buffer* casterRectBuffer;
		ID3D11Buffer* casterPointBuffer;
		ID3D11Buffer* casterSphereBuffer;
		ID3D11Buffer* casterTubeBuffer;

		StructBuf m_LightShadowStructBuffer;

		StructBuf m_EnvProbBuffer;
		StructBuf m_MaterialBuffer;

		ALIGNED_ALLOCATION

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<ScenePipeline>("ScenePipeline")
					.addFunction("Resize", &ScenePipeline::Resize)
					.addFunction("GetCamera", &ScenePipeline::GetSceneCamera)
					.addFunction("SetComponents", &ScenePipeline::SetComponents)
					.addFunction("SetHud", &ScenePipeline::SetHud)
					.addFunction("GetSRV", &ScenePipeline::GetSRV)
					.addFunction("GetShadowSRV", &ScenePipeline::GetShadowSRV)
					.addFunction("GetAlbedoSRV", &ScenePipeline::GetAlbedoSRV)
					.addFunction("GetAOSRV", &ScenePipeline::GetAOSRV)
					.addFunction("GetEmissiveSRV", &ScenePipeline::GetEmissiveSRV)
					.addFunction("GetNormalSRV", &ScenePipeline::GetNormalSRV)
					.addFunction("GetSpecularSRV", &ScenePipeline::GetSpecularSRV)
					.addFunction("GetSubsurfaceSRV", &ScenePipeline::GetSubsurfaceSRV)
					.addProperty("mode", &ScenePipeline::GetMode, &ScenePipeline::SetMode)

					.addFunction("SaveScreenshot", &ScenePipeline::SaveScreenshot)
					.addFunction("SetExposure", &ScenePipeline::SetExposure)
				.endClass();
		}

	protected:
		bool InitRts();
		bool InitAvgRt();
		bool InitDepth();
		void CloseRts();
		void CloseAvgRt();
		void HiZMips();

		ID3D11Texture2D* sceneDepth;
		ID3D11ShaderResourceView* sceneDepthSRV;
		ID3D11DepthStencilView* sceneDepthDSV;
		
		CameraLink camera;
		CameraComponent* current_camera;
		int32_t width;
		int32_t height;
		int32_t width_pow2;
		int32_t height_pow2;

		bool b_directSpec, b_indirectSpec, b_directDiff, b_indirectDiff, b_renderHud;

		uint8_t render_mode;

		bool isLightweight;

		bool cameraAdapt;
		float cameraExposure;

		ShaderCodeMgr* codemgr;
	};
}