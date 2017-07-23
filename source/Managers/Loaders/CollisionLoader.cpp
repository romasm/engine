#include "stdafx.h"
#include "CollisionLoader.h"
#include "macros.h"
#include "Log.h"
#include "Render.h"

using namespace EngineCore;

bool CollisionLoader::IsSupported(string filename)
{
	if(filename.find(EXT_COLLISION) != string::npos)
		return true;
	string extension = filename.substr(filename.rfind('.'));
	return MeshLoader::meshImporter.IsExtensionSupported(extension);
}

CollisionData* CollisionLoader::LoadCollisionFromFile(string& filename)
{
	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(filename, &size);
	if(!data)
		return nullptr;

	auto result = LoadCollisionFromMemory(filename, data, size);
	_DELETE_ARRAY(data);
	return result;
}

CollisionData* CollisionLoader::LoadCollisionFromMemory(string& resName, uint8_t* data, uint32_t size)
{
	CollisionData* newCollision;
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
}

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

	auto flags = aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_ConvertToLeftHanded;

	const aiScene* scene = MeshLoader::meshImporter.ReadFileFromMemory( data, size, flags);

	if(!scene)
	{
		ERR("Import failed for collision %s", filename.c_str());
		return nullptr;
	}

	CollisionData* collision = loadAIScene(filename, scene, onlyConvert, onlyConvert);
	MeshLoader::meshImporter.FreeScene();

	if(onlyConvert)
	{
		LOG("Collision %s converted to engine format", filename.c_str());
		_DELETE(collision);
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
			transform *= parent->mTransformation;
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

CollisionData* CollisionLoader::loadAIScene(string& filename, const aiScene* scene, bool convert, bool noInit)
{
	CollisionData* collision = new CollisionData;

	aiMesh** mesh = scene->mMeshes;

	unordered_map<uint, aiMatrix4x4> meshTransforms;

	auto root = scene->mRootNode;
	getNodesTransform(meshTransforms, root, root);

	const uint32_t hullsCount = scene->mNumMeshes;

	collision->hulls.create(hullsCount);

	uint32_t** indices = new uint32_t*[hullsCount];
	uint32_t* trisCount = new uint32_t[hullsCount];

	Vector3** vertices = new Vector3*[hullsCount];
	uint32_t* verticesCount = new uint32_t[hullsCount];

	for(uint32_t i = 0; i < hullsCount; i++)
	{
		indices[i] = nullptr;
		vertices[i] = nullptr;

		collision->hulls.push_back(ConvexHull());

		if(mesh[i]->mPrimitiveTypes != aiPrimitiveType_TRIANGLE )
		{
			ERR("Unsupported primitive type on %s", mesh[i]->mName.C_Str());
			continue;
		}

		// INDICES
		trisCount[i] = mesh[i]->mNumFaces;
		indices[i] = new uint32_t[trisCount[i] * 3];

		uint32_t k = 0;
		for(uint32_t j = 0; j < trisCount[i]; j++)
		{
			for(uint32_t q = 0; q < 3; q++)
				indices[i][k+q] = mesh[i]->mFaces[j].mIndices[q];
			k += 3;
		}

		// VERTICES
		verticesCount[i] = mesh[i]->mNumVertices;
		vertices[i] = new Vector3[verticesCount[i]];

		for(uint32_t j = 0; j < verticesCount[i]; j++)
		{
			vertices[i][j] = Vector3(mesh[i]->mVertices[j].x, mesh[i]->mVertices[j].y, mesh[i]->mVertices[j].z);
		}

		if( !noInit )
		{
			rp3d::TriangleVertexArray hull(verticesCount[i], vertices[i], sizeof(Vector3), trisCount[i], indices[i], sizeof(uint32_t), 
				rp3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE, rp3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE);

			collision->hulls[i].collider = new rp3d::ConvexMeshShape(&hull, true);
		}

		auto meshTransform = meshTransforms.find(i);
		if(meshTransform == meshTransforms.end())
		{
			ERR("Corrupted node graph in %s", filename.c_str());
		}
		else
		{
			aiVector3D pos;
			aiQuaternion rot;
			meshTransform->second.DecomposeNoScaling(rot, pos);

			collision->hulls[i].pos.x = pos.x;
			collision->hulls[i].pos.y = pos.y;
			collision->hulls[i].pos.z = pos.z;

			collision->hulls[i].rot.x = rot.x;
			collision->hulls[i].rot.y = rot.y;
			collision->hulls[i].rot.z = rot.z;
			collision->hulls[i].rot.w = rot.w;
		}
	}

	if(convert)
		saveCollision(filename, collision, indices, trisCount, vertices, verticesCount);

#ifdef _DEV
	for(uint32_t i = 0; i < hullsCount; i++)
	{
		Matrix localTransform;
		localTransform.CreateFromQuaternion(collision->hulls[i].rot);
		Matrix temp;
		temp.CreateTranslation(collision->hulls[i].pos);
		localTransform *= temp;

		for(uint32_t j = 0; j < verticesCount[i]; j++)
		{
			vertices[i][j] = Vector3::Transform(vertices[i][j], localTransform);
		}

		collision->hulls[i].vertex.size = verticesCount[i];
		collision->hulls[i].vertex.buffer = Buffer::CreateVertexBuffer(Render::Device(), sizeof(Vector3) * collision->hulls[i].vertex.size, false, vertices[i]);

		collision->hulls[i].index.size = trisCount[i] * 3;
		collision->hulls[i].index.buffer = Buffer::CreateIndexBuffer(Render::Device(), sizeof(uint32_t) * collision->hulls[i].index.size, false, indices[i]);

		if ( !collision->hulls[i].vertex.buffer || !collision->hulls[i].index.buffer )
		{
			ERR("Cant init collision debug vertex or index buffer for %s", filename.c_str());
		}
	}
#endif

	for(uint32_t i = 0; i < hullsCount; i++)
	{
		if(indices)
			_DELETE_ARRAY(indices[i]);
		if(vertices)
			_DELETE_ARRAY(vertices[i]);
	}
	_DELETE_ARRAY(indices);
	_DELETE_ARRAY(vertices);
	_DELETE_ARRAY(trisCount);
	_DELETE_ARRAY(verticesCount);

	return collision;
}

void CollisionLoader::saveCollision(string& filename, CollisionData* collision, uint32_t** indices, uint32_t* trisCount, Vector3** vertices, uint32_t* verticesCount)
{
	string clm_file = filename.substr(0, filename.rfind('.')) + EXT_COLLISION;

	CollisionHeader header;
	header.version = MESH_FILE_VERSION;
	header.hullsCount = (uint32_t)collision->hulls.size();

	// calc file size
	uint32_t file_size = sizeof(CollisionHeader);
	for(uint32_t i = 0; i < header.hullsCount; i++)
	{
		file_size += sizeof(uint32_t) + sizeof(Vector3) + sizeof(Quaternion) + sizeof(uint32_t) + sizeof(uint32_t);
		file_size += trisCount[i] * 3 * sizeof(uint32_t);
		file_size += verticesCount[i] * sizeof(Vector3);
	}

	unique_ptr<uint8_t> data(new uint8_t[file_size]);
	uint8_t* t_data = data.get();

	*(CollisionHeader*)t_data = header;
	t_data += sizeof(CollisionHeader);

	for(uint32_t i = 0; i < header.hullsCount; i++)
	{
		*(uint32_t*)t_data = collision->hulls[i].boneId;
		t_data += sizeof(uint32_t);

		*(Vector3*)t_data = collision->hulls[i].pos;
		t_data += sizeof(Vector3);

		*(Quaternion*)t_data = collision->hulls[i].rot;
		t_data += sizeof(Quaternion);

		*(uint32_t*)t_data = verticesCount[i];
		t_data += sizeof(uint32_t);

		if(vertices[i])
		{
			const uint32_t size = verticesCount[i] * sizeof(Vector3);
			memcpy(t_data, vertices[i], size);
			t_data += size;
		}

		*(uint32_t*)t_data = trisCount[i];
		t_data += sizeof(uint32_t);

		if(indices[i])
		{
			uint32_t size = trisCount[i] * 3 * sizeof(uint32_t);
			memcpy(t_data, indices[i], size);
			t_data += size;
		}
	}

	if(!FileIO::WriteFileData( clm_file, data.get(), file_size ))
	{
		ERR("Cant write collision file: %s", clm_file.c_str() );
	}
}

CollisionData* CollisionLoader::loadEngineCollisionFromMemory(string& filename, uint8_t* data, uint32_t size)
{
	Vector3 **vertices;
	uint32_t **indices;

	uint32_t *verticesCount;
	uint32_t *trisCount;

	uint8_t* t_data = data;

	CollisionHeader header(*(CollisionHeader*)t_data);
	t_data += sizeof(CollisionHeader);

	if( header.version != COLLISION_FILE_VERSION )
	{
		ERR("Collision %s has wrong version!", filename.c_str());
		return nullptr;
	}

	vertices = new Vector3*[header.hullsCount];
	indices = new uint32_t*[header.hullsCount];
	verticesCount = new uint32_t[header.hullsCount];
	trisCount = new uint32_t[header.hullsCount];

	CollisionData* collision = new CollisionData;
	collision->hulls.create(header.hullsCount);

	for(uint32_t i = 0; i < header.hullsCount; i++)
	{
		collision->hulls.push_back(ConvexHull());

		collision->hulls[i].boneId = *(uint32_t*)t_data; 
		t_data += sizeof(uint32_t);

		collision->hulls[i].pos = *(Vector3*)t_data; 
		t_data += sizeof(Vector3);

		collision->hulls[i].rot = *(Quaternion*)t_data; 
		t_data += sizeof(Quaternion);

		verticesCount[i] = *(uint32_t*)t_data; 
		t_data += sizeof(uint32_t);

		const auto sizeVB = verticesCount[i] * sizeof(Vector3);
		vertices[i] = new Vector3[verticesCount[i]];
		memcpy(vertices[i], t_data, sizeVB);
		t_data += sizeVB;

		trisCount[i] = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);

		auto sizeIB = trisCount[i] * 3;
		indices[i] = new uint32_t[sizeIB];
		sizeIB *= sizeof(uint32_t);
		memcpy(indices[i], t_data, sizeIB);
		t_data += sizeIB;
	}	

	for(uint32_t i = 0; i < header.hullsCount; i++)
	{
		rp3d::TriangleVertexArray hull(verticesCount[i], vertices[i], sizeof(Vector3), trisCount[i], indices[i], sizeof(uint32_t), 
			rp3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE, rp3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE);

		collision->hulls[i].collider = new rp3d::ConvexMeshShape(&hull, true);
		
#ifdef _DEV
		Matrix localTransform;
		localTransform.CreateFromQuaternion(collision->hulls[i].rot);
		Matrix temp;
		temp.CreateTranslation(collision->hulls[i].pos);
		localTransform *= temp;

		for(uint32_t j = 0; j < verticesCount[i]; j++)
		{
			vertices[i][j] = Vector3::Transform(vertices[i][j], localTransform);
		}

		collision->hulls[i].vertex.size = verticesCount[i];
		collision->hulls[i].vertex.buffer = Buffer::CreateVertexBuffer(Render::Device(), sizeof(Vector3) * collision->hulls[i].vertex.size, false, vertices[i]);

		collision->hulls[i].index.size = trisCount[i] * 3;
		collision->hulls[i].index.buffer = Buffer::CreateIndexBuffer(Render::Device(), sizeof(uint32_t) * collision->hulls[i].index.size, false, indices[i]);

		if ( !collision->hulls[i].vertex.buffer || !collision->hulls[i].index.buffer )
		{
			ERR("Cant init collision debug vertex or index buffer for %s", filename.c_str());
		}
#endif
	}

	for(int i = header.hullsCount - 1; i >= 0; i--)
	{
		_DELETE_ARRAY(vertices[i]);
		_DELETE_ARRAY(indices[i]);
	}
	_DELETE_ARRAY(vertices);
	_DELETE_ARRAY(indices);
	_DELETE_ARRAY(verticesCount);
	_DELETE_ARRAY(trisCount);

	LOG("Collision(.clm) loaded %s", filename.c_str());
	return collision;
}