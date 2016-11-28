#pragma once

#include "HDefines.h"
#include "Material.h"
#include "Common.h"

using namespace EngineCore;

#define MAX_RECTS_PER_WINDOW 4096

struct RectData
{
	XMFLOAT4 rect;
	SimpleShaderInst* shaderInst;

	RectData()
	{
		rect = XMFLOAT4(0,0,0,0);
		shaderInst = nullptr;
	}
};

class HRectMgr
{
public:
	HRectMgr();
	~HRectMgr();

	uint16_t AddRect(HWND hwnd);
	void DeleteRect(uint16_t id);

	inline static HRectMgr* Get() {return instance;}

private:
	static HRectMgr* instance;

	unordered_map<HWND, uint16_t> winIDs;
	DArray< SArray<RectData, MAX_RECTS_PER_WINDOW> > rectsPerWindow;
};