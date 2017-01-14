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
	const unsigned int stride = sizeof(UnlitVertex);
	const unsigned int offset = 0;
	CONTEXT->IASetVertexBuffers(0, 1, &Buffer::TriangVertex, &stride, &offset);
	CONTEXT->IASetIndexBuffer(Buffer::TriangIndex, DXGI_FORMAT_R16_UINT, 0);

	Render::SetTopology(IA_TOPOLOGY::TRISLIST);
	
	shaderInst->Set();
	
	CONTEXT->DrawIndexed(3, 0, 0);
}

ScreenPlane::~ScreenPlane()
{
	_DELETE(shaderInst);
}

void ScreenPlane::createPlane(string& shadername)
{
	shaderInst = new SimpleShaderInst(shadername);
	if(!shaderInst)
		return;
}