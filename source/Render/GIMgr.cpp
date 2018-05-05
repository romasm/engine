#include "stdafx.h"

#include "GIMgr.h"
#include "RenderMgrs.h"
#include "Render.h"
#include "Frustum.h"
#include "World.h"
#include "Utils\Profiler.h"

using namespace EngineCore;

GIMgr::GIMgr(BaseWorld* wrd)
{
	world = wrd;

	giVolume = nullptr;
	giVolumeSRV = nullptr;
	giVolumeUAV = nullptr;

	sgVolume = TexMgr::nullres;
	sampleDataGPU = nullptr;

	voxelSize = DEFAULT_VOXEL_SIZE;

	bDebugOctree = true;

 	if(!InitBuffers())
	{
		ERR("Cant init GI buffers");
	}
}

GIMgr::~GIMgr()
{
	_RELEASE(giVolumeUAV);
	_RELEASE(giVolumeSRV);
	_RELEASE(giVolume);

	TEXTURE_DROP(sgVolume);
	_RELEASE(sampleDataGPU);
}

bool GIMgr::InitBuffers()
{
#ifdef _EDITOR
	const DXGI_FORMAT format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	const int32_t resolution = 64;

	D3D11_TEXTURE3D_DESC volumeDesc;
	ZeroMemory(&volumeDesc, sizeof(volumeDesc));
	volumeDesc.Width = resolution;
	volumeDesc.Height = resolution;
	volumeDesc.Depth = resolution;
	volumeDesc.MipLevels = 1;
	volumeDesc.Usage = D3D11_USAGE_DEFAULT;
	volumeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	volumeDesc.CPUAccessFlags = 0;
	volumeDesc.MiscFlags = 0;
	volumeDesc.Format = format;
	if( FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &giVolume)) )
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC volumeSRVDesc;
	ZeroMemory(&volumeSRVDesc, sizeof(volumeSRVDesc));
	volumeSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	volumeSRVDesc.Texture3D.MipLevels = -1;
	volumeSRVDesc.Texture3D.MostDetailedMip = 0;
	volumeSRVDesc.Format = format;
	if( FAILED(Render::CreateShaderResourceView(giVolume, &volumeSRVDesc, &giVolumeSRV)) )
		return false;

	D3D11_UNORDERED_ACCESS_VIEW_DESC volumeUAVDesc;
	ZeroMemory(&volumeUAVDesc, sizeof(volumeUAVDesc));
	volumeUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	volumeUAVDesc.Texture3D.MipSlice = 0;
	volumeUAVDesc.Texture3D.WSize = resolution;
	volumeUAVDesc.Format = format;
	if( FAILED(Render::CreateUnorderedAccessView(giVolume, &volumeUAVDesc, &giVolumeUAV)) )
		return false;
#endif

	sampleDataGPU = Buffer::CreateConstantBuffer(Render::Device(), sizeof(GISampleData), true);


	// TEMP

#ifdef _EDITOR
	Render::ClearUnorderedAccessViewFloat(giVolumeUAV, Vector4(0, 0, 0, 0));
#endif

	sampleData.minCorner = Vector3(-5.0f, -5.0f, -5.0f);
	sampleData.worldSizeRcp = 1.0f / 10.0f;
	Render::UpdateDynamicResource(sampleDataGPU, &sampleData, sizeof(GISampleData));

	return true;
}

bool GIMgr::ReloadGIData()
{
	DropGIData();

	string giPath = world->GetWorldName() + GI_VOLUME;

#ifdef _EDITOR
	sgVolume = TexMgr::Get()->GetResource(giPath, true);
#else
	sgVolume = TexMgr::Get()->GetResource(giPath, false);
#endif

	return (sgVolume != TexMgr::nullres);
}

void GIMgr::DropGIData()
{
	TEXTURE_DROP(sgVolume);
}

ID3D11ShaderResourceView* GIMgr::GetGIVolumeSRV()
{
	if(giVolumeSRV)
		return giVolumeSRV;
	else
		return TEXTURE_GETPTR(sgVolume);
}

void GIMgr::DebugDrawOctree(DebugDrawer* dbgDrawer)
{
	if (!bDebugOctree)
		return;

	for (auto& item : debugOctreeVisuals)
	{
		float colorParam = (item.Extents.x - voxelSize * 0.5f) / (worldBBPow2.Extents.x * 0.5f - voxelSize * 0.5f);
		
		Vector3 color;
		color.x = powf(1.0f - clamp(0.0f, colorParam, 1.0f), 16.0f);
		color.y = 1.0f - powf(clamp(0.0f, 2.0f * abs(colorParam - 0.5f), 1.0f), 8.0f);
		color.z = powf(clamp(0.0f, colorParam, 1.0f), 0.4f);

		dbgDrawer->PushBoundingBox(item, color, true);
	}
}

bool GIMgr::BuildVoxelOctree()
{
	auto transformSys = world->GetTransformSystem();
	auto visibilitySys = world->GetVisibilitySystem();
	auto meshSys = world->GetStaticMeshSystem();
	
	BoundingBox worldBB;
	DArray<VoxelizeSceneItem> staticScene;
	Vector3 corners[8];

	string editorType(EDITOR_TYPE);

	for (auto& item : *transformSys->components.data())
	{
		if (item.mobility != MOBILITY_STATIC)
			continue;

		if (!visibilitySys->HasComponent(item.parent) || !meshSys->HasComponent(item.parent))
			continue;

		if (world->IsEntityType(item.parent, editorType))
			continue;

		VoxelizeSceneItem newItem;

		newItem.meshID = meshSys->GetMeshID(item.parent);
		if (MeshMgr::GetResourcePtr(newItem.meshID)->vertexFormat == MeshVertexFormat::LIT_SKINNED_VERTEX)
			continue;

		visibilitySys->GetBBoxW(item.parent).GetCorners(corners);
		BoundingBox::CreateFromPoints(newItem.bbox, 8, corners, sizeof(Vector3));

		newItem.transform = *transformSys->sceneGraph->GetWorldTransformation(item.nodeID);

		staticScene.push_back(newItem);

		BoundingBox::CreateMerged(worldBB, worldBB, newItem.bbox);
	}

	float maxWorldSize = max(worldBB.Extents.x, max(worldBB.Extents.y, worldBB.Extents.z)) * 2.0f;
	
	int32_t octreeDepth = (int32_t)ceilf(log2f(maxWorldSize / voxelSize));
	
	Vector3 minCornet = worldBB.Center - worldBB.Extents;
	Vector3 maxCornet = worldBB.Center + worldBB.Extents;

	float worldSizePow2 = powf(2.0f, (float)octreeDepth) * voxelSize;

	BoundingBox::CreateFromPoints(worldBBPow2, minCornet, minCornet + Vector3(worldSizePow2));

	octree.clear();
	octree.reserve(16384);

	debugOctreeVisuals.clear();
	debugOctreeVisuals.reserve(2048);

	octree.push_back();
	ProcessOctreeBranch(octree, staticScene, 0, worldBBPow2, octreeDepth - 1);

	LOG_GOOD("Octree nodes count: %u", (uint32_t)octree.size());

	return true;
}

bool GIMgr::SceneBoxIntersect(DArray<VoxelizeSceneItem>& staticScene, BoundingBox& bbox)
{
	for (auto& i : staticScene)
	{
		if (i.bbox.Intersects(bbox))
		{
			auto mesh = MeshMgr::GetResourcePtr(i.meshID);
			if (!mesh)
				continue;

			if (MeshLoader::MeshBoxOverlap(mesh, i.transform, bbox))
				return true;
		}
	}
	return false;
}

void GIMgr::ProcessOctreeBranch(DArray<OctreeBranch>& octree, DArray<VoxelizeSceneItem>& staticScene, uint32_t branchID, BoundingBox& bbox, int32_t octreeDepth)
{
	Vector3 corners[8];
	bbox.GetCorners(corners);

	OctreeBranch& branch = octree[branchID];
	memset(&branch, 0, sizeof(OctreeBranch));

	BoundingBox leafBox;
	for (int32_t i = 0; i < 8; i++)
	{
		BoundingBox::CreateFromPoints(leafBox, Vector3(bbox.Center), corners[i]);

		if (octreeDepth > 0 && SceneBoxIntersect(staticScene, leafBox))
		{
			MarkAsBranch(branch.leaf[i]);
			
			octree.push_back();
			ProcessOctreeBranch(octree, staticScene, uint32_t(octree.size() - 1), leafBox, octreeDepth - 1);
		}
		else
		{
			MarkAsLeaf(branch.leaf[i]);

			debugOctreeVisuals.push_back(leafBox);
		}
	}
}