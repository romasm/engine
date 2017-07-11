#include "stdafx.h"
#include "LineGeometrySystem.h"
#include "World.h"
#include "Render.h"
#include "MaterialMgr.h"

using namespace EngineCore;

LineGeometrySystem::LineGeometrySystem(BaseWorld* w, uint32_t maxCount)
{
	world = w;
	frustumMgr = w->GetFrustumMgr();

	transformSys = w->GetTransformSystem();
	visibilitySys = w->GetVisibilitySystem();
	earlyVisibilitySys = w->GetEarlyVisibilitySystem();
	
	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);
}

void LineGeometrySystem::RegToDraw()
{
	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		EarlyVisibilityComponent* earlyVisibilityComponent = earlyVisibilitySys->GetComponent(i.get_entity());
		
		bitset<FRUSTUM_MAX_COUNT> bits;
		if(earlyVisibilityComponent)
		{
			bits = earlyVisibilityComponent->inFrust;	
			if(bits == 0)
				continue;
		}
		else
		{
			VisibilityComponent* visComponent = visibilitySys->GetComponent(i.get_entity());

			if(visComponent)
			{
				bits = visComponent->inFrust;	
				if(bits == 0)
					continue;
			}
			else
				bits = 0;
		}

		if(i.dirty)
		{
			XMMATRIX worldMatrix = transformSys->GetTransformW(i.get_entity());

			LineGeometryBuffer mb;
			mb.world = XMMatrixTranspose(worldMatrix);
			Render::UpdateDynamicResource(i.constantBuffer, (void*)&mb, sizeof(LineGeometryBuffer));			
					
			i.dirty = false;
		}
		
		if(bits == 0)
		{
			for(auto f: frustumMgr->camDataArray)
				((SceneRenderMgr*)f->rendermgr)->RegMesh(i.index_count, i.vertexBuffer, i.indexBuffer, i.constantBuffer, sizeof(LineGeometryVertex), i.material, 
					IA_TOPOLOGY::LINELIST);
			
			continue;
		}

		for(auto f: frustumMgr->camDataArray)
		{
			if((bits & f->bit) == f->bit)
			{
				((SceneRenderMgr*)f->rendermgr)->RegMesh(i.index_count, i.vertexBuffer, i.indexBuffer, i.constantBuffer, sizeof(LineGeometryVertex), i.material,
					IA_TOPOLOGY::LINELIST);

				bits &= ~f->bit;
				if(bits == 0) break;
			}
		}
	}
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == components.capacity())	return res;\
	auto& comp = components.getDataByArrayIdx(idx);

bool LineGeometrySystem::IsDirty(Entity e)
{
	GET_COMPONENT(false)
	return comp.dirty;
}

bool LineGeometrySystem::SetDirty(Entity e)
{
	GET_COMPONENT(false)
	comp.dirty = true;
	return true;
}

bool LineGeometrySystem::IsActive(Entity e)
{
	GET_COMPONENT(false)
	return comp.dirty;
}

bool LineGeometrySystem::SetActive(Entity e, bool active)
{
	GET_COMPONENT(false)
	comp.active = active;
	return true;
}

bool LineGeometrySystem::SetFromVis(Entity e, bool forceEVS)
{
	GET_COMPONENT(false)
	
	VisibilityComponent* visComponent = visibilitySys->GetComponent(comp.get_entity());
		
	if(visComponent && !forceEVS)
	{
		SetBox(e, visComponent->localBox);
	}
	else
	{
		EarlyVisibilityComponent* earlyVisibilityComponent = earlyVisibilitySys->GetComponent(comp.get_entity());

		if(earlyVisibilityComponent)
		{
			switch (earlyVisibilityComponent->type)
			{
			case BT_BOX:
				SetBox(e, earlyVisibilityComponent->localBox);
				break;
			case BT_SPHERE:
				SetSphere(e, earlyVisibilityComponent->localSphere.Radius);
				break;
			case BT_FRUSTUM:
			case BT_FRUSTUM_SPHERE:
				SetBox(e, earlyVisibilityComponent->localFrustum);
				break;
			}
		}
		else
		{
			SetPoint(e);
			return false;
		}
	}

	return true;
}

LineGeometryTypes LineGeometrySystem::GetType(Entity e)
{
	GET_COMPONENT(LineGeometryTypes())
	return comp.type;
}

void LineGeometrySystem::DeleteComponent(Entity e)
{
	GET_COMPONENT(void())

	DestroyGeometry(&comp, true);
	components.remove(e.index());
}

void LineGeometrySystem::DestroyGeometry(LineGeometryComponent* comp, bool delete_all)
{
	if(delete_all)
	{
		_RELEASE(comp->constantBuffer);
	}
	MATERIAL_PTR_DROP(comp->material);
	_RELEASE(comp->indexBuffer);
	comp->index_count = 0;
	_RELEASE(comp->vertexBuffer);
}

#define INIT_BUFS \
	comp.vertexBuffer = Buffer::CreateVertexBuffer(Render::Device(), sizeof(LineGeometryVertex) * vertex_count, false, &vertices);\
	if(!comp.vertexBuffer)return false;\
	comp.index_count = index_count;\
	comp.indexBuffer = Buffer::CreateIndexBuffer(Render::Device(), sizeof(unsigned long) * index_count, false, &indices);\
	if(!comp.indexBuffer)return false;\
	return true;

#define UPD_BUFS \
	Render::UpdateSubresource(comp.vertexBuffer, 0, nullptr, (void*)vertices, 0, 0); return true;

#define INIT_MAT \
	comp.material = MATERIAL_S(LG_MAT);

bool LineGeometrySystem::SetLine(Entity e, XMFLOAT3 p1, XMFLOAT3 p2)
{
	GET_COMPONENT(false)

	const UINT vertex_count = 2;
	LineGeometryVertex vertices[vertex_count];
	vertices[0].pos = p1;
	vertices[1].pos = p2;

	if(comp.type == LG_LINE && comp.vertexBuffer)
	{
		UPD_BUFS
	}
	else
	{
		DestroyGeometry(&comp, false);
		comp.type = LG_LINE;

		const UINT index_count = 2;
		const unsigned long indices[index_count] = {0,1};

		INIT_MAT
		INIT_BUFS
	}
}

#define INIT_BOX_VRTX \
	const UINT vertex_count = 8;\
	LineGeometryVertex vertices[vertex_count];\
	XMFLOAT3 box_corners[8];\
	box.GetCorners(box_corners);\
	vertices[0].pos = box_corners[0];\
	vertices[1].pos = box_corners[1];\
	vertices[2].pos = box_corners[2];\
	vertices[3].pos = box_corners[3];\
	vertices[4].pos = box_corners[4];\
	vertices[5].pos = box_corners[5];\
	vertices[6].pos = box_corners[6];\
	vertices[7].pos = box_corners[7];

#define INIT_BOX \
	DestroyGeometry(&comp, false);\
	comp.type = LG_BOX;\
	const UINT index_count = 24;\
	const unsigned long indices[index_count] = {0,1,1,2,0,4,2,3,0,3,2,6,1,5,6,7,4,5,4,7,5,6,3,7};

bool LineGeometrySystem::SetBox(Entity e, BoundingBox box)
{
	GET_COMPONENT(false)
		
	INIT_BOX_VRTX

	if(comp.type == LG_BOX && comp.vertexBuffer)
	{
		UPD_BUFS
	}
	else
	{
		INIT_BOX
		INIT_MAT
		INIT_BUFS
	}	
}

bool LineGeometrySystem::SetBox(Entity e, BoundingOrientedBox box)
{
	GET_COMPONENT(false)
		
	INIT_BOX_VRTX

	if(comp.type == LG_BOX && comp.vertexBuffer)
	{
		UPD_BUFS
	}
	else
	{
		INIT_BOX
		INIT_MAT
		INIT_BUFS
	}	
}

bool LineGeometrySystem::SetBox(Entity e, BoundingFrustum box)
{
	GET_COMPONENT(false)
		
	INIT_BOX_VRTX

	if(comp.type == LG_BOX && comp.vertexBuffer)
	{
		UPD_BUFS
	}
	else
	{
		INIT_BOX
		INIT_MAT
		INIT_BUFS
	}	
}

bool LineGeometrySystem::SetSpline(Entity e, XMFLOAT3* p, UINT size)
{
	GET_COMPONENT(false)

	const UINT vertex_count = size;
	const UINT index_count = (vertex_count - 1) * 2;

	const UINT max_vertex_count = LG_SPLINE_MAX / 2 + 1;
	LineGeometryVertex vertices[max_vertex_count];

	for(UINT i=0; i<vertex_count; i++)
		vertices[i].pos = p[i];

	if(comp.type == LG_SPLINE && comp.vertexBuffer)
	{
		comp.index_count = index_count;
		UPD_BUFS
	}
	else
	{
		DestroyGeometry(&comp, false);
		comp.type = LG_SPLINE;

		INIT_MAT

		unsigned long indices[LG_SPLINE_MAX];
		for(UINT i=0; i<LG_SPLINE_MAX; i+=2)
		{
			indices[i] = i / 2;
			indices[i+1] = indices[i] + 1;
		}

		comp.vertexBuffer = Buffer::CreateVertexBuffer(Render::Device(), sizeof(LineGeometryVertex) * max_vertex_count, false, &vertices);
		if(!comp.vertexBuffer)
			return false;
	
		comp.indexBuffer = Buffer::CreateIndexBuffer(Render::Device(), sizeof(unsigned long) * LG_SPLINE_MAX, false, &indices);
		if(!comp.indexBuffer)
			return false;

		comp.index_count = index_count;

		return true;
	}
}

bool LineGeometrySystem::SetPoint(Entity e)
{
	GET_COMPONENT(false)

	if(comp.type == LG_POINT && comp.vertexBuffer)
		return true;

	DestroyGeometry(&comp, false);
	comp.type = LG_POINT;

	const UINT vertex_count = 6;
	const UINT index_count = 6;
	
	const unsigned long indices[index_count] = {0,1,2,3,4,5};

	LineGeometryVertex vertices[vertex_count];
	vertices[0].pos = XMFLOAT3(LG_POINT_SIZE,0.0f,0.0f);
	vertices[1].pos = XMFLOAT3(-LG_POINT_SIZE,0.0f,0.0f);
	vertices[2].pos = XMFLOAT3(0.0f,LG_POINT_SIZE,0.0f);
	vertices[3].pos = XMFLOAT3(0.0f,-LG_POINT_SIZE,0.0f);
	vertices[4].pos = XMFLOAT3(0.0f,0.0f,LG_POINT_SIZE);
	vertices[5].pos = XMFLOAT3(0.0f,0.0f,-LG_POINT_SIZE);

	INIT_MAT
	INIT_BUFS
}

bool LineGeometrySystem::SetSphere(Entity e, float radius)
{
	GET_COMPONENT(false)

	if(comp.type == LG_SPHERE && comp.vertexBuffer)
	{
		comp.material->SetFloat(radius, 0, SHADER_VS);
		return true;
	}

	DestroyGeometry(&comp, false);
	comp.type = LG_SPHERE;

	const UINT vertex_count = LG_SPHERE_SUBD * 3;
	const UINT index_count = vertex_count * 2;
	
	unsigned long indices[index_count];
	LineGeometryVertex vertices[vertex_count];

	const float step = XM_2PI / LG_SPHERE_SUBD;

	for(int i=0; i<LG_SPHERE_SUBD; i++)
	{
		float angle = i * step;
		int vetex_i = i;
		vertices[vetex_i].pos = XMFLOAT3(sin(angle), cos(angle), 0);
		int index_i = vetex_i * 2;
		indices[index_i] = vetex_i;
		if(i == LG_SPHERE_SUBD - 1)
			indices[index_i + 1] = 0;
		else
			indices[index_i + 1] = vetex_i + 1;

		vetex_i = LG_SPHERE_SUBD + i;
		vertices[vetex_i].pos = XMFLOAT3(sin(angle), 0, cos(angle));
		index_i = vetex_i * 2;
		indices[index_i] = vetex_i;
		if(i == LG_SPHERE_SUBD - 1)
			indices[index_i + 1] = LG_SPHERE_SUBD;
		else
			indices[index_i + 1] = vetex_i + 1;

		vetex_i = 2 * LG_SPHERE_SUBD + i;
		vertices[vetex_i].pos = XMFLOAT3(0, sin(angle), cos(angle));
		index_i = vetex_i * 2;
		indices[index_i] = vetex_i;
		if(i == LG_SPHERE_SUBD - 1)
			indices[index_i + 1] = 2 * LG_SPHERE_SUBD;
		else
			indices[index_i + 1] = vetex_i + 1;
	}

	comp.material = MATERIAL_S(LG_MAT_SPHERE);

	comp.material->SetFloat(radius, 0, SHADER_VS);

	INIT_BUFS
}

bool LineGeometrySystem::SetColor(Entity e, XMFLOAT4 color)
{
	GET_COMPONENT(false)
	if(!comp.material)
		return false;

	comp.material->SetVector(color, 0, SHADER_PS);
	return true;
}