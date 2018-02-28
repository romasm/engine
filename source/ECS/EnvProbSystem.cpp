#include "stdafx.h"
#include "EnvProbSystem.h"
#include "World.h"
#include "TexMgr.h"

using namespace EngineCore;

int32_t EnvProbSystem::epResolutions[EP_QUAL_COUNT] = {256, 128, 32};
DXGI_FORMAT EnvProbSystem::epFormats[EP_QUAL_COUNT] = {DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM};

EnvProbSystem::EnvProbSystem(BaseWorld* wrd, uint32_t maxCount)
{
	maxCount = min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);
	components.reserve(ENVPROBS_INIT_COUNT);

	world = wrd;

	transformSys = world->GetTransformSystem();
	earlyVisibilitySys = world->GetEarlyVisibilitySystem();

	FrustumMgr* frustumMgr = world->GetFrustumMgr();
	frustums = &frustumMgr->camDataArray;
}

EnvProbComponent* EnvProbSystem::AddComponent(Entity e)
{
	EnvProbComponent* comp = components.add(e.index());
	if(!comp)
		return nullptr;

	comp->parent = e;
	comp->dirty = true;
	comp->farClip = 100000.0f;
	comp->nearClip = 1.0f;
	comp->offset = Vector3::Zero;
	comp->quality = EP_STANDART;
	comp->priority = 1;
	comp->type = EP_PARALLAX_NONE;
	comp->probName = "";
	comp->fade = 0.1f;
	comp->mipsCount = 1;
	comp->cachedDistance = 1.0f;
	comp->cachedShape = Vector3::Zero;
	comp->needRebake = true;
	
	comp->probId = TexMgr::nullres;

	/*if(earlyVisibilitySys && earlyVisibilitySys->HasComponent(e))
	{
		earlyVisibilitySys->SetType(e, BT_SPHERE);
		earlyVisibilitySys->SetBSphere(e, BoundingSphere(Vector3(0,0,0), 1.0f));
	}*/

	return comp;
}

void EnvProbSystem::DeleteComponent(Entity e)
{
	auto comp = GetComponent(e);
	if(!comp)
		return;
	TEXTURE_DROP(comp->probId);
	components.remove(e.index());
}

void EnvProbSystem::CopyComponent(Entity src, Entity dest)
{
	auto copyBuffer = world->GetCopyBuffer();

	if( !Serialize(src, copyBuffer) )
		return;

	Deserialize(dest, copyBuffer);
	
	auto comp = GetComponent(dest);
	TEXTURE_DROP(comp->probId);

	comp->probName = "";
	comp->probId = TexMgr::nullres;
	comp->needRebake = true;
}

void EnvProbSystem::RegToScene()
{
	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		bitset<FRUSTUM_MAX_COUNT> bits;
		EarlyVisibilityComponent* earlyVisComponent = nullptr;

		if(earlyVisibilitySys)
			earlyVisComponent = earlyVisibilitySys->GetComponent(i.get_entity());

		if(earlyVisComponent)
		{
			bits = earlyVisComponent->inFrust;	
			if(bits == 0)
				continue;
		}
		
		if(i.dirty)
		{
			auto worldMatrix = transformSys->GetTransform_W(i.get_entity());
			
			Vector3 scale, translation;
			Quaternion rotation;
			worldMatrix.Decompose(scale, rotation, translation);
			worldMatrix = Matrix::CreateFromQuaternion(rotation);
			worldMatrix.Translation(translation);
			
			if(earlyVisComponent)
			{
				switch(i.type)
				{
				case EP_PARALLAX_BOX:
					i.cachedShape = earlyVisComponent->worldBox.Extents;
					i.cachedDistance = i.cachedShape.Length() * 2.0f;
					break;
				case EP_PARALLAX_SPHERE:
				case EP_PARALLAX_NONE:
					i.cachedShape = Vector3(earlyVisComponent->worldSphere.Radius, 0, 0);
					i.cachedDistance = i.cachedShape.x * 2.0f;
					break;
				}
			}
			else
			{
				i.cachedShape = Vector3(max(max(scale.x, scale.y), scale.z), 0, 0);
			}

			i.cachedInvTransform = XMMatrixInverse(nullptr, XMMatrixTranspose(worldMatrix));

			XMVECTOR cubePivot = XMVector3TransformCoord(XMVectorSet(0,0,0,1), worldMatrix);
			XMStoreFloat3(&i.cachedPos, cubePivot);

			i.dirty = false;
		}

		// TODO: update & validation on load
		//if(i.needRebake)
		{
			// Verify texture params
			if( i.probId == TexMgr::nullres || i.probName.empty() )
				continue;

			// TODO: move to texture mgr !!!!!!!!!!!!!!!!!!

			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			const auto texCubeSrv = TEXTURE_GETPTR(i.probId);
			texCubeSrv->GetDesc(&desc);

			if( desc.Format != GetFormat(i.quality) )
				continue;
			
			ID3D11Resource* resource = nullptr;
			texCubeSrv->GetResource(&resource);
			if(!resource)
				continue;

			ID3D11Texture2D *cube = 0;
			resource->QueryInterface<ID3D11Texture2D>(&cube);
			if(!cube)
				continue;

			D3D11_TEXTURE2D_DESC texDesc;
			cube->GetDesc(&texDesc);

			if( texDesc.Width != GetResolution(i.quality) )
				continue;
		}

		// TODO
		//if(i.mipsCount <= 1)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			TEXTURE_GETPTR(i.probId)->GetDesc(&desc);
			i.mipsCount = desc.TextureCube.MipLevels;
		}

		if(!earlyVisComponent)
		{
			EnvProbData epData(i.probId, i.quality, i.cachedPos, i.mipsCount, i.cachedDistance, i.fade, 
				EnvParallaxType::EP_PARALLAX_NONE, i.offset, i.cachedShape, i.cachedInvTransform, ENVPROBS_PRIORITY_ALWAYS);

			for(auto f: *frustums)
			{
				if(f->rendermgr->IsShadow())
					continue;

				((SceneRenderMgr*)f->rendermgr)->RegEnvProb(epData);
			}
			continue;
		}
		
		EnvProbData epData(i.probId, i.quality, i.cachedPos, i.mipsCount, i.cachedDistance, i.fade, 
			i.type, i.offset, i.cachedShape, i.cachedInvTransform, i.priority);

		for(auto f: *frustums)
		{
			if(f->rendermgr->IsShadow())
				continue;

			if((bits & f->bit) == f->bit)
			{
				((SceneRenderMgr*)f->rendermgr)->RegEnvProb(epData);

				bits &= ~f->bit;
				if(bits == 0) break;
			}
		}
	}
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

uint32_t EnvProbSystem::Serialize(Entity e, uint8_t* data)
{
	GET_COMPONENT(0)

	uint8_t* t_data = data;

	uint32_t dummySize = 0;
	StringSerialize(comp.probName, &t_data, &dummySize);

	*(uint8_t*)t_data = (uint8_t)comp.type;
	t_data += sizeof(uint8_t);
	
	*(float*)t_data = (float)comp.fade;
	t_data += sizeof(float);

	*(Vector3*)t_data = (Vector3)comp.offset;
	t_data += sizeof(Vector3);

	*(float*)t_data = (float)comp.nearClip;
	t_data += sizeof(float);

	*(float*)t_data = (float)comp.farClip;
	t_data += sizeof(float);

	*(uint8_t*)t_data = (uint8_t)comp.quality;
	t_data += sizeof(uint8_t);

	*(uint32_t*)t_data = (uint32_t)comp.priority;
	t_data += sizeof(uint32_t);

	return (uint32_t)(t_data - data);
}

uint32_t EnvProbSystem::Deserialize(Entity e, uint8_t* data)
{
	auto comp = AddComponent(e);
	if(!comp)
		return 0; // TODO must return size

	uint8_t* t_data = data;

	comp->probName = StringDeserialize(&t_data);

	comp->type = EnvParallaxType(*(uint8_t*)t_data);
	t_data += sizeof(uint8_t);

	comp->fade = *(float*)t_data;
	t_data += sizeof(float);

	comp->offset = *(Vector3*)t_data;
	t_data += sizeof(Vector3);
	
	comp->nearClip = *(float*)t_data;
	t_data += sizeof(float);

	comp->farClip = *(float*)t_data;
	t_data += sizeof(float);

	comp->quality = EnvProbQuality(*(uint8_t*)t_data);
	t_data += sizeof(uint8_t);

	comp->priority = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);

	comp->probId = RELOADABLE_TEXTURE(GetProbFileName(comp->probName), true);

	return (uint32_t)(t_data - data);
}

uint32_t EnvProbSystem::GetType(Entity e)
{
	GET_COMPONENT(0)
	return (uint32_t)comp.type;
}

bool EnvProbSystem::SetType(Entity e, uint32_t type)
{
	GET_COMPONENT(false)
	comp.type = EnvParallaxType(type);
	UpdateProps(e);
	return true;
}

float EnvProbSystem::GetFade(Entity e)
{
	GET_COMPONENT(0)
	return comp.fade;
}

bool EnvProbSystem::SetFade(Entity e, float fade)
{
	GET_COMPONENT(false)
	comp.fade = fade;
	return true;
}

Vector3 EnvProbSystem::GetOffset(Entity e)
{
	GET_COMPONENT(Vector3::Zero)
	return comp.offset;
}

bool EnvProbSystem::SetOffset(Entity e, Vector3& offset)
{
	GET_COMPONENT(false)
	comp.offset = offset;
	comp.needRebake = true;
	return true;
}

float EnvProbSystem::GetNearClip(Entity e)
{
	GET_COMPONENT(0)
	return comp.nearClip;
}

bool EnvProbSystem::SetNearClip(Entity e, float clip)
{
	GET_COMPONENT(false)
	comp.nearClip = clip;
	comp.needRebake = true;
	return true;
}

float EnvProbSystem::GetFarClip(Entity e)
{
	GET_COMPONENT(0)
	return comp.farClip;
}

bool EnvProbSystem::SetFarClip(Entity e, float clip)
{
	GET_COMPONENT(false)
	comp.farClip = clip;
	comp.needRebake = true;
	return true;
}

uint32_t EnvProbSystem::GetPriority(Entity e)
{
	GET_COMPONENT(0)
	return comp.priority;
}

bool EnvProbSystem::SetPriority(Entity e, uint32_t priority)
{
	GET_COMPONENT(false)
	comp.priority = priority;
	return true;
}

uint32_t EnvProbSystem::GetQuality(Entity e)
{
	GET_COMPONENT(0)
	return (uint32_t)comp.quality;
}

bool EnvProbSystem::SetQuality(Entity e, uint32_t quality)
{
	GET_COMPONENT(false)
	comp.quality = EnvProbQuality(quality);
	comp.needRebake = true;
	return true;
}

void EnvProbSystem::UpdateProps(Entity e)
{
	GET_COMPONENT(void())

	if(!earlyVisibilitySys)
		return;

	EarlyVisibilityComponent* earlyVisComponent = earlyVisibilitySys->GetComponent(e);
	if(!earlyVisComponent)
		return;

	switch (comp.type)
	{
	case EP_PARALLAX_SPHERE:
	case EP_PARALLAX_NONE:
		if( earlyVisComponent->type != BT_SPHERE )
		{
			earlyVisibilitySys->SetType(e, BT_SPHERE);
			earlyVisibilitySys->SetBSphere(e, BoundingSphere(Vector3(0,0,0), comp.cachedDistance));
		}
		break;
	case EP_PARALLAX_BOX:
		if( earlyVisComponent->type != BT_BOX )
		{
			earlyVisibilitySys->SetType(e, BT_BOX);
			earlyVisibilitySys->SetBBox(e, BoundingBox(Vector3(0,0,0), Vector3(comp.cachedDistance, comp.cachedDistance, comp.cachedDistance)));
		}
		break;
	}
}

string EnvProbSystem::GetProbFileName(string& probName) const
{
	return RemoveExtension(world->GetWorldName()) + ENVPROBS_SUBFOLDER + probName + EXT_TEXTURE;
}

bool EnvProbSystem::Bake(Entity e)
{
	GET_COMPONENT(false)

	if(comp.probName.empty())
	{
		comp.probName = RandomString(ENVPROBS_NAME_LENGTH);
	}

	HRESULT hr;

	DXGI_FORMAT fmt = GetFormat(comp.quality);
	int32_t resolution = GetResolution(comp.quality);

	Vector3 env_pos = XMVector3Transform(XMLoadFloat3(&comp.offset), transformSys->GetTransform_WInternal(comp.get_entity()));
	Entity env_cam = world->GetEntityMgr()->CreateEntity();
	
	transformSys->AddComponent(env_cam);
	transformSys->SetPosition_L(env_cam, env_pos);
	transformSys->SetRotationPYR_L3F(env_cam, 0, 0, 0);
	transformSys->SetScale_L3F(env_cam, 1, 1, 1);

	auto camSys = world->GetCameraSystem();
	camSys->AddComponent(env_cam);
	camSys->SetAspect(env_cam, 1.0f);
	camSys->SetFar(env_cam, comp.farClip);
	camSys->SetNear(env_cam, comp.nearClip);
	camSys->SetFov(env_cam, XM_PIDIV2);

	// TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	auto env_scene = world->CreateScene(env_cam, resolution, resolution, false);
	
	unique_ptr<ScreenPlane> sp(new ScreenPlane(ENVPROBS_MAT));
	sp->SetFloat(comp.farClip, 0);
	
	int mipNum = 1;
	int cur_mip_res = resolution;
	while( cur_mip_res > ENVPROBS_SPEC_MIN )
	{
		cur_mip_res /= 2;
		mipNum++;
	}

	ScratchImage raw_cube[6];

	Vector3 m_cam_rot[6] = {
		Vector3(0, XM_PIDIV2, 0),
		Vector3(0, -XM_PIDIV2, 0),
		Vector3(-XM_PIDIV2, 0, 0),
		Vector3(XM_PIDIV2, 0, 0),
		Vector3(0, 0, 0),
		Vector3(0, XM_PI, 0)
	};
	
	for(int i=0; i<6; i++)
	{
		transformSys->SetRotationPYR_L(env_cam, m_cam_rot[i]);
		
		world->Snapshot(env_scene);
		
		RenderTarget* face = new RenderTarget;
		if(!face->Init(resolution, resolution))
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
	mipgen_cube.InitializeCube(fmt, resolution, resolution, 1, 1);
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
	
	const uint32_t facesCount = 6 * mipNum;

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

		int mip_res = resolution / int(pow(2, i));
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
		mip_sp->SetFloat(float(resolution), 2);

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
	cube.InitializeCube(fmt, resolution, resolution, 1, mipNum);
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

	mipcube.Release();
	for(int i=0; i<6; i++)
		raw_cube[i].Release();

	for(uint32_t i=0; i<facesCount; i++)
		i_faces[i].Release();
	i_faces.destroy();

	// TODO: save async
	// TEMP
	string envPath = RemoveExtension(world->GetWorldName()) + "/probes/";
	FileIO::CreateDir(envPath);

	string envTexName = GetProbFileName(comp.probName);
	hr = SaveToDDSFile( cube.GetImages(), cube.GetImageCount(), cube.GetMetadata(), DDS_FLAGS_NONE, StringToWstring(envTexName).data() );
	if ( FAILED(hr) )
	{
		ERR("Cant save environment prob specular file %s !", envTexName.c_str());
		cube.Release();
		return false;
	}

	cube.Release();	

	comp.needRebake = false;
	if( comp.probId == TexMgr::nullres )
		comp.probId = RELOADABLE_TEXTURE(envTexName, true);

	world->UpdateEnvProbRenderData(e);

	return true;
}