#include "stdafx.h"
#include "LuaVM.h"
#include "FileIO.h"
#include "Util.h"
#include "Text.h"
#include "LocalTimer.h"
#include "WinAPIDialogs.h"

using namespace EngineCore;

LuaVM* LuaVM::m_instance = nullptr;

LuaVM::LuaVM()
{	
	if (!m_instance)
	{
		m_instance = this;
		L = nullptr;
		Init();
	}
	/*else
		ERR(LuaVM ��� ��� ������);*/
}

LuaVM::~LuaVM()
{
	m_instance = nullptr;
	Close();
}

static int printfunc(lua_State* L)
{
	int nargs = lua_gettop(L);

	string output;
	for (int i=1; i <= nargs; ++i) 
		output += lua_tostring(L, i);

	LUA("%s", output.data());

	return 0;
}

static int errorfunc(lua_State* L) // todo
{
	int nargs = lua_gettop(L);

	string output;
	for (int i=1; i <= nargs; ++i) 
		output += lua_tostring(L, i);

	LUA_ERROR("%s", output.data());

	return 0;
}

template<typename F, typename T>
static T* castTypeLuaPtr(F* p)
{
	return static_cast<T*>(p);
}
#define ADD_LUACAST_PTR(from, to) .addFunction(#from"_to_"#to, &castTypeLuaPtr<##from, ##to>)

template<typename F, typename T>
static T castTypeLua(F p)
{
	return static_cast<T>(p);
}
#define ADD_LUACAST(from, to) .addFunction(#from"_to_"#to, &castTypeLua<##from, ##to>)

bool LuaVM::Init()
{
	L = luaL_newstate();
    luaL_openlibs(L);

	getGlobalNamespace(L)
		.addFunction("print", &printfunc)
		.addFunction("error", &errorfunc)
		
		.beginNamespace("cast")
		ADD_LUACAST(LONG, int)
		.endNamespace();

	RegLuaToolClass();
	Timer::RegLuaClass();
	LocalTimer::RegLuaClass();
	FileIO::RegLuaFunctions();

	// temp
	Text::RegLuaClass();
	Material::RegLuaClass();
	SimpleShaderInst::RegLuaClass();

	return true;
}

void LuaVM::RegLuaToolClass()
{
	RegLuaMath();

	getGlobalNamespace(LSTATE)
		.beginClass<luaStringList>("StringList")
			.addConstructor<void (*)(void)>() 
			.addFunction("Add", &luaStringList::Add)
			.addFunction("Get", &luaStringList::Get)
			.addFunction("Size", &luaStringList::Size)
		.endClass()

		.beginClass<winDialogFilter>("dlgFilter")
			.addFunction("Add", &winDialogFilter::_Add)
			.addConstructor<void (*)(void)>()
		.endClass()
		.addFunction("dlgOpenFile", &_winDialogOpenFile)
		.addFunction("dlgOpenFolder", &_winDialogOpenFolder)
		.addFunction("dlgOpenFilesMultiple", &_winDialogOpenFilesMultiple)
		.addFunction("dlgSaveFile", &_winDialogSaveFile)

		.beginClass<luaSRV>("SRV")
			.addFunction("is_null", &luaSRV::is_null)
		.endClass();
}

void LuaVM::Close()
{
	//lua_gc(L, LUA_GCCOLLECT, 0);
}

void LuaVM::HandleError()
{
    std::string str = lua_tostring(L, lua_gettop(L));
    lua_pop(L, 1);
    
    lua_Debug entry;
    int depth = 0;
    
    int v;
    while ((v = lua_getstack(L, depth, &entry)) != 0)
    {
        int status = lua_getinfo(L, "Sln", &entry);
        
		LUA_ERROR("%s(%i)%s", entry.short_src, entry.currentline, entry.name ? entry.name : "?");
		depth++;
    }
	LUA_ERROR("%s", str.c_str());
}

bool LuaVM::LoadScript(string& filename)
{  
	string sourceFile = filename + EXT_SCRIPT;
	string comlipedFile = filename + EXT_SCRIPT_COMPILED;
	if( FileIO::IsExist(comlipedFile) )
		sourceFile = comlipedFile;

	LOG("Loading script %s", sourceFile.data());
	int error = luaL_dofile(L, sourceFile.data());
    if(error)
	{
        HandleError();
    }
	else
	{
		LOG("Script %s is loaded", sourceFile.data());
        return true;
    }
	
    return false;
}
