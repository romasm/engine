#pragma once

#include "stdafx.h"
#include "macros.h"

namespace EngineCore
{
//------------------------------------------------------------------
	struct StructBuf
	{
		ID3D11Buffer *buf;
		ID3D11ShaderResourceView *srv;

		StructBuf()
		{
			buf = nullptr;
			srv = nullptr;
		}

		void Release()
		{
			_RELEASE(buf);
			_RELEASE(srv);
		}
	};

	class Buffer
	{
	public:
		Buffer();
		~Buffer();

		static ID3D11Buffer* CreateVertexBuffer(ID3D11Device *device, int size, bool dynamic, const void *Mem);
		static ID3D11Buffer* CreateIndexBuffer(ID3D11Device *device, int size, bool dynamic, const void *Mem);
		static ID3D11Buffer* CreateConstantBuffer(ID3D11Device *device, int size, bool dynamic);
		static StructBuf CreateStructedBuffer(ID3D11Device *device, int element_count, int element_size, bool dynamic);
		static StructBuf CreateStructedBuffer(ID3D11Device *device, int element_count, int element_size, bool dynamic, void* initData);
	
		static ID3D11Buffer* QuadVertex;
		static ID3D11Buffer* QuadIndex;
		static ID3D11Buffer* TriangVertex;
		static ID3D11Buffer* TriangIndex;

		static ID3D11Buffer* nullCBs[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
		static ID3D11ShaderResourceView* nullRESs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
		static ID3D11UnorderedAccessView* nullRWs[D3D11_1_UAV_SLOT_COUNT];
	};

//------------------------------------------------------------------
}