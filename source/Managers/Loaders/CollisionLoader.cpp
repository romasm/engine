#include "stdafx.h"
#include "CollisionLoader.h"
#include "macros.h"
#include "Log.h"
#include "Render.h"
#include "CollisionMgr.h"

using namespace EngineCore;

btCollisionShape* CollisionLoader::LoadCollision(string& resName)
{
	btCollisionShape* newCollision = nullptr;
	if(resName.find(EXT_COLLISION) == string::npos)
		return nullptr;

	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(resName, &size);
	if(data)
	{
		newCollision = loadEngineCollisionFromMemory( resName, data, size );
		_DELETE_ARRAY(data);
	}

#ifdef _EDITOR
#ifdef _DEV
	if(!newCollision)
	{
		uint32_t date;
		ImportInfo info;
		ResourceProcessor::LoadImportInfo(resName, info, date);

		if( info.importBytes == 0 )
		{
			string resourceName = RemoveExtension(resName);
			string fbxMesh = resourceName + ".fbx";

			if( !FileIO::IsExist(fbxMesh) )
			{
				fbxMesh = resourceName + ".FBX";
				if( !FileIO::IsExist(fbxMesh) )
				{
					//LOG("Reimport failed for %s", fbxMesh.c_str());
					return nullptr;
				}
			}

			// standard settings
			info.filePath = fbxMesh;
			info.resourceName = resourceName;
			info.importBytes = IMP_BYTE_COLLISION;
		}		

		if( ResourceProcessor::ImportResource(info, true) )
		{
			data = FileIO::ReadFileData(resName, &size);
			if(data)
			{
				newCollision = loadEngineCollisionFromMemory( resName, data, size );
				_DELETE_ARRAY(data);
			}
		}
	}
#endif
#endif

	return newCollision;
}

btCollisionShape* CollisionLoader::copyCollision(btCollisionShape* original)
{
	btCollisionShape* collision = nullptr;

	auto type = original->getShapeType();

	switch( type )
	{
	case COMPOUND_SHAPE_PROXYTYPE:
		{
			const auto originalCopmound = ((btCompoundShape*)original);
			int32_t hullsCount = originalCopmound->getNumChildShapes();

			collision = new btCompoundShape();

			for(int32_t i = 0; i < hullsCount; i++)
			{
				auto shape = originalCopmound->getChildShape(i);
				auto& transform = originalCopmound->getChildTransform(i);

				if( shape->getShapeType() != CONVEX_HULL_SHAPE_PROXYTYPE )
					continue;

				auto newShape = new btConvexHullShape(*(btConvexHullShape*)shape);
				((btCompoundShape*)collision)->addChildShape(transform, newShape);
			}
		}
		break;
	}

	return collision;
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

	LOG("Collision loaded %s", filename.c_str());
	return collision;
}

bool CollisionLoader::IsNative(string filename)
{
	if(filename.find(EXT_COLLISION) != string::npos)
		return true;
	return false;
}

bool CollisionLoader::IsSupported(string filename)
{
	if(filename.find(EXT_COLLISION) != string::npos)
		return true;
	return MeshLoader::meshImporter.IsExtensionSupported(GetExtension(filename));
}

bool CollisionLoader::ConvertCollisionToEngineFormat(string& sourceFile, string& resFile) 
{
	string ext = GetExtension(sourceFile);

	if( !MeshLoader::meshImporter.IsExtensionSupported(ext) )
	{
		ERR("Extension is not supported for collision %s", sourceFile.data());
		return false;
	}

	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(sourceFile, &size);
	if(!data)
		return false;
	
	auto flags = aiProcess_JoinIdenticalVertices | aiProcess_MakeLeftHanded;

	const aiScene* scene = MeshLoader::meshImporter.ReadFileFromMemory( data, size, flags, ext.data() );
	if(!scene)
	{
		ERR("Import failed for collision %s with error:\n %s", sourceFile.c_str(), MeshLoader::meshImporter.GetErrorString());
		_DELETE_ARRAY(data);
		return false;
	}

	btCollisionShape* collision = convertAIScene(sourceFile, resFile, scene);
	MeshLoader::meshImporter.FreeScene();

	bool status = SaveCollision(resFile, collision);
	CollisionMgr::Get()->ResourceDeallocate(collision);

	if(status)
		LOG_GOOD("Collision %s converted to engine format", sourceFile.c_str());
	else
		ERR("Collision %s IS NOT converted to engine format", sourceFile.c_str());

	_DELETE_ARRAY(data);
	return status;
}

void getNodesTransform(unordered_map<uint32_t, aiMatrix4x4>& meshTransforms, aiNode* root, aiNode* node)
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
btCollisionShape* CollisionLoader::convertAIScene(string& filename, string& resFile, const aiScene* scene)
{
	btCollisionShape* collision = new btCompoundShape;
	aiMesh** mesh = scene->mMeshes;

	unordered_map<uint32_t, aiMatrix4x4> meshTransforms;

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

	return collision;
}

// TODO: only convex hulls for now
bool CollisionLoader::SaveCollision(string& filename, btCollisionShape* collision)
{
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

	if(!FileIO::WriteFileData( filename, data.get(), file_size ))
	{
		ERR("Cant write collision file: %s", filename.c_str() );
		return false;
	}
	return true;
}