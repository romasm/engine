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

	inline XMFLOAT3 GetViewPoint() const
	{
		if(ortho) return orthographic.Center;
		else return perspective.Origin;
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

template<class pointer, class compare, class swapdata, class array_pointer>
inline void QSortSwap(pointer begin, pointer end, compare compare_func, swapdata swap_func, array_pointer p_array)
{
	if(begin >= end)
		return;
    
    pointer mid = _QS_partition(begin, end, compare_func, swap_func, p_array);
    QSortSwap(begin, mid, compare_func, swap_func, p_array);
    QSortSwap(mid + 1, end, compare_func, swap_func, p_array);
}

/*inline bool RayBoxIntersect(XMVECTOR ray_origin, XMVECTOR ray_dir, XMFLOAT3 box_min, XMFLOAT3 box_max, CXMMATRIX box_transform, float min_dist, float max_dist)
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

	XMFLOAT3 f_ray_dir, f_ray_origin;
	XMFLOAT3 bounds[2];
	XMStoreFloat3(&f_ray_dir, l_ray_dir); 
	XMStoreFloat3(&f_ray_origin, l_ray_origin); 
	XMStoreFloat3(&bounds[0], l_box_min); 
	XMStoreFloat3(&bounds[1], l_box_max); 
	

	XMFLOAT3 ray_inv_dir = XMFLOAT3( 1.0f/f_ray_dir.x, 1.0f/f_ray_dir.y, 1.0f/f_ray_dir.z );
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
	XMMatrixDecompose(&Scale, &Rotation, &Translate, M);

    vOrientation = XMQuaternionMultiply( vOrientation, Rotation );

    // Transform the center.
    vCenter = XMVector3Transform( vCenter, M );

    // Scale the box extents.
    vExtents = vExtents * Scale;

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

inline XMMATRIX TransformationFromViewPos(CXMMATRIX view, XMFLOAT3 pos)
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

inline void DirectionToPitchYaw(XMFLOAT3 dir_float, float *pitch, float* yaw)
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
	XMFLOAT3 dir_float;
	XMStoreFloat3(&dir_float, dir);

	DirectionToPitchYaw(dir_float, pitch, yaw);
}

inline void QuaternionToPitchYawRoll(XMVECTOR quat, float* pitch, float* yaw, float* roll)
{
	XMFLOAT4 q;
	XMStoreFloat4(&q, quat);

	/*XMVECTOR dir;
	float angle;
	XMQuaternionToAxisAngle(&dir, &angle, quat);
	XMFLOAT4 d;
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
	XMFLOAT3 z_rot_f;
	XMStoreFloat3(&z_rot_f, z_rot);

	XMVECTOR x_unit = XMVectorSet(1, 0, 0, 0);
	XMVECTOR x_rot = XMVector3Normalize(XMVector3TransformNormal(x_unit, M));
	XMFLOAT3 x_rot_f;
	XMStoreFloat3(&x_rot_f, x_rot);

	const float z_proj_len = sqrtf(z_rot_f.z * z_rot_f.z + z_rot_f.x * z_rot_f.x);
	const float t = (z_rot_f.z * x_rot_f.x - z_rot_f.x * x_rot_f.z) / z_proj_len;

	if (t <= -1.0f) return XM_PI;
	if (abs(z_proj_len) < numeric_limits<float>::epsilon()  || t >= 1.0f) return 0.0f;

	float roll = acosf(clamp(-1.0f, t, 1.0f));

	return x_rot_f.y < 0.0f ? -roll : roll;
}

inline XMFLOAT3 PYRFromMatrix(CXMMATRIX M)
{
	XMFLOAT3 res;
	
	XMVECTOR z_unit = XMVectorSet(0, 0, 1, 0);
	XMVECTOR z_rot = XMVector3Normalize(XMVector3TransformNormal(z_unit, M));
	XMFLOAT3 z_rot_f;
	XMStoreFloat3(&z_rot_f, z_rot);

	XMVECTOR x_unit = XMVectorSet(1, 0, 0, 0);
	XMVECTOR x_rot = XMVector3Normalize(XMVector3TransformNormal(x_unit, M));
	XMFLOAT3 x_rot_f;
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
	XMFLOAT3 z_rot_f;
	XMStoreFloat3(&z_rot_f, z_rot);

	XMVECTOR x_unit = XMVectorSet(1, 0, 0, 0);
	XMVECTOR x_rot = XMVector3Normalize(XMVector3Rotate(x_unit, Q));
	XMFLOAT3 x_rot_f;
	XMStoreFloat3(&x_rot_f, x_rot);

	const float z_proj_len = sqrtf(z_rot_f.z * z_rot_f.z + z_rot_f.x * z_rot_f.x);
	const float t = (z_rot_f.z * x_rot_f.x - z_rot_f.x * x_rot_f.z) / z_proj_len;

	if (t <= -1.0f) return XM_PI;
	if (abs(z_proj_len) < numeric_limits<float>::epsilon()  || t >= 1.0f) return 0.0f;

	float roll = acosf(clamp(-1.0f, t, 1.0f));

	return x_rot_f.y < 0.0f ? -roll : roll;
}

inline XMFLOAT3 PYRFromQuat(XMVECTOR Q)
{
	XMFLOAT3 res;
	
	XMVECTOR z_unit = XMVectorSet(0, 0, 1, 0);
	XMVECTOR z_rot = XMVector3Normalize(XMVector3Rotate(z_unit, Q));
	XMFLOAT3 z_rot_f;
	XMStoreFloat3(&z_rot_f, z_rot);

	XMVECTOR x_unit = XMVectorSet(1, 0, 0, 0);
	XMVECTOR x_rot = XMVector3Normalize(XMVector3Rotate(x_unit, Q));
	XMFLOAT3 x_rot_f;
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