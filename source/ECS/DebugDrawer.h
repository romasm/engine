#pragma once

#include "Common.h"
#include "Render.h"
#include "MaterialMgr.h"

#define DEBUG_ARRAY_SIZE 512000
#define DEBUG_MATERIAL "$" PATH_SHADERS "objects/editor/debug_lines"
#define DEBUG_MATERIAL_DEPTHCULL "$" PATH_SHADERS "objects/editor/debug_lines_cull"

namespace EngineCore
{
	struct DBGLine
	{
		Vector3 A;
		Vector3 colorA;
		Vector3 B;
		Vector3 colorB;
	};
	
	// TODO: extent for static data
	class DebugDrawer
	{
	public:
		DebugDrawer()
		{
			dbgLines.create(DEBUG_ARRAY_SIZE);
			lineBuffer = Buffer::CreateVertexBuffer(DEVICE, DEBUG_ARRAY_SIZE * sizeof(DBGLine), true, nullptr);
			lineMat = MATERIAL_S(DEBUG_MATERIAL);
			lineCount = 0;

			dbgLinesDepthCull.create(DEBUG_ARRAY_SIZE);
			lineBufferDepthCull = Buffer::CreateVertexBuffer(DEVICE, DEBUG_ARRAY_SIZE * sizeof(DBGLine), true, nullptr);
			lineMatDepthCull = MATERIAL_S(DEBUG_MATERIAL_DEPTHCULL);
			lineCountDepthCull = 0;
		}

		~DebugDrawer()
		{
			_RELEASE(lineBuffer);
			MATERIAL_PTR_DROP(lineMat);

			_RELEASE(lineBufferDepthCull);
			MATERIAL_PTR_DROP(lineMatDepthCull);
		}

		void PushLine(Vector3& A, Vector3& B, Vector3& colorA, Vector3& colorB, bool depthCull = false)
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

		void PushLine(Vector3& A, Vector3& B, Vector3& color, bool depthCull = false)
		{
			PushLine(A, B, color, color, depthCull);
		}

		void PushBoundingBox(BoundingBox& box, Vector3& color, bool depthCull = false)
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

		void Prepare()
		{
			if( !dbgLines.empty() )
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

		void Drop()
		{
			lineCount = 0;
			dbgLines.resize(0);

			lineCountDepthCull = 0;
			dbgLinesDepthCull.resize(0);
		}

		void Render()
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
		}

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
	};
}