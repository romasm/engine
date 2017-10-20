#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "VisibilitySystem.h"
#include "EarlyVisibilitySystem.h"
#include "MeshMgr.h"

namespace EngineCore
{
#define BONE_TRANSFORM_ACC_SIZE 1024

	struct AnimationSeq
	{
		uint32_t animationID;
		float currentTime;
		float blendFactor;
		float speed;
		bool looped;
		bool playing;

		AnimationSeq() : animationID(AnimationMgr::nullres), looped(false), playing(false),
			blendFactor(0.0f), currentTime(0), speed(1.0f) {}
	};

	struct SkeletonComponent
	{
		ENTITY_IN_COMPONENT

		bool dirty;
		bool animated;

		uint32_t skeletonID;
		DArray<uint32_t> bones;
		DArray<AnimationSeq> animations;

		StructBuf gpuMatrixBuffer;
		DArrayAligned<XMMATRIX> matrixBuffer;

		SkeletonComponent()
		{
			parent.setnull();
			dirty = true;
			animated = false;
			skeletonID = SkeletonMgr::nullres;
		}
	};

	class BaseWorld;

	class SkeletonSystem
	{
		friend BaseWorld;
	public:
		SkeletonSystem(BaseWorld* w, uint32_t maxCount);
		~SkeletonSystem();

		void SetStaticMeshSys(class StaticMeshSystem* sys);

		SkeletonComponent* AddComponent(Entity e);

		void CopyComponent(Entity src, Entity dest);
		void DeleteComponent(Entity e);

		bool HasComponent(Entity e) const {return components.has(e.index());}
		size_t ComponentsCount() {return components.dataSize();}

		SkeletonComponent* GetComponent(Entity e)
		{
			size_t idx = components.getArrayIdx(e.index());
			if(idx == components.capacity()) return nullptr;
			return &components.getDataByArrayIdx(idx);
		}
		
		uint32_t Serialize(Entity e, uint8_t* data);
		uint32_t Deserialize(Entity e, uint8_t* data);
		
		void Animate(float dt);
		void UpdateBuffers();

		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		bool SetSkeleton(Entity e, string mesh);
		bool SetSkeletonAndCallback(Entity e, string mesh, LuaRef func);
		
		bool SetAnimationSingle(Entity e, string anim);
		bool SetAnimationSingleAndCallback(Entity e, string anim, LuaRef func);

		int32_t AddAnimationBlended(Entity e, string anim);
		int32_t AddAnimationBlendedAndCallback(Entity e, string anim, LuaRef func);

		bool ClearAnimations(Entity e);
		int32_t GetAnimationsCount(Entity e);

		bool SetAnimationBlend(Entity e, int32_t animID, float blendWeight);
		bool SetAnimationSpeed(Entity e, int32_t animID, float speed);
		bool SetAnimationTime(Entity e, int32_t animID, float time);
		bool SetAnimationLooping(Entity e, int32_t animID, bool looped);
		bool SetAnimationPlaying(Entity e, int32_t animID, bool playing);

		float GetAnimationBlend(Entity e, int32_t animID);
		float GetAnimationSpeed(Entity e, int32_t animID);
		float GetAnimationTime(Entity e, int32_t animID);
		bool GetAnimationLooping(Entity e, int32_t animID);
		bool GetAnimationPlaying(Entity e, int32_t animID);
		float GetAnimationDuration(Entity e, int32_t animID);

		inline bool _AddComponent(Entity e)
		{
			if(AddComponent(e))	return true;
			else return false;
		}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<SkeletonSystem>("SkeletonSystem")
					.addFunction("AddComponent", &SkeletonSystem::_AddComponent)
					.addFunction("DeleteComponent", &SkeletonSystem::DeleteComponent)
					.addFunction("HasComponent", &SkeletonSystem::HasComponent)

					.addFunction("SetSkeleton", &SkeletonSystem::SetSkeleton)
					.addFunction("SetSkeletonAndCallback", &SkeletonSystem::SetSkeletonAndCallback)		

					.addFunction("SetAnimationSingle", &SkeletonSystem::SetAnimationSingle)
					.addFunction("SetAnimationSingleAndCallback", &SkeletonSystem::SetAnimationSingleAndCallback)
					.addFunction("AddAnimationBlended", &SkeletonSystem::AddAnimationBlended)
					.addFunction("AddAnimationBlendedAndCallback", &SkeletonSystem::AddAnimationBlendedAndCallback)
					.addFunction("ClearAnimations", &SkeletonSystem::ClearAnimations)
					.addFunction("GetAnimationsCount", &SkeletonSystem::GetAnimationsCount)

					.addFunction("SetAnimationBlend", &SkeletonSystem::SetAnimationBlend)
					.addFunction("SetAnimationSpeed", &SkeletonSystem::SetAnimationSpeed)
					.addFunction("SetAnimationTime", &SkeletonSystem::SetAnimationTime)
					.addFunction("SetAnimationLooping", &SkeletonSystem::SetAnimationLooping)
					.addFunction("SetAnimationPlaying", &SkeletonSystem::SetAnimationPlaying)
					.addFunction("GetAnimationBlend", &SkeletonSystem::GetAnimationBlend)
					.addFunction("GetAnimationSpeed", &SkeletonSystem::GetAnimationSpeed)
					.addFunction("GetAnimationTime", &SkeletonSystem::GetAnimationTime)
					.addFunction("GetAnimationLooping", &SkeletonSystem::GetAnimationLooping)
					.addFunction("GetAnimationPlaying", &SkeletonSystem::GetAnimationPlaying)
					.addFunction("GetAnimationDuration", &SkeletonSystem::GetAnimationDuration)
				.endClass();
		}

		bool updateSkeleton(SkeletonComponent& comp);

	private:
		bool _setSkeleton(SkeletonComponent* comp, string& skeleton, LuaRef func);
		inline void destroySkeleton(SkeletonComponent& comp)
		{
			for(int32_t i = (int32_t)comp.bones.size() - 1; i >= 0; i--)
				sceneGraph->DeleteNode(comp.bones[i]);
			SkeletonMgr::Get()->DeleteResource(comp.skeletonID);
			comp.skeletonID = SkeletonMgr::nullres;
			comp.bones.destroy();
			comp.matrixBuffer.destroy();
			comp.gpuMatrixBuffer.Release();
		};
		void setAnimationTransformations(SkeletonComponent& comp, AnimationData* animData, BoundingBox& bbox, float& totalBlend,
			float sampleKeyID, float blendFactor, int32_t keysCountMinusOne, bool looped);
		
		inline void _clearAnimations(SkeletonComponent& comp)
		{
			for(auto& it: comp.animations)
				AnimationMgr::Get()->DeleteResource(it.animationID);
			comp.animations.clear();
			comp.animated = false;
			_setIdle(comp);
		}
		int32_t _addAnimation(SkeletonComponent* comp, string& anim, LuaRef func);

		void _setIdle(SkeletonComponent& comp);

		ComponentRArray<SkeletonComponent> components;
		
		struct BoneAcc
		{
			XMMATRIX transform;
			Vector3 position;
			float totalBlendWeight;
		};
		DArrayAligned<BoneAcc> transformAcc;

		SceneGraph* sceneGraph;
		TransformSystem* transformSys;
		VisibilitySystem* visibilitySys;
		EarlyVisibilitySystem* earlyVisibilitySys;
		class StaticMeshSystem* staticMeshSys;
		BaseWorld* world;

		XMVECTOR zeroOrigin;
	};
}