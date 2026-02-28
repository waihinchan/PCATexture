// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PCACompressedEditor : ModuleRules
{
	public PCACompressedEditor(ReadOnlyTargetRules Target) : base(Target)
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
				"PCACompressed",
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
