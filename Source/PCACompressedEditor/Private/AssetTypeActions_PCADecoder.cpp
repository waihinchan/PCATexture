// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions_PCADecoder.h"
#include "PCADecoderAsset.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_PCADecoder"

FText FAssetTypeActions_PCADecoder::GetName() const
{
	return LOCTEXT("AssetTypeActions_PCADecoder", "PCA Decoder (HLSL)");
}

FColor FAssetTypeActions_PCADecoder::GetTypeColor() const
{
	return FColor(64, 192, 192); // Cyan-ish for code/logic assets
}

UClass* FAssetTypeActions_PCADecoder::GetSupportedClass() const
{
	return UPCADecoderAsset::StaticClass();
}

uint32 FAssetTypeActions_PCADecoder::GetCategories()
{
	return EAssetTypeCategories::Materials;
}

#undef LOCTEXT_NAMESPACE
