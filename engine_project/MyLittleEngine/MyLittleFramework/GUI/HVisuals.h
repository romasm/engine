#pragma once

#include "HDefines.h"
#include "Material.h"
#include "Common.h"

using namespace EngineCore;

#define MAX_RECTS_PER_WINDOW 4096
#define MAX_TEXTS_PER_WINDOW 1024

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

struct TextData
{


	TextData()
	{

	}
};

class HVisuals
{
public:
	HVisuals();
	~HVisuals();

	uint16_t AddRect(uint16_t winId);
	void DeleteRect(uint16_t winId, uint16_t id);

	void GetRect(uint16_t winId, uint16_t id);

	inline static HVisuals* Get() {return instance;}

private:
	static HVisuals* instance;

	DArray< SArray<RectData, MAX_RECTS_PER_WINDOW> > rectsPerWindow;
	DArray< SArray<TextData, MAX_TEXTS_PER_WINDOW> > textsPerWindow;
};