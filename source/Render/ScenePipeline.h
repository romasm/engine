#pragma once

#include "Common.h"
#include "Render.h"
#include "RenderTarget.h"
#include "CubeRenderTarget.h"
#include "ScreenPlane.h"
#include "LocalTimer.h"
#include "GaussianBlur.h"
#include "Compute.h"
#include "ECS\CameraSystem.h"
#include "GIMgr.h"

namespace EngineCore
{

#define AO_TYPE 0
#define AO_FILTER

#define RENDERING_TRANSPARENT_CHANNEL_COUNT 3

#define SP_MATERIAL_DEPTH_OPAC_DIR PATH_SHADERS "system/depth_copy_hiz"

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

#define SHADER_DEFFERED_OPAQUE_IBL PATH_SHADERS "system/deffered_opaque_simple", "DefferedLightingIBL"
#define SHADER_DEFFERED_OPAQUE_FULL PATH_SHADERS "system/deffered_opaque", "DefferedLighting"
	
	struct RenderConfig
	{
		uint32_t bufferViewMode;
		uint32_t voxelViewMode;
		uint32_t voxelCascade;
		bool bloomEnable;
		bool cameraAdoptEnable;
		float cameraConstExposure;
		float analyticLightDiffuse;
		float analyticLightSpecular;
		float ambientLightDiffuse;
		float ambientLightSpecular;
		bool editorGuiEnable;
		bool active;

		bool _dirty;

		RenderConfig()
		{
			bufferViewMode = 0;
			voxelViewMode = 0;
			voxelCascade = 0;
			bloomEnable = true;
			cameraAdoptEnable = true;
			cameraConstExposure = 1.0f;
			analyticLightDiffuse = 1.0f;
			analyticLightSpecular = 1.0f;
			ambientLightDiffuse = 1.0f;
			ambientLightSpecular = 1.0f;
			editorGuiEnable = true;
			active = true;

			_dirty = true;
		}

#define RC_ADD_LUA_PROPERTY_FUNC(type, name) inline type get_##name() const {return name;} \
	inline void set_##name(type value){name = value; _dirty = true;}
#define RC_ADD_LUA_PROPERTY_DEF(type, name) .addProperty(#name, &RenderConfig::get_##name, &RenderConfig::set_##name)

		RC_ADD_LUA_PROPERTY_FUNC(uint32_t, bufferViewMode)
		RC_ADD_LUA_PROPERTY_FUNC(uint32_t, voxelViewMode)
		RC_ADD_LUA_PROPERTY_FUNC(uint32_t, voxelCascade)
		RC_ADD_LUA_PROPERTY_FUNC(bool, bloomEnable)
		RC_ADD_LUA_PROPERTY_FUNC(bool, cameraAdoptEnable)
		RC_ADD_LUA_PROPERTY_FUNC(float, cameraConstExposure)
		RC_ADD_LUA_PROPERTY_FUNC(float, analyticLightDiffuse)
		RC_ADD_LUA_PROPERTY_FUNC(float, analyticLightSpecular)
		RC_ADD_LUA_PROPERTY_FUNC(float, ambientLightDiffuse)
		RC_ADD_LUA_PROPERTY_FUNC(float, ambientLightSpecular)
		RC_ADD_LUA_PROPERTY_FUNC(bool, editorGuiEnable)
		RC_ADD_LUA_PROPERTY_FUNC(bool, active)

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<RenderConfig>("RenderConfig")
				RC_ADD_LUA_PROPERTY_DEF(uint32_t, bufferViewMode)
				RC_ADD_LUA_PROPERTY_DEF(uint32_t, voxelViewMode)
				RC_ADD_LUA_PROPERTY_DEF(uint32_t, voxelCascade)
				RC_ADD_LUA_PROPERTY_DEF(bool, bloomEnable)
				RC_ADD_LUA_PROPERTY_DEF(bool, cameraAdoptEnable)
				RC_ADD_LUA_PROPERTY_DEF(float, cameraConstExposure)
				RC_ADD_LUA_PROPERTY_DEF(float, analyticLightDiffuse)
				RC_ADD_LUA_PROPERTY_DEF(float, analyticLightSpecular)
				RC_ADD_LUA_PROPERTY_DEF(float, ambientLightDiffuse)
				RC_ADD_LUA_PROPERTY_DEF(float, ambientLightSpecular)
				RC_ADD_LUA_PROPERTY_DEF(bool, editorGuiEnable)
				RC_ADD_LUA_PROPERTY_DEF(bool, active)
				.endClass();
		}
	};

	class ScenePipeline
	{
		struct SharedBuffer
		{
			XMMATRIX viewProjection;
			XMMATRIX invViewProjection;
			XMMATRIX view;
			XMMATRIX projection;

			Vector3 CamPos;
			int screenW;

			int screenH;
			Vector3 CamDir;

			float time; 
			float dt; 
			Vector2 PixSize; 

			Vector4 maxScreenCoords;	

			Vector3 CamTangent; 
			float mipCount; 

			Vector3 CamBinormal; 
			float perspParam;

			Vector2 uvCorrectionForPow2;
			Vector2 padding0;

			// 0 ---- 1
			// |      |
			// 2 ---- 3
			Vector3 g_CamFrust0;
			float g_nearMulFar;
			Vector3 g_CamFrust1;
			float g_far;
			Vector3 g_CamFrust2;
			float g_farMinusNear;
			Vector3 g_CamFrust3;
			float g_near;
		};

		struct DefferedConfigData
		{
			float dirDiff;
			float dirSpec;
			float indirDiff;
			float indirSpec;
			
			float isLightweight;
			float _padding0;
			float _padding1;
			float _padding2;
		};
		
	public:
		ScenePipeline();
		~ScenePipeline();

		inline void ResolveShadowmaps() 
		{
			render_mgr->shadowsRenderer->ResolveShadowMaps();
		}

		inline bool IsLighweight() {return isLightweight;}
		
		bool UIStage();
		void UIOverlayStage();

		void OpaqueForwardStage();
		void TransparentForwardStage();

		void OpaqueDefferedStage();
		void HDRtoLDRStage();
		//void AllCombineStage();

		void LinearAndDepthToRT(RenderTarget* rt, ScreenPlane* sp);
		ID3D11ShaderResourceView* GetLinearAndDepthSRV();

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

		void Close();

		bool Init(BaseWorld* wrd, int t_width, int t_height, bool lightweight);

		bool Resize(int t_width, int t_height);

		ID3D11ShaderResourceView* GetOpaqueSceneDepth() const 
		{return rt_OpaqueForward->GetShaderResourceView(4);}

		inline CameraLink GetSceneCamera() const {return camera;}
		void SetCamera(CameraLink cam);

		inline SceneRenderMgr* GetRenderMgr() const {return render_mgr;}

		bool StartFrame(LocalTimer* timer);
		void EndFrame();
		
		uint32_t LoadLights(uint32_t startOffset, bool isCS);

		bool SaveScreenshot(string path, uint32_t targetX, uint32_t targetY);

		RenderConfig* GetConfig() {return &renderConfig;}
		bool ApplyConfig();
		bool IsActive() const {return renderConfig.active;}

		static void RegLuaClass()
		{
			RenderConfig::RegLuaClass();

			getGlobalNamespace(LSTATE)
				.beginClass<ScenePipeline>("ScenePipeline")
				.addFunction("Resize", &ScenePipeline::Resize)
				.addFunction("GetCamera", &ScenePipeline::GetSceneCamera)
				.addFunction("GetSRV", &ScenePipeline::GetSRV)
				.addFunction("GetShadowSRV", &ScenePipeline::GetShadowSRV)
				.addFunction("GetAlbedoSRV", &ScenePipeline::GetAlbedoSRV)
				.addFunction("GetAOSRV", &ScenePipeline::GetAOSRV)
				.addFunction("GetEmissiveSRV", &ScenePipeline::GetEmissiveSRV)
				.addFunction("GetNormalSRV", &ScenePipeline::GetNormalSRV)
				.addFunction("GetSpecularSRV", &ScenePipeline::GetSpecularSRV)
				.addFunction("GetSubsurfaceSRV", &ScenePipeline::GetSubsurfaceSRV)
				.addFunction("GetConfig", &ScenePipeline::GetConfig)
				.addFunction("SaveScreenshot", &ScenePipeline::SaveScreenshot)
				.endClass();
		}

		RenderTarget *rt_OpaqueForward;
		RenderTarget *rt_AO;
		RenderTarget *rt_OpaqueDefferedDirect;
		RenderTarget *rt_OpaqueFinal;
		RenderTarget *rt_HiZDepth;
		RenderTarget *rt_TransparentForward;
		RenderTarget *rt_TransparentPrepass;
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

		StructBuf lightSpotBuffer;
		StructBuf lightDiskBuffer;
		StructBuf lightRectBuffer;
		StructBuf lightPointBuffer;
		StructBuf lightSphereBuffer;
		StructBuf lightTubeBuffer;
		StructBuf lightDirBuffer;

		StructBuf casterSpotBuffer;
		StructBuf casterDiskBuffer;
		StructBuf casterRectBuffer;
		StructBuf casterPointBuffer;
		StructBuf casterSphereBuffer;
		StructBuf casterTubeBuffer;

		StructBuf lightsPerTile;
		LightsIDs lightsIDs;

		ID3D11Buffer* lightsPerTileCount;
		LightsCount lightsCount;

		StructBuf m_MaterialBuffer;

		GIMgr* giMgr;
		
		ALIGNED_ALLOCATION

	protected:
		bool InitRts();
		bool InitAvgRt();
		bool InitDepth();
		void CloseRts();
		void CloseAvgRt();
		void HiZMips();

		uint32_t textureIBLLUT;

		ID3D11Texture2D* sceneDepth;
		ID3D11ShaderResourceView* sceneDepthSRV;
		ID3D11DepthStencilView* sceneDepthDSV;

		ID3D11Texture2D* transparencyDepth;
		ID3D11ShaderResourceView* transparencyDepthSRV;
		ID3D11DepthStencilView* transparencyDepthDSV;
		
		Compute* defferedOpaqueCompute;
		DefferedConfigData defferedConfigData;
		ID3D11Buffer* defferedConfigBuffer;
		
		CameraLink camera;
		CameraComponent* current_camera;
		int32_t width;
		int32_t height;
		int32_t width_pow2;
		int32_t height_pow2;
		
		bool isLightweight;
		RenderConfig renderConfig;

		ShaderCodeMgr* codemgr;
	};
}