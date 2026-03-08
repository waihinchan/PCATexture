// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Misc/MonitoredProcess.h"
#include "Misc/ScopedSlowTask.h"

class IAssetTypeActions;
class FExtender;
class FMenuBuilder;
struct FAssetData;
class UPCACompressedTextureAsset;

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

	// Async Python execution handlers
	TSharedPtr<FMonitoredProcess> PythonProcess;
	TSharedPtr<FScopedSlowTask> CompressionTask;
	UPCACompressedTextureAsset* PendingPCATexture = nullptr;
	FString PendingOutputDir;
	FString PendingAssetName;
	FString PendingPackagePath;

	// Global Material Configurator Tab
	TSharedRef<class SDockTab> OnSpawnMaterialConfiguratorTab(const class FSpawnTabArgs& SpawnTabArgs);
	class UPCAMaterialConfigurator* GlobalConfigurator = nullptr;

	void OnPythonOutput(FString Output);
	void OnPythonCanceled();
	void OnPythonCompleted(int32 ReturnCode);
	void ImportPythonResults();

	float LastProgress = 0.0f;
	FString PythonLogOutput;
};
