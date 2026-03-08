// Copyright Epic Games, Inc. All Rights Reserved.

#include "../Public/PCACompressedEditorModule.h"
#include "AssetTypeActions_PCACompressedTexture.h"
#include "AssetTypeActions_PCADecoder.h"
#include "PCACompressedTextureAsset.h"
#include "PCACompressedTextureFactory.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Engine/Texture2D.h"
#include "Misc/MessageDialog.h"
#include "Misc/MonitoredProcess.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "EditorFramework/AssetImportData.h"
#include "Factories/TextureFactory.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PackageTools.h"
#include "Misc/ScopedSlowTask.h"
#include "Async/Async.h"
#include "Editor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "PCAMaterialConfigurator.h"

#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "../Public/PCACompressedSettings.h"

#define LOCTEXT_NAMESPACE "FPCACompressedEditorModule"

void FPCACompressedEditorModule::StartupModule()
{
	// Register settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "PCA Compressed Textures",
			LOCTEXT("RuntimeSettingsName", "PCA Compressed Textures"),
			LOCTEXT("RuntimeSettingsDescription", "Configure the Python environment for PCA texture compression"),
			GetMutableDefault<UPCACompressedSettings>()
		);
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	
	// Register PCA Texture Asset
	TSharedRef<IAssetTypeActions> PCAAction = MakeShareable(new FAssetTypeActions_PCACompressedTexture());
	AssetTools.RegisterAssetTypeActions(PCAAction);
	RegisteredAssetTypeActions.Add(PCAAction);

	// Register PCA Decoder Asset
	TSharedRef<IAssetTypeActions> DecoderAction = MakeShareable(new FAssetTypeActions_PCADecoder());
	AssetTools.RegisterAssetTypeActions(DecoderAction);
	RegisteredAssetTypeActions.Add(DecoderAction);

	// Register Content Browser Context Menu Extender
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBAssetMenuExtenders = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	CBAssetMenuExtenders.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(this, &FPCACompressedEditorModule::OnExtendContentBrowserAssetSelectionMenu));

	// Register Tab Spawner
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		TEXT("PCAMaterialConfiguratorTab"),
		FOnSpawnTab::CreateRaw(this, &FPCACompressedEditorModule::OnSpawnMaterialConfiguratorTab)
	)
	.SetDisplayName(LOCTEXT("PCAMaterialConfiguratorTabTitle", "PCA Material Configurator"))
	.SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory());
}

void FPCACompressedEditorModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TEXT("PCAMaterialConfiguratorTab"));

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "PCA Compressed Textures");
	}

	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (int32 i = 0; i < RegisteredAssetTypeActions.Num(); ++i)
		{
			if (RegisteredAssetTypeActions[i].IsValid())
			{
				AssetTools.UnregisterAssetTypeActions(RegisteredAssetTypeActions[i].ToSharedRef());
			}
		}
	}
	RegisteredAssetTypeActions.Empty();
}

TSharedRef<FExtender> FPCACompressedEditorModule::OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();

	// Only show if everything selected is a UTexture2D
	bool bAllTextures = SelectedAssets.Num() > 0;
	for (const FAssetData& AssetData : SelectedAssets)
	{
		if (!AssetData.IsInstanceOf(UTexture2D::StaticClass()))
		{
			bAllTextures = false;
			break;
		}
	}

	if (bAllTextures)
	{
		Extender->AddMenuExtension(
			"GetAssetActions",
			EExtensionHook::After,
			nullptr,
			FMenuExtensionDelegate::CreateRaw(this, &FPCACompressedEditorModule::AddPCAMenuEntry, SelectedAssets)
		);
	}

	return Extender;
}

void FPCACompressedEditorModule::AddPCAMenuEntry(FMenuBuilder& MenuBuilder, TArray<FAssetData> SelectedAssets)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("CreatePCATexture_Label", "Create PCA Compressed Texture"),
		LOCTEXT("CreatePCATexture_Tooltip", "Creates a new PCA Compressed Texture asset containing the selected textures."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FPCACompressedEditorModule::ExecuteCreatePCATexture, SelectedAssets)
		)
	);
}

void FPCACompressedEditorModule::ExecuteCreatePCATexture(TArray<FAssetData> SelectedAssets)
{
	if (SelectedAssets.Num() == 0) return;

	// Make sure we're not already running one
	if (PythonProcess.IsValid() && PythonProcess->Update())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ProcessRunning", "A PCA Compression process is already running. Please wait for it to finish."));
		return;
	}

	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// Determine default name and path based on the first selected texture
	FString DefaultName = SelectedAssets[0].AssetName.ToString() + TEXT("_PCA");
	FString PackagePath = SelectedAssets[0].PackagePath.ToString();

	UPCACompressedTextureFactory* Factory = NewObject<UPCACompressedTextureFactory>();

	UObject* NewAsset = AssetTools.CreateAssetWithDialog(DefaultName, PackagePath, UPCACompressedTextureAsset::StaticClass(), Factory);
	PendingPCATexture = Cast<UPCACompressedTextureAsset>(NewAsset);
	
	if (!PendingPCATexture) return;

	PendingPCATexture->RawTextures.Empty();

	TArray<FString> InputImagePaths;
	for (int32 i = 0; i < SelectedAssets.Num(); ++i)
	{
		if (UTexture2D* Tex = Cast<UTexture2D>(SelectedAssets[i].GetAsset()))
		{
			PendingPCATexture->RawTextures.Add(Tex);
			
			// Find the source file on disk
			if (Tex->AssetImportData)
			{
				FString SourcePath = Tex->AssetImportData->GetFirstFilename();
				if (!SourcePath.IsEmpty() && FPaths::FileExists(SourcePath))
				{
					InputImagePaths.Add(SourcePath);
				}
				else
				{
					FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("MissingSource", "Source file for texture '{0}' not found at path:\n{1}"), FText::FromString(Tex->GetName()), FText::FromString(SourcePath)));
					return;
				}
			}
			else
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("NoImportData", "Texture '{0}' has no AssetImportData (it may not be a valid imported source texture)."), FText::FromString(Tex->GetName())));
				return;
			}
		}
	}

	if (InputImagePaths.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoValidTextures", "No valid source textures were found to process."));
		return;
	}

	PendingPCATexture->MarkPackageDirty();

	// Prepare Python process execution
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("PCACompressed"));
	if (!Plugin.IsValid()) return;
	
	FString PluginDir = Plugin->GetBaseDir();
	FString ScriptPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(PluginDir, TEXT("Resources/MLScripts/pca_runner.py")));
	FString TempDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PCATemp")));
	
	PendingOutputDir = TempDir;
	PendingAssetName = DefaultName;
	PendingPackagePath = PackagePath;

	// Read Python executable from settings
	const UPCACompressedSettings* Settings = GetDefault<UPCACompressedSettings>();
	FString Executable = Settings->GetPythonExecutable();
	int32 Iterations = Settings->TrainingIterations;

	// Build command arguments
	FString Args = FString::Printf(TEXT("\"%s\" --out_dir \"%s\" --name \"%s\" --iters %d --inputs"), *ScriptPath, *TempDir, *DefaultName, Iterations);
	for (const FString& InputPath : InputImagePaths)
	{
		Args += FString::Printf(TEXT(" \"%s\""), *InputPath);
	}
	
	PythonLogOutput.Empty();
	
	PythonProcess = MakeShared<FMonitoredProcess>(Executable, Args, true, true);
	
	PythonProcess->OnOutput().BindRaw(this, &FPCACompressedEditorModule::OnPythonOutput);
	PythonProcess->OnCanceled().BindRaw(this, &FPCACompressedEditorModule::OnPythonCanceled);
	PythonProcess->OnCompleted().BindRaw(this, &FPCACompressedEditorModule::OnPythonCompleted);

	CompressionTask = MakeShared<FScopedSlowTask>((float)Iterations, LOCTEXT("CompressingTextures", "Compressing Textures..."));
	CompressionTask->MakeDialog(true); // Allow cancellation
	
	PythonProcess->Launch();
}

void FPCACompressedEditorModule::OnPythonOutput(FString Output)
{
	AsyncTask(ENamedThreads::GameThread, [this, Output]()
	{
		if (Output.Contains(TEXT("[UE_PROGRESS]")))
		{
			// Output format: "[UE_PROGRESS] iter/max - msg"
			FString Prefix = TEXT("[UE_PROGRESS]");
			int32 StartIdx = Output.Find(Prefix) + Prefix.Len();
			FString ProgressStr = Output.RightChop(StartIdx).TrimStartAndEnd();
			
			// Try to parse "10/500 - msg"
			FString FractionStr, MsgStr;
			if (ProgressStr.Split(TEXT("-"), &FractionStr, &MsgStr))
			{
				FString CurrentStr, MaxStr;
				if (FractionStr.Split(TEXT("/"), &CurrentStr, &MaxStr))
				{
					float Current = FCString::Atof(*CurrentStr);
					float Max = FCString::Atof(*MaxStr);
					if (Max > 0 && CompressionTask.IsValid())
					{
						// Calculate tick amount
						if (Current == 0.0f) LastProgress = 0.0f; // reset

						float Delta = Current - LastProgress;
						CompressionTask->EnterProgressFrame(Delta, FText::FromString(MsgStr.TrimStartAndEnd()));
						LastProgress = Current;
					}
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Python: %s"), *Output);
			PythonLogOutput += Output + TEXT("\n");
		}

		if (CompressionTask.IsValid() && CompressionTask->ShouldCancel())
		{
			if (PythonProcess.IsValid())
			{
				PythonProcess->Cancel(true);
			}
		}
	});
}

void FPCACompressedEditorModule::OnPythonCanceled()
{
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		CompressionTask.Reset();
		PythonProcess.Reset();
		PendingPCATexture = nullptr;
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("CompressionCanceled", "PCA Compression was canceled."));
	});
}

void FPCACompressedEditorModule::OnPythonCompleted(int32 ReturnCode)
{
	// NOTE: We cannot safely call AssetTools.ImportAssets from inside an AsyncTask directly due to
	// complex task graph recursion guard assertions in Interchange.
	// Instead, we dispatch it via the editor's timer/tick or queue it to be run on the next frame safely.
	
	AsyncTask(ENamedThreads::GameThread, [this, ReturnCode]()
	{
		CompressionTask.Reset();
		PythonProcess.Reset();

		if (ReturnCode == 0)
		{
			// Postpone the import to the next tick to avoid Interchange AsyncTask recursion issues
			GEditor->GetTimerManager()->SetTimerForNextTick(FTimerDelegate::CreateRaw(this, &FPCACompressedEditorModule::ImportPythonResults));
		}
		else
		{
			PendingPCATexture = nullptr;
			FString ErrorMsg = FString::Printf(TEXT("PCA Compression failed with return code %d.\n\nPython Output:\n%s"), ReturnCode, *PythonLogOutput);
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ErrorMsg));
		}
	});
}

void FPCACompressedEditorModule::ImportPythonResults()
{
	if (!PendingPCATexture) return;

	FString LatentImagePath = FPaths::Combine(PendingOutputDir, PendingAssetName + TEXT("_latent.png"));
	FString ParamsJsonPath = FPaths::Combine(PendingOutputDir, PendingAssetName + TEXT("_params.json"));

	if (!FPaths::FileExists(LatentImagePath) || !FPaths::FileExists(ParamsJsonPath))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("MissingResults", "Failed to find generated Python output files."));
		PendingPCATexture = nullptr;
		return;
	}

	// 1. Import Latent Texture
	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
	
	TArray<FString> FilesToImport;
	FilesToImport.Add(LatentImagePath);
	
	TArray<UObject*> ImportedAssets = AssetTools.ImportAssets(FilesToImport, PendingPackagePath, nullptr, false);
	UTexture2D* LatentTex = nullptr;
	if (ImportedAssets.Num() > 0)
	{
		LatentTex = Cast<UTexture2D>(ImportedAssets[0]);
		if (LatentTex)
		{
			// Make sure compression settings are set to standard compression (BC1/BC3) without sRGB.
			LatentTex->SRGB = false;
			LatentTex->CompressionSettings = TC_Default; // Standard DXT/BC compression
			LatentTex->UpdateResource();
			LatentTex->MarkPackageDirty();

			FPCATextureParam NewParam;
			NewParam.Name = TEXT("LatentMap");
			NewParam.Texture = LatentTex;
			PendingPCATexture->FeatureMaps.Empty();
			PendingPCATexture->FeatureMaps.Add(NewParam);
		}
	}

	// 2. Read Parameters from JSON
	FString JsonContent;
	if (FFileHelper::LoadFileToString(JsonContent, *ParamsJsonPath))
	{
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
		TSharedPtr<FJsonObject> JsonObj;
		if (FJsonSerializer::Deserialize(Reader, JsonObj))
		{
			PendingPCATexture->Parameters.Empty();

			// Expecting JSON like { "Decoder_Params_Weight_0": [val, val, val...], ... }
			for (auto& Elem : JsonObj->Values)
			{
				FPCAScalarParam Param;
				Param.Name = FName(*Elem.Key);
				
				if (Elem.Value->Type == EJson::Array)
				{
					TArray<TSharedPtr<FJsonValue>> Arr = Elem.Value->AsArray();
					Param.Dimensions = FMath::Clamp(Arr.Num(), 1, 4);
					if (Arr.Num() > 0) Param.Value.X = Arr[0]->AsNumber();
					if (Arr.Num() > 1) Param.Value.Y = Arr[1]->AsNumber();
					if (Arr.Num() > 2) Param.Value.Z = Arr[2]->AsNumber();
					if (Arr.Num() > 3) Param.Value.W = Arr[3]->AsNumber();
				}
				else if (Elem.Value->Type == EJson::Number)
				{
					Param.Dimensions = 1;
					Param.Value.X = Elem.Value->AsNumber();
				}

				PendingPCATexture->Parameters.Add(Param);
			}
		}
	}

	PendingPCATexture->MarkPackageDirty();

	// Success message
	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("CompressionSuccess", "PCA Compression completed successfully and data was imported!"));
	PendingPCATexture = nullptr;
}

TSharedRef<SDockTab> FPCACompressedEditorModule::OnSpawnMaterialConfiguratorTab(const FSpawnTabArgs& SpawnTabArgs)
{
	// Initialize the configurator object if it doesn't exist
	if (!GlobalConfigurator)
	{
		GlobalConfigurator = NewObject<UPCAMaterialConfigurator>();
		GlobalConfigurator->AddToRoot(); // Prevent garbage collection
	}

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;

	TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(GlobalConfigurator);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			DetailsView
		];
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPCACompressedEditorModule, PCACompressedEditor)
