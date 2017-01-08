#pragma once

#include "DataTypes.h"
#include "Render.h"
#include "ScenePipeline.h"
#include "LocalTimer.h"

#include "ECS/Entity.h"
#include "ECS/VisibilitySystem.h"
#include "ECS/EarlyVisibilitySystem.h"
#include "ECS/TransformSystem.h"
#include "ECS/CameraSystem.h"
#include "ECS/StaticMeshSystem.h"
#include "ECS/ControllerSystem.h"
#include "ECS/FreeCamController.h"
#include "ECS/EnvProbSystem.h"
#include "ECS/LightSystem.h"
#include "ECS/LineGeometrySystem.h"
#include "ECS/ShadowSystem.h"
#include "ECS/GlobalLightSystem.h"
#include "ECS/TypeMgr.h"
#include "ECS/ScriptSystem.h"
#include "Frustum.h"
#include "TransformControls.h"

#define DEFAULT_ENV L"preview"
#define ENV_MESH PATH_SYS_MESHES "sky_shpere" EXT_STATIC

#define TRANSFORM_BYTE 't'
#define EARLYVIS_BYTE 'e'
#define VIS_BYTE 'v'
#define STATIC_BYTE 's'
#define LIGHT_BYTE 'l'
#define GLIGHT_BYTE 'g'
#define CAMERA_BYTE 'c'
#define SCRIPT_BYTE 'u'

#define SMALL_ENTITY_COUNT 128

namespace EngineCore
{
	class TypeMgr;
	
	class BaseWorld
	{
		friend TypeMgr;
	public:
		BaseWorld();

		bool Init(string filename);
		bool Init();
		
		virtual void Snapshot(ScenePipeline* scene) = 0;
		virtual void Frame() = 0;
		virtual void Close();
		
		void SetDirty(Entity e);
				
		string GetWorldName() const {return world_name;}
		UINT GetID() const {return ID;}
		void SetID(UINT l_ID) {ID = l_ID;}

		void AddScene(ScenePipeline* scene)
		{
			if(m_scenes.find(scene) == m_scenes.end())
				m_scenes.push_back(scene);
		}

		ScenePipeline* CreateScene(Entity cam, int w, int h, bool lightweight)
		{
			ScenePipeline* scene = new ScenePipeline();
			if(!scene->Init(w, h, lightweight))
			{_CLOSE(scene); return nullptr;}
			m_cameraSystem->Activate(cam, scene);
			m_scenes.push_back(scene);
			return scene;
		}

		bool DeleteScene(ScenePipeline* scene)
		{
			auto it = m_scenes.find(scene);
			if(it != m_scenes.end())
			{
				_CLOSE(scene);
				m_scenes.erase(it);
				return true;
			}
			return false;
		}

		inline void PostStMeshesReload()
		{
			if(m_staticMeshSystem)
				m_staticMeshSystem->PostReload();
		}
				
		bool IsActive() const {return b_active;}
		void SetActive(bool active) {b_active = active;}

		inline float GetDT() const {return m_dt;} 

		inline FrustumMgr* GetFrustumMgr() const {return m_frustumMgr;}
		inline EntityMgr* GetEntityMgr() const {return m_entityMgr;}
		inline TransformSystem* GetTransformSystem() const {return m_transformSystem;}
		inline VisibilitySystem* GetVisibilitySystem() const {return m_visibilitySystem;}
		inline EarlyVisibilitySystem* GetEarlyVisibilitySystem() const {return m_earlyVisibilitySystem;}
		inline StaticMeshSystem* GetStaticMeshSystem() const {return m_staticMeshSystem;}
		inline CameraSystem* GetCameraSystem() const {return m_cameraSystem;}
		inline EnvProbSystem* GetEnvProbSystem() const {return m_envProbSystem;}
		inline LightSystem* GetLightSystem() const {return m_lightSystem;}
		inline LineGeometrySystem* GetLineGeometrySystem() const {return m_lineGeometrySystem;}
		inline ControllerSystem* GetControllerSystem() const {return m_controllerSystem;}
		inline ShadowSystem* GetShadowSystem() const {return m_shadowSystem;}
		inline GlobalLightSystem* GetGlobalLightSystem() const {return m_globalLightSystem;}
		inline ScriptSystem* GetScriptSystem() const {return m_scriptSystem;}

		inline TypeMgr* GetTypeMgr() const {return m_typeMgr;}
		inline NameMgr* GetNameMgr() const {return m_nameMgr;}

		inline Entity CreateEntity() {return m_entityMgr->CreateEntity();}
		Entity CreateNamedEntity(string name) 
		{
			Entity res = m_entityMgr->CreateEntity();
			m_nameMgr->SetName(res, name);
			return res;
		}
		string GetEntityName(Entity e)
		{ 
			if(e.isnull())
				return "";
			return m_nameMgr->GetName(e);
		}
		Entity GetEntityByName(string name) { return m_nameMgr->GetEntityByName(name); }
				
		inline void SetEntityEnable(Entity e, bool enable) {m_entityMgr->SetEnable(e, enable);}
		inline bool IsEntityEnable(Entity e) {return m_entityMgr->IsEnable(e);}

		inline bool IsEntityNeedProcess(Entity e) {return m_entityMgr->IsNeedProcess(e);}

	#ifdef _EDITOR
		inline void SetEntityEditorVisible(Entity e, bool visible) {m_entityMgr->SetEditorVisible(e, visible);}
		inline bool IsEntityEditorVisible(Entity e) {return m_entityMgr->IsEditorVisible(e);}
	#endif

		inline Entity RestoreEntity() {return m_entityMgr->RestoreEntity();}

		inline bool IsEntityAlive(Entity e) {return m_entityMgr->IsAlive(e);}
		void DestroyEntity(Entity e);
		Entity CopyEntity(Entity e);

		inline void RenameEntity(Entity e, string name) {m_nameMgr->SetName(e, name);}

		inline bool SetEntityType(Entity e, string type) {return m_typeMgr->SetType(e, type);}
		inline string GetEntityType(Entity e) {return m_typeMgr->GetType(e);}
		inline Entity GetFirstEntityByType(string type) {return m_typeMgr->GetFirstByType(type);}
		inline Entity GetNextEntityByType() {return m_typeMgr->GetNextByType();}

		inline LuaRef WrapEntityForLua(Entity e) // must be used somewhere
		{
			LuaRef res = m_scriptSystem->GetLuaClassInstance(e);
			if(res.isNil())
			{
				auto type = m_typeMgr->GetType(e);
				if(!type.empty())
					return m_typeMgr->LuaConstructor(type, e);
			}
			return res;
		}

	#ifdef _DEV
		void UpdateLuaFuncs()
		{
			m_typeMgr->UpdateLuaFuncs();
			m_scriptSystem->UpdateLuaFuncs();
		}
	#endif
		
		struct WorldHeader
		{
			wchar_t env_name[256];
			XMVECTOR env_rot;
			
			XMVECTOR free_cam_pos;
			XMVECTOR free_cam_rot;
		};

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<BaseWorld>("BaseWorld")
					.addFunction("GetID", &BaseWorld::GetID)
					.addFunction("GetWorldName", &BaseWorld::GetWorldName)

					.addFunction("AddScene", &BaseWorld::AddScene)
					.addFunction("CreateScene", &BaseWorld::CreateScene)
					.addFunction("DeleteScene", &BaseWorld::DeleteScene)

					.addFunction("Snapshot", &BaseWorld::Snapshot)
					
					.addFunction("CreateEntity", &BaseWorld::CreateEntity)
					.addFunction("CreateNamedEntity", &BaseWorld::CreateNamedEntity)
					.addFunction("GetEntityName", &BaseWorld::GetEntityName)
					.addFunction("IsEntityAlive", &BaseWorld::IsEntityAlive)
					.addFunction("DestroyEntity", &BaseWorld::DestroyEntity)
					.addFunction("RestoreEntity", &BaseWorld::RestoreEntity)
					.addFunction("CopyEntity", &BaseWorld::CopyEntity)
					.addFunction("RenameEntity", &BaseWorld::RenameEntity)
					.addFunction("GetEntityByName", &BaseWorld::GetEntityByName)

					.addFunction("SetEntityType", &BaseWorld::SetEntityType)
					.addFunction("GetEntityType", &BaseWorld::GetEntityType)
					.addFunction("GetFirstEntityByType", &BaseWorld::GetFirstEntityByType)
					.addFunction("GetNextEntityByType", &BaseWorld::GetNextEntityByType)

					.addFunction("SetEntityEnable", &BaseWorld::SetEntityEnable)
					.addFunction("IsEntityEnable", &BaseWorld::IsEntityEnable)

				#ifdef _EDITOR
					.addFunction("SetEntityEditorVisible", &BaseWorld::SetEntityEditorVisible)
					.addFunction("IsEntityEditorVisible", &BaseWorld::IsEntityEditorVisible)
				#endif

					.addProperty("transform", &BaseWorld::GetTransformSystem)
					.addProperty("visibility", &BaseWorld::GetVisibilitySystem)
					.addProperty("earlyVisibility", &BaseWorld::GetEarlyVisibilitySystem)
					.addProperty("staticMesh", &BaseWorld::GetStaticMeshSystem)
					.addProperty("camera", &BaseWorld::GetCameraSystem)
					.addProperty("light", &BaseWorld::GetLightSystem)
					.addProperty("globalLight", &BaseWorld::GetGlobalLightSystem)
					.addProperty("lineGeometry", &BaseWorld::GetLineGeometrySystem)
					.addProperty("controller", &BaseWorld::GetControllerSystem)
					.addProperty("script", &BaseWorld::GetScriptSystem)

					.addProperty("active", &BaseWorld::IsActive, &BaseWorld::SetActive)
				.endClass();
		}

	protected:
		bool loadWorld(string& filename, WorldHeader& header);
		void initMainEntities(WorldHeader header);
		
		bool saveWorld(string& filename);

		bool b_active;

		LocalTimer m_world_timer;
		float m_dt;

		string world_name;
		UINT ID;

		Entity skyEP;

		wstring envName;

		DArray<ScenePipeline*> m_scenes;

		FrustumMgr* m_frustumMgr;

		EntityMgr* m_entityMgr;
		TransformSystem* m_transformSystem;
		VisibilitySystem* m_visibilitySystem;
		EarlyVisibilitySystem* m_earlyVisibilitySystem;
		StaticMeshSystem* m_staticMeshSystem;
		CameraSystem* m_cameraSystem;
		ControllerSystem* m_controllerSystem;
		EnvProbSystem* m_envProbSystem;
		LightSystem* m_lightSystem;
		LineGeometrySystem* m_lineGeometrySystem;
		ShadowSystem* m_shadowSystem;
		GlobalLightSystem* m_globalLightSystem;
		ScriptSystem* m_scriptSystem;
		
		TypeMgr* m_typeMgr;
		NameMgr* m_nameMgr;
	};
	
	class World: public BaseWorld
	{
	public:
		World();
		
		void Snapshot(ScenePipeline* scene);
		void Frame();
		void Close();
		
		bool SaveWorld(string filename)
		{
			return saveWorld(filename);
		}
		
		inline TransformControls* GetTransformControls() const {return m_transformControls;}
		
		void RebakeSky()
		{
			m_envProbSystem->Bake(skyEP);
		}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.deriveClass<World, BaseWorld>("World")
					.addFunction("Save", &World::SaveWorld)
					.addFunction("RebakeSky", &World::RebakeSky)

					.addProperty("transformControls", &World::GetTransformControls)
				.endClass();
		}

	protected:
		TransformControls* m_transformControls;
	};

	class SmallWorld: public BaseWorld
	{
	public:
		SmallWorld();
		
		void Snapshot(ScenePipeline* scene);
		void Frame();
		void Close();
		
		bool SaveWorld(string filename)
		{
			return saveWorld(filename);
		}
		
		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.deriveClass<SmallWorld, BaseWorld>("SmallWorld")
					.addFunction("Save", &SmallWorld::SaveWorld)
				.endClass();
		}
	};
}