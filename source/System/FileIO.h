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
		wstring value;
		unordered_map<wstring, FileNode>* node;
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
	typedef unordered_map<wstring, FileNode> file_map;

public:
	FileIO(string& filename, bool overwrite = false)
	{init(filename, overwrite);}
	FileIO(char* filename, bool overwrite = false)
	{init(string(filename), overwrite);}

	~FileIO();

	bool Save(){return SaveAs(fileName);}
	bool SaveAs(string& file);
	inline bool SaveAsS(char* file)
	{return SaveAs(string(file));}

#define FUNCTION_READ_C(return_type, name) inline return_type name(string str, file_map* node){return name(StringToWstring(str), node);}
#define FUNCTION_WRITE_C(name, pass_type) inline void name(string str, pass_type value, file_map* node){name(StringToWstring(str), value, node);}

		file_map* Root(){return fileMap;}
		file_map* Node(wstring name, file_map* node)
			{if(!node)return nullptr; auto n = node->find(name);
			if(n == node->end())return nullptr; return n->second.node;}
		FUNCTION_READ_C(file_map*, Node)
		file_map* CreateNode(wstring name, file_map* node)
			{if(!node)return nullptr; auto n = node->find(name);
			if(n != node->end())return n->second.node; FileNode newnode;
			(*node)[name] = newnode; (*node)[name].node = new file_map; 
			return (*node)[name].node;}
		FUNCTION_READ_C(file_map*, CreateNode)
		void DeleteNode(wstring name, file_map* node)
			{if(!node)return; auto n = node->find(name);
			if(n == node->end())return; node->erase(n);}
		FUNCTION_READ_C(void, DeleteNode)

		wstring ReadString(wstring name, file_map* node)
			{if(!node)return L""; auto n = node->find(name); 
			if(n == node->end())return L""; return n->second.value;}
		inline string ReadString(string name, file_map* node)
		{return WstringToString(ReadString(StringToWstring(name), node));}

		int ReadInt(wstring name, file_map* node)
			{if(!node)return 0; auto n = node->find(name); 
			if(n == node->end())return 0; return WCharToInt((wchar_t*)n->second.value.data());}
		FUNCTION_READ_C(int, ReadInt)
		float ReadFloat(wstring name, file_map* node)
			{if(!node)return 0; auto n = node->find(name); 
			if(n == node->end())return 0; return WCharToFloat((wchar_t*)n->second.value.data());}
		FUNCTION_READ_C(float, ReadFloat)
		bool ReadBool(wstring name, file_map* node)
			{if(!node)return false; auto n = node->find(name); 
			if(n == node->end())return false; return WCharToBool((wchar_t*)n->second.value.data());}
		FUNCTION_READ_C(bool, ReadBool)
		XMFLOAT3 ReadFloat3(wstring name, file_map* node)
			{if(!node)return XMFLOAT3(0,0,0); auto n = node->find(name); 
			if(n == node->end())return XMFLOAT3(0,0,0); return WCharToXMFloat3((wchar_t*)n->second.value.data());}
		FUNCTION_READ_C(XMFLOAT3, ReadFloat3)
		XMFLOAT4 ReadFloat4(wstring name, file_map* node)
			{if(!node)return XMFLOAT4(0,0,0,0); auto n = node->find(name); 
			if(n == node->end())return XMFLOAT4(0,0,0,0); return WCharToXMFloat4((wchar_t*)n->second.value.data());}
		FUNCTION_READ_C(XMFLOAT4, ReadFloat4)
		uint8_t ReadByte(wstring name, file_map* node)
			{if(!node)return 0; auto n = node->find(name); 
			if(n == node->end())return 0; return WCharToByte((wchar_t*)n->second.value.data());}
		FUNCTION_READ_C(uint8_t, ReadByte)

		bool IsNodeExist(wstring name, file_map* node)
			{if(!node)return false; auto n = node->find(name); 
			if(n == node->end())return false; return true;}
		FUNCTION_READ_C(bool, IsNodeExist)

		void WriteString(wstring name, wstring value, file_map* node)
			{if(!node)return; auto n = node->find(name); 
			if(n != node->end())n->second.value = value; else{FileNode newvalue;
			(*node)[name] = newvalue; (*node)[name].value = value;}}
		inline void WriteString(string name, string value, file_map* node){WriteString(StringToWstring(name), StringToWstring(value), node);}
		void WriteInt(wstring name, int value, file_map* node)
			{if(!node)return; auto n = node->find(name);
			if(n != node->end())n->second.value = IntToWString(value); else{FileNode newvalue;
			(*node)[name] = newvalue; (*node)[name].value = IntToWString(value);}}
		FUNCTION_WRITE_C(WriteInt, int)
		void WriteFloat(wstring name, float value, file_map* node)
			{if(!node)return; auto n = node->find(name);
			if(n != node->end())n->second.value = FloatToWString(value); else{FileNode newvalue;
			(*node)[name] = newvalue; (*node)[name].value = FloatToWString(value);}}
		FUNCTION_WRITE_C(WriteFloat, float)
		void WriteBool(wstring name, bool value, file_map* node)
			{if(!node)return; auto n = node->find(name);
			if(n != node->end())n->second.value = BoolToWSring(value); else{FileNode newvalue;
			(*node)[name] = newvalue; (*node)[name].value = BoolToWSring(value);}}
		FUNCTION_WRITE_C(WriteBool, bool)
		void WriteFloat3(wstring name, XMFLOAT3 value, file_map* node)
			{if(!node)return; auto n = node->find(name);
			if(n != node->end())n->second.value = XMFloat3ToWSring(value); else{FileNode newvalue;
			(*node)[name] = newvalue; (*node)[name].value = XMFloat3ToWSring(value);}}
		FUNCTION_WRITE_C(WriteFloat3, XMFLOAT3)
		void WriteFloat4(wstring name, XMFLOAT4 value, file_map* node)
			{if(!node)return; auto n = node->find(name);
			if(n != node->end())n->second.value = XMFloat4ToWSring(value); else{FileNode newvalue;
			(*node)[name] = newvalue; (*node)[name].value = XMFloat4ToWSring(value);}}
		FUNCTION_WRITE_C(WriteFloat4, XMFLOAT4)
		void WriteByte(wstring name, uint8_t value, file_map* node)
			{if(!node)return; auto n = node->find(name);
			if(n != node->end())n->second.value = ByteToWString(value); else{FileNode newvalue;
			(*node)[name] = newvalue; (*node)[name].value = ByteToWString(value);}}
		FUNCTION_WRITE_C(WriteByte, uint8_t)

private:
	void init(string& filename, bool overwrite);

	void buildMap(uint8_t* fileData, uint32_t fileSize);
	void buildBlockMap(char* data, uint32_t dataSize, uint32_t& current, file_map* mmap);		
	void readValue(char* data, uint32_t dataSize, uint32_t& current, FileNode* node);	

	void writeMap(ofstream* file, file_map* node, uint16_t depth);		

	static bool deleteDirContent(string& path);
	static bool copyDirContent(string& fromPath, string& toPath);

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
	static bool Copy(string fromPath, string toPath);
	static bool Delete(string path);

	static uint32_t GetDateModifRaw(string& filename);
	inline static uint32_t GetDateModifRawS(char* filename){return GetDateModifRaw(string(filename));}
	inline static uint32_t GetDateModifRaw_lua(string filename){return GetDateModifRaw(filename);}
	static string GetDateModif(string filename);
	static string GetDateCreate(string path);
	static uint32_t GetSize(string filename);

	static uint8_t* ReadFileData(string& filename, uint32_t *ret_size);
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
				.addFunction("Delete", &FileIO::Delete)

				.addFunction("GetFileDateModifRaw", &FileIO::GetDateModifRaw_lua)
				.addFunction("GetFileDateModif", &FileIO::GetDateModif)
				.addFunction("GetFileDateCreate", &FileIO::GetDateCreate)
				.addFunction("GetFileSize", &FileIO::GetSize)

				.beginClass<DirList>("DirList")
					.addFunction("getnext", &DirList::next)
					.addFunction("reset", &DirList::reset)
					.addFunction("size", &DirList::size)
					.addFunction("destruct", &DirList::destruct)
				.endClass()
			.endNamespace();
	}
};

}