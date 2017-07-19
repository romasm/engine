/********************************************************************************
* ReactPhysics3D physics library, http://www.reactphysics3d.com                 *
* Copyright (c) 2010-2016 Daniel Chappuis                                       *
*********************************************************************************
*                                                                               *
* This software is provided 'as-is', without any express or implied warranty.   *
* In no event will the authors be held liable for any damages arising from the  *
* use of this software.                                                         *
*                                                                               *
* Permission is granted to anyone to use this software for any purpose,         *
* including commercial applications, and to alter it and redistribute it        *
* freely, subject to the following restrictions:                                *
*                                                                               *
* 1. The origin of this software must not be misrepresented; you must not claim *
*    that you wrote the original software. If you use this software in a        *
*    product, an acknowledgment in the product documentation would be           *
*    appreciated but is not required.                                           *
*                                                                               *
* 2. Altered source versions must be plainly marked as such, and must not be    *
*    misrepresented as being the original software.                             *
*                                                                               *
* 3. This notice may not be removed or altered from any source distribution.    *
*                                                                               *
********************************************************************************/

// Libraries
#include "Transform.h"

// Namespaces
using namespace reactphysics3d;

// Constructor
Transform::Transform() : mPosition(Vector3(0.0, 0.0, 0.0)), mOrientation(Quaternion::identity()) {

}

// Constructor
Transform::Transform(const Vector3& position, const Matrix3x3& orientation)
          : mPosition(position), mOrientation(Quaternion(orientation)) {

}

// Constructor
Transform::Transform(const Vector3& position, const Quaternion& orientation)
          : mPosition(position), mOrientation(orientation) {

}

// Copy-constructor
Transform::Transform(const Transform& transform)
          : mPosition(transform.mPosition), mOrientation(transform.mOrientation) {

}

/// From in-engine format conversion
Transform::Transform(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Quaternion& rot) 
	: mPosition(pos), mOrientation(rot) {

}

/// From in-engine format conversion
Transform::Transform(const DirectX::XMMATRIX& transformMatrix)
{
	DirectX::XMVECTOR scale, rot, pos;
	DirectX::XMMatrixDecompose(&scale, &rot, &pos, transformMatrix);
	mPosition = DirectX::SimpleMath::Vector3(pos);
	mOrientation = DirectX::SimpleMath::Quaternion(rot);
}

// Destructor
Transform::~Transform() {
    
}

/// To in-engine format conversion
Transform::operator DirectX::XMMATRIX() const
{
	return DirectX::XMMatrixRotationQuaternion(DirectX::SimpleMath::Quaternion(mOrientation)) * 
		DirectX::XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
}