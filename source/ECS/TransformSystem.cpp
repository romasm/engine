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

	*(uint32_t*)t_data = (uint32_t)comp.mobility;
	t_data += sizeof(uint32_t);
	size += sizeof(uint32_t);

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

	comp->mobility = Mobility(*(uint32_t*)t_data);
	t_data += sizeof(uint32_t);
	size += sizeof(uint32_t);

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
		attachments_map->insert(make_pair((uint32_t)e, parentName));

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
			Attach(it.first, parent);
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

bool TransformSystem::SetMobility(Entity e, uint32_t mbl)
{
	GET_COMPONENT(false)
	// TODO: switch scene graph for no editor
	comp.mobility = Mobility(mbl);
	return true;
}

uint32_t TransformSystem::GetMobility(Entity e)
{
	GET_COMPONENT(0)
	return (uint32_t)comp.mobility;
}

// Set Transform
#define SET_TRANSFORM_L(transform) \
	GET_COMPONENT(false)\
	XMVECTOR pos, rot, scale;\
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));\
	XMMATRIX matrix = transform;\
	sceneGraph->SetTransformation(comp.nodeID, matrix);\
	world->SetDirty(e);\
	return true;

bool TransformSystem::SetPosition_L3F(Entity e, float x, float y, float z)
{
	return SetPosition_L(e, Vector3(x, y, z));
}

bool TransformSystem::SetPosition_L(Entity e, Vector3& p)
{
	SET_TRANSFORM_L(XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(p));
}

bool TransformSystem::SetRotationPYR_L3F(Entity e, float p, float y, float r)
{
	return SetRotationPYR_L(e, Vector3(p, y, r));
}

bool TransformSystem::SetRotationPYR_L(Entity e, Vector3& r)
{
	SET_TRANSFORM_L(XMMatrixScalingFromVector(scale) * XMMatrixRotationRollPitchYawFromVector(r) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::SetRotation_L(Entity e, Quaternion& quat)
{
	SET_TRANSFORM_L(XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(quat) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::SetRotationAxis_L(Entity e, Vector3& normalAxis, float angle)
{
	SET_TRANSFORM_L(XMMatrixScalingFromVector(scale) * XMMatrixRotationNormal(normalAxis, angle) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::SetScale_L3F(Entity e, float x, float y, float z)
{
	return SetScale_L(e, Vector3(x, y, z));
}

bool TransformSystem::SetScale_L(Entity e, Vector3& s)
{
	SET_TRANSFORM_L(XMMatrixScalingFromVector(s) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::SetPosition_W3F(Entity e, float x, float y, float z)
{
	return SetPosition_W(e, Vector3(x, y, z));
}

#define SET_TRANSFORM_W(transform) \
	GET_COMPONENT(false)\
	XMVECTOR pos, rot, scale;\
	const XMMATRIX* worldMatrix = sceneGraph->GetWorldTransformation(comp.nodeID);\
	XMMatrixDecompose(&scale, &rot, &pos, *worldMatrix);\
	XMMATRIX matrix = transform;\
	auto parent = sceneGraph->GetParent(comp.nodeID);\
	if(parent != SCENEGRAPH_NULL_ID){\
		const XMMATRIX* parentMatrix = sceneGraph->GetWorldTransformation(parent);\
		XMMATRIX invParent = XMMatrixInverse(nullptr, *parentMatrix);\
		matrix = DirectX::XMMatrixMultiply( matrix, invParent );}\
	sceneGraph->SetTransformation(comp.nodeID, matrix);\
	world->SetDirty(e);\
	return true;

bool TransformSystem::SetPosition_W(Entity e, Vector3& p)
{
	SET_TRANSFORM_W(XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(p));
}

bool TransformSystem::SetRotationPYR_W3F(Entity e, float p, float y, float r)
{
	return SetRotationPYR_W(e, Vector3(p, y, r));
}

bool TransformSystem::SetRotationPYR_W(Entity e, Vector3& r)
{
	SET_TRANSFORM_W(XMMatrixScalingFromVector(scale) * XMMatrixRotationRollPitchYawFromVector(r) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::SetRotation_W(Entity e, Quaternion& quat)
{
	SET_TRANSFORM_W(XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(quat) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::SetRotationAxis_W(Entity e, Vector3& normalAxis, float angle)
{
	SET_TRANSFORM_W(XMMatrixScalingFromVector(scale) * XMMatrixRotationNormal(normalAxis, angle) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::SetScale_W3F(Entity e, float x, float y, float z)
{
	return SetScale_W(e, Vector3(x, y, z));
}

bool TransformSystem::SetScale_W(Entity e, Vector3& s)
{
	SET_TRANSFORM_W(XMMatrixScalingFromVector(s) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::SetTransform_LInternal(Entity e, XMMATRIX& mat)
{
	GET_COMPONENT(false)
	sceneGraph->SetTransformation(comp.nodeID, mat);
	world->SetDirty(e);
	return true;
}

bool TransformSystem::SetTransform_WInternal(Entity e, XMMATRIX& mat)
{
	GET_COMPONENT(false)
	auto parent = sceneGraph->GetParent(comp.nodeID);
	if(parent != SCENEGRAPH_NULL_ID)
	{
		const XMMATRIX* parentMatrix = sceneGraph->GetWorldTransformation(parent);
		XMMATRIX invParent = XMMatrixInverse(nullptr, *parentMatrix);
		sceneGraph->SetTransformation(comp.nodeID, DirectX::XMMatrixMultiply( mat, invParent ));
	}
	else
	{
		sceneGraph->SetTransformation(comp.nodeID, mat);
	}
	world->SetDirty(e);
	return true;
}

// Get Transform

Vector3 TransformSystem::GetPosition_L(Entity e)
{
	GET_COMPONENT(Vector3::Zero)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	return pos;
}

Quaternion TransformSystem::GetRotation_L(Entity e)
{
	GET_COMPONENT(Quaternion::Identity)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	return rot;
}

Vector3 TransformSystem::GetRotationPYR_L(Entity e)
{
	GET_COMPONENT(Vector3::Zero)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	return PYRFromQuat(rot);
}

Vector3 TransformSystem::GetForward_L(Entity e)
{
	GET_COMPONENT(Vector3())
	return XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(1.0f, 0, 0, 0), *sceneGraph->GetLocalTransformation(comp.nodeID)));
}

Vector3 TransformSystem::GetUpward_L(Entity e)
{
	GET_COMPONENT(Vector3())
		return XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0, 1.0f, 0, 0), *sceneGraph->GetLocalTransformation(comp.nodeID)));
}

Vector3 TransformSystem::GetRightward_L(Entity e)
{
	GET_COMPONENT(Vector3())
		return XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0, 0, 1.0f, 0), *sceneGraph->GetLocalTransformation(comp.nodeID)));
}

Vector3 TransformSystem::GetScale_L(Entity e)
{
	GET_COMPONENT(Vector3::Zero)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetLocalTransformation(comp.nodeID));
	return scale;
}

const XMMATRIX& TransformSystem::GetTransform_LInternal(Entity e)
{
	GET_COMPONENT(SceneGraph::XMMatrixIdentityConst)
	return *sceneGraph->GetLocalTransformation(comp.nodeID);
}

Vector3 TransformSystem::GetPosition_W(Entity e)
{
	GET_COMPONENT(Vector3::Zero)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetWorldTransformation(comp.nodeID));
	return pos;
}

Quaternion TransformSystem::GetRotation_W(Entity e)
{
	GET_COMPONENT(Quaternion::Identity)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetWorldTransformation(comp.nodeID));
	return rot;
}

Vector3 TransformSystem::GetRotationPYR_W(Entity e)
{
	GET_COMPONENT(Vector3::Zero)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetWorldTransformation(comp.nodeID));
	return PYRFromQuat(rot);
}

Vector3 TransformSystem::GetForward_W(Entity e)
{
	GET_COMPONENT(Vector3())
	return XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(1.0f, 0, 0, 0), *sceneGraph->GetWorldTransformation(comp.nodeID)));
}

Vector3 TransformSystem::GetUpward_W(Entity e)
{
	GET_COMPONENT(Vector3())
		return XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0, 1.0f, 0, 0), *sceneGraph->GetWorldTransformation(comp.nodeID)));
}

Vector3 TransformSystem::GetRightward_W(Entity e)
{
	GET_COMPONENT(Vector3())
		return XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0, 0, 1.0f, 0), *sceneGraph->GetWorldTransformation(comp.nodeID)));
}

Vector3 TransformSystem::GetScale_W(Entity e)
{
	GET_COMPONENT(Vector3::Zero)
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, *sceneGraph->GetWorldTransformation(comp.nodeID));
	return scale;
}

const XMMATRIX& TransformSystem::GetTransform_WInternal(Entity e)
{
	GET_COMPONENT(SceneGraph::XMMatrixIdentityConst)
	return *sceneGraph->GetWorldTransformation(comp.nodeID);
}

// Add Transform

bool TransformSystem::AddPosition_L3F(Entity e, float x, float y, float z)
{
	return AddPosition_L(e, Vector3(x, y, z));
}

bool TransformSystem::AddPosition_L(Entity e, Vector3& p)
{
	SET_TRANSFORM_L(XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos) * XMMatrixTranslationFromVector(p));
}

bool TransformSystem::AddRotationPYR_L3F(Entity e, float p, float y, float r)
{
	return AddRotationPYR_L(e, Vector3(p, y, r));
}

bool TransformSystem::AddRotationPYR_L(Entity e, Vector3& r)
{
	SET_TRANSFORM_L(XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixRotationRollPitchYawFromVector(r) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::AddRotationAxis_L(Entity e, Vector3& normalAxis, float angle)
{
	SET_TRANSFORM_L(XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixRotationNormal(normalAxis, angle) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::AddRotation_L(Entity e, Quaternion& quat)
{
	SET_TRANSFORM_L(XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixRotationQuaternion(quat) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::AddScale_L3F(Entity e, float x, float y, float z)
{
	return AddScale_L(e, Vector3(x, y, z));
}

bool TransformSystem::AddScale_L(Entity e, Vector3& s)
{
	SET_TRANSFORM_L(XMMatrixScalingFromVector(scale) * XMMatrixScalingFromVector(s) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::AddPosition_W3F(Entity e, float x, float y, float z)
{
	return AddPosition_W(e, Vector3(x, y, z));
}

bool TransformSystem::AddPosition_W(Entity e, Vector3& p)
{
	SET_TRANSFORM_W(XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos) * XMMatrixTranslationFromVector(p));
}

bool TransformSystem::AddRotationPYR_W3F(Entity e, float p, float y, float r)
{
	return AddRotationPYR_W(e, Vector3(p, y, r));
}

bool TransformSystem::AddRotationPYR_W(Entity e, Vector3& r)
{
	SET_TRANSFORM_W(XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixRotationRollPitchYawFromVector(r) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::AddRotationAxis_W(Entity e, Vector3& normalAxis, float angle)
{
	SET_TRANSFORM_W(XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixRotationNormal(normalAxis, angle) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::AddRotation_W(Entity e, Quaternion& quat)
{
	SET_TRANSFORM_W(XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rot) * XMMatrixRotationQuaternion(quat) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::AddScale_W3F(Entity e, float x, float y, float z)
{
	return AddScale_W(e, Vector3(x, y, z));
}

bool TransformSystem::AddScale_W(Entity e, Vector3& s)
{
	SET_TRANSFORM_W(XMMatrixScalingFromVector(scale) * XMMatrixScalingFromVector(s) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos));
}

bool TransformSystem::PostTransform_L(Entity e, Matrix& mat)
{
	GET_COMPONENT(false)
	const XMMATRIX* localMat = sceneGraph->GetLocalTransformation(comp.nodeID);
	sceneGraph->SetTransformation(comp.nodeID, DirectX::XMMatrixMultiply(*localMat, mat));
	world->SetDirty(e);
	return true;
}

bool TransformSystem::PostTransform_W(Entity e, Matrix& mat)
{
	GET_COMPONENT(false)

	const XMMATRIX* worldMat = sceneGraph->GetWorldTransformation(comp.nodeID);
	XMMATRIX matrix = DirectX::XMMatrixMultiply(*worldMat, mat);

	auto parent = sceneGraph->GetParent(comp.nodeID);
	if(parent != SCENEGRAPH_NULL_ID)
	{
		const XMMATRIX* parentMatrix = sceneGraph->GetWorldTransformation(parent);
		XMMATRIX invParent = XMMatrixInverse(nullptr, *parentMatrix);
		matrix = DirectX::XMMatrixMultiply( matrix, invParent );
	}

	sceneGraph->SetTransformation(comp.nodeID, matrix);
	world->SetDirty(e);
	return true;
}

bool TransformSystem::PreTransform_L(Entity e, Matrix& mat)
{
	GET_COMPONENT(false)
	const XMMATRIX* localMat = sceneGraph->GetLocalTransformation(comp.nodeID);
	sceneGraph->SetTransformation(comp.nodeID, DirectX::XMMatrixMultiply(mat, *localMat));
	world->SetDirty(e);
	return true;
}

bool TransformSystem::PreTransform_W(Entity e, Matrix& mat)
{
	GET_COMPONENT(false)

	const XMMATRIX* worldMat = sceneGraph->GetWorldTransformation(comp.nodeID);
	XMMATRIX matrix = DirectX::XMMatrixMultiply(mat, *worldMat);

	auto parent = sceneGraph->GetParent(comp.nodeID);
	if (parent != SCENEGRAPH_NULL_ID)
	{
		const XMMATRIX* parentMatrix = sceneGraph->GetWorldTransformation(parent);
		XMMATRIX invParent = XMMatrixInverse(nullptr, *parentMatrix);
		matrix = DirectX::XMMatrixMultiply(matrix, invParent);
	}

	sceneGraph->SetTransformation(comp.nodeID, matrix);
	world->SetDirty(e);
	return true;
}