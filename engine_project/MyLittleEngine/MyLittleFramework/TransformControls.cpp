#include "stdafx.h"
#include "TransformControls.h"
#include "Render.h"
#include "World.h"
#include "MaterialMgr.h"

using namespace EngineCore;

TransformControls::TransformControls(World* wrd)
{
	FrustumMgr* frustumMgr = wrd->GetFrustumMgr();
	frustums = &frustumMgr->camDataArray;

	camSys = wrd->GetCameraSystem();
	transformSys = wrd->GetTransformSystem();

	isLocal = false;
	m_transformation = XMMatrixIdentity();
	m_pos = XMVectorZero();
	
	arrow_x = StMeshMgr::Get()->GetStMesh(string(MESH_ARROW_X));
	arrow_y = StMeshMgr::Get()->GetStMesh(string(MESH_ARROW_Y));
	arrow_z = StMeshMgr::Get()->GetStMesh(string(MESH_ARROW_Z));
	plane_xy = StMeshMgr::Get()->GetStMesh(string(MESH_PLANE_XY));
	plane_xz = StMeshMgr::Get()->GetStMesh(string(MESH_PLANE_XZ));
	plane_yz = StMeshMgr::Get()->GetStMesh(string(MESH_PLANE_YZ));
	box_all = StMeshMgr::Get()->GetStMesh(string(MESH_BOX_ALL));
	box_x = StMeshMgr::Get()->GetStMesh(string(MESH_BOX_X));
	box_y = StMeshMgr::Get()->GetStMesh(string(MESH_BOX_Y));
	box_z = StMeshMgr::Get()->GetStMesh(string(MESH_BOX_Z));
	rot_all = StMeshMgr::Get()->GetStMesh(string(MESH_ROT_ALL));
	rot_x = StMeshMgr::Get()->GetStMesh(string(MESH_ROT_X));
	rot_y = StMeshMgr::Get()->GetStMesh(string(MESH_ROT_Y));
	rot_z = StMeshMgr::Get()->GetStMesh(string(MESH_ROT_Z));
	
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
		matricies[i].constantBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(StmMatrixBuffer), false);
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
	StMeshMgr::Get()->DeleteStMesh(arrow_x);
	StMeshMgr::Get()->DeleteStMesh(arrow_y);
	StMeshMgr::Get()->DeleteStMesh(arrow_z);
	StMeshMgr::Get()->DeleteStMesh(plane_xy);
	StMeshMgr::Get()->DeleteStMesh(plane_xz);
	StMeshMgr::Get()->DeleteStMesh(plane_yz);
	StMeshMgr::Get()->DeleteStMesh(box_all);
	StMeshMgr::Get()->DeleteStMesh(box_x);
	StMeshMgr::Get()->DeleteStMesh(box_y);
	StMeshMgr::Get()->DeleteStMesh(box_z);
	StMeshMgr::Get()->DeleteStMesh(rot_all);
	StMeshMgr::Get()->DeleteStMesh(rot_x);
	StMeshMgr::Get()->DeleteStMesh(rot_y);
	StMeshMgr::Get()->DeleteStMesh(rot_z);

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

void TransformControls::calcSreenScaleMat(CXMVECTOR campos, uint id)
{
	XMVECTOR v_dist = campos - m_pos;
	matricies[id].scale = XMVectorGetX(XMVector3Length(v_dist)) * m_scale;
	matricies[id].actualTransform = XMMatrixScaling(matricies[id].scale, matricies[id].scale, matricies[id].scale) * m_transformation;

	StmMatrixBuffer mb;
	mb.world = XMMatrixTranspose(matricies[id].actualTransform);
	mb.norm = XMMatrixIdentity();
	Render::UpdateSubresource(matricies[id].constantBuffer, 0, NULL, &mb, 0, 0);
}

void TransformControls::RegToDraw()
{
	if(!active || mode == E_PIVOT)
		return;

	uint frustCount = 0;
	switch (mode)
	{
	case TransformControls::E_MOVE:
		for(auto f: *frustums)
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

			auto arrow_x_mesh = StMeshMgr::GetStMeshPtr(arrow_x);
			auto arrow_y_mesh = StMeshMgr::GetStMeshPtr(arrow_y);
			auto arrow_z_mesh = StMeshMgr::GetStMeshPtr(arrow_z);
			auto arrow_xy_mesh = StMeshMgr::GetStMeshPtr(plane_xy);
			auto arrow_xz_mesh = StMeshMgr::GetStMeshPtr(plane_xz);
			auto arrow_yz_mesh = StMeshMgr::GetStMeshPtr(plane_yz);

			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_x_mesh->indexCount[0], arrow_x_mesh->vertexBuffer[0], arrow_x_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_axis_x, XMVector3TransformCoord(XMLoadFloat3(&arrow_x_mesh->box.Center), m_transformation));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_y_mesh->indexCount[0], arrow_y_mesh->vertexBuffer[0], arrow_y_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_axis_y, XMVector3TransformCoord(XMLoadFloat3(&arrow_y_mesh->box.Center), m_transformation));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_z_mesh->indexCount[0], arrow_z_mesh->vertexBuffer[0], arrow_z_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_axis_z, XMVector3TransformCoord(XMLoadFloat3(&arrow_z_mesh->box.Center), m_transformation));

			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_xy_mesh->indexCount[0], arrow_xy_mesh->vertexBuffer[0], arrow_xy_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_plane_xy, XMVector3TransformCoord(XMLoadFloat3(&arrow_xy_mesh->box.Center), m_transformation));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_xz_mesh->indexCount[0], arrow_xz_mesh->vertexBuffer[0], arrow_xz_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_plane_xz, XMVector3TransformCoord(XMLoadFloat3(&arrow_xz_mesh->box.Center), m_transformation));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_yz_mesh->indexCount[0], arrow_yz_mesh->vertexBuffer[0], arrow_yz_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_plane_yz, XMVector3TransformCoord(XMLoadFloat3(&arrow_yz_mesh->box.Center), m_transformation));
	
			frustCount++;
		}
		break;
	case TransformControls::E_ROT:
		for(auto f: *frustums)
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

			auto rot_x_mesh = StMeshMgr::GetStMeshPtr(rot_x);
			auto rot_y_mesh = StMeshMgr::GetStMeshPtr(rot_y);
			auto rot_z_mesh = StMeshMgr::GetStMeshPtr(rot_z);
			auto rot_all_mesh = StMeshMgr::GetStMeshPtr(rot_all);

			((SceneRenderMgr*)f->rendermgr)->RegMesh(rot_x_mesh->indexCount[0], rot_x_mesh->vertexBuffer[0], rot_x_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_rot_x, XMVector3TransformCoord(XMLoadFloat3(&rot_x_mesh->box.Center), m_transformation));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(rot_y_mesh->indexCount[0], rot_y_mesh->vertexBuffer[0], rot_y_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_rot_y, XMVector3TransformCoord(XMLoadFloat3(&rot_y_mesh->box.Center), m_transformation));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(rot_z_mesh->indexCount[0], rot_z_mesh->vertexBuffer[0], rot_z_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_rot_z, XMVector3TransformCoord(XMLoadFloat3(&rot_z_mesh->box.Center), m_transformation));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(rot_all_mesh->indexCount[0], rot_all_mesh->vertexBuffer[0], rot_all_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_all, XMVector3TransformCoord(XMLoadFloat3(&rot_all_mesh->box.Center), m_transformation));
		
			frustCount++;
		}
		break;
	case TransformControls::E_SCALE:
		for(auto f: *frustums)
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

			auto box_x_mesh = StMeshMgr::GetStMeshPtr(box_x);
			auto box_y_mesh = StMeshMgr::GetStMeshPtr(box_y);
			auto box_z_mesh = StMeshMgr::GetStMeshPtr(box_z);
			auto box_all_mesh = StMeshMgr::GetStMeshPtr(box_all);
			auto arrow_xy_mesh = StMeshMgr::GetStMeshPtr(plane_xy);
			auto arrow_xz_mesh = StMeshMgr::GetStMeshPtr(plane_xz);
			auto arrow_yz_mesh = StMeshMgr::GetStMeshPtr(plane_yz);

			((SceneRenderMgr*)f->rendermgr)->RegMesh(box_x_mesh->indexCount[0], box_x_mesh->vertexBuffer[0], box_x_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_axis_x, XMVector3TransformCoord(XMLoadFloat3(&box_x_mesh->box.Center), m_transformation));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(box_y_mesh->indexCount[0], box_y_mesh->vertexBuffer[0], box_y_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_axis_y, XMVector3TransformCoord(XMLoadFloat3(&box_y_mesh->box.Center), m_transformation));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(box_z_mesh->indexCount[0], box_z_mesh->vertexBuffer[0], box_z_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_axis_z, XMVector3TransformCoord(XMLoadFloat3(&box_z_mesh->box.Center), m_transformation));

			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_xy_mesh->indexCount[0], arrow_xy_mesh->vertexBuffer[0], arrow_xy_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_plane_xy, XMVector3TransformCoord(XMLoadFloat3(&arrow_xy_mesh->box.Center), m_transformation));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_xz_mesh->indexCount[0], arrow_xz_mesh->vertexBuffer[0], arrow_xz_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_plane_xz, XMVector3TransformCoord(XMLoadFloat3(&arrow_xz_mesh->box.Center), m_transformation));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(arrow_yz_mesh->indexCount[0], arrow_yz_mesh->vertexBuffer[0], arrow_yz_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_plane_yz, XMVector3TransformCoord(XMLoadFloat3(&arrow_yz_mesh->box.Center), m_transformation));
			((SceneRenderMgr*)f->rendermgr)->RegMesh(box_all_mesh->indexCount[0], box_all_mesh->vertexBuffer[0], box_all_mesh->indexBuffer[0], matricies[frustCount].constantBuffer, 
				sizeof(LitVertex), mat_all, XMVector3TransformCoord(XMLoadFloat3(&box_all_mesh->box.Center), m_transformation));
		
			frustCount++;
		}
		break;
	}
}

XMFLOAT3 TransformControls::CalcMove(XMFLOAT3 Ray, XMFLOAT3 PrevRay, Entity cam)
{
	XMFLOAT3 res = XMFLOAT3(0,0,0);
	if(!active || b_hover == E_TRANSFORM_NOPE || mode != E_MOVE)
		return res;

	XMVECTOR pos, rot, scale;
	if(!XMMatrixDecompose(&scale, &rot, &pos, m_transformation))
		return res;

	auto camComp = camSys->GetComponent(cam);

	XMVECTOR l_PrevRay = XMVector3Normalize(XMLoadFloat3(&PrevRay));
	XMVECTOR l_Ray = XMVector3Normalize(XMLoadFloat3(&Ray));

	XMVECTOR axis;
	switch (b_hover)
	{
	case TransformControls::E_TRANSFORM_X: axis = XMVectorSet(1.0f,0.0f,0.0f,0.0f);
		break;
	case TransformControls::E_TRANSFORM_Y: axis = XMVectorSet(0.0f,1.0f,0.0f,0.0f);
		break;
	case TransformControls::E_TRANSFORM_Z: axis = XMVectorSet(0.0f,0.0f,1.0f,0.0f);
		break;
	case TransformControls::E_TRANSFORM_XY: axis = XMVectorSet(0.0f,0.0f,1.0f,0.0f);
		break;
	case TransformControls::E_TRANSFORM_XZ: axis = XMVectorSet(0.0f,1.0f,0.0f,0.0f);
		break;
	case TransformControls::E_TRANSFORM_YZ: axis = XMVectorSet(1.0f,0.0f,0.0f,0.0f);
		break;
	}

	switch (b_hover)
	{
	case TransformControls::E_TRANSFORM_X:
	case TransformControls::E_TRANSFORM_Y:
	case TransformControls::E_TRANSFORM_Z:
		{
			XMVECTOR dist = XMVector3Normalize(m_pos - camComp->camPos);
			XMVECTOR dir = XMVector3Transform(axis, XMMatrixRotationQuaternion(rot));

			XMVECTOR norm = XMVector3Normalize(XMVector3Cross(dir, XMVector3Cross(dir, dist)));
			XMVECTOR plane = XMPlaneFromPointNormal(m_pos, norm);

			XMVECTOR point_from = XMPlaneIntersectLine(plane, camComp->camPos, l_PrevRay * camComp->far_clip);
			XMVECTOR point_to = XMPlaneIntersectLine(plane, camComp->camPos, l_Ray * camComp->far_clip);
			XMVECTOR move_vect = point_to - point_from;
			XMStoreFloat3(&res, XMVectorGetX(XMVector3Dot(move_vect, dir)) * dir);
		}
		break;
	case TransformControls::E_TRANSFORM_XY:
	case TransformControls::E_TRANSFORM_XZ:
	case TransformControls::E_TRANSFORM_YZ:
		{
			XMVECTOR norm = XMVector3Transform(axis, XMMatrixRotationQuaternion(rot));
			XMVECTOR plane = XMPlaneFromPointNormal(m_pos, norm);

			XMVECTOR point_from = XMPlaneIntersectLine(plane, camComp->camPos, l_PrevRay * camComp->far_clip);
			XMVECTOR point_to = XMPlaneIntersectLine(plane, camComp->camPos, l_Ray * camComp->far_clip);
			XMStoreFloat3(&res, point_to - point_from);
		}
		break;
	}

	return res;
}

XMFLOAT4 TransformControls::CalcScale(XMFLOAT3 Ray, XMFLOAT3 PrevRay, Entity cam)
{
	XMFLOAT4 res = XMFLOAT4(0,0,0,0);
	if(!active || b_hover == E_TRANSFORM_NOPE || mode != E_SCALE)
		return res;

	XMVECTOR pos, rot, scale;
	if(!XMMatrixDecompose(&scale, &rot, &pos, m_transformation))
		return res;

	auto camComp = camSys->GetComponent(cam);

	XMVECTOR l_PrevRay = XMVector3Normalize(XMLoadFloat3(&PrevRay));
	XMVECTOR l_Ray = XMVector3Normalize(XMLoadFloat3(&Ray));

	XMVECTOR axis;
	XMVECTOR plane_axis;
	switch (b_hover)
	{
	case TransformControls::E_TRANSFORM_X: axis = XMVectorSet(1.0f,0.0f,0.0f,0.0f); 
		res.x = 1;
		break;
	case TransformControls::E_TRANSFORM_Y: axis = XMVectorSet(0.0f,1.0f,0.0f,0.0f);
		res.y = 1;
		break;
	case TransformControls::E_TRANSFORM_Z: axis = XMVectorSet(0.0f,0.0f,1.0f,0.0f); 
		res.z = 1;
		break;
	case TransformControls::E_TRANSFORM_XY: plane_axis = XMVectorSet(0.0f,0.0f,1.0f,0.0f); axis = XMVectorSet(1.0f,1.0f,0.0f,0.0f);
		res.x = 1; res.y = 1;
		break;
	case TransformControls::E_TRANSFORM_XZ: plane_axis = XMVectorSet(0.0f,1.0f,0.0f,0.0f); axis = XMVectorSet(1.0f,0.0f,1.0f,0.0f); 
		res.x = 1; res.z = 1;
		break;
	case TransformControls::E_TRANSFORM_YZ: plane_axis = XMVectorSet(1.0f,0.0f,0.0f,0.0f); axis = XMVectorSet(0.0f,1.0f,1.0f,0.0f);
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
			XMVECTOR dist = XMVector3Normalize(m_pos - camComp->camPos);
			XMVECTOR dir = XMVector3Transform(axis, XMMatrixRotationQuaternion(rot));

			XMVECTOR norm = XMVector3Normalize(XMVector3Cross(dir, XMVector3Cross(dir, dist)));
			XMVECTOR plane = XMPlaneFromPointNormal(m_pos, norm);

			XMVECTOR point_from = XMPlaneIntersectLine(plane, camComp->camPos, l_PrevRay * camComp->far_clip);
			XMVECTOR point_to = XMPlaneIntersectLine(plane, camComp->camPos, l_Ray * camComp->far_clip);
			XMVECTOR move_vect = point_to - point_from;
			res.w = XMVectorGetX(XMVector3Dot(move_vect, dir));
		}
		break;
	case TransformControls::E_TRANSFORM_XY:
	case TransformControls::E_TRANSFORM_XZ:
	case TransformControls::E_TRANSFORM_YZ:
		{
			XMVECTOR norm = XMVector3Normalize(XMVector3Transform(plane_axis, XMMatrixRotationQuaternion(rot)));
			XMVECTOR axis_proj = XMVector3Normalize(XMVector3Transform(axis, XMMatrixRotationQuaternion(rot)));
			XMVECTOR plane = XMPlaneFromPointNormal(m_pos, norm);

			XMVECTOR point_from = XMPlaneIntersectLine(plane, camComp->camPos, l_PrevRay * camComp->far_clip);
			XMVECTOR point_to = XMPlaneIntersectLine(plane, camComp->camPos, l_Ray * camComp->far_clip);
			XMVECTOR move_vect = point_to - point_from;
			res.w = XMVectorGetX(XMVector3Dot(move_vect, axis_proj));
		}
		break;
	case TransformControls::E_TRANSFORM_ALL:
		{
			XMVECTOR plane = XMPlaneFromPointNormal(m_pos, XMVector3Normalize(-camComp->camLookDir));
			XMVECTOR point_from = XMPlaneIntersectLine(plane, camComp->camPos, l_PrevRay * camComp->far_clip);
			XMVECTOR point_to = XMPlaneIntersectLine(plane, camComp->camPos, l_Ray * camComp->far_clip);
			XMVECTOR move_vect = point_to - point_from;
			res.w = XMVectorGetX(XMVector3Dot(move_vect, XMVector3Normalize(XMVector3Cross(camComp->camUp, camComp->camLookDir) + camComp->camUp)));
		}
		break;
	}

	res.w *= TC_SCALE_MUL;

	return res;
}

XMFLOAT4 TransformControls::CalcRot(XMFLOAT3 Ray, XMFLOAT3 PrevRay, Entity cam)
{
	XMFLOAT4 res = XMFLOAT4(1,0,0,0);
	if(!active || b_hover == E_TRANSFORM_NOPE || mode != E_ROT)
		return res;

	XMVECTOR pos, rot, scale;
	if(!XMMatrixDecompose(&scale, &rot, &pos, m_transformation))
		return res;

	auto camComp = camSys->GetComponent(cam);

	XMVECTOR l_PrevRay = XMVector3Normalize(XMLoadFloat3(&PrevRay));
	XMVECTOR l_Ray = XMVector3Normalize(XMLoadFloat3(&Ray));

	XMVECTOR axis;
	switch (b_hover)
	{
	case TransformControls::E_TRANSFORM_X: axis = XMVectorSet(1.0f,0.0f,0.0f,0.0f);
		break;
	case TransformControls::E_TRANSFORM_Y: axis = XMVectorSet(0.0f,1.0f,0.0f,0.0f);
		break;
	case TransformControls::E_TRANSFORM_Z: axis = XMVectorSet(0.0f,0.0f,1.0f,0.0f);
		break;
	}

	switch (b_hover)
	{
	case TransformControls::E_TRANSFORM_X:
	case TransformControls::E_TRANSFORM_Y:
	case TransformControls::E_TRANSFORM_Z:
		{
			XMVECTOR norm = XMVector3Transform(axis, XMMatrixRotationQuaternion(rot));
			XMVECTOR plane = XMPlaneFromPointNormal(m_pos, norm);

			XMVECTOR point_from = XMPlaneIntersectLine(plane, camComp->camPos, l_PrevRay * camComp->far_clip);
			XMVECTOR point_to = XMPlaneIntersectLine(plane, camComp->camPos, l_Ray * camComp->far_clip);
			point_from = point_from - m_pos;
			point_to = point_to - m_pos;

			float angle = XMVectorGetX(XMVector3AngleBetweenVectors(point_from, point_to));
			XMVECTOR rot_dir_test = XMVector3Cross(point_from, point_to);
			if(XMVectorGetX(XMVector3Dot(norm, rot_dir_test)) < 0)
				angle = -angle;

			res = XMFLOAT4(XMVectorGetX(norm), XMVectorGetY(norm), XMVectorGetZ(norm), angle);
		}
		break;
	case TransformControls::E_TRANSFORM_ALL:
		{
			XMVECTOR norm = XMVector3Normalize(-camComp->camLookDir);
			XMVECTOR plane = XMPlaneFromPointNormal(m_pos, norm);
			XMVECTOR point_from = XMPlaneIntersectLine(plane, camComp->camPos, l_PrevRay * camComp->far_clip);
			XMVECTOR point_to = XMPlaneIntersectLine(plane, camComp->camPos, l_Ray * camComp->far_clip);
			point_from = point_from - m_pos;
			point_to = point_to - m_pos;

			float angle = XMVectorGetX(XMVector3AngleBetweenVectors(point_from, point_to));
			XMVECTOR rot_dir_test = XMVector3Cross(point_from, point_to);
			if(XMVectorGetX(XMVector3Dot(norm, rot_dir_test)) < 0)
				angle = -angle;
			res = XMFLOAT4(XMVectorGetX(norm), XMVectorGetY(norm), XMVectorGetZ(norm), angle);
		}
		break;
	}

	return res;
}

void TransformControls::ApplyMove(XMFLOAT3 move, Entity e)
{
	transformSys->AddPosition(e, move.x, move.y, move.z);
	SetTransformation(transformSys->GetTransformL(e)); // todo
}

void TransformControls::ApplyRot(XMFLOAT4 rot, Entity e)
{
	transformSys->AddRotation(e, XMFLOAT3(rot.x, rot.y, rot.z), rot.w);
	SetTransformation(transformSys->GetTransformL(e)); // todo
}

void TransformControls::ApplyScale(XMFLOAT4 scale, Entity e)
{
	transformSys->AddScale(e, 1 + (scale.x * scale.w), 1 + (scale.y * scale.w), 1 + (scale.z * scale.w));
}

bool TransformControls::CheckHover(XMFLOAT3 HoverRay, Entity cam)
{
	if(!active || mode == E_PIVOT)
		return false;

	auto camComp = camSys->GetComponent(cam);

	float curdist = camComp->far_clip;
	XMVECTOR camPos = camComp->camPos;
	eDirections hover = E_TRANSFORM_NOPE;

	XMVECTOR rayDir = XMVector3Normalize(XMLoadFloat3(&HoverRay));
	CXMMATRIX transform = matricies[frust_ids[camComp->frust_id]].actualTransform;
	float scale = matricies[frust_ids[camComp->frust_id]].scale;
	
	switch (mode)
	{
	case TransformControls::E_MOVE:
		{
			float dist;

			auto arrow_x_mesh = StMeshMgr::GetStMeshPtr(arrow_x);
			auto arrow_y_mesh = StMeshMgr::GetStMeshPtr(arrow_y);
			auto arrow_z_mesh = StMeshMgr::GetStMeshPtr(arrow_z);
			auto arrow_xy_mesh = StMeshMgr::GetStMeshPtr(plane_xy);
			auto arrow_xz_mesh = StMeshMgr::GetStMeshPtr(plane_xz);
			auto arrow_yz_mesh = StMeshMgr::GetStMeshPtr(plane_yz);

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

			auto rot_x_mesh = StMeshMgr::GetStMeshPtr(rot_x);
			auto rot_y_mesh = StMeshMgr::GetStMeshPtr(rot_y);
			auto rot_z_mesh = StMeshMgr::GetStMeshPtr(rot_z);
			auto rot_all_mesh = StMeshMgr::GetStMeshPtr(rot_all);

			if( (dist = RayBoxIntersect(camPos, rayDir, rot_x_mesh->box, transform)) >= 0 )
			{
				float radialDistSq = XMVectorGetX(XMVector3LengthSq( ((camPos + rayDir * dist) - m_pos) / scale ));
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
					float radialDistSq = XMVectorGetX(XMVector3LengthSq( ((camPos + rayDir * dist) - m_pos) / scale ));
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
					float radialDistSq = XMVectorGetX(XMVector3LengthSq( ((camPos + rayDir * dist) - m_pos) / scale ));
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

			auto box_x_mesh = StMeshMgr::GetStMeshPtr(box_x);
			auto box_y_mesh = StMeshMgr::GetStMeshPtr(box_y);
			auto box_z_mesh = StMeshMgr::GetStMeshPtr(box_z);
			auto box_all_mesh = StMeshMgr::GetStMeshPtr(box_all);
			auto arrow_xy_mesh = StMeshMgr::GetStMeshPtr(plane_xy);
			auto arrow_xz_mesh = StMeshMgr::GetStMeshPtr(plane_xz);
			auto arrow_yz_mesh = StMeshMgr::GetStMeshPtr(plane_yz);

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