#include "stdafx.h"
#include "TransformSystem.h"
#include "..\World.h"

using namespace EngineCore;

TransformSystem::TransformSystem(BaseWorld* w, uint32_t maxCount)
{	
	world = w;
	structureChanged = true;
	
	capacity = min(maxCount, ENTITY_COUNT);

	components.create(capacity);

	lookup.create(capacity);
	lookup.resize(capacity);
	lookup.assign(capacity);

	dirty.create(capacity);
	hierarchy_sort.create(capacity);
	links_fix.create(capacity);

	attachments_map = nullptr;
}

void TransformSystem::Update()
{
	if(structureChanged)
	{
		hierarchy_sort.resize(components.size());
		links_fix.resize(components.size());
		
		// build hierarchy params
		for(uint32_t i = 0; i < hierarchy_sort.size(); i++)
		{
			hierarchy_sort[i].new_id = i;
			hierarchy_sort[i].hierarchy = -1;
		}

		for(uint32_t i = 0; i < components.size(); i++)
		{
			if(hierarchy_sort[i].hierarchy >= 0)
				continue;
			
			uint32_t compID = i;
			auto comp = &components[compID];

			hi_buffer.resize(0);
			hi_buffer.push_back(compID);

			while( comp->parentID < capacity && hierarchy_sort[comp->parentID].hierarchy < 0 )
			{
				compID = comp->parentID;
				comp = &components[compID];
				hi_buffer.push_back(compID);
			}
			
			if(hi_buffer.full())
				ERR("Scene hierarchy is too deep! Unpredictable behavior expected!");
			
			int16_t hi = 0;
			if(comp->parentID < capacity)
				hi = hierarchy_sort[comp->parentID].hierarchy + 1;

			for(int32_t j = (int32_t)hi_buffer.size() - 1; j >= 0; j--)
			{
				hierarchy_sort[hi_buffer[j]].hierarchy = hi;
				hi++;
			}
		}

		// sorting IDs
		sort(hierarchy_sort.begin(), hierarchy_sort.end(), 
			[](const sort_data& a, const sort_data& b) -> bool { return a.hierarchy < b.hierarchy;});
		
		// reorder components
		for (uint32_t i = 0; i < components.size(); i++)
		{
			if (hierarchy_sort[i].hierarchy < 0)
				continue;
			
			uint32_t move_to = i;

			TransformComponent temp_comp = components[move_to];
			bool temp_dirty = dirty[move_to];
						
			uint32_t move_from;
			while( (move_from = hierarchy_sort[move_to].new_id) != i )
			{
				hierarchy_sort[move_from].hierarchy = -1;

				components[move_to] = components[move_from];
				dirty[move_to] = dirty[move_from];
				lookup[components[move_to].get_id()] = move_to;

				links_fix[move_from] = move_to;

				move_to = move_from;				
			}

			hierarchy_sort[move_from].hierarchy = -1;

			components[move_to] = temp_comp;
			dirty[move_to] = temp_dirty;
			lookup[components[move_to].get_id()] = move_to;

			links_fix[i] = move_to;
		}

		// fix links
		for(uint32_t i = 0; i < components.size(); i++)
		{
			if(components[i].parentID < capacity)
				components[i].parentID = links_fix[components[i].parentID];
			if(components[i].firstChildID < capacity)
				components[i].firstChildID = links_fix[components[i].firstChildID];
			if(components[i].prevID < capacity)
				components[i].prevID = links_fix[components[i].prevID];
			if(components[i].nextID < capacity)
				components[i].nextID = links_fix[components[i].nextID];
		}

		structureChanged = false;
	}

	for(uint32_t i = 0; i < dirty.size(); i++)
	{
		if(dirty[i])
			UpdateComponent(i);
	}
}

#define GET_COMPONENT(res) uint32_t idx = lookup[e.index()];\
	if(idx >= capacity)	return res;\
	auto& comp = components[idx];

void TransformSystem::DeleteComponent(Entity e)
{
	uint32_t currentID = lookup[e.index()];
	if(currentID >= capacity)
		return;
	auto& comp = components[currentID];

	_detach(comp, currentID);
	_detachChildren(comp);
	
	uint32_t lastID = (uint32_t)components.size() - 1;
	if(lastID != currentID)
		_moveComponentPrepare(lastID, currentID);

	uint32_t rmv_id = lookup[e.index()];
	if(rmv_id >= capacity)
		return;
		
	if(rmv_id != lastID)
	{
		uint32_t old_id = lastID;
		lookup[components[old_id].get_id()] = rmv_id;

		structureChanged = true;
	}
	components.erase_and_pop_back(rmv_id);
	dirty.erase_and_pop_back((size_t)rmv_id);
	lookup[e.index()] = capacity;
}

bool TransformSystem::IsDirty(Entity e)
{
	uint32_t idx = lookup[e.index()];
	if(idx >= capacity)
		return false;
	return dirty[idx];
}

bool TransformSystem::SetDirty(Entity e)
{
	uint32_t idx = lookup[e.index()];
	if(idx >= capacity)
		return false;
	dirty[idx] = true;

	//	TODO???
	auto& comp = components[idx];
	uint32_t child = comp.firstChildID;
	while( child < capacity )
	{
		auto& childComp = components[child];
		world->SetDirty(childComp.get_entity());
		child = childComp.nextID;
	}

	return true;
}

bool TransformSystem::Attach(Entity child, Entity parent)
{
	uint32_t childID = lookup[child.index()];
	if(childID >= capacity)
	{
		ERR("Attachable component does not exist!");
		return false;
	}

	if(parent.isnull())
		return Detach(child);

	auto& childComp = components[childID];

	uint32_t parentID = lookup[parent.index()];
	if(parentID >= capacity)
	{
		ERR("Component to attach does not exist!");
		return false;
	}
	auto& parentComp = components[parentID];

	_detach(childComp, childID);
	
	childComp.parentID = parentID;

	if(parentComp.firstChildID < capacity)
	{
		uint32_t otherChildID = parentComp.firstChildID;
		auto& otherChildComp = components[otherChildID];
		while( otherChildComp.nextID < capacity )
		{
			otherChildID = otherChildComp.nextID;
			otherChildComp = components[otherChildID];
		}

		otherChildComp.nextID = childID;
		childComp.prevID = otherChildID;
	}
	else
	{
		parentComp.firstChildID = childID;
	}

	if(childID != components.size() - 1)
		structureChanged = true;
	return true;
}

bool TransformSystem::Detach(Entity e)
{
	GET_COMPONENT(false)
	_detach(comp, (uint32_t)idx);
	return true;
}

bool TransformSystem::DetachChildren(Entity e)
{
	GET_COMPONENT(false)
	_detachChildren(comp);
	return true;
}

Entity TransformSystem::GetParent(Entity e)
{
	Entity res;
	res.setnull();
	GET_COMPONENT(res)
	if(comp.parentID >= capacity)
		return res;
	return components[comp.parentID].get_entity();
}

Entity TransformSystem::GetChildFirst(Entity e)
{
	Entity res;
	res.setnull();
	GET_COMPONENT(res)
	if(comp.firstChildID >= capacity)
		return res;
	return components[comp.firstChildID].get_entity();
}

Entity TransformSystem::GetChildNext(Entity e)
{
	Entity res;
	res.setnull();
	GET_COMPONENT(res)
	if(comp.nextID >= capacity)
		return res;
	return components[comp.nextID].get_entity();
}

uint32_t TransformSystem::Serialize(Entity e, uint8_t* data)
{
	GET_COMPONENT(0)

	uint8_t* t_data = data;
	uint32_t size = 0;

	for(uint32_t row = 0; row < 4; row++)
	{
		XMFLOAT4 row_data;
		XMStoreFloat4(&row_data, comp.localMatrix.r[row]);

		*(XMFLOAT4*)t_data = row_data;
		t_data += sizeof(XMFLOAT4);
		size += sizeof(XMFLOAT4);
	}

	if(comp.parentID >= capacity)
	{
		*(uint32_t*)t_data = 0;
		t_data += sizeof(uint32_t);
		size += sizeof(uint32_t);
	}
	else
	{
		auto& parentComp = components[comp.parentID];
		string name = world->GetNameMgr()->GetName(parentComp.get_entity());

		uint32_t name_size = (uint32_t)name.size();
		*(uint32_t*)t_data = name_size;
		t_data += sizeof(uint32_t);
		size += sizeof(uint32_t);

		if(name_size > 0)
		{
			memcpy_s(t_data, name_size, name.data(), name_size);
			t_data += name_size * sizeof(char);
			size += name_size * sizeof(char);
		}
	}

	return size;
}

uint32_t TransformSystem::Deserialize(Entity e, uint8_t* data)
{
	auto comp = AddComponent(e);
	if(!comp)
		return 0;
		
	uint8_t* t_data = data;
	uint32_t size = 0;

	for(uint32_t row = 0; row < 4; row++)
	{
		XMFLOAT4 row_data;
		row_data = *(XMFLOAT4*)t_data;
		t_data += sizeof(XMFLOAT4);
		size += sizeof(XMFLOAT4);

		comp->localMatrix.r[row] = XMLoadFloat4(&row_data);
	}

	uint32_t parentName_size = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);
	size += sizeof(uint32_t);

	if(parentName_size == 0)
		return size;
	
	string parentName((char*)t_data, parentName_size);
	t_data += parentName_size * sizeof(char);
	size += parentName_size * sizeof(char);

	if(!attachments_map)
		ERR("Attachments map uninitialized, need PreLoad call first!");
	else
		attachments_map->insert(make_pair(UintFromEntity(e), parentName));

	return size;
}

void TransformSystem::PreLoad()
{
	if(attachments_map)
		attachments_map->clear();
	else
		attachments_map = new unordered_map<uint32_t, string>;
	attachments_map->reserve(components.capacity()); // too much space?
}

bool TransformSystem::PostLoadParentsResolve()
{
	if(!attachments_map)
		return false;

	for(auto& it: *attachments_map)
	{
		Entity parent = world->GetNameMgr()->GetEntityByName(it.second);
		if(parent.isnull())
			ERR("Parent entity %s does not exist!", it.second.c_str());
		else
			Attach(EntityFromUint(it.first), parent);
	}

	attachments_map->clear();
	_DELETE(attachments_map);
	return true;
}

bool TransformSystem::SetPosition(Entity e, float x, float y, float z)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	comp.localMatrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslation(x, y, z);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetPosition(Entity e, XMVECTOR p)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	comp.localMatrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(p);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetRotation(Entity e, float p, float y, float r)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	comp.localMatrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationRollPitchYaw(p, y, r) * XMMatrixTranslationFromVector(pos);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetRotation(Entity e, XMVECTOR normalAxis, float angle)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	comp.localMatrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationNormal(normalAxis, angle) * XMMatrixTranslationFromVector(pos);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetRotation(Entity e, XMVECTOR quat)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	comp.localMatrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(quat) * XMMatrixTranslationFromVector(pos);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetScale(Entity e, float x, float y, float z)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	comp.localMatrix = XMMatrixScaling(x, y, z) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetScale(Entity e, XMVECTOR s)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	comp.localMatrix = XMMatrixScalingFromVector(s) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetTransform(Entity e, CXMMATRIX mat)
{
	GET_COMPONENT(false)
	comp.localMatrix = mat;

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddPosition(Entity e, float x, float y, float z)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	comp.localMatrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos) * XMMatrixTranslation(x, y, z);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddPosition(Entity e, XMVECTOR p)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	comp.localMatrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos) * XMMatrixTranslationFromVector(p);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddRotation(Entity e, float p, float y, float r)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	comp.localMatrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixRotationRollPitchYaw(p, y, r) * XMMatrixTranslationFromVector(pos);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddRotation(Entity e, XMVECTOR normalAxis, float angle)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	comp.localMatrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixRotationNormal(normalAxis, angle) * XMMatrixTranslationFromVector(pos);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddRotation(Entity e, XMVECTOR quat)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	comp.localMatrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixRotationQuaternion(quat) * XMMatrixTranslationFromVector(pos);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddScale(Entity e, float x, float y, float z)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	comp.localMatrix = XMMatrixScalingFromVector(scale) * XMMatrixScaling(x, y, z) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddScale(Entity e, XMVECTOR s)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	comp.localMatrix = XMMatrixScalingFromVector(scale) * XMMatrixScalingFromVector(s) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddTransform(Entity e, CXMMATRIX mat)
{
	GET_COMPONENT(false)
	comp.localMatrix *= mat;

	world->SetDirty(e);
	return true;
}

XMVECTOR TransformSystem::GetVectPositionL(Entity e)
{
	GET_COMPONENT(XMVECTOR())
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	return pos;
}

XMFLOAT3 TransformSystem::GetRotationL(Entity e)
{
	GET_COMPONENT(XMFLOAT3())
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	return PYRFromQuat(rot);
}

XMVECTOR TransformSystem::GetVectScaleL(Entity e)
{
	GET_COMPONENT(XMVECTOR())
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.localMatrix);
	return scale;
}

XMMATRIX TransformSystem::GetTransformL(Entity e)
{
	GET_COMPONENT(XMMatrixIdentity())
	return comp.localMatrix;
}

XMVECTOR TransformSystem::GetVectPositionW(Entity e)
{
	GET_COMPONENT(XMVECTOR())
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.worldMatrix);
	return pos;
}

XMFLOAT3 TransformSystem::GetRotationW(Entity e)
{
	GET_COMPONENT(XMFLOAT3())
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.worldMatrix);
	return PYRFromQuat(rot);
}

XMVECTOR TransformSystem::GetVectScaleW(Entity e)
{
	GET_COMPONENT(XMVECTOR())
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, comp.worldMatrix);
	return scale;
}

XMMATRIX TransformSystem::GetTransformW(Entity e)
{
	GET_COMPONENT(XMMatrixIdentity())
	return comp.worldMatrix;
}