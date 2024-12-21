// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UE5Coro.h"
#include "FGBuildable.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MyGameInstanceSubsystem.generated.h"

class AFGLightweightBuildableSubsystem;
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


private:
    UE5Coro::TCoroutine<> DoWork(AFGLightweightBuildableSubsystem* inst, FForceLatentCoroutine = {});


protected:
    UPROPERTY(EditAnywhere)
    TObjectPtr<UCanvasRenderTarget2D> CanvasRenderTarget;

    UPROPERTY(EditAnywhere)
    TObjectPtr<UTexture> RenderTexture;

    UPROPERTY(EditAnywhere)
    TObjectPtr<UTexture> RenderTexture2;

    UPROPERTY(EditAnywhere)
    TSubclassOf<AFGBuildable> BuildableClass;

    bool ShouldDraw = false;

    FDrawToRenderTargetContext DrawContext;
    bool IsDrawingFinished = false;
};
