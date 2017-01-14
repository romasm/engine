#pragma once
#include "ECS\Entity.h"
#include "RenderMgrs.h"

#define FRUSTUM_MAX_COUNT 256
#define VOLUME_MAX_COUNT 8

namespace EngineCore
{
	struct Frustum
	{
		friend class FrustumMgr;
	public:
		inline size_t get_frustum_id() const {return id;}

		BoundingFrustumEx frustum;
		BaseRenderMgr* rendermgr;
		bool is_volume;

		bitset<FRUSTUM_MAX_COUNT> bit;

		ENTITY_IN_COMPONENT
	private:	
		size_t id;
	};

	class FrustumMgr
	{
	public:
		FrustumMgr()
		{
			m_frustums.create(FRUSTUM_MAX_COUNT);

			frustums_free_list.resize(FRUSTUM_MAX_COUNT);
			for(int i=0; i<FRUSTUM_MAX_COUNT; i++)
				frustums_free_list[i] = i;
		}

		inline Frustum& GetFrustum(int id) {return m_frustums.getDataById(id);}
		
		Frustum* AddFrustum(Entity e, BoundingFrustum* frust, BaseRenderMgr* rendermgr, XMMATRIX* WV = nullptr, XMMATRIX* Proj = nullptr, bool shadow = false) 
		{
			if(frustums_free_list.size() == 0 || !frust || !rendermgr)
			{
				WRN("Frustums overflow!");
				return nullptr;
			}

			int id = int(frustums_free_list.front());
			frustums_free_list.pop_front();

			Frustum data;
			data.frustum.Create(*frust, WV, Proj);
			data.rendermgr = rendermgr;

			data.bit = 1;
			data.bit <<= id;
			data.id = id;
			data.is_volume = false;

			data.parent = e;

			m_frustums.add(id, data);

			if(!shadow)
				camDataArray.push_back(&GetFrustum(id));

			return &m_frustums.getDataById(id);
		}

		Frustum* AddFrustum(Entity e, BoundingOrientedBox* box, BaseRenderMgr* rendermgr, XMMATRIX* WV = nullptr, XMMATRIX* Proj = nullptr, bool not_camera = false, bool is_volume = false) 
		{
			if(frustums_free_list.size() == 0 || !box || !rendermgr)
			{
				WRN("Frustums overflow!");
				return nullptr;
			}

			int id = int(frustums_free_list.front());
			frustums_free_list.pop_front();

			Frustum data;
			data.frustum.Create(*box, WV, Proj);
			data.rendermgr = rendermgr;

			data.bit = 1;
			data.bit <<= id;
			data.id = id;
			data.is_volume = is_volume;

			data.parent = e;

			m_frustums.add(id, data);

			if(!not_camera)
				camDataArray.push_back(&GetFrustum(id));

			return &m_frustums.getDataById(id);
		}
		
		void Clear()
		{
			frustums_free_list.clear();
			frustums_free_list.resize(FRUSTUM_MAX_COUNT);
			for(int i=0; i<FRUSTUM_MAX_COUNT; i++)
				frustums_free_list[i] = i;

			m_frustums.clear();
			camDataArray.clear();
		}

		template<class BoundingShape>
		static float CalcScreenSize(const BoundingFrustumEx& screen, const BoundingShape& shape)
		{
			if(shape.Contains(XMLoadFloat3(&screen.GetViewPoint())))
				return 1.0f;
			
			XMFLOAT3 corners[8];
			shape.GetCorners(corners);

			XMFLOAT3 corners_vs[8];
			XMVector3TransformCoordStream(corners_vs, sizeof(XMFLOAT3), corners, sizeof(XMFLOAT3), 8, screen.GetWV());
			
			// TODO: некоректное проецирование точек за камерой
			for(uint8_t i = 0; i < 8; i++)
			{
				auto& p1 = corners_vs[i];
				if(p1.z >= 0)
					continue;

				uint8_t j = i > 3 ? i - 4 : i + 4;
				auto& p2 = corners_vs[j];
				if(p2.z < 0)
				{
					p1.z = 0;
					p2.z = 0;
					continue;
				}

				float k = - p2.z / (p1.z - p2.z);
				p1.x = p1.x + k * (p2.x - p1.x);
				p1.y = p1.y + k * (p2.y - p1.y);
				p1.z = 0.1f;
			}

			XMFLOAT3 corners_cs[8];
			XMVector3TransformCoordStream(corners_cs, sizeof(XMFLOAT3), corners_vs, sizeof(XMFLOAT3), 8, screen.GetProj());

			XMFLOAT2 p_min, p_max;
			p_min.x = p_min.y = 1.0f;
			p_max.x = p_max.y = -1.0f;
			for(uint8_t i = 0; i < 8; i++)
			{
				 p_min.x = min(p_min.x, corners_cs[i].x);
				 p_min.y = min(p_min.y, corners_cs[i].y);
				 p_max.x = max(p_max.x, corners_cs[i].x);
				 p_max.y = max(p_max.y, corners_cs[i].y);
			}

			p_min.x = max(p_min.x, -1.0f);
			p_min.y = max(p_min.y, -1.0f);
			p_max.x = min(p_max.x, 1.0f);
			p_max.y = min(p_max.y, 1.0f);

			return (p_max.x - p_min.x) * (p_max.y - p_min.y) * 0.25f; // / 4 - screen square
		}

		SArray<Frustum*, FRUSTUM_MAX_COUNT> camDataArray;

		ComponentRArray<Frustum> m_frustums;

	private:
		SDeque<size_t, FRUSTUM_MAX_COUNT> frustums_free_list;
	};
}