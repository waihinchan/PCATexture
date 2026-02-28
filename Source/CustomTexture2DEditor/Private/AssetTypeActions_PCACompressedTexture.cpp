// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions_PCACompressedTexture.h"
#include "PCACompressedTextureAsset.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_PCACompressedTexture"

FText FAssetTypeActions_PCACompressedTexture::GetName() const
{
	return LOCTEXT("AssetTypeActions_PCACompressedTexture", "PCA Compressed Texture");
}

FColor FAssetTypeActions_PCACompressedTexture::GetTypeColor() const
{
	return FColor(192, 64, 64);
}

UClass* FAssetTypeActions_PCACompressedTexture::GetSupportedClass() const
{
	return UPCACompressedTextureAsset::StaticClass();
}

uint32 FAssetTypeActions_PCACompressedTexture::GetCategories()
{
	return EAssetTypeCategories::Textures;
}

#undef LOCTEXT_NAMESPACE
