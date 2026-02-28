// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CustomTexture2D : ModuleRules
{
	public CustomTexture2D(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine"
			});
	}
}
