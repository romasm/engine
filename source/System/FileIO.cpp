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

bool FileIO::IsExist(string& path)
{
	struct _stat32 buffer;   
	return (_stat32(path.data(), &buffer) == 0);  
}

bool FileIO::IsFile(string filename)
{
	struct _stat32 buffer;   
	if( _stat32(filename.data(), &buffer) != 0 )
		return false;
	return (buffer.st_mode & S_IFDIR) == 0;
}

uint32_t FileIO::GetDateModifRaw(string& filename)
{
	struct _stat32 buffer;
	if(_stat32(filename.data(), &buffer) != 0)
		return 0; 
	return uint32_t(buffer.st_mtime);
}

string FileIO::GetDateModif(string filename)
{
	struct _stat32 buffer;
	if(_stat32(filename.data(), &buffer) != 0)
		return 0;
	char str[26];
	ctime_s(str, 26, (const time_t*)(&buffer.st_mtime));
	return string(str);
}

string FileIO::GetDateCreate(string path)
{
	struct _stat32 buffer;
	if(_stat32(path.data(), &buffer) != 0)
		return 0;
	char str[26];
	ctime_s(str, 26, (const time_t*)(&buffer.st_ctime));
	return string(str);
}

uint32_t FileIO::GetSize(string filename)
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

bool FileIO::CreateDir(string path)
{
	if(IsExist(path))
	{
		WRN("Path %s allready exist", path.data());
		return false;
	}

	auto res = _mkdir(path.data());
	if( res == ENOENT )
	{
		auto slash = path.rfind('\\');
		if(slash == string::npos)
			slash = 0;
		slash = max(slash, path.rfind('/'));
		if(slash == string::npos)
		{
			WRN("Can't create directory %s", path.data());
			return false;
		}

		string newDir = path.substr(0, slash);

		if( !CreateDir(newDir) )
		{
			WRN("Can't create directory %s", path.data());
			return false;
		}
		
		res = _mkdir(path.data());
	}

	LOG_GOOD("Successfully created directory %s", path.data());
	return res == 0;
}

FileIO::DirList* FileIO::GetDirList(string dirname)
{
	DirList* res = new DirList;

	DIR *dir;
	if( (dir = opendir( dirname.data() )) == nullptr )
		return res;
	
	struct dirent *ent;
	while( (ent = readdir(dir)) != nullptr )
	{
		string name(ent->d_name);
		if( name[0] == '.' && ( name.size() == 1 ||	(name[1] == '.' && name.size() == 2) ) )
			continue;
		res->namesList.push_back(name);
	}
	closedir(dir);
	return res;
}

bool FileIO::Rename(string oldPath, string newPath)
{
	if(!IsExist(oldPath))
	{
		WRN("Path %s does not exist", oldPath.data());
		return false;
	}

	if(IsExist(newPath))
	{
		WRN("Path %s allready exist", newPath.data());
		return false;
	}

	auto slash = newPath.rfind('\\');
	if(slash == string::npos)
		slash = 0;
	slash = max(slash, newPath.rfind('/'));
	if(slash != string::npos)
	{
		string upDir = newPath.substr(0, slash);
		if(!IsExist(upDir))
			if(!CreateDir(upDir))
			{
				WRN("Can\'t rename %s to %s", oldPath.data(), newPath.data());
				return false;
			}
	}

	if( rename( oldPath.data(), newPath.data() ) != 0 )
	{
		WRN("Can\'t rename %s to %s", oldPath.data(), newPath.data());
		return false;
	}

	LOG_GOOD("Successfully renamed %s to %s", oldPath.data(), newPath.data());
	return true;
}

bool FileIO::copyDirContent(string& fromPath, string& toPath)
{
	if(!IsExist(toPath))
		if(!CreateDir(toPath))
			return false;

	DIR *dir;
	if( (dir = opendir( fromPath.data() )) == nullptr )
		return false;
	
	struct dirent *ent;
	while( (ent = readdir(dir)) != nullptr )
	{
		string name(ent->d_name);
		if( name[0] == '.' && ( name.size() == 1 ||	(name[1] == '.' && name.size() == 2) ) )
			continue;

		name = fromPath + '/' + name;
		string newName = toPath + '/' + name;

		if(IsFile(name))
		{
			if( FAILED(CopyFileA( (LPCSTR)name.data(), (LPCSTR)newName.data(), TRUE )) )
			{
				closedir(dir);
				return false;
			}
		}
		else
		{
			if(!copyDirContent(name, newName))
			{
				closedir(dir);
				return false;
			}
		}
	}
	closedir(dir);
	return true;
}

bool FileIO::Copy(string fromPath, string toPath)
{
	if(!IsExist(fromPath))
	{
		WRN("Path %s doesn\'t exist", fromPath.data());
		return false;
	}

	if(IsExist(toPath))
	{
		WRN("Path %s allready exist", toPath.data());
		return false;
	}

	if(IsFile(fromPath))
	{
		if( FAILED(CopyFileA( (LPCSTR)fromPath.data(), (LPCSTR)toPath.data(), TRUE )) )
		{
			WRN("Can\'t copy file %s to %s", fromPath.data(), toPath.data());
			return false;
		}
	}
	else
	{
		if(!CreateDir(toPath))
		{
			WRN("Can\'t copy directory %s to %s", fromPath.data(), toPath.data());
			return false;
		}
		if(!copyDirContent(fromPath, toPath))
		{
			WRN("Can\'t copy directory %s to %s", fromPath.data(), toPath.data());
			return false;
		}
	}

	LOG_GOOD("Successfully copied from %s to %s", fromPath.data(), toPath.data());
	return true;
}

bool FileIO::deleteDirContent(string& path)
{
	DIR *dir;
	if( (dir = opendir( path.data() )) == nullptr )
		return false;
	
	struct dirent *ent;
	while( (ent = readdir(dir)) != nullptr )
	{
		string name(ent->d_name);
		if( name[0] == '.' && ( name.size() == 1 ||	(name[1] == '.' && name.size() == 2) ) )
			continue;

		name = path + '/' + name;

		if(IsFile(name))
		{
			if( remove( name.data() ) != 0 )
			{
				closedir(dir);
				return false;
			}
		}
		else
		{
			if(!deleteDirContent(name))
			{
				closedir(dir);
				return false;
			}
		}
	}
	closedir(dir);
	return true;
}

bool FileIO::Delete(string path)
{
	if(!IsExist(path))
	{
		WRN("Path %s doesn\'t exist", path.data());
		return false;
	}

	if(IsFile(path))
	{
		if( remove( path.data() ) != 0 )
		{
			WRN("Can\'t delete file %s", path.data());
			return false;
		}
	}
	else
	{
		if(!deleteDirContent(path))
		{
			WRN("Can\'t delete directory %s", path.data());
			return false;
		}
		if( _rmdir( path.data() ) != 0 )
		{
			WRN("Can\'t delete directory %s", path.data());
			return false;
		}
	}

	LOG_GOOD("Successfully deleted %s", path.data());
	return true;
}