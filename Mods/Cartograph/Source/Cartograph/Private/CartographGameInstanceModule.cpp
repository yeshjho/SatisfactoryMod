#include "CartographGameInstanceModule.h"

#include "AssetRegistryModule.h"
#include "CanvasItem.h"
#include "CanvasPanelSlot.h"
#include "HorizontalBox.h"
#include "HorizontalBoxSlot.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
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
#include "FGRecipeManager.h"
#include "FGSaveSession.h"
#include "FGSplineBuildableInterface.h"

#include "Patching/NativeHookManager.h"

#include "CartographModSubsystem.h"
#include "CartographRemoteCallObject.h"
#include "Cartograph_ConfigStruct.h"
#include "QuantizedVector2DSerialization.h"

#define LOCTEXT_NAMESPACE "Cartograph"


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


// This is used for actual equality check while removing
bool FBuildingData::operator==(const FBuildingData& Other) const noexcept
{
	return BuildableClassHash == Other.BuildableClassHash && Transform.Equals(Other.Transform) && BuildableExtraData == Other.BuildableExtraData;
	// Ignoring CustomizationData on purpose
}


// This is used for sorting, so we only compare Z values
std::partial_ordering FBuildingData::operator<=>(const FBuildingData& Other) const noexcept
{
	return Transform.GetLocation().Z <=> Other.Transform.GetLocation().Z;
}


std::partial_ordering FBuildingData::operator<=>(float Z) const noexcept
{
    return Transform.GetLocation().Z <=> Z;
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
	Ar.SerializeBits(&BuildableClassHash, 32);

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
			break;
        }
    }

	if (Ar.IsLoading())
	{
		if (const TSubclassOf<AFGBuildable>* Class = UCartographGameInstanceModule::Instance->ClassIDToClassPtrMap.Find(BuildableClassHash))
		{
			FillInCache(*Class);
		}
		else
		{
			UE_LOG(LogCartograph, Warning, TEXT("Class ID %u not found in ClassIDToClassPtrMap"), BuildableClassHash);
		}
	}

    bOutSuccess = true;
    return true;
}


void FBuildingData::FillInCache(TSubclassOf<AFGBuildable> OriginalBuildableClass)
{
    DataType = EBuildingDataType::Invalid;

	const auto& BuildableClassRedirectMap = UCartographGameInstanceModule::Instance->BuildableClassRedirectMap;
	const TSoftClassPtr<AFGBuildable>* RedirectClass = BuildableClassRedirectMap.Find(OriginalBuildableClass.Get());
	const TSubclassOf<AFGBuildable> BuildableClass = RedirectClass ? RedirectClass->LoadSynchronous() : OriginalBuildableClass.Get();

	if (BuildableClass->ImplementsInterface(UFGSplineBuildableInterface::StaticClass()))
	{
		const auto& BuildableSplineDataMap = UCartographGameInstanceModule::Instance->BuildableSplineDataMap;

		const FSplineData* SplineData = BuildableSplineDataMap.Find(BuildableClass.Get());
		if (!SplineData)
		{
			UE_LOG(LogCartograph, Warning, TEXT("Can't find spline data for %s"), *BuildableClass->GetName());
			return;
		}

		if (SplineData->Thickness <= 0)  // We don't need to draw, so don't even bother initializing the data cache.
		{
			return;
		}

		const FSplineExtraData* SplineExtraData = std::get_if<FSplineExtraData>(&BuildableExtraData);
		if (!SplineExtraData)
		{
			UE_LOG(LogCartograph, Error, TEXT("Can't find spline extra data"));
			return;
		}

		if (const int SplinePointCount = SplineExtraData->SplinePoints.Num();
			SplinePointCount < 2)  // This should never happen, but just in case.
		{
			return;
		}

        DataType = EBuildingDataType::Spline;
		DataCache = FSplineDataCache{
			.SplineData = SplineData,
		};
        CalculateSplinePoints();
		return;
	}

	if (BuildableClass->IsChildOf(AFGBuildableWire::StaticClass()))
	{
		const auto& BuildableWireDataMap = UCartographGameInstanceModule::Instance->BuildableWireDataMap;

		const FWireData* WireData = BuildableWireDataMap.Find(BuildableClass.Get());
		if (!WireData)
		{
			UE_LOG(LogCartograph, Warning, TEXT("Can't find wire data for %s"), *BuildableClass->GetName());
			return;
		}

		if (WireData->Thickness <= 0)  // We don't need to draw, so don't even bother initializing the data cache.
		{
			return;
		}

		DataType = EBuildingDataType::Wire;
		DataCache = WireData;
		return;
	}

	if (BuildableClass->IsChildOf(AFGBuildableBeam::StaticClass()))
	{
		const auto& BuildableWireDataMap = UCartographGameInstanceModule::Instance->BuildableWireDataMap;

		const FWireData* BeamData = BuildableWireDataMap.Find(BuildableClass.Get());
		if (!BeamData)
		{
			UE_LOG(LogCartograph, Warning, TEXT("Can't find beam data for %s"), *BuildableClass->GetName());
			return;
		}

		if (BeamData->Thickness <= 0)  // We don't need to draw, so don't even bother initializing the data cache.
		{
			return;
		}

		DataType = EBuildingDataType::Beam;
		DataCache = BeamData;
		return;
	}


	auto& BuildableSizeOverrideMap = UCartographGameInstanceModule::Instance->BuildableSizeOverrideMap;
	FVector2D* Size = BuildableSizeOverrideMap.Find(BuildableClass.Get());
	if (!Size)
	{
		const FBox ClearanceBox = Cast<AFGBuildable>(BuildableClass->ClassDefaultObject)->GetCombinedClearanceBox();
		if (!ClearanceBox.IsValid)
		{
			UE_LOG(LogCartograph, Warning, TEXT("Can't find size for %s"), *BuildableClass->GetName());
			return;
		}

		FVector2D ClearanceBoxSize{ ClearanceBox.GetSize() };
		Size = &ClearanceBoxSize;
	}
	if (Size->X == 0.f || Size->Y == 0.f)  // We don't need to draw, so don't even bother initializing the data cache.
	{
		return;
	}
	*Size *= FVector2D{ Transform.GetScale3D() };

	FRotator Rotation = Transform.GetRotation().Rotator();
	auto& BuildableExtraRotationMap = UCartographGameInstanceModule::Instance->BuildableExtraRotationMap;
    if (const FRotator* ExtraRotation = BuildableExtraRotationMap.Find(BuildableClass.Get()))
    {
        Rotation += *ExtraRotation;
    }

	const FVector2D ScreenPosition = world_position_to_screen_position(Transform.GetLocation(), *Size);

	auto& BuildableIconOverrideMap = UCartographGameInstanceModule::Instance->BuildableIconOverrideMap;
	if (const TSoftObjectPtr<UTexture2D>* Texture = BuildableIconOverrideMap.Find(BuildableClass.Get());
		Texture && !Texture->IsNull())
	{
        DataType = EBuildingDataType::Icon;
        DataCache = FNormalDataCache{
            .ScreenPosition = ScreenPosition,
            .Size = *Size,
            .Rotation = Rotation,
            .IconOrRectangleData = *Texture,
        };

		return;
	}


	auto& BuildableBuildCategoryDataOverrideMap = UCartographGameInstanceModule::Instance->BuildableBuildCategoryDataOverrideMap;
	const FCategoryData* CategoryData = BuildableBuildCategoryDataOverrideMap.Find(BuildableClass.Get());
	if (!CategoryData)
	{
		auto& BuildCategoryDataMap = UCartographGameInstanceModule::Instance->BuildCategoryDataMap;
		const AFGRecipeManager* RecipeManager = AFGRecipeManager::Get(UCartographGameInstanceModule::Instance->GetWorld());
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
		return;
	}

	FRectangleDataCache RectangleData{
		.CategoryData = CategoryData,
	};

	const float HalfWidth = Size->X / 2;
	const float HalfHeight = Size->Y / 2;
	RectangleData.LocalCorners[0] = { -HalfWidth, -HalfHeight, 0 };
    RectangleData.LocalCorners[1] = { HalfWidth, -HalfHeight, 0 };
    RectangleData.LocalCorners[2] = { HalfWidth, HalfHeight, 0 };
    RectangleData.LocalCorners[3] = { -HalfWidth, HalfHeight, 0 };

	FTransform TransformNoScale = Transform;
	TransformNoScale.SetScale3D(FVector::OneVector);
	for (FVector& Corner : RectangleData.LocalCorners)
	{
		Corner = TransformNoScale.TransformPosition(Corner);
	}

    DataType = EBuildingDataType::Rectangle;
	DataCache = FNormalDataCache{
		.ScreenPosition = ScreenPosition,
		.Size = *Size,
		.Rotation = Rotation,
		.IconOrRectangleData = std::move(RectangleData),
	};
}


void FBuildingData::FillInHash(TSubclassOf<AFGBuildable> BuildableClass)
{
	if (const uint32* Hash = UCartographGameInstanceModule::Instance->ClassPtrToClassIDMap.Find(BuildableClass))
	{
		BuildableClassHash = *Hash;
	}
	else
	{
		UE_LOG(LogCartograph, Warning, TEXT("Class %s not found in ClassPtrToClassIDMap"), *BuildableClass->GetName());
	}
}


void FBuildingData::FillInHashAndCache(TSubclassOf<AFGBuildable> BuildableClass)
{
    FillInHash(BuildableClass);
	if (BuildableClassHash != 0)
	{
		FillInCache(BuildableClass);
	}
	else
	{
		DataType = EBuildingDataType::Invalid;
	}
}


void FBuildingData::CalculateSplinePoints()
{
    if (DataType != EBuildingDataType::Spline)
    {
        return;
    }

    auto& [SplineData, StartPoints, EndPoints] = std::get<FSplineDataCache>(DataCache);
	const FSplineExtraData* SplineExtraData = std::get_if<FSplineExtraData>(&BuildableExtraData);
	if (!SplineExtraData)
	{
		UE_LOG(LogCartograph, Error, TEXT("Can't find spline extra data"));
		return;
	}

	const int SplinePointCount = SplineExtraData->SplinePoints.Num();
	if (SplinePointCount < 2)  // This should never happen, but just in case.
	{
		return;
	}

	const int Segments = SplineData->SegmentsCached;
	const float Step = 1.f / Segments;

	const std::pair<TArray<FVector2D>, TArray<FVector2D>>* Tangents = SplineExtraData->Tangents.GetPtrOrNull();

    StartPoints.Empty(SplinePointCount * Segments);
    EndPoints.Empty(SplinePointCount * Segments);
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

				StartPoints.Add(FVector2D{ Transform.TransformPosition(FVector{
					FMath::CubicInterp(PrevPoint, LeaveTangent, NextPoint, ArriveTangent, j * Step), 0 }) });
				EndPoints.Add(FVector2D{ Transform.TransformPosition(FVector{
					FMath::CubicInterp(PrevPoint, LeaveTangent, NextPoint, ArriveTangent, (j + 1) * Step), 0 }) });
			}
			else
			{
				StartPoints.Add(FVector2D{ Transform.TransformPosition(FVector{
					FMath::Lerp(PrevPoint, NextPoint, j * Step), 0 }) });
				EndPoints.Add(FVector2D{ Transform.TransformPosition(FVector{
					FMath::Lerp(PrevPoint, NextPoint, (j + 1) * Step), 0 }) });
			}
		}
	}
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
					.Transform = BuildableInstanceData.Transform,
					//.CustomizationData = BuildableInstanceData.CustomizationData,
			};
            Data.FillInHashAndCache(BuildableClass);
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
					.Transform = ReplicationData.Transform,
					//.CustomizationData = ReplicationData.CustomizationData,
			};
			Data.FillInHashAndCache(BuildableClass);
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
					.Transform = Buildable->GetTransform(),
					//.CustomizationData = Buildable->GetCustomizationData_Native(),
			};
			AddExtraData(Data, Buildable);
			Data.FillInHashAndCache(Buildable->GetClass());
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
					.Transform = LightweightData->Transform,
					//.CustomizationData = Data->CustomizationData,
			};
			//Data.FillInHashAndCache(BuildableClass);  // Cache are not used in comparison (==, <=>) so we don't need to fill it
			Data.FillInHash(BuildableClass);
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
                    .Transform = Buildable->GetTransform(),
                    //.CustomizationData = Buildable->GetCustomizationData_Native(),
            };
			AddExtraData(Data, Buildable);
            //Data.FillInHashAndCache(Buildable->GetClass());  // Cache are not used in comparison (==, <=>) so we don't need to fill it
			Data.FillInHash(Buildable->GetClass());
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
	}
}


void UCartographGameInstanceModule::OnWorldLoaded()
{
	CARTO_LOG_DEBUG("OnWorldLoaded");

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
	CARTO_LOG_DEBUG("OnWorldUnloaded");

	IsInWorld = false;
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
			.Transform = Factory->GetTransform(),
            //.CustomizationData = Factory->GetCustomizationData_Native(),
		};
        AddExtraData(NewBuildingData, Factory.Get());
		NewBuildingData.FillInHashAndCache(Factory->GetClass());

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
				.Transform = InstanceData.Transform,
				//.CustomizationData = InstanceData.CustomizationData,
			};
			NewBuildingData.FillInHashAndCache(Type);

			const int32 Pos = Algo::LowerBound(CurrentBuildingData, NewBuildingData);
			CurrentBuildingData.Insert(std::move(NewBuildingData), Pos);

			InitializeProgress = static_cast<float>(++Processed) / Total;
			co_await Budget;
		}
	}

	MinHeight = !CurrentBuildingData.IsEmpty() ? CurrentBuildingData[0].Transform.GetLocation().Z : -100;
	MaxHeight = !CurrentBuildingData.IsEmpty() ? CurrentBuildingData.Last().Transform.GetLocation().Z : 100;
	OnZFilterUpdated(0, 1);

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
	        CARTO_LOG_DEBUG("AddedBuilding: %u", AddedBuildingData.BuildableClassHash);

			const int32 Pos = Algo::LowerBound(CurrentBuildingData, AddedBuildingData);
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
					CurrentBuildingData.RemoveAt(i);
	                break;
	            }
                if (RemovedBuildingData > CurrentBuildingData[i])
                {
                    UE_LOG(LogCartograph, Warning, TEXT("Can't find removed building data"));
                    break;
                }
	        }

	        co_await Budget;
	    }

        MinHeight = !CurrentBuildingData.IsEmpty() ? CurrentBuildingData[0].Transform.GetLocation().Z : -100;
        MaxHeight = !CurrentBuildingData.IsEmpty() ? CurrentBuildingData.Last().Transform.GetLocation().Z : 100;

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

    const int32 Min = Algo::LowerBound(CurrentBuildingData, MinZFilter);
    const int32 Max = Algo::UpperBound(CurrentBuildingData, MaxZFilter);
    if (Min >= CurrentBuildingData.Num() || Max <= 0)
    {
        co_return;
    }

	for (int32 i = Min; i < Max; i++)
	{
        const auto& [ClassHash, Transform/*, CustomizationData*/, BuildableExtraData, DataType, DataCache] = CurrentBuildingData[i];

		CARTO_LOG_VERY_VERBOSE("Buildable: %u, Transform: %s", ClassHash, *Transform.ToString());

		switch (DataType)
		{
		case EBuildingDataType::Invalid:
			break;

		case EBuildingDataType::Icon:
		{
			const auto& [ScreenPosition, Size, Rotation, IconOrRectangleData] = std::get<FNormalDataCache>(DataCache);
            const TSoftObjectPtr<UTexture2D>& Texture = std::get<TSoftObjectPtr<UTexture2D>>(IconOrRectangleData);

			// The texture might have gotten unloaded between redraws, so we can't cache it.
			const UTexture2D* LoadedTexture = Texture.Get();
			if (!LoadedTexture)
			{
				LoadedTexture = co_await UE5Coro::Latent::AsyncLoadObject(Texture);
			}

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
			const auto& [ScreenPosition, Size, Rotation, IconOrRectangleData] = std::get<FNormalDataCache>(DataCache);
			const auto& [CategoryData, LocalCorners] = std::get<FRectangleDataCache>(IconOrRectangleData);

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
				draw_line(Canvas, LocalCorners[0], LocalCorners[1], CategoryData->OutlineColor, CategoryData->OutlineThickness);
				co_await Budget;
				draw_line(Canvas, LocalCorners[1], LocalCorners[2], CategoryData->OutlineColor, CategoryData->OutlineThickness);
				co_await Budget;
				draw_line(Canvas, LocalCorners[2], LocalCorners[3], CategoryData->OutlineColor, CategoryData->OutlineThickness);
				co_await Budget;
				draw_line(Canvas, LocalCorners[3], LocalCorners[0], CategoryData->OutlineColor, CategoryData->OutlineThickness);
			}

			break;
		}

		case EBuildingDataType::Spline:
		{
			const auto& [SplineData, StartPoints, EndPoints] = std::get<FSplineDataCache>(DataCache);
            const int Num = StartPoints.Num();
            for (int i = 0; i < Num; i++)
            {
                draw_line(Canvas, StartPoints[i], EndPoints[i], SplineData->Color, SplineData->Thickness);
                co_await Budget;
            }

			break;
		}

		case EBuildingDataType::Wire:
		{
			const FWireExtraData* WireExtraData = std::get_if<FWireExtraData>(&BuildableExtraData);
			if (!WireExtraData)
			{
				UE_LOG(LogCartograph, Error, TEXT("Can't find wire extra data"));
				continue;
			}

			const FWireData* WireData = std::get<const FWireData*>(DataCache);

			draw_line(Canvas, Transform.GetLocation(), WireExtraData->End, WireData->Color, WireData->Thickness);

			break;
		}

        case EBuildingDataType::Beam:
		{
			const FBeamExtraData* BeamExtraData = std::get_if<FBeamExtraData>(&BuildableExtraData);
			if (!BeamExtraData)
			{
				UE_LOG(LogCartograph, Error, TEXT("Can't find beam extra data"));
				continue;
			}

			const FWireData* BeamData = std::get<const FWireData*>(DataCache);

			const float Length = BeamExtraData->Length;
			const FVector Start = Transform.GetLocation();
			const FVector End = Start + Transform.GetRotation().Vector() * Length;
			draw_line(Canvas, Start, End, BeamData->Color, BeamData->Thickness);

            break;
		}

		default:
			break;
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


void UCartographGameInstanceModule::RegisterMenuButton()
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
	if (!ShowHideButton)
	{
        UE_LOG(LogCartograph, Error, TEXT("ShowHideButton not found"));
		return;
	}
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
    if (!TextProperty)
    {
        UE_LOG(LogCartograph, Error, TEXT("mText not found"));
		return;
    }
	FText* TextPtr = TextProperty->ContainerPtrToValuePtr<FText>(CartographMenuShowHideButton);
    if (!TextPtr)
    {
        UE_LOG(LogCartograph, Error, TEXT("TextPtr not found"));
		return;
    }
    *TextPtr = LOCTEXT("CartographMenuShow", "Show Cartograph Menu");

    TArray<UPanelSlot*>& MutablePanelSlots = UCartographPanelWidgetAccessor::GetPanelSlots(Parent);
	MutablePanelSlots.Insert(HBoxPanelSlot, Index);


	const UWidget* Menu = WidgetTree->FindWidget("BPW_MapMenu");
	if (!Menu)
	{
		UE_LOG(LogCartograph, Error, TEXT("Menu not found"));
		return;
	}
	const auto* MenuPanelSlot = Cast<UCanvasPanelSlot>(Menu->Slot);
    if (!MenuPanelSlot)
    {
        UE_LOG(LogCartograph, Error, TEXT("MenuPanelSlot not found"));
        return;
    }

	UWidget* CartographMenu = NewObject<UWidget>(HBox, MenuWidget, "CartographMenu", RF_Transient);
	auto* CartographMenuPanelSlot = Cast<UCanvasPanelSlot>(Parent->AddChild(CartographMenu));
    CartographMenuPanelSlot->SetLayout(MenuPanelSlot->GetLayout());
    CartographMenuPanelSlot->SetPosition(MenuPanelSlot->GetPosition());
    CartographMenuPanelSlot->SetSize(MenuPanelSlot->GetSize());
    CartographMenuPanelSlot->SetAutoSize(MenuPanelSlot->GetAutoSize());
    CartographMenuPanelSlot->SetZOrder(MenuPanelSlot->GetZOrder());

	CartographMenu->SetVisibility(ESlateVisibility::Collapsed);
}


void UCartographGameInstanceModule::AfterSplineSegmentsModified()
{
	FCartograph_ConfigStruct ConfigInstance = FCartograph_ConfigStruct::GetActiveConfig(GetWorld());
	for (auto& [_, SplineData] : BuildableSplineDataMap)
	{
		const FProperty* Property = FCartograph_ConfigStruct::StaticStruct()->FindPropertyByName(SplineData.SegmentsConfigName);
		if (!Property)
		{
			UE_LOG(LogCartograph, Error, TEXT("SparsityConfigName not found: %s"), *SplineData.SegmentsConfigName.ToString());
			continue;
		}
		SplineData.SegmentsCached = *Property->ContainerPtrToValuePtr<int>(&ConfigInstance);
	}

	if (!IsInWorld)
	{
		return;
	}

    for (FBuildingData& BuildingData : CurrentBuildingData)
    {
        BuildingData.CalculateSplinePoints();
    }

	RedrawMap();
}


void UCartographGameInstanceModule::OnZFilterUpdated(float Min, float Max)
{
	const float Length = MaxHeight - MinHeight;
	MinZFilter = FMath::Floor(Min * Length + MinHeight);
    MaxZFilter = FMath::CeilToInt(Max * Length + MinHeight);

	if (!IsInitializing)
	{
        RedrawMap();
	}
}


void UCartographGameInstanceModule::OnCartographMenuButtonClicked(UUserWidget* Widget, bool IsOpen)
{
	for (UWidget* ChildWidget : Widget->GetParent()->GetParent()->GetAllChildren())
	{
        if (ChildWidget->GetName() == "CartographMenu")
        {
            CARTO_LOG_DEBUG("CartographMenuButtonClicked: %d", IsOpen);
            ChildWidget->SetVisibility(IsOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
            break;
        }
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


#undef LOCTEXT_NAMESPACE
