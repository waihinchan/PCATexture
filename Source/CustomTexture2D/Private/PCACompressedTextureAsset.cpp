// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCACompressedTextureAsset.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(PCACompressedTextureAsset)

UPCACompressedTextureAsset::UPCACompressedTextureAsset()
{
	Decoder = nullptr;

	FPCATextureParam DefaultTex;
	DefaultTex.Name = TEXT("texture1");
	Textures.Add(DefaultTex);

	FPCAScalarParam DefaultParam;
	DefaultParam.Name = TEXT("param1");
	DefaultParam.Value = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	DefaultParam.Dimensions = 4;
	Parameters.Add(DefaultParam);
}
