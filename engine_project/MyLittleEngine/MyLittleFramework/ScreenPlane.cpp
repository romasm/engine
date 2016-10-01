#include "stdafx.h"
#include "ScreenPlane.h"
#include "macros.h"
#include "Util.h"
#include "Shader.h"
#include "Buffer.h"
#include "DataTypes.h"

using namespace EngineCore;

void ScreenPlane::Draw()
{
	if(!indexBuffer)
		return;

	const unsigned int stride = sizeof(UnlitVertex);
	const unsigned int offset = 0;
	CONTEXT->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	CONTEXT->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);

	Render::SetTopology(IA_TOPOLOGY::TRISLIST);
	
	shaderInst->Set();
	
	CONTEXT->DrawIndexed(3, 0, 0);
}

ScreenPlane::~ScreenPlane()
{
	_RELEASE(indexBuffer);
	_RELEASE(vertexBuffer);
	_DELETE(shaderInst);
}

void ScreenPlane::createPlane(uint32_t sw, uint32_t sh, string& shadername)
{
	shaderInst = new SimpleShaderInst(shadername);
	if(!shaderInst)
		return;

	UnlitVertex* SP_vert = new UnlitVertex[3];
	SP_vert[0].Pos = XMFLOAT3(-1.0f, 1.0f, 0.0f);
	SP_vert[0].Tex = XMFLOAT2(0.0f, 0.0f);
	SP_vert[1].Pos = XMFLOAT3(3.0f ,1.0f, 0.0f);
	SP_vert[1].Tex = XMFLOAT2(2.0f, 0.0f);
	SP_vert[2].Pos = XMFLOAT3(-1.0f, -3.0f, 0.0f);
	SP_vert[2].Tex = XMFLOAT2(0.0f,2.0f);

	unsigned short *indices = new unsigned short[3];
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;

	vertexBuffer = Buffer::CreateVertexBuffer(DEVICE, sizeof(UnlitVertex) * 3, false, SP_vert);
	if (!vertexBuffer)
		return;

	indexBuffer = Buffer::CreateIndexBuffer(DEVICE, sizeof(unsigned short) * 3, false, indices);
	if (!indexBuffer)
		return;

	_DELETE_ARRAY(SP_vert);
	_DELETE_ARRAY(indices);
}