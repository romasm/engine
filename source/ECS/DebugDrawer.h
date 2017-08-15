#pragma once

#include "Common.h"
#include "Render.h"
#include "MaterialMgr.h"

#define DEBUG_ARRAY_SIZE 32768
#define DEBUG_MATERIAL "$" PATH_SHADERS "objects/editor/debug_lines"

namespace EngineCore
{
	struct DBGLine
	{
		Vector3 A;
		Vector3 colorA;
		Vector3 B;
		Vector3 colorB;
	};
	
	class DebugDrawer
	{
	public:
		DebugDrawer()
		{
			dbgLines.create(DEBUG_ARRAY_SIZE);
			lineBuffer = Buffer::CreateVertexBuffer(DEVICE, DEBUG_ARRAY_SIZE * sizeof(DBGLine), true, nullptr);
			lineMat = MATERIAL_S(DEBUG_MATERIAL);
			lineCount = 0;
		}

		~DebugDrawer()
		{
			_RELEASE(lineBuffer);
			MATERIAL_PTR_DROP(lineMat);
		}

		void PushLine(Vector3& A, Vector3& B, Vector3& colorA, Vector3& colorB)
		{
			auto line = dbgLines.push_back();
			if(!line)
				return;
			line->A = A;
			line->B = B;
			line->colorA = colorA;
			line->colorB = colorB;
		}

		void Prepare()
		{
			if( dbgLines.empty() )
			{
				lineCount = 0;
				return;
			}

			Render::UpdateDynamicResource(lineBuffer, dbgLines.data(), dbgLines.size() * sizeof(DBGLine));
			lineCount = (uint32_t)dbgLines.size();
			dbgLines.resize(0);
		}

		void Render()
		{
			if( lineCount == 0 )
				return;

			const uint32_t stride = sizeof(DBGLine) / 2;
			const uint32_t offset = 0;
			Render::Context()->IASetVertexBuffers(0, 1, &lineBuffer, &stride, &offset);

			lineMat->Set();
			Render::SetTopology(IA_TOPOLOGY::LINELIST);
			Render::Context()->Draw(lineCount * 2, 0);

			lineCount = 0;
		}

	private:
		RArray<DBGLine> dbgLines;
		uint32_t lineCount;

		ID3D11Buffer* lineBuffer;
		Material* lineMat;
	};
}