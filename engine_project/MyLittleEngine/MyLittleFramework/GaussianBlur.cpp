#include "stdafx.h"
#include "GaussianBlur.h"

using namespace EngineCore;

GaussianBlur::GaussianBlur()
{
	m_render = Render::Get();

	horzPass = nullptr;
	sp_horz = nullptr;
	sp_vert = nullptr;

	isDepthDependent = false;

	blurMips = GB_ONLYMIP0;
}

GaussianBlur::~GaussianBlur()
{
	Close();
}

void GaussianBlur::Close()
{
	_CLOSE(horzPass);
	_DELETE(sp_horz);
	_DELETE(sp_vert);
}

bool GaussianBlur::Init(int width, int height, DXGI_FORMAT format, eKernel kernel, eBlurWhat mipsBlur, bool depthDependent)
{
	horzPass = new RenderTarget;
	blurMips = mipsBlur;

	isDepthDependent = depthDependent;

	int t_width, t_height;

	switch (mipsBlur)
	{
	case GB_ONLYMIP0:
		if(!horzPass->Init(width, height))return false;
		if(!horzPass->AddRT(format, 1, false, true))return false; 
		t_width = width;
		t_height = height;
		break;
	case GB_ALLMIPS:
		if(isDepthDependent)
		{
			ERR("Depth-dependent Gaussian Blur is allowed only for GB_ONLYMIP0");
			isDepthDependent = false;
		}
		if(!horzPass->Init(width, height))return false;
		if(!horzPass->AddRT(format, 0, false, true))return false; 
		t_width = width;
		t_height = height;
		break;
	case GB_ALLMIPSBUT0:
		if(isDepthDependent)
		{
			ERR("Depth-dependent Gaussian Blur is allowed only for GB_ONLYMIP0");
			isDepthDependent = false;
		}
		t_width = width/2;
		t_height = height/2;
		if(!horzPass->Init(t_width, t_height))return false;
		if(!horzPass->AddRT(format, 0, false, true))return false; 
		break;
	}

	char* mat_h = nullptr;
	char* mat_v = nullptr;

	if(isDepthDependent)
	{
		switch (kernel)
		{
		case GB_KERNEL3:
			mat_h = GAUSSIAN_KERNEL_3_h_dd;
			mat_v = GAUSSIAN_KERNEL_3_v_dd;
			break;
		case GB_KERNEL5:
			mat_h = GAUSSIAN_KERNEL_5_h_dd;
			mat_v = GAUSSIAN_KERNEL_5_v_dd;
			break;
		case GB_KERNEL7:
			mat_h = GAUSSIAN_KERNEL_7_h_dd;
			mat_v = GAUSSIAN_KERNEL_7_v_dd;
			break;
		case GB_KERNEL9:
			mat_h = GAUSSIAN_KERNEL_9_h_dd;
			mat_v = GAUSSIAN_KERNEL_9_v_dd;
			break;
		case GB_KERNEL15:
			mat_h = GAUSSIAN_KERNEL_15_h_dd;
			mat_v = GAUSSIAN_KERNEL_15_v_dd;
			break;
		case GB_KERNEL21:
			ERR("GB_KERNEL15 is max for depth-dependent blur");
			mat_h = GAUSSIAN_KERNEL_15_h_dd;
			mat_v = GAUSSIAN_KERNEL_15_v_dd;
			break;
		case GB_KERNEL31:
			ERR("GB_KERNEL15 is max for depth-dependent blur");
			mat_h = GAUSSIAN_KERNEL_15_h_dd;
			mat_v = GAUSSIAN_KERNEL_15_v_dd;
			break;
		}
	}
	else
	{
		switch (kernel)
		{
		case GB_KERNEL3:
			mat_h = GAUSSIAN_KERNEL_3_h;
			mat_v = GAUSSIAN_KERNEL_3_v;
			break;
		case GB_KERNEL5:
			mat_h = GAUSSIAN_KERNEL_5_h;
			mat_v = GAUSSIAN_KERNEL_5_v;
			break;
		case GB_KERNEL7:
			mat_h = GAUSSIAN_KERNEL_7_h;
			mat_v = GAUSSIAN_KERNEL_7_v;
			break;
		case GB_KERNEL9:
			mat_h = GAUSSIAN_KERNEL_9_h;
			mat_v = GAUSSIAN_KERNEL_9_v;
			break;
		case GB_KERNEL15:
			mat_h = GAUSSIAN_KERNEL_15_h;
			mat_v = GAUSSIAN_KERNEL_15_v;
			break;
		case GB_KERNEL21:
			mat_h = GAUSSIAN_KERNEL_21_h;
			mat_v = GAUSSIAN_KERNEL_21_v;
			break;
		case GB_KERNEL31:
			mat_h = GAUSSIAN_KERNEL_31_h;
			mat_v = GAUSSIAN_KERNEL_31_v;
			break;
		}
	}
	
	sp_horz = new ScreenPlane(t_width, t_height, mat_h);
	if(!sp_horz)
		return false;
	sp_vert = new ScreenPlane(t_width, t_height, mat_v);
	if(!sp_vert)
		return false;

	return true;
}

void GaussianBlur::blur(RenderTarget* inout, uint8_t id, ID3D11ShaderResourceView* depth)
{
	if(!inout)return;

	horzPass->ClearRenderTargets();

	switch (blurMips)
	{
	case GB_ONLYMIP0:
		{
			float rcpW = 1.0f/float(inout->t_width);
			float rcpH = 1.0f/float(inout->t_height);

			horzPass->SetRenderTarget(0);
			
			sp_horz->SetTexture(inout->GetShaderResourceView(id), 0);
			if(isDepthDependent)
				sp_horz->SetTexture(depth, 1);
			sp_horz->SetFloat(rcpW, 0);
			sp_horz->SetFloat(rcpH, 1);
			sp_horz->Draw();

			inout->SetRenderTarget(id);
			
			sp_vert->SetTexture(horzPass->GetShaderResourceView(0), 0);
			if(isDepthDependent)
				sp_horz->SetTexture(depth, 1);
			sp_vert->SetFloat(rcpW, 0);
			sp_vert->SetFloat(rcpH, 1);
			sp_vert->Draw();
		}
		break;

	case GB_ALLMIPS:
		{
			ID3D11RenderTargetView* r_target;

			float rcpW = 1.0f/float(inout->t_width);
			float rcpH = 1.0f/float(inout->t_height);

			horzPass->SetRenderTarget(0);
			
			sp_horz->SetTexture(inout->GetShaderResourceView(id), 0);
			sp_horz->SetFloat(rcpW, 0);
			sp_horz->SetFloat(rcpH, 1);
			sp_horz->Draw();

			inout->SetRenderTarget(id);
			
			sp_vert->SetTexture(horzPass->GetShaderResourceView(0), 0);
			sp_vert->SetFloat(rcpW, 0);
			sp_vert->SetFloat(rcpH, 1);
			sp_vert->Draw();

			float t_width = horzPass->m_viewport.Width;
			float t_height = horzPass->m_viewport.Height;

			for(int i=0; i<inout->mipRes[id].mipCount-1; i++)
			{
				float rcpMW = 1.0f/float(inout->mip_res[i].x);
				float rcpMH = 1.0f/float(inout->mip_res[i].y);

				horzPass->m_viewport.Width = float(horzPass->mip_res[i].x);
				horzPass->m_viewport.Height = float(horzPass->mip_res[i].y);
				m_render->m_pImmediateContext->RSSetViewports(1, &horzPass->m_viewport);

				r_target = horzPass->mipRes[0].mip_RTV[i];
				m_render->m_pImmediateContext->OMSetRenderTargets(1, &r_target, nullptr);

				sp_horz->SetTexture(inout->mipRes[id].mip_SRV[i+1], 0);
				sp_horz->SetFloat(rcpMW, 0);
				sp_horz->SetFloat(rcpMH, 1);
				sp_horz->Draw();

				r_target = inout->mipRes[id].mip_RTV[i];
				m_render->m_pImmediateContext->OMSetRenderTargets(1, &r_target, nullptr);

				sp_vert->SetTexture(horzPass->mipRes[0].mip_SRV[i+1], 0);
				sp_vert->SetFloat(rcpMW, 0);
				sp_vert->SetFloat(rcpMH, 1);
				sp_vert->Draw();
			}

			horzPass->m_viewport.Width = t_width;
			horzPass->m_viewport.Height = t_height;
		}
		break;

	case GB_ALLMIPSBUT0:
		{
			ID3D11RenderTargetView* r_target;

			float rcpW = 2.0f/float(inout->t_width);
			float rcpH = 2.0f/float(inout->t_height);

			horzPass->SetRenderTarget(0);
			
			sp_horz->SetTexture(inout->mipRes[id].mip_SRV[1], 0);
			sp_horz->SetFloat(rcpW, 0);
			sp_horz->SetFloat(rcpH, 1);
			sp_horz->Draw();

			m_render->m_pImmediateContext->RSSetViewports(1, &horzPass->m_viewport);

			r_target = inout->mipRes[id].mip_RTV[0];
			m_render->m_pImmediateContext->OMSetRenderTargets(1, &r_target, nullptr);
			
			sp_vert->SetTexture(horzPass->GetShaderResourceView(0), 0);
			sp_vert->SetFloat(rcpW, 0);
			sp_vert->SetFloat(rcpH, 1);
			sp_vert->Draw();

			float t_width = horzPass->m_viewport.Width;
			float t_height = horzPass->m_viewport.Height;

			for(int i=1; i<inout->mipRes[id].mipCount-1; i++)
			{
				float rcpMW = 1.0f/float(inout->mip_res[i].x);
				float rcpMH = 1.0f/float(inout->mip_res[i].y);

				horzPass->m_viewport.Width = float(horzPass->mip_res[i-1].x);
				horzPass->m_viewport.Height = float(horzPass->mip_res[i-1].y);
				m_render->m_pImmediateContext->RSSetViewports(1, &horzPass->m_viewport);

				r_target = horzPass->mipRes[0].mip_RTV[i-1];
				m_render->m_pImmediateContext->OMSetRenderTargets(1, &r_target, nullptr);

				sp_horz->SetTexture(inout->mipRes[id].mip_SRV[i+1], 0);
				sp_horz->SetFloat(rcpMW, 0);
				sp_horz->SetFloat(rcpMH, 1);
				sp_horz->Draw();

				r_target = inout->mipRes[id].mip_RTV[i];
				m_render->m_pImmediateContext->OMSetRenderTargets(1, &r_target, nullptr);

				sp_vert->SetTexture(horzPass->mipRes[0].mip_SRV[i], 0);
				sp_vert->SetFloat(rcpMW, 0);
				sp_vert->SetFloat(rcpMH, 1);
				sp_vert->Draw();
			}

			horzPass->m_viewport.Width = t_width;
			horzPass->m_viewport.Height = t_height;
		}
		break;
	}
}
	
void GaussianBlur::blur(RenderTarget* in, RenderTarget* out, int in_id, int out_id, ID3D11ShaderResourceView* depth)
{
	if(!in || !out)return;

	horzPass->ClearRenderTargets();

	switch (blurMips)
	{
	case GB_ONLYMIP0:
		{
			float rcpW = 1.0f/float(out->t_width);
			float rcpH = 1.0f/float(out->t_height);

			horzPass->SetRenderTarget(0);
			
			sp_horz->SetTexture(in->GetShaderResourceView(in_id), 0);
			if(isDepthDependent)
				sp_horz->SetTexture(depth, 1);
			sp_horz->SetFloat(rcpW, 0);
			sp_horz->SetFloat(rcpH, 1);
			sp_horz->Draw();

			out->SetRenderTarget(out_id);
			
			sp_vert->SetTexture(horzPass->GetShaderResourceView(0), 0);
			if(isDepthDependent)
				sp_horz->SetTexture(depth, 1);
			sp_vert->SetFloat(rcpW, 0);
			sp_vert->SetFloat(rcpH, 1);
			sp_vert->Draw();
		}
		break;

	case GB_ALLMIPS:
		{
			if(in->mip_count != out->mip_count)
			{
				ERR("Количество мп-уровней не совпадает в in и out");
			}

			ID3D11RenderTargetView* r_target;

			float rcpW = 1.0f/float(in->t_width);
			float rcpH = 1.0f/float(in->t_height);

			horzPass->SetRenderTarget(0);
			
			sp_horz->SetTexture(in->GetShaderResourceView(in_id), 0);
			sp_horz->SetFloat(rcpW, 0);
			sp_horz->SetFloat(rcpH, 1);
			sp_horz->Draw();

			out->SetRenderTarget(out_id);
			
			sp_vert->SetTexture(horzPass->GetShaderResourceView(0), 0);
			sp_vert->SetFloat(rcpW, 0);
			sp_vert->SetFloat(rcpH, 1);
			sp_vert->Draw();

			float t_width = horzPass->m_viewport.Width;
			float t_height = horzPass->m_viewport.Height;

			for(int i=0; i<in->mipRes[in_id].mipCount-1; i++)
			{
				float rcpMW = 1.0f/float(in->mip_res[i].x);
				float rcpMH = 1.0f/float(in->mip_res[i].y);

				horzPass->m_viewport.Width = float(horzPass->mip_res[i].x);
				horzPass->m_viewport.Height = float(horzPass->mip_res[i].y);
				m_render->m_pImmediateContext->RSSetViewports(1, &horzPass->m_viewport);

				r_target = horzPass->mipRes[0].mip_RTV[i];
				m_render->m_pImmediateContext->OMSetRenderTargets(1, &r_target, nullptr);

				sp_horz->SetTexture(in->mipRes[in_id].mip_SRV[i+1], 0);
				sp_horz->SetFloat(rcpMW, 0);
				sp_horz->SetFloat(rcpMH, 1);
				sp_horz->Draw();

				r_target = out->mipRes[out_id].mip_RTV[i];
				m_render->m_pImmediateContext->OMSetRenderTargets(1, &r_target, nullptr);

				sp_vert->SetTexture(horzPass->mipRes[0].mip_SRV[i+1], 0);
				sp_vert->SetFloat(rcpMW, 0);
				sp_vert->SetFloat(rcpMH, 1);
				sp_vert->Draw();
			}

			horzPass->m_viewport.Width = t_width;
			horzPass->m_viewport.Height = t_height;
		}
		break;

	case GB_ALLMIPSBUT0:
		ERR("Режим GB_ALLMIPSBUT0 не поддерживаеться");
		break;
	}
}