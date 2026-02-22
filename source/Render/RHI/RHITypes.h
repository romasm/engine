#pragma once
#include "stdafx.h"

// RHI (Rendering Hardware Interface) — backend-agnostic GPU resource types.
// DX11 and DX12 backends each provide concrete implementations.

namespace EngineCore::RHI
{
	// -----------------------------------------------------------------------
	// Resource usage hints

	enum class BufferUsage : uint8_t
	{
		Static,   // written once, GPU read-only; copied via upload buffer on DX12
		Dynamic,  // written every frame from CPU; upload heap ring-buffer on DX12
	};

	enum class TextureDimension : uint8_t
	{
		Tex2D,
		Tex3D,
		CubeMap,
	};

	// -----------------------------------------------------------------------
	// Pipeline topology

	enum class Topology : uint8_t
	{
		PointList    = 1,
		LineList     = 2,
		TriangleList = 4,
		Patch3       = 33, // D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST
	};

	// -----------------------------------------------------------------------
	// Resource state (explicit on DX12, implicit on DX11)

	enum class ResourceState : uint32_t
	{
		Common              = 0,
		VertexBuffer        = 1 << 0,
		IndexBuffer         = 1 << 1,
		RenderTarget        = 1 << 2,
		UnorderedAccess     = 1 << 3,
		DepthWrite          = 1 << 4,
		DepthRead           = 1 << 5,
		ShaderResource      = 1 << 6,   // pixel + non-pixel shader read
		IndirectArgument    = 1 << 7,
		CopyDest            = 1 << 8,
		CopySource          = 1 << 9,
		Present             = 1 << 10,
		AccelerationStructure = 1 << 11,
	};

	// -----------------------------------------------------------------------
	// Shader stage flags (used when binding resources per-stage)

	enum class ShaderStage : uint8_t
	{
		Vertex    = 1 << 0,
		Hull      = 1 << 1,
		Domain    = 1 << 2,
		Geometry  = 1 << 3,
		Pixel     = 1 << 4,
		Compute   = 1 << 5,
		Mesh      = 1 << 6,
		All       = 0x7F,
	};

	inline ShaderStage operator|(ShaderStage a, ShaderStage b)
	{
		return static_cast<ShaderStage>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
	}

	inline bool StageHas(ShaderStage set, ShaderStage flag)
	{
		return (static_cast<uint8_t>(set) & static_cast<uint8_t>(flag)) != 0;
	}

	// -----------------------------------------------------------------------
	// Viewport (value type, not a pointer resource)

	struct GfxViewport
	{
		float topLeftX;
		float topLeftY;
		float width;
		float height;
		float minDepth;
		float maxDepth;

		GfxViewport() : topLeftX(0), topLeftY(0), width(0), height(0), minDepth(0), maxDepth(1) {}
		GfxViewport(float width, float height)
			: topLeftX(0), topLeftY(0), width(width), height(height), minDepth(0), maxDepth(1) {}
		GfxViewport(float x, float y, float width, float height)
			: topLeftX(x), topLeftY(y), width(width), height(height), minDepth(0), maxDepth(1) {}
	};

	// -----------------------------------------------------------------------
	// Opaque GPU resource base types
	// Concrete implementations live in DX11/ and DX12/ subdirectories.

	struct GfxBuffer
	{
		virtual ~GfxBuffer() {}
	};

	struct GfxTexture
	{
		virtual ~GfxTexture() {}
	};

	// Views
	struct GfxRTV { virtual ~GfxRTV() {} };  // render target view
	struct GfxDSV { virtual ~GfxDSV() {} };  // depth stencil view
	struct GfxSRV { virtual ~GfxSRV() {} };  // shader resource view
	struct GfxUAV { virtual ~GfxUAV() {} };  // unordered access view
	struct GfxSampler { virtual ~GfxSampler() {} };

	// Pipeline state (combined shaders + blend/depth/raster state = PSO in DX12)
	struct GfxPipelineState { virtual ~GfxPipelineState() {} };

	// Input vertex layout
	struct GfxInputLayout { virtual ~GfxInputLayout() {} };

	// -----------------------------------------------------------------------
	// Raytracing resource base types (DXR — DX12 only)

	struct GfxBLAS               { virtual ~GfxBLAS()               {} }; // bottom-level AS
	struct GfxTLAS               { virtual ~GfxTLAS()               {} }; // top-level AS
	struct GfxRaytracingPipeline { virtual ~GfxRaytracingPipeline() {} }; // RT state object

	// -----------------------------------------------------------------------
	// Raytracing description structs — used by IGfxDevice and IGfxCommandList.

	struct BLASDesc
	{
		GfxBuffer* vertexBuffer   = nullptr;
		uint32_t   vertexCount    = 0;
		uint32_t   vertexStride   = 0;    // bytes per vertex
		uint32_t   positionOffset = 0;    // byte offset to XYZF32 position within each vertex
		GfxBuffer* indexBuffer    = nullptr;  // nullptr = non-indexed geometry
		uint32_t   indexCount     = 0;
		bool       index16bit     = false;   // true = UINT16 indices, false = UINT32
		bool       allowUpdate    = false;   // enable ALLOW_UPDATE for skinned meshes
	};

	struct TLASDesc
	{
		uint32_t maxInstances = 0;  // upper bound on per-frame instance count
	};

	// Per-instance data passed to IGfxCommandList::BuildTLAS every frame.
	struct RaytracingInstance
	{
		float    transform[3][4] = {};   // row-major 3×4 affine world matrix
		uint32_t instanceID      = 0;    // SV_InstanceID in closest-hit shader (24-bit)
		uint8_t  mask            = 0xFF; // visibility mask; TraceRay culls if (mask & instanceMask)==0
		uint32_t hitGroupIndex   = 0;    // offset into hit-group shader table (24-bit)
		uint32_t flags           = 0;    // D3D12_RAYTRACING_INSTANCE_FLAGS bitmask
		GfxBLAS* blas            = nullptr;
	};

	// Descriptor for a raytracing pipeline (state object + shader table).
	// shaderBytecode must point to a DXIL library compiled with DXC (lib_6_6 target).
	struct RaytracingPipelineDesc
	{
		const uint8_t* shaderBytecode     = nullptr;
		size_t         shaderBytecodeSize = 0;
		const wchar_t* raygenEntryPoint   = nullptr; // e.g. L"RayGen"
		const wchar_t* missEntryPoint     = nullptr; // e.g. L"Miss"
		const wchar_t* closestHitEntry    = nullptr; // e.g. L"ClosestHit"
		const wchar_t* hitGroupName       = nullptr; // e.g. L"HitGroup"
		uint32_t maxPayloadSize    = 32;  // bytes; must match payload struct in HLSL
		uint32_t maxAttributeSize  = 8;   // bytes; 2×float for standard barycentrics
		uint32_t maxRecursionDepth = 1;
	};

	// -----------------------------------------------------------------------
	// Query heap (GPU timestamp/disjoint queries)

	enum class QueryType : uint8_t
	{
		Timestamp,
		TimestampDisjoint,
	};

	struct GfxQueryHeap { virtual ~GfxQueryHeap() {} };

	// -----------------------------------------------------------------------
	// Box region (used by UpdateSubresource)

	struct GfxBox
	{
		uint32_t left, top, front;
		uint32_t right, bottom, back;
	};

} // namespace EngineCore::RHI
