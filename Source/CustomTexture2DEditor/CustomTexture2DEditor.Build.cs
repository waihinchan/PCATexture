// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CustomTexture2DEditor : ModuleRules
{
	public CustomTexture2DEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CustomTexture2D",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"AssetTools",
				"ImageCore",
				"ContentBrowser",
				"AssetRegistry"
			});
	}
}
