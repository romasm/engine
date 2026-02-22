#pragma once
#include "RHITypes.h"

// IGfxDescriptorHeap — bindless descriptor table management.
// DX11 backend: no-op stub (DX11 uses slot-based binding, no explicit descriptor heap).
// DX12 backend: manages CBV/SRV/UAV descriptor heap for bindless resource access.
//
// Bindless layout (DX12 only):
//   Slot [0 .. N)   : SRVs  (textures and structured buffers)
//   Slot [N .. M)   : UAVs
//   Separate heap   : Samplers
//
// Shaders access resources by 32-bit index pushed via root constants.

namespace EngineCore::RHI
{

// Handle to a descriptor slot in the bindless heap.
// Shader Model 6.x shaders receive this index via root constants
// and index into ResourceDescriptorHeap[] / SamplerDescriptorHeap[].
struct DescriptorHandle
{
	static constexpr uint32_t Invalid = UINT32_MAX;
	uint32_t index = Invalid;

	bool IsValid() const { return index != Invalid; }
};

class IGfxDescriptorHeap
{
public:
	virtual ~IGfxDescriptorHeap() {}

	// Register a resource view and return its bindless index.
	// The index is stable for the resource lifetime.
	virtual DescriptorHandle RegisterSRV(GfxSRV* srv) = 0;
	virtual DescriptorHandle RegisterUAV(GfxUAV* uav) = 0;
	virtual DescriptorHandle RegisterSampler(GfxSampler* sampler) = 0;

	// Free a previously registered descriptor slot.
	virtual void FreeSRV(DescriptorHandle handle) = 0;
	virtual void FreeUAV(DescriptorHandle handle) = 0;
	virtual void FreeSampler(DescriptorHandle handle) = 0;

	// Bind the heap to the given command list so shaders can access it.
	// On DX11: no-op. On DX12: SetDescriptorHeaps().
	virtual void BindToCommandList(class IGfxCommandList* commandList) = 0;

	// Total capacity
	virtual uint32_t SRVCapacity()     const = 0;
	virtual uint32_t UAVCapacity()     const = 0;
	virtual uint32_t SamplerCapacity() const = 0;
};

} // namespace EngineCore::RHI
