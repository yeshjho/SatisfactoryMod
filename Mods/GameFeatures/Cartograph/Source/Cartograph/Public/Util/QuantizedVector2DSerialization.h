// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Math/MathFwd.h"
#include "Serialization/Archive.h"


bool WriteQuantizedVector2D(const int32 Scale, const FVector2D& Value, FArchive& Ar);
bool ReadQuantizedVector2D(const int32 Scale, FVector2D& Value, FArchive& Ar);


template<int32 Scale>
bool SerializeQuantizedVector2D(FVector2D& Value, FArchive& Ar)
{
	if (Ar.IsLoading())
	{
		return ReadQuantizedVector2D(Scale, Value, Ar);
	}
	else
	{
		return WriteQuantizedVector2D(Scale, Value, Ar);
	}
}
