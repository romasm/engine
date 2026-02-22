#include "stdafx.h"
#include "DX12CommandList.h"

// DX12CommandList::SetPipelineState is the only method that will need
// to reference ShaderCodeMgr (Phase 4). In Phase 2 it is a no-op stub
// so the project compiles cleanly without requiring the full PSO system.

namespace EngineCore::RHI::DX12
{

void DX12CommandList::SetPipelineState(GfxPipelineState* pso)
{
	if(!pso)
		return;

	auto* dx12pso = Cast(pso);
	m_isCompute = dx12pso->isCompute;

	if(dx12pso->rootSignature)
	{
		if(dx12pso->isCompute)
			m_commandList->SetComputeRootSignature(dx12pso->rootSignature);
		else
			m_commandList->SetGraphicsRootSignature(dx12pso->rootSignature);
	}

	if(dx12pso->pso)
		m_commandList->SetPipelineState(dx12pso->pso);
}

} // namespace EngineCore::RHI::DX12
