#pragma once

#include "DataTypes.h"
#include "Render.h"
#include "ScenePipeline.h"
#include "LocalTimer.h"
#include "CubeRenderTarget.h"

#include "Entity.h"
#include "VisibilitySystem.h"
#include "EarlyVisibilitySystem.h"
#include "TransformSystem.h"
#include "CameraSystem.h"
#include "StaticMeshSystem.h"
#include "ControllerSystem.h"
#include "EnvProbSystem.h"
#include "LightSystem.h"
#include "LineGeometrySystem.h"
#include "ShadowSystem.h"
#include "GlobalLightSystem.h"
#include "TypeMgr.h"
#include "ScriptSystem.h"
#include "PhysicsSystem.h"
#include "TriggerSystem.h"
#include "CollisionSystem.h"
#include "SkeletonSystem.h"

#include "GIMgr.h"

#include "Frustum.h"
#include "SceneGraph.h"

#include "FunctionCodeMgr.h"
#include "DebugDrawer.h"

#define DEFAULT_ENV "default"
#define ENV_MESH PATH_SYS_MESHES "sky_shpere" EXT_MESH

#define TRANSFORM_BYTE		't'
#define EARLYVIS_BYTE		'e'
#define VIS_BYTE			'v'
#define STATIC_BYTE			's'
#define SKELETON_BYTE		'k'
#define LIGHT_BYTE			'l'
#define GLIGHT_BYTE			'g'
#define CAMERA_BYTE			'c'
#define SCRIPT_BYTE			'u'
#define COLLISION_BYTE		'C'
#define PHYSICS_BYTE		'f'
#define TRIGGER_BYTE		'T'
#define ENVPROB_BYTE		'E'

#define SMALL_ENTITY_COUNT 128
#define SMALL_SCENEGRAPH_SIZE SMALL_ENTITY_COUNT * 4

#define SCENEGRAPH_SIZE ENTITY_COUNT * 4

#define WORLD_FILE_VERSION 105

#define COPY_BUFFER_SIZE 512*1024

#define SHADER_PROB_CAPTURTE PATH_SHADERS "offline/prob_capture", "Capture"

#define FILE_WORLD_DATA "/world"
#define FILE_CODE_DATA "/code"

namespace EngineCore
{
	class TypeMgr;
	
	class BaseWorld
	{
		friend TypeMgr;
	public:

		// TODO: move editor camera to separete file in lua
		struct WorldHeader
		{
			uint32_t version;

			char env_name[256];
			XMVECTOR env_rot;

			XMVECTOR free_cam_pos;
			XMVECTOR free_cam_rot;
		};

		enum StateMode
		{
			NO_LIVE = 0,
			LIVE,
		};

		BaseWorld(uint32_t id);

		bool Init(string filename);
		bool Init();

		virtual void Snapshot(ScenePipeline* scene) = 0;
		virtual void Frame() = 0;
		virtual void Close();

		void SetDirty(Entity e);
		void SetDirtyFromSceneGraph(Entity e);

		string GetWorldName() const { return world_name; }
		uint32_t GetID() const { return ID; }

		void AddScene(ScenePipeline* scene)
		{
			if (m_scenes.find(scene) == m_scenes.end())
				m_scenes.push_back(scene);
		}

		ScenePipeline* CreateScene(Entity cam, int w, int h, bool lightweight)
		{
			ScenePipeline* scene = new ScenePipeline();

			RenderInitConfig initCfg;
			initCfg.lightweight = lightweight;

			if (!scene->Init(this, w, h, initCfg))
			{
				_CLOSE(scene); return nullptr;
			}
			m_cameraSystem->AssignScene(cam, scene);
			m_scenes.push_back(scene);
			return scene;
		}

		bool DeleteScene(ScenePipeline* scene)
		{
			auto it = m_scenes.find(scene);
			if (it != m_scenes.end())
			{
				_CLOSE(scene);
				m_scenes.erase(it);
				return true;
			}
			return false;
		}

		ScenePipeline* GetScene(int32_t i)
		{
			if (i >= m_scenes.size())
				return nullptr;
			return m_scenes[i];
		}

#ifdef _EDITOR
		inline void PostMeshesReload()
		{
			if (m_staticMeshSystem)
				m_staticMeshSystem->FixBBoxes();
		}
#endif

		void RawInput(RawInputData& data)
		{
			m_controllerSystem->RawInput(data);
		}

		bool IsActive() const { return b_active; }
		void SetActive(bool active) { b_active = active; }

		int32_t GetMode() const { return (int32_t)m_mode; }
		void SetMode(int32_t m) { m_mode = (StateMode)m; }

		inline float GetDT() const { return m_dt; }

		bool BeginCaptureProb(int32_t resolution, DXGI_FORMAT fmt, bool isLightweight = false, uint32_t arrayCount = 1);
		ID3D11ShaderResourceView* CaptureProb(Matrix& probTransform, float nearClip, float farClip, uint32_t arrayID = 0);
		bool CaptureProbMipGen();
		void CaptureProbClear();
		void EndCaptureProb();

		inline ID3D11ShaderResourceView* GetCaptureProbSRV()
		{
			return probTarget.GetShaderResourceView();
		}

		inline FrustumMgr* GetFrustumMgr() const {return m_frustumMgr;}
		inline SceneGraph* GetSceneGraph() const {return m_sceneGraph;}

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
		inline CollisionSystem* GetCollisionSystem() const {return m_collisionSystem;}
		inline PhysicsSystem* GetPhysicsSystem() const {return m_physicsSystem;}
		inline TriggerSystem* GetTriggerSystem() const {return m_triggerSystem;}
		inline SkeletonSystem* GetSkeletonSystem() const {return m_skeletonSystem;}

		inline TypeMgr* GetTypeMgr() const {return m_typeMgr;}
		inline NameMgr* GetNameMgr() const { return m_nameMgr; }

		inline GIMgr* GetGIMgr() const {return giMgr;}

		virtual DebugDrawer* GetDebugDrawer() { return nullptr; }

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
				
		void SetEntityEnable(Entity e, bool enable, bool noCamera = false);
		inline void SetEntityEnableLua(Entity e, bool enable) {SetEntityEnable(e, enable);}

		inline bool IsEntityEnable(Entity e) {return m_entityMgr->IsEnable(e);}

		inline bool IsEntityNeedProcess(Entity e) {return m_entityMgr->IsNeedProcess(e);}

		inline bool IsEntityAlive(Entity e) {return m_entityMgr->IsAlive(e);}
		void DestroyEntity(Entity e);
		void DestroyEntityHierarchically(Entity e);
		// TODO: move to serialization (expl: PhysicsSystem)
		Entity CopyEntity(Entity e);

		inline void RenameEntity(Entity e, string name) {m_nameMgr->SetName(e, name);}

		inline bool SetEntityType(Entity e, string type) {return m_typeMgr->SetType(e, type);}
		inline string GetEntityType(Entity e) {return m_typeMgr->GetType(e);}
		inline bool IsEntityType(Entity e, string t) {return m_typeMgr->IsThisType(e, t);}
		inline Entity GetFirstEntityByType(string type) {return m_typeMgr->GetFirstByType(type);}
		inline Entity GetNextEntityByType() {return m_typeMgr->GetNextByType();}

		float GetTimeScale() const {return m_world_timer.GetScale();}
		void SetTimeScale(float scale) {m_world_timer.SetScale(scale);}

		inline LuaRef WrapEntityForLua(Entity e)
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

		void UpdateEnvProbRenderData(Entity e);

		void UpdateScript(Entity e)
		{
			m_scriptSystem->UpdateScript(e);
			m_triggerSystem->UpdateCallbacks(e);
		}

		void UpdateCollision(Entity e)
		{
			m_collisionSystem->UpdateState(e);
			m_triggerSystem->UpdateState(e);
			m_physicsSystem->UpdateState(e);
		}

#ifdef _EDITOR
		inline void SetEntityEditorVisible(Entity e, bool visible) {m_entityMgr->SetEditorVisible(e, visible);}
		inline bool IsEntityEditorVisible(Entity e) {return m_entityMgr->IsEditorVisible(e);}

		inline void ConstructEditorGui(Entity e)
		{
			LuaRef luaEnt = WrapEntityForLua(e);
			if(!luaEnt.isNil())
			{
				LuaRef editorConstructor = luaEnt["editor_init"];
				if(editorConstructor.isFunction())
					LUA_CALL(editorConstructor(luaEnt),);
			}
		}

		inline void AddCode(string func, string code) { codeMgr.Add(func, code); }
		inline void RemoveCode(string func)	{ codeMgr.Remove(func); }
		inline string GetCode(string func) { return codeMgr.Get(func); }
#endif

#ifdef _DEV
		void UpdateLuaFuncs()
		{
			m_typeMgr->UpdateLuaFuncs();
			m_scriptSystem->UpdateLuaFuncs();
			m_controllerSystem->UpdateLuaFuncs();
			m_triggerSystem->UpdateLuaFuncs();
		}
#endif

		void SetSceneGraphDebugDraw(bool draw)
		{
			b_sceneGraphDbg = draw;
		}
		
		inline uint8_t* GetCopyBuffer() {return copyBuffer;}

		bool BakeGI()
		{
#ifdef _EDITOR
			return giMgr->BakeGI();
#else
			return false;
#endif
		}

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
					.addFunction("DestroyEntityHierarchically", &BaseWorld::DestroyEntityHierarchically)
					.addFunction("CopyEntity", &BaseWorld::CopyEntity)
					.addFunction("RenameEntity", &BaseWorld::RenameEntity)
					.addFunction("GetEntityByName", &BaseWorld::GetEntityByName)

					.addFunction("SetEntityType", &BaseWorld::SetEntityType)
					.addFunction("GetEntityType", &BaseWorld::GetEntityType)
					.addFunction("IsEntityType", &BaseWorld::IsEntityType)
					.addFunction("GetFirstEntityByType", &BaseWorld::GetFirstEntityByType)
					.addFunction("GetNextEntityByType", &BaseWorld::GetNextEntityByType)

					.addFunction("SetEntityEnable", &BaseWorld::SetEntityEnableLua)
					.addFunction("IsEntityEnable", &BaseWorld::IsEntityEnable)

					.addFunction("GetLuaEntity", &BaseWorld::WrapEntityForLua)
					.addFunction("UpdateScript", &BaseWorld::UpdateScript)
					.addFunction("UpdateCollision", &BaseWorld::UpdateCollision)
					.addFunction("UpdateEnvProbRenderData", &BaseWorld::UpdateEnvProbRenderData)					

				#ifdef _EDITOR
					.addFunction("SetEntityEditorVisible", &BaseWorld::SetEntityEditorVisible)
					.addFunction("IsEntityEditorVisible", &BaseWorld::IsEntityEditorVisible)

					.addFunction("AddCode", &BaseWorld::AddCode)
					.addFunction("RemoveCode", &BaseWorld::RemoveCode)
					.addFunction("GetCode", &BaseWorld::GetCode)
				#endif

					.addProperty("transform", &BaseWorld::GetTransformSystem)
					.addProperty("visibility", &BaseWorld::GetVisibilitySystem)
					.addProperty("earlyVisibility", &BaseWorld::GetEarlyVisibilitySystem)
					.addProperty("staticMesh", &BaseWorld::GetStaticMeshSystem)
					.addProperty("camera", &BaseWorld::GetCameraSystem)
					.addProperty("light", &BaseWorld::GetLightSystem)
					.addProperty("envprobs", &BaseWorld::GetEnvProbSystem)
					.addProperty("globalLight", &BaseWorld::GetGlobalLightSystem)
					.addProperty("lineGeometry", &BaseWorld::GetLineGeometrySystem)
					.addProperty("controller", &BaseWorld::GetControllerSystem)
					.addProperty("script", &BaseWorld::GetScriptSystem)
					.addProperty("collision", &BaseWorld::GetCollisionSystem)
					.addProperty("physics", &BaseWorld::GetPhysicsSystem)
					.addProperty("trigger", &BaseWorld::GetTriggerSystem)
					.addProperty("skeleton", &BaseWorld::GetSkeletonSystem)

					.addProperty("active", &BaseWorld::IsActive, &BaseWorld::SetActive)
					.addProperty("mode", &BaseWorld::GetMode, &BaseWorld::SetMode)
					.addProperty("timeScale", &BaseWorld::GetTimeScale, &BaseWorld::SetTimeScale)

					.addFunction("SetSceneGraphDebugDraw", &BaseWorld::SetSceneGraphDebugDraw)

					.addFunction("BakeGI", &BaseWorld::BakeGI)
				.endClass();
		}

	protected:
		void destroyEntity(Entity e);

		bool loadWorld(string& filename, WorldHeader& header);
		
		bool saveWorld(string& filename);

		bool b_active;
		StateMode m_mode;

		bool b_sceneGraphDbg;

		LocalTimer m_world_timer;
		float m_dt;

		string world_name;
		uint32_t ID;
				
		DArray<ScenePipeline*> m_scenes;

		FrustumMgr* m_frustumMgr;
		
		SceneGraph* m_sceneGraph;
		
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
		CollisionSystem* m_collisionSystem;
		PhysicsSystem* m_physicsSystem;
		TriggerSystem* m_triggerSystem;
		SkeletonSystem* m_skeletonSystem;

		TypeMgr* m_typeMgr;
		NameMgr* m_nameMgr;

#ifdef _EDITOR
		FunctionCodeMgr codeMgr;
#endif

		// GI
		GIMgr* giMgr;

		// physics world
		btDefaultCollisionConfiguration* physCollisionConfiguration;
		btCollisionDispatcher* physCollisionDispatcher;
		btBroadphaseInterface* physBroadphase;
		btSequentialImpulseConstraintSolver* physConstraintSolver;
		btGhostPairCallback* physGhostCallback;
		btDiscreteDynamicsWorld* physDynamicsWorld;

		uint8_t* copyBuffer;

		uint32_t frameID;

		// prob capture
		ScenePipeline* probScene;
		Entity probCamera;
		CubeRenderTarget probTarget;
		Compute* probCaptureShader;
	};
	
	class World: public BaseWorld
	{
	public:
		World( uint32_t id );
		
		void Snapshot(ScenePipeline* scene);
		void Frame();
		virtual void Close();

		virtual DebugDrawer* GetDebugDrawer() override { return &dbgDrawer; }

		bool SaveWorld(string filename)
		{
			return saveWorld(filename);
		}

		void GIDebugSetState(int32_t s)
		{
#ifdef _DEV
			giMgr->DebugSetState(GIMgr::DebugState(s));
#endif
		}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.deriveClass<World, BaseWorld>("World")
					.addFunction("Save", &World::SaveWorld)
					.addFunction("GIDebugSetState", &World::GIDebugSetState)
				.endClass();
		}

	protected:
		DebugDrawer dbgDrawer;

		CollisionDebugDrawer* physDebugDrawer;
	};

	class SmallWorld: public BaseWorld
	{
	public:
		SmallWorld( uint32_t id );
		
		void Snapshot(ScenePipeline* scene);
		void Frame();
		virtual void Close();
		
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