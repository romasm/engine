#include "stdafx.h"
#include "SkeletonSystem.h"
#include "World.h"
#include "WorldMgr.h"

using namespace EngineCore;

SkeletonSystem::SkeletonSystem(BaseWorld* w, uint32_t maxCount)
{
	world = w;

	sceneGraph = world->GetSceneGraph();
	transformSys = w->GetTransformSystem();
	visibilitySys = w->GetVisibilitySystem();
	earlyVisibilitySys = w->GetEarlyVisibilitySystem();

	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);

	zeroOrigin = XMVectorSet(0,0,0,1.0f);
}

SkeletonSystem::~SkeletonSystem()
{
	for(auto& i: *components.data())
		destroySkeleton(i);
}

void SkeletonSystem::Animate(float dt)
{
	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		// VISIBILITY?

		if(i.animations.empty())
			continue;

		// TEMP
		AnimationSeq& animSeq = i.animations[0];
		if(animSeq.blendFactor == 0)
			continue;

		auto animPtr = AnimationMgr::GetResourcePtr(animSeq.animationID);
		if(!animPtr)
			continue;

		const int32_t keysCount = animPtr->keysCount - 1;
		const float sampleKeyID = (animSeq.currentTime / animPtr->duration) * keysCount;

		setAnimationTransformations(i, animPtr, sampleKeyID, animSeq.blendFactor, keysCount);

		animSeq.currentTime += dt;

		// TEMP looped
		while( animSeq.currentTime >= animPtr->duration )
			animSeq.currentTime -= animPtr->duration;

		i.dirty = true;
	}
}

void SkeletonSystem::setAnimationTransformations(SkeletonComponent& comp, AnimationData* animData, float sampleKeyID, float blendFactor, int32_t keysCount)
{
	const int32_t prevKey = int32_t(sampleKeyID);
	const int32_t nextKey = prevKey + 1;
	const float lerpFactor = sampleKeyID - (float)prevKey;

	const uint32_t animBonesSize = (uint32_t)animData->bones.size();

	for(uint32_t i = 0; i < (uint32_t)comp.bones.size(); i++)
	{
		if( i >= animBonesSize )
			break;

		auto& keys = animData->bones[i].keys;
		if(keys.size() == 0)
			continue;
		
		XMMATRIX finalMatrix;
		if( nextKey > keysCount )
		{
			BoneTransformation& transform = keys[prevKey];
			finalMatrix = XMMatrixAffineTransformation(transform.scale, zeroOrigin, transform.rotation, transform.translation);
		}
		else
		{
			BoneTransformation& prevTransform = keys[prevKey];
			BoneTransformation& nextTransform = keys[nextKey];

			XMVECTOR scale = XMVectorLerp(prevTransform.scale, nextTransform.scale, lerpFactor);
			XMVECTOR rot = XMQuaternionSlerp(prevTransform.rotation, nextTransform.rotation, lerpFactor);
			XMVECTOR pos = XMVectorLerp(prevTransform.translation, nextTransform.translation, lerpFactor);
			
			finalMatrix = XMMatrixAffineTransformation(scale, zeroOrigin, rot, pos);
		}
		
		sceneGraph->SetTransformation(comp.bones[i], finalMatrix);
	}
}

void SkeletonSystem::UpdateBuffers()
{
	for(auto& i: *components.data())
	{
		if( i.dirty || false/* animation? */ )
		{
			// precashe visibility in Animate
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

			if( bits == 0 )
				continue;

			for( uint32_t j = 0; j < (uint32_t)i.bones.size(); j++)
			{
				const XMMATRIX* worldMatrix = sceneGraph->GetWorldTransformation(i.bones[j]);

				XMVECTOR scale, pos, rot;
				XMMatrixDecompose(&scale, &rot, &pos, *worldMatrix);
				XMMATRIX rotM = XMMatrixRotationQuaternion(rot);
				XMMATRIX scaleM = XMMatrixScalingFromVector(scale);

				XMMATRIX normalMatrix = XMMatrixInverse(nullptr, scaleM);
				normalMatrix = normalMatrix * rotM;

				const auto matrixID = j * 2;
				i.matrixBuffer[matrixID] = XMMatrixTranspose(*worldMatrix);
				i.matrixBuffer[matrixID + 1] = XMMatrixTranspose(normalMatrix);
			}

			Render::UpdateDynamicResource(i.gpuMatrixBuffer.buf, (void*)i.matrixBuffer.data(), sizeof(XMMATRIX) * i.matrixBuffer.size());

			i.dirty = false;
		}
	}
}

bool SkeletonSystem::updateSkeleton(SkeletonComponent& comp)
{
	auto entity = comp.get_entity();
	auto transformComp = transformSys->GetComponent(entity);
	if(!transformComp)
	{
		ERR("Skeletal component needs trasform component first");
		return false;
	}

	auto skeletonPtr = SkeletonMgr::GetResourcePtr(comp.skeletonID);
	if(!skeletonPtr)
		return false;
	
	// free old
	for(int32_t i = (int32_t)comp.bones.size() - 1; i >= 0; i--)
		sceneGraph->DeleteNode(comp.bones[i]);
	comp.bones.destroy();
	comp.matrixBuffer.destroy();
	comp.gpuMatrixBuffer.Release();

	// init new
	int32_t boneCount = (int32_t)skeletonPtr->bData.size();
	comp.bones.reserve(boneCount);
	
	for(int32_t i = 0; i < boneCount; i++)
	{
		auto& bone = skeletonPtr->bData[i];
		uint32_t nodeID = sceneGraph->AddNode(entity);
		comp.bones.push_back(nodeID);

		uint32_t parentNode;
		if(bone.parent < 0)
		{
			parentNode = transformComp->nodeID;
		}
		else
		{
			if( bone.parent >= comp.bones.size() )
			{
				ERR("Skeleton bones needs to be presorted in hierarchy order for %s", SkeletonMgr::GetName(comp.skeletonID));
				parentNode = transformComp->nodeID;
			}
			else
			{
				parentNode = comp.bones[bone.parent];					
			}
		}

		sceneGraph->Attach(nodeID, parentNode);
		sceneGraph->SetTransformation(nodeID, bone.localTransform);
	}

	auto matrixCount = boneCount * 2;

	comp.matrixBuffer.reserve(matrixCount);
	comp.matrixBuffer.resize(matrixCount);
	comp.gpuMatrixBuffer = Buffer::CreateStructedBuffer(Render::Device(), matrixCount, sizeof(XMMATRIX), true);

	comp.dirty = true;

	return true;
}

SkeletonComponent* SkeletonSystem::AddComponent(Entity e)
{
	SkeletonComponent* res = components.add(e.index());
	res->parent = e;

	updateSkeleton(*res);

	return res;
}

void SkeletonSystem::CopyComponent(Entity src, Entity dest)
{
	auto copyBuffer = world->GetCopyBuffer();

	if( !Serialize(src, copyBuffer) )
		return;

	Deserialize(dest, copyBuffer);
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == components.capacity())	return res;\
	auto& comp = components.getDataByArrayIdx(idx);

bool SkeletonSystem::IsDirty(Entity e)
{
	GET_COMPONENT(false)
	return comp.dirty;
}

bool SkeletonSystem::SetDirty(Entity e)
{
	GET_COMPONENT(false)
	comp.dirty = true;
	return true;
}

uint32_t SkeletonSystem::Serialize(Entity e, uint8_t* data)
{
	GET_COMPONENT(0)

	auto t_data = data;
	uint32_t size = 0;

	uint32_t* size_slot = (uint32_t*)t_data;
	t_data += sizeof(uint32_t);
	size += sizeof(uint32_t);
	
	string skeleton_name = SkeletonMgr::GetName(comp.skeletonID);
	uint32_t skeleton_name_size = (uint32_t)skeleton_name.size();

	*(uint32_t*)t_data = skeleton_name_size;
	t_data += sizeof(uint32_t);
	size += sizeof(uint32_t);

	memcpy_s(t_data, skeleton_name_size, skeleton_name.data(), skeleton_name_size);
	t_data += skeleton_name_size * sizeof(char);
	size += skeleton_name_size * sizeof(char);
	
	*size_slot = size - sizeof(uint32_t);

	return size;
}

uint32_t SkeletonSystem::Deserialize(Entity e, uint8_t* data)
{
	auto t_data = data;
	uint32_t size = 0;

	size = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);
	
	uint32_t skeleton_name_size = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);
	string skeleton_name((char*)t_data, skeleton_name_size);
	t_data += skeleton_name_size * sizeof(char);

	auto comp = AddComponent(e);
	if(comp)
		setSkeleton(comp, skeleton_name, LuaRef(LSTATE));

	return size + sizeof(uint32_t);
}

void SkeletonSystem::DeleteComponent(Entity e)
{
	GET_COMPONENT(void())
	destroySkeleton(comp);
	components.remove(e.index());
}

bool SkeletonSystem::SetSkeleton(Entity e, string mesh)
{
	GET_COMPONENT(false)

	return setSkeleton(&comp, mesh, LuaRef(LSTATE));
}

bool SkeletonSystem::SetSkeletonAndCallback(Entity e, string mesh, LuaRef func)
{
	GET_COMPONENT(false)

	LuaRef nullLuaPtr(LSTATE);
	return setSkeleton(&comp, mesh, func);
}

bool SkeletonSystem::setSkeleton(SkeletonComponent* comp, string& skeleton, LuaRef func)
{
	auto oldSkeleton = comp->skeletonID;

	auto worldID = world->GetID();
	auto ent = comp->get_entity();

	// TODO: potential memory leak if callback never will be called
	// This fixes wrong LuaRef capture by lambda
	LuaRef* luaRef = new LuaRef(func);

	comp->skeletonID = SkeletonMgr::Get()->GetResource( skeleton, CONFIG(bool, reload_resources), 

		[ent, worldID, luaRef](uint32_t id, bool status) -> void
	{
		auto skeletonPtr = SkeletonMgr::GetResourcePtr(id);
		if(!skeletonPtr)
		{
			_DELETE((LuaRef*)luaRef);
			return;
		}

		auto worldPtr = WorldMgr::Get()->GetWorld(worldID);
		if(!worldPtr || !worldPtr->IsEntityAlive(ent))
		{
			SkeletonMgr::Get()->DeleteResource(id);
			_DELETE((LuaRef*)luaRef);
			return;
		}

		auto skeletonSys = worldPtr->GetSkeletonSystem();
		auto comp = skeletonSys->GetComponent(ent);
		if(!comp)
		{
			SkeletonMgr::Get()->DeleteResource(id);
			_DELETE((LuaRef*)luaRef);
			return;
		}

		skeletonSys->updateSkeleton(*comp);

		if(luaRef->isFunction())
			LUA_CALL((*luaRef)(worldPtr, comp->get_entity(), id, status),);

		_DELETE((LuaRef*)luaRef);
	});

	MeshMgr::Get()->DeleteResource(oldSkeleton);
	return true;
}

// TEMP
bool SkeletonSystem::SetAnimation(Entity e, string anim)
{
	GET_COMPONENT(false)
	
	auto worldID = world->GetID();
	auto ent = comp.get_entity();
	
	AnimationSeq& animSeq = comp.animations.push_back();
	animSeq.currentTime = 0;
	animSeq.blendFactor = 0;

	animSeq.animationID = AnimationMgr::Get()->GetResource( anim, CONFIG(bool, reload_resources), 

		[ent, worldID](uint32_t id, bool status) -> void
	{
		auto animPtr = AnimationMgr::GetResourcePtr(id);
		if(!animPtr)
		{
			return;
		}

		auto worldPtr = WorldMgr::Get()->GetWorld(worldID);
		if(!worldPtr || !worldPtr->IsEntityAlive(ent))
		{
			AnimationMgr::Get()->DeleteResource(id);
			return;
		}

		auto skeletonSys = worldPtr->GetSkeletonSystem();
		auto comp = skeletonSys->GetComponent(ent);
		if(!comp)
		{
			AnimationMgr::Get()->DeleteResource(id);
			return;
		}

		// TODO
		comp->animations[comp->animations.size() - 1].blendFactor = 1.0f;
		comp->dirty = true;
	});

	return true;
}