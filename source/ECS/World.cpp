#include "stdafx.h"
#include "World.h"
#include "Common.h"
#include "TypeMgr.h"
#include "Profiler.h"
#include "RenderMgrs.h"
#include "EnvProbMgr.h"

using namespace EngineCore;

BaseWorld::BaseWorld( uint32_t id )
{
	b_active = false;
	m_mode = StateMode::NO_LIVE;
	b_sceneGraphDbg = false;

	world_name = "";
	ID = id;

	m_scenes.reserve(16);

	m_dt = 0;
	frameID = 0;
	probScene = nullptr;

	probCaptureShader = new Compute(SHADER_PROB_CAPTURTE);

	copyBuffer = new uint8_t[COPY_BUFFER_SIZE];

	giMgr = new GIMgr(this);
}

void BaseWorld::SetDirty(Entity e)
{
	m_transformSystem->SetDirty(e);
}

void BaseWorld::SetDirtyFromSceneGraph(Entity e)
{
	m_collisionSystem->SetDirty(e);
	m_physicsSystem->SetDirty(e);
	m_triggerSystem->SetDirty(e);

	m_visibilitySystem->SetDirty(e);

	if(m_earlyVisibilitySystem)
		m_earlyVisibilitySystem->SetDirty(e);

	m_skeletonSystem->SetDirty(e);
	m_staticMeshSystem->SetDirty(e);
	m_cameraSystem->SetDirty(e);
	m_envProbSystem->SetDirty(e);

	if(m_lightSystem)
		m_lightSystem->SetDirty(e);
	if(m_shadowSystem)
		m_shadowSystem->SetDirty(e);
	if(m_globalLightSystem)
		m_globalLightSystem->SetDirty(e);
	if(m_lineGeometrySystem)
		m_lineGeometrySystem->SetDirty(e);
}

void BaseWorld::UpdateEnvProbRenderData(Entity e)
{
	auto comp = m_envProbSystem->GetComponent(e);
	for(auto it: m_scenes)
		it->render_mgr->envProbMgr->ForceUpdate(comp->probId);
}

bool BaseWorld::Init(string filename)
{
	if(world_name.size() > 0)
		return false;

	WorldHeader header;

	world_name = filename;
	if( !loadWorld(filename, header) )
	{
		ERR("Can\'t load world %s", filename.data());
		return false;
	}
	
	m_world_timer.Start();
		
	return true;
}

bool BaseWorld::Init()
{
	if(world_name.size() > 0)
		return false;

	world_name = "new_scene.mls";

	WorldHeader header;
	header.version = WORLD_FILE_VERSION;
	strcpy_s(header.env_name, DEFAULT_ENV);
	header.free_cam_pos = XMVectorZero();
	header.free_cam_rot = XMVectorZero();
	header.env_rot = XMVectorZero();
	
	m_world_timer.Start();
	
	return true;
}

void BaseWorld::Close()
{
	world_name = "";
		
	probScene = nullptr;
	probCamera.setnull();

	for(auto& it: m_scenes)
	{
		if(it)
		{
			ScenePipeline* scene = it;
			_CLOSE(scene);
		}
	}
	m_scenes.clear();
	
	_DELETE(giMgr);

	_DELETE(m_staticMeshSystem);
	_DELETE(m_skeletonSystem);
	_DELETE(m_cameraSystem);
	_DELETE(m_controllerSystem);
	_DELETE(m_envProbSystem);
	_DELETE(m_lightSystem);
	_DELETE(m_shadowSystem);
	_DELETE(m_globalLightSystem);
	_DELETE(m_lineGeometrySystem);

	_DELETE(m_frustumMgr);
	_DELETE(m_sceneGraph);
	_DELETE(m_entityMgr);
	_DELETE(m_transformSystem);
	_DELETE(m_visibilitySystem);
	_DELETE(m_earlyVisibilitySystem);
	_DELETE(m_scriptSystem);
	_DELETE(m_physicsSystem);
	_DELETE(m_triggerSystem);
	_DELETE(m_collisionSystem);

	_DELETE(m_typeMgr);
	_DELETE(m_nameMgr);

	_DELETE(physDynamicsWorld);
	_DELETE(physConstraintSolver);
	_DELETE(physBroadphase);
	_DELETE(physCollisionDispatcher);
	_DELETE(physGhostCallback);
	_DELETE(physCollisionConfiguration);

	_DELETE_ARRAY(copyBuffer);

	_DELETE(probCaptureShader);
}

void BaseWorld::DestroyEntityHierarchically(Entity e)
{
	Entity child = m_transformSystem->GetChildFirst(e);
	while(!child.isnull()) 
	{
		Entity next = m_transformSystem->GetChildNext(child);
		DestroyEntityHierarchically(child);
		child = next;
	}	

	return destroyEntity(e);
}

void BaseWorld::DestroyEntity(Entity e)
{
#ifdef _EDITOR
	Entity child = m_transformSystem->GetChildFirst(e);
	string editorType(EDITOR_TYPE);
	while(!child.isnull()) 
	{
		Entity next = m_transformSystem->GetChildNext(child);
		if(m_typeMgr->IsThisType(child, editorType))
			DestroyEntity(child);
		child = next;
	}	
#endif

	return destroyEntity(e);
}

void BaseWorld::destroyEntity(Entity e)
{
	m_scriptSystem->SendKill(e);

	// immidiate
	m_transformSystem->DeleteComponent(e);
	m_visibilitySystem->DeleteComponent(e);

	if(m_earlyVisibilitySystem)
		m_earlyVisibilitySystem->DeleteComponent(e);

	m_physicsSystem->DeleteComponent(e);
	m_triggerSystem->DeleteComponent(e);
	m_collisionSystem->DeleteComponent(e);
	m_scriptSystem->DeleteComponent(e);
	m_staticMeshSystem->DeleteComponent(e);
	m_skeletonSystem->DeleteComponent(e);
	m_cameraSystem->DeleteComponent(e);
	m_controllerSystem->DeleteComponent(e);
	m_envProbSystem->DeleteComponent(e);

	if(m_lightSystem)
		m_lightSystem->DeleteComponent(e);
	if(m_shadowSystem)
		m_shadowSystem->DeleteComponents(e);
	if(m_globalLightSystem)
		m_globalLightSystem->DeleteComponent(e);
	if(m_lineGeometrySystem)
		m_lineGeometrySystem->DeleteComponent(e);

	m_typeMgr->ClearType(e);
	m_nameMgr->ClearName(e);

	return m_entityMgr->Destroy(e);
}

Entity BaseWorld::CopyEntity(Entity e)
{
	Entity newEnt;
	newEnt.setnull();

	if(!m_entityMgr->IsAlive(e))
		return newEnt;

	newEnt = CreateEntity();
	if(newEnt.isnull())
		return newEnt;

	m_transformSystem->CopyComponent(e, newEnt);
	m_visibilitySystem->CopyComponent(e, newEnt);

	if(m_earlyVisibilitySystem)
		m_earlyVisibilitySystem->CopyComponent(e, newEnt);

	m_scriptSystem->CopyComponent(e, newEnt);

	m_skeletonSystem->CopyComponent(e, newEnt);
	m_staticMeshSystem->CopyComponent(e, newEnt);

	m_collisionSystem->CopyComponent(e, newEnt);
	m_physicsSystem->CopyComponent(e, newEnt);
	m_triggerSystem->CopyComponent(e, newEnt);

	m_cameraSystem->CopyComponent(e, newEnt);
	m_envProbSystem->CopyComponent(e, newEnt);
	
	if(m_lightSystem)
		m_lightSystem->CopyComponent(e, newEnt);
	if(m_globalLightSystem)
		m_globalLightSystem->CopyComponent(e, newEnt);

	m_typeMgr->SetType(newEnt, m_typeMgr->GetType(e));

#ifdef _EDITOR
	ConstructEditorGui(newEnt);
#endif

	return newEnt;
}

void BaseWorld::SetEntityEnable(Entity e, bool enable, bool noCamera)
{
	m_entityMgr->SetEnable(e, enable);

	m_collisionSystem->SetEnable(e, enable);
	m_physicsSystem->SetEnable(e, enable);
	m_triggerSystem->SetEnable(e, enable);
	
	if(!noCamera)
		m_cameraSystem->SetEnable(e, enable);
}

#define SAVE_BUFFER_SIZE 16384

bool BaseWorld::loadWorld(string& filename, WorldHeader& header)
{
	string worldDataFile = filename + FILE_WORLD_DATA;

	uint32_t data_size = 0;
	unique_ptr<uint8_t> data(FileIO::ReadFileData(worldDataFile, &data_size));
	uint8_t* t_data = data.get();

	if(!t_data)
	{
		ERR("Cant read world file %s !", filename.c_str());
		return false;
	}

	if( data_size < sizeof(WorldHeader) + sizeof(uint32_t) )
	{
		ERR("World file %s corrupted!", filename.c_str());
		return false;
	}

	// header
	memcpy_s( &header, sizeof(WorldHeader), t_data, sizeof(WorldHeader) );
	t_data += sizeof(WorldHeader);

	if(header.version != WORLD_FILE_VERSION)
	{
		ERR("World %s has wrong version!", filename.c_str());
		return false;
	}
	
	// GI
	GISampleData giData;
	memcpy_s(&giData, sizeof(GISampleData), t_data, sizeof(GISampleData));
	t_data += sizeof(GISampleData);
	giMgr->LoadGIData(giData);

	m_transformSystem->PreLoad();

	// entities
	uint32_t entCount = *(uint32_t*)t_data;
	if(entCount == 0)
		return true;
	t_data += sizeof(uint32_t);

	while( entCount > 0 && (t_data - data.get()) < data_size )
	{
		uint32_t type_size = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);
		string type((char*)t_data, type_size);
		t_data += type_size * sizeof(char);

		uint32_t name_size = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);
		string name((char*)t_data, name_size);
		t_data += name_size * sizeof(char);

		Entity ent = CreateNamedEntity(name);
		if(!SetEntityType(ent, type))
			WRN("Cant set type %s to entity %s", type.c_str(), name.c_str());

		bool enableState = (*(uint8_t*)t_data) > 0;
		t_data += sizeof(uint8_t);

		uint32_t comp_count = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);
		string components((char*)t_data, comp_count);
		t_data += comp_count * sizeof(char);

		for(uint16_t i = 0; i < comp_count; i++)
		{
			uint32_t compSize = 0;
			switch (components[i])
			{
			case TRANSFORM_BYTE: compSize = m_transformSystem->Deserialize(ent, t_data);
				break;
			case EARLYVIS_BYTE: compSize = m_earlyVisibilitySystem->Deserialize(ent, t_data);
				break;
			case VIS_BYTE: compSize = m_visibilitySystem->Deserialize(ent, t_data);
				break;
			case STATIC_BYTE: compSize = m_staticMeshSystem->Deserialize(ent, t_data);
				break;
			case SKELETON_BYTE: compSize = m_skeletonSystem->Deserialize(ent, t_data);
				break;
			case LIGHT_BYTE: compSize = m_lightSystem->Deserialize(ent, t_data);
				break;
			case GLIGHT_BYTE: compSize = m_globalLightSystem->Deserialize(ent, t_data);
				break;
			case CAMERA_BYTE: compSize = m_cameraSystem->Deserialize(ent, t_data);
				break;
			case SCRIPT_BYTE: compSize = m_scriptSystem->Deserialize(ent, t_data);
				break;
			case COLLISION_BYTE: compSize = m_collisionSystem->Deserialize(ent, t_data);
				break;
			case PHYSICS_BYTE: compSize = m_physicsSystem->Deserialize(ent, t_data);
				break;
			case TRIGGER_BYTE: compSize = m_triggerSystem->Deserialize(ent, t_data);
				break;
			case ENVPROB_BYTE: compSize = m_envProbSystem->Deserialize(ent, t_data);
				break;
			}
			t_data += compSize;
		}

	#ifdef _EDITOR
		ConstructEditorGui(ent);
	#endif

		SetEntityEnable(ent, enableState);

		entCount--;
	}

	m_transformSystem->PostLoadParentsResolve();

#ifdef _EDITOR
	string codeDataFile = filename + FILE_CODE_DATA;
	codeMgr.LoadFromFile(codeDataFile);
#endif

	return true;
}

bool BaseWorld::saveWorld(string& filename)
{
	if (!FileIO::IsExist(filename))
		if (!FileIO::CreateDir(filename))
			return false;

	string worldDataFile = filename + FILE_WORLD_DATA;

	ofstream file;
	file.open(worldDataFile, std::ios::trunc | std::ios::binary );
	if(!file.is_open())
		return false;

	world_name = filename;

	// header
	WorldHeader header;
	header.version = WORLD_FILE_VERSION;

	Entity editorCamera = GetEntityByName(EDITOR_TYPE "Camera");
	if(!editorCamera.isnull())
	{
		XMMATRIX cam_mat = m_transformSystem->GetTransform_WInternal(editorCamera);
		XMVECTOR temp;
		XMMatrixDecompose(&temp, &header.free_cam_rot, &header.free_cam_pos, cam_mat);
	}

	file.write( (char*)&header, sizeof(WorldHeader) );

	// GI
	GISampleData* giData = giMgr->SaveGIData();
	file.write((char*)giData, sizeof(GISampleData));

	// entities
	uint32_t entCount = m_entityMgr->GetEntityCount();
	if(entCount < 2)
		ERR("Wrong entity count in world!");
	else
		entCount -= 2;

	file.write( (char*)&entCount, sizeof(uint32_t) );

	uint8_t* buffer = new uint8_t[SAVE_BUFFER_SIZE];

	Entity iterator;
	iterator.setnull();
	while( !(iterator = m_entityMgr->GetNextEntity(iterator)).isnull() )
	{
		string name = m_nameMgr->GetName(iterator);
		if( name.find(EDITOR_TYPE) != string::npos )
			continue;

		string type = m_typeMgr->GetType(iterator);
		if( type == EDITOR_TYPE )
			continue;

		uint32_t type_size = (uint32_t)type.size();
		file.write( (char*)&type_size, sizeof(uint32_t) );
		if(type_size > 0)
			file.write( (char*)type.data(), type_size * sizeof(char) );

		uint32_t name_size = (uint32_t)name.size();
		file.write( (char*)&name_size, sizeof(uint32_t) );
		if(name_size > 0)
			file.write( (char*)name.data(), name_size * sizeof(char) );

		uint8_t enableState = m_entityMgr->IsEnable(iterator) ? 1 : 0;
		file.write( (char*)&enableState, sizeof(uint8_t) );
		
		string components = "";
		if(m_transformSystem->HasComponent(iterator))
			components += TRANSFORM_BYTE;
		if(m_earlyVisibilitySystem->HasComponent(iterator))
			components += EARLYVIS_BYTE;
		if(m_visibilitySystem->HasComponent(iterator))
			components += VIS_BYTE;
		if(m_staticMeshSystem->HasComponent(iterator))
			components += STATIC_BYTE;
		if(m_skeletonSystem->HasComponent(iterator))
			components += SKELETON_BYTE;
		if(m_lightSystem->HasComponent(iterator))
			components += LIGHT_BYTE;
		if(m_globalLightSystem->HasComponent(iterator))
			components += GLIGHT_BYTE;
		if(m_cameraSystem->HasComponent(iterator))
			components += CAMERA_BYTE;
		if(m_scriptSystem->HasComponent(iterator))
			components += SCRIPT_BYTE;
		if(m_collisionSystem->HasComponent(iterator))
			components += COLLISION_BYTE;
		if(m_physicsSystem->HasComponent(iterator))
			components += PHYSICS_BYTE;
		if(m_triggerSystem->HasComponent(iterator))
			components += TRIGGER_BYTE;
		if(m_envProbSystem->HasComponent(iterator))
			components += ENVPROB_BYTE;

		uint32_t comp_count = (uint32_t)components.size();
		file.write( (char*)&comp_count, sizeof(uint32_t) );
		if(comp_count == 0)
			continue;
		file.write( (char*)components.data(), comp_count * sizeof(char) );

		for(uint16_t i = 0; i < comp_count; i++)
		{
			uint32_t compSize = 0;
			switch (components[i])
			{
			case TRANSFORM_BYTE: compSize = m_transformSystem->Serialize(iterator, buffer);
				break;
			case EARLYVIS_BYTE: compSize = m_earlyVisibilitySystem->Serialize(iterator, buffer);
				break;
			case VIS_BYTE: compSize = m_visibilitySystem->Serialize(iterator, buffer);
				break;
			case STATIC_BYTE: compSize = m_staticMeshSystem->Serialize(iterator, buffer);
				break;
			case SKELETON_BYTE: compSize = m_skeletonSystem->Serialize(iterator, buffer);
				break;
			case LIGHT_BYTE: compSize = m_lightSystem->Serialize(iterator, buffer);
				break;
			case GLIGHT_BYTE: compSize = m_globalLightSystem->Serialize(iterator, buffer);
				break;
			case CAMERA_BYTE: compSize = m_cameraSystem->Serialize(iterator, buffer);
				break;
			case SCRIPT_BYTE: compSize = m_scriptSystem->Serialize(iterator, buffer);
				break;
			case COLLISION_BYTE: compSize = m_collisionSystem->Serialize(iterator, buffer);
				break;
			case PHYSICS_BYTE: compSize = m_physicsSystem->Serialize(iterator, buffer);
				break;
			case TRIGGER_BYTE: compSize = m_triggerSystem->Serialize(iterator, buffer);
				break;
			case ENVPROB_BYTE: compSize = m_envProbSystem->Serialize(iterator, buffer);
				break;
			}

			if(compSize == 0)
				continue;

			file.write( (char*)buffer, compSize );
		}
	}
	_DELETE_ARRAY(buffer);
	
	file.close();

#ifdef _EDITOR
	string codeDataFile = filename + FILE_CODE_DATA;
	codeMgr.DumpToFile(codeDataFile);
#endif

	LOG_GOOD("World %s saved", world_name.data());

	return true;
}

bool BaseWorld::BeginCaptureProb(int32_t resolution, DXGI_FORMAT fmt, bool isLightweight, uint32_t arrayCount)
{
	if (probScene)
		return probScene->Resize(resolution, resolution);

	probCamera = m_entityMgr->CreateEntity();
	
	m_transformSystem->AddComponent(probCamera);

	m_cameraSystem->AddComponent(probCamera);
	m_cameraSystem->SetAspect(probCamera, 1.0f);
	m_cameraSystem->SetFov(probCamera, XM_PIDIV2);

	// TODO
	probScene = CreateScene(probCamera, resolution, resolution, isLightweight);

	if(!probScene)
	{
		DestroyEntity(probCamera);
		probCamera.setnull();
		return false;
	}
	
	if(!probTarget.Init(resolution, fmt, true, arrayCount))
	{
		EndCaptureProb();
		return false;
	}
	
	return true;
}

ID3D11ShaderResourceView* BaseWorld::CaptureProb(Matrix& probTransform, float nearClip, float farClip, uint32_t arrayID)
{
	if( !probScene || probCamera.isnull() || !probCaptureShader || probTarget.GetMipsCount() == 0 )
		return nullptr;
		
	m_transformSystem->SetTransform_L(probCamera, probTransform);
	m_transformSystem->ForceUpdate(probCamera);
	m_cameraSystem->SetNear(probCamera, nearClip);
	m_cameraSystem->SetFar(probCamera, farClip);

	Vector3 cameraRot[6] = {
		Vector3(0, XM_PIDIV2, 0),
		Vector3(0, -XM_PIDIV2, 0),
		Vector3(-XM_PIDIV2, 0, 0),
		Vector3(XM_PIDIV2, 0, 0),
		Vector3(0, 0, 0),
		Vector3(0, XM_PI, 0)
	};
	
	probTarget.ClearCube(arrayID);
	uint32_t groupCount = (uint32_t)ceil(float(probTarget.GetResolution()) / 16);

	Quaternion baseRotation = m_transformSystem->GetRotation_W(probCamera);

	for(int32_t i = 0; i < 6; i++)
	{
		Quaternion faceRotation = Quaternion::CreateFromYawPitchRoll(cameraRot[i].y, cameraRot[i].x, cameraRot[i].z);
		faceRotation = faceRotation * baseRotation;

		m_transformSystem->SetRotation_W(probCamera, faceRotation);
		
		Snapshot(probScene);

		auto uav = probTarget.GetUnorderedAccessView(i, arrayID);
		auto srv = probScene->GetLinearAndDepthSRV();
		
		Render::CSSetShaderResources( 0, 1, &srv );

		probCaptureShader->BindUAV( uav );
		probCaptureShader->Dispatch( groupCount, groupCount, 1 );
		probCaptureShader->UnbindUAV();
	}
	
	return probTarget.GetShaderResourceView();
}

bool BaseWorld::CaptureProbMipGen()
{
	if (probTarget.GetMipsCount() == 0)
		return false;

	probTarget.GenerateMips();
	return true;
}

void BaseWorld::CaptureProbClear()
{
	probTarget.ClearCubeArray();
}

void BaseWorld::EndCaptureProb()
{
	if(!probScene)
		return;
	
	probTarget.Close();
	DeleteScene(probScene);
	probScene = nullptr;
	DestroyEntity(probCamera);
	probCamera.setnull();
}

// World ---------------------
World::World( uint32_t id ) : BaseWorld( id )
{
	physCollisionConfiguration = new btDefaultCollisionConfiguration();
	physCollisionDispatcher = new btCollisionDispatcher(physCollisionConfiguration);
	physBroadphase = new btDbvtBroadphase();
	physConstraintSolver = new btSequentialImpulseConstraintSolver();
	physDynamicsWorld = new btDiscreteDynamicsWorld(physCollisionDispatcher, physBroadphase, physConstraintSolver, physCollisionConfiguration);

	Vector3 defaultGravity(0, -10.0f, 0); // TODO
	physDynamicsWorld->setGravity(defaultGravity);
	
	physDebugDrawer = new CollisionDebugDrawer(&dbgDrawer);
	physDynamicsWorld->setDebugDrawer(physDebugDrawer);

	physGhostCallback = new btGhostPairCallback();
	physDynamicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(physGhostCallback);
	
	m_frustumMgr = new FrustumMgr;
	
	m_sceneGraph = new SceneGraph(SCENEGRAPH_SIZE, this);

	m_entityMgr = new EntityMgr(ENTITY_COUNT);
	m_typeMgr = new TypeMgr(this, ENTITY_COUNT);
	m_nameMgr = new NameMgr(ENTITY_COUNT);

	m_transformSystem = new TransformSystem(this, ENTITY_COUNT);
	m_visibilitySystem = new VisibilitySystem(this, ENTITY_COUNT);
	m_earlyVisibilitySystem = new EarlyVisibilitySystem(this, ENTITY_COUNT);
	m_scriptSystem = new ScriptSystem(this, ENTITY_COUNT);
	m_collisionSystem = new CollisionSystem(this, physDynamicsWorld, ENTITY_COUNT);
	m_physicsSystem = new PhysicsSystem(this, physDynamicsWorld, ENTITY_COUNT);
	m_triggerSystem = new TriggerSystem(this, physDynamicsWorld, ENTITY_COUNT);

	m_skeletonSystem = new SkeletonSystem(this, ENTITY_COUNT);
	m_staticMeshSystem = new StaticMeshSystem(this, ENTITY_COUNT);
	m_skeletonSystem->SetStaticMeshSys(m_staticMeshSystem);

	m_cameraSystem = new CameraSystem(this, ENTITY_COUNT);
	m_controllerSystem = new ControllerSystem(this);
	m_envProbSystem = new EnvProbSystem(this, ENTITY_COUNT);
	m_shadowSystem = new ShadowSystem(this, ENTITY_COUNT);
	m_lightSystem = new LightSystem(this, ENTITY_COUNT);
	m_shadowSystem->SetLightSys(m_lightSystem);

	m_globalLightSystem = new GlobalLightSystem(this, ENTITY_COUNT);
	m_cameraSystem->SetGlobalLightSys(m_globalLightSystem);

	m_lineGeometrySystem = new LineGeometrySystem(this, ENTITY_COUNT);
}

void World::Snapshot(ScenePipeline* scene)
{
	if(!scene->IsActive())
		return;

#ifdef _DEV
	bool profiler_state = Profiler::Get()->IsRunning();
	if(profiler_state)
		Profiler::Get()->Stop();
#endif

	LocalTimer tempTimer(m_world_timer);
	tempTimer.Frame();
	m_dt = 0.0f;
	
	m_sceneGraph->Update();

	m_lightSystem->Update();
	m_shadowSystem->Update();

	m_frustumMgr->Clear();
	m_cameraSystem->RegSingle(scene->GetSceneCamera().e);

	m_globalLightSystem->Update();
	
	m_earlyVisibilitySystem->CheckEarlyVisibility();

	m_lightSystem->RegShadowMaps();
	m_globalLightSystem->RegShadowMaps();

	if(!scene->IsLighweight())
		scene->ResolveShadowmaps();

	m_envProbSystem->RegToScene();
	m_lightSystem->RegToScene();
	m_globalLightSystem->RegToScene();

	m_visibilitySystem->CheckVisibility();

	m_skeletonSystem->UpdateBuffers();
	m_staticMeshSystem->RegToDraw();
	
	m_shadowSystem->RenderShadows();
	m_globalLightSystem->RenderShadows();

	m_shadowSystem->ClearShadowsQueue();
	m_globalLightSystem->ClearShadowsQueue();

	if(scene->StartFrame(&tempTimer))
	{
		scene->OpaqueForwardStage();
		scene->OpaqueDefferedStage();
		scene->TransparentForwardStage();
		scene->HDRtoLDRStage();
		scene->EndFrame();
	}

#ifdef _DEV
	if(profiler_state)
		Profiler::Get()->Start();
#endif
}

void World::Frame()
{
	if(!b_active)
		return;

	PERF_CPU_BEGIN(_SCENE_UPDATE);

	m_world_timer.Frame();
	m_dt = m_world_timer.dt();
	frameID++;
	if( frameID == numeric_limits<uint32_t>::max() )
		frameID = 1;
	
	// start update

	if( m_mode == StateMode::LIVE )
	{
		m_scriptSystem->Update(m_dt, frameID);
		m_skeletonSystem->Animate(m_dt);
	}

	m_physicsSystem->UpdateTransformations();
	if( m_mode == StateMode::LIVE )
		m_physicsSystem->SimulateAndUpdate(m_dt);

	m_sceneGraph->Update();
	if(b_sceneGraphDbg)
		m_sceneGraph->DebugDraw(&dbgDrawer);
	
	m_collisionSystem->UpdateTransformations();
	m_triggerSystem->UpdateTransformations();

	if( m_mode == StateMode::LIVE )
		m_triggerSystem->CheckOverlaps(m_dt, frameID);

	bool somethingActive = false;
	for(auto& it: m_scenes)
		somethingActive = somethingActive || it->IsActive();

	if(!somethingActive)
	{
		dbgDrawer.Drop();
		PERF_CPU_END(_SCENE_UPDATE);
		return;
	}

	m_collisionSystem->DebugDraw();	

	m_lightSystem->Update();
	m_shadowSystem->Update();

	m_frustumMgr->Clear();
	m_cameraSystem->RegToDraw();

	m_globalLightSystem->Update();
	
	dbgDrawer.Prepare();

	m_earlyVisibilitySystem->CheckEarlyVisibility();

	m_lightSystem->RegShadowMaps();
	m_globalLightSystem->RegShadowMaps();

	for(auto it: m_scenes)
	{
		if(it->IsLighweight())
			continue;
		it->ResolveShadowmaps();
	}

	m_envProbSystem->RegToScene();
	m_lightSystem->RegToScene();
	m_globalLightSystem->RegToScene(); // TODO: now all RegToScene fill render queues endless if no scenes

	m_visibilitySystem->CheckVisibility();

	PERF_CPU_END(_SCENE_UPDATE);

	PERF_CPU_BEGIN(_SCENE_DRAW);

	m_skeletonSystem->UpdateBuffers();
	m_staticMeshSystem->RegToDraw();

	m_lineGeometrySystem->RegToDraw();

	PERF_GPU_TIMESTAMP(_SCENE_SHADOWS);
	m_shadowSystem->RenderShadows();
	m_globalLightSystem->RenderShadows();

	m_shadowSystem->ClearShadowsQueue();
	m_globalLightSystem->ClearShadowsQueue();

	for(auto& it: m_scenes)
	{
		if(it->StartFrame(&m_world_timer))
		{
			PERF_GPU_TIMESTAMP(_SCENE_FORWARD);
			it->OpaqueForwardStage(&dbgDrawer);

			PERF_GPU_TIMESTAMP(_SCENE_DEFFERED);
			it->OpaqueDefferedStage();
			
			PERF_GPU_TIMESTAMP(_SCENE_APLHA);
			it->TransparentForwardStage();
			
			PERF_GPU_TIMESTAMP(_SCENE_UI);
			if( it->UIStage() )
				dbgDrawer.Render();

			it->UIOverlayStage();
			
			PERF_GPU_TIMESTAMP(_SCENE_LDR);
			it->HDRtoLDRStage();

			it->EndFrame();
		}
	}

	dbgDrawer.Drop();

	PERF_CPU_END(_SCENE_DRAW);
}

void World::Close()
{
	BaseWorld::Close();

	_DELETE(physDebugDrawer);
}

// World ---------------------
SmallWorld::SmallWorld( uint32_t id ) : BaseWorld(id)
{
	m_earlyVisibilitySystem = nullptr;
	m_shadowSystem = nullptr;
	m_lightSystem = nullptr;
	m_globalLightSystem = nullptr;
	m_lineGeometrySystem = nullptr;

	physCollisionConfiguration = new btDefaultCollisionConfiguration();
	physCollisionDispatcher = new btCollisionDispatcher(physCollisionConfiguration);
	physBroadphase = new btDbvtBroadphase();
	physConstraintSolver = new btSequentialImpulseConstraintSolver();
	physDynamicsWorld = new btDiscreteDynamicsWorld(physCollisionDispatcher, physBroadphase, physConstraintSolver, physCollisionConfiguration);

	Vector3 defaultGravity(0, -10.0f, 0); // TODO
	physDynamicsWorld->setGravity(defaultGravity);

	physGhostCallback = new btGhostPairCallback();
	physDynamicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(physGhostCallback);

	m_frustumMgr = new FrustumMgr;
	
	m_sceneGraph = new SceneGraph(SMALL_SCENEGRAPH_SIZE, this);

	m_entityMgr = new EntityMgr(SMALL_ENTITY_COUNT);
	m_typeMgr = new TypeMgr(this, SMALL_ENTITY_COUNT);
	m_nameMgr = new NameMgr(SMALL_ENTITY_COUNT);

	m_transformSystem = new TransformSystem(this, SMALL_ENTITY_COUNT);
	m_visibilitySystem = new VisibilitySystem(this, SMALL_ENTITY_COUNT);
	m_scriptSystem = new ScriptSystem(this, SMALL_ENTITY_COUNT);
	m_collisionSystem = new CollisionSystem(this, physDynamicsWorld, SMALL_ENTITY_COUNT);
	m_physicsSystem = new PhysicsSystem(this, physDynamicsWorld, SMALL_ENTITY_COUNT);
	m_triggerSystem = new TriggerSystem(this, physDynamicsWorld, SMALL_ENTITY_COUNT);

	m_skeletonSystem = new SkeletonSystem(this, SMALL_ENTITY_COUNT);
	m_staticMeshSystem = new StaticMeshSystem(this, SMALL_ENTITY_COUNT);
	m_skeletonSystem->SetStaticMeshSys(m_staticMeshSystem);

	m_cameraSystem = new CameraSystem(this, SMALL_ENTITY_COUNT);
	m_controllerSystem = new ControllerSystem(this);
	m_envProbSystem = new EnvProbSystem(this, SMALL_ENTITY_COUNT);
}

void SmallWorld::Snapshot(ScenePipeline* scene)
{
	if(!scene->IsActive())
		return;

#ifdef _DEV
	bool profiler_state = Profiler::Get()->IsRunning();
	if(profiler_state)
		Profiler::Get()->Stop();
#endif

	LocalTimer tempTimer(m_world_timer);
	tempTimer.Frame();
	m_dt = 0.0f;
	
	m_sceneGraph->Update();
	
	m_frustumMgr->Clear();
	m_cameraSystem->RegSingle(scene->GetSceneCamera().e);
	
	m_envProbSystem->RegToScene();

	m_visibilitySystem->CheckVisibility();

	m_skeletonSystem->UpdateBuffers();
	m_staticMeshSystem->RegToDraw();
	
	if(scene->StartFrame(&tempTimer))
	{
		scene->OpaqueForwardStage();
		scene->OpaqueDefferedStage();
		scene->TransparentForwardStage();
		scene->HDRtoLDRStage();
		scene->EndFrame();
	}

#ifdef _DEV
	if(profiler_state)
		Profiler::Get()->Start();
#endif
}

void SmallWorld::Frame()
{
	if(!b_active)
		return;
	
#ifdef _DEV
	bool profiler_state = Profiler::Get()->IsRunning();
	if(profiler_state)
		Profiler::Get()->Stop();
#endif

	m_world_timer.Frame();
	m_dt = m_world_timer.dt();
	frameID++;
	if( frameID == numeric_limits<uint32_t>::max() )
		frameID = 1;
	
	// start update

	if( m_mode == StateMode::LIVE )
	{
		m_scriptSystem->Update(m_dt, frameID);
		m_skeletonSystem->Animate(m_dt);
	}

	m_physicsSystem->UpdateTransformations();
	if( m_mode == StateMode::LIVE )
		m_physicsSystem->SimulateAndUpdate(m_dt);

	m_sceneGraph->Update();
	
	m_collisionSystem->UpdateTransformations();
	m_triggerSystem->UpdateTransformations();
	
	if( m_mode == StateMode::LIVE )
		m_triggerSystem->CheckOverlaps(m_dt, frameID);

	bool somethingActive = false;
	for(auto& it: m_scenes)
		somethingActive = somethingActive || it->IsActive();

	if(somethingActive)
	{
		m_frustumMgr->Clear();
		m_cameraSystem->RegToDraw();
	
		m_envProbSystem->RegToScene();

		m_visibilitySystem->CheckVisibility();

		m_skeletonSystem->UpdateBuffers();
		m_staticMeshSystem->RegToDraw();

		for(auto& it: m_scenes)
		{
			if(it->StartFrame(&m_world_timer))
			{
				it->OpaqueForwardStage();

				it->OpaqueDefferedStage();
			
				it->TransparentForwardStage();

				it->UIStage();
				it->UIOverlayStage();
			
				it->HDRtoLDRStage();

				it->EndFrame();
			}
		}
	}

#ifdef _DEV
	if(profiler_state)
		Profiler::Get()->Start();
#endif
}

void SmallWorld::Close()
{
	BaseWorld::Close();
}