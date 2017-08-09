#pragma once

#include "stdafx.h"
#include "Log.h"
#include "DataTypes.h"

using namespace EngineCore;

__forceinline Vector3 LRHconvertion(const Vector3& v)
{
	return Vector3(v.x, v.y, -v.z);
}

static Quaternion q_identity(0, 0, 0, 1.0f);
static Vector2 v2_zero(0, 0);
static Vector3 v3_zero(0, 0, 0);
static Vector4 v4_zero(0, 0, 0, 0);

inline float lerp(float f1, float f2, float l)
{
	return f1 * (1 - l) + f2 * l;
}

inline float clamp(float fmin, float f, float fmax)
{
	return max<float>(fmin, min<float>(f, fmax));
}

inline int FloatRoundInt(float f)
{
	return int(f < 0.0f ? ceil(f - 0.5f) : floor(f + 0.5f));
}

inline Vector4 Vector4Add(Vector4& v1, Vector4& v2)
{return v1 + v2;}
inline Vector3 Vector3Add(Vector3& v1, Vector3& v2)
{return v1 + v2;}
inline Vector2 Vector2Add(Vector2& v1, Vector2& v2)
{return v1 + v2;}

inline Vector4 Vector4Sub(Vector4& v1, Vector4& v2)
{return v1 - v2;}
inline Vector3 Vector3Sub(Vector3& v1, Vector3& v2)
{return v1 - v2;}
inline Vector2 Vector2Sub(Vector2& v1, Vector2& v2)
{return v1 - v2;}

inline Vector4 Vector4Inverse(Vector4& v1)
{return -v1;}
inline Vector3 Vector3Inverse(Vector3& v1)
{return -v1;}
inline Vector2 Vector2Inverse(Vector2& v1)
{return -v1;}

inline Vector4 Vector4Mul(Vector4& v1, Vector4& v2)
{return v1 * v2;}
inline Vector3 Vector3Mul(Vector3& v1, Vector3& v2)
{return v1 * v2;}
inline Vector2 Vector2Mul(Vector2& v1, Vector2& v2)
{return v1 * v2;}

inline Vector4 Vector4MulScalar(Vector4& v1, float v2)
{return v1 * v2;}
inline Vector3 Vector3MulScalar(Vector3& v1, float v2)
{return v1 * v2;}
inline Vector2 Vector2MulScalar(Vector2& v1, float v2)
{return v1 * v2;}

inline void RegLuaMath()
{
	getGlobalNamespace(LSTATE)
		.beginClass<Quaternion>("Quaternion")
		.addData("x", &Quaternion::x)
		.addData("y", &Quaternion::y)
		.addData("z", &Quaternion::z)
		.addData("w", &Quaternion::w)
		.addConstructor<void (*)(float, float, float, float)>() 
		.addStaticData("Identity", &q_identity, false)
		.endClass()

		.beginClass<Vector4>("Vector4")
		.addData("x", &Vector4::x)
		.addData("y", &Vector4::y)
		.addData("z", &Vector4::z)
		.addData("w", &Vector4::w)
		.addConstructor<void (*)(float, float, float, float)>() 
		.addStaticData("Zero", &v4_zero, false)
		.addStaticFunction("Lerp", static_cast<Vector4 (*)(const Vector4&, const Vector4&, float)>(&Vector4::Lerp))
		.addStaticFunction("Add", &Vector4Add)
		.addStaticFunction("Sub", &Vector4Sub)
		.addStaticFunction("Inverse", &Vector4Inverse)
		.addStaticFunction("Mul", &Vector4Mul)
		.addStaticFunction("MulScalar", &Vector4MulScalar)
		.addFunction("Normalize", static_cast<void (Vector4::*)(void)>(&Vector4::Normalize))
		.addFunction("Dot", &Vector4::Dot)
		.endClass()

		.beginClass<Vector3>("Vector3")
		.addData("x", &Vector3::x)
		.addData("y", &Vector3::y)
		.addData("z", &Vector3::z)
		.addConstructor<void (*)(float, float, float)>() 
		.addStaticData("Zero", &v3_zero, false)
		.addStaticFunction("Lerp", static_cast<Vector3 (*)(const Vector3&, const Vector3&, float)>(&Vector3::Lerp))
		.addStaticFunction("Add", &Vector3Add)
		.addStaticFunction("Sub", &Vector3Sub)
		.addStaticFunction("Inverse", &Vector3Inverse)
		.addStaticFunction("Mul", &Vector3Mul)
		.addStaticFunction("MulScalar", &Vector3MulScalar)
		.addFunction("Normalize", static_cast<void (Vector3::*)(void)>(&Vector3::Normalize))
		.addFunction("Dot", &Vector3::Dot)
		.endClass()

		.beginClass<Vector2>("Vector2")
		.addData("x", &Vector2::x)
		.addData("y", &Vector2::y)
		.addConstructor<void (*)(float, float)>() 
		.addStaticData("Zero", &v2_zero, false)
		.addStaticFunction("Lerp", static_cast<Vector2 (*)(const Vector2&, const Vector2&, float)>(&Vector2::Lerp))
		.addStaticFunction("Add", &Vector2Add)
		.addStaticFunction("Sub", &Vector2Sub)
		.addStaticFunction("Inverse", &Vector2Inverse)
		.addStaticFunction("Mul", &Vector2Mul)
		.addStaticFunction("MulScalar", &Vector2MulScalar)
		.addFunction("Normalize", static_cast<void (Vector2::*)(void)>(&Vector2::Normalize))
		.addFunction("Dot", &Vector3::Dot)
		.endClass()

		.beginClass<MLRECT>("Rect")
		.addData("l", &MLRECT::left)
		.addData("t", &MLRECT::top)
		.addData("w", &MLRECT::width)
		.addData("h", &MLRECT::height)
		.addConstructor<void (*)(int, int, int, int)>() 
		.endClass()

		.beginClass<RECT>("WinRect")
		.addData("l", &RECT::left)
		.addData("t", &RECT::top)
		.addData("r", &RECT::right)
		.addData("b", &RECT::bottom)
		.addConstructor<void (*)(void)>()
		.endClass()

		.beginClass<POINT>("Point")
		.addData("x", &POINT::x)
		.addData("y", &POINT::y)
		.endClass();
}