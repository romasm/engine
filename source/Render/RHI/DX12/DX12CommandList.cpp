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

	// Root signatures are set once in Open() and shared by all PSOs of the
	// same type (graphics or compute).  Re-setting them here would invalidate
	// all root parameter bindings (CBVs, descriptor tables), causing shaders
	// to read from undefined GPU addresses and crash.
	// Only the PSO itself needs to be swapped per draw/dispatch.

	if(dx12pso->pso)
		m_commandList->SetPipelineState(dx12pso->pso);
}

} // namespace EngineCore::RHI::DX12
