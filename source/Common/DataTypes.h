#pragma once

#include "stdafx.h"

// -------------------------------

#define NULLSTR "#null"
#define NULLSTR_L L"#null"

#define SRT_SIZE 256

#define SELECT_3D_MAX_DIST 10000.0f

#define QUAT_ROT_NULL Vector4(0,0,0,1.0f)

// global constants
#define XM_PI_SQR XM_PI * XM_PI
#define XM_PI_SQRT sqrt(XM_PI)

#define VECTOR2_CAST(xmfloat) Vector3(xmfloat.x, xmfloat.y)
#define VECTOR3_CAST(xmfloat) Vector3(xmfloat.x, xmfloat.y, xmfloat.z)
#define VECTOR4_CAST(xmfloat) Vector3(xmfloat.x, xmfloat.y, xmfloat.z, xmfloat.w)

// -------------------------------
static float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
// --------------------------------

enum ReloadingType
{
	RELOAD_NONE = 0,
	RELOAD_ALWAYS,
	RELOAD_ONCE
};

struct MLRECT
{
	int32_t left;
	int32_t top;
	int32_t width;
	int32_t height;

	MLRECT(int l, int t, int w, int h)
	{
		left = l;
		top = t;
		width = w;
		height = h;
	}

	MLRECT()
	{
		top = 0;
		left = 0;
		width = 0;
		height = 0;
	}

	MLRECT& operator=(const MLRECT& r)
	{
		top = r.top;
		left = r.left;
		width = r.width;
		height = r.height;
		return *this;
	}
};

struct MLPOINT
{
	int32_t x;
	int32_t y;

	MLPOINT(int32_t nx, int32_t ny)
	{
		x = nx;
		y = ny;
	}

	MLPOINT()
	{
		x = 0;
		y = 0;
	}

	MLPOINT& operator=(const MLPOINT& r)
	{
		x = r.x;
		y = r.y;
		return *this;
	}
};

// --------------------------------

#define uint uint32_t // TODO: REMOVE
#define uchar uint8_t

// --------------------------------
struct CollVertex
{
	XMVECTOR Pos;
};

struct OnlyVertex
{
	Vector3 Pos;
};

struct UnlitVertex
{
	Vector3 Pos;
	Vector2 Tex;
};

struct LitVertex
{
	Vector3 Pos;
	Vector2 Tex;
	Vector3 Norm;
	Vector3 Tang;
	Vector3 Binorm;
};

#define BONE_PER_VERTEX_MAXCOUNT 8

struct LitSkinnedVertex
{
	Vector3 Pos;
	Vector2 Tex;
	Vector3 Norm;
	Vector3 Tang;
	Vector3 Binorm;
	int32_t boneId[BONE_PER_VERTEX_MAXCOUNT];
	float boneWeight[BONE_PER_VERTEX_MAXCOUNT];
};

// ------------------------------

struct BBox
{
	Vector3 p_max;
	Vector3 p_min;

	BBox()
	{
		p_max = Vector3(0.0f, 0.0f, 0.0f);
		p_min = Vector3(0.0f, 0.0f, 0.0f);
	}

	BBox& operator=(const BBox& right)
	{
		p_max = right.p_max;
		p_min = right.p_min;
		return *this;
	};
};

// -------------------------------------

struct StmMatrixBuffer
{
	XMMATRIX world;
	XMMATRIX norm;

	StmMatrixBuffer()
	{
		world = XMMatrixIdentity();
		norm = XMMatrixIdentity();
	}
};

struct SimpleMatrixBuffer
{
	XMMATRIX WVP;
};

struct ShadowMatrixBuffer
{
	XMMATRIX WVP;
	XMMATRIX WV;
};
// --------------------------------------

//#define MATNUM 50

struct StMeshInfo
{
	ID3D11Buffer **m_vertexBuffer;
	ID3D11Buffer **m_indexBuffer;
	
	uint16_t mat_num;
	uint32_t* m_indexCount;
	uint32_t* m_vertexCount;

	BoundingSphere sphere;
	BoundingBox box;

	wstring name;
};

// --------------------------------------

template <typename PointerClass=void> struct QueuePointer
{
	PointerClass * current;
	QueuePointer * next;
	QueuePointer * prev;

	QueuePointer()
	{
		current = nullptr;
		next = nullptr;
		prev = nullptr;
	}
};

// ------------------------------------
