#pragma once

#include "stdafx.h"
#include "macros.h"
#include "RHITypes.h"

namespace EngineCore
{
//------------------------------------------------------------------
	struct StructBuf
	{
		RHI::GfxBuffer* buf = nullptr;
		RHI::GfxSRV* srv = nullptr;

		void Release()
		{
			_DELETE(buf);
			_DELETE(srv);
		}
	};

	class Buffer
	{
	public:
		Buffer();
		~Buffer();

		static RHI::GfxBuffer* CreateVertexBuffer(int size, bool dynamic, const void *Mem);
		static RHI::GfxBuffer* CreateIndexBuffer(int size, bool dynamic, const void *Mem);
		static RHI::GfxBuffer* CreateConstantBuffer(int size, bool dynamic);
		static StructBuf CreateStructedBuffer(int element_count, int element_size, bool dynamic);
		static StructBuf CreateStructedBuffer(int element_count, int element_size, bool dynamic, void* initData);

		static RHI::GfxBuffer* QuadVertex;
		static RHI::GfxBuffer* QuadIndex;
		static RHI::GfxBuffer* TriangVertex;
		static RHI::GfxBuffer* TriangIndex;
	};

//------------------------------------------------------------------
}
