#include "CartographGameInstanceModule.h"

#include "AssetRegistryModule.h"
#include "CanvasItem.h"
#include "CanvasPanelSlot.h"
#include "CanvasRender.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
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


void clear_render_target_portion(UCanvas* Canvas, const FBox2D& Region)
{
	const FVector2D Size = Region.GetSize();
	const FVector2D ScreenPosition = world_position_to_screen_position(Region.GetCenter(), Size);
	FCanvasTileItem TileItem{
		ScreenPosition,
		{ Size.X * PIXEL_PER_CENTIMETER[0], Size.Y * PIXEL_PER_CENTIMETER[1] },
		{ 0, 0, 0, 0 }
	};
	TileItem.PivotPoint = { 0.5, 0.5 };
	TileItem.BlendMode = SE_BLEND_Opaque;

	Canvas->DrawItem(TileItem);
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


#pragma region Serialization
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


FArchive& operator<<(FArchive& Ar, FBuildingData& BuildingData)
{
	bool _;
	BuildingData.NetSerialize(Ar, nullptr, _);
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
			CARTO_LOG_WARNING("Class ID %u not found in ClassIDToClassPtrMap", BuildableClassHash);
		}
	}

    bOutSuccess = true;
    return true;
}
#pragma endregion


#pragma region Cache
void FBuildingData::FillInCache(TSubclassOf<AFGBuildable> OriginalBuildableClass)
{
    DataType = EBuildingDataType::Invalid;

	const auto& BuildableClassRedirectMap = UCartographGameInstanceModule::Instance->BuildableClassRedirectMap;
	const TSoftClassPtr<AFGBuildable>* RedirectClass = BuildableClassRedirectMap.Find(OriginalBuildableClass.Get());
	const TSubclassOf<AFGBuildable> BuildableClass = RedirectClass ? RedirectClass->LoadSynchronous() : OriginalBuildableClass.Get();

	const uint32* ClassID = UCartographGameInstanceModule::Instance->ClassPtrToClassIDMap.Find(BuildableClass);
	if (!ClassID)
	{
		CARTO_LOG_ERROR("Can't find hash for %s", *BuildableClass->GetName());
		return;
	}

	const FBuildLayerData* LayerData = UCartographGameInstanceModule::Instance->GetBuildLayerData(*ClassID);
	if (!LayerData)
	{
		return;
	}
	LayerDataCache = LayerData;


	if (BuildableClass->ImplementsInterface(UFGSplineBuildableInterface::StaticClass()))
	{
		const auto& BuildableSplineDataMap = UCartographGameInstanceModule::Instance->BuildableSplineDataMap;

		const FSplineData* SplineData = BuildableSplineDataMap.Find(BuildableClass.Get());
		if (!SplineData)
		{
			CARTO_LOG_WARNING("Can't find spline data for %s", *BuildableClass->GetName());
			return;
		}

		if (SplineData->Thickness <= 0)  // We don't need to draw, so don't even bother initializing the data cache.
		{
			return;
		}

		const FSplineExtraData* SplineExtraData = std::get_if<FSplineExtraData>(&BuildableExtraData);
		if (!SplineExtraData)
		{
			CARTO_LOG_ERROR("Can't find spline extra data");
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
			CARTO_LOG_WARNING("Can't find wire data for %s", *BuildableClass->GetName());
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
			CARTO_LOG_WARNING("Can't find beam data for %s", *BuildableClass->GetName());
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
			CARTO_LOG_WARNING("Can't find size for %s", *BuildableClass->GetName());
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

	const FCategoryData* CategoryData = UCartographGameInstanceModule::Instance->GetDataByBuildableClass(
		UCartographGameInstanceModule::Instance->BuildableBuildCategoryDataOverrideMap,
		UCartographGameInstanceModule::Instance->BuildCategoryDataMap,
		BuildableClass.Get());
	if (!CategoryData)
	{
		CARTO_LOG_WARNING("Can't find category data for %s", *BuildableClass->GetName());
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


void FBuildingData::FillInHash(TSubclassOf<AFGBuildable> OriginalBuildableClass)
{
	const auto& BuildableClassRedirectMap = UCartographGameInstanceModule::Instance->BuildableClassRedirectMap;
	const TSoftClassPtr<AFGBuildable>* RedirectClass = BuildableClassRedirectMap.Find(OriginalBuildableClass.Get());
	const TSubclassOf<AFGBuildable> BuildableClass = RedirectClass ? RedirectClass->LoadSynchronous() : OriginalBuildableClass.Get();

	if (const uint32* Hash = UCartographGameInstanceModule::Instance->ClassPtrToClassIDMap.Find(BuildableClass))
	{
		BuildableClassHash = *Hash;
	}
	else
	{
		CARTO_LOG_WARNING("Class %s not found in ClassPtrToClassIDMap", *BuildableClass->GetName());
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

    FSplineDataCache* SplineDataCachePtr = std::get_if<FSplineDataCache>(&DataCache);
	CARTO_LOG_ERROR_RETURN_IF_NULL(SplineDataCachePtr);

    auto& [SplineData, StartPoints, EndPoints] = *SplineDataCachePtr;
	const FSplineExtraData* SplineExtraData = std::get_if<FSplineExtraData>(&BuildableExtraData);
	CARTO_LOG_ERROR_RETURN_IF_NULL(SplineExtraData);

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
			CARTO_LOG_WARNING("Can't find spline data for %s", *BuildableClass->GetName());
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


void UCartographGameInstanceModule::AfterSplineSegmentsModified()
{
	FCartograph_ConfigStruct ConfigInstance = FCartograph_ConfigStruct::GetActiveConfig(GetWorld());
	for (auto& [_, SplineData] : BuildableSplineDataMap)
	{
		const FProperty* Property = FCartograph_ConfigStruct::StaticStruct()->FindPropertyByName(SplineData.SegmentsConfigName);
		if (!Property)
		{
			CARTO_LOG_ERROR("SparsityConfigName not found: %s", *SplineData.SegmentsConfigName.ToString());
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

    CARTO_LOG("Spline segments modified");
	RedrawMap();
}
#pragma endregion


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

			FBuildingData Data{
					.Transform = BuildableInstanceData.Transform,
					//.CustomizationData = BuildableInstanceData.CustomizationData,
			};
            Data.FillInHashAndCache(BuildableClass);
			PendingAddBuildingData.Add(std::move(Data));

			RedrawMap();
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

			FBuildingData Data{
					.Transform = ReplicationData.Transform,
					//.CustomizationData = ReplicationData.CustomizationData,
			};
			Data.FillInHashAndCache(BuildableClass);
			PendingAddBuildingData.Add(std::move(Data));

			RedrawMap();
		};


	const auto LambdaAfterAddBuildable =
		[this](AFGBuildableSubsystem* ClassInstance, AFGBuildable* Buildable)
		{
			CARTO_LOG_VERBOSE("AddBuildable: %s, Skip: %d", *Buildable->GetClass()->GetName(), ShouldInitialize || IsClient);

			if (ShouldInitialize || IsClient || !GIsRunning)
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
		[this](AFGLightweightBuildableSubsystem* ClassInstance, TSubclassOf<AFGBuildable> BuildableClass, int32 Index)
		{
			CARTO_LOG_VERBOSE("InvalidateRuntimeInstanceDataForIndex: %s, Skip: %d", *BuildableClass->GetName(), ShouldInitialize || IsClient);

			if (ShouldInitialize || IsClient || !GIsRunning)
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

			RedrawMap();
		};


	const auto LambdaAfterRemoveBuildable =
		[this](AFGBuildableSubsystem* ClassInstance, AFGBuildable* Buildable)
		{
			CARTO_LOG_VERBOSE("RemoveBuildable: %s, Skip: %d", *Buildable->GetClass()->GetName(), ShouldInitialize || IsClient);

			if (ShouldInitialize || IsClient || !GIsRunning)
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
				SCOPE_CYCLE_COUNTER(STAT_Canvas_GetBatchElementsTime);

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
					INC_DWORD_STAT(STAT_Canvas_NumBatchesCreated);

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

	RedrawMap();
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
    BuildingCountMap.Empty();

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
        BuildingCountMap.FindOrAdd(NewBuildingData.BuildableClassHash)++;

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
            BuildingCountMap.FindOrAdd(NewBuildingData.BuildableClassHash)++;

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
	ExecuteRedrawMapCoroutine();
}


// AddedBuildings/RemovedBuildings: Intentional copies
UE5Coro::TCoroutine<> UCartographGameInstanceModule::RedrawMapCoroutine(
	TArray<FBuildingData> AddedBuildings, TArray<FBuildingData> RemovedBuildings, FForceLatentCoroutine)
{
	ON_SCOPE_EXIT
	{
        OnCoroutineFinishedOrCancelled();
	};

	CARTO_LOG("RedrawMapCoroutine Started");

	const float TimeBudget = FCartograph_ConfigStruct::GetActiveConfig(GetWorld()).RedrawTimeBudget;
	UE5Coro::Latent::FTickTimeBudget Budget = UE5Coro::Latent::FTickTimeBudget::Milliseconds(TimeBudget);

	{
        UE5Coro::FCancellationGuard Guard{};  // We'll lose added/removed building information if the coroutine is cancelled

		for (FBuildingData& AddedBuildingData : AddedBuildings)
		{
	        CARTO_LOG_DEBUG("AddedBuilding: %u", AddedBuildingData.BuildableClassHash);

			const int32 Pos = Algo::LowerBound(CurrentBuildingData, AddedBuildingData);
			CurrentBuildingData.Insert(std::move(AddedBuildingData), Pos);
            BuildingCountMap.FindOrAdd(AddedBuildingData.BuildableClassHash)++;

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
                    BuildingCountMap.FindChecked(RemovedBuildingData.BuildableClassHash)--;
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

	clear_render_target_portion(Canvas, FBox2D{ { WEST_BOUND_CENTIMETERS, NORTH_BOUND_CENTIMETERS }, { EAST_BOUND_CENTIMETERS, SOUTH_BOUND_CENTIMETERS } });

    const int32 Min = Algo::LowerBound(CurrentBuildingData, MinZFilter);
    const int32 Max = Algo::UpperBound(CurrentBuildingData, MaxZFilter);
    if (Min >= CurrentBuildingData.Num() || Max <= 0)
    {
        co_return;
    }

	CARTO_LOG_DEBUG("From %d to %d out of %d", Min, Max, CurrentBuildingData.Num());

	for (int32 i = Min; i < Max; i++)
	{
        const auto& [ClassHash, Transform/*, CustomizationData*/, BuildableExtraData, DataType, DataCache, LayerDataCache] = CurrentBuildingData[i];

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

			const auto& [CategoryData, LocalCorners] = *RectangleDataCachePtr;
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

		co_await Budget;
	}

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
	ExecuteRedrawMapCoroutine();
}


void UCartographGameInstanceModule::ExecuteRedrawMapCoroutine()
{
	Coroutine = RedrawMapCoroutine(PendingAddBuildingData, PendingRemoveBuildingData);
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
	const float Length = MaxHeight - MinHeight;
	MinZFilter = FMath::Floor(Min * Length + MinHeight);
	MaxZFilter = FMath::CeilToInt(Max * Length + MinHeight);

	CARTO_LOG("Min is now %f and max is now %f", MinZFilter, MaxZFilter);

	if (!IsInitializing)
	{
		RedrawMap();
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
		if (!OriginalBuildableClass)
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


#if WITH_EDITOR
void UCartographGameInstanceModule::PostCDOContruct()
{
	// Need to redo it every time the game is updated.
	return;

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
			if (!AssetPath.GetPackageName().ToString().StartsWith("/Game/FactoryGame")
				|| Name.StartsWith("SKEL_") || Name.StartsWith("REINST_"))
			{
				continue;
			}

			const uint32 Hash = TextKeyUtil::HashString(AssetPath.ToString());
			ClassPtrToClassIDMap.Add(Class, Hash);
			ClassIDToClassPtrMap.Add(Hash, Class);
			CARTO_LOG("Path: %s, Class: %s, Hash: %u", *AssetPath.ToString(), *Name, Hash);
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
			if (!AssetPath.GetPackageName().ToString().StartsWith("/Game/FactoryGame")
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
                    CARTO_LOG("SubCategory is null: %s", *Name);
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
			
            CARTO_LOG("Path: %s, Class: %s, BuildableClass: %s",
				*AssetPath.ToString(), 
				*Name, 
				*BuildableClass->GetName());
		}
	}
}
#endif


#undef LOCTEXT_NAMESPACE
