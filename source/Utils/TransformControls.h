#pragma once
#include "stdafx.h"
#include "Common.h"
#include "MeshMgr.h"
#include "Material.h"
#include "Frustum.h"
#include "CameraSystem.h"
#include "TransformSystem.h"

// TODO: REMOVE from core code

namespace EngineCore
{
#define MESH_ARROW_X PATH_SYS_MESHES "editor/move_arrow_x" EXT_MESH
#define MESH_ARROW_Y PATH_SYS_MESHES "editor/move_arrow_y" EXT_MESH
#define MESH_ARROW_Z PATH_SYS_MESHES "editor/move_arrow_z" EXT_MESH
#define MESH_BOX_X PATH_SYS_MESHES "editor/scale_arrow_x" EXT_MESH
#define MESH_BOX_Y PATH_SYS_MESHES "editor/scale_arrow_y" EXT_MESH
#define MESH_BOX_Z PATH_SYS_MESHES "editor/scale_arrow_z" EXT_MESH
#define MESH_BOX_ALL PATH_SYS_MESHES "editor/scale_center" EXT_MESH
#define MESH_ROT_X PATH_SYS_MESHES "editor/rotate_x" EXT_MESH
#define MESH_ROT_Y PATH_SYS_MESHES "editor/rotate_y" EXT_MESH
#define MESH_ROT_Z PATH_SYS_MESHES "editor/rotate_z" EXT_MESH
#define MESH_ROT_ALL PATH_SYS_MESHES "editor/rotate_center" EXT_MESH
#define MESH_PLANE_XY PATH_SYS_MESHES "editor/move_square_xy" EXT_MESH
#define MESH_PLANE_XZ PATH_SYS_MESHES "editor/move_square_xz" EXT_MESH
#define MESH_PLANE_YZ PATH_SYS_MESHES "editor/move_square_yz" EXT_MESH

#define MATERIAL_ALL PATH_SYS_MESHES "editor/arrow_all" EXT_MATERIAL
#define MATERIAL_X PATH_SYS_MESHES "editor/arrow_x" EXT_MATERIAL
#define MATERIAL_Y PATH_SYS_MESHES "editor/arrow_y" EXT_MATERIAL
#define MATERIAL_Z PATH_SYS_MESHES "editor/arrow_z" EXT_MATERIAL
#define MATERIAL_XY PATH_SYS_MESHES "editor/plane_xy" EXT_MATERIAL
#define MATERIAL_XZ PATH_SYS_MESHES "editor/plane_xz" EXT_MATERIAL
#define MATERIAL_YZ PATH_SYS_MESHES "editor/plane_yz" EXT_MATERIAL
#define MATERIAL_ROT_X PATH_SYS_MESHES "editor/rot_x" EXT_MATERIAL
#define MATERIAL_ROT_Y PATH_SYS_MESHES "editor/rot_y" EXT_MATERIAL
#define MATERIAL_ROT_Z PATH_SYS_MESHES "editor/rot_z" EXT_MATERIAL

#define TC_COLOR_ALL Vector4(0.9f, 0.9f, 0.9f, 1.0f)
#define TC_COLOR_X Vector4(1.0f, 0.0f, 0.0f, 1.0f)
#define TC_COLOR_Y Vector4(0.0f, 1.0f, 0.0f, 1.0f)
#define TC_COLOR_Z Vector4(0.0f, 0.0f, 1.0f, 1.0f)
#define TC_COLOR_XY Vector4(0.7f, 0.7f, 0.0f, 1.0f)
#define TC_COLOR_XZ Vector4(0.7f, 0.0f, 0.7f, 1.0f)
#define TC_COLOR_YZ Vector4(0.0f, 0.7f, 0.7f, 1.0f)
#define TC_COLOR_HOVER Vector4(1.0f, 0.5f, 0.0f, 1.0f)

#define TC_MAX_CAMS 10

#define TC_SCALE_MUL 0.12f

#define TC_ROT_RADIAL_COLLISION_MAX 1.06f
#define TC_ROT_RADIAL_COLLISION_MIN 0.78f

#define TC_ROT_RADIAL_COLLISION_MAX_SQ TC_ROT_RADIAL_COLLISION_MAX * TC_ROT_RADIAL_COLLISION_MAX
#define TC_ROT_RADIAL_COLLISION_MIN_SQ TC_ROT_RADIAL_COLLISION_MIN * TC_ROT_RADIAL_COLLISION_MIN

	class World;

	class TransformControls
	{
	public:

		enum eModeType
		{
			E_PIVOT = 0,
			E_MOVE = 1,
			E_ROT = 2,
			E_SCALE = 3
		};

		enum eDirections
		{
			E_TRANSFORM_NOPE = 0,
			E_TRANSFORM_X,
			E_TRANSFORM_Y,
			E_TRANSFORM_Z,
			E_TRANSFORM_XY,
			E_TRANSFORM_XZ,
			E_TRANSFORM_YZ,
			E_TRANSFORM_ALL
		};

		TransformControls(World* wrd);
		~TransformControls();

		void SetTransformation(XMMATRIX transform)
		{
			XMVECTOR pos, rot, scale;
			if(!XMMatrixDecompose(&scale, &rot, &pos, transform))
				return;
			m_pos = pos;
			if(isLocal)
				m_transformation = XMMatrixRotationQuaternion(rot);
			else
				m_transformation = XMMatrixIdentity();
			m_transformation *= XMMatrixTranslationFromVector(pos);
		}

		void ApplyMove(Vector3 move, Entity e);
		void ApplyRot(Vector4 rot, Entity e);
		void ApplyScale(Vector4 scale, Entity e);

		void SetFromEntity(Entity e)
		{SetTransformation(transformSys->GetTransformW(e));}

		void _SetMode(uint32_t type) {mode = eModeType(type);}
		void SetMode(eModeType type) {mode = type;}
		void SetCoordStyleLocal(bool local) {isLocal = local;}

		inline uint32_t _GetMode() const {return uint32_t(mode);}
		inline eModeType GetMode() const {return mode;}
		inline bool IsCoordStyleLocal() const {return isLocal;}

		void SetScale(float scale) {m_scale = scale;}
		float GetScale() const {return m_scale;}

		void RegToDraw();

		bool CheckHover(Vector3 HoverRay, Entity cam);
		void Unhover() {b_hover = E_TRANSFORM_NOPE;}
		Vector3 CalcMove(Vector3 Ray, Vector3 PrevRay, Entity cam);
		Vector4 CalcScale(Vector3 Ray, Vector3 PrevRay, Entity cam);
		Vector4 CalcRot(Vector3 Ray, Vector3 PrevRay, Entity cam);

		void Activate() {active = true;}
		void Deactivate() {active = false;}

		ALIGNED_ALLOCATION

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<TransformControls>("TransformControlsOld")
					.addFunction("SetTransformation", &TransformControls::SetTransformation)
					.addFunction("SetFromEntity", &TransformControls::SetFromEntity)
					.addProperty("mode", &TransformControls::_GetMode, &TransformControls::_SetMode)				
					.addProperty("islocal", &TransformControls::IsCoordStyleLocal, &TransformControls::SetCoordStyleLocal)	
					.addProperty("scale", &TransformControls::GetScale, &TransformControls::SetScale)	
					.addFunction("CheckHover", &TransformControls::CheckHover)
					.addFunction("Unhover", &TransformControls::Unhover)
					.addFunction("CalcMove", &TransformControls::CalcMove)
					.addFunction("CalcScale", &TransformControls::CalcScale)
					.addFunction("CalcRot", &TransformControls::CalcRot)
					.addFunction("Activate", &TransformControls::Activate)
					.addFunction("Deactivate", &TransformControls::Deactivate)
					.addFunction("ApplyMove", &TransformControls::ApplyMove)
					.addFunction("ApplyRot", &TransformControls::ApplyRot)
					.addFunction("ApplyScale", &TransformControls::ApplyScale)
				.endClass();
		}

	private:
		void calcSreenScaleMat(const Vector3& campos, uint32_t id);

		uint32_t arrow_x;
		uint32_t arrow_y;
		uint32_t arrow_z;
		uint32_t box_x;
		uint32_t box_y;
		uint32_t box_z;
		uint32_t box_all;
		uint32_t plane_xy;
		uint32_t plane_xz;
		uint32_t plane_yz;
		uint32_t rot_x;
		uint32_t rot_y;
		uint32_t rot_z;
		uint32_t rot_all;

		Material* mat_axis_x;
		Material* mat_axis_y;
		Material* mat_axis_z;
		Material* mat_plane_xy;
		Material* mat_plane_xz;
		Material* mat_plane_yz;
		Material* mat_rot_x;
		Material* mat_rot_y;
		Material* mat_rot_z;
		Material* mat_all;

		bool isLocal;
		Vector3 m_pos;
		XMMATRIX m_transformation;

		float m_scale;

		eDirections b_hover;
		eModeType mode;
		bool active;

		struct
		{
			ID3D11Buffer* constantBuffer;
			XMMATRIX actualTransform;
			float scale;
		} matricies[TC_MAX_CAMS];
		int frust_ids[FRUSTUM_MAX_COUNT];

		FrustumMgr* frustumMgr;
		CameraSystem* camSys;
		TransformSystem* transformSys;
	};
}
