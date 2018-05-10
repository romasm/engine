#pragma once

#include "Common.h"
#include "Render.h"
#include "MaterialMgr.h"

#define DEBUG_ARRAY_SIZE 512000
#define DEBUG_MATERIAL "$" PATH_SHADERS "objects/editor/debug_lines"
#define DEBUG_MATERIAL_DEPTHCULL "$" PATH_SHADERS "objects/editor/debug_lines_cull"

#define DEBUG_GEOM_MAXCOUNT 128

namespace EngineCore
{
	struct DBGLine
	{
		Vector3 A;
		Vector3 colorA;
		Vector3 B;
		Vector3 colorB;

		DBGLine() {}
		DBGLine(const Vector3& aa, const Vector3& colAA, const Vector3& bb, const Vector3& colBB) : A(aa), colorA(colAA), B(bb), colorB(colBB) {}
	};

	class DebugDrawer
	{
		struct DebugGeomHandle
		{
			ID3D11Buffer* verts;
			Material* mat;
			IA_TOPOLOGY topo;
			uint32_t vertCount;
			uint32_t vertSize;
			int32_t lookup;
		};

	public:
		DebugDrawer();
		~DebugDrawer();

		void PushLine(Vector3& A, Vector3& B, Vector3& colorA, Vector3& colorB, bool depthCull = false);
		inline void PushLine(Vector3& A, Vector3& B, Vector3& color, bool depthCull = false)
		{
			PushLine(A, B, color, color, depthCull);
		}

		void PushBoundingBox(BoundingBox& box, Vector3& color, bool depthCull = false);

		int32_t CreateGeometryHandle(string& matName, IA_TOPOLOGY topo, uint32_t maxPrimCount, uint32_t vertSize);
		void UpdateGeometry(int32_t handleId, void* verts, uint32_t count);
		void DeleteGeometryHandle(int32_t handleId);

		void Prepare();
		void Drop();
		void Render();

	private:
		RArray<DBGLine> dbgLines;
		uint32_t lineCount;

		RArray<DBGLine> dbgLinesDepthCull;
		uint32_t lineCountDepthCull;

		ID3D11Buffer* lineBuffer;
		Material* lineMat;

		ID3D11Buffer* lineBufferDepthCull;
		Material* lineMatDepthCull;

		Vector3 bboxCorners[8];

		RArray<DebugGeomHandle> vertexGeometry;
		RDeque<int32_t> vertexGeometryFreeId;
		RDeque<int32_t> vertexGeometryLookup;
	};
}