#include "stdafx.h"
#include "SkeletalMeshSystem.h"
#include "../World.h"
#include "../Render.h"

using namespace EngineCore;

SkeletalMeshSystem::SkeletalMeshSystem(BaseWorld* w, uint32_t maxCount)
{
	world = w;
	frustumMgr = w->GetFrustumMgr();

	transformSys = w->GetTransformSystem();
	visibilitySys = w->GetVisibilitySystem();
	earlyVisibilitySys = w->GetEarlyVisibilitySystem();

	maxCount = min(maxCount, ENTITY_COUNT);
	components.create(maxCount);
}

SkeletalMeshSystem::~SkeletalMeshSystem()
{
	for(auto& i: *components.data())
		destroyMeshData(i);
}

void SkeletalMeshSystem::PostReload()
{
	for(auto& i: *components.data())
	{
		if(!SkeletalMeshMgr::Get()->IsJustReloaded(i.SkeletalMesh))
			continue;

		auto meshPtr = SkeletalMeshMgr::GetSkeletalMeshPtr(i.SkeletalMesh);
		visibilitySys->SetBBox(i.parent, meshPtr->box);
	}
}

void SkeletalMeshSystem::RegToDraw()
{
	// temp
	StmMatrixBuffer matrixBuffer;

	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

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
			if(earlyVisibilitySys)
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
			else
				bits = 0;
		}

		SkeletalMeshData* meshPtr = nullptr;

		if(i.dirty || true) // TEMP FOR VCTGI
		{
			meshPtr = SkeletalMeshMgr::GetSkeletalMeshPtr(i.SkeletalMesh);
			TransformComponent* transformComponent = transformSys->GetComponent(i.get_entity());

			XMVECTOR scale, pos, rot;
			XMMatrixDecompose(&scale, &rot, &pos, transformComponent->worldMatrix);
			XMMATRIX rotM = XMMatrixRotationQuaternion(rot);
			XMMATRIX scaleM = XMMatrixScalingFromVector(scale);

			XMMATRIX normalMatrix = XMMatrixInverse(nullptr, scaleM);
			normalMatrix = normalMatrix * rotM;

			i.center = XMVector3TransformCoord(XMLoadFloat3(&meshPtr->box.Center), transformComponent->worldMatrix);

			matrixBuffer.world = XMMatrixTranspose(transformComponent->worldMatrix);
			matrixBuffer.norm = XMMatrixTranspose(normalMatrix);
			Render::UpdateDynamicResource(i.constantBuffer, (void*)&matrixBuffer, sizeof(StmMatrixBuffer));
					
			i.dirty = false;
		}
		
		if( bits == 0 )
		{
			for( auto& f: *(frustumMgr->m_frustums.data()) )
			{
				if( !f.rendermgr->IsShadow() )
				{
					if(!meshPtr)
						meshPtr = SkeletalMeshMgr::GetSkeletalMeshPtr(i.SkeletalMesh);

					((SceneRenderMgr*)f.rendermgr)->RegMultiMesh(meshPtr->indexCount, meshPtr->vertexBuffer, meshPtr->indexBuffer, i.constantBuffer, sizeof(LitVertex), i.materials, i.center);
				}
			}
			continue;
		}

		for( auto& f: *(frustumMgr->m_frustums.data()) )
		{
			if( (bits & f.bit) == f.bit )
			{
				if(!meshPtr)
					meshPtr = SkeletalMeshMgr::GetSkeletalMeshPtr(i.SkeletalMesh);

				if( f.rendermgr->IsShadow() )// todo
				{
					if(i.cast_shadow)
						((ShadowRenderMgr*)f.rendermgr)->RegMultiMesh(meshPtr->indexCount, meshPtr->vertexBuffer, meshPtr->indexBuffer, i.constantBuffer, sizeof(LitVertex), i.materials, i.center);
				}
				else
				{
					if( !f.is_volume )
					{
						((SceneRenderMgr*)f.rendermgr)->RegMultiMesh(meshPtr->indexCount, meshPtr->vertexBuffer, meshPtr->indexBuffer, i.constantBuffer, sizeof(LitVertex), i.materials, i.center);
					}
					else if(visComponent)		// voxelize
					{
						for(int32_t mat_i = 0; mat_i < i.materials.size(); mat_i++)
							((SceneRenderMgr*)f.rendermgr)->voxelRenderer->RegMeshForVCT(meshPtr->indexCount[mat_i], sizeof(LitVertex), meshPtr->indexBuffer[mat_i], 
								meshPtr->vertexBuffer[mat_i], i.materials[mat_i], matrixBuffer, visComponent->worldBox);
					}
				}

				bits &= ~f.bit;
				if(bits == 0) break;
			}
		}
	}
}

void SkeletalMeshSystem::CopyComponent(Entity src, Entity dest) // TODO!!! copied with sky shader, then crashed on delete
{
	auto comp = GetComponent(src);
	if(!comp) 
		return;

	auto newComp = AddComponent(dest);

	newComp->SkeletalMesh = SkeletalMeshMgr::Get()->GetSkeletalMesh(SkeletalMeshMgr::GetSkeletalMeshName(comp->SkeletalMesh), true);

	auto meshPtr = SkeletalMeshMgr::GetSkeletalMeshPtr(newComp->SkeletalMesh);

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
	if(idx == components.capacity())	return res;\
	auto& comp = components.getDataByArrayIdx(idx);

bool SkeletalMeshSystem::IsDirty(Entity e)
{
	GET_COMPONENT(false)
	return comp.dirty;
}

bool SkeletalMeshSystem::SetDirty(Entity e)
{
	GET_COMPONENT(false)
	comp.dirty = true;
	return true;
}

uint32_t SkeletalMeshSystem::Serialize(Entity e, uint8_t* data)
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

	string mesh_name = SkeletalMeshMgr::GetSkeletalMeshName(comp.SkeletalMesh);
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

uint32_t SkeletalMeshSystem::Deserialize(Entity e, uint8_t* data)
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

bool SkeletalMeshSystem::IsEditorOnly(Entity e)
{
	GET_COMPONENT(false)
	return comp.editor_only;
}

bool SkeletalMeshSystem::SetEditorOnly(Entity e, bool editor_only)
{
	GET_COMPONENT(false)
	comp.editor_only = editor_only;
	return true;
}

bool SkeletalMeshSystem::GetShadow(Entity e)
{
	GET_COMPONENT(false)
	return comp.cast_shadow;
}

bool SkeletalMeshSystem::SetShadow(Entity e, bool cast)
{
	GET_COMPONENT(false)
	comp.cast_shadow = cast;
	return true;
}

bool SkeletalMeshSystem::SetMesh(Entity e, string mesh)
{
	GET_COMPONENT(false)
	
	auto oldMesh = comp.SkeletalMesh;
	uint16_t oldMatCount = (uint16_t)comp.materials.capacity();
	
	comp.SkeletalMesh = SkeletalMeshMgr::Get()->GetSkeletalMesh( mesh, true );
	SkeletalMeshMgr::Get()->DeleteSkeletalMesh(oldMesh);

	auto meshPtr = SkeletalMeshMgr::GetSkeletalMeshPtr(comp.SkeletalMesh);

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

bool SkeletalMeshSystem::SetMaterial(Entity e, int i, string matname)
{
	GET_COMPONENT(false)
	if(comp.materials[i])
		MATERIAL_PTR_DROP(comp.materials[i]);
	comp.materials[i] = MATERIAL(matname);

	return true;
}

bool SkeletalMeshSystem::SetMeshMats(Entity e, string& mesh, RArray<string>& mats)
{
	GET_COMPONENT(false)

	for(int i=0; i<comp.materials.size(); i++)
		MATERIAL_PTR_DROP(comp.materials[i]);
		
	comp.materials.destroy();

	auto oldMesh = comp.SkeletalMesh;

	comp.SkeletalMesh = SkeletalMeshMgr::Get()->GetSkeletalMesh( mesh, true );
	SkeletalMeshMgr::Get()->DeleteSkeletalMesh(oldMesh);

	auto meshPtr = SkeletalMeshMgr::GetSkeletalMeshPtr(comp.SkeletalMesh);

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

uint32_t SkeletalMeshSystem::GetMeshID(Entity e)
{
	GET_COMPONENT(SKELETAL_MESH_NULL)
	return comp.SkeletalMesh;
}

Material* SkeletalMeshSystem::GetMaterial(Entity e, int i)
{
	GET_COMPONENT(nullptr)
	return comp.materials[i];
}

uint16_t SkeletalMeshSystem::GetMaterialsCount(Entity e)
{
	GET_COMPONENT(0)
	return (uint16_t)comp.materials.size();
}

XMVECTOR SkeletalMeshSystem::GetCenter(Entity e)
{
	GET_COMPONENT(XMVectorZero())
	return comp.center;
}

void SkeletalMeshSystem::DeleteComponent(Entity e)
{
	GET_COMPONENT(void())
	destroyMeshData(comp);
	components.remove(e.index());
}

void SkeletalMeshSystem::destroyMeshData(SkeletalMeshComponent& comp)
{
	for(int i=0; i<comp.materials.size(); i++)
		MATERIAL_PTR_DROP(comp.materials[i]);
		
	comp.materials.destroy();

	SkeletalMeshMgr::Get()->DeleteSkeletalMesh(comp.SkeletalMesh);
	comp.SkeletalMesh = SKELETAL_MESH_NULL;

	_RELEASE(comp.constantBuffer);
}