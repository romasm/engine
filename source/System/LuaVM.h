#pragma once
#include "Log.h"
#include "macros.h"

namespace EngineCore
{
#define LSTATE LuaVM::Get()->GetState()
#define LUA_CALL(call, err) try{call;} catch(LuaException e){err;}

	class LuaVM
	{
	public:
		LuaVM();
		~LuaVM();

		bool LoadScript(string& filename);

		lua_State* GetState(){return L;}

		inline static LuaVM* Get(){return m_instance;}
		
	private:
		static LuaVM* m_instance;

		void RegLuaToolClass();

		bool Init();
		void Close();
		void HandleError();

		lua_State *L;
	};

	class luaStringList
	{
	public:
		luaStringList(){;}
		~luaStringList()
		{strs.clear();}

		void Add(string str)
		{strs.push_back(str);}

		string Get(int i)
		{
			if(i<0 || i>=(int)strs.size())
				return 0;
			return strs[i];
		}

		int Size()
		{return (int)strs.size();}

	private:
		vector<string> strs;
	};

	struct luaSRV
	{
		ID3D11ShaderResourceView* srv;
		luaSRV(){srv = nullptr;}
		luaSRV(ID3D11ShaderResourceView* view){srv = view;}
		bool is_null(){return srv == nullptr;}
	};
}