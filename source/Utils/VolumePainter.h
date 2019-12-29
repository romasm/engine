#pragma once

#include "Common.h"
#include "Compute.h"

#define COMPUTE_IMPORT_TEXTURE PATH_SHADERS "volume/import_texture#Copy#"
#define COMPUTE_DRAW_BRUSH PATH_SHADERS "volume/draw_brush#Draw#"
#define COMPUTE_HISTORY_STEP_BACK PATH_SHADERS "volume/history_steps#StepBack#"
#define COMPUTE_HISTORY_STEP_FORWARD PATH_SHADERS "volume/history_steps#StepForward#"

#define COPMUTE_TREADS_X 8
#define COPMUTE_TREADS_Y 4
#define COPMUTE_TREADS_Z 2

#define VOXEL_DIFF_SIZE 8
#define HISTORY_LENGTH 256

namespace EngineCore
{
	class VolumePainter
	{
		struct BrushInfo
		{
			Vector3 position;
			float radius;

			Vector4 colorOpacity;

			float hardness;
			Vector3 prevPosition;

			float erase;
			Vector3 padding;
		};

		struct VolumeInfo
		{
			Vector3 minCorner;
			float _padding0;

			Vector3 sizeInv;
			float _padding1;

			Vector3 size;
			float _padding2;
		};
		
		struct VolumeDiff
		{
#define AREA_ALIGMENT_X (64 / VOXEL_DIFF_SIZE)
#define AREA_ALIGMENT_Y 2

			int32_t minX, minY, minZ;
			int32_t maxX, maxY, maxZ;
			int32_t resX, resY, resZ;

			uint8_t* data;

			inline int64_t GetVoxelsSize()
			{
				return resX * resY * resZ * VOXEL_DIFF_SIZE;
			}

			inline D3D11_BOX GetD3DBox()
			{
				D3D11_BOX volumeBox;
				volumeBox.left = minX;
				volumeBox.top = minY;
				volumeBox.front = minZ;
				volumeBox.right = maxX;
				volumeBox.bottom = maxY;
				volumeBox.back = maxZ;
				return volumeBox;
			}

			VolumeDiff()
			{
				minX = minY = minZ = 0;
				maxX = maxY = maxZ = 0;
				resX = resY = resZ = 0;
				data = nullptr;
			}

			void Init(const Vector3& minCorner, const Vector3& maxCorner)
			{
				Clear();

				minX = (int32_t)floor(minCorner.x / AREA_ALIGMENT_X) * AREA_ALIGMENT_X;
				minY = (int32_t)floor(minCorner.y / AREA_ALIGMENT_Y) * AREA_ALIGMENT_Y;
				minZ = (int32_t)floor(minCorner.z);

				maxX = (int32_t)ceil(maxCorner.x / AREA_ALIGMENT_X) * AREA_ALIGMENT_X;
				maxY = (int32_t)ceil(maxCorner.y / AREA_ALIGMENT_Y) * AREA_ALIGMENT_Y;
				maxZ = (int32_t)ceil(maxCorner.z);

				resX = maxX - minX;
				resY = maxY - minY;
				resZ = maxZ - minZ;

				if (resX <= 0 || resY <= 0 || resZ <= 0)
					ERR("VolumeArea wrong size");

				int64_t voxelsSize = GetVoxelsSize();
				data = new uint8_t[voxelsSize];
			}

			void Clear()
			{
				_DELETE_ARRAY(data);

				minX = minY = minZ = 0;
				maxX = maxY = maxZ = 0;
				resX = resY = resZ = 0;
			}

			~VolumeDiff()
			{
				Clear();
			}
		};

	public:

		VolumePainter();
		~VolumePainter();

		bool Init(uint32_t width, uint32_t height, uint32_t depth, uint32_t historyBufferSizeMB);

		luaSRV GetSRV()	{ return luaSRV(volumeTextureSRV); }

		void ImportTexture(string textureName);
		void ExportTexture(string textureName, int32_t packingType, int32_t storageType);
		void DrawBrush(Vector3& prevPosition, Vector3& position, float radius, Vector4& colorOpacity, float hardness, bool erase);

		void PushDifference(Vector3& minCorner, Vector3& maxCorner);
		void HistoryStepBack();
		void HistoryStepForward();

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<VolumePainter>("VolumePainter")
				.addConstructor<void(*)(void)>()
				.addFunction("Init", &VolumePainter::Init)
				.addFunction("GetSRV", &VolumePainter::GetSRV)
				.addFunction("ImportTexture", &VolumePainter::ImportTexture)
				.addFunction("ExportTexture", &VolumePainter::ExportTexture)
				.addFunction("DrawBrush", &VolumePainter::DrawBrush)
				.addFunction("PushDifference", &VolumePainter::PushDifference)
				.addFunction("HistoryStepBack", &VolumePainter::HistoryStepBack)
				.addFunction("HistoryStepForward", &VolumePainter::HistoryStepForward)
				.endClass();
		}

	private:

		uint32_t volumeResolutionX;
		uint32_t volumeResolutionY;
		uint32_t volumeResolutionZ;

		ID3D11Texture3D* volumeTexture;
		ID3D11UnorderedAccessView* volumeTextureUAV;
		ID3D11ShaderResourceView* volumeTextureSRV;

		ID3D11Texture3D* volumeDifference;
		ID3D11UnorderedAccessView* volumeDifferenceUAV;

		Compute* computeImportTexture;
		Compute* computeDrawBrush;

		Compute* computeHistoryStepBack;
		Compute* computeHistoryStepForward;

		ID3D11Buffer* brushInfoBuffer;
		ID3D11Buffer* volumeInfoBuffer;
		
		RDeque<VolumeDiff> history;
		int64_t historySize;
		int64_t historyMaxSize;
		int32_t historyMark;
	};
}