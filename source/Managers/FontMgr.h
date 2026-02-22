#pragma once
#include "stdafx.h"
#include "Font.h"

#define FONT_INIT_COUNT 32

namespace EngineCore
{
	class FontMgr
	{
	public:
		FontMgr();
		~FontMgr();

		inline static FontMgr* Get(){return instance;}

		Font* GetFont(const string& name);
		void DeleteFont(const string& name);

	private:
		Font* AddFontToList(const string& name);
		Font* FindFontInList(const string& name);

		struct FontHandle
		{
			Font* font;
			uint32_t refcount;
			string name;

			FontHandle()
			{
				font = nullptr;
				refcount = 0;
			}
		};

		static FontMgr* instance;

		unordered_map<string, FontHandle> font_map;
	};
}