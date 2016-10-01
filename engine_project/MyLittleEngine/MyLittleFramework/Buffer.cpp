#include "stdafx.h"
#include "Buffer.h"
#include "Render.h"

using namespace EngineCore;

ID3D11Buffer* Buffer::QuadVertex = nullptr;
ID3D11Buffer* Buffer::QuadIndex = nullptr;
ID3D11Buffer* Buffer::TriangVertex = nullptr;
ID3D11Buffer* Buffer::TriangIndex = nullptr;

Buffer::Buffer()
{
	UnlitVertex vertices_quad[4];
	
	vertices_quad[0].Pos = XMFLOAT3(-1.0f, 1.0f, 0.0f);
	vertices_quad[0].Tex = XMFLOAT2(0.0f, 0.0f);

	vertices_quad[1].Pos = XMFLOAT3(1.0f, -1.0f, 0.0f);
	vertices_quad[1].Tex = XMFLOAT2(1.0f, 1.0f);

	vertices_quad[2].Pos = XMFLOAT3(-1.0f, -1.0f, 0.0f);
	vertices_quad[2].Tex = XMFLOAT2(0.0f, 1.0f);

	vertices_quad[3].Pos = XMFLOAT3(1.0f, 1.0f, 0.0f);
	vertices_quad[3].Tex = XMFLOAT2(1.0f, 0.0f);
		
	unsigned short indices_quad[6] = 
	{
		0,1,2,
		0,3,1
	};
	
	_RELEASE(QuadVertex);
	_RELEASE(QuadIndex);
	QuadVertex = Buffer::CreateVertexBuffer(DEVICE, sizeof(UnlitVertex)*4, false, &vertices_quad);
	QuadIndex = Buffer::CreateIndexBuffer(DEVICE, sizeof(unsigned short)*6, false, &indices_quad);
}

Buffer::~Buffer()
{
	_RELEASE(QuadVertex);
	_RELEASE(QuadIndex);
	_RELEASE(TriangVertex);
	_RELEASE(TriangIndex);
}

ID3D11Buffer* Buffer::CreateVertexBuffer(ID3D11Device *device, int size, bool dynamic, const void *Mem)
{
	ID3D11Buffer *vb = nullptr;
		
	D3D11_BUFFER_DESC BufferDesc;
	BufferDesc.ByteWidth = size;
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;	
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	if (dynamic)
	{
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		BufferDesc.Usage = D3D11_USAGE_DEFAULT;
		BufferDesc.CPUAccessFlags = 0;
	}	
	
	if(!Mem)
	{
		HRESULT hr = device->CreateBuffer(&BufferDesc, NULL, &vb);
		if( FAILED(hr) )
			return nullptr;
	}
	else
	{
		D3D11_SUBRESOURCE_DATA Data;	
		Data.pSysMem = Mem;
		Data.SysMemPitch = 0;
		Data.SysMemSlicePitch = 0;
		HRESULT hr = device->CreateBuffer(&BufferDesc, &Data, &vb);
		if( FAILED(hr) )
			return nullptr;
	}

	return vb;
}

ID3D11Buffer* Buffer::CreateIndexBuffer(ID3D11Device *device, int size, bool dynamic, const void *Mem)
{
	ID3D11Buffer *ib = nullptr;

	D3D11_BUFFER_DESC BufferDesc;
	BufferDesc.ByteWidth = size;
	BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;	
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	if (dynamic)
	{
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		BufferDesc.Usage = D3D11_USAGE_DEFAULT;
		BufferDesc.CPUAccessFlags = 0;
	}	

	D3D11_SUBRESOURCE_DATA Data;	
	Data.pSysMem = Mem;
	Data.SysMemPitch = 0;
	Data.SysMemSlicePitch = 0;
	HRESULT hr = device->CreateBuffer(&BufferDesc, &Data, &ib);
	if( FAILED(hr) )
		return nullptr;

	return ib;
}

ID3D11Buffer* Buffer::CreateConstantBuffer(ID3D11Device *device, int size, bool dynamic)
{
	ID3D11Buffer *cb = nullptr;

	D3D11_BUFFER_DESC BufferDesc;
	BufferDesc.ByteWidth = size;
	BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;	
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	if (dynamic)
	{
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		BufferDesc.Usage = D3D11_USAGE_DEFAULT;
		BufferDesc.CPUAccessFlags = 0;
	}	
	
	HRESULT hr = device->CreateBuffer(&BufferDesc, NULL, &cb);
	if( FAILED(hr) )
		return nullptr;

	return cb;
}

StructBuf Buffer::CreateStructedBuffer(ID3D11Device *device, int size, int stride, int numElements, bool dynamic)
{
	StructBuf res;

	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
	bufferDesc.ByteWidth           = size;
	bufferDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.StructureByteStride = stride;
	if (dynamic)
	{
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.CPUAccessFlags = 0;
	}	
	
	HRESULT hr = device->CreateBuffer(&bufferDesc, nullptr, &res.buf);
	if( FAILED(hr) )
		return res;

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	ZeroMemory(&viewDesc, sizeof(viewDesc));
	viewDesc.Format              = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
	viewDesc.Buffer.NumElements = numElements;

	if (FAILED(device->CreateShaderResourceView(res.buf, &viewDesc, &res.srv)))
		return res;

	return res;
}