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

	voxelSize = DEFAULT_OCTREE_VOXEL_SIZE;
	chunkSize = powf(2.0f, (float)OCTREE_DEPTH) * DEFAULT_OCTREE_VOXEL_SIZE;

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

bool GIMgr::BuildVoxelOctree()
{
	octreeArray.destroy();
	chunks.destroy();
	bricks.destroy();
	probesArray.destroy();
	probesLookup.clear();
	
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

				octree->lookup = new uint32_t**[octree->lookupRes];
				for (int32_t x = 0; x < octree->lookupRes; x++)
				{
					octree->lookup[x] = new uint32_t*[octree->lookupRes];
					for (int32_t y = 0; y < octree->lookupRes; y++)
					{
						octree->lookup[x][y] = new uint32_t[octree->lookupRes];
						memset(octree->lookup[x][y], (int)0xffffffff, sizeof(uint32_t) * octree->lookupRes);
					}
				}

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

	LOG_GOOD("Bricks count: %i", (int32_t)bricks.size());
	LOG_GOOD("Probes count: %i", (int32_t)probesArray.size());

#ifndef _DEV
	octreeArray.destroy();
	chunks.destroy();
	bricks.destroy();
	probesArray.destroy();
	probesLookup.clear();
#endif

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
		newBrick.levelInv = octreeDepth;
		
		Vector3 bboxMin = octreeHelper * (bbox.Center - octreeCorner - bbox.Extents);
		Vector3 bboxMax = octreeHelper * (bbox.Center - octreeCorner + bbox.Extents);

		for (int32_t x = (int32_t)roundf(bboxMin.x); x < (int32_t)roundf(bboxMax.x); x++)
		{
			for (int32_t y = (int32_t)roundf(bboxMin.y); y < (int32_t)roundf(bboxMax.y); y++)
			{
				for (int32_t z = (int32_t)roundf(bboxMin.z); z < (int32_t)roundf(bboxMax.z); z++)
				{
					uint32_t& lookupNode = octree.lookup[x][y][z];
					if (lookupNode == 0xffffffff)
					{
						lookupNode = SetLookupNode(brickID, octreeDepth);
					}
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

				prob.interpolated = false;
				prob.pos = probPos;

				probesLookup.insert(make_pair(posForHash, probID));
				newBrick.probes[i] = probID;
			}
			else
			{
				newBrick.probes[i] = probIt->second;
			}
		}
	}
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
				if (item.levelInv >= OCTREE_DEPTH - 1)
					continue;

				const Vector3& color = octreeColors[item.levelInv];
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

		break;
	}
}
#endif