#include "stdafx.h"
#include "RenderTarget.h"
#include "ScenePipeline.h"

using namespace EngineCore;

RenderTarget::RenderTarget()
{
	for(int i=0; i<MAX_RENDERTARGETS; i++)
	{
		m_RTTexture[i] = nullptr;
		m_RTV[i] = nullptr;
		m_SRV[i] = nullptr;
		m_UAV[i] = nullptr;
		b_mips[i] = false;
	}
	m_DSTexture = nullptr;
	m_DSV = nullptr;
	t_width = 0;
	t_height = 0;
	RT_count = 0;
	
	msaa = false;
	m_msaa_count = 1;
	m_msaa_quality = 0;

	mip_mat = nullptr;
	mip_count = 0;
	plane = nullptr;
	mip_res = nullptr;

	b_importedDS = false;
}

bool RenderTarget::Init(int width, int height, DXGI_FORMAT depthStencil, int msaa_count, int msaa_quality)
{
	b_importedDS = false;

	t_width = width;
	t_height = height;
	
	if(msaa_count > 1 || msaa_quality > 0)
	{
		msaa = true;
		m_msaa_count = msaa_count;
		m_msaa_quality = msaa_quality;
	}

	if(depthStencil != DXGI_FORMAT_UNKNOWN)
	{
		D3D11_TEXTURE2D_DESC depthBufferDesc;
		ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));
		depthBufferDesc.Width = t_width;
		depthBufferDesc.Height = t_height;
		depthBufferDesc.MipLevels = 1;
		depthBufferDesc.ArraySize = 1;
		depthBufferDesc.Format = depthStencil;
		depthBufferDesc.SampleDesc.Count = m_msaa_count;
		depthBufferDesc.SampleDesc.Quality = m_msaa_quality;
		depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthBufferDesc.CPUAccessFlags = 0;
		depthBufferDesc.MiscFlags = 0;
		if( FAILED(Render::CreateTexture2D(&depthBufferDesc, NULL, &m_DSTexture)) )
			return false;

		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
		ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
		depthStencilViewDesc.Format = depthBufferDesc.Format;
		depthStencilViewDesc.ViewDimension = msaa ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;
		if( FAILED(Render::CreateDepthStencilView(m_DSTexture, &depthStencilViewDesc, &m_DSV)) )
			return false;
	}

	initVP();

	return true;
}

bool RenderTarget::Init(int width, int height, ID3D11DepthStencilView* depthStencilView, int msaa_count, int msaa_quality)
{
	b_importedDS = true;

	t_width = width;
	t_height = height;
	
	if(msaa_count > 1 || msaa_quality > 0)
	{
		msaa = true;
		m_msaa_count = msaa_count;
		m_msaa_quality = msaa_quality;
	}

	m_DSV = depthStencilView;
	
	initVP();
	
	return true;
}

bool RenderTarget::Init(RenderTarget* importDepthStencil)
{
	b_importedDS = true;

	t_width = importDepthStencil->t_width;
	t_height = importDepthStencil->t_height;
	
	if(importDepthStencil->m_msaa_count > 0)
	{
		msaa = true;
		m_msaa_count = importDepthStencil->m_msaa_count;
		m_msaa_quality = importDepthStencil->m_msaa_quality;
	}

	m_DSV = importDepthStencil->m_DSV;
	
	initVP();
	
	return true;
}

void RenderTarget::initVP()
{
	m_viewport.Width = (float)t_width;
	m_viewport.Height = (float)t_height;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
}

bool RenderTarget::AddRT(DXGI_FORMAT RT_format, unsigned short mips, bool UAV, bool forceMipRes)
{
	if(RT_count >= MAX_RENDERTARGETS)
	{
		ERR("Достигнуто максимальное количество рендер таргетов: %i", MAX_RENDERTARGETS);
		return false;
	}

	b_mips[RT_count] = mips;

	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = t_width;
	textureDesc.Height = t_height;
	textureDesc.MipLevels = mips;
	textureDesc.ArraySize = 1;
	textureDesc.Format = RT_format;
	textureDesc.SampleDesc.Count = m_msaa_count;
	textureDesc.SampleDesc.Quality = m_msaa_quality;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	if(!UAV)
		textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	else
		textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = (mips != 1) ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;
	if( FAILED(Render::CreateTexture2D(&textureDesc, NULL, &m_RTTexture[RT_count])) )
		return false;
	
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = msaa ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;
	if( FAILED(Render::CreateRenderTargetView(m_RTTexture[RT_count], &renderTargetViewDesc, &m_RTV[RT_count])) )
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = msaa ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = (mips == 0) ? -1 : mips;	
	if( FAILED(Render::CreateShaderResourceView(m_RTTexture[RT_count], &shaderResourceViewDesc, &m_SRV[RT_count])) )
		return false;

	if(UAV)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVdesc;
        ZeroMemory( &UAVdesc, sizeof(UAVdesc));
        UAVdesc.Format=RT_format;
        UAVdesc.ViewDimension=D3D11_UAV_DIMENSION_TEXTURE2D;
        UAVdesc.Texture2D.MipSlice=0;
		if( FAILED(Render::CreateUnorderedAccessView(m_RTTexture[RT_count], &UAVdesc, &m_UAV[RT_count])) )
			return false;
	}
	
	if(mips != 1)
	{
		if(mip_count == 0)
		{
			int res = max(t_width, t_height);
			mip_count = 1;
			while(res > 1)
			{
				res = res / 2;
				mip_count++;
			}
			if(!mip_res)
			{
				mip_res = new POINT[mip_count-1];
				for(int i=1; i<mip_count; i++)
				{
					mip_res[i-1].x = max(int(t_width / pow(2, i)), 1);
					mip_res[i-1].y = max(int(t_height / pow(2, i)), 1);
				}
			}
		}

		if(mip_mat || forceMipRes)
		{
			mipRes[RT_count].mipCount = 0;

			if(mips == 0)
				mipRes[RT_count].mipCount = mip_count;
			else
				mipRes[RT_count].mipCount = mips;

			if(mipRes[RT_count].mipCount > 1)
			{
				mipRes[RT_count].mip_RTV = new ID3D11RenderTargetView*[mipRes[RT_count].mipCount-1];
				mipRes[RT_count].mip_SRV = new ID3D11ShaderResourceView*[mipRes[RT_count].mipCount];

				if(!plane && mip_mat)
					plane = new ScreenPlane(*mip_mat);
				
				shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
				shaderResourceViewDesc.Texture2D.MipLevels = 1;	
				if( FAILED(Render::CreateShaderResourceView(m_RTTexture[RT_count], &shaderResourceViewDesc, &mipRes[RT_count].mip_SRV[0])) )
					return false;

				for(unsigned short i=1; i<mipRes[RT_count].mipCount; i++)
				{
					renderTargetViewDesc.Texture2D.MipSlice = i;
					if( FAILED(Render::CreateRenderTargetView(m_RTTexture[RT_count], &renderTargetViewDesc, &mipRes[RT_count].mip_RTV[i-1])) )
						return false;

					ID3D11Resource* Mip_Res;
					mipRes[RT_count].mip_RTV[i-1]->GetResource(&Mip_Res);

					shaderResourceViewDesc.Texture2D.MostDetailedMip = i;
					shaderResourceViewDesc.Texture2D.MipLevels = 1;	
					if( FAILED(Render::CreateShaderResourceView(Mip_Res, &shaderResourceViewDesc, &mipRes[RT_count].mip_SRV[i])) )
						return false;

					_RELEASE(Mip_Res);
				}
			}
		}
	}

	RT_count++;

	return true;
}

bool RenderTarget::AddBackBufferRT(ID3D11Texture2D* p_BB)
{
	HRESULT hr = Render::CreateRenderTargetView( p_BB, nullptr, &m_RTV[RT_count] );
	
	if( FAILED(hr) )
		return false;

	RT_count++;
	
	return true;
}

void RenderTarget::Close()
{
	if(!b_importedDS)
	{
		_RELEASE(m_DSV);
		_RELEASE(m_DSTexture);
	}
	else
	{
		m_DSV = nullptr;
	}

	for(int i=0; i<MAX_RENDERTARGETS; i++)
	{
		_RELEASE(m_SRV[i]);
		_RELEASE(m_RTV[i]);
		_RELEASE(m_UAV[i]);
		_RELEASE(m_RTTexture[i]);

		if(mipRes[i].mip_SRV)
			_RELEASE(mipRes[i].mip_SRV[0]);
		for(int j=0; j<mipRes[i].mipCount-1; j++)
			_RELEASE(mipRes[i].mip_RTV[j]);
		for(int j=0; j<mipRes[i].mipCount; j++)
			_RELEASE(mipRes[i].mip_SRV[j]);
		_DELETE(mipRes[i].mip_RTV);
		_DELETE(mipRes[i].mip_SRV);
		_DELETE(plane);
	}
	_DELETE(mip_mat);
	_DELETE_ARRAY(mip_res);
}

void RenderTarget::GenerateMipmaps(ScenePipeline* scene)
{
	for(unsigned int i=0; i<RT_count; i++)
	{
		if(b_mips[i] != 1)
		{
			if(!mip_mat)
			{
				Render::GenerateMips(m_SRV[i]);
			}
			else
			{
				int prevX = t_width;
				int prevY = t_height;
				for(int j=0; j<mipRes[i].mipCount-1; j++)
				{
					m_viewport.Width = float(mip_res[j].x);
					m_viewport.Height = float(mip_res[j].y);
					Render::RSSetViewports(1, &m_viewport);

					Render::OMSetRenderTargets(1, &(mipRes[i].mip_RTV[j]), nullptr);
					plane->ClearTex();
					plane->SetTexture(mipRes[i].mip_SRV[j], 0);
					plane->SetVector(XMFLOAT4(1.0f/float(mip_res[j].x), 1.0f/float(mip_res[j].y), float(prevX%2==0), float(prevY%2==0)), 0);
					plane->Draw();
					prevX = mip_res[j].x;
					prevY = mip_res[j].y;
				}
				m_viewport.Width = float(t_width);
				m_viewport.Height = float(t_height);
			}
		}
	}
}

void RenderTarget::SetRenderTarget(UINT rt_start, UINT rt_end)
{
	UINT count = (rt_end == 0 ? RT_count : rt_end) - rt_start;
	Render::OMSetRenderTargets(count, &(m_RTV[rt_start]), m_DSV);
	Render::RSSetViewports(1, &m_viewport);
}

void RenderTarget::ClearRenderTargets(float red, float green, float blue, float alpha, bool clearDS)
{
	float color[4] = {red, green, blue, alpha};
	for(int i=0; i<(int)RT_count; i++)
		Render::ClearRenderTargetView(m_RTV[i], color);
	
	if(clearDS && m_DSV) Render::ClearDepthStencilView(m_DSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

ID3D11ShaderResourceView* RenderTarget::GetShaderResourceView(int id)
{
	if(id>=int(RT_count) || id<0)
		return nullptr;
	return m_SRV[id];
}

ID3D11UnorderedAccessView* RenderTarget::GetUnorderedAccessView(int id)
{
	if(id>=int(RT_count) || id<0)
		return nullptr;
	return m_UAV[id];
}

ID3D11RenderTargetView* RenderTarget::GetRenderTargetView(int id)
{
	if(id>=int(RT_count) || id<0)
		return nullptr;
	return m_RTV[id];
}

ID3D11DepthStencilView* RenderTarget::GetDepthStencilView()
{
	return m_DSV;
}

ID3D11Texture2D* RenderTarget::GetTexture(int id)
{
	if(id>=int(RT_count) || id<0)
		return nullptr;
	return m_RTTexture[id];
}