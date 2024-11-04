// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class FactoryGameSteamTarget : FactoryGameTarget
{
    public FactoryGameSteamTarget(TargetInfo Target) : base(Target)
    {
	    // CustomConfig = "Steam"; // MODDING EDIT: Cannot deploy targets with conflicting CustomConfig values
	    GlobalDefinitions.Add("STOREFRONT_ID=steam");
    }
}
