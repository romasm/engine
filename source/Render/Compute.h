#pragma once
#include "Shader.h"
#include "ShaderMgr.h"
#include "Buffer.h"
#include "TexMgr.h"
#include "Material.h"
#include "Render.h"

namespace EngineCore
{
	class Compute
	{
	public:
		Compute(string& shader) {	Init(shader); }
		Compute(char* shader) { Init(string(shader)); }

		~Compute()
		{
			SHADERCODE_DROP(shaderID, SHADER_CS);
			shaderID = SHADER_NULL;
		}

		static void Preload(string& shader)
		{
			auto res = ShaderCodeMgr::Get()->GetShaderCode(shader, SHADER_CS);
			if( res != SHADER_NULL )
				LOG("Compute shader %s preloaded", shader.c_str());
		}

		void Dispatch(uint32_t x, uint32_t y, uint32_t z)
		{
			auto& handle = ShaderCodeMgr::Get()->GetShaderCodeRef(shaderID);
			if(!handle.input.samplers.empty())
				Render::CSSetSamplers(0, (uint32_t)handle.input.samplers.size(), handle.input.samplers.data());
			Render::CSSetShader( (ID3D11ComputeShader*)handle.code, nullptr, 0 );

			CONTEXT->Dispatch(x, y, z);

			Render::CSSetShader( nullptr, nullptr, 0 );
		}

		uint16_t BindUAV(ID3D11UnorderedAccessView* uav)
		{
			CONTEXT->CSSetUnorderedAccessViews(uavs_count, 1, &uav, nullptr);
			uavs_count++;
			return uavs_count - 1;
		}

		void UnbindUAV()
		{
			ID3D11UnorderedAccessView* null_uav = nullptr;
			for(uint16_t i = 0; i < uavs_count; i++)
				CONTEXT->CSSetUnorderedAccessViews(i, 1, &null_uav, nullptr);
			uavs_count = 0;
		}

	private:
		void Init(string& shader)
		{
			uavs_count = 0;
			shaderID = ShaderCodeMgr::Get()->GetShaderCode(shader, SHADER_CS);
			if(shaderID == SHADER_NULL)
				ERR("Cant init compute shader %s !", shader.c_str());
		}

		uint16_t shaderID;
		uint16_t uavs_count;
	};
}