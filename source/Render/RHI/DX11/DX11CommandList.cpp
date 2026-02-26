#include "stdafx.h"
#include "DX11CommandList.h"
#include "ShaderCodeMgr.h"
#include "RenderState.h"
#include "MaterialData.h"

// DX11CommandList::SetPipelineState is the only method that depends on the
// higher-level ShaderCodeMgr and RenderStateMgr systems, so it lives in
// this .cpp file rather than the header.

namespace EngineCore::RHI::DX11
{

void DX11CommandList::SetPipelineState(GfxPipelineState* pso)
{
	if(!pso)
	{
		// Unbind compute shader (for cleanup after compute dispatch)
		m_context->CSSetShader(nullptr, nullptr, 0);
		return;
	}

	auto* dx11pso = Cast(pso);

	// Compute pipeline — bind CS shader + samplers only
	if(dx11pso->isCompute)
	{
		auto& shaderCS = ShaderCodeMgr::GetShaderCodeRef(dx11pso->computeShaderID);
		m_context->CSSetShader(static_cast<ID3D11ComputeShader*>(shaderCS.code), nullptr, 0);
		if(!shaderCS.input.samplers.empty())
		{
			auto count = static_cast<uint32_t>(shaderCS.input.samplers.size());
			auto* arr = ExtractSamplers(shaderCS.input.samplers.data(), count);
			m_context->CSSetSamplers(0, count, arr);
		}
		return;
	}

	// Graphics pipeline — render states + all shader stages
	m_context->OMSetDepthStencilState(static_cast<ID3D11DepthStencilState*>(RenderStateMgr::GetDepthStatePtr(dx11pso->depthState)), 1);
	m_context->OMSetBlendState(static_cast<ID3D11BlendState*>(RenderStateMgr::GetBlendStatePtr(dx11pso->blendState)), nullptr, 0xffffffff);
	m_context->RSSetState(static_cast<ID3D11RasterizerState*>(RenderStateMgr::GetRSStatePtr(dx11pso->rastState)));

	// Vertex shader (always required)
	auto& shaderVS = ShaderCodeMgr::GetShaderCodeRef(dx11pso->shadersID[SHADER_VS]);
	m_context->VSSetShader(static_cast<ID3D11VertexShader*>(shaderVS.code), nullptr, 0);
	m_context->IASetInputLayout(shaderVS.input.layout ? Cast(shaderVS.input.layout)->layout : nullptr);
	if(!shaderVS.input.samplers.empty())
	{
		auto count = static_cast<uint32_t>(shaderVS.input.samplers.size());
		auto* arr = ExtractSamplers(shaderVS.input.samplers.data(), count);
		m_context->VSSetSamplers(0, count, arr);
	}

	// Pixel shader
	auto& shaderPS = ShaderCodeMgr::GetShaderCodeRef(dx11pso->shadersID[SHADER_PS]);
	m_context->PSSetShader(static_cast<ID3D11PixelShader*>(shaderPS.code), nullptr, 0);
	if(!shaderPS.input.samplers.empty())
	{
		auto count = static_cast<uint32_t>(shaderPS.input.samplers.size());
		auto* arr = ExtractSamplers(shaderPS.input.samplers.data(), count);
		m_context->PSSetSamplers(0, count, arr);
	}

	// Hull shader
	auto& shaderHS = ShaderCodeMgr::GetShaderCodeRef(dx11pso->shadersID[SHADER_HS]);
	m_context->HSSetShader(static_cast<ID3D11HullShader*>(shaderHS.code), nullptr, 0);
	if(!shaderHS.input.samplers.empty())
	{
		auto count = static_cast<uint32_t>(shaderHS.input.samplers.size());
		auto* arr = ExtractSamplers(shaderHS.input.samplers.data(), count);
		m_context->HSSetSamplers(0, count, arr);
	}

	// Domain shader
	auto& shaderDS = ShaderCodeMgr::GetShaderCodeRef(dx11pso->shadersID[SHADER_DS]);
	m_context->DSSetShader(static_cast<ID3D11DomainShader*>(shaderDS.code), nullptr, 0);
	if(!shaderDS.input.samplers.empty())
	{
		auto count = static_cast<uint32_t>(shaderDS.input.samplers.size());
		auto* arr = ExtractSamplers(shaderDS.input.samplers.data(), count);
		m_context->DSSetSamplers(0, count, arr);
	}

	// Geometry shader
	auto& shaderGS = ShaderCodeMgr::GetShaderCodeRef(dx11pso->shadersID[SHADER_GS]);
	m_context->GSSetShader(static_cast<ID3D11GeometryShader*>(shaderGS.code), nullptr, 0);
	if(!shaderGS.input.samplers.empty())
	{
		auto count = static_cast<uint32_t>(shaderGS.input.samplers.size());
		auto* arr = ExtractSamplers(shaderGS.input.samplers.data(), count);
		m_context->GSSetSamplers(0, count, arr);
	}
}

} // namespace EngineCore::RHI::DX11
