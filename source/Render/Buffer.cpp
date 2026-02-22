#include "stdafx.h"
#include "Buffer.h"
#include "Render.h"

using namespace EngineCore;
using namespace EngineCore::RHI;

GfxBuffer* Buffer::QuadVertex = nullptr;
GfxBuffer* Buffer::QuadIndex = nullptr;
GfxBuffer* Buffer::TriangVertex = nullptr;
GfxBuffer* Buffer::TriangIndex = nullptr;

Buffer::Buffer()
{
	UnlitVertex vertices_quad[4];

	vertices_quad[0].Pos = Vector3(-1.0f, 1.0f, 0.0f);
	vertices_quad[0].Tex = Vector2(0.0f, 0.0f);

	vertices_quad[1].Pos = Vector3(1.0f, -1.0f, 0.0f);
	vertices_quad[1].Tex = Vector2(1.0f, 1.0f);

	vertices_quad[2].Pos = Vector3(-1.0f, -1.0f, 0.0f);
	vertices_quad[2].Tex = Vector2(0.0f, 1.0f);

	vertices_quad[3].Pos = Vector3(1.0f, 1.0f, 0.0f);
	vertices_quad[3].Tex = Vector2(1.0f, 0.0f);

	unsigned short indices_quad[6] =
	{
		0,1,2,
		0,3,1
	};

	_DELETE(QuadVertex);
	_DELETE(QuadIndex);
	QuadVertex = Buffer::CreateVertexBuffer(sizeof(UnlitVertex)*4, false, &vertices_quad);
	QuadIndex = Buffer::CreateIndexBuffer(sizeof(unsigned short)*6, false, &indices_quad);

	UnlitVertex vertices_tris[3];
	vertices_tris[0].Pos = Vector3(-1.0f, 1.0f, 0.0f);
	vertices_tris[0].Tex = Vector2(0.0f, 0.0f);
	vertices_tris[1].Pos = Vector3(3.0f ,1.0f, 0.0f);
	vertices_tris[1].Tex = Vector2(2.0f, 0.0f);
	vertices_tris[2].Pos = Vector3(-1.0f, -3.0f, 0.0f);
	vertices_tris[2].Tex = Vector2(0.0f,2.0f);

	unsigned short indices_tris[3] =
	{
		0,1,2
	};

	_DELETE(TriangVertex);
	_DELETE(TriangIndex);
	TriangVertex = Buffer::CreateVertexBuffer(sizeof(UnlitVertex)*3, false, &vertices_tris);
	TriangIndex = Buffer::CreateIndexBuffer(sizeof(unsigned short)*3, false, &indices_tris);
}

Buffer::~Buffer()
{
	_DELETE(QuadVertex);
	_DELETE(QuadIndex);
	_DELETE(TriangVertex);
	_DELETE(TriangIndex);
}

GfxBuffer* Buffer::CreateVertexBuffer(int size, bool dynamic, const void *Mem)
{
	BufferDesc desc = {};
	desc.sizeBytes = size;
	desc.usage = dynamic ? BufferUsage::Dynamic : BufferUsage::Static;
	desc.isVertex = true;
	desc.isIndex = false;
	desc.isConstant = false;
	desc.allowSRV = false;
	desc.allowUAV = false;
	desc.structureStrideBytes = 0;
	desc.initialData = Mem;
	return GFX_DEVICE->CreateBuffer(desc);
}

GfxBuffer* Buffer::CreateIndexBuffer(int size, bool dynamic, const void *Mem)
{
	BufferDesc desc = {};
	desc.sizeBytes = size;
	desc.usage = dynamic ? BufferUsage::Dynamic : BufferUsage::Static;
	desc.isVertex = false;
	desc.isIndex = true;
	desc.isConstant = false;
	desc.allowSRV = false;
	desc.allowUAV = false;
	desc.structureStrideBytes = 0;
	desc.initialData = Mem;
	return GFX_DEVICE->CreateBuffer(desc);
}

GfxBuffer* Buffer::CreateConstantBuffer(int size, bool dynamic)
{
	BufferDesc desc = {};
	desc.sizeBytes = size;
	desc.usage = dynamic ? BufferUsage::Dynamic : BufferUsage::Static;
	desc.isVertex = false;
	desc.isIndex = false;
	desc.isConstant = true;
	desc.allowSRV = false;
	desc.allowUAV = false;
	desc.structureStrideBytes = 0;
	desc.initialData = nullptr;
	return GFX_DEVICE->CreateBuffer(desc);
}

StructBuf Buffer::CreateStructedBuffer(int element_count, int element_size, bool dynamic)
{
	StructBuf res;

	BufferDesc bufDesc = {};
	bufDesc.sizeBytes = element_count * element_size;
	bufDesc.usage = dynamic ? BufferUsage::Dynamic : BufferUsage::Static;
	bufDesc.isVertex = false;
	bufDesc.isIndex = false;
	bufDesc.isConstant = false;
	bufDesc.allowSRV = true;
	bufDesc.allowUAV = false;
	bufDesc.structureStrideBytes = element_size;
	bufDesc.initialData = nullptr;

	res.buf = GFX_DEVICE->CreateBuffer(bufDesc);
	if(!res.buf)
		return res;

	res.srv = GFX_DEVICE->CreateSRV(res.buf, 0, element_count);
	return res;
}

StructBuf Buffer::CreateStructedBuffer(int element_count, int element_size, bool dynamic, void* initData)
{
	StructBuf res;

	BufferDesc bufDesc = {};
	bufDesc.sizeBytes = element_count * element_size;
	bufDesc.usage = dynamic ? BufferUsage::Dynamic : BufferUsage::Static;
	bufDesc.isVertex = false;
	bufDesc.isIndex = false;
	bufDesc.isConstant = false;
	bufDesc.allowSRV = true;
	bufDesc.allowUAV = false;
	bufDesc.structureStrideBytes = element_size;
	bufDesc.initialData = initData;

	res.buf = GFX_DEVICE->CreateBuffer(bufDesc);
	if(!res.buf)
		return res;

	res.srv = GFX_DEVICE->CreateSRV(res.buf, 0, element_count);
	return res;
}
