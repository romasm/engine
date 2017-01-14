#pragma once
#include "stdafx.h"
#include "Material.h"

#define MATERIAL(name) MaterialMgr::Get()->GetMaterial(name)
#define MATERIAL_S(name) MaterialMgr::Get()->GetMaterial(string(name))
#define MATERIAL_DROP(name) MaterialMgr::Get()->DeleteMaterial(name)
#define MATERIAL_DROP_S(name) MaterialMgr::Get()->DeleteMaterial(string(name))
#define MATERIAL_PTR_DROP(mat) if(mat){MaterialMgr::Get()->DeleteMaterial(mat->GetName()); mat = nullptr;}

#define MATERIAL_INIT_COUNT 256

namespace EngineCore
{
	class MaterialMgr
	{
	public:
		MaterialMgr();
		~MaterialMgr();
		
		inline static MaterialMgr* Get(){return instance;}

		inline static bool IsNull(Material* mat){return mat == null_material;}

		Material* GetMaterial(string& name);
		void DeleteMaterial(string& name);

	private:
		Material* AddMaterialToList(string& name);
		Material* FindMaterialInList(string& name);

		struct MaterialHandle
		{
			Material* material;
			uint32_t refcount;
			string name;

			MaterialHandle()
			{
				material = nullptr;
				refcount = 0;
			}
		};

		static MaterialMgr *instance;
		static Material* null_material;

		unordered_map<string, MaterialHandle> material_map;
	};
}