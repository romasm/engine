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
		Tex2DArray,
		Tex3D,
		CubeMap,
		CubeMapArray,
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
	// Render state enums (values match D3D11/D3D12 — binary-compatible)

	enum ComparisonFunc : int32_t
	{
		ComparisonNever = 1, ComparisonLess, ComparisonEqual, ComparisonLessEqual,
		ComparisonGreater, ComparisonNotEqual, ComparisonGreaterEqual, ComparisonAlways
	};

	enum StencilOp : int32_t
	{
		StencilKeep = 1, StencilZero, StencilReplace, StencilIncrSat,
		StencilDecrSat, StencilInvert, StencilIncr, StencilDecr
	};

	enum DepthWriteMask : int32_t
	{
		DepthWriteZero = 0, DepthWriteAll = 1
	};

	enum Blend : int32_t
	{
		BlendZero = 1, BlendOne, BlendSrcColor, BlendInvSrcColor,
		BlendSrcAlpha, BlendInvSrcAlpha, BlendDestAlpha, BlendInvDestAlpha,
		BlendDestColor, BlendInvDestColor, BlendSrcAlphaSat, // 11
		BlendBlendFactor = 14, BlendInvBlendFactor
	};

	enum BlendOp : int32_t
	{
		BlendOpAdd = 1, BlendOpSubtract, BlendOpRevSubtract, BlendOpMin, BlendOpMax
	};

	enum FillMode : int32_t
	{
		FillWireframe = 2, FillSolid = 3
	};

	enum CullMode : int32_t
	{
		CullNone = 1, CullFront, CullBack
	};

	enum Filter : int32_t
	{
		FilterMinMagMipPoint             = 0x00,
		FilterMinMagPointMipLinear       = 0x01,
		FilterMinPointMagLinearMipPoint  = 0x04,
		FilterMinPointMagMipLinear       = 0x05,
		FilterMinLinearMagMipPoint       = 0x10,
		FilterMinLinearMagPointMipLinear = 0x11,
		FilterMinMagLinearMipPoint       = 0x14,
		FilterMinMagMipLinear            = 0x15,
		FilterAnisotropic                = 0x55,
		FilterComparisonMinMagMipLinear  = 0x95,
		FilterComparisonAnisotropic      = 0xD5
	};

	enum TextureAddressMode : int32_t
	{
		AddressWrap = 1, AddressMirror, AddressClamp, AddressBorder
	};

	enum ColorWriteEnable : uint8_t
	{
		ColorWriteRed = 1, ColorWriteGreen = 2, ColorWriteBlue = 4, ColorWriteAlpha = 8,
		ColorWriteAll = 0x0F
	};

	enum InputClassification : int32_t
	{
		InputPerVertex = 0, InputPerInstance = 1
	};

	constexpr uint32_t AppendAligned = 0xFFFFFFFF;

	// -----------------------------------------------------------------------
	// Render state descriptor structs (binary-compatible with D3D11 equivalents)

	struct StencilOpDesc
	{
		StencilOp      stencilFailOp;
		StencilOp      stencilDepthFailOp;
		StencilOp      stencilPassOp;
		ComparisonFunc stencilFunc;
	};

	struct DepthStencilDesc
	{
		BOOL           depthEnable;
		DepthWriteMask depthWriteMask;
		ComparisonFunc depthFunc;
		BOOL           stencilEnable;
		uint8_t        stencilReadMask;
		uint8_t        stencilWriteMask;
		StencilOpDesc  frontFace;
		StencilOpDesc  backFace;
	};

	struct RenderTargetBlendDesc
	{
		BOOL    blendEnable;
		Blend   srcBlend;
		Blend   destBlend;
		BlendOp blendOp;
		Blend   srcBlendAlpha;
		Blend   destBlendAlpha;
		BlendOp blendOpAlpha;
		uint8_t renderTargetWriteMask;
	};

	struct BlendDesc
	{
		BOOL                  alphaToCoverageEnable;
		BOOL                  independentBlendEnable;
		RenderTargetBlendDesc renderTarget[8];
	};

	struct RasterizerDesc
	{
		FillMode fillMode;
		CullMode cullMode;
		BOOL     frontCounterClockwise;
		int32_t  depthBias;
		float    depthBiasClamp;
		float    slopeScaledDepthBias;
		BOOL     depthClipEnable;
		BOOL     scissorEnable;
		BOOL     multisampleEnable;
		BOOL     antialiasedLineEnable;
	};

	struct SamplerDesc
	{
		Filter             filter;
		TextureAddressMode addressU;
		TextureAddressMode addressV;
		TextureAddressMode addressW;
		float              mipLODBias;
		uint32_t           maxAnisotropy;
		ComparisonFunc     comparisonFunc;
		float              borderColor[4];
		float              minLOD;
		float              maxLOD;
	};

	struct InputElementDesc
	{
		const char*         semanticName;
		uint32_t            semanticIndex;
		DXGI_FORMAT         format;
		uint32_t            inputSlot;
		uint32_t            alignedByteOffset;
		InputClassification inputSlotClass;
		uint32_t            instanceDataStepRate;
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
		uint32_t         width     = 0;
		uint32_t         height    = 0;
		uint32_t         depth     = 1;      // array slices or 3D depth
		uint32_t         mipLevels = 1;
		DXGI_FORMAT      format    = DXGI_FORMAT_UNKNOWN;
		TextureDimension dimension = TextureDimension::Tex2D;

		virtual ~GfxTexture() {}
	};

	// Compute subresource index from mip + array slice (matches D3D11CalcSubresource / D3D12CalcSubresource)
	inline uint32_t CalcSubresource(uint32_t mipSlice, uint32_t arraySlice, uint32_t mipLevels)
	{
		return mipSlice + arraySlice * mipLevels;
	}

	// Views
	struct GfxRTV { virtual ~GfxRTV() {} };  // render target view
	struct GfxDSV { virtual ~GfxDSV() {} };  // depth stencil view

	struct GfxSRV
	{
		GfxTexture* sourceTexture     = nullptr; // backing texture (set by CreateSRV / LoadDDS)
		bool        ownsSourceTexture = false;    // if true, destructor deletes sourceTexture

		virtual ~GfxSRV()
		{
			if(ownsSourceTexture && sourceTexture) { delete sourceTexture; sourceTexture = nullptr; }
		}
	};

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

	// Result data from a TimestampDisjoint query (binary-compatible with D3D11_QUERY_DATA_TIMESTAMP_DISJOINT)
	struct TimestampDisjointData
	{
		uint64_t frequency;
		BOOL     disjoint;
	};

	// -----------------------------------------------------------------------
	// Box region (used by UpdateSubresource)

	struct GfxBox
	{
		uint32_t left, top, front;
		uint32_t right, bottom, back;
	};

} // namespace EngineCore::RHI

// Binary compatibility with D3D11 structs (serialized material files depend on exact layout)
static_assert(sizeof(EngineCore::RHI::DepthStencilDesc) == sizeof(D3D11_DEPTH_STENCIL_DESC), "DepthStencilDesc layout mismatch");
static_assert(sizeof(EngineCore::RHI::BlendDesc) == sizeof(D3D11_BLEND_DESC), "BlendDesc layout mismatch");
static_assert(sizeof(EngineCore::RHI::RasterizerDesc) == sizeof(D3D11_RASTERIZER_DESC), "RasterizerDesc layout mismatch");
static_assert(sizeof(EngineCore::RHI::SamplerDesc) == sizeof(D3D11_SAMPLER_DESC), "SamplerDesc layout mismatch");
static_assert(sizeof(EngineCore::RHI::InputElementDesc) == sizeof(D3D11_INPUT_ELEMENT_DESC), "InputElementDesc layout mismatch");
static_assert(sizeof(EngineCore::RHI::TimestampDisjointData) == sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), "TimestampDisjointData layout mismatch");
