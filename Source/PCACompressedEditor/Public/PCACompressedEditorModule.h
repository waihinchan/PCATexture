// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class IAssetTypeActions;
class FExtender;
class FMenuBuilder;
struct FAssetData;

class FPCACompressedEditorModule : public IModuleInterface
{
public:
	// IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;

	TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets);
	void AddPCAMenuEntry(FMenuBuilder& MenuBuilder, TArray<FAssetData> SelectedAssets);
	void ExecuteCreatePCATexture(TArray<FAssetData> SelectedAssets);
};
