#pragma once

#include "HDefines.h"
#include "Material.h"
#include "Common.h"
#include "WindowsMgr.h"

using namespace EngineCore;

#define MAX_RECTS 4096

#define DEPTH_LAYER_MAX 4096

#define MAX_RECTS_PER_WINDOW 2048
#define MAX_ALPHAS_PER_WINDOW 1024

#define MAX_RECT_GROUPS 512
/*
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

class HRectMgr
{
public:
	HRectMgr();
	~HRectMgr();

	uint16_t AddRect(string& shader);
	void DeleteRects(uint16_t id);

	RectData* GetRect(uint16_t id);
	bool SetRect(uint16_t id, MLRECT& rect);

	inline static HRectMgr* Get() {return instance;}
	
private:
	static HRectMgr* instance;
	
	SArray<uint16_t, MAX_RECTS> ids;
	SDeque<uint16_t, MAX_RECTS> free_ids;
	SArray<RectData, MAX_RECTS> data;
};
*/
//////////////////////////////////

struct GuiGroups
{
	uint16_t begin;
	uint16_t end;
};

struct RectInstData
{
	Vector4 rect;
	Vector4 depth;

	RectInstData(): rect(0,0,0,0), depth(0,0,0,0) {}
	RectInstData& operator=(const RectInstData& r)
	{
		rect = r.rect;
		depth = r.depth;
		return *this;
	}
};

class GuiRenderMgr
{
public:
	GuiRenderMgr();
	~GuiRenderMgr();

	void RegRect(RectData& rect);
	void RegText(RectData& rect);

	void Draw();

	inline static GuiRenderMgr* Get() {return instance;}

	static bool CompareRects(RectData& a, RectData& b);
	static void SwapRects(RectData* first, RectData* second, SArray<uint16_t, MAX_RECTS_PER_WINDOW>* idsArr, 
		SArray<RectInstData, MAX_RECTS_PER_WINDOW>* gpuArr);

private:
	static GuiRenderMgr* instance;

	void allocWindow(int16_t winId);
	void updateGPUData(Window* win, int16_t winId, uint16_t arrId);
	void sortRects();

	struct GuiPerWin
	{
		SArray<RectInstData, MAX_RECTS_PER_WINDOW> solidData;
		SArray<MatData, MAX_RECTS_PER_WINDOW> solidMats;
		SArray<RectExtraData, MAX_RECTS_PER_WINDOW> solidExtra;
		SArray<GuiGroups, MAX_RECT_GROUPS> solidGroups;

		SArray<RectTextData, MAX_ALPHAS_PER_WINDOW> alphaData;
		SArray<MatData, MAX_ALPHAS_PER_WINDOW> alphaMats;
	};

	SArray< GuiPerWin*, MAX_GUI_WINDOWS > guiPerWindow;
};