#pragma once
#include "stdafx.h"
#include "Common.h"
#include "BaseMgr.h"
#include "CollisionLoader.h"

namespace EngineCore
{
	enum CollisionStorageType
	{
		LOCAL = 0,
		RESOURCE,
	};

	// TODO: overwrite Update to support scaled collisions
	class CollisionMgr : public BaseMgr<btCollisionShape, RESOURCE_MAX_COUNT>
	{
	public:
		CollisionMgr() : BaseMgr<btCollisionShape, RESOURCE_MAX_COUNT>()
		{
			null_resource = new btBoxShape( btVector3(0.5f, 0.5f, 0.5f) );
			resType = ResourceType::COLLISION;	
			resExt = EXT_COLLISION;
		}

		inline static void RescaleCollision(uint32_t origId, uint32_t newgId, onLoadCallback callback)
		{
			auto& original = instance->resource_array[origId];
			btCollisionShape* scaledCollision = CollisionLoader::copyCollision(original.resource);
			auto status = LoadingStatus::LOADED;
			if( !scaledCollision )
			{
				ERR("Cant rescale collision %s", GetName(newgId).data());
				status = LoadingStatus::FAILED;
			}
			else
			{
				string& name = GetName(newgId);
				ImportInfo info;
#ifdef _EDITOR
				info = original.impInfo;
				info.resourceName = name;
#endif
				Vector3 scale = GetScaleFromResourceId(name);
				swap(scale.z, scale.y);
				scaledCollision->setLocalScaling(scale);

				instance->OnLoad(newgId, scaledCollision, info, original.filedate);
			}

			instance->CallCallback(newgId, callback, status);
		}
		
		uint32_t AddResourceToList(string& name, bool reload, onLoadCallback callback)
		{
			if(free_ids.size() == 0)
			{
				ERR("Resources amount overflow!");
				return nullres;
			}

			uint32_t idx = free_ids.front();
			auto& handle = resource_array[idx];

			handle.name = name;
			handle.resource = null_resource;
			handle.refcount = 1;

			auto scaleStrN = name.rfind('?');
			if( scaleStrN != string::npos )
			{
#ifdef _EDITOR
				if(reload)
					handle.reloadStatus = ReloadingType::RELOAD_ALWAYS;
				else
					handle.reloadStatus = ReloadingType::RELOAD_NONE;
				handle.filedate = 0;
#endif

				resource_map.insert(make_pair(name, idx));
				free_ids.pop_front();

				string pureName = name.substr(0, scaleStrN);

				GetResource(pureName, reload, [idx, callback](uint32_t id, bool status) -> void { RescaleCollision(id, idx, callback); });
				return idx;
			}
			else
			{
				if(!FileIO::IsExist(name))
				{
#ifdef _EDITOR
					WRN("File %s doesn\'t exist, creation expected in future.", name.data());
					if(reload)
						handle.reloadStatus = ReloadingType::RELOAD_ALWAYS;
					else
						handle.reloadStatus = ReloadingType::RELOAD_ONCE;
					handle.filedate = 1;

					ResourceProcessor::Get()->QueueLoad(idx, resType, callback);
#else
					ERR("File %s doesn\'t exist", name.data());
#endif
				}
				else
				{
#ifdef _EDITOR
					if(reload)
						handle.reloadStatus = ReloadingType::RELOAD_ALWAYS;
					else
						handle.reloadStatus = ReloadingType::RELOAD_NONE;
					handle.filedate = 0;
#endif
					ResourceProcessor::Get()->QueueLoad(idx, resType, callback);
				}

				resource_map.insert(make_pair(name, idx));
				free_ids.pop_front();

				return idx;
			}
		}

		void DeleteResource(uint32_t id)
		{
			string name = GetName(id);
			auto scaleStrN = name.rfind('?');
			if( scaleStrN != string::npos )
			{
				string pureName = name.substr(0, scaleStrN);
				BaseMgr<btCollisionShape, RESOURCE_MAX_COUNT>::DeleteResourceByName(pureName);
			}

			BaseMgr<btCollisionShape, RESOURCE_MAX_COUNT>::DeleteResource(id);
		}

		inline static string SetScaleToResourceId(string& name, Vector3& scale)
		{
			auto scaleStrN = name.rfind('?');
			if( scaleStrN == string::npos )
				return name + "?" + Vector3ToString(scale);
			else
				return name.substr(0, scaleStrN) + "?" + Vector3ToString(scale);
		}

		inline static Vector3 GetScaleFromResourceId(string& name)
		{
			auto scaleStrN = name.rfind('?');
			if( scaleStrN == string::npos )
				return Vector3(1.0f);
			else
				return CharToVector3( (char*)name.substr(scaleStrN + 1).data() );
		}

		inline static CollisionMgr* Get(){return (CollisionMgr*)BaseMgr<btCollisionShape, RESOURCE_MAX_COUNT>::Get();}

		virtual void ResourceDeallocate(btCollisionShape*& resource)
		{
			if(!resource)
				return;

			if( resource->getShapeType() == COMPOUND_SHAPE_PROXYTYPE )
			{
				btCompoundShape* cShape = (btCompoundShape*)resource;

				btCompoundShapeChild* childrenPtrs = nullptr;

				auto childrenCount = cShape->getNumChildShapes();
				if(childrenCount > 0)
					childrenPtrs = cShape->getChildList();

				while( childrenCount > 0 )
				{
					auto child = childrenPtrs->m_childShape;
					cShape->removeChildShapeByIndex(0);
					_DELETE(child);
					childrenCount--;
				}
			}

			_DELETE(resource);
		}
	};
}