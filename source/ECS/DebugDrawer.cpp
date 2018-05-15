#include "stdafx.h"

#include "DebugDrawer.h"

using namespace EngineCore;

DebugDrawer::DebugDrawer()
{
	dbgLines.create(DEBUG_ARRAY_SIZE);
	lineBuffer = Buffer::CreateVertexBuffer(DEVICE, DEBUG_ARRAY_SIZE * sizeof(DBGLine), true, nullptr);
	lineMat = MaterialMgr::Get()->GetMaterial(string(DEBUG_MATERIAL));
	lineCount = 0;

	dbgLinesDepthCull.create(DEBUG_ARRAY_SIZE);
	lineBufferDepthCull = Buffer::CreateVertexBuffer(DEVICE, DEBUG_ARRAY_SIZE * sizeof(DBGLine), true, nullptr);
	lineMatDepthCull = MaterialMgr::Get()->GetMaterial(string(DEBUG_MATERIAL_DEPTHCULL));
	lineCountDepthCull = 0;

	vertexGeometry.create(DEBUG_GEOM_MAXCOUNT);

	vertexGeometryLookup.create(DEBUG_GEOM_MAXCOUNT);
	vertexGeometryLookup.resize(DEBUG_GEOM_MAXCOUNT);
	vertexGeometryLookup.assign(-1);

	vertexGeometryFreeId.create(DEBUG_GEOM_MAXCOUNT);
	vertexGeometryFreeId.resize(DEBUG_GEOM_MAXCOUNT);
	for (int32_t i = 0; i < DEBUG_GEOM_MAXCOUNT; i++)
		vertexGeometryFreeId[i] = i;
}

DebugDrawer::~DebugDrawer()
{
	_RELEASE(lineBuffer);
	if (lineMat)
	{
		MaterialMgr::Get()->DeleteMaterial(lineMat->GetName());
		lineMat = nullptr;
	}

	_RELEASE(lineBufferDepthCull);
	if (lineMatDepthCull)
	{
		MaterialMgr::Get()->DeleteMaterial(lineMatDepthCull->GetName());
		lineMatDepthCull = nullptr;
	}

	for (auto& i : vertexGeometry)
	{
		_RELEASE(i.verts);
		if (i.mat)
		{
			MaterialMgr::Get()->DeleteMaterial(i.mat->GetName());
			i.mat = nullptr;
		}
	}
}

void DebugDrawer::PushLine(Vector3& A, Vector3& B, Vector3& colorA, Vector3& colorB, bool depthCull)
{
	DBGLine* line;

	if (depthCull)
		line = dbgLinesDepthCull.push_back();
	else
		line = dbgLines.push_back();

	if (!line)
		return;

	line->A = A;
	line->colorA = colorA;
	line->B = B;
	line->colorB = colorB;
}

void DebugDrawer::PushBoundingBox(BoundingBox& box, Vector3& color, bool depthCull)
{
	box.GetCorners(bboxCorners);
	PushLine(bboxCorners[0], bboxCorners[1], color, depthCull);
	PushLine(bboxCorners[0], bboxCorners[3], color, depthCull);
	PushLine(bboxCorners[0], bboxCorners[4], color, depthCull);
	PushLine(bboxCorners[1], bboxCorners[2], color, depthCull);
	PushLine(bboxCorners[1], bboxCorners[5], color, depthCull);
	PushLine(bboxCorners[2], bboxCorners[3], color, depthCull);
	PushLine(bboxCorners[2], bboxCorners[6], color, depthCull);
	PushLine(bboxCorners[3], bboxCorners[7], color, depthCull);
	PushLine(bboxCorners[4], bboxCorners[7], color, depthCull);
	PushLine(bboxCorners[4], bboxCorners[5], color, depthCull);
	PushLine(bboxCorners[5], bboxCorners[6], color, depthCull);
	PushLine(bboxCorners[6], bboxCorners[7], color, depthCull);
}

int32_t DebugDrawer::CreateGeometryHandle(string& matName, IA_TOPOLOGY topo, uint32_t maxPrimCount, uint32_t vertSize)
{
	if (vertexGeometryFreeId.empty())
		return -1;

	uint32_t handleId = vertexGeometryFreeId.front();
	vertexGeometryFreeId.pop_front();

	vertexGeometryLookup[handleId] = (int32_t)vertexGeometry.size();

	auto handle = vertexGeometry.push_back();
	handle->mat = MaterialMgr::Get()->GetMaterial(matName);
	handle->topo = topo;
	handle->vertCount = 0;
	handle->vertSize = vertSize;
	handle->verts = Buffer::CreateVertexBuffer(DEVICE, maxPrimCount * vertSize, true, nullptr);
	handle->lookup = handleId;

	return handleId;
}

void DebugDrawer::UpdateGeometry(int32_t handleId, void* verts, uint32_t count)
{
	int32_t& lookupId = vertexGeometryLookup[handleId];
	if (lookupId < 0)
		return;

	DebugGeomHandle& handle = vertexGeometry[lookupId];
	handle.vertCount = count;

	Render::UpdateDynamicResource(handle.verts, verts, count * handle.vertSize);
}

void DebugDrawer::DeleteGeometryHandle(int32_t handleId)
{
	if (handleId < 0)
		return;

	int32_t& lookupId = vertexGeometryLookup[handleId];
	if (lookupId < 0)
		return;

	DebugGeomHandle& handle = vertexGeometry[lookupId];

	_RELEASE(handle.verts);
	if (handle.mat)
	{
		MaterialMgr::Get()->DeleteMaterial(handle.mat->GetName());
		handle.mat = nullptr;
	}

	vertexGeometry.erase_and_pop_back(lookupId);
	if (!vertexGeometry.empty())
	{
		vertexGeometryLookup[vertexGeometry[lookupId].lookup] = lookupId;
	}

	vertexGeometryFreeId.push_back(lookupId);
	lookupId = -1;
}

void DebugDrawer::Prepare()
{
	if (!dbgLines.empty())
	{
		Render::UpdateDynamicResource(lineBuffer, dbgLines.data(), dbgLines.size() * sizeof(DBGLine));
		lineCount = (uint32_t)dbgLines.size();
		dbgLines.resize(0);
	}

	if (!dbgLinesDepthCull.empty())
	{
		Render::UpdateDynamicResource(lineBufferDepthCull, dbgLinesDepthCull.data(), dbgLinesDepthCull.size() * sizeof(DBGLine));
		lineCountDepthCull = (uint32_t)dbgLinesDepthCull.size();
		dbgLinesDepthCull.resize(0);
	}
}

void DebugDrawer::Drop()
{
	lineCount = 0;
	dbgLines.resize(0);

	lineCountDepthCull = 0;
	dbgLinesDepthCull.resize(0);
}

void DebugDrawer::Render()
{
	const uint32_t stride = sizeof(DBGLine) / 2;
	const uint32_t offset = 0;

	if (lineCount > 0)
	{
		Render::Context()->IASetVertexBuffers(0, 1, &lineBuffer, &stride, &offset);

		lineMat->Set();
		Render::SetTopology(IA_TOPOLOGY::LINELIST);
		Render::Context()->Draw(lineCount * 2, 0);
	}

	if (lineCountDepthCull > 0)
	{
		Render::Context()->IASetVertexBuffers(0, 1, &lineBufferDepthCull, &stride, &offset);

		lineMatDepthCull->Set();
		Render::SetTopology(IA_TOPOLOGY::LINELIST);
		Render::Context()->Draw(lineCountDepthCull * 2, 0);
	}
	
	for (auto& i : vertexGeometry)
	{
		Render::Context()->IASetVertexBuffers(0, 1, &i.verts, &i.vertSize, &offset);
		i.mat->Set();
		Render::SetTopology(i.topo);
		Render::Context()->Draw(i.vertCount, 0);
	}
}