#include "stdafx.h"
#include "FreeCamController.h"
#include "World.h"

using namespace EngineCore;

FreeCamController::FreeCamController(): Controller()
{
	move_forward = false;
	move_backward = false;
	move_right = false;
	move_left = false;
	move_down = false;
	move_up = false;

	rot_speed = 0.010f;
	move_speed = 0.02f;

	forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	rightward = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	upward = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	rot_coords.x = 0;
	rot_coords.y = 0;
}

void FreeCamController::SetPlayerStand(float x, float y, float z)
{
	XMMATRIX rotM = XMMatrixRotationRollPitchYaw(x, y, z);
	forward = XMVector3TransformNormal(XMVector3Normalize(forward), rotM);
	rightward = XMVector3TransformNormal(XMVector3Normalize(rightward), rotM);
	upward = XMVector3TransformNormal(XMVector3Normalize(upward), rotM);
}

void FreeCamController::Process()
{
	float dt = world->GetDT();

	XMVECTOR move = XMVectorZero();
	float dist = dt * move_speed;

	XMVECTOR scale, rot, pos;
	XMMatrixDecompose(&scale, &rot, &pos, transform);

	XMVECTOR camforward, camrightward, camupward;
	camforward = XMVector3TransformNormal(forward, transform);
	camupward = XMVector3TransformNormal(upward, transform);
	camrightward = XMVector3TransformNormal(rightward, transform);

	if(move_forward) move += dist * camforward;
	if(move_backward) move -= dist * camforward;
	if(move_left) move -= dist * camrightward;
	if(move_right) move += dist * camrightward;
	if(move_up) move += dist * camupward;
	if(move_down) move -= dist * camupward;

	pos += move;

	// remove with pitch??
	bool lookup = XMVectorGetX(XMVector3Dot(camforward, upward)) >= 0;

	XMVECTOR toHorz = XMVector3Cross(camrightward, upward);
	float vertAngle = XMVectorGetX(XMVector3AngleBetweenNormals(camforward, toHorz));
	float deltaVertAngle = float(-rot_coords.y) * rot_speed;
	float newVertAngle = vertAngle + (lookup ? -deltaVertAngle : deltaVertAngle);

	if(newVertAngle > XM_PIDIV2)
	{
		if(lookup) deltaVertAngle = vertAngle - XM_PIDIV2;
		else deltaVertAngle = XM_PIDIV2 - vertAngle;
	}
	// ---

	XMMATRIX rotM = XMMatrixRotationQuaternion(rot) * XMMatrixRotationNormal(camrightward, deltaVertAngle) * XMMatrixRotationNormal(upward, float(rot_coords.x) * rot_speed);

	camforward = XMVector3TransformNormal(forward, rotM);
	camupward = XMVector3TransformNormal(upward, rotM);
	camrightward = XMVector3TransformNormal(rightward, rotM);

	transform = XMMatrixIdentity();
	transform *= rotM;
	transform *= XMMatrixTranslationFromVector(pos);

	rot_coords.x = 0;
	rot_coords.y = 0;
}

void FreeCamController::GetInput(ControllerComands cmd, float param1, float param2)
{
	switch (cmd)
	{
	case CC_DELTA_ROT:
		rot_coords.x += int(param1);
		rot_coords.y += int(param2);
		break;
	case CC_FORWARD_START:
		move_forward = true;
		break;
	case CC_FORWARD_END:
		move_forward = false;
		break;
	case CC_BACK_START:
		move_backward = true;
		break;
	case CC_BACK_END:
		move_backward = false;
		break;
	case CC_LEFT_START:
		move_left = true;
		break;
	case CC_LEFT_END:
		move_left = false;
		break;
	case CC_RIGHT_START:
		move_right = true;
		break;
	case CC_RIGHT_END:
		move_right = false;
		break;
	case CC_UP_START:
		move_up = true;
		break;
	case CC_UP_END:
		move_up = false;
		break;
	case CC_DOWN_START:
		move_down = true;
		break;
	case CC_DOWN_END:
		move_down = false;
		break;
	case CC_MOVE_SPEED_CHANGE:
		move_speed = param1;
		break;
	}
}