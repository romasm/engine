#include "stdafx.h"
#include "StaticMeshSystem.h"
#include "../World.h"
#include "../Render.h"

using namespace EngineCore;

StaticMeshSystem::StaticMeshSystem(World* world)
{
	FrustumMgr* frustumMgr = world->GetFrustumMgr();
	frustums = frustumMgr->dataArray;

	transformSys = world->GetTransformSystem();
	visibilitySys = world->GetVisibilitySystem();
	earlyVisibilitySys = world->GetEarlyVisibilitySystem();
}

StaticMeshSystem::~StaticMeshSystem()
{
	for(auto& i: *components.data())
		destroyMeshData(i);
}

void StaticMeshSystem::PostReload()
{
	for(auto& i: *components.data())
	{
		if(!StMeshMgr::Get()->IsJustReloaded(i.stmesh))
			continue;

		auto meshPtr = StMeshMgr::GetStMeshPtr(i.stmesh);
		visibilitySys->SetBBox(i.parent, meshPtr->box);
	}
}

void StaticMeshSystem::RegToDraw()
{
	for(auto& i: *components.data())
	{
		VisibilityComponent* visComponent = visibilitySys->GetComponent(i.get_entity());

		bitset<FRUSTUM_MAX_COUNT> bits;
		if(visComponent)
		{
			bits = visComponent->inFrust;	
			if(bits == 0)
				continue;
		}
		else
		{
			EarlyVisibilityComponent* earlyVisibilityComponent = earlyVisibilitySys->GetComponent(i.get_entity());

			if(earlyVisibilityComponent)
			{
				bits = earlyVisibilityComponent->inFrust;	
				if(bits == 0)
					continue;
			}
			else
				bits = 0;
		}

		StMeshData* meshPtr = nullptr;

		if(i.dirty)
		{
			meshPtr = StMeshMgr::GetStMeshPtr(i.stmesh);
			TransformComponent* transformComponent = transformSys->GetComponent(i.get_entity());

			XMVECTOR scale, pos, rot;
			XMMatrixDecompose(&scale, &rot, &pos, transformComponent->worldMatrix);
			XMMATRIX rotM = XMMatrixRotationQuaternion(rot);
			XMMATRIX scaleM = XMMatrixScalingFromVector(scale);

			XMMATRIX normalMatrix = XMMatrixInverse(nullptr, scaleM);
			normalMatrix = normalMatrix * rotM;

			i.center = XMVector3TransformCoord(XMLoadFloat3(&meshPtr->box.Center), transformComponent->worldMatrix);

			StmMatrixBuffer mb;
			mb.world = XMMatrixTranspose(transformComponent->worldMatrix);
			mb.norm = XMMatrixTranspose(normalMatrix);
			Render::UpdateDynamicResource(i.constantBuffer, (void*)&mb, sizeof(StmMatrixBuffer));
					
			i.dirty = false;
		}
		
		if(bits == 0)
		{
			for(auto& f: *frustums)
			{
				if(!f.rendermgr->IsShadow())
				{
					if(!meshPtr)
						meshPtr = StMeshMgr::GetStMeshPtr(i.stmesh);

					((SceneRenderMgr*)f.rendermgr)->RegMultiMesh(meshPtr->indexCount, meshPtr->vertexBuffer, meshPtr->indexBuffer, i.constantBuffer, sizeof(LitVertex), i.materials, i.center);
				}
			}
			continue;
		}

		for(auto& f: *frustums)
		{
			if((bits & f.bit) == f.bit)
			{
				if(!meshPtr)
					meshPtr = StMeshMgr::GetStMeshPtr(i.stmesh);

				if(f.rendermgr->IsShadow())// todo
				{
					if(i.cast_shadow)
						((ShadowRenderMgr*)f.rendermgr)->RegMultiMesh(meshPtr->indexCount, meshPtr->vertexBuffer, meshPtr->indexBuffer, i.constantBuffer, sizeof(LitVertex), i.materials, i.center);
				}
				else
					((SceneRenderMgr*)f.rendermgr)->RegMultiMesh(meshPtr->indexCount, meshPtr->vertexBuffer, meshPtr->indexBuffer, i.constantBuffer, sizeof(LitVertex), i.materials, i.center);
				
				bits &= ~f.bit;
				if(bits == 0) break;
			}
		}
	}
}

void StaticMeshSystem::CopyComponent(Entity src, Entity dest)
{
	auto comp = GetComponent(src);
	if(!comp) 
		return;

	auto newComp = AddComponent(dest);

	newComp->stmesh = StMeshMgr::Get()->GetStMesh(StMeshMgr::GetStMeshName(comp->stmesh), true);

	auto meshPtr = StMeshMgr::GetStMeshPtr(newComp->stmesh);

	newComp->dirty = true;
	visibilitySys->SetBBox(dest, meshPtr->box);

	newComp->constantBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(StmMatrixBuffer), true);
	
	newComp->materials.create(meshPtr->matCount);
	for(int i=0; i < meshPtr->matCount; i++)
		newComp->materials.push_back(MATERIAL(comp->materials[i]->GetName()));

	newComp->cast_shadow = comp->cast_shadow;

	newComp->editor_only = comp->editor_only;
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == ENTITY_COUNT)	return res;\
	auto& comp = components.getDataByArrayIdx(idx);

bool StaticMeshSystem::IsDirty(Entity e)
{
	GET_COMPONENT(false)
	return comp.dirty;
}

bool StaticMeshSystem::SetDirty(Entity e)
{
	GET_COMPONENT(false)
	comp.dirty = true;
	return true;
}

uint32_t StaticMeshSystem::Serialize(Entity e, uint8_t* data)
{
	GET_COMPONENT(0)

	auto t_data = data;
	uint32_t size = 0;

	uint32_t* size_slot = (uint32_t*)t_data;
	t_data += sizeof(uint32_t);
	size += sizeof(uint32_t);

	*(bool*)t_data = comp.editor_only;
	t_data += sizeof(bool);
	size += sizeof(bool);

	string mesh_name = StMeshMgr::GetStMeshName(comp.stmesh);
	uint32_t mesh_name_size = (uint32_t)mesh_name.size();

	*(uint32_t*)t_data = mesh_name_size;
	t_data += sizeof(uint32_t);
	size += sizeof(uint32_t);

	memcpy_s(t_data, mesh_name_size, mesh_name.data(), mesh_name_size);
	t_data += mesh_name_size * sizeof(char);
	size += mesh_name_size * sizeof(char);

	*(uint32_t*)t_data = (uint32_t)comp.materials.size();
	t_data += sizeof(uint32_t);
	size += sizeof(uint32_t);

	for(auto it: comp.materials)
	{
		string mat_name = it->GetName();
		uint32_t mat_name_size = (uint32_t)mat_name.size();

		*(uint32_t*)t_data = mat_name_size;
		t_data += sizeof(uint32_t);
		size += sizeof(uint32_t);

		memcpy_s(t_data, mat_name_size, mat_name.data(), mat_name_size);
		t_data += mat_name_size;
		size += mat_name_size;
	}

	*size_slot = size - sizeof(uint32_t);

	return size;
}

uint32_t StaticMeshSystem::Deserialize(Entity e, uint8_t* data)
{
	auto t_data = data;
	uint32_t size = 0;

	size = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);

	bool editor = *(bool*)t_data;
	t_data += sizeof(bool);

	uint32_t mesh_name_size = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);
	string mesh_name((char*)t_data, mesh_name_size);
	t_data += mesh_name_size * sizeof(char);

	uint32_t matCount = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);

	RArray<string> mat_names;
	mat_names.create(matCount);
	mat_names.resize(matCount);

	for(auto& it: mat_names)
	{
		uint32_t name_size = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);
		string name((char*)t_data, name_size);
		t_data += name_size * sizeof(char);
		it = name;
	}

	AddComponent(e, mesh_name, mat_names);
	SetEditorOnly(e, editor);				// TODO
	return size + sizeof(uint32_t);
}

bool StaticMeshSystem::IsEditorOnly(Entity e)
{
	GET_COMPONENT(false)
	return comp.editor_only;
}

bool StaticMeshSystem::SetEditorOnly(Entity e, bool editor_only)
{
	GET_COMPONENT(false)
	comp.editor_only = editor_only;
	return true;
}

bool StaticMeshSystem::GetShadow(Entity e)
{
	GET_COMPONENT(false)
	return comp.cast_shadow;
}

bool StaticMeshSystem::SetShadow(Entity e, bool cast)
{
	GET_COMPONENT(false)
	comp.cast_shadow = cast;
	return true;
}

bool StaticMeshSystem::SetMesh(Entity e, string mesh)
{
	GET_COMPONENT(false)
	
	auto oldMesh = comp.stmesh;
	uint16_t oldMatCount = (uint16_t)comp.materials.capacity();
	
	comp.stmesh = StMeshMgr::Get()->GetStMesh( mesh, true );
	StMeshMgr::Get()->DeleteStMesh(oldMesh);

	auto meshPtr = StMeshMgr::GetStMeshPtr(comp.stmesh);

	comp.dirty = true;
	visibilitySys->SetBBox(e, meshPtr->box);

	if(meshPtr->matCount == oldMatCount)
		return true;

	RArray<Material*> temp_materials;
	temp_materials.create(meshPtr->matCount);
	temp_materials.resize(meshPtr->matCount);

	if(meshPtr->matCount < oldMatCount)
	{
		for(int i=0; i < oldMatCount; i++)
		{
			if(i < meshPtr->matCount) temp_materials[i] = comp.materials[i];
			else MATERIAL_PTR_DROP(comp.materials[i]);
		}
		comp.materials.swap(temp_materials);
		temp_materials.destroy();
	}
	else
	{
		for(int i=0; i < meshPtr->matCount; i++)
		{
			if(i < oldMatCount) temp_materials[i] = comp.materials[i];
			else
				temp_materials[i] = MATERIAL_S("$"PATH_SHADERS"objects/opaque_main");
		}
		comp.materials.swap(temp_materials);
		temp_materials.destroy();
	}

	return true;
}

bool StaticMeshSystem::SetMaterial(Entity e, int i, string matname)
{
	GET_COMPONENT(false)
	if(comp.materials[i])
		MATERIAL_PTR_DROP(comp.materials[i]);
	comp.materials[i] = MATERIAL(matname);

	return true;
}

bool StaticMeshSystem::SetMeshMats(Entity e, string& mesh, RArray<string>& mats)
{
	GET_COMPONENT(false)

	for(int i=0; i<comp.materials.size(); i++)
		MATERIAL_PTR_DROP(comp.materials[i]);
		
	comp.materials.destroy();

	auto oldMesh = comp.stmesh;

	comp.stmesh = StMeshMgr::Get()->GetStMesh( mesh, true );
	StMeshMgr::Get()->DeleteStMesh(oldMesh);

	auto meshPtr = StMeshMgr::GetStMeshPtr(comp.stmesh);

	comp.dirty = true;
	visibilitySys->SetBBox(e, meshPtr->box);
	
	comp.materials.create(meshPtr->matCount);

	if(mats.size() != meshPtr->matCount)
		WRN("Materials wrong count for mesh %s", mesh.c_str());

	for(int i=0; i < meshPtr->matCount; i++)
	{
		string mat_name;
		if(i < mats.size())
			mat_name = mats[i];
		else
			mat_name = "";

		comp.materials.push_back(MATERIAL(mat_name));
	}

	return true;
}

uint32_t StaticMeshSystem::GetMeshID(Entity e)
{
	GET_COMPONENT(STMESH_NULL)
	return comp.stmesh;
}

Material* StaticMeshSystem::GetMaterial(Entity e, int i)
{
	GET_COMPONENT(nullptr)
	return comp.materials[i];
}

uint16_t StaticMeshSystem::GetMaterialsCount(Entity e)
{
	GET_COMPONENT(0)
	return (uint16_t)comp.materials.size();
}

XMVECTOR StaticMeshSystem::GetCenter(Entity e)
{
	GET_COMPONENT(XMVectorZero())
	return comp.center;
}

void StaticMeshSystem::DeleteComponent(Entity e)
{
	GET_COMPONENT(void())
	destroyMeshData(comp);
	components.remove(e.index());
}

void StaticMeshSystem::destroyMeshData(StaticMeshComponent& comp)
{
	for(int i=0; i<comp.materials.size(); i++)
		MATERIAL_PTR_DROP(comp.materials[i]);
		
	comp.materials.destroy();

	StMeshMgr::Get()->DeleteStMesh(comp.stmesh);
	comp.stmesh = STMESH_NULL;

	_RELEASE(comp.constantBuffer);
}