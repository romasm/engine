#pragma once

#include "HDefines.h"
#include "Material.h"
#include "Common.h"
#include "WindowsMgr.h"

using namespace EngineCore;

#define MAX_RECTS_PER_WINDOW 4096
#define MAX_TEXTS_PER_WINDOW 1024

struct ScreenRect
{
	int16_t l;
	int16_t t;
	int16_t w;
	int16_t h;

	ScreenRect(): l(0), t(0), w(0), h(0) {}
	ScreenRect& operator=(const ScreenRect& r)
	{
		l = r.l;
		t = r.t;
		w = r.w;
		h = r.h;
		return *this;
	}
};

struct RectData // 18 bites
{
	ScreenRect rect;
	uint16_t depth;
	SimpleShaderInst* shaderInst;
	uint16_t id;
	uint32_t rectHash;

	RectData(): depth(0), shaderInst(nullptr), id(MAX_RECTS_PER_WINDOW), rectHash(0) {}
	RectData& operator=(const RectData& r)
	{
		rect = r.rect;
		depth = r.depth;
		shaderInst = r.shaderInst;
		id = r.id;
		rectHash = r.rectHash;
		return *this;
	}
};

class HRectMgr
{
public:
	HRectMgr();
	~HRectMgr();

	RectData* AddRect(int16_t winId);
	void DeleteRects(int16_t winId, uint16_t id);

	RectData* GetRect(int16_t winId, uint16_t id);

	inline static HRectMgr* Get() {return instance;}

private:
	static HRectMgr* instance;

	void allocWindow(int16_t winId);

	struct GuiQuadVertex
	{
		XMFLOAT4 vertex[4];
	};
	struct GuiQuadIndex
	{
		uint32_t index[6];
	};
	struct GuiGroups
	{
		uint16_t begin;
		uint16_t end;
		SimpleShaderInst* shaderInst;
	};

	struct RectsPerWin
	{
		// 112 kb
		SArray<uint16_t, MAX_RECTS_PER_WINDOW> ids;
		SDeque<uint16_t, MAX_RECTS_PER_WINDOW> free_ids;
		SArray<RectData, MAX_RECTS_PER_WINDOW> data;

		// 400 kb
		SArray<GuiQuadVertex, MAX_RECTS_PER_WINDOW> solidVertex;
		SArray<GuiQuadIndex, MAX_RECTS_PER_WINDOW> solidIndex;
		SArray<GuiGroups, MAX_RECTS_PER_WINDOW> solidGroups;
		
		// 400 kb
		SArray<GuiQuadVertex, MAX_RECTS_PER_WINDOW> alphaVertex;
		SArray<GuiQuadIndex, MAX_RECTS_PER_WINDOW> alphaIndex;
		SArray<GuiGroups, MAX_RECTS_PER_WINDOW> alphaGroups;
	};

	SArray< RectsPerWin*, MAX_GUI_WINDOWS > rectsPerWindow;
};