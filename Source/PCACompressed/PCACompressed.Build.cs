// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PCACompressed : ModuleRules
{
	public PCACompressed(ReadOnlyTargetRules Target) : base(Target)
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
