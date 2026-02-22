#include "stdafx.h"
#include "Image.h"
#include "Shader.h"
#include "Common.h"
#include "Buffer.h"
#include "MaterialMgr.h"

using namespace EngineCore;

Image2D::Image2D()
{
	shaderInst = nullptr;
	vertexBuffer = nullptr;
	dirty = false;
}

void Image2D::Init(const string& shadername)
{
	vertexBuffer = Buffer::CreateVertexBuffer(sizeof(UnlitVertex)*4, true, nullptr);
	if(!vertexBuffer)
	{
		ERR("Cant create vertex buffer for Image2D");
		return;
	}

	shaderInst = new SimpleShaderInst(shadername);
	
	dirty = true;
}

void Image2D::Draw()
{
	if(dirty)
	{
		if(!updateVertices())
			return;
		dirty = false;
	}

	GFX_CMD->SetVertexBuffer(0, vertexBuffer, sizeof(UnlitVertex));
	GFX_CMD->SetIndexBuffer(Buffer::QuadIndex, false);

	GFX_CMD->SetTopology(RHI::Topology::TriangleList);

	shaderInst->Set();

	GFX_CMD->DrawIndexed(6);
}

bool Image2D::updateVertices()
{
	if(!Render::Get()->CurrentHudWindow)
		return false;

	float left = - 1.0f + 2.0f * float(quad_rect.left) / Render::Get()->CurrentHudWindow->GetWidth();
	float top = 1.0f - 2.0f * float(quad_rect.top) / Render::Get()->CurrentHudWindow->GetHeight();
	float right = - 1.0f + 2.0f * float(quad_rect.left + quad_rect.width) / Render::Get()->CurrentHudWindow->GetWidth();
	float bottom = 1.0f - 2.0f * float(quad_rect.top + quad_rect.height) / Render::Get()->CurrentHudWindow->GetHeight();

	UnlitVertex vertices_quad[4];

	vertices_quad[0].Pos = Vector3(left, top, 0.0f);
	vertices_quad[0].Tex = Vector2(0.0f, 0.0f);

	vertices_quad[1].Pos = Vector3(right, bottom, 0.0f);
	vertices_quad[1].Tex = Vector2(1.0f, 1.0f);

	vertices_quad[2].Pos = Vector3(left, bottom, 0.0f);
	vertices_quad[2].Tex = Vector2(0.0f, 1.0f);

	vertices_quad[3].Pos = Vector3(right, top, 0.0f);
	vertices_quad[3].Tex = Vector2(1.0f, 0.0f);

	GFX_CMD->UpdateBuffer(vertexBuffer, (void*)vertices_quad, sizeof(UnlitVertex) * 4);

	return true;
}

void Image2D::Close()
{
	_DELETE(vertexBuffer);
	_DELETE(shaderInst);
}