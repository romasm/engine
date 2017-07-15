#include "stdafx.h"
#include "TransformSystem.h"
#include "World.h"

using namespace EngineCore;

TransformSystem::TransformSystem(BaseWorld* w, uint32_t maxCount)
{	
	world = w;
	sceneGraph = world->GetSceneGraph();
	
	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);

	attachments_map = nullptr;
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == components.capacity())	return res;\
	auto& comp = components.getDataByArrayIdx(idx);

bool TransformSystem::IsDirty(Entity e)
{
	GET_COMPONENT(false);
	return sceneGraph->IsDirty(comp.nodeID);
}

bool TransformSystem::SetDirty(Entity e)
{
	GET_COMPONENT(false);
	return sceneGraph->SetDirty(comp.nodeID);
}

bool TransformSystem::Attach(Entity child, Entity parent)
{
	size_t idx = components.getArrayIdx(child.index());
	if(idx == components.capacity())
		return false;
	auto& childComp = components.getDataByArrayIdx(idx);

	if(parent.isnull())
		return sceneGraph->Attach(childComp.nodeID, SCENEGRAPH_NULL_ID); 

	idx = components.getArrayIdx(parent.index());
	if(idx == components.capacity())
		return false;
	auto& parentComp = components.getDataByArrayIdx(idx);

	return sceneGraph->Attach(childComp.nodeID, parentComp.nodeID); 
}

bool TransformSystem::Detach(Entity e)
{
	GET_COMPONENT(false);
	return sceneGraph->Detach(comp.nodeID);
}

bool TransformSystem::DetachChildren(Entity e)
{
	GET_COMPONENT(false);
	return sceneGraph->DetachChildren(comp.nodeID);
}

Entity TransformSystem::GetParent(Entity e)
{
	Entity res;
	res.setnull();
	GET_COMPONENT(res);
	uint32_t nodeID = sceneGraph->GetParent(comp.nodeID);
	if(nodeID == SCENEGRAPH_NULL_ID)
		return res;
	return sceneGraph->GetEntityByNode(nodeID);
}

Entity TransformSystem::GetChildFirst(Entity e)
{
	Entity res;
	res.setnull();
	GET_COMPONENT(res);
	uint32_t nodeID = sceneGraph->GetChildFirst(comp.nodeID);
	if(nodeID == SCENEGRAPH_NULL_ID)
		return res;
	return sceneGraph->GetEntityByNode(nodeID);
}

Entity TransformSystem::GetChildNext(Entity e)
{
	Entity res;
	res.setnull();
	GET_COMPONENT(res);
	uint32_t nodeID = sceneGraph->GetChildNext(comp.nodeID);
	if(nodeID == SCENEGRAPH_NULL_ID)
		return res;
	return sceneGraph->GetEntityByNode(nodeID);
}

uint32_t TransformSystem::Serialize(Entity e, uint8_t* data)
{
	GET_COMPONENT(0)

	uint8_t* t_data = data;
	uint32_t size = 0;

	const XMMATRIX* localMatrix = sceneGraph->GetLocalTransformation(comp.nodeID);

	for(uint32_t row = 0; row < 4; row++)
	{
		Vector4 row_data;
		XMStoreFloat4(&row_data, localMatrix->r[row]);

		*(Vector4*)t_data = row_data;
		t_data += sizeof(Vector4);
		size += sizeof(Vector4);
	}

	uint32_t parentNode = sceneGraph->GetParent(comp.nodeID);

	if(parentNode == SCENEGRAPH_NULL_ID)
	{
		*(uint32_t*)t_data = 0;
		t_data += sizeof(uint32_t);
		size += sizeof(uint32_t);
	}
	else
	{
		Entity parentEntity = sceneGraph->GetEntityByNode(parentNode);
		string name = world->GetNameMgr()->GetName(parentEntity);

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

	XMMATRIX localMatrix;
	for(uint32_t row = 0; row < 4; row++)
	{
		Vector4 row_data;
		row_data = *(Vector4*)t_data;
		t_data += sizeof(Vector4);
		size += sizeof(Vector4);

		localMatrix.r[row] = XMLoadFloat4(&row_data);
	}

	sceneGraph->SetTransformation(comp->nodeID, localMatrix);

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

bool TransformSystem::SetPhysicsTransform(Entity e, XMMATRIX& transform)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));

	XMMATRIX matrix = XMMatrixScalingFromVector(scale) * transform;
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetPosition(Entity e, float x, float y, float z)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	XMMATRIX matrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslation(x, y, z);
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetPosition(Entity e, XMVECTOR p)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	XMMATRIX matrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(p);
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetRotation(Entity e, float p, float y, float r)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	XMMATRIX matrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationRollPitchYaw(p, y, r) * XMMatrixTranslationFromVector(pos);
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetRotation(Entity e, XMVECTOR normalAxis, float angle)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	XMMATRIX matrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationNormal(normalAxis, angle) * XMMatrixTranslationFromVector(pos);
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetRotation(Entity e, XMVECTOR quat)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	XMMATRIX matrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(quat) * XMMatrixTranslationFromVector(pos);
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetScale(Entity e, float x, float y, float z)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	XMMATRIX matrix = XMMatrixScaling(x, y, z) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetScale(Entity e, XMVECTOR s)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	XMMATRIX matrix = XMMatrixScalingFromVector(s) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetTransform(Entity e, CXMMATRIX mat)
{
	GET_COMPONENT(false)
	sceneGraph->SetTransformation(comp.nodeID, mat);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddPosition(Entity e, float x, float y, float z)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	XMMATRIX matrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos) * XMMatrixTranslation(x, y, z);
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddPosition(Entity e, XMVECTOR p)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	XMMATRIX matrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos) * XMMatrixTranslationFromVector(p);
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddPositionLocal(Entity e, float x, float y, float z)
{
	GET_COMPONENT(false)
	XMMATRIX matrix = XMMatrixTranslation(x, y, z) * (*sceneGraph->GetLocalTransformation(comp.nodeID));
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddRotation(Entity e, float p, float y, float r)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	XMMATRIX matrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixRotationRollPitchYaw(p, y, r) * XMMatrixTranslationFromVector(pos);
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddRotation(Entity e, XMVECTOR normalAxis, float angle)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	XMMATRIX matrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixRotationNormal(normalAxis, angle) * XMMatrixTranslationFromVector(pos);
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddRotation(Entity e, XMVECTOR quat)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	XMMATRIX matrix = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixRotationQuaternion(quat) * XMMatrixTranslationFromVector(pos);
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddScale(Entity e, float x, float y, float z)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	XMMATRIX matrix = XMMatrixScalingFromVector(scale) * XMMatrixScaling(x, y, z) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddScale(Entity e, XMVECTOR s)
{
	GET_COMPONENT(false)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	XMMATRIX matrix = XMMatrixScalingFromVector(scale) * XMMatrixScalingFromVector(s) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);
	sceneGraph->SetTransformation(comp.nodeID, matrix);

	world->SetDirty(e);
	return true;
}

bool TransformSystem::AddTransform(Entity e, CXMMATRIX mat)
{
	GET_COMPONENT(false)
	sceneGraph->SetTransformation(comp.nodeID, mat);

	world->SetDirty(e);
	return true;
}

XMVECTOR TransformSystem::GetVectPositionL(Entity e)
{
	GET_COMPONENT(XMVECTOR())
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	return pos;
}

Quaternion TransformSystem::GetQuatRotationL(Entity e)
{
	GET_COMPONENT(Quaternion())
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	return rot;
}

Vector3 TransformSystem::GetRotationL(Entity e)
{
	GET_COMPONENT(Vector3())
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	return PYRFromQuat(rot);
}

Vector3 TransformSystem::GetDirectionL(Entity e)
{
	GET_COMPONENT(Vector3())
	return XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(1.0f, 0, 0, 0), *sceneGraph->GetLocalTransformation(comp.nodeID)));
}

XMVECTOR TransformSystem::GetVectScaleL(Entity e)
{
	GET_COMPONENT(XMVECTOR())
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	return scale;
}

XMMATRIX TransformSystem::GetTransformL(Entity e)
{
	GET_COMPONENT(XMMatrixIdentity())
	return *sceneGraph->GetLocalTransformation(comp.nodeID);
}

XMVECTOR TransformSystem::GetVectPositionW(Entity e)
{
	GET_COMPONENT(XMVECTOR())
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetWorldTransformation(comp.nodeID));
	return pos;
}

Quaternion TransformSystem::GetQuatRotationW(Entity e)
{
	GET_COMPONENT(Quaternion())
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetWorldTransformation(comp.nodeID));
	return rot;
}

Vector3 TransformSystem::GetRotationW(Entity e)
{
	GET_COMPONENT(Vector3())
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetWorldTransformation(comp.nodeID));
	return PYRFromQuat(rot);
}

Vector3 TransformSystem::GetDirectionW(Entity e)
{
	GET_COMPONENT(Vector3())
	return XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(1.0f, 0, 0, 0), *sceneGraph->GetWorldTransformation(comp.nodeID)));
}

XMVECTOR TransformSystem::GetVectScaleW(Entity e)
{
	GET_COMPONENT(XMVECTOR())
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetWorldTransformation(comp.nodeID));
	return scale;
}

XMMATRIX TransformSystem::GetTransformW(Entity e)
{
	GET_COMPONENT(XMMatrixIdentity())
	return *sceneGraph->GetWorldTransformation(comp.nodeID);
}