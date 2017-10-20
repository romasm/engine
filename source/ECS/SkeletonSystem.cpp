#include "stdafx.h"
#include "SkeletonSystem.h"
#include "StaticMeshSystem.h"
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
	staticMeshSys = nullptr;

	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);

	transformAcc.reserve(BONE_TRANSFORM_ACC_SIZE);
	zeroOrigin = XMVectorSet(0,0,0,1.0f);
}

void SkeletonSystem::SetStaticMeshSys(StaticMeshSystem* sys)
{
	staticMeshSys = sys;
}

SkeletonSystem::~SkeletonSystem()
{
	for(auto& i: *components.data())
		destroySkeleton(i);
}

void SkeletonSystem::Animate(float dt)
{
	BoundingBox bbox;
	float totalBlend;
	
	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		if(!i.animated)
			continue;

		transformAcc.resize(0);
		transformAcc.reserve(i.bones.size());

		bbox.Center = Vector3::Zero;
		bbox.Extents = Vector3::Zero;
		totalBlend = 0;

		for(auto& animSeq: i.animations)
		{
			if( !animSeq.playing || animSeq.blendFactor == 0 )
				continue;

			if( animSeq.speed != 0 || i.dirty == true )
			{
				auto animPtr = AnimationMgr::GetResourcePtr(animSeq.animationID);
				if(!animPtr)
					continue;

				if( animPtr->duration > 0 )
				{
					if(animSeq.looped)
					{
						while( animSeq.currentTime > animPtr->duration )
							animSeq.currentTime -= animPtr->duration;
					}
					else
					{
						if( animSeq.currentTime > animPtr->duration )
						{
							animSeq.playing = false;
							animSeq.currentTime = animPtr->duration;
						}
					}

					const float sampleKeyID = (animSeq.currentTime / animPtr->duration) * animPtr->keysCountMinusOne;
					setAnimationTransformations(i, animPtr, bbox, totalBlend, sampleKeyID, animSeq.blendFactor, animPtr->keysCountMinusOne, animSeq.looped);

					animSeq.currentTime += dt * animSeq.speed;	
				}
				else
				{
					setAnimationTransformations(i, animPtr, bbox, totalBlend, 0, animSeq.blendFactor, animPtr->keysCountMinusOne, false);
					animSeq.playing = false;
				}

				i.dirty = true;
			}
		}	
				
		if(!transformAcc.empty())
		{
			if( transformAcc.size() != i.bones.size() )
				WRN("Wrong transformations for skeleton %s during animation", SkeletonMgr::GetName(i.skeletonID).data());

			auto skeletonPtr = SkeletonMgr::GetResourcePtr(i.skeletonID);

			for(uint32_t j = 0; j < (uint32_t)transformAcc.size(); j++)
			{
				XMMATRIX boneFinalTransform;
				if( transformAcc[j].totalBlendWeight != 0 )
				{
					const BoneTransformation& bTrfm = transformAcc[j].transform;
					boneFinalTransform = XMMatrixAffineTransformation(bTrfm.scale, zeroOrigin, bTrfm.rotation, bTrfm.translation);
				}
				else
				{
					boneFinalTransform = skeletonPtr->bData[j].localTransform;
				}

				sceneGraph->SetTransformation(i.bones[j], boneFinalTransform);
			}

			float maxVertexOffset = MeshMgr::GetResourcePtr(staticMeshSys->GetMeshID(i.get_entity()))->maxVertexOffset;

			if( totalBlend != 0 && totalBlend != 1.0f )
			{
				bbox.Center.x /= totalBlend;
				bbox.Center.y /= totalBlend;
				bbox.Center.z /= totalBlend;
			}

			bbox.Extents.x += maxVertexOffset;
			bbox.Extents.y += maxVertexOffset;
			bbox.Extents.z += maxVertexOffset;

			visibilitySys->SetBBox(i.get_entity(), bbox);
		}
	}
}

void SkeletonSystem::setAnimationTransformations(SkeletonComponent& comp, AnimationData* animData, BoundingBox& bbox, float& totalBlend,
												 float sampleKeyID, float blendFactor, int32_t keysCountMinusOne, bool looped)
{
	const int32_t prevKey = int32_t(sampleKeyID);
	int32_t nextKey = prevKey + 1;
	if( nextKey > keysCountMinusOne && looped )
		nextKey = 0;
	const float lerpFactor = sampleKeyID - (float)prevKey;

	// TODO: Bbox realtime calculation
	Vector3 finalCenter;
	Vector3 finalExtents;
	if( nextKey > keysCountMinusOne )
	{
		MeshBBox& box = animData->bboxes[prevKey];
		finalCenter = box.center;
		finalExtents = box.extents;
	}
	else
	{
		MeshBBox& box1 = animData->bboxes[prevKey];
		MeshBBox& box2 = animData->bboxes[nextKey];
		finalCenter = Vector3::Lerp(box1.center, box2.center, lerpFactor);
		finalExtents = Vector3::Lerp(box1.extents, box2.extents, lerpFactor);
	}

	finalCenter *= blendFactor;

	bbox.Center.x += finalCenter.x;
	bbox.Center.y += finalCenter.y;
	bbox.Center.z += finalCenter.z;
	bbox.Extents.x = max(bbox.Extents.x, finalExtents.x);
	bbox.Extents.y = max(bbox.Extents.y, finalExtents.y);
	bbox.Extents.z = max(bbox.Extents.z, finalExtents.z);
	totalBlend += blendFactor;
	// Bbox calc

	const uint32_t animBonesSize = (uint32_t)animData->bones.size();

	for(uint32_t i = 0; i < (uint32_t)comp.bones.size(); i++)
	{
		if( i >= animBonesSize )
			break;

		BoneAcc* acc = nullptr;
		if( i >= transformAcc.size() )
		{
			acc = &transformAcc.push_back();
			acc->totalBlendWeight = 0;
			acc->transform.SetZero();
		}
		else
		{
			acc = &transformAcc[i];
		}

		auto& keys = animData->bones[i].keys;
		if(keys.size() == 0)
			continue;
		
		BoneTransformation finalTransform;
		if( nextKey > keysCountMinusOne )
		{
			finalTransform = keys[prevKey];
		}
		else
		{
			BoneTransformation& prevTransform = keys[prevKey];
			BoneTransformation& nextTransform = keys[nextKey];
			BoneTransformation::Lerp(prevTransform, nextTransform, lerpFactor, finalTransform);
		}

		if( acc->totalBlendWeight == 0 )
		{
			acc->transform = finalTransform;
		}
		else
		{
			const float lerpFactor = blendFactor / ( acc->totalBlendWeight + blendFactor );
			BoneTransformation::Lerp(acc->transform, finalTransform, lerpFactor, acc->transform);
		}
		acc->totalBlendWeight += blendFactor;
	}
}

void SkeletonSystem::_setIdle(SkeletonComponent& comp)
{
	if(comp.animated)
		return;

	auto skeletonPtr = SkeletonMgr::GetResourcePtr(comp.skeletonID);

	for(uint32_t j = 0; j < (uint32_t)comp.bones.size(); j++)
		sceneGraph->SetTransformation(comp.bones[j], skeletonPtr->bData[j].localTransform);

	Entity ent = comp.get_entity();
	visibilitySys->SetBBox( ent, MeshMgr::GetResourcePtr(staticMeshSys->GetMeshID(ent))->box );
}

void SkeletonSystem::UpdateBuffers()
{
	for(auto& i: *components.data())
	{
		if( i.dirty || false/* animation? */ )
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
		//sceneGraph->SetTransformation(nodeID, bone.localTransform); ???
	}

	auto matrixCount = boneCount * 2;

	comp.matrixBuffer.reserve(matrixCount);
	comp.matrixBuffer.resize(matrixCount);
	comp.gpuMatrixBuffer = Buffer::CreateStructedBuffer(Render::Device(), matrixCount, sizeof(XMMATRIX), true);

	comp.dirty = true;
	_setIdle(comp);

	return true;
}

SkeletonComponent* SkeletonSystem::AddComponent(Entity e)
{
	SkeletonComponent* res = components.add(e.index());
	res->parent = e;

	updateSkeleton(*res);
	_setIdle(*res);
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
		_setSkeleton(comp, skeleton_name, LuaRef(LSTATE));

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
	return _setSkeleton(&comp, mesh, LuaRef(LSTATE));
}

bool SkeletonSystem::SetSkeletonAndCallback(Entity e, string mesh, LuaRef func)
{
	GET_COMPONENT(false)
	return _setSkeleton(&comp, mesh, func);
}

bool SkeletonSystem::_setSkeleton(SkeletonComponent* comp, string& skeleton, LuaRef func)
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
		if( !skeletonPtr )
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

		if( id != comp->skeletonID )
			comp->skeletonID = id;

		skeletonSys->updateSkeleton(*comp);

		if(luaRef->isFunction())
			LUA_CALL((*luaRef)(worldPtr, comp->get_entity(), id, status),);

		_DELETE((LuaRef*)luaRef);
	});

	MeshMgr::Get()->DeleteResource(oldSkeleton);
	return true;
}

int32_t SkeletonSystem::_addAnimation(SkeletonComponent* comp, string& anim, LuaRef func)
{
	auto worldID = world->GetID();
	auto ent = comp->get_entity();

	// TODO: potential memory leak if callback never will be called
	// This fixes wrong LuaRef capture by lambda
	LuaRef* luaRef = new LuaRef(func);

	comp->animations.push_back(AnimationSeq());
	int32_t animID = (int32_t)comp->animations.size() - 1;
	AnimationSeq& animSeq = comp->animations[animID];

	animSeq.animationID = AnimationMgr::Get()->GetResource( anim, CONFIG(bool, reload_resources), 

		[ent, worldID, animID, luaRef](uint32_t id, bool status) -> void
	{
		auto animPtr = AnimationMgr::GetResourcePtr(id);
		if(!animPtr)
		{
			_DELETE((LuaRef*)luaRef);
			return;
		}

		auto worldPtr = WorldMgr::Get()->GetWorld(worldID);
		if(!worldPtr || !worldPtr->IsEntityAlive(ent))
		{
			AnimationMgr::Get()->DeleteResource(id);
			_DELETE((LuaRef*)luaRef);
			return;
		}

		auto skeletonSys = worldPtr->GetSkeletonSystem();
		auto comp = skeletonSys->GetComponent(ent);
		if(!comp)
		{
			AnimationMgr::Get()->DeleteResource(id);
			_DELETE((LuaRef*)luaRef);
			return;
		}

		if( animID >= comp->animations.size() )
		{
			WRN("Cant set single animation %s because animations of component were modified");
			AnimationMgr::Get()->DeleteResource(id);
			_DELETE((LuaRef*)luaRef);
			return;
		}

		comp->dirty = true;
		comp->animated = true;

		if(luaRef->isFunction())
			LUA_CALL((*luaRef)(worldPtr, comp->get_entity(), id, status),);

		_DELETE((LuaRef*)luaRef);
	});

	return animID;
}

bool SkeletonSystem::SetAnimationSingle(Entity e, string anim)
{
	GET_COMPONENT(false)
	_clearAnimations(comp);
	_addAnimation(&comp, anim, LuaRef(LSTATE));
	comp.animations[0].blendFactor = 1.0f;
	return true;
}

bool SkeletonSystem::SetAnimationSingleAndCallback(Entity e, string anim, LuaRef func)
{
	GET_COMPONENT(false)
	_clearAnimations(comp);
	_addAnimation(&comp, anim, func);
	comp.animations[0].blendFactor = 1.0f;
	return true;
}

int32_t SkeletonSystem::AddAnimationBlended(Entity e, string anim)
{
	GET_COMPONENT(false)
	return _addAnimation(&comp, anim, LuaRef(LSTATE));
}

int32_t SkeletonSystem::AddAnimationBlendedAndCallback(Entity e, string anim, LuaRef func)
{
	GET_COMPONENT(false)
	return _addAnimation(&comp, anim, func);
}

bool SkeletonSystem::ClearAnimations(Entity e)
{
	GET_COMPONENT(false)
	_clearAnimations(comp);
	_setIdle(comp);
	comp.dirty = true;
	return true;
}

int32_t SkeletonSystem::GetAnimationsCount(Entity e)
{
	GET_COMPONENT(0)
	return (int32_t)comp.animations.size();
}

bool SkeletonSystem::SetAnimationBlend(Entity e, int32_t animID, float blendWeight)
{
	GET_COMPONENT(false)
	if( animID >= comp.animations.size() )
		return false;
	comp.animations[animID].blendFactor = blendWeight;
/*
#ifdef _DEV
	float totalWeight = 0;
	for(auto& it: comp.animations)
		totalWeight += it.blendFactor;
	if( abs(totalWeight - 1.0f) > 0.001f )
		WRN("Animations blending weights != 1.0");
#endif*/
	return true;
}

bool SkeletonSystem::SetAnimationSpeed(Entity e, int32_t animID, float speed)
{
	GET_COMPONENT(false)
	if( animID >= comp.animations.size() )
		return false;
	comp.animations[animID].speed = speed;
	return true;
}

bool SkeletonSystem::SetAnimationTime(Entity e, int32_t animID, float time)
{
	GET_COMPONENT(false)
	if( animID >= comp.animations.size() )
		return false;
	comp.animations[animID].currentTime = time;
	comp.dirty = true;
	return true;
}

bool SkeletonSystem::SetAnimationLooping(Entity e, int32_t animID, bool looped)
{
	GET_COMPONENT(false)
	if( animID >= comp.animations.size() )
		return false;
	comp.animations[animID].looped = looped;
	return true;
}

bool SkeletonSystem::SetAnimationPlaying(Entity e, int32_t animID, bool playing)
{
	GET_COMPONENT(false)
	if( animID >= comp.animations.size() )
		return false;
	comp.animations[animID].playing = playing;
	return true;
}

float SkeletonSystem::GetAnimationBlend(Entity e, int32_t animID)
{
	GET_COMPONENT(-1.0f)
	if( animID >= comp.animations.size() )
		return -1.0f;
	return comp.animations[animID].blendFactor;
}

float SkeletonSystem::GetAnimationSpeed(Entity e, int32_t animID)
{
	GET_COMPONENT(0)
	if( animID >= comp.animations.size() )
		return 0;
	return comp.animations[animID].speed;
}

float SkeletonSystem::GetAnimationTime(Entity e, int32_t animID)
{
	GET_COMPONENT(-1.0f)
	if( animID >= comp.animations.size() )
		return -1.0f;
	return comp.animations[animID].currentTime;
}

bool SkeletonSystem::GetAnimationLooping(Entity e, int32_t animID)
{
	GET_COMPONENT(false)
	if( animID >= comp.animations.size() )
		return false;
	return comp.animations[animID].looped;
}

bool SkeletonSystem::GetAnimationPlaying(Entity e, int32_t animID)
{
	GET_COMPONENT(false)
	if( animID >= comp.animations.size() )
		return false;
	return comp.animations[animID].playing;
}

float SkeletonSystem::GetAnimationDuration(Entity e, int32_t animID)
{
	GET_COMPONENT(0)
	if( animID >= comp.animations.size() )
		return false;
	return AnimationMgr::GetResourcePtr(comp.animations[animID].animationID)->duration;
}