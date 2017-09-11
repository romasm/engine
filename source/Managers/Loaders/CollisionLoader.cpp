#include "stdafx.h"
#include "CollisionLoader.h"
#include "macros.h"
#include "Log.h"
#include "Render.h"
#include "CollisionMgr.h"

using namespace EngineCore;

void CollisionLoader::CollisionDeallocate(btCollisionShape*& resource)
{
	if(!resource)
		return;

	if( resource->getShapeType() == COMPOUND_SHAPE_PROXYTYPE )
	{
		btCompoundShape* cShape = (btCompoundShape*)resource;

		auto childrenCount = cShape->getNumChildShapes();
		auto childrenPtrs = cShape->getChildList();

		while( childrenCount > 0 )
		{
			auto child = childrenPtrs->m_childShape;
			cShape->removeChildShapeByIndex(0);
			_DELETE(child);
			childrenCount--;
		}
	}

	_DELETE(resource);
};

bool CollisionLoader::IsSupported(string filename)
{
	if(filename.find(EXT_COLLISION) != string::npos)
		return true;
	string extension = filename.substr(filename.rfind('.'));
	return MeshLoader::meshImporter.IsExtensionSupported(extension);
}

CollisionData* CollisionLoader::LoadCollision(string& resName)
{
	CollisionData* newCollision = nullptr;
	if(resName.find(EXT_COLLISION) == string::npos)
		return nullptr;

	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(resName, &size);
	if(!data)
		return nullptr;

	newCollision = loadEngineCollisionFromMemory( resName, data, size );
	_DELETE_ARRAY(data);

#ifdef _EDITOR
#ifdef _DEV
	if(!newCollision)
	{
		string resourceName = resName.substr(0, resName.find(EXT_COLLISION));
		string fbxMesh = resourceName + ".fbx";
		if( FileIO::IsExist(fbxMesh) )
		{
			LOG("Trying to reimport collision %s", fbxMesh.c_str());

			// standard settings
			ImportInfo info;
			ZeroMemory(&info, sizeof(info));
			info.filePath = fbxMesh;
			info.resourceName = resourceName;
			info.importCollision = true;

			if( ResourceProcessor::ImportResource(info) )
			{
				data = FileIO::ReadFileData(resName, &size);
				if(data)
				{
					newCollision = loadEngineCollisionFromMemory( resName, data, size );
					_DELETE_ARRAY(data);
				}
			}
		}
	}
#endif
#endif

	return newCollision;
}
/*
btCollisionShape* CollisionLoader::LoadCollisionFromMemory(string& resName, uint8_t* data, uint32_t size)
{
	btCollisionShape* newCollision;
	if(resName.find(EXT_COLLISION) != string::npos)
	{
		newCollision = loadEngineCollisionFromMemory( resName, data, size );

#ifdef _EDITOR
		if(!newCollision)
		{
			string fbxMesh = resName.substr(0, resName.find(EXT_COLLISION)) + ".fbx";
			if( FileIO::IsExist(fbxMesh) )
			{
				LOG("Trying to reimport collision %s \n App restart may be needed!", fbxMesh.c_str());
				ConvertCollisionToEngineFormat(fbxMesh);
			}
		}
#endif

	}
	else
	{
		newCollision = loadNoNativeCollisionFromMemory( resName, data, size );
	}

	return newCollision;
}*/

void CollisionLoader::ConvertCollisionToEngineFormat(string& filename) 
{
	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(filename, &size);
	if(!data)
		return;

	loadNoNativeCollisionFromMemory(filename, data, size, true);
	_DELETE_ARRAY(data);
}

CollisionData* CollisionLoader::loadNoNativeCollisionFromMemory(string& filename, uint8_t* data, uint32_t size, bool onlyConvert)
{
	string extension = filename.substr(filename.rfind('.'));

	if( !MeshLoader::meshImporter.IsExtensionSupported(extension) )
	{
		ERR("Extension %s is not supported for collisions", extension.data());
		return nullptr;
	}

	auto flags = aiProcess_JoinIdenticalVertices | aiProcess_MakeLeftHanded;

	const aiScene* scene = MeshLoader::meshImporter.ReadFileFromMemory( data, size, flags);

	if(!scene)
	{
		ERR("Import failed for collision %s", filename.c_str());
		return nullptr;
	}

	btCollisionShape* collision = loadAIScene(filename, scene, onlyConvert);
	MeshLoader::meshImporter.FreeScene();

	if(onlyConvert)
	{
		LOG("Collision %s converted to engine format", filename.c_str());
		CollisionLoader::CollisionDeallocate(collision);
		return nullptr;
	}

	LOG("Collision %s loaded", filename.c_str());
	return collision;
}

void getNodesTransform(unordered_map<uint, aiMatrix4x4>& meshTransforms, aiNode* root, aiNode* node)
{
	for(uint32_t i = 0; i < node->mNumMeshes; i++)
	{
		auto meshId = node->mMeshes[i];
		aiMatrix4x4 transform;

		auto parent = node;
		while( parent != root )
		{
			transform = parent->mTransformation * transform;
			parent = parent->mParent;
		}

		meshTransforms.insert(make_pair(meshId, transform));
	}

	for(uint32_t j = 0; j < node->mNumChildren; j++)
	{
		auto childNode = node->mChildren[j];

		getNodesTransform(meshTransforms, root, childNode);
	}
}

// TODO: only convex hulls for now
CollisionData* CollisionLoader::loadAIScene(string& filename, const aiScene* scene, bool convert)
{
	btCollisionShape* collision = new btCompoundShape;
	
	aiMesh** mesh = scene->mMeshes;

	unordered_map<uint, aiMatrix4x4> meshTransforms;

	auto root = scene->mRootNode;
	getNodesTransform(meshTransforms, root, root);

	const uint32_t hullsCount = scene->mNumMeshes;

	for(uint32_t i = 0; i < hullsCount; i++)
	{		
		if( mesh[i]->mPrimitiveTypes == aiPrimitiveType_LINE || mesh[i]->mPrimitiveTypes == aiPrimitiveType_POINT )
		{
			ERR("Unsupported primitive type on %s", mesh[i]->mName.C_Str());
			continue;
		}

		uint32_t verticesCount = mesh[i]->mNumVertices;
		Vector3* vertices = new Vector3[verticesCount];

		for(uint32_t j = 0; j < verticesCount; j++)
		{
			vertices[j] = Vector3(mesh[i]->mVertices[j].x, mesh[i]->mVertices[j].y, mesh[i]->mVertices[j].z);
		}

		btTransform transform;

		auto meshTransform = meshTransforms.find(i);
		if(meshTransform == meshTransforms.end())
		{
			ERR("Corrupted node graph in %s", filename.c_str());
			transform.setIdentity();
		}
		else
		{
			aiVector3D scale;
			aiVector3D pos;
			aiQuaternion rot;
			meshTransform->second.Decompose(scale, rot, pos);
			transform = btTransform(Quaternion(rot.x, rot.y, rot.z, rot.w), Vector3(pos.x, pos.y, pos.z));

			// prescaling
			for(uint32_t j = 0; j < verticesCount; j++)
			{
				vertices[j] = vertices[j] * Vector3(scale.x, scale.y, scale.z);
			}
		}

		auto shape = new btConvexHullShape((float*)vertices, verticesCount, sizeof(float) * 3);
		shape->optimizeConvexHull(CONVEX_DISTANCE_MARGIN);

		((btCompoundShape*)collision)->addChildShape(transform, shape);

		_DELETE_ARRAY(vertices);
	}

	if(convert)
		saveCollision(filename, collision);
	
	return collision;
}

// TODO: only convex hulls for now
void CollisionLoader::saveCollision(string& filename, btCollisionShape* collision)
{
	string clm_file = filename.substr(0, filename.rfind('.')) + EXT_COLLISION;

	CollisionHeader header;
	header.version = COLLISION_FILE_VERSION;
	header.type = COMPOUND_SHAPE_PROXYTYPE;

	auto compound = (btCompoundShape*)collision;

	// calc file size
	uint32_t file_size = sizeof(CollisionHeader);
	file_size += sizeof(uint32_t);

	uint32_t hullsCount = (uint32_t)compound->getNumChildShapes();

	for(uint32_t i = 0; i < hullsCount; i++)
	{
		file_size += sizeof(uint32_t) + sizeof(Vector3) + sizeof(Quaternion);
		file_size += ((btConvexHullShape*)compound->getChildShape(i))->getNumPoints() * sizeof(Vector3);
	}

	unique_ptr<uint8_t> data(new uint8_t[file_size]);
	uint8_t* t_data = data.get();

	*(CollisionHeader*)t_data = header;
	t_data += sizeof(CollisionHeader);

	*(uint32_t*)t_data = hullsCount;
	t_data += sizeof(uint32_t);

	for(uint32_t i = 0; i < hullsCount; i++)
	{
		auto child = (btConvexHullShape*)compound->getChildShape(i);

		//*(uint32_t*)t_data = collision->hulls[i].boneId;
		//t_data += sizeof(uint32_t);

		*(Vector3*)t_data = compound->getChildTransform(i).getOrigin();
		t_data += sizeof(Vector3);

		*(Quaternion*)t_data = compound->getChildTransform(i).getRotation();
		t_data += sizeof(Quaternion);

		uint32_t pointsCount = child->getNumPoints();

		*(uint32_t*)t_data = pointsCount;
		t_data += sizeof(uint32_t);

		auto collisionPoints = child->getUnscaledPoints();

		const uint32_t size = pointsCount * sizeof(Vector3);
		Vector3* points = new Vector3[pointsCount];
		for(uint32_t k = 0; k < pointsCount; k++)
			points[k] = collisionPoints[k];

		memcpy(t_data, points, size);
		t_data += size;

		_DELETE_ARRAY(points);
	}

	if(!FileIO::WriteFileData( clm_file, data.get(), file_size ))
	{
		ERR("Cant write collision file: %s", clm_file.c_str() );
	}
}

btCollisionShape* CollisionLoader::loadEngineCollisionFromMemory(string& filename, uint8_t* data, uint32_t size)
{
	btCollisionShape* collision = nullptr;

	uint8_t* t_data = data;

	CollisionHeader header(*(CollisionHeader*)t_data);
	t_data += sizeof(CollisionHeader);

	if( header.version != COLLISION_FILE_VERSION )
	{
		ERR("Collision %s has wrong version!", filename.c_str());
		return nullptr;
	}

	switch( header.type )
	{
	case COMPOUND_SHAPE_PROXYTYPE:
		{
			uint32_t hullsCount = *(uint32_t*)t_data;
			t_data += sizeof(uint32_t);

			collision = new btCompoundShape();

			for(uint32_t i = 0; i < hullsCount; i++)
			{
				Vector3 pos = *(Vector3*)t_data;
				t_data += sizeof(Vector3);

				Quaternion rot = *(Quaternion*)t_data;
				t_data += sizeof(Quaternion);
				
				uint32_t pointsCount = *(uint32_t*)t_data;
				t_data += sizeof(uint32_t);

				const int stride = sizeof(float) * 3;

				float *points = (float*)t_data;
				t_data += stride * pointsCount;

				btTransform transform(rot, pos);
				auto shape = new btConvexHullShape(points, pointsCount, stride);

				((btCompoundShape*)collision)->addChildShape(transform, shape);
			}
		}
		break;
	default:
		ERR("Unsupported collision type for %s", filename.c_str());
		return nullptr;
	}

	LOG("Collision(.clm) loaded %s", filename.c_str());
	return collision;
}