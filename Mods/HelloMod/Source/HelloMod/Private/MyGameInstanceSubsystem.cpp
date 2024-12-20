// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstanceSubsystem.h"

#include "CanvasItem.h"
#include "Patching/NativeHookManager.h"
#include "FGSaveSession.h"
#include "FGBuildable.h"
#include "FGBuildableFoundation.h"
#include "FGLightweightBuildableSubsystem.h"
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

	UE_LOG(LogTemp, Warning, TEXT("INITIALIZE"));

	
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
			//for (const auto [obj, ver] : UFGSaveSession::mObjectToSerailizedVersion)
			//{
			//	if (obj->IsA<AFGBuildable>())
			//	{
			//		auto* buildable = Cast<AFGBuildable>(obj);
			//		UE_LOG(LogTemp, Warning, TEXT("Object: %s, Class: %s, Pos: %s"), *obj->GetName(), *obj->GetClass()->GetName(), *buildable->GetActorLocation().ToString());

   //                 FVector2D ScreenPosition = FVector2D(originUV[0] + buildable->GetActorLocation().X / mapWorldSize[0], originUV[1] + buildable->GetActorLocation().Y / mapWorldSize[1]) * renderTextureSize;
   //                 float Rotation = buildable->GetActorRotation().Yaw;
			//		FCanvasTileItem TileItem(ScreenPosition, RenderTexture->GetResource(), { 50, 50 }, { 0, 0 }, { 1, 1 }, FLinearColor::White);
			//		TileItem.Rotation = FRotator(0, Rotation, 0);
			//		TileItem.PivotPoint = { 0.5, 0.5 };
			//		TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Translucent);
   //                 TileItem.Z = buildable->GetActorLocation().Z;
			//		canvas->DrawItem(TileItem);
			//	}
   //         }

			UE_LOG(LogTemp, Warning, TEXT("LOAD GAME"));

			auto* lwbss = AFGLightweightBuildableSubsystem::Get(UFGSaveSession::mObjectToSerailizedVersion.begin()->Key);
			for (const auto& [type, arr] : lwbss->mBuildableClassToInstanceArray)
			{
                UE_LOG(LogTemp, Warning, TEXT("Type: %s"), *type->GetName());
			}
			//if (!lwbss)
			//{
   //             UE_LOG(LogTemp, Warning, TEXT("LightweightBuildableSubsystem not found"));
   //             return;
			//}

			//if (!BuildableClass)
			//{
   //             UE_LOG(LogTemp, Warning, TEXT("BuildableClass not set"));
   //             return;
			//}

			//for (int i = 0; true; i++)
			//{
			//	FRuntimeBuildableInstanceData* data = lwbss->
			//			GetRuntimeDataForBuildableClassAndIndex(BuildableClass, i);
   //             if (!data)
   //             {
			//		UE_LOG(LogTemp, Warning, TEXT("No more buildable %i"), i);
			//		break;
   //             }

			//	FVector loc = data->Transform.GetLocation();
   //             FRotator rot = data->Transform.GetRotation().Rotator();

			//	FVector2D ScreenPosition = FVector2D(originUV[0] + loc.X / mapWorldSize[0], originUV[1] + loc.Y / mapWorldSize[1]) * renderTextureSize;
			//	float Rotation = rot.Yaw;
			//	FCanvasTileItem TileItem(ScreenPosition, RenderTexture->GetResource(), { 50, 50 }, { 0, 0 }, { 1, 1 }, FLinearColor::White);
			//	TileItem.Rotation = FRotator(0, Rotation, 0);
			//	TileItem.PivotPoint = { 0.5, 0.5 };
			//	TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Translucent);
			//	TileItem.Z = loc.Z;
			//	canvas->DrawItem(TileItem);
			//}

            UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, context);
		};
	SUBSCRIBE_UOBJECT_METHOD_AFTER(UFGSaveSession, LoadGame, lambda);


	const auto l = [this](int32 ret, AFGLightweightBuildableSubsystem* inst, TSubclassOf< class AFGBuildable > buildableClass, FRuntimeBuildableInstanceData& buildableInstanceData, bool fromSaveData = false, int32 saveDataBuildableIndex = INDEX_NONE, uint16 constructId = MAX_uint16, AActor* buildEffectInstigator = nullptr, int32 blueprintBuildEffectIndex = INDEX_NONE)
		{
            UE_LOG(LogTemp, Warning, TEXT("Buildable %s"), *buildableClass->GetName());
			ShouldDraw = true;
		};

	SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, AddFromBuildableInstanceData, l);

	const auto l2 = [this](AFGLightweightBuildableSubsystem* inst)
		{
            if (!ShouldDraw)
            {
                return;
            }
			ShouldDraw = false;

			const auto& [type, arr] : inst->mBuildableClassToInstanceArray

		ParallelFor()

			AsyncTask(ENamedThreads::Type::, [this, inst]()
				{
					UCanvas* canvas;
					FVector2D size;
					FDrawToRenderTargetContext context;
					UKismetRenderingLibrary::ClearRenderTarget2D(this, CanvasRenderTarget, { 0, 0, 0, 0 });
					UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, CanvasRenderTarget, canvas, size, context);


					for (const auto& [type, arr] : inst->mBuildableClassToInstanceArray)
					{
						for (const auto& data : arr)
						{
							FVector loc = data.Transform.GetLocation();
							FRotator rot = data.Transform.GetRotation().Rotator();

							FVector2D ScreenPosition = FVector2D(originUV[0] + loc.X / mapWorldSize[0], originUV[1] + loc.Y / mapWorldSize[1]) * renderTextureSize;
							float Rotation = rot.Yaw;
							FCanvasTileItem TileItem(ScreenPosition, RenderTexture->GetResource(), { 50, 50 }, { 0, 0 }, { 1, 1 }, FLinearColor::White);
							TileItem.Rotation = FRotator(0, Rotation, 0);
							TileItem.PivotPoint = { 0.5, 0.5 };
							TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Translucent);
							TileItem.Z = loc.Z;
							canvas->DrawItem(TileItem);
						}
					}

					UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, context);
				});
		};

	SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, RemoveStaleTemporaryBuildables, l2);
}
