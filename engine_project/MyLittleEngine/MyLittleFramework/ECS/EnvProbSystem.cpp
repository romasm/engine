#include "stdafx.h"
#include "EnvProbSystem.h"
#include "../World.h"
#include "../TexMgr.h"

using namespace EngineCore;

EnvProbSystem::EnvProbSystem(BaseWorld* wrd, uint32_t maxCount)
{
	maxCount = min(maxCount, ENTITY_COUNT);
	components.create(maxCount);
	components.reserve(ENVPROBS_INIT_COUNT); // only dist for now

	world = wrd;

	transformSys = world->GetTransformSystem();
	earlyVisibilitySys = world->GetEarlyVisibilitySystem();

	FrustumMgr* frustumMgr = world->GetFrustumMgr();
	frustums = &frustumMgr->camDataArray;
}

void EnvProbSystem::RegToScene()
{
	for(auto& i: *components.data())
	{
		if(!i.active)
			continue;

		if(i.mips_count <= 1)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			TEXTURE_GETPTR(i.specCube)->GetDesc(&desc);
			i.mips_count = desc.TextureCube.MipLevels;
		}

		if(i.is_distant) // to do: env rotation
		{
			TransformComponent* transfComponent = transformSys->GetComponent(i.get_entity());

			for(auto f: *frustums)
				((SceneRenderMgr*)f->rendermgr)->RegDistEnvProb(TEXTURE_GETPTR(i.specCube), TEXTURE_GETPTR(i.diffCube), i.mips_count, transfComponent->worldMatrix);
			continue;
		}

		// Locals disable for now 
		/*
		EarlyVisibilityComponent* earlyVisComponent = earlyVisibilitySys->GetComponent(i.get_entity());
		
		auto bits = earlyVisComponent->inFrust;	
		if(bits == 0)
			continue;

		TransformComponent* transfComponent = transformSys->GetComponent(i.get_entity());

		if(i.dirty)
		{
			switch(i.type)
			{
			case EP_PARALLAX_BOX:
				{
					i.invTransform = XMMatrixInverse(nullptr, XMMatrixTranspose(transfComponent->worldMatrix));
					XMVECTOR extv = XMLoadFloat3(&earlyVisComponent->worldBox.Extents);
					i.distance = XMVectorGetX(XMVector3Length(extv));
				}
				break;
			case EP_PARALLAX_SPHERE:
				i.distance = earlyVisComponent->worldSphere.Radius;
				break;
			}

			XMVECTOR cubePivot = XMVector3TransformCoord(XMVectorSet(0,0,0,1), transfComponent->worldMatrix);
			XMStoreFloat3(&i.pos, cubePivot);
		}

		for(auto& f: *frustums)
		{
			if(f.rendermgr->IsShadow())
				continue;

			if((bits & f.bit) == f.bit)
			{
				EnvProbBuffer data;
				data.Pos = i.pos;
				data.NumMips = i.mips_count;
				data.Radius = i.distance;
				data.Fade = i.fade;
				data.Type = UINT(i.type);
				data.Offset = i.offset;
				data.textureID = 0;			// TODO
				
				if(i.type == EP_PARALLAX_BOX)
				{
					data.Extend = earlyVisComponent->worldBox.Extents;
					data.InvTransform = i.invTransform;
				}

				((SceneRenderMgr*)f.rendermgr)->RegEnvProb(i.specCube, data);

				bits &= ~f.bit;
				if(bits == 0) break;
			}
		}*/
	}
}

void EnvProbSystem::LoadCubemap(EnvProbComponent* comp)
{
	if(comp->specCube == TEX_NULL)
	{
		string spec_to_load = comp->eptex_name + ENVPROBS_POSTFIX_S;
		comp->specCube = RELOADABLE_TEXTURE(spec_to_load, true);
	}
	
	if(comp->diffCube == TEX_NULL && comp->is_distant)
	{
		string diff_to_load = comp->eptex_name + ENVPROBS_POSTFIX_D;
		comp->diffCube = RELOADABLE_TEXTURE(diff_to_load, true);
	}

	comp->mips_count = 1;
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == components.capacity())	return res;\
	auto& comp = components.getDataByArrayIdx(idx);

bool EnvProbSystem::IsDirty(Entity e)
{
	GET_COMPONENT(false)
	return comp.dirty;
}

bool EnvProbSystem::SetDirty(Entity e)
{
	GET_COMPONENT(false)
	comp.dirty = true;
	return true;
}

bool EnvProbSystem::IsActive(Entity e)
{
	GET_COMPONENT(false)
	return comp.active;
}

bool EnvProbSystem::SetActive(Entity e, bool active)
{
	GET_COMPONENT(false)
	comp.active = active;
	return true;
}

bool EnvProbSystem::Bake(Entity e)
{
	GET_COMPONENT(false)

	HRESULT hr;

	DXGI_FORMAT fmt;
	XMVECTOR env_pos;
	if(comp.is_distant)
	{
		fmt = DXGI_FORMAT_R16G16B16A16_FLOAT;
		env_pos = XMVectorZero();
	}
	else
	{
		fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
		env_pos = XMVector3Transform(XMLoadFloat3(&comp.offset), transformSys->GetTransformW(comp.get_entity()));
	}

	Entity env_cam = world->GetEntityMgr()->CreateEntity();
	
	transformSys->AddComponent(env_cam);
	transformSys->SetPosition(env_cam, env_pos);
	transformSys->SetRotation(env_cam, 0, 0, 0);
	transformSys->SetScale(env_cam, 1, 1, 1);

	auto camSys = world->GetCameraSystem();
	camSys->AddComponent(env_cam);
	camSys->SetAspect(env_cam, 1.0f);
	camSys->SetFar(env_cam, comp.far_clip);
	camSys->SetNear(env_cam, comp.near_clip);
	camSys->SetFov(env_cam, XM_PIDIV2);

	auto env_scene = world->CreateScene(env_cam, comp.resolution, comp.resolution, true);

	env_scene->SetComponents(false, true, true, true); // TODO

	unique_ptr<ScreenPlane> sp(new ScreenPlane(ENVPROBS_MAT));
	
	int mipNum = 1;
	int cur_mip_res = comp.resolution;
	while( cur_mip_res > ENVPROBS_SPEC_MIN )
	{
		cur_mip_res /= 2;
		mipNum++;
	}

	ScratchImage raw_cube[6];

	const XMFLOAT3 m_cam_rot[6] = {
		XMFLOAT3(0, XM_PIDIV2, 0),
		XMFLOAT3(0, -XM_PIDIV2, 0),
		XMFLOAT3(-XM_PIDIV2, 0, 0),
		XMFLOAT3(XM_PIDIV2, 0, 0),
		XMFLOAT3(0, 0, 0),
		XMFLOAT3(0, XM_PI, 0)
	};
	
	for(int i=0; i<6; i++)
	{
		transformSys->SetRotation(env_cam, m_cam_rot[i].x, m_cam_rot[i].y, m_cam_rot[i].z);
		
		if(comp.is_distant /**/ && false)
		{
			// TODO only sky
		}
		else
		{
			world->Snapshot(env_scene);
		}

		RenderTarget* face = new RenderTarget;
		if(!face->Init(comp.resolution, comp.resolution))
		{
			_CLOSE(face);
			for(int i=0; i<6; i++)raw_cube[i].Release();
			world->DeleteScene(env_scene);
			return false;
		}
		if(!face->AddRT(fmt))
		{
			_CLOSE(face);
			for(int i=0; i<6; i++)raw_cube[i].Release();
			world->DeleteScene(env_scene);
			return false;
		}

		env_scene->LinearAndDepthToRT(face, sp.get());

		ID3D11Resource* resource = nullptr;
		face->GetShaderResourceView(0)->GetResource(&resource);

		hr = CaptureTexture(Render::Device(), Render::Context(), resource, raw_cube[i]);
		if ( FAILED(hr) )
		{
			_CLOSE(face);
			for(int i=0; i<6; i++)raw_cube[i].Release();
			world->DeleteScene(env_scene);
			return false;
		}
		_CLOSE(face);
	}
	
	world->DeleteScene(env_scene);
	world->DestroyEntity(env_cam);

	ScratchImage mipgen_cube;
	mipgen_cube.InitializeCube(fmt, comp.resolution, comp.resolution, 1, 1);
	mipgen_cube.FillData(raw_cube, 6);

	ScratchImage mipcube;
	hr = GenerateMipMaps(mipgen_cube.GetImages(), mipgen_cube.GetImageCount(), mipgen_cube.GetMetadata(), TEX_FILTER_DEFAULT | TEX_FILTER_BOX | TEX_FILTER_WRAP | TEX_FILTER_FORCE_NON_WIC, 0, mipcube);
	mipgen_cube.Release();
	if ( FAILED(hr) )
	{
		for(int i=0; i<6; i++)raw_cube[i].Release();
		ERR("Cant bake environment prob!");
		return false;
	}

	ID3D11ShaderResourceView* cubemap_srv = nullptr;
	hr = CreateShaderResourceView(Render::Device(), mipcube.GetImages(), 
		mipcube.GetImageCount(), mipcube.GetMetadata(), &cubemap_srv);
	if ( FAILED(hr) )
	{
		_RELEASE(cubemap_srv);
		mipcube.Release();
		for(int i=0; i<6; i++)raw_cube[i].Release();
		return false;
	}
	
	uint32_t facesCount = 6 * mipNum;

	RArray<ScratchImage> i_faces;
	i_faces.create(facesCount);
	i_faces.resize(facesCount);

	for(int i=0; i<mipNum; i++)
	{
		if(i==0)
		{
			for(int j=0; j<6; j++)
				i_faces[j * mipNum].InitializeFromImage(*raw_cube[j].GetImage(0,0,0));
			continue;
		}

		int mip_res = comp.resolution / int(pow(2, i));
		float roughness = pow(float(i) / float(mipNum - 1), 2);

		RenderTarget* mip_rt = new RenderTarget;
		if(!mip_rt->Init(mip_res, mip_res))
		{
			for(uint32_t i=0; i<facesCount; i++)
				i_faces[i].Release();
			_CLOSE(mip_rt);
			_RELEASE(cubemap_srv);
			mipcube.Release();
			for(int i=0; i<6; i++)raw_cube[i].Release();
			return false;
		}
		for(int j=0; j<6; j++)
		{
			if(!mip_rt->AddRT(fmt))
			{
				for(uint32_t i=0; i<facesCount; i++)
					i_faces[i].Release();
				_CLOSE(mip_rt);
				_RELEASE(cubemap_srv);
				mipcube.Release();
				for(int i=0; i<6; i++)raw_cube[i].Release();
				return false;
			}
		}

		unique_ptr<ScreenPlane> mip_sp(new ScreenPlane(ENVPROBS_MIPS_MAT));
		mip_sp->SetTextureByNameS(TEX_HAMMERSLEY, 0);
		mip_sp->SetTexture(cubemap_srv, 1);
		mip_sp->SetFloat(roughness, 0);
		mip_sp->SetFloat(float(mipNum + ENVPROBS_SPEC_MIPS_OFFSET), 1);
		mip_sp->SetFloat(float(comp.resolution), 2);

		mip_rt->ClearRenderTargets();
		mip_rt->SetRenderTarget();

		mip_sp->Draw();

		for(int j=0; j<6; j++)
		{
			ID3D11Resource* resource = nullptr;
			mip_rt->GetShaderResourceView(j)->GetResource(&resource);

			hr = CaptureTexture(Render::Device(), Render::Context(), resource, i_faces[j * mipNum + i]);
			if ( FAILED(hr) )
			{
				for(uint32_t i=0; i<facesCount; i++) 
					i_faces[i].Release();
				_CLOSE(mip_rt);
				_RELEASE(cubemap_srv);
				mipcube.Release();
				for(int i=0; i<6; i++) raw_cube[i].Release();
				return false;
			}
		}

		_CLOSE(mip_rt);
	}

	_RELEASE(cubemap_srv);

	ScratchImage cube;
	cube.InitializeCube(fmt, comp.resolution, comp.resolution, 1, mipNum);
	cube.FillData(i_faces.data(), facesCount);

	ID3D11ShaderResourceView* cubemap_filtered = nullptr;
	hr = CreateShaderResourceView(Render::Device(), cube.GetImages(), 
		cube.GetImageCount(), cube.GetMetadata(), &cubemap_filtered);
	if ( FAILED(hr) )
	{
		for(uint32_t i=0; i<facesCount; i++)
			i_faces[i].Release();
		_RELEASE(cubemap_filtered);
		mipcube.Release();
		for(int i=0; i<6; i++)raw_cube[i].Release();
		cube.Release();
		return false;
	}

	// diffuse
	if(comp.is_distant)
	{
		RArray<ScratchImage> i_faces_diffuse;
		i_faces_diffuse.create(6);
		i_faces_diffuse.resize(6);

		int diff_res = comp.resolution / ENVPROBS_DIFFUSE_DIV;

		RenderTarget* diff_rt = new RenderTarget;
		if(!diff_rt->Init(diff_res, diff_res))
		{
			for(uint32_t i=0; i<facesCount; i++)
				i_faces[i].Release();
			for(int i=0; i<6; i++)
				i_faces_diffuse[i].Release();
			_CLOSE(diff_rt);
			_RELEASE(cubemap_filtered);
			mipcube.Release();
			for(int i=0; i<6; i++)raw_cube[i].Release();
			cube.Release();
			return false;
		}
		for(int j=0; j<6; j++)
		{
			if(!diff_rt->AddRT(fmt))
			{
				for(uint32_t i=0; i<facesCount; i++)
					i_faces[i].Release();
				for(uint32_t i=0; i<6; i++)
					i_faces_diffuse[i].Release();
				_CLOSE(diff_rt);
				_RELEASE(cubemap_filtered);
				mipcube.Release();
				for(int i=0; i<6; i++)raw_cube[i].Release();
				cube.Release();
				return false;
			}
		}

		unique_ptr<ScreenPlane> diff_sp(new ScreenPlane(ENVPROBS_DIFF_MAT));
		diff_sp->SetTextureByNameS(TEX_HAMMERSLEY, 0);
		diff_sp->SetTexture(cubemap_filtered, 1);

		diff_rt->ClearRenderTargets();
		diff_rt->SetRenderTarget();

		diff_sp->Draw();

		for(int j=0; j<6; j++)
		{
			ID3D11Resource* resource = nullptr;
			diff_rt->GetShaderResourceView(j)->GetResource(&resource);

			hr = CaptureTexture(Render::Device(), Render::Context(), resource, i_faces_diffuse[j]);
			if ( FAILED(hr) )
			{
				for(uint32_t i=0; i<facesCount; i++)
					i_faces[i].Release();
				for(uint32_t i=0; i<6; i++)
					i_faces_diffuse[i].Release();
				_CLOSE(diff_rt);
				_RELEASE(cubemap_filtered);
				mipcube.Release();
				for(int i=0; i<6; i++)raw_cube[i].Release();
				cube.Release();
				return false;
			}
		}

		_CLOSE(diff_rt);

		ScratchImage diff_cube;
		diff_cube.InitializeCube(fmt, diff_res, diff_res, 1, 1);
		diff_cube.FillData(i_faces_diffuse.data(), 6);

		for(int i=0; i<6; i++)
			i_faces_diffuse[i].Release();
		i_faces_diffuse.destroy();

		string env_diff_tex = comp.eptex_name + ENVPROBS_POSTFIX_D;

		hr = SaveToDDSFile( diff_cube.GetImages(), diff_cube.GetImageCount(), diff_cube.GetMetadata(), DDS_FLAGS_NONE, StringToWstring(env_diff_tex).data() );
		if ( FAILED(hr) )
		{
			for(uint32_t i=0; i<facesCount; i++)
				i_faces[i].Release();
			_RELEASE(cubemap_filtered);
			mipcube.Release();
			for(int i=0; i<6; i++)raw_cube[i].Release();
			ERR("Cant save environment prob diffuse file %s !", env_diff_tex.c_str());
			diff_cube.Release();
			cube.Release();
			return false;
		}
	}

	mipcube.Release();
	for(int i=0; i<6; i++)
		raw_cube[i].Release();

	for(uint32_t i=0; i<facesCount; i++)
		i_faces[i].Release();
	i_faces.destroy();

	string env_tex = comp.eptex_name + ENVPROBS_POSTFIX_S;

	hr = SaveToDDSFile( cube.GetImages(), cube.GetImageCount(), cube.GetMetadata(), DDS_FLAGS_NONE, StringToWstring(env_tex).data() );
	if ( FAILED(hr) )
	{
		ERR("Cant save environment prob specular file %s !", env_tex.c_str());
		cube.Release();
		return false;
	}

	cube.Release();

	//LoadCubemap(&comp);
	
	return true;
}