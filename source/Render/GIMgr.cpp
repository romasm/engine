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

	bricksAtlas = nullptr;
	bricksAtlasSRV = nullptr;
	bricksAtlasUAV = nullptr;

	bricksTempAtlas = nullptr;
	bricksTempAtlasSRV = nullptr;
	bricksTempAtlasUAV = nullptr;

	chunksLookup = nullptr;
	chunksLookupSRV = nullptr;

	bricksLookup = nullptr;
	bricksLookupSRV = nullptr;

	bricksTexX = 0;
	bricksTexY = 0;

	bricksAtlasResource = TexMgr::nullres;
	bricksLookupsResource = TexMgr::nullres;
	chunksResource = TexMgr::nullres;

	sampleDataGPU = nullptr;

	cubemapToSH = nullptr;
	adressBuffer = nullptr;
	copyBricks = nullptr;
	interpolateProbes = nullptr;

	voxelSize = DEFAULT_OCTREE_VOXEL_SIZE;
	lookupMaxSize = (uint32_t)powf(2.0f, float(OCTREE_DEPTH - 1));
	chunkSize = (float)lookupMaxSize * 2.0f * DEFAULT_OCTREE_VOXEL_SIZE;

#ifdef _DEV
	debugGeomHandleOctree = -1;
	debugGeomHandleProbes = -1;
	debugGeomHandleVoxel = -1;
#endif

 	if(!InitBuffers())
	{
		ERR("Cant init GI buffers");
	}
}

GIMgr::~GIMgr()
{
#ifdef _DEV
	auto dbg = world->GetDebugDrawer();
	if (dbg)
	{
		dbg->DeleteGeometryHandle(debugGeomHandleOctree);
		dbg->DeleteGeometryHandle(debugGeomHandleProbes);
		dbg->DeleteGeometryHandle(debugGeomHandleVoxel);
	}
#endif

	DeleteResources();

	TEXTURE_DROP(bricksAtlasResource);
	TEXTURE_DROP(bricksLookupsResource);
	TEXTURE_DROP(chunksResource);

	_RELEASE(sampleDataGPU);

	_DELETE(cubemapToSH);
	_RELEASE(adressBuffer);
	_DELETE(copyBricks);
	_DELETE(interpolateProbes);
}

bool GIMgr::InitBuffers()
{
	sampleDataGPU = Buffer::CreateConstantBuffer(Render::Device(), sizeof(GISampleData), true);

	sampleData.minCorner = Vector3(0.0f);
	sampleData.chunkSizeRcp = 0.0f;
	sampleData.chunksCount = Vector3Uint32(1, 1, 1);
	sampleData.minHalfVoxelSize = voxelSize * 0.5f;
	sampleData.brickAtlasOffset = Vector3(1.0f, 1.0f, 1.0f / BRICK_COEF_COUNT);
	sampleData.halfBrickVoxelSize = 0.5f * Vector3(1.0f / BRICK_RESOLUTION, 1.0f / BRICK_RESOLUTION, 1.0f / (BRICK_RESOLUTION * BRICK_COEF_COUNT));
	sampleData.brickSampleSize = sampleData.halfBrickVoxelSize * 4.0f;
	Render::UpdateDynamicResource(sampleDataGPU, &sampleData, sizeof(GISampleData));

#ifdef _EDITOR

	cubemapToSH = new Compute(SHADER_CUBEMAP_TO_SH);
	adressBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(SHAdresses), true);

	copyBricks = new Compute(SHADER_BRICKS_COPY);
	interpolateProbes = new Compute(SHADER_INTERPOLATE_PROBES);

#endif

	return true;
}

ID3D11ShaderResourceView* GIMgr::GetGIBricksSRV()
{
	if (!bricksAtlasSRV)
		return TexMgr::GetResourcePtr(bricksAtlasResource);

	return bricksAtlasSRV;
}

ID3D11ShaderResourceView* GIMgr::GetGIChunksSRV()
{
	if (!chunksLookupSRV)
		return TexMgr::GetResourcePtr(chunksResource);

	return chunksLookupSRV;
}

ID3D11ShaderResourceView* GIMgr::GetGILookupsSRV()
{
	if (!bricksLookupSRV)
		return TexMgr::GetResourcePtr(bricksLookupsResource);

	return bricksLookupSRV;
}

void GIMgr::DeleteResources()
{
	_RELEASE(chunksLookupSRV);
	_RELEASE(chunksLookup);

	_RELEASE(bricksLookupSRV);
	_RELEASE(bricksLookup);

	_RELEASE(bricksAtlasUAV);
	_RELEASE(bricksAtlasSRV);
	_RELEASE(bricksAtlas);

	_RELEASE(bricksTempAtlasUAV);
	_RELEASE(bricksTempAtlasSRV);
	_RELEASE(bricksTempAtlas);
}

bool GIMgr::CompareOctrees(Octree& first, Octree& second)
{
	return first.lookupRes < second.lookupRes;
}

void GIMgr::SwapOctrees(Octree* first, Octree* second, RArray<RArray<RArray<int32_t>>>* arr)
{
	swap((*arr)[first->parentChunk.x][first->parentChunk.y][first->parentChunk.z], 
		(*arr)[second->parentChunk.x][second->parentChunk.y][second->parentChunk.z]);
	Octree::Swap(*first, *second);
}

bool GIMgr::CompareInterpolationLinks(ProbInterpolation& first, ProbInterpolation& second)
{
	return first.minDepth < second.minDepth;
}

void GIMgr::LoadGIData(GISampleData& giData)
{
	string bricksAtlasFile = world->GetWorldName() + GI_BRICKS_ATLAS_PATH;
	string bricksLookupsFile = world->GetWorldName() + GI_BRICKS_LOOKUP_PATH;
	string chunksFile = world->GetWorldName() + GI_CHUNKS_PATH;

	if (FileIO::IsExist(bricksAtlasFile))
		bricksAtlasResource = TexMgr::Get()->GetResource(bricksAtlasFile, false);
	if (FileIO::IsExist(bricksLookupsFile))
		bricksLookupsResource = TexMgr::Get()->GetResource(bricksLookupsFile, false);
	if (FileIO::IsExist(chunksFile))
		chunksResource = TexMgr::Get()->GetResource(chunksFile, false);

	sampleData = giData;
	Render::UpdateDynamicResource(sampleDataGPU, &sampleData, sizeof(GISampleData));
}

GISampleData* GIMgr::SaveGIData()
{
	if (!bricksAtlas || !bricksLookup || !chunksLookup)
	{
		LOG("GI data not baked");
		return &sampleData;
	}

	ScratchImage bricksAtlasVolume;
	if (FAILED(CaptureTexture(Render::Device(), Render::Context(), bricksAtlas, bricksAtlasVolume)))
	{
		ERR("Cant get GI bricks atlas");
	}
	else
	{
		string bricksAtlasFile = world->GetWorldName() + GI_BRICKS_ATLAS_PATH;
		if (FAILED(SaveToDDSFile(bricksAtlasVolume.GetImages(), bricksAtlasVolume.GetImageCount(), bricksAtlasVolume.GetMetadata(),
			DDS_FLAGS_NONE, StringToWstring(bricksAtlasFile).data())))
			ERR("Cant write GI bricks atlas");
	}

	ScratchImage bricksLookupsVolume;
	if (FAILED(CaptureTexture(Render::Device(), Render::Context(), bricksLookup, bricksLookupsVolume)))
	{
		ERR("Cant get GI bricks lookups");
	}
	else
	{
		string bricksLookupsFile = world->GetWorldName() + GI_BRICKS_LOOKUP_PATH;
		if (FAILED(SaveToDDSFile(bricksLookupsVolume.GetImages(), bricksLookupsVolume.GetImageCount(), bricksLookupsVolume.GetMetadata(),
			DDS_FLAGS_NONE, StringToWstring(bricksLookupsFile).data())))
			ERR("Cant write GI bricks lookups");
	}

	ScratchImage chunksVolume;
	if (FAILED(CaptureTexture(Render::Device(), Render::Context(), chunksLookup, chunksVolume)))
	{
		ERR("Cant get GI chunks");
	}
	else
	{
		string chunksFile = world->GetWorldName() + GI_CHUNKS_PATH;
		if (FAILED(SaveToDDSFile(chunksVolume.GetImages(), chunksVolume.GetImageCount(), chunksVolume.GetMetadata(),
			DDS_FLAGS_NONE, StringToWstring(chunksFile).data())))
			ERR("Cant write GI chunks");
	}

	return &sampleData;
}

bool GIMgr::RecreateResources()
{
	DeleteResources();
	
	sampleData.minCorner = worldBox.corner;
	sampleData.minHalfVoxelSize = voxelSize * 0.5f;

	// bricks atlas temp
	const DXGI_FORMAT formatBricksTemp = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;

	D3D11_TEXTURE3D_DESC volumeDesc;
	ZeroMemory(&volumeDesc, sizeof(volumeDesc));
	volumeDesc.Width = bricksTexX * BRICK_RESOLUTION;
	volumeDesc.Height = bricksTexY * BRICK_RESOLUTION;
	volumeDesc.Depth = BRICK_RESOLUTION * BRICK_COEF_COUNT;
	volumeDesc.MipLevels = 1;
	volumeDesc.Usage = D3D11_USAGE_DEFAULT;
	volumeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	volumeDesc.CPUAccessFlags = 0;
	volumeDesc.MiscFlags = 0;
	volumeDesc.Format = formatBricksTemp;
	if (FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &bricksTempAtlas)))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC volumeSRVDesc;
	ZeroMemory(&volumeSRVDesc, sizeof(volumeSRVDesc));
	volumeSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	volumeSRVDesc.Texture3D.MipLevels = -1;
	volumeSRVDesc.Texture3D.MostDetailedMip = 0;
	volumeSRVDesc.Format = formatBricksTemp;
	if (FAILED(Render::CreateShaderResourceView(bricksTempAtlas, &volumeSRVDesc, &bricksTempAtlasSRV)))
		return false;

	D3D11_UNORDERED_ACCESS_VIEW_DESC volumeUAVDesc;
	ZeroMemory(&volumeUAVDesc, sizeof(volumeUAVDesc));
	volumeUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	volumeUAVDesc.Texture3D.MipSlice = 0;
	volumeUAVDesc.Texture3D.WSize = volumeDesc.Depth;
	volumeUAVDesc.Format = formatBricksTemp;
	if (FAILED(Render::CreateUnorderedAccessView(bricksTempAtlas, &volumeUAVDesc, &bricksTempAtlasUAV)))
		return false;
	 
	Render::ClearUnorderedAccessViewFloat(bricksTempAtlasUAV, Vector4(0, 0, 0, 0));
	 
	sampleData.brickAtlasOffset = Vector3(1.0f / bricksTexX, 1.0f / bricksTexY, 1.0f / BRICK_COEF_COUNT);
	sampleData.halfBrickVoxelSize = 0.5f * Vector3(1.0f / volumeDesc.Width, 1.0f / volumeDesc.Height, 1.0f / volumeDesc.Depth);
	sampleData.brickSampleSize = sampleData.halfBrickVoxelSize * 4.0f;

	// brick atlas final
	// TODO: DXGI_FORMAT_R9G9B9E5_SHAREDEXP, DXGI_FORMAT_R11G11B10_FLOAT, DXGI_FORMAT_BC6H_UF16 ???
	const DXGI_FORMAT formatBricks = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;

	volumeDesc.Format = formatBricks;
	if (FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &bricksAtlas)))
		return false;

	volumeSRVDesc.Format = formatBricks;
	if (FAILED(Render::CreateShaderResourceView(bricksAtlas, &volumeSRVDesc, &bricksAtlasSRV)))
		return false;

	volumeUAVDesc.Format = formatBricks;
	if (FAILED(Render::CreateUnorderedAccessView(bricksAtlas, &volumeUAVDesc, &bricksAtlasUAV)))
		return false;

	Render::ClearUnorderedAccessViewFloat(bricksAtlasUAV, Vector4(0, 0, 0, 0));

	LOG_GOOD("Brick atlas size: %i mb", volumeDesc.Width * volumeDesc.Height * volumeDesc.Depth * sizeof(Vector4Uint16) / 1048576);
	// ----------------------------------------------------------------------
	
	// chunks lookup
	uint32_t chunksX = (uint32_t)chunks.size();
	if (chunksX == 0)
		return false;
	uint32_t chunksY = (uint32_t)chunks[0].size();
	if (chunksY == 0)
		return false;
	uint32_t chunksZ = (uint32_t)chunks[0][0].size();
	
	// constuct lookup 3d volume
	QSortSwap(octreeArray.begin(), octreeArray.end(), GIMgr::CompareOctrees, GIMgr::SwapOctrees, &chunks);

	// estimate size
	float lookupCount = 0;
	for (auto& item : octreeArray)
		lookupCount += 1.0f / powf(8.0f, float(OCTREE_DEPTH - item.depth));

	lookupCount = ceilf(lookupCount);

	uint32_t lookupXCount = (uint32_t)ceilf(sqrtf(lookupCount));
	uint32_t lookupYCount = (uint32_t)ceilf(lookupCount / lookupXCount);

	uint32_t lookupArrayX = lookupMaxSize * lookupXCount;
	uint32_t lookupArrayY = lookupMaxSize * lookupYCount;

	// reserve adresses & fill chunks links
	Vector4Uint16* chunksArray = new Vector4Uint16[chunksX * chunksY * chunksZ];
	uint32_t zStride = chunksX * chunksY;

	uint32_t offsetX = 0;
	uint32_t offsetY = 0;
	uint32_t offsetZ = 0;
	uint32_t prevLevelX = 0;
	uint32_t prevLevelY = 0;
	uint32_t prevLevelZ = 0;

	int32_t currentDepth = octreeArray[0].depth;

	for (auto& item : octreeArray)
	{
		if (item.depth != currentDepth)
		{
			prevLevelX = offsetX;
			prevLevelY = offsetY;
			prevLevelZ = offsetZ;
			currentDepth = item.depth;
		}

		uint32_t chunkID = item.parentChunk.z * zStride + item.parentChunk.y * chunksX + item.parentChunk.x;

		Vector4Uint16 adress((uint16_t)offsetX, (uint16_t)offsetY, (uint16_t)offsetZ, (uint16_t)item.lookupRes);		
		chunksArray[chunkID] = adress;
		item.lookupAdress = adress;
		
		// next adress
		offsetX += item.lookupRes;
		if (offsetX % lookupMaxSize == 0 && item.depth != OCTREE_DEPTH)
		{
			uint32_t tempZ = offsetZ + item.lookupRes;
			if (tempZ >= lookupMaxSize)
			{
				uint32_t tempY = offsetY + item.lookupRes;
				offsetZ = 0;

				if (tempY % lookupMaxSize == 0)
				{
					offsetY = tempY - lookupMaxSize;
				}
				else
				{
					offsetX -= lookupMaxSize;
					offsetY = tempY;
				}
			}
			else
			{
				offsetZ = tempZ;
				offsetX -= lookupMaxSize;
			}
		}

		// TODO: for different sizes this doesnt work !!!!!!!!!!!!!!!!!!!!!!
		
		if (offsetX >= lookupArrayX)
		{
			offsetX = 0;
			offsetY += lookupMaxSize;
		}
	}
	
	// chunks to GPU
	const DXGI_FORMAT formatChunks = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;

	D3D11_SUBRESOURCE_DATA chunksData;
	chunksData.pSysMem = chunksArray;
	chunksData.SysMemPitch = chunksX * sizeof(Vector4Uint16);
	chunksData.SysMemSlicePitch = zStride * sizeof(Vector4Uint16);

	volumeDesc.Width = chunksX;
	volumeDesc.Height = chunksY;
	volumeDesc.Depth = chunksZ;
	volumeDesc.Usage = D3D11_USAGE_IMMUTABLE;
	volumeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	volumeDesc.Format = formatChunks;

	HRESULT hr = Render::CreateTexture3D(&volumeDesc, &chunksData, &chunksLookup);
	_DELETE_ARRAY(chunksArray);

	if (FAILED(hr))
		return false;

	volumeSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	volumeSRVDesc.Format = formatChunks;
	if (FAILED(Render::CreateShaderResourceView(chunksLookup, &volumeSRVDesc, &chunksLookupSRV)))
		return false;
	
	sampleData.chunkSizeRcp = 1.0f / chunkSize;
	sampleData.chunksCount = Vector3Uint32(chunksX, chunksY, chunksZ);

	LOG_GOOD("Chunks lookup size: %i b", chunksX * chunksY * chunksZ * sizeof(Vector4Uint16));
	// ----------------------------------------------

	// fill lookups links
	uint32_t* lookupArray = new uint32_t[lookupArrayX * lookupArrayY * lookupMaxSize];
	zStride = lookupArrayX * lookupArrayY;

	for (auto& item : octreeArray)
	{
		const int32_t lookupResSq = item.lookupRes * item.lookupRes;
		for (int32_t x = 0; x < item.lookupRes; x++)
		{
			const int32_t arrX = item.lookupAdress.x + x;
			for (int32_t y = 0; y < item.lookupRes; y++)
			{
				const int32_t luYX = y * item.lookupRes + x;
				const int32_t arrYX = (item.lookupAdress.y + y) * lookupArrayX + arrX;
				for (int32_t z = 0; z < item.lookupRes; z++)
				{
					uint32_t brickAdress = bricksLinks[item.lookup[z * lookupResSq + luYX]];
					lookupArray[(item.lookupAdress.z + z) * zStride + arrYX] = brickAdress;
				}
			}
		}
	}

	// lookups to GPU
	const DXGI_FORMAT formatLookup = DXGI_FORMAT::DXGI_FORMAT_R32_UINT;

	D3D11_SUBRESOURCE_DATA lookupArrayData;
	lookupArrayData.pSysMem = lookupArray;
	lookupArrayData.SysMemPitch = lookupArrayX * sizeof(uint32_t);
	lookupArrayData.SysMemSlicePitch = zStride * sizeof(uint32_t);

	volumeDesc.Width = lookupArrayX;
	volumeDesc.Height = lookupArrayY;
	volumeDesc.Depth = lookupMaxSize;
	volumeDesc.Format = formatLookup;

	hr = Render::CreateTexture3D(&volumeDesc, &lookupArrayData, &bricksLookup);
	_DELETE_ARRAY(lookupArray);

	if (FAILED(hr))
		return false;

	volumeSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	volumeSRVDesc.Format = formatLookup;
	if (FAILED(Render::CreateShaderResourceView(bricksLookup, &volumeSRVDesc, &bricksLookupSRV)))
		return false;

	LOG_GOOD("Brick lookup size: %i kb", lookupArrayX * lookupArrayY * lookupMaxSize * sizeof(uint32_t) / 1024);

	Render::UpdateDynamicResource(sampleDataGPU, &sampleData, sizeof(GISampleData));

	return true;
}

static const int32_t captureResolutions[PROB_CAPTURE_BOUNCES] = 
{ PROB_CAPTURE_RESOLUTION_B0, PROB_CAPTURE_RESOLUTION_B1, PROB_CAPTURE_RESOLUTION_B2, PROB_CAPTURE_RESOLUTION_B3 };

const uint32_t probesOffset[27][3] =
{
	{ 0,0,0 },
	{ 1,0,0 },
	{ 2,0,0 },
	{ 0,1,0 },
	{ 1,1,0 },
	{ 2,1,0 },
	{ 0,2,0 },
	{ 1,2,0 },
	{ 2,2,0 },

	{ 0,0,1 },
	{ 1,0,1 },
	{ 2,0,1 },
	{ 0,1,1 },
	{ 1,1,1 },
	{ 2,1,1 },
	{ 0,2,1 },
	{ 1,2,1 },
	{ 2,2,1 },

	{ 0,0,2 },
	{ 1,0,2 },
	{ 2,0,2 },
	{ 0,1,2 },
	{ 1,1,2 },
	{ 2,1,2 },
	{ 0,2,2 },
	{ 1,2,2 },
	{ 2,2,2 }
};

bool GIMgr::BakeGI()
{
	octreeArray.destroy();
	chunks.destroy();
	bricks.destroy();
	probesArray.destroy();
	staticScene.destroy();

	// collect static geometry
	auto transformSys = world->GetTransformSystem();
	auto visibilitySys = world->GetVisibilitySystem();
	auto meshSys = world->GetStaticMeshSystem();

	BoundingBox worldBBraw;
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

		BoundingBox::CreateMerged(worldBBraw, worldBBraw, newItem.bbox);
	}

	// build chunk array
	// TODO: configurate
	worldBBraw.Extents = worldBBraw.Extents + Vector3(voxelSize);
	worldBox.corner = worldBBraw.Center - worldBBraw.Extents;
	worldBox.size = worldBBraw.Extents * 2.0f;

	Vector3 chunkCount = worldBox.size * (1.0f / chunkSize);
	chunkCount.x = ceilf(chunkCount.x);
	chunkCount.y = ceilf(chunkCount.y);
	chunkCount.z = ceilf(chunkCount.z);

	worldBox.size = chunkCount * chunkSize;

	int32_t xSize = (int32_t)chunkCount.x;
	int32_t ySize = (int32_t)chunkCount.y;
	int32_t zSize = (int32_t)chunkCount.z;

	chunks.create(xSize);
	chunks.resize(xSize);
	for (int32_t x = 0; x < xSize; x++)
	{
		chunks[x].create(ySize);
		chunks[x].resize(ySize);
		for (int32_t y = 0; y < ySize; y++)
		{
			chunks[x][y].create(zSize);
			chunks[x][y].resize(zSize);
			chunks[x][y].assign(-1);
		}
	}

	BoundingBox chunkBox;
	chunkBox.Extents = Vector3(chunkSize * 0.5f);

	octreeArray.create(xSize * ySize * zSize);

	for (int32_t x = 0; x < xSize; x++)
	{
		for (int32_t y = 0; y < ySize; y++)
		{
			for (int32_t z = 0; z < zSize; z++)
			{
				chunkBox.Center = worldBox.corner + chunkSize * Vector3(float(x), float(y), float(z)) + chunkBox.Extents;

				auto* octree = octreeArray.push_back();
				octree->bbox = chunkBox;

				if (SceneBoxIntersect(chunkBox))
				{
					// TODO: configurate per chunk
					octree->depth = OCTREE_DEPTH;
				}
				else
				{
					octree->depth = 1;
				}

				octree->lookupRes = (int32_t)powf(2.0f, float(octree->depth - 1));

				uint32_t lookupPlanarSize = octree->lookupRes * octree->lookupRes * octree->lookupRes;

				octree->lookup = new int32_t[lookupPlanarSize];
				memset(octree->lookup, -1, sizeof(int32_t) * lookupPlanarSize);

				octree->parentChunk = Vector3Uint32(x, y, z);

				chunks[x][y][z] = (int32_t)octreeArray.size() - 1;
			}
		}
	}

	LOG_GOOD("Chunks count: %i", (int32_t)octreeArray.size());

	// build octrees
	for (auto& octree : octreeArray)
	{
		int32_t estimatedCount = (int32_t)powf(2.0f, (float)octree.depth);
		estimatedCount = estimatedCount * estimatedCount * estimatedCount;
		estimatedCount = max(1, int32_t(estimatedCount * 0.004f));

		Vector3 octreeHelper = Vector3((float)octree.lookupRes) / (octree.bbox.Extents * 2.0f);
		Vector3 octreeCorner = octree.bbox.Center - octree.bbox.Extents;

		ProcessOctreeBranch(octree, octree.bbox, octree.depth - 1, octreeHelper, octreeCorner);
	}

	probesLookup.clear();

	LOG_GOOD("Bricks count: %i", (int32_t)bricks.size());
	LOG_GOOD("Probes count: %i", (int32_t)probesArray.size());

	bricksTexX = (uint32_t)ceilf(sqrtf((float)bricks.size()));
	bricksTexY = (uint32_t)ceilf((float)bricks.size() / bricksTexX);

	// reserve bricks places & set probes adresses
	bricksLinks.reserve(bricks.size());
	for (uint32_t i = 0; i < (uint32_t)bricks.size(); i++)
	{
		uint32_t y = i / bricksTexX;
		uint32_t x = i % bricksTexX;
		bricksLinks.push_back(SetLookupNode(x, y, bricks[i].depth));

		x *= 3;
		y *= 3;

		for (uint32_t k = 0; k < BRICK_RESOLUTION * BRICK_RESOLUTION * BRICK_RESOLUTION; k++)
		{
			Prob& prob = probesArray[bricks[i].probes[k]];
			prob.adresses.push_back(Vector3Uint32(x + probesOffset[k][0], y + probesOffset[k][1], probesOffset[k][2]));
		}
	}

	if (!RecreateResources())
	{
		ERR("Cant create resources for GI");
		return false;
	}

	bricksLinks.clear();

	// post process probes
	Vector3 cornerMax = worldBox.corner + worldBox.size;

	RArray<ProbInterpolation> interpolationArray;
	interpolationArray.create(probesArray.size());

	VoxelizedCube<PROB_CAPTURE_OFFSET_VOXEL_RES> voxelsCube;

	for (int32_t i = 0; i < (int32_t)probesArray.size(); i++)
	{
		auto& prob = probesArray[i];

		// check is baking
		if (prob.minDepth == 0 || prob.neighborFlags == 0xff)
			prob.bake = true;
		else
		{
			Vector3 posCheckMin = prob.pos - worldBox.corner;
			Vector3 posCheckMax = prob.pos - cornerMax;

			posCheckMin.x = abs(posCheckMin.x);
			posCheckMin.y = abs(posCheckMin.y);
			posCheckMin.z = abs(posCheckMin.z);
			posCheckMax.x = abs(posCheckMax.x);
			posCheckMax.y = abs(posCheckMax.y);
			posCheckMax.z = abs(posCheckMax.z);

			posCheckMin.x = min(posCheckMin.x, min(posCheckMin.y, posCheckMin.z));
			posCheckMax.x = min(posCheckMax.x, min(posCheckMax.y, posCheckMax.z));

			if (min(posCheckMin.x, posCheckMax.x) < 0.001f)
				prob.bake = true;
		}

		if (prob.bake)
		{
			if (prob.geomCloseProbe)
			{
				AdjustProbPos(prob.pos, voxelsCube);
			}
		}
		else
		{
			// prepare interpolation
			ProbInterpolation* probInterp = interpolationArray.push_back();
			probInterp->probID = i;
			probInterp->minDepth = prob.minDepth;
			probInterp->lerpOffset = GetOutterVectorFromNeighbors(prob.neighborFlags);
		}
	}

	// baking
	for (int32_t b = 0; b < PROB_CAPTURE_BOUNCES; b++)
	{
		const int32_t captureResolution = captureResolutions[b];
		const DXGI_FORMAT formatProb = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;

		if (!world->BeginCaptureProb(captureResolution, formatProb, false))
		{
			ERR("Cant init prob capture");
			return false;
		}

		ID3D11ShaderResourceView* probSRV = world->GetCaptureProbSRV();
		if (!probSRV)
			return false;

		SHAdresses adresses;
		adresses.pixelCountRcp = 1.0f / (captureResolution * captureResolution * 6);

		uint32_t groupsCountXY = captureResolution / 16;
		uint32_t groupsCountZ = 6;

		double startTime = Timer::ForcedGetCurrentTime();
		double currentTime = 0;
		int32_t bakedCount = 0;
		double prevTime = startTime;

		for (auto& prob : probesArray)
		{
			if (!prob.bake)
				continue;

			for (int32_t i = 0; i < (int32_t)prob.adresses.size(); i++)
			{
				auto& adr = prob.adresses[i];
				adresses.adresses[i] = Vector4((float)adr.x, (float)adr.y, (float)adr.z, 0);
			}
			adresses.adresses[0].w = (float)prob.adresses.size();

			world->CaptureProb(Matrix::CreateTranslation(prob.pos), PROB_CAPTURE_NEARCLIP, PROB_CAPTURE_FARCLIP);

			Render::UpdateDynamicResource(adressBuffer, &adresses, sizeof(SHAdresses));

			Render::CSSetConstantBuffers(0, 1, &adressBuffer);
			Render::CSSetShaderResources(0, 1, &probSRV);
			cubemapToSH->BindUAV(bricksTempAtlasUAV);
			cubemapToSH->Dispatch(groupsCountXY, groupsCountXY, groupsCountZ);
			cubemapToSH->UnbindUAV();

			bakedCount++;

			currentTime = Timer::ForcedGetCurrentTime();
			if (currentTime - prevTime >= 1000.0f)
			{
				prevTime = currentTime;
				DBG_SHORT("Baked: %i", bakedCount);
			}
		}

		world->EndCaptureProb();
		LOG("Baked with speed %f probes per second", 1000.0f * float(bakedCount) / float(currentTime - startTime));
		LOG("Bounce %i baked", b);

		// lerp probes
		InterpolateProbes(interpolationArray);

		CopyBricks();
	}
	
	_RELEASE(bricksTempAtlasUAV);
	_RELEASE(bricksTempAtlasSRV);
	_RELEASE(bricksTempAtlas);

	interpolationArray.destroy();

#ifndef _DEV
	octreeArray.destroy();
	chunks.destroy();
	bricks.destroy();
	probesArray.destroy();
	staticScene.destroy();
#endif
	
	return true;
}

bool GIMgr::SceneBoxIntersect(BoundingBox& bbox)
{
	BoundingBox extendedBBox = bbox;
	extendedBBox.Extents = extendedBBox.Extents + Vector3(voxelSize * OCTREE_INTERSECT_TOLERANCE);

	for (auto& i : staticScene)
	{
		if (i.bbox.Intersects(extendedBBox))
		{
			if (MeshMgr::MeshBoxOverlap(i.meshID, i.transform, extendedBBox))
				return true;
		}
	}
	return false;
}

const int32_t brickPointsWeights[27][8] =
{
	{ 0,0, 0,0, 1,0, 0,0 },
	{ 0,0, 0,0, 1,1, 0,0 },
	{ 0,0, 0,0, 0,1, 0,0 },

	{ 0,0, 0,0, 1,0, 0,1 },
	{ 0,0, 0,0, 1,1, 1,1 },
	{ 0,0, 0,0, 0,1, 1,0 },

	{ 0,0, 0,0, 0,0, 0,1 },
	{ 0,0, 0,0, 0,0, 1,1 },
	{ 0,0, 0,0, 0,0, 1,0 },

	{ 1,0, 0,0, 1,0, 0,0 },
	{ 1,1, 0,0, 1,1, 0,0 },
	{ 0,1, 0,0, 0,1, 0,0 },

	{ 1,0, 0,1, 1,0, 0,1 },
	{ 1,1, 1,1, 1,1, 1,1 },
	{ 0,1, 1,0, 0,1, 1,0 },

	{ 0,0, 0,1, 0,0, 0,1 },
	{ 0,0, 1,1, 0,0, 1,1 },
	{ 0,0, 1,0, 0,0, 1,0 },

	{ 1,0, 0,0, 0,0, 0,0 },
	{ 1,1, 0,0, 0,0, 0,0 },
	{ 0,1, 0,0, 0,0, 0,0 },

	{ 1,0, 0,1, 0,0, 0,0 },
	{ 1,1, 1,1, 0,0, 0,0 },
	{ 0,1, 1,0, 0,0, 0,0 },

	{ 0,0, 0,1, 0,0, 0,0 },
	{ 0,0, 1,1, 0,0, 0,0 },
	{ 0,0, 1,0, 0,0, 0,0 }
};

const int32_t probesFlag[27] = {7,6,6,3,2,2,3,2,2,5,4,4,1,-1,0,1,0,0,5,4,4,1,0,0,1,0,0};

void GIMgr::ProcessOctreeBranch(Octree& octree, BoundingBox& bbox, int32_t octreeDepth, Vector3& octreeHelper, Vector3& octreeCorner)
{
	Vector3 corners[8];
	bbox.GetCorners(corners);
	
	bool hasBrick = false;

	if (octreeDepth > 0)
	{
		BoundingBox leafBox;
		for (int32_t i = 0; i < 8; i++)
		{
			BoundingBox::CreateFromPoints(leafBox, Vector3(bbox.Center), corners[i]);

			if (SceneBoxIntersect(leafBox))
			{
				ProcessOctreeBranch(octree, leafBox, octreeDepth - 1, octreeHelper, octreeCorner);
			}
			else
			{
				hasBrick = true;
			}
		}
	}
	else
	{
		hasBrick = true;
	}

	if (hasBrick)
	{
		Brick& newBrick = bricks.push_back();
		uint32_t brickID = uint32_t(bricks.size() - 1);

		newBrick.bbox = bbox;
		newBrick.depth = octree.depth - 1 - octreeDepth;
		
		Vector3 bboxMin = octreeHelper * (bbox.Center - octreeCorner - bbox.Extents);
		Vector3 bboxMax = octreeHelper * (bbox.Center - octreeCorner + bbox.Extents);

		const int32_t lookupResSq = octree.lookupRes * octree.lookupRes;

		for (int32_t x = (int32_t)roundf(bboxMin.x); x < (int32_t)roundf(bboxMax.x); x++)
		{
			for (int32_t y = (int32_t)roundf(bboxMin.y); y < (int32_t)roundf(bboxMax.y); y++)
			{
				for (int32_t z = (int32_t)roundf(bboxMin.z); z < (int32_t)roundf(bboxMax.z); z++)
				{
					int32_t& lookupNode = octree.lookup[z * lookupResSq + y * octree.lookupRes + x];
					if (lookupNode < 0)
						lookupNode = brickID;
				}
			}
		}
		
		// probs registration
		for (int32_t i = 0; i < BRICK_RESOLUTION * BRICK_RESOLUTION * BRICK_RESOLUTION; i++)
		{
			Vector3 probPos;
			int32_t count = 0;
			for (int32_t k = 0; k < 8; k++)
			{
				if (brickPointsWeights[i][k] > 0)
				{
					probPos += corners[k];
					count++;
				}
			}

			probPos *= Vector3(1.0f / count);

			uint64_t posForHash = PosToUint(probPos);

			auto probIt = probesLookup.find(posForHash);
			if (probIt == probesLookup.end())
			{
				Prob& prob = probesArray.push_back();
				uint32_t probID = uint32_t(probesArray.size() - 1);

				prob.bake = false;
				prob.pos = probPos;
				prob.adresses.destroy();
				prob.minDepth = newBrick.depth;
				prob.neighborFlags = GetNeighborFlag(i);
				prob.geomCloseProbe = (octreeDepth == 0);

				probesLookup.insert(make_pair(posForHash, probID));
				newBrick.probes[i] = probID;
			}
			else
			{
				newBrick.probes[i] = probIt->second;

				Prob& prob = probesArray[probIt->second];
				prob.neighborFlags = (prob.neighborFlags | GetNeighborFlag(i));
				prob.minDepth = min(prob.minDepth, (uint8_t)newBrick.depth);
				prob.geomCloseProbe = prob.geomCloseProbe || (octreeDepth == 0);
			}
		}
	}
}

template<class voxelGrid>
void GIMgr::AdjustProbPos(Vector3& pos, voxelGrid& voxels)
{
	voxels.Clear();

	BoundingBox voxelsBox;
	voxelsBox.Center = pos;
	voxelsBox.Extents = Vector3(voxelSize + voxelSize / (voxels.resolution - 1));

	for (auto& i : staticScene)
	{
		if (i.bbox.Intersects(voxelsBox))
			MeshMgr::MeshVoxelize(i.meshID, i.transform, voxelsBox, voxels);
	}
	
	if (voxels.empty)
		return;

	voxels.Grow();

	int32_t centerVoxel = voxels.resolution / 2;
	int32_t minOffsetSq = 3 * voxels.resolution * voxels.resolution;
	Vector3Int32 offset(0, 0, 0);

	for (int32_t x = 0; x < (int32_t)voxels.resolution; x++)
		for (int32_t y = 0; y < (int32_t)voxels.resolution; y++)
			for (int32_t z = 0; z < (int32_t)voxels.resolution; z++)
			{
				if (voxels.voxels[x][y][z] != 0)
					continue;

				int32_t xOffset = x - centerVoxel;
				int32_t yOffset = y - centerVoxel;
				int32_t zOffset = z - centerVoxel;

				int32_t offsetSq = xOffset * xOffset + yOffset * yOffset + zOffset * zOffset;
				if (offsetSq < minOffsetSq)
				{
					minOffsetSq = offsetSq;
					offset.x = xOffset;
					offset.y = yOffset;
					offset.z = zOffset;
				}
			}
	
	pos += voxelsBox.Extents * Vector3((float)offset.x, (float)offset.y, (float)offset.z) * (2.0f / voxels.resolution);
}

void GIMgr::InterpolateProbes(RArray<ProbInterpolation>& interpolationArray)
{
	// sort probes interpolation links
	sort(interpolationArray.begin(), interpolationArray.end(), GIMgr::CompareInterpolationLinks);

	RArray<ProbInterpolationGPU> interpolationDataGPU;
	interpolationDataGPU.create(interpolationArray.size());

	int32_t startProbe = 0;
	int32_t currentProbe = 0;

	while (currentProbe < (int32_t)interpolationArray.size())
	{
		uint8_t currentDepth = interpolationArray[startProbe].minDepth;

		CopyBricks();

		for (currentProbe = startProbe; currentProbe < (int32_t)interpolationArray.size(); currentProbe++)
		{
			auto& interpProb = interpolationArray[currentProbe];

			if (currentDepth != interpProb.minDepth)
			{
				startProbe = currentProbe;
				break;
			}

			auto& prob = probesArray[interpProb.probID];

			auto gpuData = interpolationDataGPU.push_back();

			for (int32_t k = 0; k < (int32_t)prob.adresses.size(); k++)
			{
				auto& adr = prob.adresses[k];
				gpuData->targetAdresses[k] = Vector4((float)adr.x, (float)adr.y, (float)adr.z, 0);
			}
			gpuData->targetAdresses[0].w = (float)prob.adresses.size();

			gpuData->pos = Vector4(prob.pos.x, prob.pos.y, prob.pos.z, 0.0f);
			gpuData->offset = Vector4(interpProb.lerpOffset.x, interpProb.lerpOffset.y, interpProb.lerpOffset.z, 0.0f);
		}

		if (interpolationDataGPU.empty())
			continue;

		StructBuf interpolationBuffer = Buffer::CreateStructedBuffer(Render::Device(), (int32_t)interpolationDataGPU.size(), sizeof(ProbInterpolationGPU), false, interpolationDataGPU.data());

		uint32_t groupsCountX = (uint32_t)ceilf(float(interpolationDataGPU.size()) / 128);

		Render::CSSetShaderResources(0, 1, &interpolationBuffer.srv);
		Render::CSSetShaderResources(1, 1, &chunksLookupSRV);
		Render::CSSetShaderResources(2, 1, &bricksLookupSRV);
		Render::CSSetShaderResources(3, 1, &bricksAtlasSRV);
		Render::CSSetConstantBuffers(0, 1, &sampleDataGPU);

		interpolateProbes->BindUAV(bricksTempAtlasUAV);
		interpolateProbes->Dispatch(groupsCountX, 1, 1);
		interpolateProbes->UnbindUAV();

		interpolationBuffer.Release();
		interpolationDataGPU.clear();
	}
}

void GIMgr::CopyBricks()
{
	uint32_t groupsCountX = (uint32_t)ceilf(float(bricksTexX * BRICK_RESOLUTION) / 8);
	uint32_t groupsCountY = (uint32_t)ceilf(float(bricksTexY * BRICK_RESOLUTION) / 8);
	uint32_t groupsCountZ = (uint32_t)ceilf(float(BRICK_RESOLUTION * BRICK_COEF_COUNT) / 4);

	Render::CSSetShaderResources(0, 1, &bricksTempAtlasSRV);
	copyBricks->BindUAV(bricksAtlasUAV);
	copyBricks->Dispatch(groupsCountX, groupsCountY, groupsCountZ);
	copyBricks->UnbindUAV();
}

const Vector3 heighborsDirs[8] = 
{
	{ -1.0f, -1.0f, -1.0f },
	{ 1.0f, -1.0f, -1.0f },
	{ -1.0f, 1.0f, -1.0f },
	{ 1.0f, 1.0f, -1.0f },
	{ -1.0f, -1.0f, 1.0f },
	{ 1.0f, -1.0f, 1.0f },
	{ -1.0f, 1.0f, 1.0f },
	{ 1.0f, 1.0f, 1.0f },
};

Vector3 GIMgr::GetOutterVectorFromNeighbors(uint8_t flags)
{
	Vector3 res = Vector3::Zero;
	for (int32_t i = 0; i < 8; i++)
	{
		if ((flags & (1 << i)) == 0)
		{
			res = heighborsDirs[i];
			res.Normalize();
			res *= voxelSize * PROB_INTERPOLATION_OFFSET_SIZE;
			break;
		}
	}
	return res;
}
/*
void GIMgr::GenerateProbOffsetVectors()
{
	probOffsetsArray.clear();
	probOffsetsArray.create(PROB_CAPTURE_OFFSET_VECTORS);

	float inc = XM_PI * (3.0f - sqrtf(5.0f));
	float off = 2.0f / PROB_CAPTURE_OFFSET_VECTORS;
	for (uint32_t k = 0; k < PROB_CAPTURE_OFFSET_VECTORS; ++k)
	{
		float y = k * off - 1.0f + (off / 2.0f);
		float r = sqrtf(1.0f - y * y);
		float phi = k * inc;

		Vector3 dir = Vector3(cosf(phi) * r, sinf(phi) * r, y);
		dir.Normalize();

		probOffsetsArray.push_back(dir);
	}
}
*/
uint8_t GIMgr::GetNeighborFlag(int32_t i)
{
	switch (i)
	{
	case PROB_FACE_ID_Zm:
		return (0x80 | 0x40 | 0x20 | 0x10);
	case PROB_FACE_ID_Zp:
		return (0x8 | 0x4 | 0x2 | 0x1);
	case PROB_FACE_ID_Ym:
		return (0x80 | 0x40 | 0x8 | 0x4);
	case PROB_FACE_ID_Yp:
		return (0x20 | 0x10 | 0x2 | 0x1);
	case PROB_FACE_ID_Xm:
		return (0x80 | 0x20 | 0x8 | 0x2);
	case PROB_FACE_ID_Xp:
		return (0x40 | 0x10 | 0x4 | 0x1);

	case PROB_SIDE_ID_Ym_Zm:
		return (0x80 | 0x40);
	case PROB_SIDE_ID_Xm_Zm:
		return (0x80 | 0x20);
	case PROB_SIDE_ID_Xp_Zm:
		return (0x40 | 0x10);
	case PROB_SIDE_ID_Yp_Zm:
		return (0x20 | 0x10);
	case PROB_SIDE_ID_Xm_Ym:
		return (0x80 | 0x8);
	case PROB_SIDE_ID_Xp_Ym:
		return (0x40 | 0x4);
	case PROB_SIDE_ID_Xm_Yp:
		return (0x20 | 0x2);
	case PROB_SIDE_ID_Xp_Yp:
		return (0x10 | 0x1);
	case PROB_SIDE_ID_Ym_Zp:
		return (0x8 | 0x4);
	case PROB_SIDE_ID_Xm_Zp:
		return (0x8 | 0x2);
	case PROB_SIDE_ID_Xp_Zp:
		return (0x4 | 0x1);
	case PROB_SIDE_ID_Yp_Zp:
		return (0x2 | 0x1);

	case PROB_CORNER_ID_Xm_Ym_Zm:
		return 0x80;
	case PROB_CORNER_ID_Xp_Ym_Zm:
		return 0x40;
	case PROB_CORNER_ID_Xm_Yp_Zm:
		return 0x20;
	case PROB_CORNER_ID_Xp_Yp_Zm:
		return 0x10;
	case PROB_CORNER_ID_Xm_Ym_Zp:
		return 0x8;
	case PROB_CORNER_ID_Xp_Ym_Zp:
		return 0x4;
	case PROB_CORNER_ID_Xm_Yp_Zp:
		return 0x2;
	case PROB_CORNER_ID_Xp_Yp_Zp:
		return 0x1;
	}
	return 0xff;
}

#ifdef _DEV
const Vector3 octreeColors[OCTREE_DEPTH] =
{
	{ 0.0f, 0.0f, 0.0f },
	{ 1.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 1.0f },
	{ 0.0f, 1.0f, 1.0f },
	{ 0.0f, 1.0f, 0.0f },
	{ 1.0f, 1.0f, 0.0f },
	{ 1.0f, 0.0f, 0.0f }
};

#define PUSH_BOX_TO_LINES \
	lines.push_back(DBGLine(bboxCorners[0], color, bboxCorners[1], color));\
	lines.push_back(DBGLine(bboxCorners[0], color, bboxCorners[3], color));\
	lines.push_back(DBGLine(bboxCorners[0], color, bboxCorners[4], color));\
	lines.push_back(DBGLine(bboxCorners[1], color, bboxCorners[2], color));\
	lines.push_back(DBGLine(bboxCorners[1], color, bboxCorners[5], color));\
	lines.push_back(DBGLine(bboxCorners[2], color, bboxCorners[3], color));\
	lines.push_back(DBGLine(bboxCorners[2], color, bboxCorners[6], color));\
	lines.push_back(DBGLine(bboxCorners[3], color, bboxCorners[7], color));\
	lines.push_back(DBGLine(bboxCorners[4], color, bboxCorners[7], color));\
	lines.push_back(DBGLine(bboxCorners[4], color, bboxCorners[5], color));\
	lines.push_back(DBGLine(bboxCorners[5], color, bboxCorners[6], color));\
	lines.push_back(DBGLine(bboxCorners[6], color, bboxCorners[7], color));

void GIMgr::DebugSetState(DebugState state)
{
	auto dbgDrawer = world->GetDebugDrawer();
	if (!dbgDrawer || bricks.empty())
		return;
	
	switch (state)
	{
	case DebugState::DS_NONE:
		dbgDrawer->DeleteGeometryHandle(debugGeomHandleOctree);
		dbgDrawer->DeleteGeometryHandle(debugGeomHandleProbes);
		dbgDrawer->DeleteGeometryHandle(debugGeomHandleVoxel);
		break;

	case DebugState::DS_OCTREE:
		{
			dbgDrawer->DeleteGeometryHandle(debugGeomHandleOctree);

			Vector3 bboxCorners[8];
			RArray<DBGLine> lines;
			lines.create((bricks.size() + octreeArray.size()) * 12);
			
			for (auto& item : bricks)
			{
				if (item.depth == 0)
					continue;

				const Vector3& color = octreeColors[item.depth];
				item.bbox.GetCorners(bboxCorners);
				PUSH_BOX_TO_LINES
			}

			for (auto& item : octreeArray)
			{
				const Vector3 color(1.0f);
				item.bbox.GetCorners(bboxCorners);
				PUSH_BOX_TO_LINES
			}

			uint32_t vertsCount = (uint32_t)lines.size() * 2;

			debugGeomHandleOctree = dbgDrawer->CreateGeometryHandle(string(DEBUG_MATERIAL_DEPTHCULL), IA_TOPOLOGY::LINELIST, vertsCount, (uint32_t)sizeof(DBGLine) / 2);
			if (debugGeomHandleOctree < 0)
				return;

			dbgDrawer->UpdateGeometry(debugGeomHandleOctree, lines.data(), vertsCount);
		}
		break;

	case DebugState::DS_PROBES:
		{
			dbgDrawer->DeleteGeometryHandle(debugGeomHandleProbes);
			
			struct ProbVertex
			{
				Vector3 pos;
				Vector2 dbg;
				ProbVertex() {}
				ProbVertex(Vector3& p, Vector2& d) : pos(p), dbg(d) {}
			};

			RArray<ProbVertex> points;
			points.create(probesArray.size());
			Vector2 dbg;

			for (auto& item : probesArray)
			{
				dbg.x = item.bake ? 0.0f : 1.0f;

				// for additional debug
				dbg.y = 0.0f;
				//if(!item.bake && item.minDepth > 0 && GetProbLocation(item.brickLastPos) == ProbLocation::PROB_CORNER)
				//	dbg.y = 1.0f;

				points.push_back(ProbVertex(item.pos, dbg));
			}

			debugGeomHandleProbes = dbgDrawer->CreateGeometryHandle(string(DEBUG_MATERIAL_PROBES), IA_TOPOLOGY::POINTLIST, (uint32_t)points.size(), (uint32_t)sizeof(ProbVertex), true);
			if (debugGeomHandleProbes < 0)
				return;
			dbgDrawer->UpdateGeometry(debugGeomHandleProbes, points.data(), (uint32_t)points.size());
		}
		break;

	case DebugState::DS_VOXELS:
	{
		dbgDrawer->DeleteGeometryHandle(debugGeomHandleVoxel);
		
		if (probesArray.empty())
			return;

		int32_t randProbeID;
		int32_t itr = 0;
		do
		{
			randProbeID = int32_t((probesArray.size() - 1) * float(rand()) / float(RAND_MAX));
			itr++;

		} while (!(probesArray[randProbeID].geomCloseProbe && probesArray[randProbeID].bake) && itr < 100000);

		Vector3 voxelsCubeTempPos = probesArray[randProbeID].pos;
		VoxelizedCube<PROB_CAPTURE_OFFSET_VOXEL_RES> voxelsCubeTemp;
		voxelsCubeTemp.Clear();

		AdjustProbPos(voxelsCubeTempPos, voxelsCubeTemp);

		Vector3 bboxCorners[8];
		RArray<DBGLine> lines;
		lines.create(voxelsCubeTemp.resolution * voxelsCubeTemp.resolution * voxelsCubeTemp.resolution * 12);

		Vector3 color;
		int32_t centerVoxel = voxelsCubeTemp.resolution / 2;
		float voxelSizeHalf = voxelSize / voxelsCubeTemp.resolution;

		for (int32_t x = 0; x < (int32_t)voxelsCubeTemp.resolution; x++)
			for (int32_t y = 0; y < (int32_t)voxelsCubeTemp.resolution; y++)
				for (int32_t z = 0; z < (int32_t)voxelsCubeTemp.resolution; z++)
				{
					if (voxelsCubeTemp.voxels[x][y][z] == 0)
						color = Vector3(0, 1.0f, 0);
					else
						color = Vector3(1.0f, 0, 0);
					
					BoundingBox bbox;
					bbox.Center = voxelsCubeTempPos;
					bbox.Center.x += (x - centerVoxel) * voxelSizeHalf * 2.0f;
					bbox.Center.y += (y - centerVoxel) * voxelSizeHalf * 2.0f;
					bbox.Center.z += (z - centerVoxel) * voxelSizeHalf * 2.0f;

					bbox.Extents = Vector3(voxelSizeHalf);

					bbox.GetCorners(bboxCorners);
					PUSH_BOX_TO_LINES
				}

		uint32_t vertsCount = (uint32_t)lines.size() * 2;

		debugGeomHandleVoxel = dbgDrawer->CreateGeometryHandle(string(DEBUG_MATERIAL_DEPTHCULL), IA_TOPOLOGY::LINELIST, vertsCount, (uint32_t)sizeof(DBGLine) / 2);
		if (debugGeomHandleVoxel < 0)
			return;

		dbgDrawer->UpdateGeometry(debugGeomHandleVoxel, lines.data(), vertsCount);
	}
	break;
	}
}
#endif