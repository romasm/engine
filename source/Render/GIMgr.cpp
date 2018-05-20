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

	chunksLookup = nullptr;
	chunksLookupSRV = nullptr;

	bricksLookup = nullptr;
	bricksLookupSRV = nullptr;

	bricksTexX = 0;
	bricksTexY = 0;

	sgVolume = TexMgr::nullres;
	sampleDataGPU = nullptr;

	voxelSize = DEFAULT_OCTREE_VOXEL_SIZE;
	lookupMaxSize = (uint32_t)powf(2.0f, float(OCTREE_DEPTH - 1));
	chunkSize = (float)lookupMaxSize * 2.0f * DEFAULT_OCTREE_VOXEL_SIZE;

#ifdef _DEV
	debugGeomHandle = -1;
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
	if(dbg)
		dbg->DeleteGeometryHandle(debugGeomHandle);
#endif

	DeleteResources();

	TEXTURE_DROP(sgVolume);
	_RELEASE(sampleDataGPU);
}

bool GIMgr::InitBuffers()
{
	sampleDataGPU = Buffer::CreateConstantBuffer(Render::Device(), sizeof(GISampleData), true);
	
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
	return TEXTURE_GETPTR(sgVolume);
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
}

bool GIMgr::CompareOctrees(Octree& first, Octree& second)
{
	return first.lookupRes < second.lookupRes;
}

void GIMgr::SwapOctrees(Octree* first, Octree* second, RArray<RArray<RArray<int32_t>>>* arr)
{
	swap((*arr)[first->parentChunk.x][first->parentChunk.y][first->parentChunk.z], 
		(*arr)[second->parentChunk.x][second->parentChunk.y][second->parentChunk.z]);
	swap(*first, *second);
}

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

#define PROB_MIDDLE_ID 13

#define PROB_FACE_ID_0 4
#define PROB_FACE_ID_1 10
#define PROB_FACE_ID_2 12
#define PROB_FACE_ID_3 14
#define PROB_FACE_ID_4 16
#define PROB_FACE_ID_5 22

bool GIMgr::RecreateResources()
{
	DeleteResources();

	// bricks atlas
	// TODO: DXGI_FORMAT_R9G9B9E5_SHAREDEXP ???
	const DXGI_FORMAT formatBricks = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;

	D3D11_TEXTURE3D_DESC volumeDesc;
	ZeroMemory(&volumeDesc, sizeof(volumeDesc));
	volumeDesc.Width = bricksTexX * BKICK_RESOLUTION;
	volumeDesc.Height = bricksTexY * BKICK_RESOLUTION;
	volumeDesc.Depth = BKICK_RESOLUTION * BKICK_F4_COUNT;
	volumeDesc.MipLevels = 1;
	volumeDesc.Usage = D3D11_USAGE_DEFAULT;
	volumeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	volumeDesc.CPUAccessFlags = 0;
	volumeDesc.MiscFlags = 0;
	volumeDesc.Format = formatBricks;
	if (FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &bricksAtlas)))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC volumeSRVDesc;
	ZeroMemory(&volumeSRVDesc, sizeof(volumeSRVDesc));
	volumeSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	volumeSRVDesc.Texture3D.MipLevels = -1;
	volumeSRVDesc.Texture3D.MostDetailedMip = 0;
	volumeSRVDesc.Format = formatBricks;
	if (FAILED(Render::CreateShaderResourceView(bricksAtlas, &volumeSRVDesc, &bricksAtlasSRV)))
		return false;

	D3D11_UNORDERED_ACCESS_VIEW_DESC volumeUAVDesc;
	ZeroMemory(&volumeUAVDesc, sizeof(volumeUAVDesc));
	volumeUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	volumeUAVDesc.Texture3D.MipSlice = 0;
	volumeUAVDesc.Texture3D.WSize = BKICK_RESOLUTION * BKICK_F4_COUNT;
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
	
	LOG_GOOD("Chunks lookup size: %i", chunksX * chunksY * chunksZ * sizeof(Vector4Uint16));
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

	return true;
}

bool GIMgr::BuildVoxelOctree()
{
	octreeArray.destroy();
	chunks.destroy();
	bricks.destroy();
	probesArray.destroy();
	
	// collect static geometry
	auto transformSys = world->GetTransformSystem();
	auto visibilitySys = world->GetVisibilitySystem();
	auto meshSys = world->GetStaticMeshSystem();
	
	BoundingBox worldBBraw;
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

		BoundingBox::CreateMerged(worldBBraw, worldBBraw, newItem.bbox);
	}

	// build chunk array
	// TODO: configurate
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

				if (SceneBoxIntersect(staticScene, chunkBox))
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

		ProcessOctreeBranch(octree, staticScene, octree.bbox, octree.depth - 1, octreeHelper, octreeCorner);
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

		for (uint32_t k = 0; k < BKICK_RESOLUTION * BKICK_RESOLUTION * BKICK_RESOLUTION; k++)
		{
			Prob& prob = probesArray[bricks[i].probes[k]];
			prob.adresses.push_back(Vector3Uint32(x + probesOffset[k][0], y + probesOffset[k][1], probesOffset[k][2]));

			// copies check
			if (prob.maxDepth == bricks[i].depth)
			{
				// TODO: force outter prob to bake (inverse depth?)

				if (k == PROB_MIDDLE_ID) // center prob, always bake
				{
					prob.bake = true;
				}
				else
				{
					if (k % 2 != 0) // side prob, 4 copies for bake
					{
						prob.bake = (prob.copyCount >= 4);
					}
					else
					{
						if (k == PROB_FACE_ID_0 || k == PROB_FACE_ID_1 || k == PROB_FACE_ID_2 ||
							k == PROB_FACE_ID_3 || k == PROB_FACE_ID_4 || k == PROB_FACE_ID_5) // face prob, 2 copies for bake
						{
							prob.bake = (prob.copyCount >= 2);
						}
						else // corner prob, 8 copies for bake
						{
							prob.bake = (prob.copyCount >= 8);
						}
					}
				}
			}
		}
	}

	if (!RecreateResources())
	{
		ERR("Cant create resources for GI");
		return false;
	}

	bricksLinks.clear();

	// post process probes
	for (auto& prob : probesArray)
	{
		if(prob.bake)
			prob.pos = AdjustProbPos(prob.pos);

		
		// TODO: find interpalation locations
	}

	// TEMP baking
	const int32_t captureResolution = 64;
	const DXGI_FORMAT formatProb = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;

	if (!world->BeginCaptureProb(captureResolution, formatProb, false))
	{
		ERR("Cant init prob capture");
		return false;
	}

	ID3D11ShaderResourceView* probSRV = world->GetCaptureProbSRV();
	if (!probSRV)
		return false;

	int32_t bakedCount = 0;
	for (auto& prob : probesArray)
	{
		if (!prob.bake)
			continue;

		world->CaptureProb( Matrix::CreateTranslation(prob.pos), PROB_CAPTURE_NEARCLIP, PROB_CAPTURE_FARCLIP, false );

		bakedCount++;
		if (bakedCount % 100 == 0)
			DBG_SHORT("Baked: %i", bakedCount);
	}

	world->EndCaptureProb();

#ifndef _DEV
	octreeArray.destroy();
	chunks.destroy();
	bricks.destroy();
	probesArray.destroy();
#endif
	
	return true;
}

bool GIMgr::SceneBoxIntersect(DArray<VoxelizeSceneItem>& staticScene, BoundingBox& bbox)
{
	BoundingBox extendedBBox = bbox;
	extendedBBox.Extents = extendedBBox.Extents + Vector3(voxelSize * OCTREE_INTERSECT_TOLERANCE);

	for (auto& i : staticScene)
	{
		if (i.bbox.Intersects(extendedBBox))
		{
			auto mesh = MeshMgr::GetResourcePtr(i.meshID);
			if (!mesh)
				continue;

			if (MeshLoader::MeshBoxOverlap(mesh, i.transform, extendedBBox))
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

void GIMgr::ProcessOctreeBranch(Octree& octree, DArray<VoxelizeSceneItem>& staticScene, BoundingBox& bbox, int32_t octreeDepth, Vector3& octreeHelper, Vector3& octreeCorner)
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

			if (SceneBoxIntersect(staticScene, leafBox))
			{
				ProcessOctreeBranch(octree, staticScene, leafBox, octreeDepth - 1, octreeHelper, octreeCorner);
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
		newBrick.depth = octreeDepth;
		
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
		for (int32_t i = 0; i < BKICK_RESOLUTION * BKICK_RESOLUTION * BKICK_RESOLUTION; i++)
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
				prob.copyCount = 1;
				prob.maxDepth = octreeDepth;

				probesLookup.insert(make_pair(posForHash, probID));
				newBrick.probes[i] = probID;
			}
			else
			{
				newBrick.probes[i] = probIt->second;

				uint8_t& probMaxDepth = probesArray[probIt->second].maxDepth;
				if (probMaxDepth == (uint8_t)octreeDepth)
				{
					probesArray[probIt->second].copyCount++;
				}
				else if (probMaxDepth < (uint8_t)octreeDepth)
				{
					probMaxDepth = (uint8_t)octreeDepth;
					probesArray[probIt->second].copyCount = 1;
				}		
			}
		}
	}
}

Vector3 GIMgr::AdjustProbPos(Vector3& pos)
{
	// TODO
	return pos;
}

#ifdef _DEV
const Vector3 octreeColors[OCTREE_DEPTH] =
{
	{ 1.0f, 0.0f, 0.0f },
	{ 1.0f, 1.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f },
	{ 0.0f, 1.0f, 1.0f },
	{ 0.0f, 0.0f, 1.0f },
	{ 1.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 0.0f }
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
	auto dbg = world->GetDebugDrawer();
	if (!dbg || bricks.empty())
		return;

	dbg->DeleteGeometryHandle(debugGeomHandle);

	switch (state)
	{
	case DebugState::DS_NONE:
		break;

	case DebugState::DS_OCTREE:
		{
			Vector3 bboxCorners[8];
			RArray<DBGLine> lines;
			lines.create((bricks.size() + octreeArray.size()) * 12);
			
			for (auto& item : bricks)
			{
				if (item.depth >= OCTREE_DEPTH - 1)
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

			debugGeomHandle = dbg->CreateGeometryHandle(string(DEBUG_MATERIAL_DEPTHCULL), IA_TOPOLOGY::LINELIST, vertsCount, (uint32_t)sizeof(DBGLine) / 2);
			if (debugGeomHandle < 0)
				return;

			dbg->UpdateGeometry(debugGeomHandle, lines.data(), vertsCount);
		}
		break;

	case DebugState::DS_PROBES:
		{
			RArray<Vector3> points;
			points.create(probesArray.size());
			for (auto& item : probesArray)
			{
				if(item.bake)
					points.push_back(item.pos);
			}

			debugGeomHandle = dbg->CreateGeometryHandle(string(DEBUG_MATERIAL_PROBES), IA_TOPOLOGY::POINTLIST, (uint32_t)points.size(), (uint32_t)sizeof(Vector3), true);
			if (debugGeomHandle < 0)
				return;

			dbg->UpdateGeometry(debugGeomHandle, points.data(), (uint32_t)points.size());
		}
		break;
	}
}
#endif