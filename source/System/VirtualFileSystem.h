#pragma once
#include "Log.h"
#include "DataTypes.h"
#include "macros.h"
#include "Util.h"
#include "LuaVM.h"
#include "Arrays.h"

#define PACKAGE_FILE_VERSION 101

namespace EngineCore
{

class Package
{
public:
	Package();
	Package(string& path);
	~Package();

	void WrapUp();

	bool ReadFromDisk(string& path);
	bool WriteToDisk();

private:
	void Clear();

	struct FileLink
	{
		uint32_t date;
		uint32_t offset;
		uint32_t size;
		uint8_t* dataPointer;

		FileLink() : date(0), offset(0), size(0), dataPointer(nullptr) {}
	};

	string pkgName;
	string pkgPath;

	unordered_map<string, FileLink> pkgMap;

	uint32_t packedSize;
	uint8_t* packedData;

	bool unwrapped;

public:

	static void RegLuaFunctions()
	{
		getGlobalNamespace(LSTATE)
			.beginNamespace("FileIO")
				.addFunction("IsExist", &FileIO::IsExist_lua)
				.addFunction("IsFile", &FileIO::IsFile)

				.addFunction("CreateDir", &FileIO::CreateDir)
				.addFunction("GetDirList", &FileIO::GetDirList)

				.addFunction("Rename", &FileIO::Rename)
				.addFunction("Copy", &FileIO::Copy)
				.addFunction("CopyByExt", &FileIO::CopyByExt)
				.addFunction("Delete", &FileIO::Delete)

				.addFunction("GetFileDateModifRaw", &FileIO::GetDateModifRaw_lua)
				.addFunction("GetFileDateModif", &FileIO::GetDateModif)
				.addFunction("GetFileDateCreate", &FileIO::GetDateCreate)
				.addFunction("GetFileSize", &FileIO::GetSize)

				.beginClass<DirList>("DirList") // TODO: not safe, rework
					.addFunction("getnext", &DirList::next)
					.addFunction("reset", &DirList::reset)
					.addFunction("size", &DirList::size)
					.addFunction("destruct", &DirList::destruct)
				.endClass()
			.endNamespace();
	}
};

}