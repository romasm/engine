#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "EarlyVisibilitySystem.h"
#include "RenderMgrs.h"

#define ENVPROBS_INIT_COUNT 1 // only dist?

#define ENVPROBS_MAT PATH_SHADERS "offline/envmap_render"
#define ENVPROBS_MIPS_MAT PATH_SHADERS "offline/envmap_mipgen"
#define ENVPROBS_DIFF_MAT PATH_SHADERS "offline/envmap_diffgen"

#define ENVPROBS_DIST_RES 512
#define ENVPROBS_RES 256
#define ENVPROBS_DIFFUSE_DIV 8

#define ENVPROBS_SPEC_MIN 2
#define ENVPROBS_SPEC_MIPS_OFFSET 0

#define ENVPROBS_POSTFIX_S "_s" EXT_TEXTURE
#define ENVPROBS_POSTFIX_D "_d" EXT_TEXTURE

#define TEX_HAMMERSLEY PATH_SYS_TEXTURES "hammersley" EXT_TEXTURE

namespace EngineCore
{
	enum EnvParallaxType
	{
		EP_PARALLAX_SPHERE = 0,
		EP_PARALLAX_BOX = 1,
		EP_PARALLAX_NONE = 2
	};

	struct EnvProbComponent
	{
		ENTITY_IN_COMPONENT
			
		bool dirty;

		// static
		string eptex_name;

		// MEMORY LEAK - fixed?
		uint32_t specCube;	// TODO TextureCubeArray direct load, store ID 
		uint32_t diffCube;

		bool is_distant;

		// update on props change
		EnvParallaxType type;
		float fade;		
		uint32_t mips_count; // mipsNum - 1
		Vector3 offset;

		float near_clip, far_clip;
		int resolution;

		// update on dirty
		XMMATRIX invTransform;
		float distance;
		Vector3 pos;
		
		ALIGNED_ALLOCATION
	};

	class BaseWorld;

	class EnvProbSystem
	{
	public:
		EnvProbSystem(BaseWorld* wrd, uint32_t maxCount);
		~EnvProbSystem()
		{
			for(auto& i: *components.data())
			{
				TEXTURE_DROP(i.specCube);
				TEXTURE_DROP(i.diffCube);
			}
		}
		
		void AddComponent(Entity e, bool distant)
		{
			EnvProbComponent* comp = components.add(e.index());
			comp->parent = e;
			comp->dirty = true;
			comp->is_distant = distant;
			comp->far_clip = 100000.0f;
			comp->near_clip = 1.0f;
			comp->offset = Vector3(0,0,0);
			comp->resolution = distant ? ENVPROBS_DIST_RES : ENVPROBS_RES;
			comp->type = EP_PARALLAX_NONE;
			comp->diffCube = TexMgr::nullres;
			comp->specCube = TexMgr::nullres;
			comp->eptex_name = "";
			comp->distance = 10.0f;
			comp->fade = 0.1f;
			comp->mips_count = 1;

			if(!distant)
			{
				earlyVisibilitySys->SetType(e, BT_SPHERE);
				earlyVisibilitySys->SetBSphere(e, BoundingSphere(Vector3(0,0,0), 1));
			}
		}
		bool HasComponent(Entity e) const {return components.has(e.index());}
		size_t ComponentsCount() {return components.dataSize();}

		inline EnvProbComponent* GetComponent(Entity e)
		{
			size_t idx = components.getArrayIdx(e.index());
			if(idx == components.capacity()) return nullptr;
			return &components.getDataByArrayIdx(idx);
		}
		void DeleteComponent(Entity e)
		{
			auto comp = GetComponent(e);
			if(!comp)
				return;
			TEXTURE_DROP(comp->specCube);
			TEXTURE_DROP(comp->diffCube);
			components.remove(e.index());
		}
		
		void RegToScene();
		
		bool IsDirty(Entity e);
		bool SetDirty(Entity e);
		
		bool Bake(Entity e);

		void LoadCubemap(EnvProbComponent* comp);

		void UpdateEnvProps(Entity e);

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<EnvProbSystem>("EnvProbSystem")
					.addFunction("AddComponent", &EnvProbSystem::AddComponent)
					.addFunction("DeleteComponent", &EnvProbSystem::DeleteComponent)
					.addFunction("HasComponent", &EnvProbSystem::HasComponent)

					.addFunction("Bake", &EnvProbSystem::Bake)
				.endClass();
		}

	private:

		ComponentRDArray<EnvProbComponent> components;

		TransformSystem* transformSys;
		EarlyVisibilitySystem* earlyVisibilitySys;
		SArray<Frustum*, FRUSTUM_MAX_COUNT>* frustums;
		BaseWorld* world;
	};
}