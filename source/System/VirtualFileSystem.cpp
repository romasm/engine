#include "stdafx.h"
#include "VirtualFileSystem.h"
#include "FileIO.h"
#include "Util.h"

using namespace EngineCore;

Package::Package()
{
	packedData = nullptr;
	packedSize = 0;
	unwrapped = true;
}

Package::Package(string& path)
{
	packedData = nullptr;
	packedSize = 0;
	unwrapped = true;

	ReadFromDisk(path);
}

Package::~Package()
{
	Clear();
}

void Package::WrapUp()
{
	// TODO

	unwrapped = false;
}

void Package::Clear()
{
	for(auto& it: pkgMap)
	{
		_DELETE_ARRAY(it.second.dataPointer);
	}

	_DELETE_ARRAY(packedData);
	packedSize = 0;

	pkgMap.clear();
	pkgName.clear();
	pkgPath.clear();

	unwrapped = true;
}

bool Package::ReadFromDisk(string& path)
{
	if( GetExtension(path) != EXT_PACKAGE )
	{
		ERR("Package file %s has wrong extension", path.c_str());
		return false;
	}

	Clear();

	uint32_t size;
	uint8_t* data = FileIO::ReadFileData(path, &size);
	if(!data)
		return false;

	uint8_t* t_data = data;

	if( (*(uint32_t*)t_data) != PACKAGE_FILE_VERSION )
	{
		ERR("Package file %s has wrong version!", path.c_str());
		_DELETE_ARRAY(data);
		return false;
	}
	t_data += sizeof(uint32_t);

	uint32_t filesCount = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);

	for(uint32_t i = 0; i < filesCount; i++)
	{
		uint32_t stringSize = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);
		string fileName((char*)t_data, stringSize);
		t_data += sizeof(char) * stringSize;

		FileLink fileLink;
		fileLink.date = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);
		fileLink.offset = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);
		fileLink.size = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);

		pkgMap.insert(make_pair(fileName, fileLink));
	}

	uint32_t packedSize = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);

	packedData = new uint8_t[packedSize];
	memcpy(packedData, t_data, packedSize);

	_DELETE_ARRAY(data);

	pkgPath = path;
	pkgName = RemoveExtension(GetFilename(path));
	unwrapped = false;

	LOG_GOOD("Package %s loaded from disk", path.data());
	return true;
}

bool Package::WriteToDisk()
{
	if(pkgPath.empty())
	{
		ERR("Path must be specified for %s package before writing on disk", pkgName.data());
		return false;
	}

	if(unwrapped)
		WrapUp();

	uint32_t fileSize = sizeof(uint32_t) + sizeof(uint32_t);
	for(auto& it: pkgMap)
	{
		fileSize += sizeof(uint32_t) + (uint32_t)it.first.size() * sizeof(char);
		fileSize += sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);
	}
	fileSize += sizeof(uint32_t) + packedSize;

	uint8_t* data = new uint8_t[fileSize];
	uint8_t* t_data = data;

	*(uint32_t*)t_data = PACKAGE_FILE_VERSION;
	t_data += sizeof(uint32_t);
	
	*(uint32_t*)t_data = (uint32_t)pkgMap.size();
	t_data += sizeof(uint32_t);

	for(auto& it: pkgMap)
	{
		*(uint32_t*)t_data = (uint32_t)it.first.size();
		t_data += sizeof(uint32_t);

		memcpy(t_data, it.first.data(), it.first.size());
		t_data += it.first.size() * sizeof(char);
		
		*(uint32_t*)t_data = it.second.date;
		t_data += sizeof(uint32_t);

		*(uint32_t*)t_data = it.second.offset;
		t_data += sizeof(uint32_t);

		*(uint32_t*)t_data = it.second.size;
		t_data += sizeof(uint32_t);
	}

	*(uint32_t*)t_data = packedSize;
	t_data += sizeof(uint32_t);
	
	memcpy(t_data, packedData, packedSize);
	t_data += packedSize;

	bool status;
	if( !(status = FileIO::WriteFileData(pkgPath, data, fileSize)) )
	{
		ERR("Cant write package file %s", pkgPath.c_str() );
	}
	_DELETE_ARRAY(data);
	return status;
}