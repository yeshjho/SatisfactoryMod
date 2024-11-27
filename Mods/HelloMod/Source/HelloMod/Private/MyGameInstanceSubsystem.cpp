// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstanceSubsystem.h"

#include "CanvasItem.h"
#include "Patching/NativeHookManager.h"
#include "FGSaveSession.h"
#include "FGBuildable.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Canvas.h"


UMyGameInstanceSubsystem::UMyGameInstanceSubsystem()
{
    CanvasRenderTarget = ConstructorHelpers::FObjectFinder<UCanvasRenderTarget2D>(TEXT("/Script/Engine.CanvasRenderTarget2D'/HelloMod/NewCanvasRenderTarget2D.NewCanvasRenderTarget2D'")).Object;
    RenderTexture = ConstructorHelpers::FObjectFinder<UTexture>(TEXT("/Script/Engine.Texture2D'/HelloMod/Untitled.Untitled'")).Object;
}


void UMyGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	
	constexpr int renderTextureSize = 2048;

    constexpr float originUV[] = { 0.4329333333333333333333333333f, 0.5f };
	constexpr float mapWorldSize[] = { 750'000, 750'000 };

	const auto lambda = 
		[this](bool, UFGSaveSession*, const FString&)
		{
			UCanvas* canvas;
			FVector2D size;
            FDrawToRenderTargetContext context;
            UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, CanvasRenderTarget, canvas, size, context);
			for (const auto [obj, ver] : UFGSaveSession::mObjectToSerailizedVersion)
			{
				if (obj->IsA<AFGBuildable>())
				{
					auto* buildable = Cast<AFGBuildable>(obj);
					UE_LOG(LogTemp, Warning, TEXT("Object: %s, Class: %s, Pos: %s"), *obj->GetName(), *obj->GetClass()->GetName(), *buildable->GetActorLocation().ToString());

                    FVector2D ScreenPosition = FVector2D(originUV[0] + buildable->GetActorLocation().X / mapWorldSize[0], originUV[1] + buildable->GetActorLocation().Y / mapWorldSize[1]) * renderTextureSize;
                    float Rotation = buildable->GetActorRotation().Yaw;
					FCanvasTileItem TileItem(ScreenPosition, RenderTexture->GetResource(), { 50, 50 }, { 0, 0 }, { 1, 1 }, FLinearColor::White);
					TileItem.Rotation = FRotator(0, Rotation, 0);
					TileItem.PivotPoint = { 0.5, 0.5 };
					TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Translucent);
                    TileItem.Z = buildable->GetActorLocation().Z;
					canvas->DrawItem(TileItem);
				}
            }
            UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, context);
		};
	SUBSCRIBE_UOBJECT_METHOD_AFTER(UFGSaveSession, LoadGame, lambda);
}
