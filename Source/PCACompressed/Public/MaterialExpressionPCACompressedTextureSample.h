// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionCustom.h"
#include "PCACompressedTextureAsset.h"
#include "MaterialExpressionPCACompressedTextureSample.generated.h"

UCLASS(collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionPCACompressedTextureSample : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = MaterialExpressionPCACompressedTextureSample)
	TObjectPtr<UPCACompressedTextureAsset> PCATextureAsset;

	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Defaults to 'ConstCoordinate' if not specified"))
	FExpressionInput Coordinates;

	/** only used if Coordinates is not hooked up */
	UPROPERTY(EditAnywhere, Category = MaterialExpressionPCACompressedTextureSample, meta = (OverridingInputProperty = "Coordinates"))
	uint8 ConstCoordinate;

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
	virtual TArray<FExpressionOutput>& GetOutputs() override;
	virtual uint32 GetOutputType(int32 OutputIndex) override;

	// These are required by the Material Compiler to track texture references properly and avoid crashes
	virtual UObject* GetReferencedTexture() const override;
	virtual bool CanReferenceTexture() const override { return true; }
	virtual void GetReferencedTextureCollection(TArray<UObject*>& OutTextures);
#endif
	//~ End UMaterialExpression Interface

private:
#if WITH_EDITOR
	void RebuildOutputsFromAsset();
	void SyncInternalCustomExpression();
#endif
};
