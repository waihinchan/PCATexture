// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Texture2D.h"
#include "PCADecoderAsset.h"
#include "PCACompressedTextureAsset.generated.h"

USTRUCT(BlueprintType)
struct FPCATextureParam
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCA Parameters")
	FName Name = "texture1";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCA Parameters")
	TObjectPtr<UTexture2D> Texture;
};

USTRUCT(BlueprintType)
struct FPCAScalarParam
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCA Parameters")
	FName Name = "param1";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCA Parameters")
	FVector4 Value = FVector4(0, 0, 0, 0);

	// To determine if it should be treated as float, float2, float3, or float4 in HLSL
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCA Parameters", meta=(ClampMin="1", ClampMax="4"))
	int32 Dimensions = 4;
};

/**
 * An asset representing a PCA-compressed texture, linking to a PCADecoderAsset
 * and holding the actual texture and parameter payload.
 */
UCLASS(BlueprintType, Blueprintable)
class PCACOMPRESSED_API UPCACompressedTextureAsset : public UObject
{
	GENERATED_BODY()

public:
	UPCACompressedTextureAsset();

	// Reference to the shared decoder asset (HLSL and Output Pins)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCA Configuration")
	TObjectPtr<UPCADecoderAsset> Decoder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCA Inputs")
	TArray<FPCATextureParam> Textures;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCA Inputs")
	TArray<FPCAScalarParam> Parameters;
};
