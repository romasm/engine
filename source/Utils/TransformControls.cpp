#include "stdafx.h"
#include "TransformControls.h"
#include "Render.h"
#include "World.h"
#include "MaterialMgr.h"

using namespace EngineCore;

TransformControls::TransformControls(World* wrd)
{
	frustumMgr = wrd->GetFrustumMgr();

	camSys = wrd->GetCameraSystem();
	transformSys = wrd->GetTransformSystem();

	isLocal = false;
	m_transformation = XMMatrixIdentity();
	m_pos = XMVectorZero();
	
	arrow_x = MeshMgr::Get()->GetResource(string(MESH_ARROW_X));
	arrow_y = MeshMgr::Get()->GetResource(string(MESH_ARROW_Y));
	arrow_z = MeshMgr::Get()->GetResource(string(MESH_ARROW_Z));
	plane_xy = MeshMgr::Get()->GetResource(string(MESH_PLANE_XY));
	plane_xz = MeshMgr::Get()->GetResource(string(MESH_PLANE_XZ));
	plane_yz = MeshMgr::Get()->GetResource(string(MESH_PLANE_YZ));
	box_all = MeshMgr::Get()->GetResource(string(MESH_BOX_ALL));
	box_x = MeshMgr::Get()->GetResource(string(MESH_BOX_X));
	box_y = MeshMgr::Get()->GetResource(string(MESH_BOX_Y));
	box_z = MeshMgr::Get()->GetResource(string(MESH_BOX_Z));
	rot_all = MeshMgr::Get()->GetResource(string(MESH_ROT_ALL));
	rot_x = MeshMgr::Get()->GetResource(string(MESH_ROT_X));
	rot_y = MeshMgr::Get()->GetResource(string(MESH_ROT_Y));
	rot_z = MeshMgr::Get()->GetResource(string(MESH_ROT_Z));
	
	mat_axis_x = MATERIAL_S(MATERIAL_X);
	mat_axis_y = MATERIAL_S(MATERIAL_Y);
	mat_axis_z = MATERIAL_S(MATERIAL_Z);
	mat_all = MATERIAL_S(MATERIAL_ALL);
	mat_plane_xy = MATERIAL_S(MATERIAL_XY);
	mat_plane_xz = MATERIAL_S(MATERIAL_XZ);
	mat_plane_yz = MATERIAL_S(MATERIAL_YZ);	
	mat_rot_x = MATERIAL_S(MATERIAL_ROT_X);
	mat_rot_y = MATERIAL_S(MATERIAL_ROT_Y);
	mat_rot_z = MATERIAL_S(MATERIAL_ROT_Z);	

	for(uint i=0; i<TC_MAX_CAMS; i++)
	{
		matricies[i].constantBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(StmMatrixBuffer), true);
		matricies[i].actualTransform = XMMatrixIdentity();
	}

	for(uint i=0; i<FRUSTUM_MAX_COUNT; i++)
		frust_ids[i] = -1;

	b_hover = E_TRANSFORM_NOPE;
	mode = E_PIVOT;

	m_scale = 0.1f;

	active = false;
}

TransformControls::~TransformControls()
{
	MeshMgr::Get()->DeleteResource(arrow_x);
	MeshMgr::Get()->DeleteResource(arrow_y);
	MeshMgr::Get()->DeleteResource(arrow_z);
	MeshMgr::Get()->DeleteResource(plane_xy);
	MeshMgr::Get()->DeleteResource(plane_xz);
	MeshMgr::Get()->DeleteResource(plane_yz);
	MeshMgr::Get()->DeleteResource(box_all);
	MeshMgr::Get()->DeleteResource(box_x);
	MeshMgr::Get()->DeleteResource(box_y);
	MeshMgr::Get()->DeleteResource(box_z);
	MeshMgr::Get()->DeleteResource(rot_all);
	MeshMgr::Get()->DeleteResource(rot_x);
	MeshMgr::Get()->DeleteResource(rot_y);
	MeshMgr::Get()->DeleteResource(rot_z);

	MATERIAL_PTR_DROP(mat_axis_x);
	MATERIAL_PTR_DROP(mat_axis_y);
	MATERIAL_PTR_DROP(mat_axis_z);
	MATERIAL_PTR_DROP(mat_all);
	MATERIAL_PTR_DROP(mat_plane_xy);
	MATERIAL_PTR_DROP(mat_plane_xz);
	MATERIAL_PTR_DROP(mat_plane_yz);
	MATERIAL_PTR_DROP(mat_rot_x);
	MATERIAL_PTR_DROP(mat_rot_y);
	MATERIAL_PTR_DROP(mat_rot_z);

	for(uint i=0; i<TC_MAX_CAMS; i++)
		_RELEASE(matricies[i].constantBuffer);
}

void TransformControls::calcSreenScaleMat(const Vector3& campos, uint id)
{
	Vector3 v_dist = campos - m_pos;
	matricies[id].scale = v_dist.Length() * m_scale;
	matricies[id].actualTransform = XMMatrixScaling(matricies[id].scale, matricies[id].scale, matricies[id].scale) * m_transformation;

	StmMatrixBuffer mb;
	mb.world = XMMatrixTranspose(matricies[id].actualTransform);
	mb.norm = XMMatrixIdentity();
	Render::UpdateDynamicResource(matricies[id].constantBuffer, (void*)&mb, sizeof(StmMatrixBuffer));
}

void TransformControls::RegToDraw()
{
	if(!active || mode == E_PIVOT)
		return;

	uint frustCount = 0;
	switch (mode)
	{
	case TransformControls::E_MOVE:
		for(auto f: frustumMgr->camDataArray)
		{
			if(frustCount >= TC_MAX_CAMS)
				break;
			auto cam = ((SceneRenderMgr*)f->rendermgr)->GetCurrentCamera();
			calcSreenScaleMat(cam->camPos, frustCount);
			frust_ids[cam->frust_id] = frustCount;

			mat_axis_x->SetVector(TC_COLOR_X, 0, SHADER_PS);
			mat_axis_y->SetVector(TC_COLOR_Y, 0, SHADER_PS);
			mat_axis_z->SetVector(TC_COLOR_Z, 0, SHADER_PS);
			mat_plane_xy->SetVector(TC_COLOR_XY, 0, SHADER_PS);
			mat_plane_xz->SetVector(TC_COLOR_XZ, 0, SHADER_PS);
			mat_plane_yz->SetVector(TC_COLOR_YZ, 0, SHADER_PS);
			switch (b_hover)
			{
			case TransformControls::E_TRANSFORM_NOPE:
				break;
			case TransformControls::E_TRANSFORM_X:
				mat_axis_x->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			case TransformControls::E_TRANSFORM_Y:
				mat_axis_y->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			case TransformControls::E_TRANSFORM_Z:
				mat_axis_z->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			case TransformControls::E_TRANSFORM_XY:
				mat_plane_xy->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			case TransformControls::E_TRANSFORM_XZ:
				mat_plane_xz->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			case TransformControls::E_TRANSFORM_YZ:
				mat_plane_yz->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			case TransformControls::E_TRANSFORM_ALL:
				break;
			}

			auto arrow_x_mesh = MeshMgr::GetResourcePtr(arrow_x);
			auto arrow_y_mesh = MeshMgr::GetResourcePtr(arrow_y);
			auto arrow_z_mesh = MeshMgr::GetResourcePtr(arrow_z);
			auto arrow_xy_mesh = MeshMgr::GetResourcePtr(plane_xy);
			auto arrow_xz_mesh = MeshMgr::GetResourcePtr(plane_xz);
			auto arrow_yz_mesh = MeshMgr::GetResourcePtr(plane_yz);

			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_x_mesh->indexBuffers[0].size, arrow_x_mesh->vertexBuffers[0].buffer, arrow_x_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_axis_x, Vector3(XMVector3TransformCoord(XMLoadFloat3(&arrow_x_mesh->box.Center), m_transformation)));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_y_mesh->indexBuffers[0].size, arrow_y_mesh->vertexBuffers[0].buffer, arrow_y_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_axis_y, Vector3(XMVector3TransformCoord(XMLoadFloat3(&arrow_y_mesh->box.Center), m_transformation)));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_z_mesh->indexBuffers[0].size, arrow_z_mesh->vertexBuffers[0].buffer, arrow_z_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_axis_z, Vector3(XMVector3TransformCoord(XMLoadFloat3(&arrow_z_mesh->box.Center), m_transformation)));

			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_xy_mesh->indexBuffers[0].size, arrow_xy_mesh->vertexBuffers[0].buffer, arrow_xy_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_plane_xy, Vector3(XMVector3TransformCoord(XMLoadFloat3(&arrow_xy_mesh->box.Center), m_transformation)));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_xz_mesh->indexBuffers[0].size, arrow_xz_mesh->vertexBuffers[0].buffer, arrow_xz_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_plane_xz, Vector3(XMVector3TransformCoord(XMLoadFloat3(&arrow_xz_mesh->box.Center), m_transformation)));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_yz_mesh->indexBuffers[0].size, arrow_yz_mesh->vertexBuffers[0].buffer, arrow_yz_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_plane_yz, Vector3(XMVector3TransformCoord(XMLoadFloat3(&arrow_yz_mesh->box.Center), m_transformation)));
	
			frustCount++;
		}
		break;
	case TransformControls::E_ROT:
		for(auto f: frustumMgr->camDataArray)
		{
			if(frustCount >= TC_MAX_CAMS)
				break;
			auto cam = ((SceneRenderMgr*)f->rendermgr)->GetCurrentCamera();
			calcSreenScaleMat(cam->camPos, frustCount);
			frust_ids[cam->frust_id] = frustCount;

			mat_all->SetVector(TC_COLOR_ALL, 0, SHADER_PS);
			mat_rot_x->SetVector(TC_COLOR_X, 0, SHADER_PS);
			mat_rot_y->SetVector(TC_COLOR_Y, 0, SHADER_PS);
			mat_rot_z->SetVector(TC_COLOR_Z, 0, SHADER_PS);
			switch (b_hover)
			{
			case TransformControls::E_TRANSFORM_NOPE:
			case TransformControls::E_TRANSFORM_XY:
			case TransformControls::E_TRANSFORM_XZ:
			case TransformControls::E_TRANSFORM_YZ:
				break;
			case TransformControls::E_TRANSFORM_Z:
				mat_rot_z->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			case TransformControls::E_TRANSFORM_Y:
				mat_rot_y->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			case TransformControls::E_TRANSFORM_X:
				mat_rot_x->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			case TransformControls::E_TRANSFORM_ALL:
				mat_all->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			}

			auto rot_x_mesh = MeshMgr::GetResourcePtr(rot_x);
			auto rot_y_mesh = MeshMgr::GetResourcePtr(rot_y);
			auto rot_z_mesh = MeshMgr::GetResourcePtr(rot_z);
			auto rot_all_mesh = MeshMgr::GetResourcePtr(rot_all);

			((SceneRenderMgr*)f->rendermgr)->RegMesh(rot_x_mesh->indexBuffers[0].size, rot_x_mesh->vertexBuffers[0].buffer, rot_x_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_rot_x, Vector3(XMVector3TransformCoord(XMLoadFloat3(&rot_x_mesh->box.Center), m_transformation)));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(rot_y_mesh->indexBuffers[0].size, rot_y_mesh->vertexBuffers[0].buffer, rot_y_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_rot_y, Vector3(XMVector3TransformCoord(XMLoadFloat3(&rot_y_mesh->box.Center), m_transformation)));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(rot_z_mesh->indexBuffers[0].size, rot_z_mesh->vertexBuffers[0].buffer, rot_z_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_rot_z, Vector3(XMVector3TransformCoord(XMLoadFloat3(&rot_z_mesh->box.Center), m_transformation)));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(rot_all_mesh->indexBuffers[0].size, rot_all_mesh->vertexBuffers[0].buffer, rot_all_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_all, Vector3(XMVector3TransformCoord(XMLoadFloat3(&rot_all_mesh->box.Center), m_transformation)));
		
			frustCount++;
		}
		break;
	case TransformControls::E_SCALE:
		for(auto f: frustumMgr->camDataArray)
		{
			if(frustCount >= TC_MAX_CAMS)
				break;
			auto cam = ((SceneRenderMgr*)f->rendermgr)->GetCurrentCamera();
			calcSreenScaleMat(cam->camPos, frustCount);
			frust_ids[cam->frust_id] = frustCount;

			mat_all->SetVector(TC_COLOR_ALL, 0, SHADER_PS);
			mat_axis_x->SetVector(TC_COLOR_X, 0, SHADER_PS);
			mat_axis_y->SetVector(TC_COLOR_Y, 0, SHADER_PS);
			mat_axis_z->SetVector(TC_COLOR_Z, 0, SHADER_PS);
			mat_plane_xy->SetVector(TC_COLOR_XY, 0, SHADER_PS);
			mat_plane_xz->SetVector(TC_COLOR_XZ, 0, SHADER_PS);
			mat_plane_yz->SetVector(TC_COLOR_YZ, 0, SHADER_PS);
			switch (b_hover)
			{
			case TransformControls::E_TRANSFORM_NOPE:
				break;
			case TransformControls::E_TRANSFORM_X:
				mat_axis_x->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			case TransformControls::E_TRANSFORM_Y:
				mat_axis_y->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			case TransformControls::E_TRANSFORM_Z:
				mat_axis_z->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			case TransformControls::E_TRANSFORM_XY:
				mat_plane_xy->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			case TransformControls::E_TRANSFORM_XZ:
				mat_plane_xz->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			case TransformControls::E_TRANSFORM_YZ:
				mat_plane_yz->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			case TransformControls::E_TRANSFORM_ALL:
				mat_all->SetVector(TC_COLOR_HOVER, 0, SHADER_PS);
				break;
			}

			auto box_x_mesh = MeshMgr::GetResourcePtr(box_x);
			auto box_y_mesh = MeshMgr::GetResourcePtr(box_y);
			auto box_z_mesh = MeshMgr::GetResourcePtr(box_z);
			auto box_all_mesh = MeshMgr::GetResourcePtr(box_all);
			auto arrow_xy_mesh = MeshMgr::GetResourcePtr(plane_xy);
			auto arrow_xz_mesh = MeshMgr::GetResourcePtr(plane_xz);
			auto arrow_yz_mesh = MeshMgr::GetResourcePtr(plane_yz);

			((SceneRenderMgr*)f->rendermgr)->RegMesh(box_x_mesh->indexBuffers[0].size, box_x_mesh->vertexBuffers[0].buffer, box_x_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_axis_x, Vector3(XMVector3TransformCoord(XMLoadFloat3(&box_x_mesh->box.Center), m_transformation)));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(box_y_mesh->indexBuffers[0].size, box_y_mesh->vertexBuffers[0].buffer, box_y_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_axis_y, Vector3(XMVector3TransformCoord(XMLoadFloat3(&box_y_mesh->box.Center), m_transformation)));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(box_z_mesh->indexBuffers[0].size, box_z_mesh->vertexBuffers[0].buffer, box_z_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_axis_z, Vector3(XMVector3TransformCoord(XMLoadFloat3(&box_z_mesh->box.Center), m_transformation)));

			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_xy_mesh->indexBuffers[0].size, arrow_xy_mesh->vertexBuffers[0].buffer, arrow_xy_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_plane_xy, Vector3(XMVector3TransformCoord(XMLoadFloat3(&arrow_xy_mesh->box.Center), m_transformation)));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_xz_mesh->indexBuffers[0].size, arrow_xz_mesh->vertexBuffers[0].buffer, arrow_xz_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_plane_xz, Vector3(XMVector3TransformCoord(XMLoadFloat3(&arrow_xz_mesh->box.Center), m_transformation)));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_yz_mesh->indexBuffers[0].size, arrow_yz_mesh->vertexBuffers[0].buffer, arrow_yz_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_plane_yz, Vector3(XMVector3TransformCoord(XMLoadFloat3(&arrow_yz_mesh->box.Center), m_transformation)));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(box_all_mesh->indexBuffers[0].size, box_all_mesh->vertexBuffers[0].buffer, box_all_mesh->indexBuffers[0].buffer, matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_all, Vector3(XMVector3TransformCoord(XMLoadFloat3(&box_all_mesh->box.Center), m_transformation)));
		
			frustCount++;
		}
		break;
	}
}

Vector3 TransformControls::CalcMove(Vector3 Ray, Vector3 PrevRay, Entity cam)
{
	Vector3 res = Vector3(0,0,0);
	if(!active || b_hover == E_TRANSFORM_NOPE || mode != E_MOVE)
		return res;

	XMVECTOR pos, rot, scale;
	if(!XMMatrixDecompose(&scale, &rot, &pos, m_transformation))
		return res;

	auto camComp = camSys->GetComponent(cam);

	PrevRay.Normalize();
	Ray.Normalize();

	Vector3 axis;
	switch (b_hover)
	{
	case TransformControls::E_TRANSFORM_X: axis = Vector3(1.0f,0.0f,0.0f);
		break;
	case TransformControls::E_TRANSFORM_Y: axis = Vector3(0.0f,1.0f,0.0f);
		break;
	case TransformControls::E_TRANSFORM_Z: axis = Vector3(0.0f,0.0f,1.0f);
		break;
	case TransformControls::E_TRANSFORM_XY: axis = Vector3(0.0f,0.0f,1.0f);
		break;
	case TransformControls::E_TRANSFORM_XZ: axis = Vector3(0.0f,1.0f,0.0f);
		break;
	case TransformControls::E_TRANSFORM_YZ: axis = Vector3(1.0f,0.0f,0.0f);
		break;
	}

	switch (b_hover)
	{
	case TransformControls::E_TRANSFORM_X:
	case TransformControls::E_TRANSFORM_Y:
	case TransformControls::E_TRANSFORM_Z:
		{
			Vector3 dist = m_pos - camComp->camPos;
			dist.Normalize();
			Vector3 dir = XMVector3Transform(axis, XMMatrixRotationQuaternion(rot));

			Vector3 norm = dir.Cross( dir.Cross(dist) );
			norm.Normalize();
			Vector3 plane = XMPlaneFromPointNormal(m_pos, norm);

			Vector3 point_from = XMPlaneIntersectLine(plane, camComp->camPos, PrevRay * camComp->far_clip);
			Vector3 point_to = XMPlaneIntersectLine(plane, camComp->camPos, Ray * camComp->far_clip);
			Vector3 move_vect = point_to - point_from;
			res = move_vect.Dot(dir) * dir;
		}
		break;
	case TransformControls::E_TRANSFORM_XY:
	case TransformControls::E_TRANSFORM_XZ:
	case TransformControls::E_TRANSFORM_YZ:
		{
			Vector3 norm = XMVector3Transform(axis, XMMatrixRotationQuaternion(rot));
			Vector3 plane = XMPlaneFromPointNormal(m_pos, norm);

			Vector3 point_from = XMPlaneIntersectLine(plane, camComp->camPos, PrevRay * camComp->far_clip);
			Vector3 point_to = XMPlaneIntersectLine(plane, camComp->camPos, Ray * camComp->far_clip);
			res = point_to - point_from;
		}
		break;
	}

	return res;
}

Vector4 TransformControls::CalcScale(Vector3 Ray, Vector3 PrevRay, Entity cam)
{
	Vector4 res = Vector4(0,0,0,0);
	if(!active || b_hover == E_TRANSFORM_NOPE || mode != E_SCALE)
		return res;

	XMVECTOR pos, rot, scale;
	if(!XMMatrixDecompose(&scale, &rot, &pos, m_transformation))
		return res;

	auto camComp = camSys->GetComponent(cam);

	PrevRay.Normalize();
	Ray.Normalize();

	Vector3 axis;
	Vector3 plane_axis;
	switch (b_hover)
	{
	case TransformControls::E_TRANSFORM_X: axis = Vector3(1.0f,0.0f,0.0f); 
		res.x = 1;
		break;
	case TransformControls::E_TRANSFORM_Y: axis = Vector3(0.0f,1.0f,0.0f);
		res.y = 1;
		break;
	case TransformControls::E_TRANSFORM_Z: axis = Vector3(0.0f,0.0f,1.0f); 
		res.z = 1;
		break;
	case TransformControls::E_TRANSFORM_XY: plane_axis = Vector3(0.0f,0.0f,1.0f); axis = Vector3(1.0f,1.0f,0.0f);
		res.x = 1; res.y = 1;
		break;
	case TransformControls::E_TRANSFORM_XZ: plane_axis = Vector3(0.0f,1.0f,0.0f); axis = Vector3(1.0f,0.0f,1.0f); 
		res.x = 1; res.z = 1;
		break;
	case TransformControls::E_TRANSFORM_YZ: plane_axis = Vector3(1.0f,0.0f,0.0f); axis = Vector3(0.0f,1.0f,1.0f);
		res.y = 1; res.z = 1;
		break;
	case TransformControls::E_TRANSFORM_ALL:
		res.x = 1; res.y = 1; res.z = 1;
		break;
	}

	switch (b_hover)
	{
	case TransformControls::E_TRANSFORM_X:
	case TransformControls::E_TRANSFORM_Y:
	case TransformControls::E_TRANSFORM_Z:
		{
			Vector3 dist = m_pos - camComp->camPos;
			dist.Normalize();
			Vector3 dir = XMVector3Transform(axis, XMMatrixRotationQuaternion(rot));

			Vector3 norm = dir.Cross(dir.Cross(dist));
			norm.Normalize();
			Vector3 plane = XMPlaneFromPointNormal(m_pos, norm);

			Vector3 point_from = XMPlaneIntersectLine(plane, camComp->camPos, PrevRay * camComp->far_clip);
			Vector3 point_to = XMPlaneIntersectLine(plane, camComp->camPos, Ray * camComp->far_clip);
			Vector3 move_vect = point_to - point_from;
			res.w = move_vect.Dot(dir);
		}
		break;
	case TransformControls::E_TRANSFORM_XY:
	case TransformControls::E_TRANSFORM_XZ:
	case TransformControls::E_TRANSFORM_YZ:
		{
			Vector3 norm = XMVector3Transform(plane_axis, XMMatrixRotationQuaternion(rot));
			norm.Normalize();
			Vector3 axis_proj = XMVector3Transform(axis, XMMatrixRotationQuaternion(rot));
			axis_proj.Normalize();
			Vector3 plane = XMPlaneFromPointNormal(m_pos, norm);

			Vector3 point_from = XMPlaneIntersectLine(plane, camComp->camPos, PrevRay * camComp->far_clip);
			Vector3 point_to = XMPlaneIntersectLine(plane, camComp->camPos, Ray * camComp->far_clip);
			Vector3 move_vect = point_to - point_from;
			res.w = move_vect.Dot(axis_proj);
		}
		break;
	case TransformControls::E_TRANSFORM_ALL:
		{
			Vector3 plane = XMPlaneFromPointNormal(m_pos, XMVector3Normalize(-camComp->camLookDir));
			Vector3 point_from = XMPlaneIntersectLine(plane, camComp->camPos, PrevRay * camComp->far_clip);
			Vector3 point_to = XMPlaneIntersectLine(plane, camComp->camPos, Ray * camComp->far_clip);
			Vector3 move_vect = point_to - point_from;
			res.w = move_vect.Dot( XMVector3Normalize(camComp->camUp.Cross(camComp->camLookDir) + camComp->camUp) );
		}
		break;
	}

	res.w *= TC_SCALE_MUL;

	return res;
}

Vector4 TransformControls::CalcRot(Vector3 Ray, Vector3 PrevRay, Entity cam)
{
	Vector4 res = Vector4(1,0,0,0);
	if(!active || b_hover == E_TRANSFORM_NOPE || mode != E_ROT)
		return res;

	XMVECTOR pos, rot, scale;
	if(!XMMatrixDecompose(&scale, &rot, &pos, m_transformation))
		return res;

	auto camComp = camSys->GetComponent(cam);

	PrevRay.Normalize();
	Ray.Normalize();

	Vector3 axis;
	switch (b_hover)
	{
	case TransformControls::E_TRANSFORM_X: axis = Vector3(1.0f,0.0f,0.0f);
		break;
	case TransformControls::E_TRANSFORM_Y: axis = Vector3(0.0f,1.0f,0.0f);
		break;
	case TransformControls::E_TRANSFORM_Z: axis = Vector3(0.0f,0.0f,1.0f);
		break;
	}

	switch (b_hover)
	{
	case TransformControls::E_TRANSFORM_X:
	case TransformControls::E_TRANSFORM_Y:
	case TransformControls::E_TRANSFORM_Z:
		{
			Vector3 norm = XMVector3Transform(axis, XMMatrixRotationQuaternion(rot));
			Vector3 plane = XMPlaneFromPointNormal(m_pos, norm);

			Vector3 point_from = XMPlaneIntersectLine(plane, camComp->camPos, PrevRay * camComp->far_clip);
			Vector3 point_to = XMPlaneIntersectLine(plane, camComp->camPos, Ray * camComp->far_clip);
			point_from = point_from - m_pos;
			point_to = point_to - m_pos;

			float angle = XMVectorGetX(XMVector3AngleBetweenVectors(point_from, point_to));
			Vector3 rot_dir_test = XMVector3Cross(point_from, point_to);
			if(XMVectorGetX(XMVector3Dot(norm, rot_dir_test)) < 0)
				angle = -angle;

			res = Vector4(XMVectorGetX(norm), XMVectorGetY(norm), XMVectorGetZ(norm), angle);
		}
		break;
	case TransformControls::E_TRANSFORM_ALL:
		{
			Vector3 norm = XMVector3Normalize(-camComp->camLookDir);
			Vector3 plane = XMPlaneFromPointNormal(m_pos, norm);
			Vector3 point_from = XMPlaneIntersectLine(plane, camComp->camPos, PrevRay * camComp->far_clip);
			Vector3 point_to = XMPlaneIntersectLine(plane, camComp->camPos, Ray * camComp->far_clip);
			point_from = point_from - m_pos;
			point_to = point_to - m_pos;

			float angle = XMVectorGetX(XMVector3AngleBetweenVectors(point_from, point_to));
			Vector3 rot_dir_test = XMVector3Cross(point_from, point_to);
			if(XMVectorGetX(XMVector3Dot(norm, rot_dir_test)) < 0)
				angle = -angle;
			res = Vector4(XMVectorGetX(norm), XMVectorGetY(norm), XMVectorGetZ(norm), angle);
		}
		break;
	}

	return res;
}

void TransformControls::ApplyMove(Vector3 move, Entity e)
{
	transformSys->AddPosition(e, move.x, move.y, move.z);
	SetTransformation(transformSys->GetTransformL(e)); // todo
}

void TransformControls::ApplyRot(Vector4 rot, Entity e)
{
	transformSys->AddRotation(e, Vector3(rot.x, rot.y, rot.z), rot.w);
	SetTransformation(transformSys->GetTransformL(e)); // todo
}

void TransformControls::ApplyScale(Vector4 scale, Entity e)
{
	transformSys->AddScale(e, 1 + (scale.x * scale.w), 1 + (scale.y * scale.w), 1 + (scale.z * scale.w));
}

bool TransformControls::CheckHover(Vector3 HoverRay, Entity cam)
{
	if(!active || mode == E_PIVOT)
		return false;

	auto camComp = camSys->GetComponent(cam);

	float curdist = camComp->far_clip;
	Vector3 camPos = camComp->camPos;
	eDirections hover = E_TRANSFORM_NOPE;

	Vector3 rayDir = XMVector3Normalize(XMLoadFloat3(&HoverRay));
	CXMMATRIX transform = matricies[frust_ids[camComp->frust_id]].actualTransform;
	float scale = matricies[frust_ids[camComp->frust_id]].scale;
	
	switch (mode)
	{
	case TransformControls::E_MOVE:
		{
			float dist;

			auto arrow_x_mesh = MeshMgr::GetResourcePtr(arrow_x);
			auto arrow_y_mesh = MeshMgr::GetResourcePtr(arrow_y);
			auto arrow_z_mesh = MeshMgr::GetResourcePtr(arrow_z);
			auto arrow_xy_mesh = MeshMgr::GetResourcePtr(plane_xy);
			auto arrow_xz_mesh = MeshMgr::GetResourcePtr(plane_xz);
			auto arrow_yz_mesh = MeshMgr::GetResourcePtr(plane_yz);

			if( (dist = RayBoxIntersect(camPos, rayDir, arrow_x_mesh->box, transform)) >= 0 )
			{
				curdist = dist;
				hover = E_TRANSFORM_X;
			}
			if( (dist = RayBoxIntersect(camPos, rayDir, arrow_y_mesh->box, transform)) >= 0 )
			{
				if(dist < curdist)
				{
					curdist = dist;
					hover = E_TRANSFORM_Y;
				}
			}
			if( (dist = RayBoxIntersect(camPos, rayDir, arrow_z_mesh->box, transform)) >= 0 )
			{
				if(dist < curdist)
				{
					curdist = dist;
					hover = E_TRANSFORM_Z;
				}
			}
			if( (dist = RayBoxIntersect(camPos, rayDir, arrow_xy_mesh->box, transform)) >= 0 )
			{
				if(dist < curdist)
				{
					curdist = dist;
					hover = E_TRANSFORM_XY;
				}
			}
			if( (dist = RayBoxIntersect(camPos, rayDir, arrow_xz_mesh->box, transform)) >= 0 )
			{
				if(dist < curdist)
				{
					curdist = dist;
					hover = E_TRANSFORM_XZ;
				}
			}
			if( (dist = RayBoxIntersect(camPos, rayDir, arrow_yz_mesh->box, transform)) >= 0 )
			{
				if(dist < curdist)
				{
					curdist = dist;
					hover = E_TRANSFORM_YZ;
				}
			}
		}
		break;
	case TransformControls::E_ROT:
		{
			float dist;

			auto rot_x_mesh = MeshMgr::GetResourcePtr(rot_x);
			auto rot_y_mesh = MeshMgr::GetResourcePtr(rot_y);
			auto rot_z_mesh = MeshMgr::GetResourcePtr(rot_z);
			auto rot_all_mesh = MeshMgr::GetResourcePtr(rot_all);

			if( (dist = RayBoxIntersect(camPos, rayDir, rot_x_mesh->box, transform)) >= 0 )
			{
				Vector3 temp = (camPos + rayDir * dist) - m_pos;
				temp /= scale;
				float radialDistSq = temp.LengthSquared();
				if(radialDistSq <= TC_ROT_RADIAL_COLLISION_MAX_SQ && radialDistSq >= TC_ROT_RADIAL_COLLISION_MIN_SQ)
				{
					curdist = dist;
					hover = E_TRANSFORM_X;
				}
			}
			if( (dist = RayBoxIntersect(camPos, rayDir, rot_y_mesh->box, transform)) >= 0 )
			{
				if(dist < curdist)
				{
					Vector3 temp = (camPos + rayDir * dist) - m_pos;
					temp /= scale;
					float radialDistSq = temp.LengthSquared();
					if(radialDistSq <= TC_ROT_RADIAL_COLLISION_MAX_SQ && radialDistSq >= TC_ROT_RADIAL_COLLISION_MIN_SQ)
					{
						curdist = dist;
						hover = E_TRANSFORM_Y;
					}
				}
			}
			if( (dist = RayBoxIntersect(camPos, rayDir, rot_z_mesh->box, transform)) >= 0 )
			{
				if(dist < curdist)
				{
					Vector3 temp = (camPos + rayDir * dist) - m_pos;
					temp /= scale;
					float radialDistSq = temp.LengthSquared();
					if(radialDistSq <= TC_ROT_RADIAL_COLLISION_MAX_SQ && radialDistSq >= TC_ROT_RADIAL_COLLISION_MIN_SQ)
					{
						curdist = dist;
						hover = E_TRANSFORM_Z;
					}
				}
			}
			if( (dist = RayBoxIntersect(camPos, rayDir, rot_all_mesh->box, transform)) >= 0 )
			{
				if(dist < curdist)
				{
					curdist = dist;
					hover = E_TRANSFORM_ALL;
				}
			}
		}
		break;
	case TransformControls::E_SCALE:
		{
			float dist;

			auto box_x_mesh = MeshMgr::GetResourcePtr(box_x);
			auto box_y_mesh = MeshMgr::GetResourcePtr(box_y);
			auto box_z_mesh = MeshMgr::GetResourcePtr(box_z);
			auto box_all_mesh = MeshMgr::GetResourcePtr(box_all);
			auto arrow_xy_mesh = MeshMgr::GetResourcePtr(plane_xy);
			auto arrow_xz_mesh = MeshMgr::GetResourcePtr(plane_xz);
			auto arrow_yz_mesh = MeshMgr::GetResourcePtr(plane_yz);

			if( (dist = RayBoxIntersect(camPos, rayDir, box_x_mesh->box, transform)) >= 0 )
			{
				curdist = dist;
				hover = E_TRANSFORM_X;
			}
			if( (dist = RayBoxIntersect(camPos, rayDir, box_y_mesh->box, transform)) >= 0 )
			{
				if(dist < curdist)
				{
					curdist = dist;
					hover = E_TRANSFORM_Y;
				}
			}
			if( (dist = RayBoxIntersect(camPos, rayDir, box_z_mesh->box, transform)) >= 0 )
			{
				if(dist < curdist)
				{
					curdist = dist;
					hover = E_TRANSFORM_Z;
				}
			}
			if( (dist = RayBoxIntersect(camPos, rayDir, arrow_xy_mesh->box, transform)) >= 0 )
			{
				if(dist < curdist)
				{
					curdist = dist;
					hover = E_TRANSFORM_XY;
				}
			}
			if( (dist = RayBoxIntersect(camPos, rayDir, arrow_xz_mesh->box, transform)) >= 0 )
			{
				if(dist < curdist)
				{
					curdist = dist;
					hover = E_TRANSFORM_XZ;
				}
			}
			if( (dist = RayBoxIntersect(camPos, rayDir, arrow_yz_mesh->box, transform)) >= 0 )
			{
				if(dist < curdist)
				{
					curdist = dist;
					hover = E_TRANSFORM_YZ;
				}
			}
			if( (dist = RayBoxIntersect(camPos, rayDir, box_all_mesh->box, transform)) >= 0 )
			{
				if(dist < curdist)
				{
					curdist = dist;
					hover = E_TRANSFORM_ALL;
				}
			}
		}
		break;
	}

	b_hover = hover;

	if(b_hover != E_TRANSFORM_NOPE)
		return true;
	return false;
}