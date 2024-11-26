// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MyGameInstanceSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class TESTMOD_API UMyGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable)
    void CreateGrid(int count);

private:
    void UpdateTexture();


protected:
    UPROPERTY(EditDefaultsOnly)
    uint32 TextureSideSize = 2048;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    UTexture2D* DynamicTexture = nullptr;

private:
    uint8* TextureData;

    uint32 TextureDataSize;
    uint32 TextureDataSqrtSize;
    uint32 TextureTotalPixels;

    // Update Region Struct
    FUpdateTextureRegion2D TextureRegion;
};
