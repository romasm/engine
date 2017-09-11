#include "stdafx.h"
#include "MeshLoader.h"
#include "macros.h"
#include "Log.h"
#include "Render.h"

using namespace EngineCore;

bool MeshLoader::IsSupported(string filename)
{
	if(filename.find(EXT_MESH) != string::npos)
		return true;
	string extension = filename.substr(filename.rfind('.'));
	return meshImporter.IsExtensionSupported(extension);
}

MeshData* MeshLoader::LoadMesh(string& resName)
{
	MeshData* newMesh = nullptr;
	if(resName.find(EXT_MESH) == string::npos)
		return nullptr;

	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(resName, &size);
	if(!data)
		return nullptr;

	newMesh = loadEngineMeshFromMemory( resName, data, size );
	_DELETE_ARRAY(data);

#ifdef _EDITOR
#ifdef _DEV
	if(!newMesh)
	{
		string resourceName = resName.substr(0, resName.find(EXT_MESH));
		string fbxMesh = resourceName + ".fbx";
		if( FileIO::IsExist(fbxMesh) )
		{
			LOG("Trying to reimport mesh %s", fbxMesh.c_str());

			// standard settings
			ImportInfo info;
			ZeroMemory(&info, sizeof(info));
			info.filePath = fbxMesh;
			info.resourceName = resourceName;
			info.importMesh = true;
			info.isSkinnedMesh = false;

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
	}
#endif
#endif

	return newMesh;
}
/*
MeshData* MeshLoader::LoadMeshFromMemory(string& resName, uint8_t* data, uint32_t size, bool skeletal)
{
	MeshData* newMesh;
	if(resName.find(EXT_MESH) != string::npos)
	{
		newMesh = loadEngineMeshFromMemory( resName, data, size, skeletal );

#ifdef _EDITOR
		if(!newMesh)
		{
			string fbxMesh = resName.substr(0, resName.find(EXT_MESH)) + ".fbx";
			if( FileIO::IsExist(fbxMesh) )
			{
				LOG("Trying to reimport mesh %s \n App restart may be needed!", fbxMesh.c_str());
				ConvertMeshToEngineFormat(fbxMesh, skeletal);
			}
		}
#endif

	}
	else
	{
		newMesh = loadNoNativeMeshFromMemory( resName, data, size, skeletal );
	}

	return newMesh;
}
*/
void MeshLoader::ConvertMeshToEngineFormat(string& filename, bool skeletal) 
{
	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(filename, &size);
	if(!data)
		return;

	loadNoNativeMeshFromMemory(filename, data, size, skeletal, true);
	_DELETE_ARRAY(data);
}

void MeshLoader::saveEngineMesh(string& filename, MeshData* mesh, uint32_t** indices, uint8_t** vertices)
{
	string stm_file = filename.substr(0, filename.rfind('.')) + EXT_MESH;

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
		file_size += mesh->vertexBuffers[i].size * vetrexSize;
		file_size += mesh->indexBuffers[i].size * sizeof(uint32_t);
	}

	unique_ptr<uint8_t> data(new uint8_t[file_size]);
	uint8_t* t_data = data.get();

	*(MeshFileHeader*)t_data = header;
	t_data += sizeof(MeshFileHeader);

	for(uint32_t i = 0; i < header.materialCount; i++)
	{
		*(uint32_t*)t_data = mesh->vertexBuffers[i].size;
		t_data += sizeof(uint32_t);

		if(vertices[i])
		{
			uint32_t size = mesh->vertexBuffers[i].size * vetrexSize;
			memcpy(t_data, vertices[i], size);
			t_data += size;
		}

		*(uint32_t*)t_data = mesh->indexBuffers[i].size;
		t_data += sizeof(uint32_t);

		if(indices[i])
		{
			uint32_t size = mesh->indexBuffers[i].size * sizeof(uint32_t);
			memcpy(t_data, indices[i], size);
			t_data += size;
		}
	}

	if(!FileIO::WriteFileData( stm_file, data.get(), file_size ))
	{
		ERR("Cant write mesh file: %s", stm_file.c_str() );
	}
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
		vBuffer.size = *(uint32_t*)t_data; 
		t_data += sizeof(uint32_t);

		const auto sizeVB = vBuffer.size * vetrexSize;
		vertices[i] = new uint8_t[sizeVB];
		memcpy(vertices[i], t_data, sizeVB);
		t_data += sizeVB;

		mesh->vertexBuffers.push_back(vBuffer);
		
		GPUMeshBuffer iBuffer;
		iBuffer.size = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);

		const auto sizeIB = iBuffer.size * sizeof(uint32_t);
		indices[i] = new uint32_t[iBuffer.size];
		memcpy(indices[i], t_data, sizeIB);
		t_data += sizeIB;

		mesh->indexBuffers.push_back(iBuffer);
	}	

	for(uint32_t i = 0; i < header.materialCount; i++)
	{
		mesh->vertexBuffers[i].buffer = Buffer::CreateVertexBuffer(Render::Device(), vetrexSize * mesh->vertexBuffers[i].size, false, vertices[i]);
		mesh->indexBuffers[i].buffer = Buffer::CreateIndexBuffer(Render::Device(), sizeof(uint32_t) * mesh->indexBuffers[i].size, false, indices[i]);
		
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

	LOG("Mesh(.stm) loaded %s", filename.c_str());
	return mesh;
}

MeshData* MeshLoader::loadNoNativeMeshFromMemory(string& filename, uint8_t* data, uint32_t size, bool skeletal, bool onlyConvert)
{
	string extension = filename.substr(filename.rfind('.'));

	if( !meshImporter.IsExtensionSupported(extension) )
	{
		ERR("Extension %s is not supported for meshes", extension.data());
		return nullptr;
	}

	auto flags = aiProcessPreset_TargetRealtime_Fast | aiProcess_ConvertToLeftHanded;
	MeshVertexFormat format;
	if( skeletal )
	{
		format = MeshVertexFormat::LIT_SKINNED_VERTEX;
	}
	else
	{
		format = MeshVertexFormat::LIT_VERTEX;
		flags |= aiProcess_PreTransformVertices;
	}

	const aiScene* scene = meshImporter.ReadFileFromMemory( data, size, flags);

	if(!scene)
	{
		ERR("Import failed for mesh %s", filename.c_str());
		return nullptr;
	}

	MeshData* mesh = loadAIScene(filename, scene, format, onlyConvert, onlyConvert);
	meshImporter.FreeScene();

	if(onlyConvert)
	{
		LOG("Mesh %s converted to engine format", filename.c_str());
		_DELETE(mesh);
		return nullptr;
	}

	LOG("Mesh %s loaded", filename.c_str());
	return mesh;
}

MeshData* MeshLoader::loadAIScene(string& filename, const aiScene* scene, MeshVertexFormat format, bool convert, bool noGPUInit)
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

		iBuffer.size = mesh[i]->mNumFaces * 3;

		indices[i] = new uint32_t[iBuffer.size];
		uint32_t k = 0;
		for(uint32_t j = 0; j < mesh[i]->mNumFaces; j++)
		{
			for(uint32_t q = 0; q < 3; q++)
				indices[i][k+q] = mesh[i]->mFaces[j].mIndices[q];
			k += 3;
		}

		if( !noGPUInit )
		{
			iBuffer.buffer = Buffer::CreateIndexBuffer(Render::Device(), sizeof(uint32_t) * iBuffer.size, false, indices[i]);

			if ( !iBuffer.buffer )
			{
				ERR("Cant init mesh index buffer for %s", mesh[i]->mName.C_Str());
				continue;
			}
		}

		stmesh->indexBuffers.push_back(iBuffer);

		// VERTICES
		GPUMeshBuffer vBuffer;

		vBuffer.size = mesh[i]->mNumVertices;

		const uint32_t byteSize = vBuffer.size * vertexSize;
		vertices[i] = new uint8_t[byteSize];

		switch( format )
		{
		case MeshVertexFormat::LIT_VERTEX:
			loadVerticesLit(vertices[i], vBuffer.size, vertexSize, mesh[i], posMin, posMax);
			break;
		case MeshVertexFormat::LIT_SKINNED_VERTEX:
			loadVerticesSkinnedLit(vertices[i], vBuffer.size, vertexSize, mesh[i], boneOffset, posMin, posMax);
			break;
		}			

		if( !noGPUInit )
		{
			vBuffer.buffer = Buffer::CreateVertexBuffer(Render::Device(), byteSize, false, vertices[i]);

			if( !vBuffer.buffer )
			{
				ERR("Cant init static mesh vertex buffer for %s", mesh[i]->mName.C_Str());
				_RELEASE(stmesh->indexBuffers[stmesh->indexBuffers.size() - 1].buffer);
				stmesh->indexBuffers.pop_back();
				continue;
			}
		}

		stmesh->vertexBuffers.push_back(vBuffer);
	}

	Vector3 center = Vector3(0.5f * (posMin.x + posMax.x), 0.5f * (posMin.y + posMax.y), 0.5f * (posMin.z + posMax.z));
	Vector3 extents = Vector3(posMax.x - center.x, posMax.y - center.y, posMax.z - center.z);
	extents.x = std::max<float>(extents.x, 0);
	extents.y = std::max<float>(extents.y, 0);
	extents.z = std::max<float>(extents.z, 0);
	stmesh->box = BoundingBox(center, extents);

	if(convert)
		saveEngineMesh(filename, stmesh, indices, vertices);

	for(uint32_t i = 0; i < matCount; i++)
	{
		if(indices)
			_DELETE_ARRAY(indices[i]);
		if(vertices)
			_DELETE_ARRAY(vertices[i]);
	}
	_DELETE_ARRAY(indices);
	_DELETE_ARRAY(vertices);

	return stmesh;
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