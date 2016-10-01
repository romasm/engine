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
		Compute(string shader)
		{
			shaderID = SHADERCODE(shader, SHADER_CS);
		}

		~Compute()
		{
			SHADERCODE_DROP(shaderID, SHADER_CS);
		}

		void Dispatch(uint32_t x, uint32_t y, uint32_t z)
		{
			auto& Hcode = ShaderCodeMgr::GetShaderCodeRef(shaderID);
			Render::CSSetShader((ID3D11ComputeShader*)Hcode.code, nullptr, 0);
			CONTEXT->Dispatch(x, y, z);
		}

	private:
		uint16_t shaderID;
	};
}