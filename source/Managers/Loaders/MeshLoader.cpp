#include "stdafx.h"
#include "MeshLoader.h"
#include "macros.h"
#include "Log.h"
#include "Render.h"

using namespace EngineCore;

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

		if( ResourceProcessor::ImportResource(info) )
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

MeshData* MeshLoader::loadEngineMeshFromMemory(string& filename, uint8_t* data, uint32_t size)
{
	uint8_t **vertices;
	uint32_t **indices;

	uint8_t* t_data = data;

	MeshFileHeader header(*(MeshFileHeader*)t_data);
	t_data += sizeof(MeshFileHeader);

	if( header.version != MESH_FILE_VERSION )
	{
		ERR("Mesh %s has wrong version!", filename.c_str());
		return nullptr;
	}

	const uint32_t vetrexSize = GetVertexSize(header.vertexFormat);

	vertices = new uint8_t*[header.materialCount];
	indices = new uint32_t*[header.materialCount];

	MeshData* mesh = new MeshData;
	mesh->vertexBuffers.create(header.materialCount);
	mesh->indexBuffers.create(header.materialCount);
	mesh->box = BoundingBox(header.bboxCenter, header.bboxExtents);
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

	for(int i = header.materialCount - 1; i >= 0; i--)
	{
		_DELETE_ARRAY(vertices[i]);
		_DELETE_ARRAY(indices[i]);
	}
	_DELETE_ARRAY(vertices);
	_DELETE_ARRAY(indices);

	LOG("Mesh loaded %s", filename.c_str());
	return mesh;
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

bool MeshLoader::ConvertMeshToEngineFormat(string& sourceFile, string& resFile, bool isSkinned)
{
	string ext = GetExtension(sourceFile);

	if( !meshImporter.IsExtensionSupported(ext) )
	{
		ERR("Extension is not supported for mesh", sourceFile.data());
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
		format = MeshVertexFormat::LIT_SKINNED_VERTEX;
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
	
	if(status)
		LOG("Mesh %s converted to engine format", sourceFile.c_str());
	else
		ERR("Mesh %s IS NOT converted to engine format", sourceFile.c_str());

	_DELETE_ARRAY(data);
	return status;
}

bool MeshLoader::saveMesh(string& filename, MeshData* mesh, uint32_t** indices, uint8_t** vertices)
{
	MeshFileHeader header;
	header.version = MESH_FILE_VERSION;
	header.materialCount = (uint32_t)mesh->vertexBuffers.size();
	header.bboxCenter = VECTOR3_CAST(mesh->box.Center);
	header.bboxExtents = VECTOR3_CAST(mesh->box.Extents);
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

bool MeshLoader::convertAIScene(string& filename, const aiScene* scene, MeshVertexFormat format)
{
	MeshData* stmesh = new MeshData;
	aiMesh** mesh = scene->mMeshes;

	const uint32_t vertexSize = GetVertexSize(format);
	const uint32_t matCount = scene->mNumMeshes;

	stmesh->indexBuffers.create(matCount);
	stmesh->vertexBuffers.create(matCount);
	stmesh->vertexFormat = format;
	
	uint32_t** indices = new uint32_t*[matCount];
	uint8_t** vertices = new uint8_t*[matCount];

	Vector3 posMin = Vector3(9999999.0f,9999999.0f,9999999.0f);
	Vector3 posMax = Vector3(-9999999.0f,-9999999.0f,-9999999.0f);

	int32_t boneOffset = 0;

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
			loadVerticesSkinnedLit(vertices[i], vBuffer.count, vertexSize, mesh[i], boneOffset, posMin, posMax);
			break;
		}			

		stmesh->vertexBuffers.push_back(vBuffer);
	}

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
			vertex->Norm = Vector3(0,0,0);

		if(mesh->mTangents)
		{
			vertex->Tang = Vector3(mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z);
			vertex->Binorm = Vector3(mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z);
		}
		else
		{
			vertex->Tang = Vector3(0,0,0);
			vertex->Binorm = Vector3(0,0,0);
		}

		if(mesh->HasTextureCoords(0))
			vertex->Tex = Vector2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y);
		else
			vertex->Tex = Vector2(0.5f, 0.5f);

		offset += vertexSize;
	}
}

void MeshLoader::loadVerticesSkinnedLit(uint8_t* data, uint32_t count, uint32_t vertexSize, aiMesh* mesh, int32_t& boneOffset, Vector3& posMin, Vector3& posMax)
{
	struct vertexBone
	{
		int32_t boneId;
		float boneWeight;
	};
	DArray<DArray<vertexBone>> boneIds;
	boneIds.resize(count);

	int32_t maxBoneCount = 0;

	for(uint32_t j = 0; j < mesh->mNumBones; j++)
	{
		auto bone = mesh->mBones[j];
		for(uint32_t k = 0; k < bone->mNumWeights; k++)
		{
			auto& weight = bone->mWeights[k];
			if( weight.mWeight == 0.0f )
				continue;

			vertexBone& vBone = boneIds[weight.mVertexId].push_back();
			vBone.boneId = boneOffset + j;
			vBone.boneWeight = weight.mWeight;

			maxBoneCount = max( maxBoneCount, (int32_t)boneIds[weight.mVertexId].size() );
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
			vertex->Norm = Vector3(0,0,0);

		if(mesh->mTangents)
		{
			vertex->Tang = Vector3(mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z);
			vertex->Binorm = Vector3(mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z);
		}
		else
		{
			vertex->Tang = Vector3(0,0,0);
			vertex->Binorm = Vector3(0,0,0);
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

		if( boneIds[j].empty() )
		{
			vertex->boneId[0] = 0;
			vertex->boneWeight[0] = 1.0f;
		}
		else
		{
			for(int32_t k = 0; k < boneIds[j].size(); k++)
			{
				if( k >= BONE_PER_VERTEX_MAXCOUNT )
					break;

				vertex->boneId[k] = boneIds[j][k].boneId;
				vertex->boneWeight[k] = boneIds[j][k].boneWeight;
			}
		}

		offset += vertexSize;
	}

	boneOffset += mesh->mNumBones;
}