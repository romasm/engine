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
		ERR(LuaVM уже был создан);*/
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
	getGlobalNamespace(LSTATE)
		// common data types
		.beginClass<Quaternion>("Quaternion")
			.addData("x", &Quaternion::x)
			.addData("y", &Quaternion::y)
			.addData("z", &Quaternion::z)
			.addData("w", &Quaternion::w)
			.addConstructor<void (*)(float, float, float, float)>() 
			.addStaticData("zero", &Quaternion::Identity, false)
		.endClass()
		.beginClass<Vector4>("Vector4")
			.addData("x", &Vector4::x)
			.addData("y", &Vector4::y)
			.addData("z", &Vector4::z)
			.addData("w", &Vector4::w)
			.addConstructor<void (*)(float, float, float, float)>() 
		.endClass()
		.addFunction("Vector4Lerp", &Vector4Lerp)
		.beginClass<Vector3>("Vector3")
			.addData("x", &Vector3::x)
			.addData("y", &Vector3::y)
			.addData("z", &Vector3::z)
			.addConstructor<void (*)(float, float, float)>() 
		.endClass()
		.addFunction("Vector3Lerp", &Vector3Lerp)
		.beginClass<Vector2>("Vector2")
			.addData("x", &Vector2::x)
			.addData("y", &Vector2::y)
			.addConstructor<void (*)(float, float)>() 
		.endClass()
		.addFunction("Vector2Lerp", &Vector2Lerp)
		.beginClass<MLRECT>("Rect")
			.addData("l", &MLRECT::left)
			.addData("t", &MLRECT::top)
			.addData("w", &MLRECT::width)
			.addData("h", &MLRECT::height)
			.addConstructor<void (*)(int, int, int, int)>() 
		.endClass()
		.beginClass<RECT>("WinRect")
			.addData("l", &RECT::left)
			.addData("t", &RECT::top)
			.addData("r", &RECT::right)
			.addData("b", &RECT::bottom)
			.addConstructor<void (*)(void)>()
		.endClass()
		.beginClass<POINT>("Point")
			.addData("x", &POINT::x)
			.addData("y", &POINT::y)
		.endClass()
		.beginClass<luaStringList>("StringList")
			.addConstructor<void (*)(void)>() 
			.addFunction("Add", &luaStringList::Add)
			.addFunction("Get", &luaStringList::Get)
			.addFunction("Size", &luaStringList::Size)
		.endClass()

		// winAPI dialogs
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
        
		ERR("%s(%i)%s", entry.short_src, entry.currentline, entry.name ? entry.name : "?");
		depth++;
    }
	ERR("%s", str.c_str());
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
