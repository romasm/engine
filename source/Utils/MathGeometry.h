#pragma once

#include "stdafx.h"
#include "Log.h"
#include "Util.h"

#define ANG_RAD90 90.0f*XM_PI/180.0f
#define ANG_RAD45 45.0f*XM_PI/180.0f

using namespace EngineCore;

class BoundingFrustumEx
{
public:
	BoundingFrustumEx()
	{
		ortho = false;
	}
	BoundingFrustumEx(BoundingFrustum& f, XMMATRIX* world_view, XMMATRIX* proj)
	{
		ortho = false;
		perspective = f;
		if(world_view && proj)
		{
			WV = *world_view;
			P = *proj;
		}
		else
			WV = P = XMMatrixIdentity();
	}
	BoundingFrustumEx(BoundingOrientedBox& b, XMMATRIX* world_view, XMMATRIX* proj)
	{
		ortho = true;
		orthographic = b;
		if(world_view && proj)
		{
			WV = *world_view;
			P = *proj;
		}
		else
			WV = P = XMMatrixIdentity();
	}

	void Create(BoundingFrustum& f, XMMATRIX* world_view, XMMATRIX* proj)
	{
		ortho = false;
		perspective = f;
		if(world_view && proj)
		{
			WV = *world_view;
			P = *proj;
		}
		else
			WV = P = XMMatrixIdentity();
	}

	void Create(BoundingOrientedBox& b, XMMATRIX* world_view, XMMATRIX* proj)
	{
		ortho = true;
		orthographic = b;
		if(world_view && proj)
		{
			WV = *world_view;
			P = *proj;
		}
		else
			WV = P = XMMatrixIdentity();
	}

	inline Vector3 GetViewPoint() const
	{
		if(ortho) return VECTOR3_CAST(orthographic.Center);
		else return VECTOR3_CAST(perspective.Origin);
	}

	inline CXMMATRIX GetWV() const {return WV;}
	inline CXMMATRIX GetProj() const {return P;}
	
	inline ContainmentType Contains(FXMVECTOR Point) const
	{
		if(ortho)
			return orthographic.Contains(Point);
		else
			return perspective.Contains(Point);
	}

	inline ContainmentType Contains(FXMVECTOR V0, FXMVECTOR V1, FXMVECTOR V2) const
	{
		if(ortho)
			return orthographic.Contains(V0, V1, V2);
		else
			return perspective.Contains(V0, V1, V2);
	}

	inline ContainmentType Contains(const BoundingSphere& sp) const
	{
		if(ortho)
			return orthographic.Contains(sp);
		else
			return perspective.Contains(sp);
	}
	
	inline ContainmentType Contains(const BoundingBox& box) const
	{
		if(ortho)
			return orthographic.Contains(box);
		else
			return perspective.Contains(box);
	}
	
	inline ContainmentType Contains(const BoundingOrientedBox& box) const
	{
		if(ortho)
			return orthographic.Contains(box);
		else
			return perspective.Contains(box);
	}
	
	inline ContainmentType Contains(const BoundingFrustum& f) const
	{
		if(ortho)
			return orthographic.Contains(f);
		else
			return perspective.Contains(f);
	}

	ALIGNED_ALLOCATION

private:
	bool ortho;

	BoundingFrustum perspective;
	BoundingOrientedBox orthographic;

	XMMATRIX WV;
	XMMATRIX P;
};

template<class pointer, class compare, class swapdata, class array_pointer>
inline pointer _QS_partition(pointer begin, pointer end, compare compare_func, swapdata swap_func, array_pointer p_array)
{
	pointer i = begin;
	for(pointer j = begin + 1; j < end; j++)
	{
		if(compare_func(*begin, *j))
		{
			i = i + 1;
			if(i != j)
				swap_func(i, j, p_array);
		}
	}
	if(i != begin)
		swap_func(i, begin, p_array);
	return i;
}

// compare_func result must be inverted compare to std::sort
template<class pointer, class compare, class swapdata, class array_pointer>
inline void QSortSwap(pointer begin, pointer end, compare compare_func, swapdata swap_func, array_pointer p_array)
{
	if(begin >= end)
		return;
    
    pointer mid = _QS_partition(begin, end, compare_func, swap_func, p_array);
    QSortSwap(begin, mid, compare_func, swap_func, p_array);
    QSortSwap(mid + 1, end, compare_func, swap_func, p_array);
}

template<class pointer, class compare, class swapdata, class first_array_pointer, class second_array_pointer>
inline pointer _QS_partition(pointer begin, pointer end, compare compare_func, swapdata swap_func, first_array_pointer p_array, second_array_pointer q_array)
{
	pointer i = begin;
	for(pointer j = begin + 1; j < end; j++)
	{
		if(compare_func(*begin, *j))
		{
			i = i + 1;
			if(i != j)
				swap_func(i, j, p_array, q_array);
		}
	}
	if(i != begin)
		swap_func(i, begin, p_array, q_array);
	return i;
}

// compare_func result must be inverted compare to std::sort
template<class pointer, class compare, class swapdata, class first_array_pointer, class second_array_pointer>
inline void QSortSwap(pointer begin, pointer end, compare compare_func, swapdata swap_func, first_array_pointer p_array, second_array_pointer q_array)
{
	if(begin >= end)
		return;
    
    pointer mid = _QS_partition(begin, end, compare_func, swap_func, p_array, q_array);
    QSortSwap(begin, mid, compare_func, swap_func, p_array, q_array);
    QSortSwap(mid + 1, end, compare_func, swap_func, p_array, q_array);
}

/*inline bool RayBoxIntersect(XMVECTOR ray_origin, XMVECTOR ray_dir, Vector3 box_min, Vector3 box_max, CXMMATRIX box_transform, float min_dist, float max_dist)
{
	XMVECTOR box_pos, box_rot, box_scale;
	if(!XMMatrixDecompose(&box_scale, &box_rot, &box_pos, box_transform))
		return false;

	XMMATRIX box_pos_m, box_scale_m, ray_rot_m;

	box_pos_m = XMMatrixTranslationFromVector(box_pos);
	box_scale_m = XMMatrixScalingFromVector(box_scale);
	ray_rot_m = XMMatrixRotationQuaternion(box_rot);
	ray_rot_m = XMMatrixInverse(nullptr, ray_rot_m);

	XMVECTOR box_origin = XMVectorZero();
	box_origin = XMVector3Transform(box_origin, box_pos_m);
	XMVECTOR l_ray_origin = ray_origin - box_origin;
	l_ray_origin = XMVector3Transform(l_ray_origin, ray_rot_m);
	XMVECTOR l_ray_dir = XMVector3Transform(ray_dir, ray_rot_m);

	XMVECTOR l_box_min = XMLoadFloat3(&box_min);
	XMVECTOR l_box_max = XMLoadFloat3(&box_max);
	l_box_min = XMVector3Transform(l_box_min, box_scale_m);
	l_box_max = XMVector3Transform(l_box_max, box_scale_m);

	Vector3 f_ray_dir, f_ray_origin;
	Vector3 bounds[2];
	XMStoreFloat3(&f_ray_dir, l_ray_dir); 
	XMStoreFloat3(&f_ray_origin, l_ray_origin); 
	XMStoreFloat3(&bounds[0], l_box_min); 
	XMStoreFloat3(&bounds[1], l_box_max); 
	

	Vector3 ray_inv_dir = Vector3( 1.0f/f_ray_dir.x, 1.0f/f_ray_dir.y, 1.0f/f_ray_dir.z );
	int ray_sign[3];
	ray_sign[0] = (ray_inv_dir.x < 0);
	ray_sign[1] = (ray_inv_dir.y < 0);
	ray_sign[2] = (ray_inv_dir.z < 0);

	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	tmin = ( bounds[ray_sign[0]].x - f_ray_origin.x ) * ray_inv_dir.x;
	tmax = ( bounds[1 - ray_sign[0]].x - f_ray_origin.x ) * ray_inv_dir.x;
	tymin = ( bounds[ray_sign[1]].y - f_ray_origin.y ) * ray_inv_dir.y;
	tymax = ( bounds[1 - ray_sign[1]].y - f_ray_origin.y ) * ray_inv_dir.y;

	if ( (tmin > tymax) || (tymin > tmax) )
		return false;
	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;

	tzmin = (bounds[ray_sign[2]].z - f_ray_origin.z) * ray_inv_dir.z;
	tzmax = (bounds[1 - ray_sign[2]].z - f_ray_origin.z) * ray_inv_dir.z;

	if ( (tmin > tzmax) || (tzmin > tmax) )
		return false;
	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;

	return ( (tmin < max_dist) && (tmax > min_dist) );
}*/

inline void BoundingOrientedBoxTransformFixed( BoundingOrientedBox& In, CXMMATRIX M )
{
    // Load the box.
    XMVECTOR vCenter = XMLoadFloat3( &In.Center );
    XMVECTOR vExtents = XMLoadFloat3( &In.Extents );
    XMVECTOR vOrientation = XMLoadFloat4( &In.Orientation );

    assert( DirectX::Internal::XMQuaternionIsUnit( vOrientation ) );

    // Composite the box rotation and the transform rotation.
    XMVECTOR Rotation, Scale, Translate;
	Rotation = XMVectorSet(0, 0, 0, 1.0f);
	if(!XMMatrixDecompose(&Scale, &Rotation, &Translate, M))
		ERR("XMMatrixDecompose failed!");

    vOrientation = XMQuaternionMultiply( vOrientation, Rotation );

    // Transform the center.
    vCenter = XMVector3Transform( vCenter, M );

    // Scale the box extents.
    vExtents = XMVectorMultiply(vExtents, Scale);

    // Store the box.
    XMStoreFloat3( &In.Center, vCenter );
    XMStoreFloat3( &In.Extents, vExtents );
    XMStoreFloat4( &In.Orientation, vOrientation );
}

inline float RayBoxIntersect(XMVECTOR ray_origin, XMVECTOR ray_dir, BoundingBox box, CXMMATRIX box_transform)
{
	BoundingOrientedBox n_box(box.Center, box.Extents, QUAT_ROT_NULL);
	BoundingOrientedBoxTransformFixed(n_box, box_transform);

	float dist;
	if(!n_box.Intersects(ray_origin, XMVector3Normalize(ray_dir), dist))
		return -1.0f;
	else
		return dist;
}

inline float RayOrientedBoxIntersect(XMVECTOR ray_origin, XMVECTOR ray_dir, BoundingOrientedBox box)
{
	float dist;
	if(!box.Intersects(ray_origin, XMVector3Normalize(ray_dir), dist))
		return -1.0f;
	else
		return dist;
}

inline XMMATRIX TransformationFromViewPos(CXMMATRIX view, Vector3 pos)
{
	XMMATRIX transf = XMMatrixIdentity();
	XMVECTOR scale, rot, translate;
	XMMatrixDecompose(&scale, &rot, &translate, XMMatrixTranspose(view));
	transf *= XMMatrixRotationQuaternion(rot);
	transf *= XMMatrixTranslation(pos.x, pos.y, pos.z);
	return transf;
}

inline XMMATRIX TransformationFromViewPos(CXMMATRIX view, XMVECTOR pos)
{
	XMMATRIX transf = XMMatrixIdentity();
	XMVECTOR scale, rot, translate;
	XMMatrixDecompose(&scale, &rot, &translate, XMMatrixTranspose(view));
	transf *= XMMatrixRotationQuaternion(rot);
	transf *= XMMatrixTranslationFromVector(pos);
	return transf;
}

inline void DirectionToPitchYaw(Vector3 dir_float, float *pitch, float* yaw)
{
	XMVECTOR yaw_null = XMVectorSet(1.0f,0.0f,0.0f,0.0f);
	XMVECTOR pitch_null = XMVectorSet(0.0f,1.0f,0.0f,0.0f);
	XMVECTOR roll_null = XMVectorSet(1.0f,0.0f,0.0f,0.0f);

	if(!yaw) return;
	if(!pitch) return;

	XMVECTOR dir_vect = XMLoadFloat3(&dir_float); 

	XMVECTOR yaw_proj = XMVectorSet(dir_float.x, dir_float.y, 0.0f, 0.0f); 
	*pitch = acos( XMVectorGetX(XMVector3Dot(dir_vect, yaw_proj) / (XMVectorGetX(XMVector3Length(dir_vect)) * XMVectorGetX(XMVector3Length(yaw_proj)))) );
	if(dir_float.z>0)*pitch = -(*pitch); 

	XMVECTOR pitch_proj = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f); 
	*yaw = acos( XMVectorGetX(XMVector3Dot(yaw_proj, pitch_proj) / (XMVectorGetX(XMVector3Length(yaw_proj)) * XMVectorGetX(XMVector3Length(pitch_proj)))) );
	if(dir_float.y<0)*yaw = -(*yaw);
}

inline void DirectionToPitchYaw(XMVECTOR dir, float *pitch, float* yaw)
{
	Vector3 dir_float;
	XMStoreFloat3(&dir_float, dir);

	DirectionToPitchYaw(dir_float, pitch, yaw);
}

inline void QuaternionToPitchYawRoll(XMVECTOR quat, float* pitch, float* yaw, float* roll)
{
	Vector4 q;
	XMStoreFloat4(&q, quat);

	/*XMVECTOR dir;
	float angle;
	XMQuaternionToAxisAngle(&dir, &angle, quat);
	Vector4 d;
	XMStoreFloat4(&d, dir);

	*pitch = -atan2f(d.y, sqrt( d.x*d.x + d.z*d.z ));
	*yaw = atan2f( d.x, d.z );
	*roll = angle;
	*/
	*roll = atan2( 2*(q.x*q.y + q.z*q.w) , (1 - 2*(q.y*q.y + q.z*q.z)) );
	*yaw = -asin(2*(q.x*q.z - q.w*q.y));
	*pitch = -atan( 2*(q.x*q.w + q.y*q.z) / (1 - 2*(q.z*q.z + q.w*q.w)) );
	/*
	*roll = atan( 2*(q.y*q.w + q.x*q.z) / (1 - 2*(q.y*q.y + q.z*q.z)) );
	*yaw = asin(2*(q.x*q.y - q.w*q.z));
	*pitch = atan( 2*(q.x*q.w + q.y*q.z) / (1 - 2*(q.z*q.z + q.x*q.x)) );*/
}

inline float YawFromMatrix(CXMMATRIX M)
{
	XMVECTOR z_unit = XMVectorSet(0, 0, 1, 0);
	XMVECTOR rot = XMVector3Normalize(XMVector3TransformNormal(z_unit, M));
	return atan2f(XMVectorGetX(rot), XMVectorGetZ(rot));
}

inline float PitchFromMatrix(CXMMATRIX M)
{
	XMVECTOR z_unit = XMVectorSet(0, 0, 1, 0);
	XMVECTOR rot = XMVector3Normalize(XMVector3TransformNormal(z_unit, M));
	return -asinf(clamp(-1.0f, XMVectorGetY(rot), 1.0f));
}

inline float RollFromMatrix(CXMMATRIX M)
{
	XMVECTOR z_unit = XMVectorSet(0, 0, 1, 0);
	XMVECTOR z_rot = XMVector3Normalize(XMVector3TransformNormal(z_unit, M));
	Vector3 z_rot_f;
	XMStoreFloat3(&z_rot_f, z_rot);

	XMVECTOR x_unit = XMVectorSet(1, 0, 0, 0);
	XMVECTOR x_rot = XMVector3Normalize(XMVector3TransformNormal(x_unit, M));
	Vector3 x_rot_f;
	XMStoreFloat3(&x_rot_f, x_rot);

	const float z_proj_len = sqrtf(z_rot_f.z * z_rot_f.z + z_rot_f.x * z_rot_f.x);
	const float t = (z_rot_f.z * x_rot_f.x - z_rot_f.x * x_rot_f.z) / z_proj_len;

	if (t <= -1.0f) return XM_PI;
	if (abs(z_proj_len) < numeric_limits<float>::epsilon()  || t >= 1.0f) return 0.0f;

	float roll = acosf(clamp(-1.0f, t, 1.0f));

	return x_rot_f.y < 0.0f ? -roll : roll;
}

inline Vector3 PYRFromMatrix(CXMMATRIX M)
{
	Vector3 res;
	
	XMVECTOR z_unit = XMVectorSet(0, 0, 1, 0);
	XMVECTOR z_rot = XMVector3Normalize(XMVector3TransformNormal(z_unit, M));
	Vector3 z_rot_f;
	XMStoreFloat3(&z_rot_f, z_rot);

	XMVECTOR x_unit = XMVectorSet(1, 0, 0, 0);
	XMVECTOR x_rot = XMVector3Normalize(XMVector3TransformNormal(x_unit, M));
	Vector3 x_rot_f;
	XMStoreFloat3(&x_rot_f, x_rot);

	res.x = -asinf(clamp(-1.0f, z_rot_f.y, 1.0f));
	res.y = atan2f(z_rot_f.x, z_rot_f.z);

	const float z_proj_len = sqrtf(z_rot_f.z * z_rot_f.z + z_rot_f.x * z_rot_f.x);
	const float t = (z_rot_f.z * x_rot_f.x - z_rot_f.x * x_rot_f.z) / z_proj_len;

	if (t <= -1.0f) res.z = XM_PI;
	else if(abs(z_proj_len) < numeric_limits<float>::epsilon()  || t >= 1.0f) res.z = 0.0f;
	else
	{
		float roll = acosf(clamp(-1.0f, t, 1.0f));
		res.z = x_rot_f.y < 0.0f ? -roll : roll;
	}

	return res;
}

inline float YawFromQuat(XMVECTOR Q)
{
	XMVECTOR z_unit = XMVectorSet(0, 0, 1, 0);
	XMVECTOR rot = XMVector3Normalize(XMVector3Rotate(z_unit, Q));
	return atan2f(XMVectorGetX(rot), XMVectorGetZ(rot));
}

inline float PitchFromQuat(XMVECTOR Q)
{
	XMVECTOR z_unit = XMVectorSet(0, 0, 1, 0);
	XMVECTOR rot = XMVector3Normalize(XMVector3Rotate(z_unit, Q));
	return -asinf(clamp(-1.0f, XMVectorGetY(rot), 1.0f));
}

inline float RollFromQuat(XMVECTOR Q)
{
	XMVECTOR z_unit = XMVectorSet(0, 0, 1, 0);
	XMVECTOR z_rot = XMVector3Normalize(XMVector3Rotate(z_unit, Q));
	Vector3 z_rot_f;
	XMStoreFloat3(&z_rot_f, z_rot);

	XMVECTOR x_unit = XMVectorSet(1, 0, 0, 0);
	XMVECTOR x_rot = XMVector3Normalize(XMVector3Rotate(x_unit, Q));
	Vector3 x_rot_f;
	XMStoreFloat3(&x_rot_f, x_rot);

	const float z_proj_len = sqrtf(z_rot_f.z * z_rot_f.z + z_rot_f.x * z_rot_f.x);
	const float t = (z_rot_f.z * x_rot_f.x - z_rot_f.x * x_rot_f.z) / z_proj_len;

	if (t <= -1.0f) return XM_PI;
	if (abs(z_proj_len) < numeric_limits<float>::epsilon()  || t >= 1.0f) return 0.0f;

	float roll = acosf(clamp(-1.0f, t, 1.0f));

	return x_rot_f.y < 0.0f ? -roll : roll;
}

inline Vector3 PYRFromQuat(XMVECTOR Q)
{
	Vector3 res;
	
	XMVECTOR z_unit = XMVectorSet(0, 0, 1, 0);
	XMVECTOR z_rot = XMVector3Normalize(XMVector3Rotate(z_unit, Q));
	Vector3 z_rot_f;
	XMStoreFloat3(&z_rot_f, z_rot);

	XMVECTOR x_unit = XMVectorSet(1, 0, 0, 0);
	XMVECTOR x_rot = XMVector3Normalize(XMVector3Rotate(x_unit, Q));
	Vector3 x_rot_f;
	XMStoreFloat3(&x_rot_f, x_rot);

	res.x = -asinf(clamp(-1.0f, z_rot_f.y, 1.0f));
	res.y = atan2f(z_rot_f.x, z_rot_f.z);

	const float z_proj_len = sqrtf(z_rot_f.z * z_rot_f.z + z_rot_f.x * z_rot_f.x);
	const float t = (z_rot_f.z * x_rot_f.x - z_rot_f.x * x_rot_f.z) / z_proj_len;

	if (t <= -1.0f) res.z = XM_PI;
	else if(abs(z_proj_len) < numeric_limits<float>::epsilon()  || t >= 1.0f) res.z = 0.0f;
	else
	{
		float roll = acosf(clamp(-1.0f, t, 1.0f));
		res.z = x_rot_f.y < 0.0f ? -roll : roll;
	}

	return res;
}

inline bool PlaneBoxOverlap(Vector3& normal, Vector3& vert, Vector3& boxExtents)
{
	Vector3 vMin, vMax;

	if (normal.x > 0.0f)
	{
		vMin.x = -boxExtents.x - vert.x;
		vMax.x = boxExtents.x - vert.x;
	}
	else
	{
		vMin.x = boxExtents.x - vert.x;
		vMax.x = -boxExtents.x - vert.x;
	}

	if (normal.y > 0.0f)
	{
		vMin.y = -boxExtents.y - vert.y;
		vMax.y = boxExtents.y - vert.y;
	}
	else
	{
		vMin.y = boxExtents.y - vert.y;
		vMax.y = -boxExtents.y - vert.y;
	}

	if (normal.z > 0.0f)
	{
		vMin.z = -boxExtents.z - vert.z;
		vMax.z = boxExtents.z - vert.z;
	}
	else
	{
		vMin.z = boxExtents.z - vert.z;
		vMax.z = -boxExtents.z - vert.z;
	}

	if (normal.Dot(vMin) > 0.0f)
		return false;

	if (normal.Dot(vMax) >= 0.0f)
		return true;

	return false;
}

/*======================== X-tests ========================*/
#define AXISTEST_X01(a, b, fa, fb)			   \
p0 = a*v0.y - b*v0.z;			       	   \
p2 = a*v2.y - b*v2.z;			       	   \
if (p0<p2) { minV = p0; maxV = p2; } else { minV = p2; maxV = p0; } \
rad = fa * boxExtents.y + fb * boxExtents.z;   \
if (minV>rad || maxV<-rad) return false;

#define AXISTEST_X2(a, b, fa, fb)			   \
p0 = a*v0.y - b*v0.z;			           \
p1 = a*v1.y - b*v1.z;			       	   \
if (p0<p1) { minV = p0; maxV = p1; } else { minV = p1; maxV = p0; } \
rad = fa * boxExtents.y + fb * boxExtents.z;   \
if (minV>rad || maxV<-rad) return false;

/*======================== Y-tests ========================*/
#define AXISTEST_Y02(a, b, fa, fb)			   \
p0 = -a*v0.x + b*v0.z;		      	   \
p2 = -a*v2.x + b*v2.z;	       	       	   \
if (p0<p2) { minV = p0; maxV = p2; } else { minV = p2; maxV = p0; } \
rad = fa * boxExtents.x + fb * boxExtents.z;   \
if (minV>rad || maxV<-rad) return false;

#define AXISTEST_Y1(a, b, fa, fb)			   \
p0 = -a*v0.x + b*v0.z;		      	   \
p1 = -a*v1.x + b*v1.z;	     	       	   \
if (p0<p1) { minV = p0; maxV = p1; } else { minV = p1; maxV = p0; } \
rad = fa * boxExtents.x + fb * boxExtents.z;   \
if (minV>rad || maxV<-rad) return false;

/*======================== Z-tests ========================*/
#define AXISTEST_Z12(a, b, fa, fb)			   \
p1 = a*v1.x - b*v1.y;			           \
p2 = a*v2.x - b*v2.y;			       	   \
if (p2<p1) { minV = p2; maxV = p1; } else { minV = p1; maxV = p2; } \
rad = fa * boxExtents.x + fb * boxExtents.y;   \
if (minV>rad || maxV<-rad) return false;

#define AXISTEST_Z0(a, b, fa, fb)			   \
p0 = a*v0.x - b*v0.y;				   \
p1 = a*v1.x - b*v1.y;			           \
if (p0<p1) { minV = p0; maxV = p1; } else { minV = p1; maxV = p0; } \
rad = fa * boxExtents.x + fb * boxExtents.y;   \
if (minV>rad || maxV<-rad) return false;

#define FINDMINMAX(x0,x1,x2,minV,maxV) \
minV = maxV = x0;   \
if (x1<minV) minV = x1; \
if (x1>maxV) maxV = x1; \
if (x2<minV) minV = x2; \
if (x2>maxV) maxV = x2;

inline bool TriBoxOverlap(const BoundingBox& bbox, Vector3 vertecies[3])
{
	float minV, maxV, p0, p1, p2, rad;

	Vector3 boxCenter = bbox.Center;
	Vector3 boxExtents = bbox.Extents;

	Vector3 v0, v1, v2;
	v0 = vertecies[0] - boxCenter;
	v1 = vertecies[1] - boxCenter;
	v2 = vertecies[2] - boxCenter;

	Vector3 e0, e1, e2;
	e0 = v1 - v0;
	e1 = v2 - v1;
	e2 = v0 - v2;

	float fex, fey, fez;
	fex = fabsf(e0.x);
	fey = fabsf(e0.y);
	fez = fabsf(e0.z);
	
	AXISTEST_X01(e0.z, e0.y, fez, fey);
	AXISTEST_Y02(e0.z, e0.x, fez, fex);
	AXISTEST_Z12(e0.y, e0.x, fey, fex);

	fex = fabsf(e1.x);
	fey = fabsf(e1.y);
	fez = fabsf(e1.z);

	AXISTEST_X01(e1.z, e1.y, fez, fey);
	AXISTEST_Y02(e1.z, e1.x, fez, fex);
	AXISTEST_Z0(e1.y, e1.x, fey, fex);

	fex = fabsf(e2.x);
	fey = fabsf(e2.y);
	fez = fabsf(e2.z);

	AXISTEST_X2(e2.z, e2.y, fez, fey);
	AXISTEST_Y1(e2.z, e2.x, fez, fex);
	AXISTEST_Z12(e2.y, e2.x, fey, fex);

	FINDMINMAX(v0.x, v1.x, v2.x, minV, maxV);
	if (minV > boxExtents.x || maxV < -boxExtents.x) 
		return false;

	FINDMINMAX(v0.y, v1.y, v2.y, minV, maxV);
	if (minV > boxExtents.y || maxV < -boxExtents.y)
		return false;

	FINDMINMAX(v0.z, v1.z, v2.z, minV, maxV);
	if (minV > boxExtents.z || maxV < -boxExtents.z)
		return false;

	Vector3 normal = e0.Cross(e1);
	if (!PlaneBoxOverlap(normal, v0, boxExtents))
		return false;

	return true;
}

enum TriClipping
{
	TC_BOTH = 0,
	TC_FRONT,
	TC_BACK
};

struct TriExplicit
{
	Vector3 v[3];
};

// 0.0f - no intersection
inline float TriRayIntersect(const Vector3& origin, const Vector3& dir, Vector3 vertecies[3], bool& isFront)
{
	isFront = false;

	float dist;
	XMVECTOR dirV = dir;
	XMVECTOR v0 = vertecies[0];
	XMVECTOR v1 = vertecies[1];
	XMVECTOR v2 = vertecies[2];

	bool isIntersect = TriangleTests::Intersects(XMVECTOR(origin), dirV, v0, v1, v2, dist);

	if (!isIntersect)
		return 0.0f;

	XMVECTOR normal = XMVector3Cross(XMVectorSubtract(v1, v0), XMVectorSubtract(v2, v0));

	if (XMVectorGetX(XMVector3Dot(normal, dir)) < 0.0)
		isFront = true;
	
	return dist;
}

// 0.0f - no intersection
inline float TrisArrayRayIntersect(const Vector3& origin, const Vector3& dir, DArray<TriExplicit>& tris, float maxDist, TriClipping triClipping, bool& isFront)
{
	float minDist = 99999990000.0f;
	bool isIntersect = false;

	for (auto& tri : tris)
	{
		bool isFrontTri;
		float dist = TriRayIntersect(origin, dir, tri.v, isFrontTri);

		if (dist == 0.0f || dist >= maxDist)
			continue;

		if (triClipping == TriClipping::TC_BOTH ||
			(triClipping == TriClipping::TC_FRONT && isFrontTri) ||
			(triClipping == TriClipping::TC_BACK && !isFrontTri))
		{
			if (dist < minDist)
			{
				minDist = dist;
				isFront = isFrontTri;
			}
			isIntersect = true;
		}
	}

	if (!isIntersect)
		return 0.0f;

	return minDist;
}