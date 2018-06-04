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

struct BoxCornerSize
{
	Vector3 corner;
	Vector3 size;
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
struct Vector3Uint32
{
	uint32_t x;
	uint32_t y;
	uint32_t z;

	Vector3Uint32() : x(0), y(0), z(0) {}
	Vector3Uint32(uint32_t xx, uint32_t yy, uint32_t zz) : x(xx), y(yy), z(zz) {}
	Vector3Uint32(Vector3 v) : x((uint32_t)v.x), y((uint32_t)v.y), z((uint32_t)v.z) {}
};

struct Vector3Int32
{
	int32_t x;
	int32_t y;
	int32_t z;

	Vector3Int32() : x(0), y(0), z(0) {}
	Vector3Int32(int32_t xx, int32_t yy, int32_t zz) : x(xx), y(yy), z(zz) {}
	Vector3Int32(Vector3 v) : x((int32_t)v.x), y((int32_t)v.y), z((int32_t)v.z) {}
};

struct Vector4Uint16
{
	uint16_t x;
	uint16_t y;
	uint16_t z;
	uint16_t w;

	Vector4Uint16() : x(0), y(0), z(0), w(0) {}
	Vector4Uint16(uint16_t xx, uint16_t yy, uint16_t zz, uint16_t ww) : x(xx), y(yy), z(zz), w(ww) {}
};
/*
static const uint8_t voxelGrowFlags[26] =
{
	0b00010101,
	0b00000101,
	0b00100101,
	0b00010001,
	0b00000001,
	0b00100001,
	0b00011001,
	0b00001001,
	0b00101001,

	0b00010100,
	0b00000100,
	0b00100100,
	0b00010000,
	0b00100000,
	0b00011000,
	0b00001000,
	0b00101000,

	0b00010110,
	0b00000110,
	0b00100110,
	0b00010010,
	0b00000010,
	0b00100010,
	0b00011010,
	0b00001010,
	0b00101010
};*/

static const uint8_t voxelGrowFlags[26] =
{
	0b00010101,
	0b00011101,
	0b00100101,
	0b00011101,
	0b00111101,
	0b00101101,
	0b00011001,
	0b00111001,
	0b00101001,

	0b00010111,
	0b00110111,
	0b00100111,
	0b00011111,
	0b00101111,
	0b00011011,
	0b00111011,
	0b00101011,

	0b00010110,
	0b00110110,
	0b00100110,
	0b00011110,
	0b00111110,
	0b00101110,
	0b00011010,
	0b00111010,
	0b00101010
};

static const uint8_t voxelGrowFlagsInv[26] =
{
	0b00101010,
	0b00001010,
	0b00011010,
	0b00100010,
	0b00000010,
	0b00010010,
	0b00100110,
	0b00000110,
	0b00010110,

	0b00101000,
	0b00001000,
	0b00011000,
	0b00100000,
	0b00010000,
	0b00100100,
	0b00000100,
	0b00010100,

	0b00101001,
	0b00001001,
	0b00011001,
	0b00100001,
	0b00000001,
	0b00010001,
	0b00100101,
	0b00000101,
	0b00010101
};

static const Vector3Int32 voxelGrowDirs[26] =
{
	{ -1, -1, -1 },
	{ 0, -1, -1 },
	{ 1, -1, -1 },
	{ -1, 0, -1 },
	{ 0, 0, -1 },
	{ 1, 0, -1 },
	{ -1, 1, -1 },
	{ 0, 1, -1 },
	{ 1, 1, -1 },

	{ -1, -1, 0 },
	{ 0, -1, 0 },
	{ 1, -1, 0 },
	{ -1, 0, 0 },
	{ 1, 0, 0 },
	{ -1, 1, 0 },
	{ 0, 1, 0 },
	{ 1, 1, 0 },

	{ -1, -1, 1 },
	{ 0, -1, 1 },
	{ 1, -1, 1 },
	{ -1, 0, 1 },
	{ 0, 0, 1 },
	{ 1, 0, 1 },
	{ -1, 1, 1 },
	{ 0, 1, 1 },
	{ 1, 1, 1 }
};

template<uint32_t RES>
class VoxelizedCube
{
public:
	static const uint32_t resolution;
	// inverse normal: x, y, z
	uint8_t voxels[RES][RES][RES];
	// 6bit: x+ x- y+ y- z+ z-

	bool empty;

	void Clear()
	{
		empty = true;
		for (int32_t x = 0; x < RES; x++)
			for (int32_t y = 0; y < RES; y++)
				for (int32_t z = 0; z < RES; z++)
					voxels[x][y][z] = 0;
	}

	void Grow()
	{
		for (int32_t i = 0; i < RES - 1; i++)
		{
			for (int32_t x = 0; x < RES; x++)
				for (int32_t y = 0; y < RES; y++)
					for (int32_t z = 0; z < RES; z++)
					{
						uint8_t value = voxels[x][y][z];
						if (value == 0)
							continue;

						for (int32_t k = 0; k < 26; k++)
						{
							const Vector3Int32& dir = voxelGrowDirs[k];
							int32_t nextX = x + dir.x;
							int32_t nextY = y + dir.y;
							int32_t nextZ = z + dir.z;

							if (nextX < 0 || nextX >= RES ||
								nextY < 0 || nextY >= RES ||
								nextZ < 0 || nextZ >= RES)
								continue;

							uint8_t& nextValue = voxels[nextX][nextY][nextZ];

							if ((value & voxelGrowFlags[k]) > 0 &&
								(value & (voxelGrowFlagsInv[k])) == 0 && (nextValue & (voxelGrowFlagsInv[k])) == 0)
								nextValue |= value;
						}
					}
		}
	}
};

template<uint32_t RES>
const uint32_t VoxelizedCube<RES>::resolution = RES;