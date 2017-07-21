#include "stdafx.h"
#include "RenderMgrs.h"
#include "ScenePipeline.h"

using namespace EngineCore;

BaseRenderMgr::BaseRenderMgr()
{
	cameraPosition = XMVectorZero();
	meshgroup_count = 0;
}

bool BaseRenderMgr::CompareMeshes(RenderMesh* first, RenderMesh* second)
{
	return XMVectorGetX(XMVector3LengthSq(first->group->center)) <  XMVectorGetX(XMVector3LengthSq(second->group->center));
}

bool BaseRenderMgr::InvCompareMeshes(RenderMesh* first, RenderMesh* second)
{
	return XMVectorGetX(XMVector3LengthSq(first->group->center)) >  XMVectorGetX(XMVector3LengthSq(second->group->center));
}

void BaseRenderMgr::cleanRenderArrayOpaque()
{
	for(auto cur: opaque_array)
	{
		if(!cur->index_count)
			continue;

		if(cur->group)
		{
			MeshGroup<RenderMesh>* temp_group = cur->group;

			for(unsigned int i=0; i<temp_group->mesh_count; i++)
			{
				// CRASH on null mat assign!!!!!!
				temp_group->meshes[i]->group = nullptr;
			}
			_DELETE_ARRAY(temp_group->meshes);
			_DELETE(temp_group);
		}
		cur->index_count = 0;
		_DELETE(cur);
	}
	opaque_array.clear();
}

void BaseRenderMgr::cleanRenderArrayAlphatest()
{
	for(auto cur: alphatest_array)
	{
		if(!cur->index_count)
			continue;

		if(cur->group)
		{
			MeshGroup<RenderMesh>* temp_group = cur->group;

			for(unsigned int i=0; i<temp_group->mesh_count; i++)
			{
				temp_group->meshes[i]->group = nullptr;
			}
			_DELETE_ARRAY(temp_group->meshes);
			_DELETE(temp_group);
		}
		cur->index_count = 0;
		_DELETE(cur);
	}
	alphatest_array.clear();
}

void BaseRenderMgr::cleanRenderArrayTransparenty()
{
	for(auto cur: transparent_array)
	{
		if(!cur->index_count)
			continue;
		if(cur->group)
		{
			MeshGroup<RenderMesh>* temp_group = cur->group;

			for(unsigned int i=0; i<temp_group->mesh_count; i++)
			{
				temp_group->meshes[i]->group = nullptr;
			}
			_DELETE_ARRAY(temp_group->meshes);
			_DELETE(temp_group);
		}
		cur->index_count;
		_DELETE(cur);
	}
	transparent_array.clear();
}

ShadowRenderMgr::ShadowRenderMgr() : BaseRenderMgr()
{
	b_shadow = true;

	opaque_array.create(OPAQUE_SHADOW_MAX);
	alphatest_array.create(OPAQUE_SHADOW_ALPHATEST_MAX);
}

bool ShadowRenderMgr::RegMesh(uint32_t index_count, 
			ID3D11Buffer* vertex_buffer, ID3D11Buffer* index_buffer, ID3D11Buffer* constant_buffer,
			uint32_t vertex_size, Material* material, XMVECTOR center)
{
	if(!material)
		return false;

	bool has_tq = false;
	auto queue = material->GetTechQueue(TECHNIQUES::TECHNIQUE_SHADOW, &has_tq);

	if(!has_tq)
		return false;
	if(queue == GUI_2D || queue == GUI_2D_FONT || queue == GUI_3D || queue == GUI_3D_OVERLAY)
		return false;
	if(!IsTranparentShadows() && (queue == SC_TRANSPARENT || queue == SC_ALPHA))
		return false;
	
	RenderMesh* mesh_new = new RenderMesh;
	mesh_new->index_count = index_count;
	mesh_new->vertex_buffer = vertex_buffer;
	mesh_new->index_buffer = index_buffer;
	mesh_new->constant_buffer = constant_buffer;
	mesh_new->vertex_size = vertex_size;
	mesh_new->material = material;

	MeshGroup<RenderMesh>* group_new = new MeshGroup<RenderMesh>();
	group_new->ID = meshgroup_count;
	meshgroup_count++;
	group_new->meshes = new RenderMesh*[1];
	group_new->mesh_count = 1;
	group_new->meshes[0] = mesh_new;
	group_new->center = XMVectorZero();
	mesh_new->group = group_new;

	switch(queue)
	{
	case SC_TRANSPARENT:
	case SC_ALPHA:
		transparent_array.push_back(mesh_new);
		break;
	case SC_OPAQUE:
		opaque_array.push_back(mesh_new);
		break;
	case SC_ALPHATEST:
		alphatest_array.push_back(mesh_new);
		break;
	}

	return true;
}

bool ShadowRenderMgr::RegMultiMesh(MeshData* mesh, ID3D11Buffer* constant_buffer, DArray<Material*>& material, XMVECTOR center)
{
	const size_t matCount = min<size_t>(mesh->vertexBuffers.size(), material.size());
	if( matCount == 0 )
		return false;

	MeshGroup<RenderMesh>* group_new = new MeshGroup<RenderMesh>();
	group_new->ID = meshgroup_count;
	meshgroup_count++;
	group_new->meshes = new RenderMesh*[matCount];
	group_new->center = center - cameraPosition;
	group_new->mesh_count = (uint32_t)matCount;

	uint16_t reged = 0;
	for(uint16_t i = 0; i < matCount; i++)
	{
		if( !material[i] )
			continue;

		bool has_tq = false;
		auto queue = material[i]->GetTechQueue(TECHNIQUES::TECHNIQUE_SHADOW, &has_tq);

		if(!has_tq)
			continue;
		if(queue == GUI_2D || queue == GUI_2D_FONT || queue == GUI_3D || queue == GUI_3D_OVERLAY)
			continue;
		if(!IsTranparentShadows() && (queue == SC_TRANSPARENT || queue == SC_ALPHA))
			continue;

		RenderMesh* mesh_new = new RenderMesh;
		mesh_new->index_count = mesh->indexBuffers[i].size;
		mesh_new->vertex_buffer = mesh->vertexBuffers[i].buffer;
		mesh_new->index_buffer = mesh->indexBuffers[i].buffer;
		mesh_new->constant_buffer = constant_buffer;
		mesh_new->vertex_size = MeshLoader::GetVertexSize(mesh->vertexFormat);
		mesh_new->material = material[i];

		group_new->meshes[i] = mesh_new;
		mesh_new->group = group_new;

		reged++;
		switch(queue)
		{
		case SC_TRANSPARENT:
		case SC_ALPHA:
			transparent_array.push_back(mesh_new);
			break;
		case SC_OPAQUE:
			opaque_array.push_back(mesh_new);
			break;
		case SC_ALPHATEST:
			alphatest_array.push_back(mesh_new);
			break;
		}
	}

	if(!reged)
	{
		_DELETE(group_new);
		meshgroup_count--;
	}

	return true;
}

void ShadowRenderMgr::DrawOpaque()
{
	sort(opaque_array.begin(), opaque_array.end(), BaseRenderMgr::CompareMeshes );

	const unsigned int offset = 0;
	
	for(auto cur: opaque_array)
	{
		Render::Context()->IASetVertexBuffers(0, 1, &(cur->vertex_buffer), &(cur->vertex_size), &offset);
		Render::Context()->IASetIndexBuffer(cur->index_buffer, DXGI_FORMAT_R32_UINT, 0);

		cur->material->SetMatrixBuffer(cur->constant_buffer);

		cur->material->Set(TECHNIQUES::TECHNIQUE_SHADOW);

		Render::Context()->DrawIndexed(cur->index_count, 0, 0);
	}
}

void ShadowRenderMgr::DrawAlphatest()
{
	sort(alphatest_array.begin(), alphatest_array.end(), BaseRenderMgr::CompareMeshes );

	const unsigned int offset = 0;

	for(auto cur: alphatest_array)
	{
		Render::Context()->IASetVertexBuffers(0, 1, &(cur->vertex_buffer), &(cur->vertex_size), &offset);
		Render::Context()->IASetIndexBuffer(cur->index_buffer, DXGI_FORMAT_R32_UINT, 0);

		cur->material->SetMatrixBuffer(cur->constant_buffer);

		cur->material->Set(TECHNIQUES::TECHNIQUE_SHADOW);

		Render::Context()->DrawIndexed(cur->index_count, 0, 0);
	}
}

void ShadowRenderMgr::DrawTransparent()
{
	sort(transparent_array.begin(), transparent_array.end(), BaseRenderMgr::InvCompareMeshes );

	const unsigned int offset = 0;

	for(auto cur: transparent_array)
	{
		Render::Context()->IASetVertexBuffers(0, 1, &(cur->vertex_buffer), &(cur->vertex_size), &offset);
		Render::Context()->IASetIndexBuffer(cur->index_buffer, DXGI_FORMAT_R32_UINT, 0);

		cur->material->SetMatrixBuffer(cur->constant_buffer);

		//cur->material->SendVectorToShader(Vector4(float(scene->Light_NoShadows_Count), float(scene->Light_Shadows_Count), 0, 0), 0, ID_PS);

		cur->material->Set(TECHNIQUES::TECHNIQUE_SHADOW);

		Render::Context()->DrawIndexed(cur->index_count, 0, 0);
	}
}