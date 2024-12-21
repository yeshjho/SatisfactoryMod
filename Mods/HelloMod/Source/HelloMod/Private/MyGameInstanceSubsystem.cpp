// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstanceSubsystem.h"

#include "CanvasItem.h"
#include "Patching/NativeHookManager.h"
#include "FGSaveSession.h"
#include "FGBuildable.h"
#include "FGBuildableFoundation.h"
#include "FGLightweightBuildableSubsystem.h"
#include "TickTimeBudget.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Canvas.h"
//#include "ClearQuad.h"
//#include "RHICommandList.h"


UMyGameInstanceSubsystem::UMyGameInstanceSubsystem()
{
    CanvasRenderTarget = ConstructorHelpers::FObjectFinder<UCanvasRenderTarget2D>(TEXT("/Script/Engine.CanvasRenderTarget2D'/HelloMod/NewCanvasRenderTarget2D.NewCanvasRenderTarget2D'")).Object;
    RenderTexture = ConstructorHelpers::FObjectFinder<UTexture>(TEXT("/Script/Engine.Texture2D'/HelloMod/Untitled.Untitled'")).Object;
    RenderTexture2 = ConstructorHelpers::FObjectFinder<UTexture>(TEXT("/Script/Engine.Texture2D'/HelloMod/Untitled2.Untitled2'")).Object;
}


UE5Coro::TCoroutine<> UMyGameInstanceSubsystem::DoWork(AFGLightweightBuildableSubsystem* inst, FForceLatentCoroutine)
{
	constexpr int renderTextureSize = 2048;

	constexpr float originUV[] = { 0.4329333333333333333333333333f, 0.5f };
	constexpr float mapWorldSize[] = { 750'000, 750'000 };


	UKismetRenderingLibrary::ClearRenderTarget2D(this, CanvasRenderTarget, { 0, 0, 0, 0 });

	UCanvas* canvas;
	FVector2D size;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, CanvasRenderTarget, canvas, size, DrawContext);

	auto Budget = UE5Coro::Latent::FTickTimeBudget::Milliseconds(1);

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

			co_await Budget;
		}
	}

	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, DrawContext);
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
			//UCanvas* canvas;
			//FVector2D size;
            //FDrawToRenderTargetContext context;
            //UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, CanvasRenderTarget, canvas, size, context);
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

            //UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, context);
		};
	SUBSCRIBE_UOBJECT_METHOD_AFTER(UFGSaveSession, LoadGame, lambda);


	const auto l = [this](int32 ret, AFGLightweightBuildableSubsystem* inst, TSubclassOf< class AFGBuildable > buildableClass, FRuntimeBuildableInstanceData& buildableInstanceData, bool fromSaveData = false, int32 saveDataBuildableIndex = INDEX_NONE, uint16 constructId = MAX_uint16, AActor* buildEffectInstigator = nullptr, int32 blueprintBuildEffectIndex = INDEX_NONE)
		{
            UE_LOG(LogTemp, Warning, TEXT("Buildable %s"), *buildableClass->GetName());
			ShouldDraw = true;
		};

	SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, AddFromBuildableInstanceData, l);

	/*
	const auto l2 = [this](AFGLightweightBuildableSubsystem* inst)
		{
            if (!ShouldDraw)
            {
                return;
            }
			ShouldDraw = false;

		

			AsyncTask(ENamedThreads::Type::ActualRenderingThread, [this, inst]()
				{
					//UKismetRenderingLibrary::ClearRenderTarget2D(this, CanvasRenderTarget, { 0, 0, 0, 0 });
					UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::LogAndReturnNull);
					if (!World)
					{
						return;
					}


					FRHICommandListImmediate& RHICmdList = FRHICommandListImmediate::Get();

					FTextureRenderTargetResource* RenderTargetResource = CanvasRenderTarget->GetRenderTargetResource();
					FRHIRenderPassInfo RPInfo(RenderTargetResource->GetRenderTargetTexture(), ERenderTargetActions::DontLoad_Store);
					RHICmdList.Transition(FRHITransitionInfo(RenderTargetResource->GetRenderTargetTexture(), ERHIAccess::Unknown, ERHIAccess::RTV));
					RHICmdList.BeginRenderPass(RPInfo, TEXT("ClearRT"));
					DrawClearQuad(RHICmdList, { 0, 0, 0, 0 });
					RHICmdList.EndRenderPass();

					RHICmdList.Transition(FRHITransitionInfo(RenderTargetResource->GetRenderTargetTexture(), ERHIAccess::RTV, ERHIAccess::SRVMask));


					UCanvas* Canvas = nullptr;
					//UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, CanvasRenderTarget, canvas, size, context);

					World->FlushDeferredParameterCollectionInstanceUpdates();

					Canvas = World->GetCanvasForRenderingToTarget();

					FCanvas* NewCanvas = new FCanvas(
						RenderTargetResource,
						nullptr,
						World,
						World->GetFeatureLevel(),
						FCanvas::CDM_DeferDrawing);
					Canvas->Init(CanvasRenderTarget->SizeX, CanvasRenderTarget->SizeY, nullptr, NewCanvas);

#if  WANTS_DRAW_MESH_EVENTS
					//Context.DrawEvent = new FDrawEvent();
					//BEGIN_DRAW_EVENTF_GAMETHREAD(DrawCanvasToTarget, (*Context.DrawEvent), *CanvasRenderTarget->GetFName().ToString())
#endif // WANTS_DRAW_MESH_EVENTS

					RenderTargetResource->FlushDeferredResourceUpdate(RHICmdList);


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
							Canvas->DrawItem(TileItem);
						}
					}


					//UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, context);

					UCanvas* WorldCanvas = World->GetCanvasForRenderingToTarget();

					if (WorldCanvas->Canvas)
					{
						WorldCanvas->Canvas->Flush_RenderThread(RHICmdList);
						delete WorldCanvas->Canvas;
						WorldCanvas->Canvas = nullptr;
					}

					TransitionAndCopyTexture(RHICmdList, RenderTargetResource->GetRenderTargetTexture(), RenderTargetResource->TextureRHI, {});


#if WANTS_DRAW_MESH_EVENTS
					//STOP_DRAW_EVENT_GAMETHREAD(*Context.DrawEvent);
					//delete Context.DrawEvent;
#endif // WANTS_DRAW_MESH_EVENTS
				});
		};
		*/

	/*
	const auto l2 = [this](AFGLightweightBuildableSubsystem* inst)
		{
			if (IsDrawingFinished)
			{
				IsDrawingFinished = false;
				UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, DrawContext);
			}

			if (!ShouldDraw)
			{
				return;
			}
			ShouldDraw = false;


			UKismetRenderingLibrary::ClearRenderTarget2D(this, CanvasRenderTarget, { 0, 0, 0, 0 });

			UCanvas* canvas;
			FVector2D size;
			UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, CanvasRenderTarget, canvas, size, DrawContext);

			auto drawTask = UE::Tasks::Launch(UE_SOURCE_LOCATION, [inst, canvas, texture = RenderTexture->GetResource()]()
				{
					for (const auto& [type, arr] : inst->mBuildableClassToInstanceArray)
					{
						for (const auto& data : arr)
						{
							FVector loc = data.Transform.GetLocation();
							FRotator rot = data.Transform.GetRotation().Rotator();

							FVector2D ScreenPosition = FVector2D(originUV[0] + loc.X / mapWorldSize[0], originUV[1] + loc.Y / mapWorldSize[1]) * renderTextureSize;
							float Rotation = rot.Yaw;
							FCanvasTileItem TileItem(ScreenPosition, texture, { 50, 50 }, { 0, 0 }, { 1, 1 }, FLinearColor::White);
							TileItem.Rotation = FRotator(0, Rotation, 0);
							TileItem.PivotPoint = { 0.5, 0.5 };
							TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Translucent);
							TileItem.Z = loc.Z;
							canvas->DrawItem(TileItem);
						}
					}
				}, LowLevelTasks::ETaskPriority::BackgroundLow);

			IsDrawingFinished = false;
			UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
                {
					IsDrawingFinished = true;
                }, drawTask);
		};
		*/


	const auto l2 = [this](AFGLightweightBuildableSubsystem* inst)
	{
		if (!ShouldDraw)
		{
			return;
		}
		ShouldDraw = false;

		DoWork(inst);
	};

	SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, RemoveStaleTemporaryBuildables, l2); 
}
