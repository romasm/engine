#include "stdafx.h"
#include "EnvProbSystem.h"
#include "World.h"
#include "TexMgr.h"

using namespace EngineCore;

int32_t EnvProbSystem::epResolutions[EP_QUAL_COUNT] = {256, 128, 32};
// the coarsest mip resolution = 4 x 4
uint32_t EnvProbSystem::epMipsCount[EP_QUAL_COUNT] = {7, 6, 4};
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
	comp->cachedDistance = 1.0f;
	comp->cachedShape = Vector3::Zero;
	comp->needRebake = true;
	
	comp->probId = TexMgr::nullres;

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

		const uint32_t mipsCount = GetMipsCount(i.quality);

		// TODO: update & validation on load
		//if(i.needRebake) // Verify texture params
		{			
			if( i.probId == TexMgr::nullres || i.probName.empty() )
				continue;

			const TextureMeta info = TexMgr::GetMeta(i.probId);
			if( info.width != GetResolution(i.quality) || info.mipsCount != mipsCount || info.format != GetFormat(i.quality) )
				continue;
		}

		if(!earlyVisComponent)
		{
			EnvProbData epData(i.probId, i.quality, i.cachedPos, mipsCount, i.cachedDistance, i.fade, 
				EnvParallaxType::EP_PARALLAX_NONE, i.offset, i.cachedShape, i.cachedInvTransform, ENVPROBS_PRIORITY_ALWAYS);

			for(auto f: *frustums)
			{
				if(f->rendermgr->IsShadow())
					continue;

				((SceneRenderMgr*)f->rendermgr)->RegEnvProb(epData);
			}
			continue;
		}
		
		EnvProbData epData(i.probId, i.quality, i.cachedPos, mipsCount, i.cachedDistance, i.fade, 
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

	DXGI_FORMAT fmt = GetFormat(comp.quality);
	int32_t resolution = GetResolution(comp.quality);
	uint32_t mipNum = GetMipsCount(comp.quality);

	if( !world->BeginCaptureProb(resolution, fmt, false) )
	{
		ERR("Cant init prob capture");
		return false;
	}
	
	const Matrix localOffset = Matrix::CreateTranslation(comp.offset);
	Matrix worldTransform = transformSys->GetTransform_W(comp.get_entity());
	worldTransform = localOffset * worldTransform;

	ID3D11ShaderResourceView* cubemapSRV = world->CaptureProb(worldTransform, comp.nearClip, comp.farClip);
	
	const uint32_t facesCount = 6 * mipNum;
	RArray<ScratchImage> i_faces;
	i_faces.create(facesCount);
	i_faces.resize(facesCount);
	
	for(int i = 0; i < (int)mipNum; i++)
	{
		int mip_res = resolution / int(pow(2, i));
		float roughness = pow(float(i) / float(mipNum - 1), 2);

		RenderTarget* mip_rt = new RenderTarget;
		if(!mip_rt->Init(mip_res, mip_res))
		{
			_CLOSE(mip_rt);
			return false;
		}

		for(int j=0; j<6; j++)
		{
			if(!mip_rt->AddRT(fmt))
			{
				_CLOSE(mip_rt);
				return false;
			}
		}

		// TODO: to copmute
		if(i == 0)
		{
			unique_ptr<ScreenPlane> mip_sp(new ScreenPlane(ENVPROBS_COPY_MAT));
			mip_sp->SetTexture(cubemapSRV, 0);

			mip_rt->ClearRenderTargets();
			mip_rt->SetRenderTarget();

			mip_sp->Draw();
		}
		else
		{
			unique_ptr<ScreenPlane> mip_sp(new ScreenPlane(ENVPROBS_MIPS_MAT));
			mip_sp->SetTextureByNameS(TEX_HAMMERSLEY, 0);
			mip_sp->SetTexture(cubemapSRV, 1);
			mip_sp->SetFloat(roughness, 0);
			mip_sp->SetFloat(float(mipNum + ENVPROBS_SPEC_MIPS_OFFSET), 1);
			mip_sp->SetFloat(float(resolution), 2);

			mip_rt->ClearRenderTargets();
			mip_rt->SetRenderTarget();

			mip_sp->Draw();
		}

		for(int j=0; j<6; j++)
		{
			ID3D11Resource* resource = nullptr;
			mip_rt->GetShaderResourceView(j)->GetResource(&resource);

			if (FAILED( CaptureTexture(Render::Device(), Render::Context(), resource, i_faces[j * mipNum + i]) ))
			{
				_CLOSE(mip_rt);
				return false;
			}
		}
		_CLOSE(mip_rt);
	}
	
	world->EndCaptureProb();
	
	ScratchImage cube;
	cube.InitializeCubeFromScratchImages(i_faces.data(), facesCount, mipNum);
	
	// TODO: save async 
	// TEMP
	string envPath = RemoveExtension(world->GetWorldName()) + ENVPROBS_SUBFOLDER_NOSLASH;
	if(!FileIO::IsExist(envPath))
		FileIO::CreateDir(envPath);

	string envTexName = GetProbFileName(comp.probName);
	if (FAILED( SaveToDDSFile( cube.GetImages(), cube.GetImageCount(), cube.GetMetadata(), DDS_FLAGS_NONE, StringToWstring(envTexName).data() ) ))
	{
		ERR("Cant save environment prob specular file %s !", envTexName.c_str());
		return false;
	}
	
	comp.needRebake = false;
	if( comp.probId == TexMgr::nullres )
		comp.probId = RELOADABLE_TEXTURE(envTexName, true);

	world->UpdateEnvProbRenderData(e);

	return true;
}