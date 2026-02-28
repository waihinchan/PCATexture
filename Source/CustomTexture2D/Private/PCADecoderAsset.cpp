// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCADecoderAsset.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(PCADecoderAsset)

UPCADecoderAsset::UPCADecoderAsset()
{
	CustomHLSL = TEXT("float4 c1 = Texture2DSample(texture1, texture1Sampler, UV);\nreturn c1 * param1;");
	OutputPins.Add(TEXT("Decoded RGBA"));
}
