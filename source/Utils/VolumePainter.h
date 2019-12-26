#pragma once

#include "Common.h"
#include "Compute.h"

#define COMPUTE_IMPORT_TEXTURE PATH_SHADERS "volume/import_texture#Copy#"
#define COMPUTE_DRAW_BRUSH PATH_SHADERS "volume/draw_brush#Draw#"

#define COPMUTE_TREADS_X 8
#define COPMUTE_TREADS_Y 4
#define COPMUTE_TREADS_Z 2

#define VOXEL_DATA_SIZE 4
#define HISTORY_LENGTH 128

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
			int32_t minX, minY, minZ;
			int32_t maxX, maxY, maxZ;
			int32_t resX, resY, resZ;

			uint16_t* data;

			inline int32_t GetVoxelsSize()
			{
				return resX * resY * resZ * VOXEL_DATA_SIZE;
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
				minX = (int32_t)roundf(minCorner.x);
				minY = (int32_t)roundf(minCorner.y);
				minZ = (int32_t)roundf(minCorner.z);
				maxX = (int32_t)roundf(maxCorner.x);
				maxY = (int32_t)roundf(maxCorner.y);
				maxZ = (int32_t)roundf(maxCorner.z);

				resX = maxX - minX;
				resY = maxY - minY;
				resZ = maxZ - minZ;

				if (resX <= 0 || resY <= 0 || resZ <= 0)
					ERR("VolumeArea wrong size");

				int32_t voxelsSize = GetVoxelsSize();
				data = new uint16_t[voxelsSize];
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

		bool Init(uint32_t width, uint32_t height, uint32_t depth, uint32_t historyMaxSize);

		luaSRV GetSRV()	{ return luaSRV(volumeTextureSRV); }

		void ImportTexture(string textureName);
		void ExportTexture(string textureName, int32_t packingType, int32_t storageType);
		void DrawBrush(Vector3& prevPosition, Vector3& position, float radius, Vector4& colorOpacity, float hardness);

		void PushDifference(Vector3& minCorner, Vector3& maxCorner);
		void StoreDifference(VolumeDiff& area, uint8_t* difference);

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

		ID3D11Buffer* brushInfoBuffer;
		ID3D11Buffer* volumeInfoBuffer;

		//uint8_t* volumeData;

		RDeque<VolumeDiff> history;
		uint32_t historySize;
		uint32_t historyMaxSize;
	};
}