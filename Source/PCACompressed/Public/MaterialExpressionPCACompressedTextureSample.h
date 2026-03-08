// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionCustom.h"
#include "PCACompressedTextureAsset.h"
#include "MaterialExpressionPCACompressedTextureSample.generated.h"

USTRUCT()
struct FPCANodeInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=PCANodeInput)
	FName InputName;

	UPROPERTY()
	FExpressionInput Input;
};

UCLASS(collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionPCACompressedTextureSample : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = MaterialExpressionPCACompressedTextureSample)
	TObjectPtr<UPCACompressedTextureAsset> PCATextureAsset;

	/** Optional prefix for the exposed material parameters. Automatically generates parameters when the asset is set. */
	UPROPERTY(EditAnywhere, Category = MaterialExpressionPCACompressedTextureSample)
	FString ParameterGroupPrefix;

	/** Click to generate and link parameter nodes for all empty inputs */
	UFUNCTION(CallInEditor, Category = MaterialExpressionPCACompressedTextureSample)
	void GenerateParameterNodes();

	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Defaults to 'ConstCoordinate' if not specified"))
	FExpressionInput Coordinates;

	/** only used if Coordinates is not hooked up */
	UPROPERTY(EditAnywhere, Category = MaterialExpressionPCACompressedTextureSample, meta = (OverridingInputProperty = "Coordinates"))
	uint8 ConstCoordinate;

	UPROPERTY()
	TArray<FPCANodeInput> DynamicInputs;

	// We use an internal custom expression node to leverage the engine's HLSL compilation framework
	UPROPERTY()
	TObjectPtr<UMaterialExpressionCustom> InternalCustomExpression;

	//~ Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

	//~ Begin UMaterialExpression Interface
#if WITH_EDITOR
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual TArrayView<FExpressionInput*> GetInputsView() override;
	virtual FExpressionInput* GetInput(int32 InputIndex) override;
	virtual FName GetInputName(int32 InputIndex) const override;

	// These are required by the Material Compiler to track texture references properly and avoid crashes
	virtual UObject* GetReferencedTexture() const override;
	virtual bool CanReferenceTexture() const override { return true; }
#endif
	//~ End UMaterialExpression Interface

private:
#if WITH_EDITOR
	void RebuildOutputsFromAsset();
	void SyncInternalCustomExpression();
	
	TArray<FExpressionInput*> InputsView;
#endif
};
