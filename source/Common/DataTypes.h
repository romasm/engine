#pragma once

#include "stdafx.h"

// -------------------------------

#define NULLSTR "#null"
#define NULLSTR_L L"#null"

#define SRT_SIZE 256

#define SELECT_3D_MAX_DIST 10000.0f

#define QUAT_ROT_NULL XMFLOAT4(0,0,0,1.0f)

// global constants
#define XM_PI_SQR XM_PI * XM_PI
#define XM_PI_SQRT sqrt(XM_PI)

// -------------------------------
static float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
// --------------------------------

#define NOT_RELOAD 0
#define NEED_LOADING_ONCE 2
#define NEED_RELOADING 1

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

#define uint uint32_t
#define uchar uint8_t

// --------------------------------
struct CollVertex
{
	XMVECTOR Pos;
};

struct OnlyVertex
{
	XMFLOAT3 Pos;
};

struct UnlitVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};

struct LitVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
	XMFLOAT3 Norm;
	XMFLOAT3 Tang;
	XMFLOAT3 Binorm;
};

// ------------------------------

struct BBox
{
	XMFLOAT3 p_max;
	XMFLOAT3 p_min;

	BBox()
	{
		p_max = XMFLOAT3(0.0f, 0.0f, 0.0f);
		p_min = XMFLOAT3(0.0f, 0.0f, 0.0f);
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
