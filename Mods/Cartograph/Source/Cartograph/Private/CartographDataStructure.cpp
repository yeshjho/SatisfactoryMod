#include "CartographDataStructure.h"

#include "FGBuildable.h"
#include "FGBuildableBeam.h"
#include "FGBuildableWire.h"
#include "FGSplineBuildableInterface.h"

#include "CartographGameInstanceModule.h"
#include "QuantizedVector2DSerialization.h"


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


void FBuildingData::AddExtraData(AFGBuildable* Buildable)
{
	CARTO_LOG_ERROR_RETURN_IF_NULL(UCartographGameInstanceModule::Instance);

	const TSoftClassPtr<AFGBuildable> Class = Buildable->GetClass();
	const TSoftClassPtr<AFGBuildable>* RedirectClass = UCartographGameInstanceModule::Instance->BuildableClassRedirectMap.Find(Class);
	const TSubclassOf<AFGBuildable> BuildableClass = RedirectClass ? RedirectClass->Get() : Class.Get();

	if (BuildableClass->ImplementsInterface(UFGSplineBuildableInterface::StaticClass()))
	{
		const auto* Spline = Cast<IFGSplineBuildableInterface>(Buildable);
		const USplineComponent* SplineComponent = Spline->GetSplineComponent();
		Transform = SplineComponent->GetComponentTransform();

		TArray<FVector2D> SplinePoints;
		Algo::Transform(SplineComponent->SplineCurves.Position.Points, SplinePoints,
			[](const FInterpCurvePoint<FVector>& Point) { return FVector2D{ Point.OutVal }; });
		FSplineExtraData ExtraData{
			.SplinePoints = std::move(SplinePoints),
		};

		if (const FSplineData* SplineData = UCartographGameInstanceModule::Instance->BuildableSplineDataMap.Find(BuildableClass.Get());
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

		BuildableExtraData = std::move(ExtraData);
		return;
	}

	if (BuildableClass->IsChildOf(AFGBuildableWire::StaticClass()))
	{
		const auto* Wire = Cast<AFGBuildableWire>(Buildable);
		Transform.SetLocation(Wire->GetConnectionLocation(0));

		BuildableExtraData = FWireExtraData{
			.End = FVector2D{ Wire->GetConnectionLocation(1) },
		};

		return;
	}

	if (BuildableClass->IsChildOf(AFGBuildableBeam::StaticClass()))
	{
		const auto* Beam = Cast<AFGBuildableBeam>(Buildable);

		BuildableExtraData = FBeamExtraData{
			.Length = Beam->GetLength(),
		};

		return;
	}
}


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
