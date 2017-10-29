#include "stdafx.h"
#include "Text.h"
#include "macros.h"
#include "Buffer.h"
#include "RenderState.h"
#include "FontMgr.h"

using namespace EngineCore;

Text::Text()
{
	textFont = nullptr;
	vertexBuffer = nullptr;
	indexBuffer = nullptr;
	constantBuffer = nullptr;
	symbolsPos = nullptr;
	symbolsCount = 0;
	numIndex = 0;
	numDrawIndex = 0;
	numVertex = 0;
	is_static = true;
	maxLength = 0;

	dirty = true;
}

bool Text::Init(string &font, wstring &text, string& shaderName, bool statictext, uint16_t length, bool needSymPos)
{
	if(shaderName.size() > 0)
		shaderInst = new SimpleShaderInst(shaderName);
	else
		shaderInst = new SimpleShaderInst(string(FONT_SHADER_DEFAULT));

	if(!shaderInst)
	{
		ERR("Cant get shader for text %ls", text.c_str());
		return false;
	}

	textFont = FontMgr::Get()->GetFont(font);
	if(!textFont)
	{
		ERR("Cant get font for text %ls", text.c_str());
		return false;
	}

	is_static = statictext;
	maxLength = length;
	if(!maxLength || (maxLength<(uint16_t)text.size()) )
		maxLength = (uint16_t)text.size();

	if(needSymPos)
	{
		symbolsPos = new int16_t[maxLength+1];
		ZeroMemory(symbolsPos, sizeof(int16_t) * (maxLength + 1));
	}

	if( !initBuffers(text) )
		return false;

	dirty = true;
	return true;
}

bool Text::initBuffers(wstring &text)
{
	numVertex = maxLength * 4;
	numIndex = maxLength * 6;
	numDrawIndex = (uint16_t)text.size() * 6;
	if (numDrawIndex > numIndex)
		numDrawIndex = numIndex;
		
	UnlitVertex *vertex = new UnlitVertex[numVertex];
	if(!vertex)
		return false;

	unsigned long *indices = new unsigned long[numIndex];
	if(!indices)
	{
		_DELETE_ARRAY(vertex);
		return false;
	}

	textFont->BuildVertexArray(vertex, numVertex, text, symbolsPos, &symbolsCount);
	
	for(int i=0; i<numIndex / 6; i++)
	{
		indices[i*6+0] = i*4+0;
		indices[i*6+1] = i*4+1;
		indices[i*6+2] = i*4+2;
		indices[i*6+3] = i*4+0;
		indices[i*6+4] = i*4+3;
		indices[i*6+5] = i*4+1;
	}

	vertexBuffer = Buffer::CreateVertexBuffer(DEVICE, sizeof(UnlitVertex) * numVertex, !is_static, vertex);
	if (!vertexBuffer)
	{
		_DELETE_ARRAY(vertex);
		_DELETE_ARRAY(indices);
		return false;
	}

	indexBuffer = Buffer::CreateIndexBuffer(DEVICE, sizeof(unsigned long) * numIndex, false, indices);
	if (!indexBuffer)
	{
		_DELETE_ARRAY(vertex);
		_DELETE_ARRAY(indices);
		return false;
	}

	constantBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(SimpleMatrixBuffer), true);
	if (!constantBuffer)
	{
		_DELETE_ARRAY(vertex);
		_DELETE_ARRAY(indices);
		return false;
	}
	
	_DELETE_ARRAY(vertex);
	_DELETE_ARRAY(indices);

	return true;
}

bool Text::updateBuffer(wstring &text)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = CONTEXT->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if(FAILED(result))
		return false;

	UnlitVertex *verticesPtr = (UnlitVertex*)mappedResource.pData;

	textFont->BuildVertexArray(verticesPtr, numVertex, text, symbolsPos, &symbolsCount);

	CONTEXT->Unmap(vertexBuffer, 0);
	return true;
}

void Text::Draw()
{
	if(dirty)
	{
		if(!updateMatrix())
			return;
		dirty = false;
	}

	const unsigned int stride = sizeof(UnlitVertex); 
	const unsigned int offset = 0;
	CONTEXT->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	CONTEXT->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	
	Render::SetTopology(IA_TOPOLOGY::TRISLIST);

	textFont->SetTexture();

	shaderInst->SetMatrixBuffer(constantBuffer);
	shaderInst->Set();

	CONTEXT->DrawIndexed(numDrawIndex, 0, 0);
}

bool Text::updateMatrix()
{
	if(!Render::Get()->CurrentHudWindow)
		return false;

	float invW = 2.0f / RENDER->CurrentHudWindow->GetWidth();
	float invH = 2.0f / RENDER->CurrentHudWindow->GetHeight();

	XMMATRIX objmatrix = XMMatrixScaling(invW, invH, 1.0f);
	objmatrix *= XMMatrixTranslation( - 1.0f + invW * textPos.x, 1.0f - invH * textPos.y, 0 );
	
	SimpleMatrixBuffer cb;
	cb.WVP = XMMatrixTranspose(objmatrix);

	Render::UpdateDynamicResource(constantBuffer, (void*)&cb, sizeof(SimpleMatrixBuffer));

	return true;
}

int16_t Text::GetSymPos(uint16_t s) const 
{
	if(!symbolsPos || symbolsCount == 0 ) return 0;
	if(s > symbolsCount) return std::max<int16_t>(0, symbolsPos[symbolsCount]);
	return std::max<int16_t>(0, symbolsPos[s]);
}

uint16_t Text::GetClosestSym(uint16_t x) 
{
	if( x >= uint16_t(symbolsPos[symbolsCount-1] + symbolsPos[symbolsCount]) / 2 )
		return uint16_t(symbolsCount);
	
	for(uint16_t i = 0; i < symbolsCount; i++)
		if( x < uint16_t(symbolsPos[i] + symbolsPos[i+1]) / 2 )
			return i;
	return 0;
}

void Text::Close()
{
	FontMgr::Get()->DeleteFont(textFont->GetName());
	_RELEASE(vertexBuffer);
	_RELEASE(indexBuffer);
	_RELEASE(constantBuffer);
	_DELETE_ARRAY(symbolsPos);
	_DELETE(shaderInst);
}

bool Text::SetText(wstring &text)
{
	if (is_static)
		return false;
	
	uint16_t idx_size = (uint16_t)text.size() * 6;
	if (idx_size > numIndex)
		return false;

	numDrawIndex = idx_size;
	return updateBuffer(text);
}