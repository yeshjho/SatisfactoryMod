#include "CartographGameInstanceModule.h"

#include "AssetRegistryModule.h"
#include "CanvasItem.h"
#include "CanvasPanelSlot.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "FindLast.h"
#include "HorizontalBox.h"
#include "HorizontalBoxSlot.h"
#include "OutputDeviceNull.h"
#include "WidgetBlueprintGeneratedClass.h"

#include "FGLightweightBuildableSubsystem.h"
#include "FGBuildable.h"
#include "FGBuildableBeam.h"
#include "FGBuildableSubsystem.h"
#include "FGBuildableWire.h"
#include "FGBuildingDescriptor.h"
#include "FGBuildCategory.h"
#include "FGBuildSubCategory.h"
#include "FGPlayerController.h"
#include "FGSaveSession.h"
#include "FGSplineBuildableInterface.h"

#include "Patching/BlueprintHookHelper.h"
#include "Patching/BlueprintHookManager.h"
#include "Patching/NativeHookManager.h"

#include "CartographCanvasRenderItem.h"
#include "CartographModSubsystem.h"
#include "CartographRemoteCallObject.h"
#include "Cartograph_ConfigStruct.h"
#include "ConfigPropertyString.h"


#define LOCTEXT_NAMESPACE "Cartograph"


DEFINE_LOG_CATEGORY(LogCartograph);


template<typename T, typename U>
    requires
		(std::is_same_v<T, FVector> || std::is_same_v<T, FVector2D>) &&
		(std::is_same_v<U, FVector> || std::is_same_v<U, FVector2D>)
void draw_line(UCanvas* Canvas, const T& WorldStart, const U& WorldEnd, const FLinearColor& Color, float Thickness)
{
    const FVector2D StartScreenPosition = world_position_to_screen_position(WorldStart, FVector::ZeroVector);
    const FVector2D EndScreenPosition = world_position_to_screen_position(WorldEnd, FVector::ZeroVector);
	FCanvasLineItem LineItem{
		StartScreenPosition,
		EndScreenPosition
	};
	LineItem.LineThickness = Thickness;
	LineItem.SetColor(Color);
	// Only opaque lines are supported
	// LineItem.BlendMode = FCanvas::BlendToSimpleElementBlend
	Canvas->DrawItem(LineItem);
}


void UCartographGameInstanceModule::AfterSplineSegmentsModified()
{
	FCartograph_ConfigStruct ConfigInstance = FCartograph_ConfigStruct::GetActiveConfig(GetWorld());
	for (auto& [_, SplineData] : BuildableSplineDataMap)
	{
		if (SplineData.SegmentsConfigName.IsNone())
		{
            SplineData.SegmentsCached = UnspecifiedSplineSegments;
		}
		else
		{
			const FProperty* Property = FCartograph_ConfigStruct::StaticStruct()->FindPropertyByName(SplineData.SegmentsConfigName);
			if (!Property)
			{
				CARTO_LOG_ERROR("SparsityConfigName not found: %s", *SplineData.SegmentsConfigName.ToString());
				continue;
			}
			SplineData.SegmentsCached = *Property->ContainerPtrToValuePtr<int>(&ConfigInstance);
		}
	}

	if (!IsInWorld)
	{
		return;
	}

	for (FBuildingData& BuildingData : CurrentBuildingData)
	{
		BuildingData.CalculateSplinePoints();
	}

    CARTO_LOG("Spline segments modified");
	RedrawMap(true);
}


void UCartographGameInstanceModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
	Super::DispatchLifecycleEvent(Phase);

	switch (Phase)
	{
	case ELifecyclePhase::CONSTRUCTION:
		return;

	case ELifecyclePhase::INITIALIZATION:
		RegisterMenuButton();
		return;

	case ELifecyclePhase::POST_INITIALIZATION:
		break;
	}

    CARTO_LOG("UCartographGameInstanceModule Init")

    GatherBuildables();

	Instance = this;


	for (const auto& [Material, CategoryData] : MaterialBuildCategoryDataOverrideMap)
	{
		for (const auto& [_, Recipe] : Cast<UFGFactoryCustomizationDescriptor_Material>(Material->ClassDefaultObject)->GetBuildableMap())
		{
			TSubclassOf<AFGBuildable> Buildable = Cast<UFGBuildingDescriptor>(UFGRecipe::GetDescriptorForRecipe(Recipe)->ClassDefaultObject)->mBuildableClass;
			if (!BuildableBuildCategoryDataOverrideMap.Contains(Buildable.Get()))
			{
				BuildableBuildCategoryDataOverrideMap.Add(Buildable.Get(), CategoryData);
			}
		}
	}

	for (const auto& [Material, LayerData] : MaterialBuildLayerDataOverrideMap)
	{
		for (const auto& [_, Recipe] : Cast<UFGFactoryCustomizationDescriptor_Material>(Material->ClassDefaultObject)->GetBuildableMap())
		{
			TSubclassOf<AFGBuildable> Buildable = Cast<UFGBuildingDescriptor>(UFGRecipe::GetDescriptorForRecipe(Recipe)->ClassDefaultObject)->mBuildableClass;
			if (!BuildableBuildLayerDataOverrideMap.Contains(Buildable.Get()))
			{
				BuildableBuildLayerDataOverrideMap.Add(Buildable.Get(), LayerData);
			}
		}
	}

	AfterSplineSegmentsModified();

	// Wanted to do this in Blueprint, inheriting BP_CP_Int, but couldn't get the module manager there.
	const FConfigId ConfigId{ "Cartograph", "" };
	const UConfigManager* ConfigManager = GetWorld()->GetGameInstance()->GetSubsystem<UConfigManager>();
	const UConfigPropertySection* RootSection = ConfigManager->GetConfigurationRootSection(ConfigId);
	for (const auto& [Name, Property] : RootSection->SectionProperties)
	{
		if (Name.EndsWith("Segments"))
		{
            Property->OnPropertyValueChanged.AddDynamic(this, &UCartographGameInstanceModule::AfterSplineSegmentsModified);
		}
	}

	LoadRuntimeConfig();


#pragma region Hooking
	const auto LambdaAfterLoadGame =
		[this](bool ReturnValue, UFGSaveSession* ClassInstance, const FString& SaveName)
		{
			CARTO_LOG("LoadGame");

			IsClient = false;
			ShouldInitialize = true;
            // Wait for ACartographModSubsystem to initialize
			GetWorld()->GetTimerManager().SetTimerForNextTick(
				[this, ClassInstance]()
				{
					if (!ShouldInitialize || !GIsRunning)
					{
						return;
					}

					ShouldInitialize = false;
					IsInitializing = true;

					TArray<TWeakObjectPtr<AFGBuildable>> Factories;
					Algo::Transform(AFGBuildableSubsystem::Get(ClassInstance)->GetAllBuildablesRef(), Factories,
						[](AFGBuildable* Buildable) { return Buildable; });
					Coroutine = InitialBuildableGather(
						std::move(Factories),
						AFGLightweightBuildableSubsystem::Get(ClassInstance)->mBuildableClassToInstanceArray
					);
				});
		};


    const auto LambdaAfterAddFromBuildableInstanceData = 
        [this](int32 ReturnValue, AFGLightweightBuildableSubsystem* ClassInstance, TSubclassOf<AFGBuildable> BuildableClass,
            FRuntimeBuildableInstanceData& BuildableInstanceData, bool FromSaveData = false, int32 SaveDataBuildableIndex = INDEX_NONE, 
            uint16 ConstructId = MAX_uint16, AActor* BuildEffectInstigator = nullptr, int32 BlueprintBuildEffectIndex = INDEX_NONE)
        {
			CARTO_LOG_VERBOSE("AddFromBuildableInstanceData: %s, Skip: %d", *BuildableClass->GetName(), ShouldInitialize || FromSaveData || IsClient);

			if (ShouldInitialize || FromSaveData || IsClient || !GIsRunning)
			{
				return;
			}

			if (BuildableToIgnore.Contains(BuildableClass.Get()))
			{
				return;
			}

			FBuildingData Data{
					.Transform = BuildableInstanceData.Transform,
					//.CustomizationData = BuildableInstanceData.CustomizationData,
			};
            Data.FillInHashAndCache(BuildableClass);
			PendingAddBuildingData.Add(std::move(Data));

			RedrawMap(false);
        };


	const auto LambdaAfterAddFromReplicatedData =
		[this](AFGLightweightBuildableSubsystem* ClassInstance, TSubclassOf<AFGBuildable> BuildableClass, TSubclassOf<UFGRecipe> BuiltWithRecipe,
			const FLightweightBuildableReplicationItem& ReplicationData, int32 MaxSize, 
			AActor* BuildEffectInstigator, int32 BlueprintBuildIndex)
		{
			CARTO_LOG_VERBOSE("AddFromReplicatedData: %s, Skip: %d", *BuildableClass->GetName(), ShouldInitialize || IsClient);

			if (ShouldInitialize || IsClient || !GIsRunning)
			{
				return;
			}

			if (BuildableToIgnore.Contains(BuildableClass.Get()))
			{
				return;
			}

			FBuildingData Data{
					.Transform = ReplicationData.Transform,
					//.CustomizationData = ReplicationData.CustomizationData,
			};
			Data.FillInHashAndCache(BuildableClass);
			PendingAddBuildingData.Add(std::move(Data));

			RedrawMap(false);
		};


	const auto LambdaAfterAddBuildable =
		[this](AFGBuildableSubsystem* ClassInstance, AFGBuildable* Buildable)
		{
			CARTO_LOG_VERBOSE("AddBuildable: %s, Skip: %d", *Buildable->GetClass()->GetName(), ShouldInitialize || IsClient);

			if (ShouldInitialize || IsClient || !GIsRunning)
			{
				return;
			}

			if (BuildableToIgnore.Contains(Buildable->GetClass()))
			{
				return;
			}

			FBuildingData Data{
					.Transform = Buildable->GetTransform(),
					//.CustomizationData = Buildable->GetCustomizationData_Native(),
			};
			Data.AddExtraData(Buildable);
			Data.FillInHashAndCache(Buildable->GetClass());
			PendingAddBuildingData.Add(std::move(Data));

			RedrawMap(false);
		};


	const auto LambdaAfterInvalidateRuntimeInstanceDataForIndex =
		[this](AFGLightweightBuildableSubsystem* ClassInstance, TSubclassOf<AFGBuildable> BuildableClass, int32 Index)
		{
			CARTO_LOG_VERBOSE("InvalidateRuntimeInstanceDataForIndex: %s, Skip: %d", *BuildableClass->GetName(), ShouldInitialize || IsClient);

			if (ShouldInitialize || IsClient || !GIsRunning)
			{
				return;
			}

			if (BuildableToIgnore.Contains(BuildableClass.Get()))
			{
				return;
			}

			const FRuntimeBuildableInstanceData* LightweightData = ClassInstance->GetRuntimeDataForBuildableClassAndIndex(BuildableClass, Index);

			FBuildingData Data{
					.Transform = LightweightData->Transform,
					//.CustomizationData = Data->CustomizationData,
			};
			//Data.FillInHashAndCache(BuildableClass);  // Cache are not used in comparison (==, <=>) so we don't need to fill it
			Data.FillInHash(BuildableClass);
            PendingRemoveBuildingData.Add(std::move(Data));

			RedrawMap(false);
		};


	const auto LambdaAfterRemoveBuildable =
		[this](AFGBuildableSubsystem* ClassInstance, AFGBuildable* Buildable)
		{
			CARTO_LOG_VERBOSE("RemoveBuildable: %s, Skip: %d", *Buildable->GetClass()->GetName(), ShouldInitialize || IsClient);

			if (ShouldInitialize || IsClient || !GIsRunning)
			{
				return;
			}

			if (BuildableToIgnore.Contains(Buildable->GetClass()))
			{
				return;
			}

            FBuildingData Data{
                    .Transform = Buildable->GetTransform(),
                    //.CustomizationData = Buildable->GetCustomizationData_Native(),
            };
            Data.AddExtraData(Buildable);
            //Data.FillInHashAndCache(Buildable->GetClass());  // Cache are not used in comparison (==, <=>) so we don't need to fill it
			Data.FillInHash(Buildable->GetClass());
			PendingRemoveBuildingData.Add(std::move(Data));

			RedrawMap(false);
		};


	const auto LambdaAfterCloseRespawnUI =
        [this](AFGHUD* ClassInstance)
        {
            if (!ShouldInitialize || !GIsRunning)
            {
				return;
            }

            CARTO_LOG("CloseRespawnUI");

	        AFGPlayerController* PlayerController = Cast<AFGPlayerController>(GetWorld()->GetFirstPlayerController());
			if (PlayerController->HasAuthority())
			{
				return;
			}

			auto* RCO = PlayerController->GetRemoteCallObjectOfClass<UCartographRemoteCallObject>();
			if (RCO)
			{
				ShouldInitialize = false;
				IsInitializing = true;
				CurrentBuildingData.Empty();
                BuildingDataIndexRedirector.Empty();
                CurrentBuildingQuadTree.Empty();
                BuildingCountMap.Empty();
				RCO->ReceivedSliceCount = 0;
				RCO->Buffer.Empty();
				RCO->ServerRequestInitialBuildingData(PlayerController, EInitialDataSendPhase::Initial);
			}
			else
			{
                CARTO_LOG_ERROR("Failed to get RemoteCallObject");
			}
        };


	if (!WITH_EDITOR)
	{
		// Called only when single player or host
		SUBSCRIBE_UOBJECT_METHOD_AFTER(UFGSaveSession, LoadGame, LambdaAfterLoadGame);

		// Doing it after PlayerController::BeginPlay would interfere other network packets,
	    // resulting higher chance of packet loss (due to timeout)
		SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGHUD, CloseRespawnUI, LambdaAfterCloseRespawnUI);


		SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, AddFromBuildableInstanceData, LambdaAfterAddFromBuildableInstanceData);
	    SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, AddFromReplicatedData, LambdaAfterAddFromReplicatedData);

		SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGBuildableSubsystem, AddBuildable, LambdaAfterAddBuildable);


		SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, InvalidateRuntimeInstanceDataForIndex, LambdaAfterInvalidateRuntimeInstanceDataForIndex);

		SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGBuildableSubsystem, RemoveBuildable, LambdaAfterRemoveBuildable);


		SUBSCRIBE_METHOD(FCanvas::GetBatchedElements,
			[](auto& Scope, FCanvas* ClassInstance,
				FCanvas::EElementType InElementType, FBatchedElementParameters* InBatchedElementParameters, const FTexture* InTexture, ESimpleElementBlendMode InBlendMode, const FDepthFieldGlowInfo& GlowInfo, bool bApplyDPIScale)
			{
				// get sort element based on the current sort key from top of sort key stack
				FCanvas::FCanvasSortElement& SortElement = ClassInstance->GetSortElement(ClassInstance->TopDepthSortKey());
				// find a batch to use 
				FCartographCanvasRenderItem* RenderBatch = nullptr;
				// get the current transform entry from top of transform stack
				FCanvas::FTransformEntry FinalTransform = ClassInstance->GetTransformStack().Top();

				if (!bApplyDPIScale && ClassInstance->GetDPIScale() != 1.0f)
				{
					FinalTransform = FCanvas::FTransformEntry(FScaleMatrix(1 / ClassInstance->GetDPIScale()) * FinalTransform.GetMatrix());
				}

				// try to use the current top entry in the render batch array
				if (SortElement.RenderBatchArray.Num() > 0)
				{
					checkSlow(SortElement.RenderBatchArray.Last());
					RenderBatch = static_cast<FCartographCanvasRenderItem*>(SortElement.RenderBatchArray.Last());
				}

				// if a matching entry for this batch doesn't exist then allocate a new entry
				if (RenderBatch == nullptr ||
					!RenderBatch->IsMatch(InBatchedElementParameters, InTexture, InBlendMode, InElementType, FinalTransform, GlowInfo))
				{
					RenderBatch = new FCartographCanvasRenderItem(InBatchedElementParameters, InTexture, InBlendMode, InElementType, FinalTransform, GlowInfo);
					SortElement.RenderBatchArray.Add(RenderBatch);
				}

				Scope.Override(RenderBatch->GetBatchedElements());
			});


		if (!FPlatformProperties::IsServerOnly())
		{
			UBlueprintHookManager* HookManager = GEngine->GetEngineSubsystem<UBlueprintHookManager>();
			HookManager->HookBlueprintFunction(
				MapContainerWidget->FindFunctionByName(TEXT("SetFiltersCollapsed")),
				[](const FBlueprintHookHelper& Helper) 
				{
					TSharedRef<FBlueprintHookVariableHelper_Local> VariableHelper = Helper.GetLocalVariableHelper();
					const bool IsCollapsed = VariableHelper->GetBoolVariable(TEXT("IsCollapsed"));
					if (IsCollapsed)
					{
						return;
					}

					const auto* Widget = Cast<UUserWidget>(Helper.GetContext());
					UWidget* Menu = Widget->WidgetTree->FindWidget("CartographMenu");
					CARTO_LOG_ERROR_RETURN_IF_NULL(Menu);
					Menu->SetVisibility(ESlateVisibility::Collapsed);

					UWidget* Button = Widget->WidgetTree->FindWidget("CartographMenuShowHideButton");
					CARTO_LOG_ERROR_RETURN_IF_NULL(Button);

					FProperty* IsOpenProperty = Button->GetClass()->FindPropertyByName("IsOpen");
                    CARTO_LOG_ERROR_RETURN_IF_NULL(IsOpenProperty);
					*IsOpenProperty->ContainerPtrToValuePtr<bool>(Button) = false;

					FOutputDeviceNull Ar;
					Button->CallFunctionByNameWithArguments(TEXT("SetShowHideText"), Ar, nullptr, true);
				},
				EPredefinedHookOffset::Return);
		}
	}
#pragma endregion
}


void UCartographGameInstanceModule::OnWorldLoaded()
{
	CARTO_LOG("OnWorldLoaded");

    IsInWorld = true;
	ShouldInitialize = true;
	IsClient = GetWorld()->IsNetMode(NM_Client);

	if (!FPlatformProperties::IsServerOnly())
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget, { 0, 0, 0, 0 });
	}
}


void UCartographGameInstanceModule::OnWorldUnloaded()
{
	CARTO_LOG("OnWorldUnloaded");

	IsInWorld = false;
}


void UCartographGameInstanceModule::OnLayerConfigChanged()
{
    CARTO_LOG("OnLayerConfigChanged");

	RedrawMap(true);
	SaveRuntimeConfig();
}


const FBuildLayerData* UCartographGameInstanceModule::GetBuildLayerData(uint32 ClassHash)
{
	FillBuildLayerDataCache();

    if (const auto* DataCache = BuildLayerDataMapCache.Find(ClassHash))
    {
        return *DataCache;
    }
	return nullptr;
}


bool UCartographGameInstanceModule::DoesBuildingExist(uint32 ClassHash) const
{
	return BuildingCountMap.FindRef(ClassHash) > 0;
}


#pragma region Drawing
void UCartographGameInstanceModule::RedrawMap(bool bRedrawEntirely)
{
	if (!Coroutine.IsDone())
	{
		if (!IsInitializing)
		{
			CARTO_LOG_DEBUG("RedrawMapCoroutine Cancel Requested");
			Coroutine.Cancel();
		}
        IsPendingRedrawEntire = bRedrawEntirely;
		IsPendingRedraw = true;
	}
	else if (!IsClient || !IsInitializing)
	{
		ExecuteRedrawMapCoroutine(bRedrawEntirely);
	}
}


// Factories/Buildings: Intentional copies
UE5Coro::TCoroutine<> UCartographGameInstanceModule::InitialBuildableGather(
	TArray<TWeakObjectPtr<AFGBuildable>> Factories, TMap<TSubclassOf<AFGBuildable>, TArray<FRuntimeBuildableInstanceData>> Buildings, FForceLatentCoroutine)
{
	int BuildingCount = 0;
	for (const auto& [Type, Arr] : Buildings)
	{
        BuildingCount += Arr.Num();
	}

    CARTO_LOG("InitialBuildableGather Started. Factories: %d, Buildings: %d", Factories.Num(), BuildingCount);

	const int Total = Factories.Num() + BuildingCount;
	CurrentBuildingData.Empty(Total);
    BuildingDataIndexRedirector.Empty();
    CurrentBuildingQuadTree.Empty();
    BuildingCountMap.Empty();

	const float TimeBudget = FCartograph_ConfigStruct::GetActiveConfig(GetWorld()).InitializeTimeBudget;
	UE5Coro::Latent::FTickTimeBudget Budget = UE5Coro::Latent::FTickTimeBudget::Milliseconds(TimeBudget);

	int Processed = 0;

	for (const TWeakObjectPtr<AFGBuildable>& Factory : Factories)
	{
		if (!Factory.IsValid() || BuildableToIgnore.Contains(Factory->GetClass()))
		{
			continue;
		}
		
		FBuildingData NewBuildingData{
			.Transform = Factory->GetTransform(),
            //.CustomizationData = Factory->GetCustomizationData_Native(),
		};
        NewBuildingData.AddExtraData(Factory.Get());
		NewBuildingData.FillInHashAndCache(Factory->GetClass());

		const int32 Pos = Algo::LowerBound(CurrentBuildingData, NewBuildingData);
		OnBuildingDataAdd(NewBuildingData, Pos);
		CurrentBuildingData.Insert(std::move(NewBuildingData), Pos);

        InitializeProgress = static_cast<float>(++Processed) / Total;
		co_await Budget;
	}

	for (const auto& [Type, Arr] : Buildings)
	{
        if (BuildableToIgnore.Contains(Type.Get()))
        {
            continue;
        }

		for (const FRuntimeBuildableInstanceData& InstanceData : Arr)
		{
			FBuildingData NewBuildingData{
				.Transform = InstanceData.Transform,
				//.CustomizationData = InstanceData.CustomizationData,
			};
			NewBuildingData.FillInHashAndCache(Type);

			const int32 Pos = Algo::LowerBound(CurrentBuildingData, NewBuildingData);
            OnBuildingDataAdd(NewBuildingData, Pos);
			CurrentBuildingData.Insert(std::move(NewBuildingData), Pos);

			InitializeProgress = static_cast<float>(++Processed) / Total;
			co_await Budget;
		}
	}

	MinHeight = !CurrentBuildingData.IsEmpty() ? CurrentBuildingData[0].Transform.GetLocation().Z : -100;
	MaxHeight = !CurrentBuildingData.IsEmpty() ? CurrentBuildingData.Last().Transform.GetLocation().Z : 100;
	OnZFilterUpdated(0, 1);

	CARTO_LOG("InitialBuildableGather Finished");

	for (const auto& [ClassHash, Count] : BuildingCountMap)
	{
        CARTO_LOG_DEBUG("Building: %u, Count: %d", ClassHash, Count);
	}

	IsInitializing = false;
	IsPendingRedraw = false;
	ExecuteRedrawMapCoroutine(true);
}


// AddedBuildings/RemovedBuildings: Intentional copies
UE5Coro::TCoroutine<> UCartographGameInstanceModule::RedrawMapCoroutine(
	TArray<FBuildingData> AddedBuildings, TArray<FBuildingData> RemovedBuildings, bool bRedrawEntirely, FForceLatentCoroutine)
{
	ON_SCOPE_EXIT
	{
        OnCoroutineFinishedOrCancelled();
	};

	CARTO_LOG("RedrawMapCoroutine Started. Entire: %d", bRedrawEntirely);

	const float TimeBudget = FCartograph_ConfigStruct::GetActiveConfig(GetWorld()).RedrawTimeBudget;
	UE5Coro::Latent::FTickTimeBudget Budget = UE5Coro::Latent::FTickTimeBudget::Milliseconds(TimeBudget);

    IsRedrawingEntirely |= bRedrawEntirely;

	{
        UE5Coro::FCancellationGuard Guard{};  // We'll lose added/removed building information if the coroutine is cancelled

		for (FBuildingData& AddedBuildingData : AddedBuildings)
		{
	        CARTO_LOG_DEBUG("AddedBuilding: %u", AddedBuildingData.BuildableClassHash);

			if (!IsRedrawingEntirely && AddedBuildingData.VisualBoxCache.bIsValid)
			{
				RedrawArea += AddedBuildingData.VisualBoxCache;
			}

			const int32 Pos = Algo::LowerBound(CurrentBuildingData, AddedBuildingData);
			OnBuildingDataAdd(AddedBuildingData, Pos);
			CurrentBuildingData.Insert(std::move(AddedBuildingData), Pos);

			co_await Budget;
		}

	    for (const FBuildingData& RemovedBuildingData : RemovedBuildings)
	    {
	        CARTO_LOG_DEBUG("RemovedBuilding: %u", RemovedBuildingData.BuildableClassHash);

	        const int32 Start = Algo::LowerBound(CurrentBuildingData, RemovedBuildingData);
            const int32 End = CurrentBuildingData.Num();

	        for (int32 i = Start; i < End; ++i)
	        {
	            if (RemovedBuildingData == CurrentBuildingData[i])
	            {
					if (!IsRedrawingEntirely && CurrentBuildingData[i].VisualBoxCache.bIsValid)
					{
						RedrawArea += CurrentBuildingData[i].VisualBoxCache;
					}

					OnBuildingDataRemove(CurrentBuildingData[i], i);
                    CurrentBuildingData.RemoveAt(i);
	                break;
	            }
                if (RemovedBuildingData > CurrentBuildingData[i])
                {
                    CARTO_LOG_ERROR("Can't find removed building data");
                    break;
                }
	        }

	        co_await Budget;
	    }

        MinHeight = !CurrentBuildingData.IsEmpty() ? CurrentBuildingData[0].Transform.GetLocation().Z : -100;
        MaxHeight = !CurrentBuildingData.IsEmpty() ? CurrentBuildingData.Last().Transform.GetLocation().Z : 100;
		const float Length = MaxHeight - MinHeight;
		MinZFilter = FMath::Floor(MinCached * Length + MinHeight);
		MaxZFilter = FMath::CeilToInt(MaxCached * Length + MinHeight);

        CARTO_LOG("Buildings Change Processed");
	}

	if (FPlatformProperties::IsServerOnly())
	{
		co_return;
	}

	// Sometimes lines go crazy (goes to the top or far right) if we don't delay.
	// My guess is because EndDraw and BeginDraw are called in the same frame, so I'm putting it here.
	co_await UE5Coro::Latent::NextTick();

	UCanvas* Canvas = nullptr;
	FVector2D _;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, Canvas, _, RenderContext);
	CurrentCanvas = Canvas->Canvas;

	if (IsRedrawingEntirely)
	{
		ScissorArea = { 0, 0, RENDER_TEXTURE_SIZE, RENDER_TEXTURE_SIZE };
	}
	else
	{
		const FVector2D MinScreenPosition = world_position_to_screen_position(RedrawArea.Min, FVector::ZeroVector);
        const FVector2D MaxScreenPosition = world_position_to_screen_position(RedrawArea.Max, FVector::ZeroVector);
		const auto MinIntX = static_cast<uint32>(FMath::Max(0, FMath::FloorToInt(MinScreenPosition.X)));
        const auto MinIntY = static_cast<uint32>(FMath::Max(0, FMath::FloorToInt(MinScreenPosition.Y)));
        const auto MaxIntX = static_cast<uint32>(FMath::Min(RENDER_TEXTURE_SIZE, FMath::CeilToInt(MaxScreenPosition.X)));
        const auto MaxIntY = static_cast<uint32>(FMath::Min(RENDER_TEXTURE_SIZE, FMath::CeilToInt(MaxScreenPosition.Y)));
        ScissorArea = { MinIntX, MinIntY, MaxIntX, MaxIntY };
        RedrawArea = {
        	screen_position_to_world_position(FVector2D{ static_cast<double>(MinIntX), static_cast<double>(MinIntY) }),
			screen_position_to_world_position(FVector2D{ static_cast<double>(MaxIntX), static_cast<double>(MaxIntY) })
        };
		CARTO_LOG_DEBUG("RedrawArea: %s", *RedrawArea.ToString());
	}

	FCanvasTileItem ClearItem{
		{ 0, 0 },
		{ RENDER_TEXTURE_SIZE, RENDER_TEXTURE_SIZE },
		{ 0, 0, 0, 0 }
	};
	ClearItem.BlendMode = SE_BLEND_Opaque;

	Canvas->DrawItem(ClearItem);


    const int32 Min = Algo::LowerBound(CurrentBuildingData, MinZFilter);
    const int32 Max = Algo::UpperBound(CurrentBuildingData, MaxZFilter);
    if (Min >= CurrentBuildingData.Num() || Max <= 0)
    {
		IsRedrawingEntirely = false;
		RedrawArea = {};
		RedrawArea.bIsValid = false;
        co_return;
    }

	CARTO_LOG_DEBUG("From %d to %d out of %d", Min, Max, CurrentBuildingData.Num());

	TArray<int32> BuildingsToDraw;
	if (IsRedrawingEntirely)
	{
        BuildingsToDraw.Reserve(Max - Min);
        for (int32 i = Min; i < Max; i++)
        {
			BuildingsToDraw.Add(i);
        }
	}
	else
	{
		CurrentBuildingQuadTree.GetElements(RedrawArea, BuildingsToDraw);
		co_await Budget;

        for (int32& Index : BuildingsToDraw)
        {
            Index = BuildingDataIndexRedirector[Index];
        }
		Algo::Sort(BuildingsToDraw);
        co_await Budget;

		const int* MinIt = Algo::FindByPredicate(BuildingsToDraw, [Min](int32 Index) { return Index >= Min; });
		const int* MaxIt = Algo::FindLastByPredicate(BuildingsToDraw, [Max](int32 Index) { return Index < Max; });
        if (!MinIt || !MaxIt)
        {
			IsRedrawingEntirely = false;
			RedrawArea = {};
			RedrawArea.bIsValid = false;
            co_return;
        }

		const int* Beg = BuildingsToDraw.GetData();
        BuildingsToDraw.RemoveAt(MaxIt - Beg + 1, BuildingsToDraw.Num() - (MaxIt - Beg + 1), false);
        BuildingsToDraw.RemoveAt(0, MinIt - Beg);
		co_await Budget;

		CARTO_LOG_DEBUG("Overlapping Elements: %d", BuildingsToDraw.Num());
	}

	for (int32 i : BuildingsToDraw)
	{
        const auto& [ClassHash, Transform/*, CustomizationData*/, BuildableExtraData, 
			DataType, DataCache, LayerDataCache, VisualBoxCache] = CurrentBuildingData[i];

		CARTO_LOG_VERY_VERBOSE("Buildable: %u, Transform: %s", ClassHash, *Transform.ToString());

		if (RuntimeConfig.DisabledLayerBuildable.Contains(ClassHash))
		{
            continue;
		}
		if (LayerDataCache)
		{
			if (RuntimeConfig.DisabledLayerMainCategory.Contains(LayerDataCache->MainCategoryCache))
			{
				continue;
			}
			if (const TSet<FName>* SubCategories = RuntimeConfig.DisabledLayerSubCategory.Find(LayerDataCache->MainCategoryCache))
			{
				if (SubCategories->Contains(LayerDataCache->SubCategoryCache))
				{
					continue;
				}
			}
		}

		switch (DataType)
		{
		case EBuildingDataType::Invalid:
			break;

		case EBuildingDataType::Icon:
		{
			const FNormalDataCache* NormalDataCachePtr = std::get_if<FNormalDataCache>(&DataCache);
			CARTO_LOG_ERROR_BREAK_IF_NULL(NormalDataCachePtr);

			const auto& [ScreenPosition, Size, Rotation, IconOrRectangleData] = *NormalDataCachePtr;
            const TSoftObjectPtr<UTexture2D>* Texture = std::get_if<TSoftObjectPtr<UTexture2D>>(&IconOrRectangleData);
            CARTO_LOG_ERROR_BREAK_IF_NULL(Texture);

			// The texture might have gotten unloaded between redraws, so we can't cache it.
			const UTexture2D* LoadedTexture = Texture->Get();
			if (!LoadedTexture)
			{
				LoadedTexture = co_await UE5Coro::Latent::AsyncLoadObject(*Texture);
			}
			CARTO_LOG_ERROR_BREAK_IF_NULL(LoadedTexture);

			FCanvasTileItem TileItem{
				ScreenPosition,
				LoadedTexture->GetResource(),
				{ Size.X * PIXEL_PER_CENTIMETER[0], Size.Y * PIXEL_PER_CENTIMETER[1] },
				{ 0, 0 },
				{ 1, 1 },
				FLinearColor::White
			};
			TileItem.PivotPoint = { 0.5, 0.5 };
			TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Translucent);
			TileItem.Rotation = Rotation;

			Canvas->DrawItem(TileItem);

			break;
		}

		case EBuildingDataType::Rectangle:
		{
			const FNormalDataCache* NormalDataCachePtr = std::get_if<FNormalDataCache>(&DataCache);
            CARTO_LOG_ERROR_BREAK_IF_NULL(NormalDataCachePtr);

			const auto& [ScreenPosition, Size, Rotation, IconOrRectangleData] = *NormalDataCachePtr;
			const FRectangleDataCache* RectangleDataCachePtr = std::get_if<FRectangleDataCache>(&IconOrRectangleData);
			CARTO_LOG_ERROR_BREAK_IF_NULL(RectangleDataCachePtr);

			const auto& [CategoryData, Corners] = *RectangleDataCachePtr;
			CARTO_LOG_ERROR_BREAK_IF_NULL(CategoryData);

			FCanvasTileItem TileItem{
				ScreenPosition,
				{ Size.X * PIXEL_PER_CENTIMETER[0], Size.Y * PIXEL_PER_CENTIMETER[1] },
				CategoryData->MainColor
			};
			TileItem.PivotPoint = { 0.5, 0.5 };
			TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Translucent);
			TileItem.Rotation = Rotation;

			Canvas->DrawItem(TileItem);
			co_await Budget;

			if (CategoryData->OutlineThickness > 0)
			{
				draw_line(Canvas, Corners[0], Corners[1], CategoryData->OutlineColor, CategoryData->OutlineThickness);
				co_await Budget;
				draw_line(Canvas, Corners[1], Corners[2], CategoryData->OutlineColor, CategoryData->OutlineThickness);
				co_await Budget;
				draw_line(Canvas, Corners[2], Corners[3], CategoryData->OutlineColor, CategoryData->OutlineThickness);
				co_await Budget;
				draw_line(Canvas, Corners[3], Corners[0], CategoryData->OutlineColor, CategoryData->OutlineThickness);
			}

			break;
		}

		case EBuildingDataType::Spline:
		{
			const FSplineDataCache* SplineDataCachePtr = std::get_if<FSplineDataCache>(&DataCache);
            CARTO_LOG_ERROR_BREAK_IF_NULL(SplineDataCachePtr);

			const auto& [SplineData, StartPoints, EndPoints] = *SplineDataCachePtr;
			CARTO_LOG_ERROR_BREAK_IF_NULL(SplineData);

            const int Num = StartPoints.Num();
            for (int j = 0; j < Num; j++)
            {
                draw_line(Canvas, StartPoints[j], EndPoints[j], SplineData->Color, SplineData->Thickness);
                co_await Budget;
            }

			break;
		}

		case EBuildingDataType::Wire:
		{
			const FWireExtraData* WireExtraData = std::get_if<FWireExtraData>(&BuildableExtraData);
			CARTO_LOG_ERROR_BREAK_IF_NULL(WireExtraData);

			const FWireData* const* WireDataPtr = std::get_if<const FWireData*>(&DataCache);
			CARTO_LOG_ERROR_BREAK_IF_NULL(WireDataPtr);
            const FWireData* WireData = *WireDataPtr;

			draw_line(Canvas, Transform.GetLocation(), WireExtraData->End, WireData->Color, WireData->Thickness);

			break;
		}

        case EBuildingDataType::Beam:
		{
			const FBeamExtraData* BeamExtraData = std::get_if<FBeamExtraData>(&BuildableExtraData);
			CARTO_LOG_ERROR_BREAK_IF_NULL(BeamExtraData);

			const FWireData* const* BeamDataPtr = std::get_if<const FWireData*>(&DataCache);
            CARTO_LOG_ERROR_BREAK_IF_NULL(BeamDataPtr);
            const FWireData* BeamData = *BeamDataPtr;

			const float Length = BeamExtraData->Length;
			const FVector Start = Transform.GetLocation();
			const FVector End = Start + Transform.GetRotation().Vector() * Length;
			draw_line(Canvas, Start, End, BeamData->Color, BeamData->Thickness);

            break;
		}

		default:
			break;
		}

		if constexpr (DRAW_BOUNDARIES)
		{
            constexpr FLinearColor Color{ 1, 0, 1, 1 };
            constexpr float Thickness = 2;

			const FVector2D MinPoint = VisualBoxCache.Min;
            const FVector2D MaxPoint = VisualBoxCache.Max;
            draw_line(Canvas, FVector2D{ MinPoint.X, MinPoint.Y }, FVector2D{ MaxPoint.X, MinPoint.Y }, Color, Thickness);
			draw_line(Canvas, FVector2D{ MaxPoint.X, MinPoint.Y }, FVector2D{ MaxPoint.X, MaxPoint.Y }, Color, Thickness);
			draw_line(Canvas, FVector2D{ MaxPoint.X, MaxPoint.Y }, FVector2D{ MinPoint.X, MaxPoint.Y }, Color, Thickness);
			draw_line(Canvas, FVector2D{ MinPoint.X, MaxPoint.Y }, FVector2D{ MinPoint.X, MinPoint.Y }, Color, Thickness);
		}

		co_await Budget;
	}

	IsRedrawingEntirely = false;
	RedrawArea = {};
	RedrawArea.bIsValid = false;

	CARTO_LOG("RedrawMapCoroutine Finished");
}


void UCartographGameInstanceModule::OnCoroutineFinishedOrCancelled()
{
    CARTO_LOG_DEBUG("OnCoroutineFinishedOrCancelled");

    if (RenderContext.RenderTarget)
    {
        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, RenderContext);
		RenderContext = {};
    }

	if (!IsPendingRedraw)
	{
		return;
	}

	// The coroutine is also on the game thread, so I think no data race here.
    IsPendingRedraw = false;
	ExecuteRedrawMapCoroutine(IsPendingRedrawEntire);
}


void UCartographGameInstanceModule::ExecuteRedrawMapCoroutine(bool bRedrawEntirely)
{
	Coroutine = RedrawMapCoroutine(PendingAddBuildingData, PendingRemoveBuildingData, bRedrawEntirely);
	if (!IsClient)
	{
		if (!ACartographModSubsystem::Instance)
		{
            CARTO_LOG_ERROR("ACartographModSubsystem is not initialized");
		}
		else
		{
			ACartographModSubsystem::Instance->ClientUpdateBuildingData(PendingAddBuildingData, PendingRemoveBuildingData);
		}
	}
	PendingAddBuildingData.Empty();
	PendingRemoveBuildingData.Empty();
}


void UCartographGameInstanceModule::OnZFilterUpdated(float Min, float Max)
{
	MinCached = Min;
    MaxCached = Max;

	const float Length = MaxHeight - MinHeight;
	MinZFilter = FMath::Floor(Min * Length + MinHeight);
	MaxZFilter = FMath::CeilToInt(Max * Length + MinHeight);

	CARTO_LOG("Min is now %f and max is now %f", MinZFilter, MaxZFilter);

	if (!IsInitializing)
	{
		RedrawMap(true);
	}
}
#pragma endregion


#pragma region UI
void UCartographGameInstanceModule::RegisterMenuButton() const
{
	// Copied from WidgetBlueprintHookManager.cpp
	class UCartographPanelWidgetAccessor : UPanelWidget
	{
	public:
		static UClass* GetPanelSlotClass(const UPanelWidget* PanelWidget) {
			return static_cast<const UCartographPanelWidgetAccessor*>(PanelWidget)->GetSlotClass();
		}

		static TArray<UPanelSlot*>& GetPanelSlots(UPanelWidget* PanelWidget) {
			return static_cast<UCartographPanelWidgetAccessor*>(PanelWidget)->Slots;
		}
		UCartographPanelWidgetAccessor() = delete;
	};

	if (!FPlatformProperties::RequiresCookedData() || FPlatformProperties::IsServerOnly()) 
	{
		return;
	}

	const auto* WidgetBlueprintClass = Cast<UWidgetBlueprintGeneratedClass>(MapContainerWidget.LoadSynchronous());
	UWidgetTree* WidgetTree = WidgetBlueprintClass->GetWidgetTreeArchetype();

	UWidget* ShowHideButton = WidgetTree->FindWidget("ShowHideButton");
    CARTO_LOG_ERROR_RETURN_IF_NULL(ShowHideButton);
	int32 Index;
	UPanelWidget* Parent = UWidgetTree::FindWidgetParent(ShowHideButton, Index);

    UHorizontalBox* HBox = NewObject<UHorizontalBox>(WidgetTree, UHorizontalBox::StaticClass(), "MenuShowHideButtonHBox", RF_Transient);

    auto* HBoxPanelSlot = NewObject<UCanvasPanelSlot>(Parent, UCanvasPanelSlot::StaticClass(), NAME_None, RF_Transient);
    HBoxPanelSlot->Content = HBox;
    HBoxPanelSlot->Parent = Parent;
	HBoxPanelSlot->SetPosition({ 6, 6 });
	HBoxPanelSlot->SetAutoSize(true);

    HBox->Slot = HBoxPanelSlot;

	//ShowHideButton->RemoveFromParent();  // AddChild already removes from parent
	HBox->AddChild(ShowHideButton);

	UWidget* CartographMenuShowHideButton = NewObject<UWidget>(HBox, MenuShowHideButtonWidget, "CartographMenuShowHideButton", RF_Transient, ShowHideButton);
    auto* HBoxSlot = Cast<UHorizontalBoxSlot>(HBox->AddChild(CartographMenuShowHideButton));
	HBoxSlot->SetPadding({ 10, 0, 0, 0 });

	FProperty* TextProperty = CartographMenuShowHideButton->GetClass()->FindPropertyByName("mText");
    CARTO_LOG_ERROR_RETURN_IF_NULL(TextProperty);
	FText* TextPtr = TextProperty->ContainerPtrToValuePtr<FText>(CartographMenuShowHideButton);
    CARTO_LOG_ERROR_RETURN_IF_NULL(TextPtr);
    *TextPtr = LOCTEXT("CartographMenuShow", "Show Cartograph Menu");

    TArray<UPanelSlot*>& MutablePanelSlots = UCartographPanelWidgetAccessor::GetPanelSlots(Parent);
	MutablePanelSlots.Insert(HBoxPanelSlot, Index);


	const UWidget* Menu = WidgetTree->FindWidget("BPW_MapMenu");
    CARTO_LOG_ERROR_RETURN_IF_NULL(Menu);
	const auto* MenuPanelSlot = Cast<UCanvasPanelSlot>(Menu->Slot);
    CARTO_LOG_ERROR_RETURN_IF_NULL(MenuPanelSlot);

	UWidget* CartographMenu = NewObject<UWidget>(HBox, MenuWidget, "CartographMenu", RF_Transient);
	auto* CartographMenuPanelSlot = Cast<UCanvasPanelSlot>(Parent->AddChild(CartographMenu));
    CartographMenuPanelSlot->SetLayout(MenuPanelSlot->GetLayout());
    CartographMenuPanelSlot->SetPosition(MenuPanelSlot->GetPosition());
    CartographMenuPanelSlot->SetSize(MenuPanelSlot->GetSize());
    CartographMenuPanelSlot->SetAutoSize(MenuPanelSlot->GetAutoSize());
    CartographMenuPanelSlot->SetZOrder(MenuPanelSlot->GetZOrder());

	CartographMenu->SetVisibility(ESlateVisibility::Collapsed);

    CARTO_LOG("Menu Button Registered");
}


void UCartographGameInstanceModule::LoadRuntimeConfig()
{
#if !UE_SERVER
	const FCartograph_ConfigStruct ConfigInstance = FCartograph_ConfigStruct::GetActiveConfig(GetWorld());

	RuntimeConfig = {};

	// std::getline doesn't support std::string_view :(
	{
        std::wstringstream Stream{ *ConfigInstance.MainCategoryToggle };
        std::wstring Line;
        while (std::getline(Stream, Line, L','))
        {
			if (!Line.empty())
			{
				RuntimeConfig.DisabledLayerMainCategory.Add(FName{ Line.data() });
			}
        }
    }
    {
        std::wstringstream Stream{ *ConfigInstance.SubCategoryToggle };
        std::wstring MainCategoryLine;
        while (std::getline(Stream, MainCategoryLine, L'|'))
        {
			const size_t MainCategoryColonIndex = MainCategoryLine.find(':');
            if (MainCategoryColonIndex == std::wstring::npos)
            {
                CARTO_LOG_ERROR("Invalid BuildingToggle: %s", *ConfigInstance.BuildingToggle);
                break;
            }
            std::wstring MainCategoryName = MainCategoryLine.substr(0, MainCategoryColonIndex);
            const FName MainCategoryFName{ MainCategoryName.data() };

            std::wstringstream SubCategoryStream{ MainCategoryLine.substr(MainCategoryColonIndex + 1) };
            std::wstring SubCategoryLine;
			while (std::getline(SubCategoryStream, SubCategoryLine, L','))
			{
				if (!SubCategoryLine.empty())
				{
					RuntimeConfig.DisabledLayerSubCategory.FindOrAdd(MainCategoryFName).Add(FName{ SubCategoryLine.data() });
				}
			}
        }
	}

	{
		std::wstringstream Stream{ *ConfigInstance.BuildingToggle };
		std::wstring Line;
		while (std::getline(Stream, Line, L','))
		{
			if (!Line.empty())
			{
				RuntimeConfig.DisabledLayerBuildable.Add(std::stoul(Line));
			}
		}
	}

    CARTO_LOG("RuntimeConfig Loaded");
#endif
}


void UCartographGameInstanceModule::SaveRuntimeConfig()
{
#if !UE_SERVER
	const FConfigId ConfigId{ "Cartograph", "" };
	const UConfigManager* ConfigManager = GetWorld()->GetGameInstance()->GetSubsystem<UConfigManager>();
	const UConfigPropertySection* RootSection = ConfigManager->GetConfigurationRootSection(ConfigId);

	{
		UConfigProperty* const* MainCategoryProperty = RootSection->SectionProperties.Find("MainCategoryToggle");
		CARTO_LOG_ERROR_RETURN_IF_NULL(MainCategoryProperty);
		auto* MainCategoryStringProperty = Cast<UConfigPropertyString>(*MainCategoryProperty);
		CARTO_LOG_ERROR_RETURN_IF_NULL(MainCategoryStringProperty);

        TStringBuilder<500> Builder;
        for (const FName& Name : RuntimeConfig.DisabledLayerMainCategory)
        {
			Builder.Appendf(TEXT("%s,"), *Name.ToString());
        }
        MainCategoryStringProperty->Value = Builder.ToString();
		MainCategoryStringProperty->MarkDirty();
	}
	{
        UConfigProperty* const* SubCategoryProperty = RootSection->SectionProperties.Find("SubCategoryToggle");
        CARTO_LOG_ERROR_RETURN_IF_NULL(SubCategoryProperty);
        auto* SubCategoryStringProperty = Cast<UConfigPropertyString>(*SubCategoryProperty);
        CARTO_LOG_ERROR_RETURN_IF_NULL(SubCategoryStringProperty);
        TStringBuilder<1000> Builder;
        for (const auto& [MainCategory, SubCategories] : RuntimeConfig.DisabledLayerSubCategory)
        {
            Builder.Appendf(TEXT("%s:"), *MainCategory.ToString());
            for (const FName& Name : SubCategories)
            {
				Builder.Appendf(TEXT("%s,"), *Name.ToString());
            }
            Builder.Append(TEXT("|"));
        }
        SubCategoryStringProperty->Value = Builder.ToString();
        SubCategoryStringProperty->MarkDirty();
    }
    {
        UConfigProperty* const* BuildingProperty = RootSection->SectionProperties.Find("BuildingToggle");
        CARTO_LOG_ERROR_RETURN_IF_NULL(BuildingProperty);
        auto* BuildingStringProperty = Cast<UConfigPropertyString>(*BuildingProperty);
        CARTO_LOG_ERROR_RETURN_IF_NULL(BuildingStringProperty);
        TStringBuilder<11 * 551> Builder;
        for (const uint32& Hash : RuntimeConfig.DisabledLayerBuildable)
        {
			Builder.Appendf(TEXT("%u,"), Hash);
        }
        BuildingStringProperty->Value = Builder.ToString();
        BuildingStringProperty->MarkDirty();
	}

    CARTO_LOG("RuntimeConfig Saved");
#endif
}


void UCartographGameInstanceModule::FillBuildLayerDataCache()
{
	if (!BuildLayerDataMapCache.IsEmpty())
	{
		return;
	}

	for (const auto& [OriginalBuildableClass, _] : ClassPtrToClassIDMap)
	{
		if (!OriginalBuildableClass || BuildableToIgnore.Contains(OriginalBuildableClass.Get()))
		{
			continue;
		}

		const TSoftClassPtr<AFGBuildable>* RedirectClass = BuildableClassRedirectMap.Find(OriginalBuildableClass.Get());
		const TSubclassOf<AFGBuildable> BuildableClass = RedirectClass ? RedirectClass->LoadSynchronous() : OriginalBuildableClass.Get();

		const uint32* BuildableClassHash = ClassPtrToClassIDMap.Find(BuildableClass);
		if (!BuildableClassHash)
		{
			CARTO_LOG_ERROR("Can't find hash for %s", *BuildableClass->GetName());
			continue;
		}


        const FBuildLayerData* LayerData = GetDataByBuildableClass(BuildableBuildLayerDataOverrideMap, BuildLayerDataMap, BuildableClass);
		if (!LayerData && ModdedBuildings.Contains(BuildableClass.Get()))
		{
			const FString& ModName = *ModdedBuildings.Find(BuildableClass.Get());
            LayerData = ModdedBuildLayerData.Find(ModName);
		}
		if (!LayerData)
		{
            CARTO_LOG_WARNING("Can't find layer data for %s", *BuildableClass->GetName());
            continue;
		}

		if (LayerData->MainCategoryCache.IsNone())
		{
			TArray<FString> CategoryNames;
			LayerData->Category.ParseIntoArray(CategoryNames, TEXT("/"));

			const_cast<FBuildLayerData*>(LayerData)->MainCategoryCache = FName{ CategoryNames[0] };
			const_cast<FBuildLayerData*>(LayerData)->SubCategoryCache = CategoryNames.Num() > 1 ? FName{ CategoryNames[1] } : NAME_None;
		}

		BuildLayerDataMapCache.Add(*BuildableClassHash, LayerData);
	}

    CARTO_LOG("BuildLayerDataCache Filled. Count: %d", BuildLayerDataMapCache.Num());
}


void UCartographGameInstanceModule::OnBuildingDataAdd(const FBuildingData& AddedBuildingData, int32 Pos)
{
	if (AddedBuildingData.VisualBoxCache.bIsValid)
	{
		CurrentBuildingQuadTree.Insert(BuildingDataIndexRedirector.Num(), AddedBuildingData.VisualBoxCache);
		for (int32& Index : BuildingDataIndexRedirector)
		{
			if (Index >= Pos)
			{
				Index++;
			}
		}
		BuildingDataIndexRedirector.Add(Pos);
	}

	BuildingCountMap.FindOrAdd(AddedBuildingData.BuildableClassHash)++;
}


void UCartographGameInstanceModule::OnBuildingDataRemove(const FBuildingData& RemovedBuildingData, int32 Pos)
{
	if (RemovedBuildingData.VisualBoxCache.bIsValid)
	{
		const int32 Num = BuildingDataIndexRedirector.Num();
		for (int32 i = 0; i < Num; i++)
		{
			int32& BuildingDataArrayIndex = BuildingDataIndexRedirector[i];
			if (BuildingDataArrayIndex == Pos)
			{
				CurrentBuildingQuadTree.Remove(i, RemovedBuildingData.VisualBoxCache);
				BuildingDataArrayIndex = -1;
			}
			else if (BuildingDataArrayIndex > Pos)
			{
				BuildingDataArrayIndex--;
			}
		}
	}

	BuildingCountMap.FindChecked(RemovedBuildingData.BuildableClassHash)--;
}


void UCartographGameInstanceModule::OnCartographMenuButtonClicked(UUserWidget* Widget, bool IsOpen)
{
	for (UWidget* ChildWidget : Widget->GetParent()->GetParent()->GetAllChildren())
	{
		if (ChildWidget->GetName() == "CartographMenu")
		{
			ChildWidget->SetVisibility(IsOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			break;
		}
	}

	if (IsOpen)
	{
		auto* RootWidget = Cast<UWidget>(Widget->GetParent()->GetOuter()->GetOuter());
        CARTO_LOG_ERROR_RETURN_IF_NULL(RootWidget);
		FOutputDeviceNull Ar;
		RootWidget->CallFunctionByNameWithArguments(TEXT("SetFiltersCollapsed 1"), Ar, nullptr, true);
	}
}


void UCartographGameInstanceModule::OnShowBuildingsCheckboxChanged(bool DoShow)
{
    DoShowBuildings = DoShow;
    CARTO_LOG("DoShowBuildings: %d", DoShowBuildings);
}


TArray<FString> UCartographGameInstanceModule::GetLayerCategoryOptions() const
{
	TArray<FString> Options;
	for (const FLayerCategoryData& LayerCategory : LayerCategories)
	{
		Options.Add(LayerCategory.Name.ToString());

		for (const FLayerSubCategoryData& LayerSubCategory : LayerCategory.SubCategories)
		{
			Options.Add(FString::Printf(TEXT("%s/%s"), *LayerCategory.Name.ToString(), *LayerSubCategory.Name.ToString()));
		}
	}
	return Options;
}
#pragma endregion


void UCartographGameInstanceModule::GatherBuildables()
{
    ClassIDToClassPtrMap.Empty();
    ClassPtrToClassIDMap.Empty();
    ClassPtrToDescriptorDataMap.Empty();

	{
		TArray<UClass*> NativeRootClasses;
		NativeRootClasses.Add(AFGBuildable::StaticClass());
		GetDerivedClasses(AFGBuildable::StaticClass(), NativeRootClasses);

		TArray<FTopLevelAssetPath> NativeRootClassPaths;

		Algo::TransformIf(NativeRootClasses,
			NativeRootClassPaths,
			[](const UClass* RootClass) { return RootClass && RootClass->HasAnyClassFlags(CLASS_Native); },
			&UClass::GetClassPathName);

		TSet<FTopLevelAssetPath> AllClassPaths;
		IAssetRegistry::Get()->GetDerivedClassNames(NativeRootClassPaths, {}, AllClassPaths);

		for (const FTopLevelAssetPath& AssetPath : AllClassPaths)
		{
			const TSubclassOf<AFGBuildable> Class = StaticLoadClass(AFGBuildable::StaticClass(), nullptr, *AssetPath.ToString());
			const FString Name = Class->GetName();
			const FString PackageName = AssetPath.GetPackageName().ToString();
			if (PackageName.StartsWith("/Script")
				|| Name.StartsWith("SKEL_") || Name.StartsWith("REINST_"))
			{
				continue;
			}

			if (!PackageName.StartsWith("/Game/"))
			{
				const int32 Index = PackageName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 2);
				const FString ModName = PackageName.Mid(1, Index - 1);
				ModdedBuildings.Add(Class.Get(), ModName);
				FLayerCategoryData* ModdedCategory = LayerCategories.FindByPredicate(
					[](const FLayerCategoryData& CategoryData) { return CategoryData.Name == UnspecifiedMainCategory; });
				CARTO_LOG_ERROR_RETURN_IF_NULL(ModdedCategory);
                if (!ModdedCategory->SubCategories.FindByPredicate(
					[ModFName = FName{ ModName }](const FLayerSubCategoryData& CategoryData) { return CategoryData.Name == ModFName; }))
                {
                    ModdedCategory->SubCategories.Add(FLayerSubCategoryData{
                        .Name = FName{ ModName },
                        .DisplayName = FText::FromString(ModName),
						});
                }

				if (!ModdedBuildLayerData.Contains(ModName))
				{
					ModdedBuildLayerData.Add(ModName, FBuildLayerData{
						.MainCategoryCache = FName{ UnspecifiedMainCategory },
						.SubCategoryCache = FName{ ModName },
						});
				}
			}

			const uint32 Hash = TextKeyUtil::HashString(AssetPath.ToString());
			ClassPtrToClassIDMap.Add(Class, Hash);
			ClassIDToClassPtrMap.Add(Hash, Class);
			CARTO_LOG_DEBUG("Path: %s, Class: %s, Hash: %u", *AssetPath.ToString(), *Name, Hash);
		}
	}
	{
		TArray<UClass*> NativeRootClasses;
		NativeRootClasses.Add(UFGBuildingDescriptor::StaticClass());
		GetDerivedClasses(UFGBuildingDescriptor::StaticClass(), NativeRootClasses);

		TArray<FTopLevelAssetPath> NativeRootClassPaths;

		Algo::TransformIf(NativeRootClasses,
			NativeRootClassPaths,
			[](const UClass* RootClass) { return RootClass && RootClass->HasAnyClassFlags(CLASS_Native); },
			&UClass::GetClassPathName);

		TSet<FTopLevelAssetPath> AllClassPaths;
		IAssetRegistry::Get()->GetDerivedClassNames(NativeRootClassPaths, {}, AllClassPaths);

		for (const FTopLevelAssetPath& AssetPath : AllClassPaths)
		{
			const TSubclassOf<UFGBuildingDescriptor> Descriptor = StaticLoadClass(UFGBuildingDescriptor::StaticClass(), nullptr, *AssetPath.ToString());
			const FString Name = Descriptor->GetName();
			const FString PackageName = AssetPath.GetPackageName().ToString();
			if (PackageName.StartsWith("/Script")
				|| Name.StartsWith("SKEL_") || Name.StartsWith("REINST_"))
			{
				continue;
			}

			auto* DescriptorInstance = Cast<UFGBuildingDescriptor>(Descriptor->ClassDefaultObject);
			TSubclassOf<AFGBuildable> BuildableClass = DescriptorInstance->mBuildableClass;
			if (!BuildableClass)
			{
				continue;
			}

			TSubclassOf<UFGBuildSubCategory> BuildSubCategory;
			for (const TSubclassOf<UFGCategory> SubCategory : DescriptorInstance->mSubCategories)
			{
                if (!SubCategory)
                {
                    continue;
                }

				if (SubCategory->IsChildOf(UFGBuildSubCategory::StaticClass()))
				{
					BuildSubCategory = SubCategory;
					break;
				}
			}

            UTexture2D* Icon = DescriptorInstance->mSmallIcon;
			if (!Icon)
			{
                // Some buildings like blueprint designers don't have small icon
				Icon = DescriptorInstance->mPersistentBigIcon;
			}

            ClassPtrToDescriptorDataMap.Add(BuildableClass, FBuildingDescriptorData{
				.Category = DescriptorInstance->mCategory,
				.SubCategory = BuildSubCategory,
				.Icon = Icon,
            });
			
			CARTO_LOG_DEBUG("Path: %s, Class: %s, BuildableClass: %s, NoIcon: %d",
				*AssetPath.ToString(), 
				*Name, 
				*BuildableClass->GetName(),
				Icon == nullptr);
		}
	}

    CARTO_LOG("Buildables Gathered. Buildable: %d, Descriptor: %d", ClassPtrToClassIDMap.Num(), ClassPtrToDescriptorDataMap.Num());
}


#undef LOCTEXT_NAMESPACE
