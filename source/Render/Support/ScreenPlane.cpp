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
	GFX_CMD->SetVertexBuffer(0, Buffer::TriangVertex, sizeof(UnlitVertex));
	GFX_CMD->SetIndexBuffer(Buffer::TriangIndex, false);

	GFX_CMD->SetTopology(RHI::Topology::TriangleList);

	shaderInst->Set();

	GFX_CMD->DrawIndexed(3);
}

ScreenPlane::~ScreenPlane()
{
	_DELETE(shaderInst);
}

void ScreenPlane::createPlane(const string& shadername)
{
	shaderInst = new SimpleShaderInst(shadername);
	if(!shaderInst)
		return;
}