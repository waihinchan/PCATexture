// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PCADecoderAsset.generated.h"

UENUM(BlueprintType)
enum class EPCAPinType : uint8
{
	Float1 = 0,
	Float2 = 1,
	Float3 = 2,
	Float4 = 3
};

USTRUCT(BlueprintType)
struct FPCADecoderPin
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCA Decoder")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCA Decoder")
	EPCAPinType Type = EPCAPinType::Float4;
};

/**
 * A technical asset defining the HLSL logic and output pin structure
 * for a PCA-compressed texture. Typically authored by a Technical Artist.
 */
UCLASS(BlueprintType, Blueprintable)
class PCACOMPRESSED_API UPCADecoderAsset : public UObject
{
	GENERATED_BODY()

public:
	UPCADecoderAsset();

	// The custom HLSL code. e.g. "return texture1_sample * param1;"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCA Decoder", meta=(MultiLine="true"))
	FString CustomHLSL;

	// Names for the output pins on the material node
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCA Decoder Outputs")
	TArray<FPCADecoderPin> OutputPins;

	virtual void PostLoad() override;
};
