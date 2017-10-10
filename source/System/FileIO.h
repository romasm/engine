#pragma once
#include "Log.h"
#include "DataTypes.h"
#include "macros.h"
#include "Util.h"
#include "LuaVM.h"
#include "Arrays.h"

namespace EngineCore
{

class FileIO  // FLOAT COMMA AND ACCURACITY
{
private:

	struct FileNode
	{
		string value;
		unordered_map<string, FileNode>* node;
		FileNode()
		{
			node = nullptr;
		}
		~FileNode()
		{
			if(node)
			{
				node->clear();
				delete node;
			}
		}
	};

	struct DirList
	{
		DArray<string> namesList;
		uint32_t current;

		DirList()
		{ current = 0; }
		~DirList(){}

		string next()
		{
			if( current >= namesList.size() )
			{
				current = 0;
				return "";
			}
			current++;
			return namesList[current - 1];
		}
		void reset()
		{
			current = 0;
		}
		int32_t size()
		{
			return (int32_t)namesList.size();
		}
		void destruct()
		{
			delete this;
		}
	};

public:
	typedef unordered_map<string, FileNode> file_map;

public:
	FileIO(string& filename, bool overwrite = false)
	{init(filename, overwrite);}
	FileIO(char* filename, bool overwrite = false)
	{init(string(filename), overwrite);}

	~FileIO();

	inline bool Empty() {return fileMap->empty();}

	bool Save(){return SaveAs(fileName);}
	bool SaveAs(string& file);
	inline bool SaveAsS(char* file)
	{return SaveAs(string(file));}
	
		file_map* Root(){return fileMap;}
		file_map* Node(string name, file_map* node)
			{if(!node)return nullptr; auto n = node->find(name);
			if(n == node->end())return nullptr; return n->second.node;}
		
		file_map* CreateNode(string name, file_map* node)
			{if(!node)return nullptr; auto n = node->find(name);
			if(n != node->end())return n->second.node; FileNode newnode;
			(*node)[name] = newnode; (*node)[name].node = new file_map; 
			return (*node)[name].node;}
		
		void DeleteNode(string name, file_map* node)
			{if(!node)return; auto n = node->find(name);
			if(n == node->end())return; node->erase(n);}
		
		string ReadString(string name, file_map* node)
			{if(!node)return ""; auto n = node->find(name); 
			if(n == node->end())return ""; return n->second.value;}

		int ReadInt(string name, file_map* node)
			{if(!node)return 0; auto n = node->find(name); 
			if(n == node->end())return 0; return CharToInt((char*)n->second.value.data());}

		uint32_t ReadUint(string name, file_map* node)
			{if(!node)return 0; auto n = node->find(name); 
			if(n == node->end())return 0; return CharToUint((char*)n->second.value.data());}
		
		float ReadFloat(string name, file_map* node)
			{if(!node)return 0; auto n = node->find(name); 
			if(n == node->end())return 0; return CharToFloat((char*)n->second.value.data());}
		
		bool ReadBool(string name, file_map* node)
			{if(!node)return false; auto n = node->find(name); 
			if(n == node->end())return false; return CharToBool((char*)n->second.value.data());}
		
		Vector3 ReadFloat3(string name, file_map* node)
			{if(!node)return Vector3(0,0,0); auto n = node->find(name); 
			if(n == node->end())return Vector3(0,0,0); return CharToVector3((char*)n->second.value.data());}
		
		Vector4 ReadFloat4(string name, file_map* node)
			{if(!node)return Vector4(0,0,0,0); auto n = node->find(name); 
			if(n == node->end())return Vector4(0,0,0,0); return CharToVector4((char*)n->second.value.data());}
		
		uint8_t ReadByte(string name, file_map* node)
			{if(!node)return 0; auto n = node->find(name); 
			if(n == node->end())return 0; return CharToByte((char*)n->second.value.data());}
		
		bool IsNodeExist(string name, file_map* node)
			{if(!node)return false; auto n = node->find(name); 
			if(n == node->end())return false; return true;}
		
		void WriteString(string name, string value, file_map* node)
			{if(!node)return; auto n = node->find(name); 
			if(n != node->end())n->second.value = value; else{FileNode newvalue;
			(*node)[name] = newvalue; (*node)[name].value = value;}}

		void WriteInt(string name, int value, file_map* node)
			{if(!node)return; auto n = node->find(name);
			if(n != node->end())n->second.value = IntToString(value); else{FileNode newvalue;
			(*node)[name] = newvalue; (*node)[name].value = IntToString(value);}}

		void WriteUint(string name, uint32_t value, file_map* node)
			{if(!node)return; auto n = node->find(name);
			if(n != node->end())n->second.value = UintToString(value); else{FileNode newvalue;
			(*node)[name] = newvalue; (*node)[name].value = UintToString(value);}}
		
		void WriteFloat(string name, float value, file_map* node)
			{if(!node)return; auto n = node->find(name);
			if(n != node->end())n->second.value = FloatToString(value); else{FileNode newvalue;
			(*node)[name] = newvalue; (*node)[name].value = FloatToString(value);}}
		
		void WriteBool(string name, bool value, file_map* node)
			{if(!node)return; auto n = node->find(name);
			if(n != node->end())n->second.value = BoolToString(value); else{FileNode newvalue;
			(*node)[name] = newvalue; (*node)[name].value = BoolToString(value);}}
		
		void WriteFloat3(string name, Vector3 value, file_map* node)
			{if(!node)return; auto n = node->find(name);
			if(n != node->end())n->second.value = Vector3ToString(value); else{FileNode newvalue;
			(*node)[name] = newvalue; (*node)[name].value = Vector3ToString(value);}}
		
		void WriteFloat4(string name, Vector4 value, file_map* node)
			{if(!node)return; auto n = node->find(name);
			if(n != node->end())n->second.value = Vector4ToString(value); else{FileNode newvalue;
			(*node)[name] = newvalue; (*node)[name].value = Vector4ToString(value);}}
		
		void WriteByte(string name, uint8_t value, file_map* node)
			{if(!node)return; auto n = node->find(name);
			if(n != node->end())n->second.value = ByteToString(value); else{FileNode newvalue;
			(*node)[name] = newvalue; (*node)[name].value = ByteToString(value);}}
		

private:
	void init(string& filename, bool overwrite);

	void buildMap(uint8_t* fileData, uint32_t fileSize);
	void buildBlockMap(char* data, uint32_t dataSize, uint32_t& current, file_map* mmap);		
	void readValue(char* data, uint32_t dataSize, uint32_t& current, FileNode* node);	

	void writeMap(ofstream* file, file_map* node, uint16_t depth);		

	static bool deleteDirContent(string& path);
	static bool copyDirContent(string& fromPath, string& toPath, string& ext);

	file_map* fileMap;
	string fileName;

public:
	static bool IsExist(string& path);
	inline static bool IsExistS(char* path){return IsExist(string(path));}
	inline static bool IsExist_lua(string path){return IsExist(path);}
	static bool IsFile(string filename);

	static bool CreateDir(string path);
	static DirList* GetDirList(string dirname);

	static bool Rename(string oldPath, string newPath);
	static bool Copy(string fromPath, string toPath)
	{ return CopyByExt(fromPath, toPath, ""); }
	static bool CopyByExt(string fromDir, string toDir, string ext);
	static bool Delete(string path);

	static uint32_t GetDateModifRaw(string& filename);
	inline static uint32_t GetDateModifRawS(char* filename){return GetDateModifRaw(string(filename));}
	inline static uint32_t GetDateModifRaw_lua(string filename){return GetDateModifRaw(filename);}
	static string GetDateModif(string filename);
	static string GetDateCreate(string path);
	static uint32_t GetSize(string filename);

	static uint8_t* ReadFileData(string& filename, uint32_t *ret_size, bool noWarning = false);
	inline static uint8_t* ReadFileDataS(char* filename, uint32_t *ret_size){return ReadFileData(string(filename), ret_size);}
	static bool WriteFileData(string& filename, uint8_t* data, uint32_t size, uint32_t modif_date = 0);
	inline static bool WriteFileDataS(char* filename, uint8_t* data, uint32_t size, uint32_t modif_date = 0){return WriteFileData(string(filename), data, size, modif_date);}

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