#include "stdafx.h"
#include "FileIO.h"
#include "Util.h"

using namespace EngineCore;

void FileIO::init(string& file, bool overwrite)
{
	fileMap = nullptr;
	fileName = file;

	if(IsExist(fileName) && !overwrite)
	{
		uint8_t* fileData = nullptr;
		uint32_t fileSize = 0;

		fileData = FileIO::ReadFileData(fileName, &fileSize);
		if(!fileData)
		{
			ERR("Cant read file %s", fileName.c_str());
			return;
		}

		buildMap(fileData, fileSize);
		_DELETE(fileData);
	}
}

FileIO::~FileIO()
{
	if(fileMap)
	{
		fileMap->clear();
		delete fileMap;
	}
}

void FileIO::buildMap(uint8_t* fileData, uint32_t fileSize)
{
	fileMap = new file_map;

	char* data = (char*)fileData;
	uint32_t dataSize = fileSize / sizeof(char);
	uint32_t current = 0;

	buildBlockMap(data, dataSize, current, fileMap);
}

void FileIO::buildBlockMap(char* data, uint32_t dataSize, uint32_t& current, file_map* mmap)
{
	FileNode* node = nullptr;
	FileNode* prev_node = nullptr;
	bool looking_for_block = false;
	while( current < dataSize )
	{
		if( data[current] == '}' )
		{
			current++;
			return;
		}

		if( !node )
		{
			if( data[current] == ' ' || data[current] == '\t' || data[current] == '\n' || data[current] == '\r' || data[current] == 0 || data[current] == ';' )
			{
				if( data[current] == ';' )
					prev_node = nullptr;
				current++;
				continue;
			}
		}
		else
		{
			if( data[current] == ';' )
			{
				current++;
				node = nullptr;
				looking_for_block = false;
				continue;
			}

			if( data[current] == '\n' )
			{
				current++;
				looking_for_block = true;
				continue;
			}

			if( data[current] == ' ' || data[current] == '\t' || data[current] == '\r' || data[current] == '=' )
			{
				current++;
				continue;
			}
		}

		// comments skip
		if( current < dataSize - 1 ) 
		{
			if( data[current] == '/')
			{
				if( data[current+1] == '/') // line comment
				{
					node = nullptr;
					looking_for_block = true;
					current += 2;

					if( current < dataSize && data[current] == '~')
					{
						current += 1;
						while( current < dataSize - 2 )
						{
							if( data[current] == '/' && data[current+1] == '/' && data[current+2] == '~' )
								break;
							current ++;
						}
						current += 3;
						continue;
					}
					else
					{
						while( current < dataSize && data[current] != '\n' )
							current++;
						current++;
						continue;
					}
				}
				else if( data[current+1] == '*' ) // block comment
				{
					current += 2;
					while( current < dataSize - 1 )
					{
						if( data[current] == '*' && data[current+1] == '/' )
							break;
						current++;
					}
					current += 2;
					continue;
				}
			}
		}
		// comments skip

		if( !node )
		{
			if(prev_node)
			{
				if( data[current] == '{' )
				{
					prev_node->node = new file_map;
					current++;
					buildBlockMap(data, dataSize, current, prev_node->node);
					prev_node = nullptr;
					continue;
				}
				prev_node = nullptr;
			}

			auto start = current;
			while( current < dataSize && data[current] != ' ' && data[current] != '\t' && data[current] != '\r' && 
				data[current] != '\n' && data[current] != 0 && data[current] != ';' && 
				data[current] != '/' && data[current] != '=' && 
				data[current] != '\"' && data[current] != '\'' && data[current] != '{' )
			{
				if( data[current] == '}' )
				{
					current++;
					return;
				}
				current++;
			}
			if(start >= current-1)
			{
				while( current < dataSize && data[current] != '\n' && data[current] != ';' )
				{
					if( data[current] == '}' )
					{
						current++;
						return;
					}
					current++;
				}
				current++;
				continue;
			}

			prev_node = nullptr;

			wstring attr = StringToWstring(string(data+start, current-start));
			auto& nodeEmpty = FileNode();
			mmap->insert(make_pair(attr, nodeEmpty));
			node = &mmap->at(attr);
			continue;
		}
		else
		{
			if( data[current] == '{' )
			{
				node->node = new file_map;
				current++;
				buildBlockMap(data, dataSize, current, node->node);
				node = nullptr;
				continue;
			}
			else
			{
				readValue(data, dataSize, current, node);
				prev_node = node;
				node = nullptr;
				continue;
			}
		}
	}
}

void FileIO::readValue(char* data, uint32_t dataSize, uint32_t& current, FileNode* node)
{
	bool quoters = false;
	if(data[current] == '\'' || data[current] == '\"')
	{
		quoters = true;
		current++;
	}

	auto start = current;
	if(quoters)
		while( current < dataSize && data[current] != '\"' && data[current] != '\''  )
			current++;
	else
		while( current < dataSize && data[current] != '\n' && data[current] != '\r' && data[current] != '\t' && data[current] != ';' && data[current] != '/' && data[current] != '}' && data[current] != '{' )
			current++;

	node->value = StringToWstring(string(data+start, current-start));
	if( current < dataSize && data[current] != '/' && data[current] != '}' && data[current] != '{' )
		current++;
}

bool FileIO::SaveAs(string& file)
{
	fileName = file;

	if(!fileMap)
		return false;

	ofstream stream;

	stream.open(fileName, ofstream::out | ofstream::trunc | ofstream::binary);
	if(!stream.good())
		return false;

	writeMap(&stream, fileMap, 0);

	stream.close();
	return true;
}

void FileIO::writeMap(ofstream* file, file_map* node, uint16_t depth)
{
	for(auto& it: *node)
	{
		if(it.second.node)
		{
			string tab_str = "";
			for(uint16_t i = 0; i < depth; i++)
				tab_str += '\t';

			string str = tab_str + WstringToString(it.first);
			if(it.second.value.length() > 0)
			{
				str += " \'";
				str += WstringToString(it.second.value);
				str += "\'";
			}
			str += '\n';
			file->write((char*)str.data(), str.length() * sizeof(char));

			str = tab_str + "{\n";
			file->write((char*)str.data(), str.length() * sizeof(char));

			writeMap(file, it.second.node, depth + 1);

			str = tab_str + "}\n";
			file->write((char*)str.data(), str.length() * sizeof(char));
		}
		else
		{
			string str = "";
			for(uint16_t i = 0; i < depth; i++)
				str += '\t';
			str += WstringToString(it.first);
			str += " = \'";
			str += WstringToString(it.second.value);
			str += "\'\n";
			file->write((char*)str.data(), str.length() * sizeof(char));
		}
	}
}

// static

bool FileIO::IsExist(string& filename)
{
	struct _stat32 buffer;   
	return (_stat32(filename.data(), &buffer) == 0);  
}

uint32_t FileIO::GetDateModifRaw(string& filename)
{
	struct _stat32 buffer;
	if(_stat32(filename.data(), &buffer) != 0)
		return 0; 
	return uint32_t(buffer.st_mtime);
}

string FileIO::GetDateModif(string& filename)
{
	struct _stat32 buffer;
	if(_stat32(filename.data(), &buffer) != 0)
		return 0;
	char str[26];
	ctime_s(str, 26, (const time_t*)(&buffer.st_mtime));
	return string(str);
}

string FileIO::GetDateCreate(string& filename)
{
	struct _stat32 buffer;
	if(_stat32(filename.data(), &buffer) != 0)
		return 0;
	char str[26];
	ctime_s(str, 26, (const time_t*)(&buffer.st_ctime));
	return string(str);
}

uint32_t FileIO::GetSize(string& filename)
{
	struct _stat32 buffer;
	if(_stat32(filename.data(), &buffer) != 0)
		return 0;
	return buffer.st_size;
}

uint8_t* FileIO::ReadFileData(string& filename, uint32_t *ret_size)
{
	ifstream stream;
	uint32_t size;
	uint8_t* data;

	stream.open(filename, ifstream::in | ifstream::binary);
	if(!stream.good())
		return nullptr;
	
	stream.seekg(0, ios::end);
	size = uint32_t(stream.tellg());
	data = new uint8_t[size];
	stream.seekg(0, ios::beg);
	stream.read((char*)data, size);
	stream.close();
		
	*ret_size = size;
	return data;
}

bool FileIO::WriteFileData(string& filename, uint8_t* data, uint32_t size, uint32_t modif_date)
{
	ofstream stream;

	stream.open(filename, ofstream::out | ofstream::trunc | ofstream::binary);
	if(!stream.good())
		return false;

	if(modif_date != 0)
		stream.write((char*)(&modif_date), sizeof(uint32_t));
	
	stream.write((char*)data, size);
	stream.close();
	
	return true;
}