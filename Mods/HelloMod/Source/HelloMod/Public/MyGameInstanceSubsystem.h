// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGBuildable.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MyGameInstanceSubsystem.generated.h"

class UCanvasRenderTarget2D;
/**
 * 
 */
UCLASS()
class HELLOMOD_API UMyGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    UMyGameInstanceSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    //virtual void Deinitialize() override;


protected:
    UPROPERTY(EditAnywhere)
    TObjectPtr<UCanvasRenderTarget2D> CanvasRenderTarget;

    UPROPERTY(EditAnywhere)
    TObjectPtr<UTexture> RenderTexture;

    UPROPERTY(EditAnywhere)
    TSubclassOf<AFGBuildable> BuildableClass;

    bool ShouldDraw = true;
};
