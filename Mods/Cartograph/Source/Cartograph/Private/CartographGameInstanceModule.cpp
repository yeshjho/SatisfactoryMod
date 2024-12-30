#include "CartographGameInstanceModule.h"

#include "AssetRegistryModule.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"

#include "FGLightweightBuildableSubsystem.h"
#include "FGBuildable.h"
#include "FGBuildableBeam.h"
#include "FGBuildableSubsystem.h"
#include "FGBuildableWire.h"
#include "FGBuildingDescriptor.h"
#include "FGBuildCategory.h"
#include "FGBuildSubCategory.h"
#include "FGPlayerController.h"
#include "FGRecipeManager.h"
#include "FGSaveSession.h"
#include "FGSplineBuildableInterface.h"

#include "Patching/NativeHookManager.h"

#include "CartographModSubsystem.h"
#include "CartographRemoteCallObject.h"
#include "Cartograph_ConfigStruct.h"
#include "QuantizedVector2DSerialization.h"


constexpr int RENDER_TEXTURE_SIZE = 1024 * 8;

constexpr double WEST_BOUND_CENTIMETERS = -324'698.832031;
constexpr double EAST_BOUND_CENTIMETERS = 425'301.832031;
constexpr double NORTH_BOUND_CENTIMETERS = -375'000;
constexpr double SOUTH_BOUND_CENTIMETERS = 375'000;
constexpr double MAP_WIDTH_CENTIMETERS = EAST_BOUND_CENTIMETERS - WEST_BOUND_CENTIMETERS;
constexpr double MAP_HEIGHT_CENTIMETERS = SOUTH_BOUND_CENTIMETERS - NORTH_BOUND_CENTIMETERS;
constexpr double ORIGIN_UV[] = { -WEST_BOUND_CENTIMETERS / MAP_WIDTH_CENTIMETERS, -NORTH_BOUND_CENTIMETERS / MAP_HEIGHT_CENTIMETERS };
constexpr double PIXEL_PER_CENTIMETER[] = { RENDER_TEXTURE_SIZE / MAP_WIDTH_CENTIMETERS, RENDER_TEXTURE_SIZE / MAP_HEIGHT_CENTIMETERS };


DEFINE_LOG_CATEGORY(LogCartograph);


template<typename T, typename U>
    requires
		(std::is_same_v<T, FVector> || std::is_same_v<T, FVector2D>) &&
		(std::is_same_v<U, FVector> || std::is_same_v<U, FVector2D>)
FVector2D world_position_to_screen_position(const T& WorldPosition, const U& Size)
{
	return FVector2D{
		// TODO: Width / 2 & Height / 2: Only verified for foundations
		ORIGIN_UV[0] + (WorldPosition.X - Size.X / 2) / MAP_WIDTH_CENTIMETERS,
		ORIGIN_UV[1] + (WorldPosition.Y - Size.Y / 2) / MAP_HEIGHT_CENTIMETERS
	} * RENDER_TEXTURE_SIZE;
}

		
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


auto FBuildingData::operator<=>(const FBuildingData& Other) const noexcept
{
	return Transform.GetLocation().Z <=> Other.Transform.GetLocation().Z;
}


FArchive& operator<<(FArchive& Ar, TArray<FVector2D>& A)
{
	A.CountBytes(Ar);

	using SizeType = int32;
	SizeType SerializeNum = Ar.IsLoading() ? 0 : A.Num();

	Ar << SerializeNum;

	if (SerializeNum == 0)
	{
		// if we are loading, then we have to reset the size to 0, in case it isn't currently 0
		if (Ar.IsLoading())
		{
			A.Empty();
		}
		return Ar;
	}

	if (Ar.IsLoading())
	{
		// Required for resetting ArrayNum
		A.Empty(SerializeNum);

		for (SizeType i = 0; i < SerializeNum; i++)
		{
			SerializeQuantizedVector2D<1>(A.AddDefaulted_GetRef(), Ar);
		}
	}
	else
	{
		for (SizeType i = 0; i < SerializeNum; i++)
		{
			SerializeQuantizedVector2D<1>(A[i], Ar);
		}
	}

	return Ar;
}


FArchive& operator<<(FArchive& Ar, std::monostate&)
{
    return Ar;
}


FArchive& operator<<(FArchive& Ar, FSplineExtraData& SplineData)
{
    Ar << SplineData.SplinePoints;

    if (!Ar.IsLoading())  // Serialize
	{
		bool IsSet = SplineData.Tangents.IsSet();
		Ar.SerializeBits(&IsSet, 1);

		if (IsSet)
		{
			auto& [LeaveTangents, ArriveTangents] = SplineData.Tangents.GetValue();
			Ar << LeaveTangents;
			Ar << ArriveTangents;
		}
	}
    else  // Deserialize
	{
        bool IsSet;
        Ar.SerializeBits(&IsSet, 1);
		if (IsSet)
		{
            TArray<FVector2D> LeaveTangents, ArriveTangents;
            Ar << LeaveTangents;
            Ar << ArriveTangents;
            SplineData.Tangents = std::make_pair(std::move(LeaveTangents), std::move(ArriveTangents));
		}
	}
    
	return Ar;
}


FArchive& operator<<(FArchive& Ar, FWireExtraData& WireData)
{
	SerializeQuantizedVector2D<1>(WireData.End, Ar);
	return Ar;
}


FArchive& operator<<(FArchive& Ar, FBeamExtraData& BeamData)
{
	if (!Ar.IsLoading())  // Serialize
	{
		WriteFixedCompressedFloat<16384, 16>(BeamData.Length, Ar);
	}
    else  // Deserialize
    {
        ReadFixedCompressedFloat<16384, 16>(BeamData.Length, Ar);
    }
	return Ar;
}


bool FBuildingData::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	// We'll serialize the class name since deserializing UObject* from pure memory is such a pain.
	// Hashing it to reduce the size.

	uint32 ClassIDHash = 0;
	if (!Ar.IsLoading())  // Serialize
	{
		if (const uint32* ClassID = UCartographGameInstanceModule::Instance->ClassPtrToClassIDMap.Find(BuildableClass))
        {
            ClassIDHash = *ClassID;
		}
		else
		{
            UE_LOG(LogCartograph, Warning, TEXT("Class %s not found in ClassPtrToClassIDMap"), *BuildableClass->GetName());
		}
		Ar.SerializeBits(&ClassIDHash, 32);
	}
    else  // Deserialize
	{
		Ar.SerializeBits(&ClassIDHash, 32);
        if (const TSubclassOf<AFGBuildable>* Class = UCartographGameInstanceModule::Instance->ClassIDToClassPtrMap.Find(ClassIDHash))
        {
            BuildableClass = *Class;
        }
        else
        {
            UE_LOG(LogCartograph, Warning, TEXT("Class ID %u not found in ClassIDToClassPtrMap"), ClassIDHash);
        }
	}

	if (!Ar.IsLoading())  // Serialize
	{
		FVector Location = Transform.GetLocation();
		bOutSuccess &= SerializePackedVector<1, 24>(Location, Ar);
		Transform.Rotator().SerializeCompressedShort(Ar);

		// We really need to pack the data, so we'll ignore scales for the clients.
		//FVector Scale = Transform.GetScale3D();
        //bOutSuccess &= SerializePackedVector<1, 24>(Scale, Ar);
	}
	else
	{
        FVector Location;
        bOutSuccess &= SerializePackedVector<1, 24>(Location, Ar);
        FRotator Rotation;
        Rotation.SerializeCompressedShort(Ar);
        Transform = FTransform{ Rotation, Location };

        //FVector Scale;
        //bOutSuccess &= SerializePackedVector<1, 24>(Scale, Ar);
        //Transform.SetScale3D(Scale);
    }

    //Ar << CustomizationData;

    int ExtraDataIndex = BuildableExtraData.index();
    Ar.SerializeBits(&ExtraDataIndex, 2);  // NOTE: Increase the bits if more extra data types are added
	if (!Ar.IsLoading())  // Serialize
	{
        std::visit([&Ar](auto&& Data)
            {
                Ar << Data;
            },
            BuildableExtraData
        );
	}
    else  // Deserialize
	{
        switch (ExtraDataIndex)
        {
        case 0:
            BuildableExtraData = std::monostate{};
            break;
		case 1:
		{
			FSplineExtraData SplineData{};
			Ar << SplineData;
            BuildableExtraData = std::move(SplineData);
			break;
		}
		case 2:
		{
            FWireExtraData WireData{};
            Ar << WireData;
            BuildableExtraData = std::move(WireData);
			break;
		}
		case 3:
		{
            FBeamExtraData BeamData{};
            Ar << BeamData;
            BuildableExtraData = std::move(BeamData);
			break;
		}
        default:
            bOutSuccess = false;
            return false;
        }
    }
    bOutSuccess = true;
    return true;
}


bool FBuildingData::operator==(const FBuildingData& Other) const noexcept
{
    return BuildableClass == Other.BuildableClass && Transform.Equals(Other.Transform) && BuildableExtraData == Other.BuildableExtraData;
    // Ignoring CustomizationData on purpose
}


FArchive& operator<<(FArchive& Ar, FBuildingData& BuildingData)
{
	bool _;
    BuildingData.NetSerialize(Ar, nullptr, _);
    return Ar;
}


void UCartographGameInstanceModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
	Super::DispatchLifecycleEvent(Phase);

    if (Phase != ELifecyclePhase::POST_INITIALIZATION)
    {
        return;
    }


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
	

	const auto LambdaAfterLoadGame =
		[this](bool ReturnValue, UFGSaveSession* Instance, const FString& SaveName)
		{
			CARTO_LOG_DEBUG("LoadGame");

			IsClient = false;
			ShouldInitialize = true;
            // Wait for ACartographModSubsystem to initialize
			GetWorld()->GetTimerManager().SetTimerForNextTick(
				[this, Instance]()
				{
					if (!ShouldInitialize)
					{
						return;
					}

					ShouldInitialize = false;
					IsInitializing = true;

					TArray<TWeakObjectPtr<AFGBuildable>> Factories;
					Algo::Transform(AFGBuildableSubsystem::Get(Instance)->GetAllBuildablesRef(), Factories,
						[](AFGBuildable* Buildable) { return Buildable; });
					Coroutine = InitialBuildableGather(
						std::move(Factories),
						AFGLightweightBuildableSubsystem::Get(Instance)->mBuildableClassToInstanceArray
					);
				});
		};


    const auto LambdaAfterAddFromBuildableInstanceData = 
        [this](int32 ReturnValue, AFGLightweightBuildableSubsystem* Instance, TSubclassOf<AFGBuildable> BuildableClass, 
            FRuntimeBuildableInstanceData& BuildableInstanceData, bool FromSaveData = false, int32 SaveDataBuildableIndex = INDEX_NONE, 
            uint16 ConstructId = MAX_uint16, AActor* BuildEffectInstigator = nullptr, int32 BlueprintBuildEffectIndex = INDEX_NONE)
        {
			CARTO_LOG_VERBOSE("AddFromBuildableInstanceData: %s, Skip: %d", *BuildableClass->GetName(), ShouldInitialize || FromSaveData || IsClient);

			if (ShouldInitialize || FromSaveData || IsClient)
			{
				return;
			}

			FBuildingData Data{
					.BuildableClass = BuildableClass,
					.Transform = BuildableInstanceData.Transform,
					//.CustomizationData = BuildableInstanceData.CustomizationData,
			};
			PendingAddBuildingData.Add(std::move(Data));

			RedrawMap();
        };


	const auto LambdaAfterAddFromReplicatedData =
		[this](AFGLightweightBuildableSubsystem* Instance, TSubclassOf<AFGBuildable> BuildableClass, TSubclassOf<UFGRecipe> BuiltWithRecipe, 
			const FLightweightBuildableReplicationItem& ReplicationData, int32 MaxSize, 
			AActor* BuildEffectInstigator, int32 BlueprintBuildIndex)
		{
			CARTO_LOG_VERBOSE("AddFromReplicatedData: %s, Skip: %d", *BuildableClass->GetName(), ShouldInitialize || IsClient);

			if (ShouldInitialize || IsClient)
			{
				return;
			}

			FBuildingData Data{
					.BuildableClass = BuildableClass,
					.Transform = ReplicationData.Transform,
					//.CustomizationData = ReplicationData.CustomizationData,
			};
			PendingAddBuildingData.Add(std::move(Data));

			RedrawMap();
		};


	const auto LambdaAfterAddBuildable =
		[this](AFGBuildableSubsystem* Instance, AFGBuildable* Buildable)
		{
			CARTO_LOG_VERBOSE("AddBuildable: %s, Skip: %d", *Buildable->GetClass()->GetName(), ShouldInitialize || IsClient);

			if (ShouldInitialize || IsClient)
			{
				return;
			}

			FBuildingData Data{
					.BuildableClass = Buildable->GetClass(),
					.Transform = Buildable->GetTransform(),
					//.CustomizationData = Buildable->GetCustomizationData_Native(),
			};
			AddExtraData(Data, Buildable);
			PendingAddBuildingData.Add(std::move(Data));

			RedrawMap();
		};


	const auto LambdaAfterInvalidateRuntimeInstanceDataForIndex =
		[this](AFGLightweightBuildableSubsystem* Instance, TSubclassOf<AFGBuildable> BuildableClass, int32 Index)
		{
			CARTO_LOG_VERBOSE("InvalidateRuntimeInstanceDataForIndex: %s, Skip: %d", *BuildableClass->GetName(), ShouldInitialize || IsClient);

			if (ShouldInitialize || IsClient)
			{
				return;
			}

			const FRuntimeBuildableInstanceData* LightweightData = Instance->GetRuntimeDataForBuildableClassAndIndex(BuildableClass, Index);

			FBuildingData Data{
					.BuildableClass = BuildableClass,
					.Transform = LightweightData->Transform,
					//.CustomizationData = Data->CustomizationData,
			};
            PendingRemoveBuildingData.Add(std::move(Data));

			RedrawMap();
		};


	const auto LambdaAfterRemoveBuildable =
		[this](AFGBuildableSubsystem* Instance, AFGBuildable* Buildable)
		{
			CARTO_LOG_VERBOSE("RemoveBuildable: %s, Skip: %d", *Buildable->GetClass()->GetName(), ShouldInitialize || IsClient);

			if (ShouldInitialize || IsClient)
			{
				return;
			}

            FBuildingData Data{
                    .BuildableClass = Buildable->GetClass(),
                    .Transform = Buildable->GetTransform(),
                    //.CustomizationData = Buildable->GetCustomizationData_Native(),
            };
			AddExtraData(Data, Buildable);
			PendingRemoveBuildingData.Add(std::move(Data));

			RedrawMap();
		};


	const auto LambdaAfterCloseRespawnUI =
        [this](AFGHUD* Instance)
        {
            if (!ShouldInitialize)
            {
				return;
            }

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
				RCO->ReceivedSliceCount = 0;
				RCO->Buffer.Empty();
				RCO->ServerRequestInitialBuildingData(PlayerController, EInitialDataSendPhase::Initial);
			}
			else
			{
                UE_LOG(LogCartograph, Error, TEXT("Failed to get RemoteCallObject"));
			}
        };


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
}


void UCartographGameInstanceModule::OnWorldLoaded()
{
	CARTO_LOG_DEBUG("OnWorldLoaded");

	ShouldInitialize = true;
	IsClient = GetWorld()->IsNetMode(NM_Client);

	if (!FPlatformProperties::IsServerOnly())
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget, { 0, 0, 0, 0 });
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

    CARTO_LOG_DEBUG("InitialBuildableGather Started. Factories: %d, Buildings: %d", Factories.Num(), BuildingCount);

	const int Total = Factories.Num() + BuildingCount;
	CurrentBuildingData.Empty(Total);

	const float TimeBudget = FCartograph_ConfigStruct::GetActiveConfig(GetWorld()).InitializeTimeBudget;
	UE5Coro::Latent::FTickTimeBudget Budget = UE5Coro::Latent::FTickTimeBudget::Milliseconds(TimeBudget);

	int Processed = 0;

	for (const TWeakObjectPtr<AFGBuildable>& Factory : Factories)
	{
		if (!Factory.IsValid())
		{
			continue;
		}
		
		FBuildingData NewBuildingData{
			.BuildableClass = Factory->GetClass(),
			.Transform = Factory->GetTransform(),
            //.CustomizationData = Factory->GetCustomizationData_Native(),
		};
        AddExtraData(NewBuildingData, Factory.Get());

		const int32 Pos = Algo::LowerBound(CurrentBuildingData, NewBuildingData);
		CurrentBuildingData.Insert(std::move(NewBuildingData), Pos);

        InitializeProgress = static_cast<float>(++Processed) / Total;
		co_await Budget;
	}

	for (const auto& [Type, Arr] : Buildings)
	{
		for (const FRuntimeBuildableInstanceData& InstanceData : Arr)
		{
			FBuildingData NewBuildingData{
				.BuildableClass = Type,
				.Transform = InstanceData.Transform,
				//.CustomizationData = InstanceData.CustomizationData,
			};

			const int32 Pos = Algo::LowerBound(CurrentBuildingData, NewBuildingData);
			CurrentBuildingData.Insert(std::move(NewBuildingData), Pos);

			InitializeProgress = static_cast<float>(++Processed) / Total;
			co_await Budget;
		}
	}

	CARTO_LOG_DEBUG("InitialBuildableGather Finished");

	IsInitializing = false;
	IsPendingRedraw = false;
	ExecuteRedrawMapCoroutine();
}


void UCartographGameInstanceModule::RedrawMap()
{
	if (!Coroutine.IsDone())
	{
		if (!IsInitializing)
		{
			CARTO_LOG_DEBUG("RedrawMapCoroutine Cancel Requested");
			Coroutine.Cancel();
		}
		IsPendingRedraw = true;
	}
	else if (!IsClient || !IsInitializing)
	{
		ExecuteRedrawMapCoroutine();
	}
}


// AddedBuildings/RemovedBuildings: Intentional copies
UE5Coro::TCoroutine<> UCartographGameInstanceModule::RedrawMapCoroutine(
	TArray<FBuildingData> AddedBuildings, TArray<FBuildingData> RemovedBuildings, FForceLatentCoroutine)
{
	ON_SCOPE_EXIT
	{
        OnCoroutineFinishedOrCancelled();
	};

	CARTO_LOG_DEBUG("RedrawMapCoroutine Started");

	const float TimeBudget = FCartograph_ConfigStruct::GetActiveConfig(GetWorld()).RedrawTimeBudget;
	UE5Coro::Latent::FTickTimeBudget Budget = UE5Coro::Latent::FTickTimeBudget::Milliseconds(TimeBudget);

	{
        UE5Coro::FCancellationGuard Guard{};  // We'll lose added/removed building information if the coroutine is cancelled

		for (FBuildingData& AddedBuildingData : AddedBuildings)
		{
	        CARTO_LOG_DEBUG("AddedBuilding: %s", *AddedBuildingData.BuildableClass->GetName());

			const int32 Pos = Algo::LowerBound(CurrentBuildingData, AddedBuildingData);
			CurrentBuildingData.Insert(std::move(AddedBuildingData), Pos);

			co_await Budget;
		}

	    for (const FBuildingData& RemovedBuildingData : RemovedBuildings)
	    {
	        CARTO_LOG_DEBUG("RemovedBuilding: %s", *RemovedBuildingData.BuildableClass->GetName());

	        const int32 Start = Algo::LowerBound(CurrentBuildingData, RemovedBuildingData);
	        const int32 End = Algo::UpperBound(CurrentBuildingData, RemovedBuildingData);

	        for (int32 i = Start; i < End; ++i)
	        {
	            if (CurrentBuildingData[i] == RemovedBuildingData)
	            {
					CurrentBuildingData.RemoveAt(i);
	                break;
	            }
	        }

	        co_await Budget;
	    }

        CARTO_LOG_DEBUG("Buildings Change Processed");
	}

	if (FPlatformProperties::IsServerOnly())
	{
		co_return;
	}

	UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget, { 0, 0, 0, 0 });

	// Sometimes lines go crazy (goes to the top or far right) if we don't delay.
	// My guess is because EndDraw and BeginDraw are called in the same frame, so I'm putting it here.
	co_await UE5Coro::Latent::NextTick();

	UCanvas* Canvas = nullptr;
	FVector2D _;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, Canvas, _, RenderContext);

	FCartograph_ConfigStruct ConfigInstance = FCartograph_ConfigStruct::GetActiveConfig(GetWorld());
    for (auto& [_, SplineData] : BuildableSplineDataMap)
    {
		FProperty* Property = FCartograph_ConfigStruct::StaticStruct()->FindPropertyByName(SplineData.SegmentsConfigName);
        if (!Property)
        {
            UE_LOG(LogCartograph, Error, TEXT("SparsityConfigName not found: %s"), *SplineData.SegmentsConfigName.ToString());
            continue;
        }
		SplineData.SegmentsCached = *Property->ContainerPtrToValuePtr<int>(&ConfigInstance);
    }

	const AFGRecipeManager* RecipeManager = AFGRecipeManager::Get(GetWorld());

	for (const auto& [OriginalBuildableClass, Transform/*, CustomizationData*/, BuildableExtraData] : CurrentBuildingData)
	{
		if (!OriginalBuildableClass)
		{
			continue;
		}

		CARTO_LOG_VERY_VERBOSE("Buildable: %s, Transform: %s", *OriginalBuildableClass->GetName(), *Transform.ToString());

		const TSoftClassPtr<AFGBuildable>* RedirectClass = BuildableClassRedirectMap.Find(OriginalBuildableClass.Get());
        const TSubclassOf<AFGBuildable> BuildableClass = RedirectClass ? RedirectClass->LoadSynchronous() : OriginalBuildableClass.Get();

		if (BuildableClass->ImplementsInterface(UFGSplineBuildableInterface::StaticClass()))
		{
            const FSplineData* SplineData = BuildableSplineDataMap.Find(BuildableClass.Get());
			if (!SplineData)
			{
                UE_LOG(LogCartograph, Warning, TEXT("Can't find spline data for %s"), *BuildableClass->GetName());
				continue;
			}

			if (SplineData->Thickness < 0)
			{
				continue;
			}

			const FSplineExtraData* SplineExtraData = std::get_if<FSplineExtraData>(&BuildableExtraData);
			if (!SplineExtraData)
			{
				UE_LOG(LogCartograph, Error, TEXT("Can't find spline extra data for %s"), *BuildableClass->GetName());
				continue;
			}

			const int SplinePointCount = SplineExtraData->SplinePoints.Num();
			if (SplinePointCount < 2)
			{
				continue;
			}

			const int Segments = SplineData->SegmentsCached;
			const float Step = 1.f / Segments;

            const std::pair<TArray<FVector2D>, TArray<FVector2D>>* Tangents = SplineExtraData->Tangents.GetPtrOrNull();

			for (int i = 1; i < SplinePointCount; i++)
			{
				const FVector2D& PrevPoint = SplineExtraData->SplinePoints[i - 1];
				const FVector2D& NextPoint = SplineExtraData->SplinePoints[i];

				for (int j = 0; j < Segments; j++)
				{
					if (SplineData->UseTangents)
					{
						const FVector2D& LeaveTangent = Tangents->first[i - 1];
                        const FVector2D& ArriveTangent = Tangents->second[i];
						
						const FVector2D Start{ Transform.TransformPosition(FVector{ 
							FMath::CubicInterp(PrevPoint, LeaveTangent, NextPoint, ArriveTangent, j * Step), 0 }) };
						const FVector2D End{ Transform.TransformPosition(FVector{ 
							FMath::CubicInterp(PrevPoint, LeaveTangent, NextPoint, ArriveTangent, (j + 1) * Step), 0 }) };

						draw_line(Canvas, Start, End, SplineData->Color, SplineData->Thickness);
					}
					else
					{
						const FVector2D Start{ Transform.TransformPosition(FVector{ 
							FMath::Lerp(PrevPoint, NextPoint, j * Step), 0 }) };
						const FVector2D End{ Transform.TransformPosition(FVector{ 
							FMath::Lerp(PrevPoint, NextPoint, (j + 1) * Step), 0 }) };

						draw_line(Canvas, Start, End, SplineData->Color, SplineData->Thickness);
					}

					co_await Budget;
				}
			}

			continue;
		}

		if (BuildableClass->IsChildOf(AFGBuildableWire::StaticClass()))
        {
            const FWireData* WireData = BuildableWireDataMap.Find(BuildableClass.Get());
            if (!WireData)
            {
                UE_LOG(LogCartograph, Warning, TEXT("Can't find wire data for %s"), *BuildableClass->GetName());
                continue;
            }

			if (WireData->Thickness < 0)
			{
				continue;
			}

			const FWireExtraData* WireExtraData = std::get_if<FWireExtraData>(&BuildableExtraData);
			if (!WireExtraData)
			{
                UE_LOG(LogCartograph, Error, TEXT("Can't find wire extra data for %s"), *BuildableClass->GetName());
				continue;
			}

            draw_line(Canvas, Transform.GetLocation(), WireExtraData->End, WireData->Color, WireData->Thickness);

            co_await Budget;
			continue;
        }

		if (BuildableClass->IsChildOf(AFGBuildableBeam::StaticClass()))
		{
			const FWireData* BeamData = BuildableWireDataMap.Find(BuildableClass.Get());
			if (!BeamData)
			{
                UE_LOG(LogCartograph, Warning, TEXT("Can't find beam data for %s"), *BuildableClass->GetName());
				continue;
			}

			const FBeamExtraData* BeamExtraData = std::get_if<FBeamExtraData>(&BuildableExtraData);
			if (!BeamExtraData)
			{
				UE_LOG(LogCartograph, Error, TEXT("Can't find beam extra data for %s"), *BuildableClass->GetName());
				continue;
			}

            const float Length = BeamExtraData->Length;
			const FVector Start = Transform.GetLocation();
            const FVector End = Start + Transform.GetRotation().Vector() * Length;
			draw_line(Canvas, Start, End, BeamData->Color, BeamData->Thickness);

            co_await Budget;
			continue;
		}


		FVector2D* Size = BuildableSizeOverrideMap.Find(BuildableClass.Get());
		if (!Size)
		{
			const FBox ClearanceBox = Cast<AFGBuildable>(BuildableClass->ClassDefaultObject)->GetCombinedClearanceBox();
			if (!ClearanceBox.IsValid)
			{
                UE_LOG(LogCartograph, Warning, TEXT("Can't find size for %s"), *BuildableClass->GetName());
				continue;
			}

			FVector2D ClearanceBoxSize{ ClearanceBox.GetSize() };
			Size = &ClearanceBoxSize;
		}
        if (Size->X == 0.f || Size->Y == 0.f)
        {
            continue;
        }
		*Size *= FVector2D{ Transform.GetScale3D() };
		const FVector2D ScreenPosition = world_position_to_screen_position(Transform.GetLocation(), *Size);
		const FRotator* ExtraRotation = BuildableExtraRotationMap.Find(BuildableClass.Get());

		if (const TSoftObjectPtr<UTexture2D>* Texture = BuildableIconOverrideMap.Find(BuildableClass.Get());
			Texture && !Texture->IsNull())
		{
			const UTexture2D* LoadedTexture = Texture->Get();
			if (!LoadedTexture)
			{
				LoadedTexture = co_await UE5Coro::Latent::AsyncLoadObject(*Texture);
			}

			FCanvasTileItem TileItem{
				ScreenPosition,
				LoadedTexture->GetResource(),
				{ Size->X * PIXEL_PER_CENTIMETER[0], Size->Y * PIXEL_PER_CENTIMETER[1] },
				{ 0, 0 },
				{ 1, 1 },
				FLinearColor::White
			};
			TileItem.PivotPoint = { 0.5, 0.5 };
			TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Translucent);
			TileItem.Rotation = Transform.GetRotation().Rotator();
			if (ExtraRotation)
			{
				TileItem.Rotation += *ExtraRotation;
			}

			Canvas->DrawItem(TileItem);
		}
		else
		{
            const FCategoryData* CategoryData = BuildableBuildCategoryDataOverrideMap.Find(BuildableClass.Get());
			if (!CategoryData)
			{
				const TSubclassOf<UFGBuildingDescriptor> Descriptor = RecipeManager->FindBuildingDescriptorByClass(BuildableClass);
				if (TArray<TSubclassOf<UFGCategory>> Subcategories = UFGItemDescriptor::GetSubCategoriesOfClass(Descriptor, UFGBuildSubCategory::StaticClass());
					!Subcategories.IsEmpty())
				{
					CategoryData = BuildCategoryDataMap.Find(Subcategories[0].Get());
				}
				if (!CategoryData)
				{
					const TSubclassOf<UFGBuildCategory> Category = UFGBuildingDescriptor::GetBuildCategory(Descriptor);
					CategoryData = BuildCategoryDataMap.Find(Category.Get());
				}
			}
			if (!CategoryData)
			{
				UE_LOG(LogCartograph, Warning, TEXT("Can't find category data for %s"), *BuildableClass->GetName());
				continue;
			}

			FCanvasTileItem TileItem{
				ScreenPosition,
				{ Size->X * PIXEL_PER_CENTIMETER[0], Size->Y * PIXEL_PER_CENTIMETER[1] },
				CategoryData->MainColor
			};
			TileItem.PivotPoint = { 0.5, 0.5 };
			TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Translucent);
			TileItem.Rotation = Transform.GetRotation().Rotator();
			if (ExtraRotation)
			{
				TileItem.Rotation += *ExtraRotation;
			}

			Canvas->DrawItem(TileItem);
			co_await Budget;

			const float HalfWidth = Size->X / 2;
			const float HalfHeight = Size->Y / 2;
			FVector LocalCorners[] = {
				{ -HalfWidth, -HalfHeight, 0 },
				{ HalfWidth, -HalfHeight, 0 },
				{ HalfWidth,  HalfHeight, 0 },
				{ -HalfWidth,  HalfHeight, 0 },
			};

			FTransform TransformNoScale = Transform;
            TransformNoScale.SetScale3D(FVector::OneVector);
			for (FVector& Corner : LocalCorners)
			{
				Corner = TransformNoScale.TransformPosition(Corner);
			}

			if (CategoryData->OutlineThickness > 0)
			{
				draw_line(Canvas, LocalCorners[0], LocalCorners[1], CategoryData->OutlineColor, CategoryData->OutlineThickness);
				draw_line(Canvas, LocalCorners[1], LocalCorners[2], CategoryData->OutlineColor, CategoryData->OutlineThickness);
				draw_line(Canvas, LocalCorners[2], LocalCorners[3], CategoryData->OutlineColor, CategoryData->OutlineThickness);
				draw_line(Canvas, LocalCorners[3], LocalCorners[0], CategoryData->OutlineColor, CategoryData->OutlineThickness);
			}
		}

		co_await Budget;
	}

	CARTO_LOG_DEBUG("RedrawMapCoroutine Finished");
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
	ExecuteRedrawMapCoroutine();
}


void UCartographGameInstanceModule::ExecuteRedrawMapCoroutine()
{
	Coroutine = RedrawMapCoroutine(PendingAddBuildingData, PendingRemoveBuildingData);
	if (!IsClient)
	{
		ACartographModSubsystem::Instance->ClientUpdateBuildingData(PendingAddBuildingData, PendingRemoveBuildingData);
	}
	PendingAddBuildingData.Empty();
	PendingRemoveBuildingData.Empty();
}


void UCartographGameInstanceModule::AddExtraData(FBuildingData& BuildingData, AFGBuildable* Buildable)
{
	const TSoftClassPtr<AFGBuildable> Class = Buildable->GetClass();
	const TSoftClassPtr<AFGBuildable>* RedirectClass = BuildableClassRedirectMap.Find(Class);
	const TSubclassOf<AFGBuildable> BuildableClass = RedirectClass ? RedirectClass->Get() : Class.Get();

	if (BuildableClass->ImplementsInterface(UFGSplineBuildableInterface::StaticClass()))
	{
        const auto* Spline = Cast<IFGSplineBuildableInterface>(Buildable);
		const USplineComponent* SplineComponent = Spline->GetSplineComponent();
		BuildingData.Transform = SplineComponent->GetComponentTransform();

		TArray<FVector2D> SplinePoints;
        Algo::Transform(SplineComponent->SplineCurves.Position.Points, SplinePoints, 
			[](const FInterpCurvePoint<FVector>& Point) { return FVector2D{ Point.OutVal }; });
        FSplineExtraData ExtraData{
            .SplinePoints = std::move(SplinePoints),
        };

        if (const FSplineData* SplineData = BuildableSplineDataMap.Find(BuildableClass.Get());
			!SplineData)
		{
			UE_LOG(LogCartograph, Warning, TEXT("Can't find spline data for %s"), *BuildableClass->GetName());
		}
		else if (SplineData->UseTangents)
		{
			TArray<FVector2D> LeaveTangents;
			Algo::Transform(SplineComponent->SplineCurves.Position.Points, LeaveTangents,
				[](const FInterpCurvePoint<FVector>& Point) { return FVector2D{ Point.LeaveTangent }; });

			TArray<FVector2D> ArriveTangents;
			Algo::Transform(SplineComponent->SplineCurves.Position.Points, ArriveTangents,
				[](const FInterpCurvePoint<FVector>& Point) { return FVector2D{ Point.ArriveTangent }; });

			ExtraData.Tangents = std::make_pair(std::move(LeaveTangents), std::move(ArriveTangents));
		}

        BuildingData.BuildableExtraData = std::move(ExtraData);
		return;
	}

	if (BuildableClass->IsChildOf(AFGBuildableWire::StaticClass()))
	{
		const auto* Wire = Cast<AFGBuildableWire>(Buildable);
		BuildingData.Transform.SetLocation(Wire->GetConnectionLocation(0));

        BuildingData.BuildableExtraData = FWireExtraData{
			.End = FVector2D{ Wire->GetConnectionLocation(1) },
        };

		return;
	}

	if (BuildableClass->IsChildOf(AFGBuildableBeam::StaticClass()))
	{
        const auto* Beam = Cast<AFGBuildableBeam>(Buildable);

        BuildingData.BuildableExtraData = FBeamExtraData{
            .Length = Beam->GetLength(),
        };

		return;
	}
}


#if WITH_EDITOR
void UCartographGameInstanceModule::PostCDOContruct()
{
	// Need to redo it every time the game is updated.
	return;

    ClassIDToClassPtrMap.Empty();
    ClassPtrToClassIDMap.Empty();

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
		if (!AssetPath.GetPackageName().ToString().StartsWith("/Game/FactoryGame")
			|| Name.StartsWith("SKEL_") || Name.StartsWith("REINST_"))
		{
			continue;
		}

		const uint32 Hash = TextKeyUtil::HashString(AssetPath.ToString());
		ClassPtrToClassIDMap.Add(Class, Hash);
		ClassIDToClassPtrMap.Add(Hash, Class);
        CARTO_LOG_DEBUG("Path: %s, Class: %s, Hash: %u", *AssetPath.ToString(), *Name, Hash);
    }
}
#endif
