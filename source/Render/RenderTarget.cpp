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
		m_currentRTState[i] = RHI::ResourceState::RenderTarget;
	}
	m_DSTexture = nullptr;
	m_DSV = nullptr;
	m_currentDSState = RHI::ResourceState::DepthWrite;
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
		RHI::TextureDesc dsDesc = {};
		dsDesc.dimension = RHI::TextureDimension::Tex2D;
		dsDesc.width = t_width;
		dsDesc.height = t_height;
		dsDesc.depth = 1;
		dsDesc.mipLevels = 1;
		dsDesc.format = depthStencil;
		dsDesc.allowDSV = true;
		dsDesc.allowSRV = false;
		dsDesc.allowRTV = false;
		dsDesc.allowUAV = false;
		dsDesc.generateMips = false;
		dsDesc.msaaSamples = m_msaa_count;
		dsDesc.msaaQuality = m_msaa_quality;
		dsDesc.initialData = nullptr;
		m_DSTexture = GFX_DEVICE->CreateTexture(dsDesc);
		if(!m_DSTexture)
			return false;

		m_DSV = GFX_DEVICE->CreateDSV(m_DSTexture, depthStencil);
		if(!m_DSV)
			return false;
	}

	initVP();

	return true;
}

bool RenderTarget::Init(int width, int height, RHI::GfxDSV* depthStencilView, int msaa_count, int msaa_quality)
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
	m_viewport.width = (float)t_width;
	m_viewport.height = (float)t_height;
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;
	m_viewport.topLeftX = 0.0f;
	m_viewport.topLeftY = 0.0f;
}

bool RenderTarget::AddRT(DXGI_FORMAT RT_format, unsigned short mips, bool UAV, bool forceMipRes)
{
	if(RT_count >= MAX_RENDERTARGETS)
	{
		ERR("���������� ������������ ���������� ������ ��������: %i", MAX_RENDERTARGETS);
		return false;
	}

	b_mips[RT_count] = mips;

	RHI::TextureDesc texDesc = {};
	texDesc.dimension = RHI::TextureDimension::Tex2D;
	texDesc.width = t_width;
	texDesc.height = t_height;
	texDesc.depth = 1;
	texDesc.mipLevels = mips;
	texDesc.format = RT_format;
	texDesc.allowRTV = true;
	texDesc.allowSRV = true;
	texDesc.allowUAV = UAV;
	texDesc.allowDSV = false;
	texDesc.generateMips = (mips != 1);
	texDesc.msaaSamples = m_msaa_count;
	texDesc.msaaQuality = m_msaa_quality;
	texDesc.initialData = nullptr;
	m_RTTexture[RT_count] = GFX_DEVICE->CreateTexture(texDesc);
	if(!m_RTTexture[RT_count])
		return false;

	m_RTV[RT_count] = GFX_DEVICE->CreateRTV(m_RTTexture[RT_count], RT_format);
	if(!m_RTV[RT_count])
		return false;

	m_SRV[RT_count] = GFX_DEVICE->CreateSRV(m_RTTexture[RT_count], RT_format, 0, (mips == 0) ? UINT32_MAX : mips);
	if(!m_SRV[RT_count])
		return false;

	if(UAV)
	{
		m_UAV[RT_count] = GFX_DEVICE->CreateUAV(m_RTTexture[RT_count], RT_format);
		if(!m_UAV[RT_count])
			return false;
	}
	
	if(mips != 1)
	{
		if(mip_count == 0)
		{
			int res = max<int>(t_width, t_height);
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
					mip_res[i-1].x = max<int>(int(t_width / pow(2, i)), 1);
					mip_res[i-1].y = max<int>(int(t_height / pow(2, i)), 1);
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
				mipRes[RT_count].mip_RTV = new RHI::GfxRTV*[mipRes[RT_count].mipCount-1];
				mipRes[RT_count].mip_SRV = new RHI::GfxSRV*[mipRes[RT_count].mipCount];

				if(!plane && mip_mat)
					plane = new ScreenPlane(*mip_mat);

				mipRes[RT_count].mip_SRV[0] = GFX_DEVICE->CreateSRV(m_RTTexture[RT_count], RT_format, 0, 1);
				if(!mipRes[RT_count].mip_SRV[0])
					return false;

				for(unsigned short i=1; i<mipRes[RT_count].mipCount; i++)
				{
					mipRes[RT_count].mip_RTV[i-1] = GFX_DEVICE->CreateRTV(m_RTTexture[RT_count], RT_format, i);
					if(!mipRes[RT_count].mip_RTV[i-1])
						return false;

					mipRes[RT_count].mip_SRV[i] = GFX_DEVICE->CreateSRV(m_RTTexture[RT_count], RT_format, i, 1);
					if(!mipRes[RT_count].mip_SRV[i])
						return false;
				}
			}
		}
	}

	RT_count++;

	return true;
}

bool RenderTarget::AddBackBufferRT(RHI::GfxTexture* p_BB)
{
	m_RTV[RT_count] = GFX_DEVICE->CreateRTV(p_BB, DXGI_FORMAT_UNKNOWN);
	if(!m_RTV[RT_count])
		return false;

	RT_count++;

	return true;
}

void RenderTarget::Close()
{
	if(!b_importedDS)
	{
		if(m_DSV) { GFX_DEVICE->DestroyDSV(m_DSV); m_DSV = nullptr; }
		if(m_DSTexture) { GFX_DEVICE->DestroyTexture(m_DSTexture); m_DSTexture = nullptr; }
	}
	else
	{
		m_DSV = nullptr;
	}

	for(int i=0; i<MAX_RENDERTARGETS; i++)
	{
		if(m_SRV[i]) { GFX_DEVICE->DestroyView(m_SRV[i]); m_SRV[i] = nullptr; }
		if(m_RTV[i]) { GFX_DEVICE->DestroyRTV(m_RTV[i]); m_RTV[i] = nullptr; }
		if(m_UAV[i]) { GFX_DEVICE->DestroyView(m_UAV[i]); m_UAV[i] = nullptr; }
		if(m_RTTexture[i]) { GFX_DEVICE->DestroyTexture(m_RTTexture[i]); m_RTTexture[i] = nullptr; }

		for(int j=0; j<mipRes[i].mipCount-1; j++)
		{
			if(mipRes[i].mip_RTV[j]) { GFX_DEVICE->DestroyRTV(mipRes[i].mip_RTV[j]); mipRes[i].mip_RTV[j] = nullptr; }
		}
		for(int j=0; j<mipRes[i].mipCount; j++)
		{
			if(mipRes[i].mip_SRV[j]) { GFX_DEVICE->DestroyView(mipRes[i].mip_SRV[j]); mipRes[i].mip_SRV[j] = nullptr; }
		}
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
				GFX_CMD->GenerateMips(m_SRV[i]);
			}
			else
			{
				int prevX = t_width;
				int prevY = t_height;
				for(int j=0; j<mipRes[i].mipCount-1; j++)
				{
					m_viewport.width = float(mip_res[j].x);
					m_viewport.height = float(mip_res[j].y);
					GFX_CMD->SetViewport(m_viewport);

					GFX_CMD->SetRenderTargets(1, &(mipRes[i].mip_RTV[j]), nullptr);
					plane->ClearTex();
					plane->SetTexture(mipRes[i].mip_SRV[j], 0);
					plane->SetVector(Vector4(1.0f/float(mip_res[j].x), 1.0f/float(mip_res[j].y), float(prevX%2==0), float(prevY%2==0)), 0);
					plane->Draw();
					prevX = mip_res[j].x;
					prevY = mip_res[j].y;
				}
				m_viewport.width = float(t_width);
				m_viewport.height = float(t_height);
			}
		}
	}
}

void RenderTarget::SetRenderTarget(uint32_t rt_start, uint32_t rt_end)
{
	uint32_t end = (rt_end == 0 ? RT_count : rt_end);

	for(uint32_t i = rt_start; i < end; i++)
	{
		if(m_RTTexture[i] && m_currentRTState[i] != RHI::ResourceState::RenderTarget)
		{
			GFX_CMD->TransitionTexture(m_RTTexture[i], m_currentRTState[i], RHI::ResourceState::RenderTarget);
			m_currentRTState[i] = RHI::ResourceState::RenderTarget;
		}
	}
	if(m_DSTexture && !b_importedDS && m_currentDSState != RHI::ResourceState::DepthWrite)
	{
		GFX_CMD->TransitionTexture(m_DSTexture, m_currentDSState, RHI::ResourceState::DepthWrite);
		m_currentDSState = RHI::ResourceState::DepthWrite;
	}
	GFX_CMD->FlushBarriers();

	uint32_t count = end - rt_start;
	GFX_CMD->SetRenderTargets(count, &m_RTV[rt_start], m_DSV);
	GFX_CMD->SetViewport(m_viewport);
}

void RenderTarget::ClearRenderTargets(float red, float green, float blue, float alpha, bool clearDS)
{
	TransitionToRenderTarget();

	for(int i=0; i<(int)RT_count; i++)
		GFX_CMD->ClearRenderTarget(m_RTV[i], red, green, blue, alpha);

	if(clearDS && m_DSV) GFX_CMD->ClearDepthStencil(m_DSV, 1.0f, 0);
}

RHI::GfxSRV* RenderTarget::GetShaderResourceView(int id)
{
	if(id>=int(RT_count) || id<0)
		return nullptr;
	return m_SRV[id];
}

RHI::GfxUAV* RenderTarget::GetUnorderedAccessView(int id)
{
	if(id>=int(RT_count) || id<0)
		return nullptr;
	return m_UAV[id];
}

RHI::GfxRTV* RenderTarget::GetRenderTargetView(int id)
{
	if(id>=int(RT_count) || id<0)
		return nullptr;
	return m_RTV[id];
}

RHI::GfxDSV* RenderTarget::GetDepthStencilView()
{
	return m_DSV;
}

RHI::GfxTexture* RenderTarget::GetTexture(int id)
{
	if(id>=int(RT_count) || id<0)
		return nullptr;
	return m_RTTexture[id];
}

void RenderTarget::TransitionToRenderTarget()
{
	for(uint32_t i = 0; i < RT_count; i++)
	{
		if(m_RTTexture[i] && m_currentRTState[i] != RHI::ResourceState::RenderTarget)
		{
			GFX_CMD->TransitionTexture(m_RTTexture[i], m_currentRTState[i], RHI::ResourceState::RenderTarget);
			m_currentRTState[i] = RHI::ResourceState::RenderTarget;
		}
	}
	if(m_DSTexture && !b_importedDS && m_currentDSState != RHI::ResourceState::DepthWrite)
	{
		GFX_CMD->TransitionTexture(m_DSTexture, m_currentDSState, RHI::ResourceState::DepthWrite);
		m_currentDSState = RHI::ResourceState::DepthWrite;
	}
	GFX_CMD->FlushBarriers();
}

void RenderTarget::TransitionToShaderResource()
{
	for(uint32_t i = 0; i < RT_count; i++)
	{
		if(m_RTTexture[i] && m_currentRTState[i] != RHI::ResourceState::ShaderResource)
		{
			GFX_CMD->TransitionTexture(m_RTTexture[i], m_currentRTState[i], RHI::ResourceState::ShaderResource);
			m_currentRTState[i] = RHI::ResourceState::ShaderResource;
		}
	}
	GFX_CMD->FlushBarriers();
}

void RenderTarget::TransitionToUnorderedAccess()
{
	for(uint32_t i = 0; i < RT_count; i++)
	{
		if(m_UAV[i] && m_RTTexture[i] && m_currentRTState[i] != RHI::ResourceState::UnorderedAccess)
		{
			GFX_CMD->TransitionTexture(m_RTTexture[i], m_currentRTState[i], RHI::ResourceState::UnorderedAccess);
			m_currentRTState[i] = RHI::ResourceState::UnorderedAccess;
		}
	}
	GFX_CMD->FlushBarriers();
}

void RenderTarget::TransitionDepthToRead()
{
	if(m_DSTexture && !b_importedDS && m_currentDSState != RHI::ResourceState::ShaderResource)
	{
		GFX_CMD->TransitionTexture(m_DSTexture, m_currentDSState, RHI::ResourceState::ShaderResource);
		m_currentDSState = RHI::ResourceState::ShaderResource;
	}
	GFX_CMD->FlushBarriers();
}