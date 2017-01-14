#pragma once
#include "stdafx.h"
#include "Common.h"
#include "MaterialData.h"

#define STATES_MAX_COUNT 64
#define STATE_NULL STATES_MAX_COUNT

#define SAMPLERS_INIT_COUNT 64

#define SAMPLERS_SOURCE PATH_SHADERS "samplers" EXT_SHADER_SOURCE
#define SAMPLERS_BIN PATH_SHADERS "samplers" EXT_SHADER_TECHS

#define SAMPLER_STR_LEN 64
#define SAMPLER_SIZE (SAMPLER_STR_LEN + sizeof(D3D11_SAMPLER_DESC))

namespace EngineCore
{
	class RenderStateMgr
	{
	public:
		RenderStateMgr();
		~RenderStateMgr();

		inline static RenderStateMgr* Get(){return instance;}

		bool SetDefault();

		static uint16_t GetDepthState(D3D11_DEPTH_STENCIL_DESC& desc);
		inline static ID3D11DepthStencilState* GetDepthStatePtr(uint16_t id){return instance->depth_array[id];}
		
		static uint16_t GetBlendState(D3D11_BLEND_DESC& desc);
		inline static ID3D11BlendState* GetBlendStatePtr(uint16_t id){return instance->blend_array[id];}
		
		static uint16_t GetRSState(D3D11_RASTERIZER_DESC& desc);
		inline static ID3D11RasterizerState* GetRSStatePtr(uint16_t id){return instance->rast_array[id];}

	private:		
		static RenderStateMgr *instance;

		SArray<ID3D11DepthStencilState*, STATES_MAX_COUNT> depth_array;
		SDeque<uint16_t, STATES_MAX_COUNT> depth_free;
		unordered_map<string, uint16_t> depth_map;
		
		SArray<ID3D11BlendState*, STATES_MAX_COUNT> blend_array;
		SDeque<uint16_t, STATES_MAX_COUNT> blend_free;
		unordered_map<string, uint16_t> blend_map;
		
		SArray<ID3D11RasterizerState*, STATES_MAX_COUNT> rast_array;
		SDeque<uint16_t, STATES_MAX_COUNT> rast_free;
		unordered_map<string, uint16_t> rast_map;
	};

	typedef ID3D11SamplerState* sampler_ptr;

	class SamplerStateMgr
	{
	public:
		SamplerStateMgr();
		~SamplerStateMgr();

		bool LoadSamplers();

		inline static SamplerStateMgr* Get(){return instance;}

		static ID3D11SamplerState* GetSampler(string name);

	private:

	#ifdef _DEV
		bool CompileSamplers(uint32_t sourceDate);
	#endif

		static SamplerStateMgr *instance;

		unordered_map<string, ID3D11SamplerState*> sampler_map;
	};
}