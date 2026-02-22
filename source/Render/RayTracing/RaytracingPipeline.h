#pragma once
#include "Render.h"

// RaytracingPipelineManager — owns one DXR pipeline and its associated TLAS.
//
// Intended usage:
//   1. At startup, load the DXIL shader library (e.g. shadows.hlsl compiled
//      with DXC: dxc.exe -T lib_6_6 -Fo shadows.cso shadows.hlsl).
//   2. Call Init() with the bytecode and scene parameters.
//   3. Each frame, call UpdateTLAS() with the current set of visible instances,
//      then call Dispatch() to trace rays into the output UAV texture.
//   4. Call Destroy() at shutdown.
//
// Only usable when GFX_DEVICE->SupportsRaytracing() returns true.

namespace EngineCore::RayTracing
{

class RaytracingPipelineManager
{
public:
	~RaytracingPipelineManager() { Destroy(); }

	// Create the TLAS and RT pipeline.  bytecode / bytecodeSize must point to
	// a valid DXIL library (lib_6_6).  maxInstances bounds the per-frame TLAS.
	bool Init(const uint8_t* bytecode, size_t bytecodeSize,
	          uint32_t maxInstances,
	          const wchar_t* raygenEntry, const wchar_t* missEntry,
	          const wchar_t* closestHitEntry, const wchar_t* hitGroupName)
	{
		if(!GFX_DEVICE || !GFX_DEVICE->SupportsRaytracing()) return false;

		// TLAS
		TLASDesc tlasDesc;
		tlasDesc.maxInstances = maxInstances;
		m_tlas = GFX_DEVICE->CreateTLAS(tlasDesc);
		if(!m_tlas) return false;

		// Pipeline
		RaytracingPipelineDesc pipelineDesc;
		pipelineDesc.shaderBytecode     = bytecode;
		pipelineDesc.shaderBytecodeSize = bytecodeSize;
		pipelineDesc.raygenEntryPoint   = raygenEntry;
		pipelineDesc.missEntryPoint     = missEntry;
		pipelineDesc.closestHitEntry    = closestHitEntry;
		pipelineDesc.hitGroupName       = hitGroupName;
		m_pipeline = GFX_DEVICE->CreateRaytracingPipeline(pipelineDesc);
		if(!m_pipeline)
		{
			GFX_DEVICE->DestroyTLAS(m_tlas);
			m_tlas = nullptr;
			return false;
		}

		return true;
	}

	// Write instance data for this frame and record the TLAS build into cmdList.
	// Must be called before Dispatch() in the same command list.
	void UpdateTLAS(IGfxCommandList* commandList,
	                const RaytracingInstance* instances, uint32_t count,
	                bool update = false)
	{
		if(!m_tlas || !commandList) return;
		commandList->BuildTLAS(m_tlas, instances, count, update);
	}

	// Dispatch rays; width/height should match the output UAV texture dimensions.
	void Dispatch(IGfxCommandList* commandList, uint32_t width, uint32_t height)
	{
		if(!m_pipeline || !commandList) return;
		commandList->DispatchRays(m_pipeline, width, height);
	}

	// Release all GPU resources.
	void Destroy()
	{
		if(!GFX_DEVICE) return;
		if(m_pipeline) { GFX_DEVICE->DestroyRaytracingPipeline(m_pipeline); m_pipeline = nullptr; }
		if(m_tlas)     { GFX_DEVICE->DestroyTLAS(m_tlas);                   m_tlas     = nullptr; }
	}

	bool IsReady() const { return m_tlas && m_pipeline; }

private:
	GfxTLAS*               m_tlas     = nullptr;
	GfxRaytracingPipeline* m_pipeline = nullptr;
};

} // namespace EngineCore::RayTracing
