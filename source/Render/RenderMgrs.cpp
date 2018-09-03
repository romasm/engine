#include "stdafx.h"
#include "RenderMgrs.h"
#include "ScenePipeline.h"

using namespace EngineCore;

BaseRenderMgr::BaseRenderMgr()
{
	cameraPosition = Vector3::Zero;
}

bool BaseRenderMgr::CompareMeshes(RenderMesh& first, RenderMesh& second)
{
	return first.distanceSq < second.distanceSq;
}

bool BaseRenderMgr::InvCompareMeshes(RenderMesh& first, RenderMesh& second)
{
	return first.distanceSq > second.distanceSq;
}

ShadowRenderMgr::ShadowRenderMgr() : BaseRenderMgr()
{
	b_shadow = true;

	opaque_array.create(OPAQUE_SHADOW_MAX);
	alphatest_array.create(OPAQUE_SHADOW_ALPHATEST_MAX);
}

bool ShadowRenderMgr::RegMesh(uint32_t indexCount, ID3D11Buffer* indexBuffer, ID3D11Buffer* vertexBuffer, 
			 uint32_t vertexSize, bool isSkinned, void* gpuMatrixBuffer, Material* material, IA_TOPOLOGY topo)
{return RegMesh(indexCount, indexBuffer, vertexBuffer, vertexSize, isSkinned, gpuMatrixBuffer, material, (Vector3&)Vector3::Zero, topo);}

bool ShadowRenderMgr::RegMesh(uint32_t indexCount, ID3D11Buffer* indexBuffer, ID3D11Buffer* vertexBuffer, 
							 uint32_t vertexSize, bool isSkinned, void* gpuMatrixBuffer, Material* material, Vector3& center, IA_TOPOLOGY topo)
{
	if( !material || gpuMatrixBuffer == nullptr || topo != IA_TOPOLOGY::TRISLIST || indexCount == 0 )
		return false;

	uint32_t skinned = isSkinned ? 1 : 0;

	bool has_tq = false;
	const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_SHADOW + skinned);
	auto queue = material->GetTechQueue(tech, &has_tq);
	if(!has_tq)
		return false;
	if( queue == GUI_2D || queue == GUI_2D_FONT || queue == GUI_3D || queue == GUI_3D_OVERLAY )
		return false;
	if( !IsTranparentShadows() && (queue == SC_TRANSPARENT || queue == SC_ALPHA) )
		return false;

	RenderMesh* mesh_new = nullptr;
	switch(queue)
	{
	case SC_ALPHA:
	case SC_TRANSPARENT:
		mesh_new = transparent_array.push_back();
		break;
	case SC_OPAQUE:
		mesh_new = opaque_array.push_back();
		break;
	case SC_ALPHATEST:
		mesh_new = alphatest_array.push_back();
		break;
	default:
		return false;
	}

	mesh_new->indexCount = indexCount;
	mesh_new->vertexBuffer = vertexBuffer;
	mesh_new->indexBuffer = indexBuffer;
	mesh_new->gpuMatrixBuffer = gpuMatrixBuffer;
	mesh_new->isSkinned = skinned;
	mesh_new->vertexSize = vertexSize;
	mesh_new->material = material;
	mesh_new->topo = topo;
	mesh_new->distanceSq = (center - cameraPosition).LengthSquared();

	return true;
}

void ShadowRenderMgr::DrawOpaque()
{
	const unsigned int offset = 0;

	sort(opaque_array.begin(), opaque_array.end(), BaseRenderMgr::CompareMeshes );
		
	for(auto cur: opaque_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_SHADOW + cur.isSkinned);

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned > 0);
		cur.material->Set(tech);

		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}

	// alphatest
	sort(alphatest_array.begin(), alphatest_array.end(), BaseRenderMgr::CompareMeshes);
	
	for (auto cur : alphatest_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_SHADOW + cur.isSkinned);

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned > 0);
		cur.material->Set(tech);

		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}
}

void ShadowRenderMgr::DrawTransparent()
{
	sort(transparent_array.begin(), transparent_array.end(), BaseRenderMgr::InvCompareMeshes );

	const unsigned int offset = 0;

	for(auto cur: transparent_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_SHADOW + cur.isSkinned);

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned > 0);
		cur.material->Set(tech);

		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}
}