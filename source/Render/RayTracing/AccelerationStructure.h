#pragma once
#include "Render.h"

// AccelerationStructure — scene-level BLAS management helpers.
//
// Each drawable mesh that participates in DXR needs a bottom-level
// acceleration structure (BLAS).  This header provides a lightweight
// registry that maps mesh resource IDs to their GfxBLAS objects.
//
// Usage pattern (DX12 only; calls are guarded by GFX_DEVICE->SupportsRaytracing()):
//
//   1. Call BLASRegistry::Register() once per static mesh after the vertex/index
//      buffers have been uploaded to the GPU.
//   2. In the render loop call BLASRegistry::Get() to retrieve the BLAS pointer
//      and fill in the RaytracingInstance array for BuildTLAS().
//   3. Call BLASRegistry::Destroy() when a mesh resource is unloaded.

namespace EngineCore::RayTracing
{

class BLASRegistry
{
public:
	// Take ownership of a pre-created BLAS for the given mesh resource ID.
	// If a BLAS is already registered for that ID the old one is destroyed first.
	static void Register(uint32_t meshID, GfxBLAS* blas)
	{
		auto it = s_map.find(meshID);
		if(it != s_map.end())
		{
			if(GFX_DEVICE) GFX_DEVICE->DestroyBLAS(it->second);
			it->second = blas;
		}
		else
		{
			s_map.emplace(meshID, blas);
		}
	}

	// Return the BLAS for meshID, or nullptr if not registered.
	static GfxBLAS* Get(uint32_t meshID)
	{
		auto it = s_map.find(meshID);
		return it != s_map.end() ? it->second : nullptr;
	}

	// Release the BLAS for meshID and remove it from the registry.
	static void Destroy(uint32_t meshID)
	{
		auto it = s_map.find(meshID);
		if(it == s_map.end()) return;
		if(GFX_DEVICE) GFX_DEVICE->DestroyBLAS(it->second);
		s_map.erase(it);
	}

	// Release every registered BLAS (call at engine shutdown).
	static void DestroyAll()
	{
		if(GFX_DEVICE)
			for(auto& [id, blas] : s_map)
				GFX_DEVICE->DestroyBLAS(blas);
		s_map.clear();
	}

	// Number of registered BLASes.
	static size_t Count() { return s_map.size(); }

private:
	static inline std::unordered_map<uint32_t, GfxBLAS*> s_map;
};

} // namespace EngineCore::RayTracing
