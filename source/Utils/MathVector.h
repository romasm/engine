#pragma once

#include "stdafx.h"
#include "Log.h"
#include "DataTypes.h"

using namespace EngineCore;

__forceinline Vector3 LRHconvertion(const Vector3& v)
{
	return Vector3(v.x, v.y, -v.z);
}

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

static Quaternion qIdent = Quaternion::Identity; 
static Vector2 v2Zero = Vector2::Zero; 
static Vector3 v3Zero = Vector3::Zero; 
static Vector4 v4Zero = Vector4::Zero; 
static Vector2 v2UnitX = Vector2::UnitX; 
static Vector2 v2UnitY = Vector2::UnitY; 
static Vector3 v3UnitX = Vector3::UnitX; 
static Vector3 v3UnitY = Vector3::UnitY; 
static Vector3 v3UnitZ = Vector3::UnitZ; 
static Vector4 v4UnitX = Vector4::UnitX; 
static Vector4 v4UnitY = Vector4::UnitY; 
static Vector4 v4UnitZ = Vector4::UnitZ; 
static Vector4 v4UnitW = Vector4::UnitW; 

inline Vector4 Vector4Add(Vector4& v1, Vector4& v2)
{return v1 + v2;}
inline Vector3 Vector3Add(Vector3& v1, Vector3& v2)
{return v1 + v2;}
inline Vector2 Vector2Add(Vector2& v1, Vector2& v2)
{return v1 + v2;}
inline Quaternion QuaternionAdd(Quaternion& v1, Quaternion& v2)
{return v1 + v2;}

inline Vector4 Vector4Sub(Vector4& v1, Vector4& v2)
{return v1 - v2;}
inline Vector3 Vector3Sub(Vector3& v1, Vector3& v2)
{return v1 - v2;}
inline Vector2 Vector2Sub(Vector2& v1, Vector2& v2)
{return v1 - v2;}
inline Quaternion QuaternionSub(Quaternion& v1, Quaternion& v2)
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
inline Quaternion QuaternionMul(Quaternion& v1, Quaternion& v2)
{return v1 * v2;}

inline Vector4 Vector4Div(Vector4& v1, Vector4& v2)
{return v1 / v2;}
inline Vector3 Vector3Div(Vector3& v1, Vector3& v2)
{return v1 / v2;}
inline Vector2 Vector2Div(Vector2& v1, Vector2& v2)
{return v1 / v2;}
inline Quaternion QuaternionDiv(Quaternion& v1, Quaternion& v2)
{return v1 / v2;}

inline Vector4 Vector4MulScalar(Vector4& v1, float v2)
{return v1 * v2;}
inline Vector3 Vector3MulScalar(Vector3& v1, float v2)
{return v1 * v2;}
inline Vector2 Vector2MulScalar(Vector2& v1, float v2)
{return v1 * v2;}
inline Quaternion QuaternionMulScalar(Quaternion& v1, float v2)
{return v1 * v2;}

inline Vector4 Vector4CreatePlane(Vector3& pos, Vector3& norm)
{return XMPlaneFromPointNormal(pos, norm);}
inline Vector3 Vector4PlaneLineCollide(Vector4& plane, Vector3& pos, Vector3& rayTo)
{return XMPlaneIntersectLine(plane, pos, rayTo);}

inline void RegLuaMath()
{
	getGlobalNamespace(LSTATE)
		.beginClass<Quaternion>("Quaternion")
		.addData("x", &Quaternion::x)
		.addData("y", &Quaternion::y)
		.addData("z", &Quaternion::z)
		.addData("w", &Quaternion::w)
		.addConstructor<void (*)(float, float, float, float)>() 
		.addStaticData("Identity", &qIdent, false)
		.addStaticFunction("Add", &QuaternionAdd)
		.addStaticFunction("Sub", &QuaternionSub)
		.addStaticFunction("Mul", &QuaternionMul)
		.addStaticFunction("Div", &QuaternionDiv)
		.addStaticFunction("MulScalar", &QuaternionMulScalar)
		.addFunction("Normalize", static_cast<void (Quaternion::*)(void)>(&Quaternion::Normalize))
		.addStaticFunction("Lerp", static_cast<Quaternion (*)(const Quaternion&, const Quaternion&, float)>(&Quaternion::Lerp))
		.addStaticFunction("Slerp", static_cast<Quaternion (*)(const Quaternion&, const Quaternion&, float)>(&Quaternion::Slerp))
		.addFunction("Dot", &Quaternion::Dot)
		.addFunction("Length", &Quaternion::Length)
		.addFunction("LengthSq", &Quaternion::LengthSquared)
		.endClass()

		.beginClass<Vector2>("Vector2")
		.addData("x", &Vector2::x)
		.addData("y", &Vector2::y)
		.addConstructor<void (*)(float, float)>() 
		.addStaticData("Zero", &v2Zero, false)
		.addStaticData("UnitX", &v2UnitX, false)
		.addStaticData("UnitY", &v2UnitY, false)
		.addStaticFunction("Lerp", static_cast<Vector2 (*)(const Vector2&, const Vector2&, float)>(&Vector2::Lerp))
		.addStaticFunction("Add", &Vector2Add)
		.addStaticFunction("Sub", &Vector2Sub)
		.addStaticFunction("Inverse", &Vector2Inverse)
		.addStaticFunction("Mul", &Vector2Mul)
		.addStaticFunction("Div", &Vector2Div)
		.addStaticFunction("MulScalar", &Vector2MulScalar)
		.addFunction("Normalize", static_cast<void (Vector2::*)(void)>(&Vector2::Normalize))
		.addFunction("Dot", &Vector2::Dot)
		.addFunction("Length", &Vector2::Length)
		.addFunction("LengthSq", &Vector2::LengthSquared)
		.endClass()

		.beginClass<Vector3>("Vector3")
		.addData("x", &Vector3::x)
		.addData("y", &Vector3::y)
		.addData("z", &Vector3::z)
		.addConstructor<void (*)(float, float, float)>() 
		.addStaticData("Zero", &v3Zero, false)
		.addStaticData("UnitX", &v3UnitX, false)
		.addStaticData("UnitY", &v3UnitY, false)
		.addStaticData("UnitZ", &v3UnitZ, false)
		.addStaticFunction("Lerp", static_cast<Vector3 (*)(const Vector3&, const Vector3&, float)>(&Vector3::Lerp))
		.addStaticFunction("Rotate", static_cast<Vector3 (*)(const Vector3&, const Quaternion&)>(&Vector3::Transform))
		.addStaticFunction("Add", &Vector3Add)
		.addStaticFunction("Sub", &Vector3Sub)
		.addStaticFunction("Inverse", &Vector3Inverse)
		.addStaticFunction("Mul", &Vector3Mul)
		.addStaticFunction("Div", &Vector3Div)
		.addStaticFunction("MulScalar", &Vector3MulScalar)
		.addFunction("Normalize", static_cast<void (Vector3::*)(void)>(&Vector3::Normalize))
		.addFunction("Dot", &Vector3::Dot)
		.addFunction("Length", &Vector3::Length)
		.addFunction("LengthSq", &Vector3::LengthSquared)
		.addFunction("Cross", static_cast<Vector3 (Vector3::*)(const Vector3&) const>(&Vector3::Cross))
		.endClass()

		.beginClass<Vector4>("Vector4")
		.addData("x", &Vector4::x)
		.addData("y", &Vector4::y)
		.addData("z", &Vector4::z)
		.addData("w", &Vector4::w)
		.addConstructor<void (*)(float, float, float, float)>() 
		.addStaticData("Zero", &v4Zero, false)
		.addStaticData("UnitX", &v4UnitX, false)
		.addStaticData("UnitY", &v4UnitY, false)
		.addStaticData("UnitZ", &v4UnitZ, false)
		.addStaticData("UnitW", &v4UnitW, false)
		.addStaticFunction("Lerp", static_cast<Vector4 (*)(const Vector4&, const Vector4&, float)>(&Vector4::Lerp))
		.addStaticFunction("Rotate", static_cast<Vector4 (*)(const Vector4&, const Quaternion&)>(&Vector4::Transform))
		.addStaticFunction("Add", &Vector4Add)
		.addStaticFunction("Sub", &Vector4Sub)
		.addStaticFunction("Inverse", &Vector4Inverse)
		.addStaticFunction("Mul", &Vector4Mul)
		.addStaticFunction("Div", &Vector4Div)
		.addStaticFunction("MulScalar", &Vector4MulScalar)
		.addFunction("Normalize", static_cast<void (Vector4::*)(void)>(&Vector4::Normalize))
		.addFunction("Dot", &Vector4::Dot)
		.addFunction("Length", &Vector4::Length)
		.addFunction("LengthSq", &Vector4::LengthSquared)
		.addFunction("Cross", static_cast<Vector4 (Vector4::*)(const Vector4&, const Vector4&) const>(&Vector4::Cross))

		.addStaticFunction("CreatePlane", &Vector4CreatePlane)
		.addStaticFunction("PlaneLineCollide", &Vector4PlaneLineCollide)
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