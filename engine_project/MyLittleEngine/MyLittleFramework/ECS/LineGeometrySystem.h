#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "VisibilitySystem.h"
#include "EarlyVisibilitySystem.h"

#define LG_SPLINE_MAX 1024
#define LG_SPHERE_SUBD 32
#define LG_POINT_SIZE 0.1f

#define LG_SHADER PATH_SHADERS "objects/editor/line_geometry"
#define LG_SHADER_SPHERE PATH_SHADERS "objects/editor/line_geometry_sphere"

#define LG_MAT "$" LG_SHADER
#define LG_MAT_SPHERE "$" LG_SHADER_SPHERE

namespace EngineCore
{
	struct LineGeometryBuffer
	{
		XMMATRIX world;
	};

	struct LineGeometryVertex
	{
		XMFLOAT3 pos;
	};

	enum LineGeometryTypes
	{
		LG_LINE = 0,
		LG_BOX = 1,
		LG_SPLINE = 2,
		LG_POINT = 3,
		LG_SPHERE = 4
	};

	struct LineGeometryComponent
	{
		ENTITY_IN_COMPONENT

		bool dirty;

		bool active;

		LineGeometryTypes type;

		ID3D11Buffer *vertexBuffer; 
		ID3D11Buffer *indexBuffer;
		UINT index_count;
		ID3D11Buffer *constantBuffer;
		Material *material;
	};

	class BaseWorld;

	class LineGeometrySystem
	{
	public:
		LineGeometrySystem(BaseWorld* w, uint32_t maxCount);
		~LineGeometrySystem()
		{
			for(auto& i: *components.data())
				DestroyGeometry(&i, true);
		}

		LineGeometryComponent* AddComponent(Entity e)
		{
			LineGeometryComponent* res = components.add(e.index());
			res->dirty = true;
			res->active = true;
			res->constantBuffer = nullptr;
			res->vertexBuffer = nullptr;
			res->indexBuffer = nullptr;
			res->index_count = 0;
			res->type = LG_LINE;
			res->parent = e;
			res->material = nullptr;

			res->constantBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(LineGeometryBuffer), true);
			return res;
		}
		void DeleteComponent(Entity e);
		bool HasComponent(Entity e) const {return components.has(e.index());}
		size_t ComponentsCount() {return components.dataSize();}

		LineGeometryComponent* GetComponent(Entity e)
		{
			size_t idx = components.getArrayIdx(e.index());
			if(idx == components.capacity()) return nullptr;
			return &components.getDataByArrayIdx(idx);
		}
		
		void RegToDraw();

		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		bool IsActive(Entity e);
		bool SetActive(Entity e, bool active);

		bool SetFromVis(Entity e, bool forceEVS = false);

		bool SetLine(Entity e, XMFLOAT3 p1, XMFLOAT3 p2);

		bool SetBox(Entity e, BoundingBox box);
		bool SetBox(Entity e, BoundingOrientedBox box);
		bool SetBox(Entity e, BoundingFrustum box);

		bool SetSpline(Entity e, XMFLOAT3* p, UINT size);

		bool SetPoint(Entity e);
		bool SetSphere(Entity e, float radius);

		bool SetColor(Entity e, XMFLOAT4 color);

		LineGeometryTypes GetType(Entity e);

		void _AddComponent(Entity e){AddComponent(e);}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<LineGeometrySystem>("LineGeometrySystem")
					.addFunction("IsActive", &LineGeometrySystem::IsActive)
					.addFunction("SetActive", &LineGeometrySystem::SetActive)

					.addFunction("SetFromVis", &LineGeometrySystem::SetFromVis)
					.addFunction("SetSphere", &LineGeometrySystem::SetSphere)
					.addFunction("SetPoint", &LineGeometrySystem::SetPoint)
					.addFunction("SetColor", &LineGeometrySystem::SetColor)

					.addFunction("AddComponent", &LineGeometrySystem::_AddComponent)
					.addFunction("DeleteComponent", &LineGeometrySystem::DeleteComponent)
					.addFunction("HasComponent", &LineGeometrySystem::HasComponent)
				.endClass();
		}

	private:
		void DestroyGeometry(LineGeometryComponent* comp, bool delete_all);

		ComponentRArray<LineGeometryComponent> components;

		TransformSystem* transformSys;
		VisibilitySystem* visibilitySys;
		EarlyVisibilitySystem* earlyVisibilitySys;
		BaseWorld* world;

		FrustumMgr* frustumMgr;
	};
}