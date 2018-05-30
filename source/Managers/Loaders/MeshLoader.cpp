#include "stdafx.h"
#include "MeshLoader.h"
#include "macros.h"
#include "Log.h"
#include "Render.h"
#include "SceneGraph.h"

using namespace EngineCore;

void MeshLoader::Configurate()
{
	meshImporter.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
};

MeshData* MeshLoader::LoadMesh(string& resName)
{
	MeshData* newMesh = nullptr;
	if(resName.find(EXT_MESH) == string::npos)
		return nullptr;

	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(resName, &size);
	if(data)
	{
		newMesh = loadEngineMeshFromMemory( resName, data, size );
		_DELETE_ARRAY(data);
	}

#ifdef _EDITOR
#ifdef _DEV
	if(!newMesh)
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
			info.importBytes = IMP_BYTE_MESH;
			info.isSkinnedMesh = false;
		}		

		if( ResourceProcessor::ImportResource(info, true) )
		{
			data = FileIO::ReadFileData(resName, &size);
			if(data)
			{
				newMesh = loadEngineMeshFromMemory( resName, data, size );
				_DELETE_ARRAY(data);
			}
		}
	}
#endif
#endif

	return newMesh;
}

SkeletonData* MeshLoader::LoadSkeleton(string& resName)
{
	SkeletonData* newSkeleton = nullptr;
	if(resName.find(EXT_SKELETON) == string::npos)
		return nullptr;

	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(resName, &size);
	if(data)
	{
		newSkeleton = loadEngineSkeletonFromMemory( resName, data, size );
		_DELETE_ARRAY(data);
	}

#ifdef _EDITOR
#ifdef _DEV
	if(!newSkeleton)
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
			info.importBytes = IMP_BYTE_SKELETON;
		}		

		if( ResourceProcessor::ImportResource(info, true) )
		{
			data = FileIO::ReadFileData(resName, &size);
			if(data)
			{
				newSkeleton = loadEngineSkeletonFromMemory( resName, data, size );
				_DELETE_ARRAY(data);
			}
		}
	}
#endif
#endif

	return newSkeleton;
}

AnimationData* MeshLoader::LoadAnimation(string& resName)
{
	AnimationData* newAnimation = nullptr;
	if(resName.find(EXT_ANIMATION) == string::npos)
		return nullptr;

	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(resName, &size);
	if(data)
	{
		newAnimation = loadEngineAnimationFromMemory( resName, data, size );
		_DELETE_ARRAY(data);
	}

#ifdef _EDITOR
#ifdef _DEV
	if(!newAnimation)
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
			info.importBytes = IMP_BYTE_ANIMATION;
		}		

		if( ResourceProcessor::ImportResource(info, true) )
		{
			data = FileIO::ReadFileData(resName, &size);
			if(data)
			{
				newAnimation = loadEngineAnimationFromMemory( resName, data, size );
				_DELETE_ARRAY(data);
			}
		}
	}
#endif
#endif

	return newAnimation;
}

MeshData* MeshLoader::loadEngineMeshFromMemory(string& filename, uint8_t* data, uint32_t size)
{
	uint8_t* t_data = data;

	MeshFileHeader header(*(MeshFileHeader*)t_data);
	t_data += sizeof(MeshFileHeader);

	if( header.version != MESH_FILE_VERSION )
	{
		ERR("Mesh %s has wrong version!", filename.c_str());
		return nullptr;
	}

	const uint32_t vetrexSize = GetVertexSize(header.vertexFormat);

	MeshData* mesh = new MeshData;

#ifdef _EDITOR
	RArray<uint8_t*>& vertices = mesh->vertices;
	RArray<uint32_t*>& indices = mesh->indices;
#else
	RArray<uint8_t*> vertices;
	RArray<uint32_t*> indices;
#endif

	vertices.create(header.materialCount);
	vertices.resize(header.materialCount);
	indices.create(header.materialCount);
	indices.resize(header.materialCount);

	mesh->vertexBuffers.create(header.materialCount);
	mesh->indexBuffers.create(header.materialCount);
	mesh->box.Center = header.bbox.center;
	mesh->box.Extents = header.bbox.extents;
	mesh->maxVertexOffset = header.maxVertexOffset;
	mesh->vertexFormat = header.vertexFormat;

	for(uint32_t i = 0; i < header.materialCount; i++)
	{
		GPUMeshBuffer vBuffer;
		vBuffer.count = *(uint32_t*)t_data; 
		t_data += sizeof(uint32_t);

		const auto sizeVB = vBuffer.count * vetrexSize;

		vertices[i] = new uint8_t[sizeVB];
		memcpy(vertices[i], t_data, sizeVB);
		t_data += sizeVB;

		mesh->vertexBuffers.push_back(vBuffer);

		GPUMeshBuffer iBuffer;
		iBuffer.count = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);

		const auto sizeIB = iBuffer.count * sizeof(uint32_t);
		indices[i] = new uint32_t[iBuffer.count];
		memcpy(indices[i], t_data, sizeIB);
		t_data += sizeIB;

		mesh->indexBuffers.push_back(iBuffer);
	}	

	for(uint32_t i = 0; i < header.materialCount; i++)
	{
		mesh->vertexBuffers[i].buffer = Buffer::CreateVertexBuffer(Render::Device(), vetrexSize * mesh->vertexBuffers[i].count, false, vertices[i]);
		mesh->indexBuffers[i].buffer = Buffer::CreateIndexBuffer(Render::Device(), sizeof(uint32_t) * mesh->indexBuffers[i].count, false, indices[i]);

		if (!mesh->vertexBuffers[i].buffer || !mesh->indexBuffers[i].buffer)
		{
			ERR("Cant init mesh vertex or index buffer for %s", filename.c_str());
			_DELETE(mesh);
			break;
		}
	}

#ifdef _EDITOR
	if (header.vertexFormat == MeshVertexFormat::LIT_SKINNED_VERTEX)
	{
#endif
		for (int i = header.materialCount - 1; i >= 0; i--)
		{
			_DELETE_ARRAY(vertices[i]);
			_DELETE_ARRAY(indices[i]);
		}
		vertices.destroy();
		indices.destroy();
#ifdef _EDITOR
	}
#endif

	LOG("Mesh loaded %s", filename.c_str());
	return mesh;
}

bool MeshLoader::saveMesh(string& filename, MeshData* mesh, uint32_t** indices, uint8_t** vertices)
{
	MeshFileHeader header;
	header.version = MESH_FILE_VERSION;
	header.materialCount = (uint32_t)mesh->vertexBuffers.size();
	header.bbox.center = mesh->box.Center;
	header.bbox.extents = mesh->box.Extents;
	header.maxVertexOffset = mesh->maxVertexOffset;
	header.vertexFormat = mesh->vertexFormat;

	const uint32_t vetrexSize = GetVertexSize(header.vertexFormat);

	// calc file size
	uint32_t file_size = sizeof(MeshFileHeader);
	for(uint16_t i = 0; i < header.materialCount; i++)
	{
		file_size += sizeof(uint32_t) + sizeof(uint32_t);
		file_size += mesh->vertexBuffers[i].count * vetrexSize;
		file_size += mesh->indexBuffers[i].count * sizeof(uint32_t);
	}

	unique_ptr<uint8_t> data(new uint8_t[file_size]);
	uint8_t* t_data = data.get();

	*(MeshFileHeader*)t_data = header;
	t_data += sizeof(MeshFileHeader);

	for(uint32_t i = 0; i < header.materialCount; i++)
	{
		*(uint32_t*)t_data = mesh->vertexBuffers[i].count;
		t_data += sizeof(uint32_t);

		if(vertices[i])
		{
			uint32_t size = mesh->vertexBuffers[i].count * vetrexSize;
			memcpy(t_data, vertices[i], size);
			t_data += size;
		}

		*(uint32_t*)t_data = mesh->indexBuffers[i].count;
		t_data += sizeof(uint32_t);

		if(indices[i])
		{
			uint32_t size = mesh->indexBuffers[i].count * sizeof(uint32_t);
			memcpy(t_data, indices[i], size);
			t_data += size;
		}
	}

	if(!FileIO::WriteFileData( filename, data.get(), file_size ))
	{
		ERR("Cant write mesh file: %s", filename.c_str() );
		return false;
	}
	return true;
}

SkeletonData* MeshLoader::loadEngineSkeletonFromMemory(string& filename, uint8_t* data, uint32_t size)
{
	uint8_t* t_data = data;

	SkeletonFileHeader header(*(SkeletonFileHeader*)t_data);
	t_data += sizeof(SkeletonFileHeader);

	if( header.version != SKELETON_FILE_VERSION )
	{
		ERR("Skeleton %s has wrong version!", filename.c_str());
		return nullptr;
	}
	
	SkeletonData* skeleton = new SkeletonData;
	skeleton->bData.create(header.boneCount);
	skeleton->bData.resize(header.boneCount);
	
	uint32_t boneDataSize = header.boneCount * sizeof(BoneData);
	memcpy(skeleton->bData.data(), t_data, boneDataSize);
	t_data += boneDataSize;

	uint32_t idsCount = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);

#ifdef _EDITOR // TODO: load only cashed bones(sokets) for game

	skeleton->bIDs.reserve(idsCount);
	for(uint32_t i = 0; i < idsCount; i++)
	{
		uint32_t stringSize = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);
		string boneName((char*)t_data, stringSize);
		t_data += sizeof(char) * stringSize;

		int32_t boneId = *(int32_t*)t_data;
		t_data += sizeof(int32_t);

		skeleton->bIDs.insert(make_pair(boneName, boneId));
	}

#endif

	LOG("Skeleton loaded %s", filename.c_str());
	return skeleton;
}

bool MeshLoader::saveSkeleton(string& filename, DArray<BoneData>& boneData, unordered_map<string, int32_t>& boneIds)
{
	SkeletonFileHeader header;
	header.version = SKELETON_FILE_VERSION;
	header.boneCount = (uint32_t)boneData.size();

	// calc file size
	uint32_t file_size = sizeof(SkeletonFileHeader);
	file_size += sizeof(BoneData) * (uint32_t)boneData.size() + sizeof(uint32_t);
	for(auto& it: boneIds)
		file_size += sizeof(uint32_t) + sizeof(char) * (uint32_t)it.first.size() + sizeof(int32_t);

	unique_ptr<uint8_t> data(new uint8_t[file_size]);
	uint8_t* t_data = data.get();

	*(SkeletonFileHeader*)t_data = header;
	t_data += sizeof(SkeletonFileHeader);

	if(boneData.size())
	{
		memcpy(t_data, boneData.data(), boneData.size() * sizeof(BoneData));
		t_data += sizeof(BoneData) * boneData.size();
	}

	*(uint32_t*)t_data = (uint32_t)boneIds.size();
	t_data += sizeof(uint32_t);

	for(auto& it: boneIds)
	{
		*(uint32_t*)t_data = (uint32_t)it.first.size();
		t_data += sizeof(uint32_t);

		memcpy(t_data, it.first.data(), it.first.size());
		t_data += (uint32_t)it.first.size();

		*(int32_t*)t_data = it.second;
		t_data += sizeof(int32_t);
	}

	if(!FileIO::WriteFileData( filename, data.get(), file_size ))
	{
		ERR("Cant write skeleton file: %s", filename.c_str() );
		return false;
	}
	return true;
}

AnimationData* MeshLoader::loadEngineAnimationFromMemory(string& filename, uint8_t* data, uint32_t size)
{
	uint8_t* t_data = data;

	AnimationFileHeader header(*(AnimationFileHeader*)t_data);
	t_data += sizeof(AnimationFileHeader);

	if( header.version != ANIMATION_FILE_VERSION )
	{
		ERR("Animation %s has wrong version!", filename.c_str());
		return nullptr;
	}

	AnimationData* animation = new AnimationData;
	animation->bones.resize(header.boneCount);

	animation->duration = *(float*)t_data;
	t_data += sizeof(float);

	animation->keysCountMinusOne = *(int32_t*)t_data;
	t_data += sizeof(int32_t);

	const uint32_t totalKeysCount = (uint32_t)animation->keysCountMinusOne + 1;
	animation->bboxes.resize(totalKeysCount);
	if(totalKeysCount > 0)
	{
		const uint32_t bboxSize = totalKeysCount * sizeof(MeshBBox);
		memcpy(animation->bboxes.data(), t_data, bboxSize);
		t_data += bboxSize;
	}

	for(auto& it: animation->bones)
	{
		uint32_t keysCount = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);

		if(keysCount > 0)
		{
			const uint32_t keysSize = keysCount * sizeof(BoneTransformation);
			it.keys.resize(keysCount);
			memcpy(it.keys.data(), t_data, keysSize);
			t_data += keysSize;
		}
		else
		{
			memset(&it.keys, 0, sizeof(it.keys));
		}
	}
	
	LOG("Animation loaded %s", filename.c_str());
	return animation;
}

bool MeshLoader::saveAnimation(string& filename, DArray<AnimationData>& animations)
{
	WRN("TODO: Only 0-id animation is imported for now");
	AnimationData& anim = animations[0];

	AnimationFileHeader header;
	header.version = ANIMATION_FILE_VERSION;
	header.boneCount = (uint32_t)anim.bones.size();
	
	// calc file size
	uint32_t file_size = sizeof(AnimationFileHeader) + sizeof(float) + sizeof(int32_t);

	const uint32_t bboxSize = (uint32_t)anim.bboxes.size() * sizeof(MeshBBox);
	file_size += bboxSize;
	for(auto& it: anim.bones)
		file_size += sizeof(uint32_t) + sizeof(BoneTransformation) * (uint32_t)it.keys.size();

	unique_ptr<uint8_t> data(new uint8_t[file_size]);
	uint8_t* t_data = data.get();

	*(AnimationFileHeader*)t_data = header;
	t_data += sizeof(AnimationFileHeader);
	
	*(float*)t_data = anim.duration;
	t_data += sizeof(float);

	*(int32_t*)t_data = anim.keysCountMinusOne;
	t_data += sizeof(int32_t);
	
	if( anim.bboxes.size() != anim.keysCountMinusOne + 1 )
	{
		ERR("Bounding boxes corruption for animation %s", filename.data());
		return false;
	}

	if(bboxSize > 0)
	{
		memcpy(t_data, anim.bboxes.data(), bboxSize);
		t_data += bboxSize;
	}

	for(auto& it: anim.bones)
	{
		*(uint32_t*)t_data = (uint32_t)it.keys.size();
		t_data += sizeof(uint32_t);

		const uint32_t keysSize = (uint32_t)it.keys.size() * sizeof(BoneTransformation);
		if(keysSize > 0)
		{
			memcpy(t_data, it.keys.data(), keysSize);
			t_data += keysSize;
		}
	}

	if(!FileIO::WriteFileData( filename, data.get(), file_size ))
	{
		ERR("Cant write animation file: %s", filename.c_str() );
		return false;
	}
	return true;
}

bool MeshLoader::IsNative(string filename)
{
	if(filename.find(EXT_MESH) != string::npos)
		return true;
	return false;
}

bool MeshLoader::IsSupported(string filename)
{
	if(filename.find(EXT_MESH) != string::npos)
		return true;
	return meshImporter.IsExtensionSupported(GetExtension(filename));
}

bool MeshLoader::IsNativeSkeleton(string filename)
{
	if(filename.find(EXT_SKELETON) != string::npos)
		return true;
	return false;
}

bool MeshLoader::IsSupportedSkeleton(string filename)
{
	if(filename.find(EXT_SKELETON) != string::npos)
		return true;
	return meshImporter.IsExtensionSupported(GetExtension(filename));
}

bool MeshLoader::IsNativeAnimation(string filename)
{
	if(filename.find(EXT_ANIMATION) != string::npos)
		return true;
	return false;
}

bool MeshLoader::IsSupportedAnimation(string filename)
{
	if(filename.find(EXT_ANIMATION) != string::npos)
		return true;
	return meshImporter.IsExtensionSupported(GetExtension(filename));
}

bool MeshLoader::ConvertMeshToEngineFormat(string& sourceFile, string& resFile, bool isSkinned)
{
	string ext = GetExtension(sourceFile);

	if( !meshImporter.IsExtensionSupported(ext) )
	{
		ERR("Extension is not supported for mesh %s", sourceFile.data());
		return false;
	}

	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(sourceFile, &size);
	if(!data)
		return false;
	
	auto flags = aiProcessPreset_TargetRealtime_Fast | aiProcess_ConvertToLeftHanded;
	MeshVertexFormat format;
	if( isSkinned )
	{
		format = MeshVertexFormat::LIT_SKINNED_VERTEX; // TODO: aiProcess_LimitBoneWeight
	}
	else
	{
		format = MeshVertexFormat::LIT_VERTEX;
		flags |= aiProcess_PreTransformVertices;
	}

	const aiScene* scene = meshImporter.ReadFileFromMemory( data, size, flags, ext.data());
	if(!scene)
	{
		ERR("Import failed for mesh %s with error:\n %s", sourceFile.c_str(), meshImporter.GetErrorString());
		_DELETE_ARRAY(data);
		return false;
	}

	bool status = convertAIScene(resFile, scene, format);
	meshImporter.FreeScene();
	_DELETE_ARRAY(data);
	
	if(status)
		LOG_GOOD("Mesh %s converted to engine format", sourceFile.c_str());
	else
		ERR("Mesh %s IS NOT converted to engine format", sourceFile.c_str());

	return status;
}

bool MeshLoader::ConvertSkeletonToEngineFormat(string& sourceFile, string& resFile)
{
	string ext = GetExtension(sourceFile);

	if( !meshImporter.IsExtensionSupported(ext) )
	{
		ERR("Extension is not supported for skeleton", sourceFile.data());
		return false;
	}

	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(sourceFile, &size);
	if(!data)
		return false;

	auto flags = aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_ConvertToLeftHanded; // TODO: aiProcess_LimitBoneWeight
	const aiScene* scene = meshImporter.ReadFileFromMemory( data, size, flags, ext.data());
	if(!scene)
	{
		ERR("Import failed for skeleton %s with error:\n %s", sourceFile.c_str(), meshImporter.GetErrorString());
		_DELETE_ARRAY(data);
		return false;
	}

	bool status = false;
	unordered_map<string, int32_t> boneIds;
	DArray<BoneData> boneData;
	DArray<int32_t> boneInvRemap;

	if(loadMeshSkeleton(sourceFile, scene, boneIds, boneData, boneInvRemap, false))
	{
		boneInvRemap.destroy();
		status = saveSkeleton(resFile, boneData, boneIds);
	}

	meshImporter.FreeScene();
	_DELETE_ARRAY(data);

	if(status)
		LOG_GOOD("Skeleton %s converted to engine format", sourceFile.c_str());
	else
		ERR("Skeleton %s IS NOT converted to engine format", sourceFile.c_str());

	return status;
}

bool MeshLoader::ConverAnimationToEngineFormat(string& sourceFile, string& resFile)
{
	string ext = GetExtension(sourceFile);

	if( !meshImporter.IsExtensionSupported(ext) )
	{
		ERR("Extension is not supported for skeleton", sourceFile.data());
		return false;
	}

	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(sourceFile, &size);
	if(!data)
		return false;

	auto flags = aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_ConvertToLeftHanded; // TODO: aiProcess_LimitBoneWeight
	const aiScene* scene = meshImporter.ReadFileFromMemory( data, size, flags, ext.data());
	if(!scene)
	{
		ERR("Import failed for animation %s with error:\n %s", sourceFile.c_str(), meshImporter.GetErrorString());
		_DELETE_ARRAY(data);
		return false;
	}

	bool status = convertAnimationAIScene(resFile, scene);

	meshImporter.FreeScene();
	_DELETE_ARRAY(data);

	if(status)
		LOG_GOOD("Animation %s converted to engine format", sourceFile.c_str());
	else
		ERR("Animation %s IS NOT converted to engine format", sourceFile.c_str());

	return status;
}

bool MeshLoader::convertAIScene(string& filename, const aiScene* scene, MeshVertexFormat format)
{
	unordered_map<string, int32_t> boneIds;
	DArray<BoneData> boneData;
	DArray<int32_t> boneInvRemap;
	if(IsSkinned(format))
	{
		if(!loadMeshSkeleton(filename, scene, boneIds, boneData, boneInvRemap, true))
			return false;
	}
	boneInvRemap.destroy();

	MeshData* stmesh = new MeshData;
	aiMesh** mesh = scene->mMeshes;
	
	const uint32_t vertexSize = GetVertexSize(format);
	const uint32_t matCount = scene->mNumMeshes;

	stmesh->indexBuffers.create(matCount);
	stmesh->vertexBuffers.create(matCount);
	stmesh->vertexFormat = format;
	stmesh->maxVertexOffset = 0;
	
	uint32_t** indices = new uint32_t*[matCount];
	uint8_t** vertices = new uint8_t*[matCount];

	Vector3 posMin = Vector3(9999999.0f,9999999.0f,9999999.0f);
	Vector3 posMax = Vector3(-9999999.0f,-9999999.0f,-9999999.0f);

	for(uint32_t i = 0; i < matCount; i++)
	{
		indices[i] = nullptr;
		vertices[i] = nullptr;

		if(mesh[i]->mPrimitiveTypes != aiPrimitiveType_TRIANGLE )
		{
			ERR("Unsupported primitive type on %s", mesh[i]->mName.C_Str());
			continue;
		}

		// INDICES
		GPUMeshBuffer iBuffer;

		iBuffer.count = mesh[i]->mNumFaces * 3;

		indices[i] = new uint32_t[iBuffer.count];
		uint32_t k = 0;
		for(uint32_t j = 0; j < mesh[i]->mNumFaces; j++)
		{
			for(uint32_t q = 0; q < 3; q++)
				indices[i][k+q] = mesh[i]->mFaces[j].mIndices[q];
			k += 3;
		}

		stmesh->indexBuffers.push_back(iBuffer);

		// VERTICES
		GPUMeshBuffer vBuffer;

		vBuffer.count = mesh[i]->mNumVertices;

		const uint32_t byteSize = vBuffer.count * vertexSize;
		vertices[i] = new uint8_t[byteSize];

		switch( format )
		{
		case MeshVertexFormat::LIT_VERTEX:
			loadVerticesLit(vertices[i], vBuffer.count, vertexSize, mesh[i], posMin, posMax);
			break;
		case MeshVertexFormat::LIT_SKINNED_VERTEX:
			loadVerticesSkinnedLit(vertices[i], vBuffer.count, vertexSize, mesh[i], boneIds, boneData, posMin, posMax, stmesh->maxVertexOffset);
			break;
		}			

		stmesh->vertexBuffers.push_back(vBuffer);
	}
	boneData.destroy();
	boneIds.clear();

	Vector3 center = Vector3(0.5f * (posMin.x + posMax.x), 0.5f * (posMin.y + posMax.y), 0.5f * (posMin.z + posMax.z));
	Vector3 extents = Vector3(posMax.x - center.x, posMax.y - center.y, posMax.z - center.z);
	extents.x = std::max<float>(extents.x, 0);
	extents.y = std::max<float>(extents.y, 0);
	extents.z = std::max<float>(extents.z, 0);
	stmesh->box = BoundingBox(center, extents);

	bool status = saveMesh(filename, stmesh, indices, vertices);

	for(uint32_t i = 0; i < matCount; i++)
	{
		if(indices)
			_DELETE_ARRAY(indices[i]);
		if(vertices)
			_DELETE_ARRAY(vertices[i]);
	}
	_DELETE_ARRAY(indices);
	_DELETE_ARRAY(vertices);

	_DELETE(stmesh);
	return status;
}

void getSubNodesTransform(unordered_map<string, NodeInfo>& nodeTransforms, aiNode* root, aiNode* node, bool worldTransformation)
{
	string nodeName(node->mName.data);
	NodeInfo nInfo;
	if(node->mParent) 
		nInfo.parent = node->mParent->mName.data;

	if(worldTransformation)
	{
		if( node == root )
		{
			nInfo.transform = node->mTransformation;
		}
		else
		{
			auto parent = node;
			while( parent != root && parent != nullptr )
			{
				/*aiVector3D scale, rotAxis, pos;
				float rotAng;
				parent->mTransformation.Decompose(scale, rotAxis, rotAng, pos);

				aiMatrix4x4 parentTransformFix;
				string parentName(parent->mName.data);
				if( parentName.find("_$AssimpFbx$_PreRotation") != string::npos || parentName.find("_$AssimpFbx$_Rotation") != string::npos )
					aiMatrix4x4::Rotation(rotAng, rotAxis, parentTransformFix);
				else if( parentName.find("_$AssimpFbx$_Scaling") != string::npos )
					aiMatrix4x4::Scaling(scale, parentTransformFix);
				else if( parentName.find("_$AssimpFbx$_Translation") != string::npos )
					aiMatrix4x4::Translation(pos, parentTransformFix);
				else
					parentTransformFix = parent->mTransformation;*/

				nInfo.transform = parent->mTransformation * nInfo.transform;
				parent = parent->mParent;
			}
		}
	}
	else
	{
		nInfo.transform = node->mTransformation;
	}

	nodeTransforms.insert(make_pair(nodeName, nInfo));

	for(uint32_t j = 0; j < node->mNumChildren; j++)
	{
		auto childNode = node->mChildren[j];

		getSubNodesTransform(nodeTransforms, root, childNode, worldTransformation);
	}
}

BoneTransformation getBoneTransformationForTime(aiNodeAnim* boneAnim, float timeInTicks)
{
	BoneTransformation result;
	result.translation = Vector3::Zero;
	result.rotation = Quaternion::Identity;
	result.scale = Vector3::Zero;

	if( boneAnim->mNumPositionKeys > 0 )
	{
		if( boneAnim->mNumPositionKeys == 1 )
		{
			result.translation = aiVector3DToVector3(boneAnim->mPositionKeys[0].mValue);
		}
		else
		{
			uint32_t prevKey = 0;
			uint32_t nextKey;
			for(nextKey = 1; nextKey < boneAnim->mNumPositionKeys; nextKey++)
			{
				if(boneAnim->mPositionKeys[nextKey].mTime >= timeInTicks)
					break;
				prevKey = nextKey;
			}

			if( nextKey == boneAnim->mNumPositionKeys )
			{
				result.translation = aiVector3DToVector3(boneAnim->mPositionKeys[prevKey].mValue);
			}
			else
			{
				float lerpFactor = 0;
				float delta = (float)(boneAnim->mPositionKeys[nextKey].mTime - boneAnim->mPositionKeys[prevKey].mTime);
				if( delta != 0 )
					lerpFactor = (timeInTicks - (float)boneAnim->mPositionKeys[prevKey].mTime) / delta;
				
				result.translation = Vector3::Lerp(aiVector3DToVector3(boneAnim->mPositionKeys[prevKey].mValue), 
					aiVector3DToVector3(boneAnim->mPositionKeys[nextKey].mValue), lerpFactor);
			}
		}
	}

	if( boneAnim->mNumRotationKeys > 0 )
	{
		if( boneAnim->mNumRotationKeys == 1 )
		{
			result.rotation = aiQuaternionToQuaternion(boneAnim->mRotationKeys[0].mValue);
		}
		else
		{
			uint32_t prevKey = 0;
			uint32_t nextKey;
			for(nextKey = 1; nextKey < boneAnim->mNumRotationKeys; nextKey++)
			{
				if(boneAnim->mRotationKeys[nextKey].mTime >= timeInTicks)
					break;
				prevKey = nextKey;
			}

			if( nextKey == boneAnim->mNumRotationKeys )
			{
				result.rotation = aiQuaternionToQuaternion(boneAnim->mRotationKeys[prevKey].mValue);
			}
			else
			{
				float lerpFactor = 0;
				float delta = (float)(boneAnim->mRotationKeys[nextKey].mTime - boneAnim->mRotationKeys[prevKey].mTime);
				if( delta != 0 )
					lerpFactor = (timeInTicks - (float)boneAnim->mRotationKeys[prevKey].mTime) / delta;

				aiQuaternion interpolated;
				aiQuaternion::Interpolate(interpolated, boneAnim->mRotationKeys[prevKey].mValue,
					boneAnim->mRotationKeys[nextKey].mValue, lerpFactor);
				result.rotation = aiQuaternionToQuaternion(interpolated.Normalize());
			}
		}
	}

	if( boneAnim->mNumScalingKeys > 0 )
	{
		if( boneAnim->mNumScalingKeys == 1 )
		{
			result.scale = aiVector3DToVector3(boneAnim->mScalingKeys[0].mValue);
		}
		else
		{
			uint32_t prevKey = 0;
			uint32_t nextKey;
			for(nextKey = 1; nextKey < boneAnim->mNumScalingKeys; nextKey++)
			{
				if(boneAnim->mScalingKeys[nextKey].mTime >= timeInTicks)
					break;
				prevKey = nextKey;
			}

			if( nextKey == boneAnim->mNumScalingKeys )
			{
				result.scale = aiVector3DToVector3(boneAnim->mScalingKeys[prevKey].mValue);
			}
			else
			{
				float lerpFactor = 0;
				float delta = (float)(boneAnim->mScalingKeys[nextKey].mTime - boneAnim->mScalingKeys[prevKey].mTime);
				if( delta != 0 )
					lerpFactor = (timeInTicks - (float)boneAnim->mScalingKeys[prevKey].mTime) / delta;

				result.scale = Vector3::Lerp(aiVector3DToVector3(boneAnim->mScalingKeys[prevKey].mValue), 
					aiVector3DToVector3(boneAnim->mScalingKeys[nextKey].mValue), lerpFactor);
			}
		}
	}

	return result;
}

#define FBX_BROKEN_BONE_SUFIX "_$AssimpFbx$_"

bool MeshLoader::convertAnimationAIScene(string& filename, const aiScene* scene)
{
	if(!scene->HasAnimations())
	{
		ERR("No animations in file %s", filename.data());
		return false;
	}

	unordered_map<string, int32_t> boneIds;
	DArray<BoneData> boneData;
	DArray<int32_t> boneInvRemap;

	if(!loadMeshSkeleton(filename, scene, boneIds, boneData, boneInvRemap, false))
		return false;
	boneInvRemap.destroy();
	
	// keys map
	RArray<unordered_map<string, aiNodeAnim*>> boneKeys;
	boneKeys.create(scene->mNumAnimations);
	for(uint32_t i = 0; i < scene->mNumAnimations; i++)
	{
		const aiAnimation* animation = scene->mAnimations[i];
		auto keyMap = boneKeys.push_back();
		
		for(uint32_t j = 0; j < animation->mNumChannels; j++)
		{
			aiNodeAnim* boneAnim = animation->mChannels[j];
			string boneName = animation->mChannels[j]->mNodeName.C_Str();
			if( boneIds.find(boneName) == boneIds.end() )
				continue;

			keyMap->insert(make_pair(boneName, boneAnim));
		}

		if(keyMap->empty())
			boneKeys.pop_back();
	}
	
	DArray<AnimationData> animationsArray;
	animationsArray.reserve(scene->mNumAnimations);
	
	for(uint32_t i = 0; i < scene->mNumAnimations; i++)
	{
		if(boneKeys[i].empty())
			continue;

		auto& finalAnimation = animationsArray.push_back();
		const aiAnimation* animation = scene->mAnimations[i];
		float ticksPerSec = (float)animation->mTicksPerSecond;
		if(ticksPerSec == 0)
			ticksPerSec = 25.0f;

		finalAnimation.duration = (float)animation->mDuration / ticksPerSec;

		finalAnimation.bones.resize(boneData.size());
		memset(finalAnimation.bones.data(), 0, sizeof(BoneAnimation) * boneData.size());

		// estimate animation keys-per-second
		float minDeltaTimeInTicks = numeric_limits<float>::max();
		for(auto& it: boneKeys[i])
		{
			if(!it.second)
				continue;

			float lastTime;
			if(it.second->mNumPositionKeys > 0)
			{
				lastTime = (float)it.second->mPositionKeys[0].mTime;
				for(uint32_t j = 1; j < it.second->mNumPositionKeys; j++)
				{
					minDeltaTimeInTicks = min(minDeltaTimeInTicks, abs(float(it.second->mPositionKeys[j].mTime - lastTime)));
					lastTime = (float)it.second->mPositionKeys[j].mTime;
				}
			}

			if(it.second->mNumRotationKeys > 0)
			{
				lastTime = (float)it.second->mRotationKeys[0].mTime;
				for(uint32_t j = 1; j < it.second->mNumRotationKeys; j++)
				{
					minDeltaTimeInTicks = min(minDeltaTimeInTicks, abs(float(it.second->mRotationKeys[j].mTime - lastTime)));
					lastTime = (float)it.second->mRotationKeys[j].mTime;
				}
			}

			if(it.second->mNumScalingKeys > 0)
			{
				lastTime = (float)it.second->mScalingKeys[0].mTime;
				for(uint32_t j = 1; j < it.second->mNumScalingKeys; j++)
				{
					minDeltaTimeInTicks = min(minDeltaTimeInTicks, abs(float(it.second->mScalingKeys[j].mTime - lastTime)));
					lastTime = (float)it.second->mScalingKeys[j].mTime;
				}
			}
		}

		minDeltaTimeInTicks = max(minDeltaTimeInTicks, ticksPerSec / ANIMATION_BAKE_MAX_KPS);

		int32_t keysPerSecond = int32_t(ticksPerSec / minDeltaTimeInTicks);
		keysPerSecond = 10 * (int32_t(keysPerSecond / 10) + 1);
		keysPerSecond = min(max(ANIMATION_BAKE_MIN_KPS, keysPerSecond), ANIMATION_BAKE_MAX_KPS);

		// get transforms
		finalAnimation.keysCountMinusOne = int32_t(keysPerSecond * finalAnimation.duration) - 1;
		finalAnimation.keysCountMinusOne = max(finalAnimation.keysCountMinusOne, 0);

		auto keysBufferSize = finalAnimation.keysCountMinusOne + 1;
		
		for(auto& it: boneKeys[i])
		{
			if( it.second->mNumPositionKeys + it.second->mNumRotationKeys + it.second->mNumScalingKeys == 0 )
				continue;

			int32_t boneID = boneIds.find(it.first)->second;
			finalAnimation.bones[boneID].keys.resize(keysBufferSize);

			for(int32_t j = 0; j < keysBufferSize; j++)
			{
				float currentTime;
				if(keysBufferSize == 1)
					currentTime = (float)animation->mDuration;
				else
					currentTime = float(j) / float(finalAnimation.keysCountMinusOne);

				finalAnimation.bones[boneID].keys[j] = getBoneTransformationForTime(it.second, (float)animation->mDuration * currentTime);
			}
		}

		// TODO: bbox with actual vertex offset??? 
		// bboxes 
		finalAnimation.bboxes.resize(keysBufferSize);
		for(int32_t j = 0; j < keysBufferSize; j++)
		{
			Vector3 posMin = Vector3(9999999.0f);
			Vector3 posMax = Vector3(-9999999.0f);

			for(int32_t k = 0; k < finalAnimation.bones.size(); k++)
			{
				auto& boneAnim = finalAnimation.bones[k];

				Vector3 pos(boneAnim.keys[j].translation);
				int32_t parent = boneData[k].parent;
				while( parent >= 0 )
				{
					auto& parentBoneAnim = finalAnimation.bones[parent];
					Vector3 pPos, pScale;
					Quaternion pRot;
					if(parentBoneAnim.keys.empty())
					{
						boneData[parent].localTransform.Decompose(pScale, pRot, pPos);
					}
					else
					{
						pPos = parentBoneAnim.keys[j].translation;
						pRot = parentBoneAnim.keys[j].rotation;
						pScale = parentBoneAnim.keys[j].scale;
					}

					pos *= pScale;
					pos = Vector3::Transform(pos, pRot);
					pos += pPos;

					parent = boneData[parent].parent;
				}

				posMin = Vector3::Min(pos, posMin);
				posMax = Vector3::Max(pos, posMax);
			}
			
			MeshBBox& bbox = finalAnimation.bboxes[j];
			bbox.center = 0.5f * (posMin + posMax);
			bbox.extents = posMax - bbox.center;
			bbox.extents = Vector3::Max(bbox.extents, Vector3::Zero);
		}

		// convertion to ms
		finalAnimation.duration *= 1000.0f;
	}

	if(animationsArray.empty())
	{
		ERR("No animations was converted for file %s", filename.data());
		return false;
	}

	return saveAnimation(filename, animationsArray);
}

bool MeshLoader::loadMeshSkeleton(string& filename, const aiScene* scene, unordered_map<string, int32_t>& boneIds, DArray<BoneData>& boneData, 
							  DArray<int32_t>& boneInvRemap, bool boneInvWorldTransforms)
{
	aiMesh** mesh = scene->mMeshes;
	const uint32_t matCount = scene->mNumMeshes;
	if( matCount == 0 )
	{
		ERR("No valid meshes in skeleton file %s, meshes must be provided for skeleton importing", filename.data());
		return false;
	}

	unordered_map<string, NodeInfo> nodeTransforms;
	auto root = scene->mRootNode;
	getSubNodesTransform(nodeTransforms, root, root, boneInvWorldTransforms);

	for(uint32_t i = 0; i < matCount; i++)
	{
		if(mesh[i]->mPrimitiveTypes != aiPrimitiveType_TRIANGLE )
		{
			ERR("Unsupported primitive type on %s", mesh[i]->mName.C_Str());
			continue;
		}

		for(uint32_t j = 0; j < mesh[i]->mNumBones; j++)
		{
			auto bone = mesh[i]->mBones[j];
			string boneName(bone->mName.data);

			if(boneName.empty())
			{
				WRN("Unnamed bones in %s is not supported", filename.data());
				continue;
			}			

			auto boneIt = boneIds.find(boneName);
			if( boneIt == boneIds.end() )
			{
				int32_t boneId = (int32_t)boneData.size();
				BoneData bData;

				//if(boneInvWorldTransforms)
				//	bData.localTransform = aiMatrix4x4ToMatrix(bone->mOffsetMatrix);

				boneData.push_back(bData);
				boneIds.insert(make_pair(boneName, boneId));
			}
		}
	}

	if( boneData.empty() )
	{
		ERR("No skinned geometry found in skeleton file %s, meshes must be skinned on bones", filename.data());
		return false;
	}

	for(auto& it: boneIds)
	{
		auto node = nodeTransforms.find(it.first);
		if( node == nodeTransforms.end() )
			continue;

		auto& bData = boneData[it.second];
		auto parentBoneId = boneIds.find(node->second.parent);
		if( parentBoneId != boneIds.end() )
			bData.parent = parentBoneId->second;
		else
			bData.parent = -1;

		if(boneInvWorldTransforms)
		{
			Matrix invTransform;
			aiMatrix4x4ToMatrix(node->second.transform).Invert(bData.localTransform);
			//bData.localTransform = invTransform * bData.localTransform;
		}
		else
		{
			bData.localTransform = aiMatrix4x4ToMatrix(node->second.transform);
		}
	}

	// sort bones	
	struct BoneRemap 
	{
		uint32_t depth;
		int32_t oldID;
	};

	DArray<BoneRemap> boneRemap;
	boneRemap.reserve(boneData.size());
	boneRemap.resize(boneData.size());
	for(int32_t i = 0; i < (int32_t)boneRemap.size(); i++)
	{
		uint32_t depth = 0;
		int32_t parent = boneData[i].parent;
		while(parent >= 0)
		{
			parent = boneData[parent].parent;
			depth++;

			if( depth >= MAX_HIERARCHY_DEPTH - 1 )
			{
				ERR("Skeleton hierarchy is too deep! Max is %i", MAX_HIERARCHY_DEPTH);
				boneData[parent].parent = -1;
				break;
			}
		}

		boneRemap[i].depth = depth;
		boneRemap[i].oldID = i;
	}

	sort(boneRemap.begin(), boneRemap.end(), 
		[](const BoneRemap& a, const BoneRemap& b) -> bool { return a.depth < b.depth;});

	boneInvRemap.reserve(boneData.size());
	boneInvRemap.resize(boneData.size());
	for(int32_t i = 0; i < (int32_t)boneRemap.size(); i++)
		boneInvRemap[boneRemap[i].oldID] = i;

	boneRemap.destroy();

	DArray<BoneData> newBoneData;
	newBoneData.reserve(boneData.size());
	newBoneData.resize(boneData.size());
	for(uint32_t i = 0; i < (uint32_t)boneData.size(); i++)
		newBoneData[boneInvRemap[i]] = boneData[i];

	for(auto& it: boneIds)
		it.second = boneInvRemap[it.second];

	for(uint32_t i = 0; i < (uint32_t)newBoneData.size(); i++)
	{
		if(newBoneData[i].parent >= 0)
			newBoneData[i].parent = boneInvRemap[newBoneData[i].parent];
	}
	
	boneData.swap(newBoneData);
	newBoneData.destroy();
	return true;
}

void MeshLoader::loadVerticesLit(uint8_t* data, uint32_t count, uint32_t vertexSize, aiMesh* mesh, Vector3& posMin, Vector3& posMax)
{
	uint32_t offset = 0;

	for(uint32_t j = 0; j < count; j++)
	{
		LitVertex* vertex = (LitVertex*)(data + offset);

		vertex->Pos = Vector3(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z);

		posMin.x = min<float>(vertex->Pos.x, posMin.x);
		posMin.y = min<float>(vertex->Pos.y, posMin.y);
		posMin.z = min<float>(vertex->Pos.z, posMin.z);

		posMax.x = max<float>(vertex->Pos.x, posMax.x);
		posMax.y = max<float>(vertex->Pos.y, posMax.y);
		posMax.z = max<float>(vertex->Pos.z, posMax.z);

		if(mesh->mNormals)
			vertex->Norm = Vector3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z);
		else
			vertex->Norm = Vector3::Zero;

		if(mesh->mTangents)
		{
			vertex->Tang = Vector3(mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z);
			vertex->Binorm = Vector3(mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z);
		}
		else
		{
			vertex->Tang = Vector3::Zero;
			vertex->Binorm = Vector3::Zero;
		}

		if(mesh->HasTextureCoords(0))
			vertex->Tex = Vector2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y);
		else
			vertex->Tex = Vector2(0.5f, 0.5f);

		offset += vertexSize;
	}
}

void MeshLoader::loadVerticesSkinnedLit(uint8_t* data, uint32_t count, uint32_t vertexSize, aiMesh* mesh, unordered_map<string, int32_t>& boneIds, 
										DArray<BoneData>& boneData, Vector3& posMin, Vector3& posMax, float& vertexOffset)
{
	struct vertexBone
	{
		int32_t boneId;
		float boneWeight;
	};
	DArray<DArray<vertexBone>> vertexBoneIds;
	vertexBoneIds.resize(count);

	int32_t maxBoneCount = 0;

	for(uint32_t j = 0; j < mesh->mNumBones; j++)
	{
		int32_t boneId = 0;
		auto bone = mesh->mBones[j];
		string boneName(bone->mName.data);

		auto boneIt = boneIds.find(boneName);
		if( boneIt == boneIds.end() )
			WRN("Unknown bone %s", boneName.data());
		else
			boneId = boneIt->second;
		
		for(uint32_t k = 0; k < bone->mNumWeights; k++)
		{
			auto& weight = bone->mWeights[k];
			if( weight.mWeight == 0.0f )
				continue;

			vertexBone& vBone = vertexBoneIds[weight.mVertexId].push_back();
			vBone.boneId = boneId;
			vBone.boneWeight = weight.mWeight;

			maxBoneCount = max( maxBoneCount, (int32_t)vertexBoneIds[weight.mVertexId].size() );
		}
	}
	
	if( maxBoneCount > BONE_PER_VERTEX_MAXCOUNT )
		WRN("Too many bones affected one vertex, max is %i", BONE_PER_VERTEX_MAXCOUNT );

	uint32_t offset = 0;

	for(uint32_t j = 0; j < count; j++)
	{
		LitSkinnedVertex* vertex = (LitSkinnedVertex*)(data + offset);

		vertex->Pos = Vector3(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z);

		posMin.x = min<float>(vertex->Pos.x, posMin.x);
		posMin.y = min<float>(vertex->Pos.y, posMin.y);
		posMin.z = min<float>(vertex->Pos.z, posMin.z);

		posMax.x = max<float>(vertex->Pos.x, posMax.x);
		posMax.y = max<float>(vertex->Pos.y, posMax.y);
		posMax.z = max<float>(vertex->Pos.z, posMax.z);

		if(mesh->mNormals)
			vertex->Norm = Vector3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z);
		else
			vertex->Norm = Vector3::Zero;

		if(mesh->mTangents)
		{
			vertex->Tang = Vector3(mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z);
			vertex->Binorm = Vector3(mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z);
		}
		else
		{
			vertex->Tang = Vector3::Zero;
			vertex->Binorm = Vector3::Zero;
		}

		if(mesh->HasTextureCoords(0))
			vertex->Tex = Vector2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y);
		else
			vertex->Tex = Vector2(0.5f, 0.5f);

		for(int32_t k = 0; k < BONE_PER_VERTEX_MAXCOUNT; k++)
		{
			vertex->boneId[k] = -1;
			vertex->boneWeight[k] = 0;
		}

		BoneTransformation toLocalTransform;
		if( vertexBoneIds[j].empty() )
		{
			vertex->boneId[0] = 0;
			vertex->boneWeight[0] = 1.0f;
			toLocalTransform = MatrixToBoneTransformation(boneData[0].localTransform);
		}
		else
		{
			vertex->boneId[0] = vertexBoneIds[j][0].boneId;
			vertex->boneWeight[0] = vertexBoneIds[j][0].boneWeight;
			toLocalTransform = MatrixToBoneTransformation(boneData[vertexBoneIds[j][0].boneId].localTransform);
			float totalWeight = vertex->boneWeight[0];

			for(int32_t k = 1; k < vertexBoneIds[j].size(); k++)
			{
				if( k >= BONE_PER_VERTEX_MAXCOUNT )
					break;

				vertex->boneId[k] = vertexBoneIds[j][k].boneId;
				vertex->boneWeight[k] = vertexBoneIds[j][k].boneWeight;

				const float lerpFactor = vertex->boneWeight[k] / ( vertex->boneWeight[k] + totalWeight );
				BoneTransformation transform = MatrixToBoneTransformation(boneData[vertexBoneIds[j][k].boneId].localTransform);
				BoneTransformation::Lerp(toLocalTransform, transform, lerpFactor, toLocalTransform);
				totalWeight += vertex->boneWeight[k];
			}
		}
		
		// post transform
		XMMATRIX toLocalMatrix = BoneTransformationToMatrix(toLocalTransform);
		toLocalTransform.scale = XMVectorDivide(XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f), toLocalTransform.scale);
		XMMATRIX normalMatrix = BoneTransformationToMatrix(toLocalTransform);
		
		vertex->Pos = Vector3::Transform(vertex->Pos, toLocalMatrix);
		vertex->Norm = Vector3::TransformNormal(vertex->Norm, normalMatrix);
		vertex->Tang = Vector3::TransformNormal(vertex->Tang, normalMatrix);
		vertex->Binorm = Vector3::TransformNormal(vertex->Binorm, normalMatrix);

		vertexOffset = max(vertexOffset, vertex->Pos.Length());

		offset += vertexSize;
	}
}