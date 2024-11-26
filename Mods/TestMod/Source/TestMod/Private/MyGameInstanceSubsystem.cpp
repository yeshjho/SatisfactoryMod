// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstanceSubsystem.h"

#include "RHICommandList.h"
#include "Rendering/Texture2DResource.h"


void UMyGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

    TextureTotalPixels = TextureSideSize * TextureSideSize;

    // Get Total Bytes of Texture - Each pixel has 4 bytes for RGBA
    TextureDataSize = TextureTotalPixels * 4;
    TextureDataSqrtSize = TextureSideSize * 4;

    // Initialize Texture Data Array
    TextureData = new uint8[TextureDataSize];

    DynamicTexture = UTexture2D::CreateTransient(TextureSideSize, TextureSideSize, EPixelFormat::PF_R8G8B8A8, "DynamicTexture");
    DynamicTexture->CompressionSettings = TextureCompressionSettings::TC_EditorIcon;
    DynamicTexture->Filter = TextureFilter::TF_Default;
    DynamicTexture->SRGB = 0;
    DynamicTexture->AddToRoot();
    DynamicTexture->UpdateResource();

    TextureRegion = FUpdateTextureRegion2D{ 0, 0, 0, 0, TextureSideSize, TextureSideSize };
}


void UMyGameInstanceSubsystem::Deinitialize()
{
	Super::Deinitialize();

    DynamicTexture = nullptr;
    delete[] TextureData;
}


void UMyGameInstanceSubsystem::UpdateTexture()
{
    struct FUpdateTextureRegionsData
    {
        FTexture2DResource* Texture2DResource;
        FRHITexture2D* TextureRHI;
        int32 MipIndex;
        uint32 NumRegions;
        FUpdateTextureRegion2D* Regions;
        uint32 SrcPitch;
        uint32 SrcBpp;
        uint8* SrcData;
    };

    FTexture2DResource* Resource = static_cast<FTexture2DResource*>(DynamicTexture->GetResource());

    FUpdateTextureRegionsData RegionData{
        Resource,
        Resource->GetTexture2DRHI(),
        0,
        1,
        &TextureRegion,
        TextureDataSqrtSize,
        4,
        TextureData,
    };

    ENQUEUE_RENDER_COMMAND(UpdateTextureRegionsData)(
        [RegionData, Texture = DynamicTexture](FRHICommandListImmediate& RHICmdList)
        {
            for (uint32 RegionIndex = 0; RegionIndex < RegionData.NumRegions; ++RegionIndex)
            {
                int32 CurrentFirstMip = Texture->FirstResourceMemMip;
                if (RegionData.TextureRHI && RegionData.MipIndex >= CurrentFirstMip)
                {
                    RHIUpdateTexture2D(
                        RegionData.TextureRHI,
                        RegionData.MipIndex - CurrentFirstMip,
                        RegionData.Regions[RegionIndex],
                        RegionData.SrcPitch,
                        RegionData.SrcData
                        + RegionData.Regions[RegionIndex].SrcY * RegionData.SrcPitch
                        + RegionData.Regions[RegionIndex].SrcX * RegionData.SrcBpp
                    );
                }
            }
        });
}


void UMyGameInstanceSubsystem::CreateGrid(int count)
{
    FMemory::Memset(TextureData, 0, TextureDataSize);

    constexpr int dotInterval = 3;
    constexpr uint32 color = 0xC87F7F7F;
    const int interval = TextureSideSize / count;

    auto* pixelData = reinterpret_cast<uint32*>(TextureData);

    for (int i = 1; i < count; ++i)
    {
        uint32 linePos = i * interval;

        for (uint32 offset = 0; offset < TextureSideSize; offset += dotInterval)
        {
            // Draw the horizontal dot (y = linePos, x = offset)
            pixelData[linePos * TextureSideSize + offset] = color;

            // Draw the vertical dot (y = offset, x = linePos)
            pixelData[offset * TextureSideSize + linePos] = color;
        }
    }

    UpdateTexture();
}
