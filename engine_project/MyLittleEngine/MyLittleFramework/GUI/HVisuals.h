#pragma once

#include "HDefines.h"
#include "Material.h"
#include "Common.h"
#include "WindowsMgr.h"

using namespace EngineCore;

#define DEPTH_LAYER_MAX 4096

#define MAX_RECTS_PER_WINDOW 4096
#define MAX_RECT_INSTANCE 1024

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
	ScreenRect& operator=(const MLRECT& r)
	{
		l = r.left;
		t = r.top;
		w = r.width;
		h = r.height;
		return *this;
	}
};

struct RectData
{
	ScreenRect rect;
	uint16_t depth;
	SimpleShaderInst* shaderInst;
	uint16_t id;
	uint16_t arrId;
	uint32_t rectHash;

	RectData(): depth(0), shaderInst(nullptr), id(MAX_RECTS_PER_WINDOW), 
		arrId(MAX_RECTS_PER_WINDOW), rectHash(0) {}
	RectData& operator=(const RectData& r)
	{
		rect = r.rect;
		depth = r.depth;
		shaderInst = r.shaderInst;
		id = r.id;
		arrId = r.arrId;
		rectHash = r.rectHash;
		return *this;
	}
};

struct RectInstData
{
	XMFLOAT4 rect;
	XMFLOAT4 depth;

	RectInstData(): rect(0,0,0,0), depth(0,0,0,0) {}
	RectInstData& operator=(const RectInstData& r)
	{
		rect = r.rect;
		depth = r.depth;
		return *this;
	}
};

struct GuiGroups
{
	uint16_t begin;
	uint16_t end;
};

class HRectMgr
{
public:
	HRectMgr();
	~HRectMgr();

	RectData* AddRect(int16_t winId, SimpleShaderInst* shInst);
	void DeleteRects(int16_t winId, uint16_t id);

	RectData* GetRect(int16_t winId, uint16_t id);
	bool SetRect(int16_t winId, uint16_t id, MLRECT& rect);

	inline static HRectMgr* Get() {return instance;}

	static bool HRectMgr::CompareRects(RectData& a, RectData& b);
	static void HRectMgr::SwapRects(RectData* first, RectData* second, SArray<uint16_t, MAX_RECTS_PER_WINDOW>* idsArr, 
		SArray<RectInstData, MAX_RECTS_PER_WINDOW>* gpuArr);

private:
	static HRectMgr* instance;

	void allocWindow(int16_t winId);
	void updateGPUData(Window* win, int16_t winId, uint16_t arrId);
	void sortRects();
	
	struct RectsPerWin
	{
		SArray<uint16_t, MAX_RECTS_PER_WINDOW> ids;
		SDeque<uint16_t, MAX_RECTS_PER_WINDOW> free_ids;
		SArray<RectData, MAX_RECTS_PER_WINDOW> data;
		SArray<RectInstData, MAX_RECTS_PER_WINDOW> dataGPU;

		SArray<GuiGroups, MAX_RECTS_PER_WINDOW> solidGroups;
		SArray<GuiGroups, MAX_RECTS_PER_WINDOW> alphaGroups;
	};

	SArray< RectsPerWin*, MAX_GUI_WINDOWS > rectsPerWindow;
};