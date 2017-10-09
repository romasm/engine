#include "stdafx.h"
#include "StaticMeshSystem.h"
#include "World.h"
#include "WorldMgr.h"
#include "Render.h"

using namespace EngineCore;

StaticMeshSystem::StaticMeshSystem(BaseWorld* w, uint32_t maxCount)
{
	world = w;
	frustumMgr = w->GetFrustumMgr();

	transformSys = w->GetTransformSystem();
	visibilitySys = w->GetVisibilitySystem();
	earlyVisibilitySys = w->GetEarlyVisibilitySystem();
	skeletonSystem = w->GetSkeletonSystem();

	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);
}

StaticMeshSystem::~StaticMeshSystem()
{
	for(auto& i: *components.data())
		destroyMeshData(i);
}

#ifdef _EDITOR
void StaticMeshSystem::FixBBoxes()
{
	for(auto& i: *components.data())
	{
		if(!MeshMgr::Get()->IsJustReloaded(i.stmesh))
			continue;

		auto meshPtr = MeshMgr::GetResourcePtr(i.stmesh);
		visibilitySys->SetBBox(i.parent, meshPtr->box);
	}
}
#endif

void StaticMeshSystem::RegToDraw()
{
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

		MeshData* meshPtr = MeshMgr::GetResourcePtr(i.stmesh); // TODO: non thread safe - mesh may be deleted in background
		if(!meshPtr)
			continue;			

		auto skeleton = skeletonSystem->GetComponent(i.get_entity());
		ID3D11Buffer* matrixBuf = nullptr;
		
		if(!skeleton)
		{
			if(i.dirty)
			{		
				XMMATRIX worldMatrix = transformSys->GetTransformW(i.get_entity());

				XMVECTOR scale, pos, rot;
				XMMatrixDecompose(&scale, &rot, &pos, worldMatrix);
				XMMATRIX rotM = XMMatrixRotationQuaternion(rot);
				XMMATRIX scaleM = XMMatrixScalingFromVector(scale);

				XMMATRIX normalMatrix = XMMatrixInverse(nullptr, scaleM);
				normalMatrix = normalMatrix * rotM;

				i.center = XMVector3TransformCoord(XMLoadFloat3(&meshPtr->box.Center), worldMatrix);

				i.matrixBuffer.world = XMMatrixTranspose(worldMatrix);
				i.matrixBuffer.norm = XMMatrixTranspose(normalMatrix);
				Render::UpdateDynamicResource(i.constantBuffer, (void*)&i.matrixBuffer, sizeof(StmMatrixBuffer));

				i.dirty = false;
			}	

			matrixBuf = i.constantBuffer;
		}
		else
		{
			if(i.dirty)
			{
				i.dirty = false;
			}

			matrixBuf = skeleton->constantBuffer;
		}

		if( bits == 0 )
		{
			for( auto& f: *(frustumMgr->m_frustums.data()) )
			{
				if( !f.rendermgr->IsShadow() )
				{
					((SceneRenderMgr*)f.rendermgr)->RegMultiMesh(meshPtr, matrixBuf, i.materials, i.center);
				}
			}
			continue;
		}

		for( auto& f: *(frustumMgr->m_frustums.data()) )
		{
			if( (bits & f.bit) == f.bit )
			{
				if( f.rendermgr->IsShadow() )// todo
				{
					if(i.cast_shadow)
						((ShadowRenderMgr*)f.rendermgr)->RegMultiMesh(meshPtr, matrixBuf, i.materials, i.center);
				}
				else
				{
					if( !f.is_volume )
					{
						((SceneRenderMgr*)f.rendermgr)->RegMultiMesh(meshPtr, matrixBuf, i.materials, i.center);
					}
					else if(visComponent && !skeleton)		// voxelize, temp skeleton mesh disabled
					{
						for(int32_t mat_i = 0; mat_i < i.materials.size(); mat_i++)
							((SceneRenderMgr*)f.rendermgr)->voxelRenderer->RegMeshForVCT(meshPtr->indexBuffers[mat_i], meshPtr->vertexBuffers[mat_i], 
								meshPtr->vertexFormat, i.materials[mat_i], i.matrixBuffer, visComponent->worldBox);
					}
				}

				bits &= ~f.bit;
				if(bits == 0) break;
			}
		}
	}
}

StaticMeshComponent* StaticMeshSystem::AddComponent(Entity e)
{
	StaticMeshComponent* res = components.add(e.index());
	res->parent = e;

	auto skeleton = skeletonSystem->GetComponent(e);
	if(!skeleton)
	{
		res->constantBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(StmMatrixBuffer), true);
	}
	else
	{
		res->constantBuffer = nullptr;
	}
	
	auto meshPtr = MeshMgr::GetResourcePtr(res->stmesh);
	if(meshPtr)
		visibilitySys->SetBBox(e, meshPtr->box);

	return res;
}

void StaticMeshSystem::CopyComponent(Entity src, Entity dest)
{
	auto comp = GetComponent(src);
	if(!comp) 
		return;

	auto newComp = AddComponent(dest);

	newComp->stmesh = MeshMgr::Get()->GetResource(MeshMgr::GetName(comp->stmesh));

	auto meshPtr = MeshMgr::GetResourcePtr(newComp->stmesh);

	newComp->dirty = true;
	visibilitySys->SetBBox(dest, meshPtr->box);
	
	auto matCount = meshPtr->vertexBuffers.size();
	newComp->materials.reserve(matCount);
	for(int i=0; i < matCount; i++)
		newComp->materials.push_back(MATERIAL(comp->materials[i]->GetName()));

	newComp->cast_shadow = comp->cast_shadow;
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == components.capacity())	return res;\
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
	
	string mesh_name = MeshMgr::GetName(comp.stmesh);
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
	
	uint32_t mesh_name_size = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);
	string mesh_name((char*)t_data, mesh_name_size);
	t_data += mesh_name_size * sizeof(char);

	uint32_t matCount = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);

	DArray<string> mat_names;
	mat_names.reserve(matCount);
	mat_names.resize(matCount);

	for(auto& it: mat_names)
	{
		uint32_t name_size = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);
		string name((char*)t_data, name_size);
		t_data += name_size * sizeof(char);
		it = name;
	}

	auto comp = AddComponent(e);
	if(comp)
		setMeshMats(comp, mesh_name, mat_names);

	return size + sizeof(uint32_t);
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

	return setMesh(&comp, mesh, LuaRef(LSTATE));
}

bool StaticMeshSystem::SetMeshAndCallback(Entity e, string mesh, LuaRef func)
{
	GET_COMPONENT(false)

	LuaRef nullLuaPtr(LSTATE);
	return setMesh(&comp, mesh, func);
}

bool StaticMeshSystem::SetMaterial(Entity e, int32_t i, string matname)
{
	GET_COMPONENT(false)
	const auto oldSize = comp.materials.size();
	if(i >= oldSize)
	{
		const auto newSize = i + 1;
		comp.materials.reserve(newSize);
		comp.materials.resize(newSize);
		comp.materials.assign(nullptr, oldSize, newSize);
	}

	if(comp.materials[i])
		MATERIAL_PTR_DROP(comp.materials[i]);
	comp.materials[i] = MATERIAL(matname);

	return true;
}

bool StaticMeshSystem::setMesh(StaticMeshComponent* comp, string& mesh, LuaRef func)
{
	auto oldMesh = comp->stmesh;

	auto worldID = world->GetID();
	auto ent = comp->get_entity();

	// TODO: potential memory leak if callback never will be called
	// This fixes wrong LuaRef capture by lambda
	LuaRef* luaRef = new LuaRef(func);

	comp->stmesh = MeshMgr::Get()->GetResource( mesh, CONFIG(bool, reload_resources), 

		[ent, worldID, luaRef](uint32_t id, bool status) -> void
	{
		auto meshPtr = MeshMgr::GetResourcePtr(id);
		if(!meshPtr)
		{
			_DELETE((LuaRef*)luaRef);
			return;
		}

		auto worldPtr = WorldMgr::Get()->GetWorld(worldID);
		if(!worldPtr || !worldPtr->IsEntityAlive(ent))
		{
			MeshMgr::Get()->DeleteResource(id);
			_DELETE((LuaRef*)luaRef);
			return;
		}

		auto comp = worldPtr->GetStaticMeshSystem()->GetComponent(ent);
		if(!comp)
		{
			MeshMgr::Get()->DeleteResource(id);
			_DELETE((LuaRef*)luaRef);
			return;
		}
		
		if( MeshLoader::IsSkinned( meshPtr->vertexFormat ) )
		{
			if( !worldPtr->GetSkeletonSystem()->HasComponent(ent) )
				ERR("Skinned mesh %s is setted to static geometry, skeleton must be setted first", MeshMgr::GetName(id));
		}

		comp->dirty = true;

		const Entity e = comp->get_entity();
		worldPtr->GetVisibilitySystem()->SetBBox(e, meshPtr->box);

	#ifdef _EDITOR
		auto lineGeom = worldPtr->GetLineGeometrySystem();
		if(lineGeom)
			lineGeom->SetFromVis(e);
	#endif
		
		auto oldMatCount = comp->materials.size();
		auto newMatCount = meshPtr->vertexBuffers.size();
		if(newMatCount != oldMatCount)
		{
			DArray<Material*> temp_materials;
			temp_materials.reserve(newMatCount);
			temp_materials.resize(newMatCount);

			if(newMatCount < oldMatCount)
			{
				for(int32_t i = 0; i < oldMatCount; i++)
				{
					if(i < newMatCount) 
						temp_materials[i] = comp->materials[i];
					else 
						MATERIAL_PTR_DROP(comp->materials[i]);
				}
			}
			else
			{
				for(int32_t i = 0; i < newMatCount; i++)
				{
					if(i < oldMatCount)
						temp_materials[i] = comp->materials[i];
					else
						temp_materials[i] = MATERIAL_S("");//MATERIAL_S("$"PATH_SHADERS"objects/opaque_main");
				}
			}
			comp->materials.swap(temp_materials);
			temp_materials.destroy();
		}

		if(luaRef->isFunction())
			(*luaRef)(worldPtr, comp->get_entity(), id, status);

		_DELETE((LuaRef*)luaRef);
	});

	MeshMgr::Get()->DeleteResource(oldMesh);
	return true;
}

bool StaticMeshSystem::setMeshMats(StaticMeshComponent* comp, string& mesh, DArray<string>& mats)
{
	for(int i=0; i<comp->materials.size(); i++)
		MATERIAL_PTR_DROP(comp->materials[i]);
	comp->materials.destroy();
		
	auto matsCount = mats.size();
	comp->materials.reserve(matsCount);

	for(int32_t i = 0; i < matsCount; i++)
		comp->materials.push_back(MATERIAL(mats[i]));
	
	return setMesh(comp, mesh, LuaRef(LSTATE));
}

uint32_t StaticMeshSystem::GetMeshID(Entity e)
{
	GET_COMPONENT(MeshMgr::nullres)
	return comp.stmesh;
}

Material* StaticMeshSystem::GetMaterial(Entity e, int32_t i)
{
	GET_COMPONENT(nullptr)
	if(i >= comp.materials.size())
		return nullptr;
	return comp.materials[i];
}

uint16_t StaticMeshSystem::GetMaterialsCount(Entity e)
{
	GET_COMPONENT(0)
	return (uint16_t)comp.materials.size();
}

Vector3 StaticMeshSystem::GetCenter(Entity e)
{
	GET_COMPONENT(Vector3::Zero)
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

	MeshMgr::Get()->DeleteResource(comp.stmesh);
	comp.stmesh = MeshMgr::nullres;

	_RELEASE(comp.constantBuffer);
}