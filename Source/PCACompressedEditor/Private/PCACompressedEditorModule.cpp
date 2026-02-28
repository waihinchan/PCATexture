// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCACompressedEditorModule.h"
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
#include "AssetRegistry/AssetRegistryModule.h"
#include "PackageTools.h"

#define LOCTEXT_NAMESPACE "FPCACompressedEditorModule"

void FPCACompressedEditorModule::StartupModule()
{
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
}

void FPCACompressedEditorModule::ShutdownModule()
{
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

	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// Determine default name and path based on the first selected texture
	FString DefaultName = SelectedAssets[0].AssetName.ToString() + TEXT("_PCA");
	FString PackagePath = SelectedAssets[0].PackagePath.ToString();

	UPCACompressedTextureFactory* Factory = NewObject<UPCACompressedTextureFactory>();

	UObject* NewAsset = AssetTools.CreateAssetWithDialog(DefaultName, PackagePath, UPCACompressedTextureAsset::StaticClass(), Factory);
	if (UPCACompressedTextureAsset* PCATexture = Cast<UPCACompressedTextureAsset>(NewAsset))
	{
		// Clear default templates
		PCATexture->Textures.Empty();

		// Add all selected textures
		for (int32 i = 0; i < SelectedAssets.Num(); ++i)
		{
			if (UTexture2D* Tex = Cast<UTexture2D>(SelectedAssets[i].GetAsset()))
			{
				FPCATextureParam NewParam;
				NewParam.Name = FName(*FString::Printf(TEXT("texture%d"), i + 1));
				NewParam.Texture = Tex;
				PCATexture->Textures.Add(NewParam);
			}
		}

		// Mark package dirty
		PCATexture->MarkPackageDirty();
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPCACompressedEditorModule, PCACompressedEditor)
