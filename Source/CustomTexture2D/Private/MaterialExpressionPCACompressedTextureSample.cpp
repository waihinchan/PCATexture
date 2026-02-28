// Copyright Epic Games, Inc. All Rights Reserved.

#include "MaterialExpressionPCACompressedTextureSample.h"
#include "MaterialCompiler.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MaterialExpressionPCACompressedTextureSample)

UMaterialExpressionPCACompressedTextureSample::UMaterialExpressionPCACompressedTextureSample(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FText NAME_PCATexture;
		FConstructorStatics()
			: NAME_PCATexture(NSLOCTEXT("MaterialExpression", "PCATexture", "PCA Texture"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

#if WITH_EDITORONLY_DATA
	MenuCategories.Add(ConstructorStatics.NAME_PCATexture);
	bShowOutputNameOnPin = true;

	InternalCustomExpression = CreateDefaultSubobject<UMaterialExpressionCustom>(TEXT("InternalCustomNode"));
#endif
}

#if WITH_EDITOR

void UMaterialExpressionPCACompressedTextureSample::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildOutputsFromAsset();
}

void UMaterialExpressionPCACompressedTextureSample::RebuildOutputsFromAsset()
{
	Outputs.Reset();
	
	if (PCATextureAsset && PCATextureAsset->Decoder)
	{
		for (const FName& PinName : PCATextureAsset->Decoder->OutputPins)
		{
			// All dynamically generated pins default to Float4 output
			Outputs.Add(FExpressionOutput(PinName, 1, 1, 1, 1, 1));
		}
	}
	
	if (Outputs.Num() == 0)
	{
		Outputs.Add(FExpressionOutput(TEXT("Error: Missing Decoder"), 1, 1, 1, 1, 1));
	}
	
	SyncInternalCustomExpression();
}

void UMaterialExpressionPCACompressedTextureSample::SyncInternalCustomExpression()
{
	if (!InternalCustomExpression || !PCATextureAsset || !PCATextureAsset->Decoder)
	{
		return;
	}

	InternalCustomExpression->Code = PCATextureAsset->Decoder->CustomHLSL;
	InternalCustomExpression->Description = TEXT("Internal PCA Code");
	
	// Configure Outputs
	InternalCustomExpression->AdditionalOutputs.Empty();
	if (PCATextureAsset->Decoder->OutputPins.Num() > 0)
	{
		// First output is the main one
		InternalCustomExpression->OutputType = ECustomMaterialOutputType::CMOT_Float4;
		
		// Remaining outputs go to AdditionalOutputs
		for (int32 i = 1; i < PCATextureAsset->Decoder->OutputPins.Num(); ++i)
		{
			FCustomOutput NewOutput;
			NewOutput.OutputName = PCATextureAsset->Decoder->OutputPins[i];
			NewOutput.OutputType = ECustomMaterialOutputType::CMOT_Float4;
			InternalCustomExpression->AdditionalOutputs.Add(NewOutput);
		}
	}
	
	// Setup custom inputs
	InternalCustomExpression->Inputs.Empty();
	
	// Default UV input
	FCustomInput UVInput;
	UVInput.InputName = TEXT("UV");
	InternalCustomExpression->Inputs.Add(UVInput);

	// Add Texture inputs
	for (const FPCATextureParam& TexParam : PCATextureAsset->Textures)
	{
		FCustomInput NewTexInput;
		NewTexInput.InputName = TexParam.Name;
		InternalCustomExpression->Inputs.Add(NewTexInput);
	}
	
	// Add Param inputs
	for (const FPCAScalarParam& ScalarParam : PCATextureAsset->Parameters)
	{
		FCustomInput NewScalarInput;
		NewScalarInput.InputName = ScalarParam.Name;
		InternalCustomExpression->Inputs.Add(NewScalarInput);
	}
	
	InternalCustomExpression->RebuildOutputs();
}

int32 UMaterialExpressionPCACompressedTextureSample::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex)
{
	if (!PCATextureAsset)
	{
		return Compiler->Errorf(TEXT("Missing PCA Compressed Texture Asset"));
	}

	if (!PCATextureAsset->Decoder)
	{
		return Compiler->Errorf(TEXT("PCA Compressed Texture Asset is missing a linked Decoder"));
	}

	// 1. Validate HLSL matches the parameters
	FString UserHLSL = PCATextureAsset->Decoder->CustomHLSL;
	
	for (const FPCATextureParam& TexParam : PCATextureAsset->Textures)
	{
		if (!UserHLSL.Contains(TexParam.Name.ToString()))
		{
			return Compiler->Errorf(TEXT("HLSL validation failed: Texture '%s' is defined in the asset but not used in the Decoder HLSL."), *TexParam.Name.ToString());
		}
	}
	
	for (const FPCAScalarParam& ScalarParam : PCATextureAsset->Parameters)
	{
		if (!UserHLSL.Contains(ScalarParam.Name.ToString()))
		{
			return Compiler->Errorf(TEXT("HLSL validation failed: Parameter '%s' is defined in the asset but not used in the Decoder HLSL."), *ScalarParam.Name.ToString());
		}
	}

	// 2. Compile Inputs
	TArray<int32> CompiledInputs;
	
	// Compile UV
	int32 CoordinateIndex = Coordinates.GetTracedInput().Expression ? Coordinates.Compile(Compiler) : Compiler->TextureCoordinate(ConstCoordinate, false, false);
	CompiledInputs.Add(CoordinateIndex);
	
	// Compile Textures
	for (const FPCATextureParam& TexParam : PCATextureAsset->Textures)
	{
		if (!TexParam.Texture)
		{
			return Compiler->Errorf(TEXT("Missing texture for parameter '%s'"), *TexParam.Name.ToString());
		}
		
		int32 TextureReferenceIndex = INDEX_NONE;
		int32 TextureCodeIndex = Compiler->Texture(TexParam.Texture, TextureReferenceIndex, SAMPLERTYPE_Color, SSM_FromTextureAsset, TMVM_None);
		
		if (TextureCodeIndex == INDEX_NONE)
		{
			return INDEX_NONE;
		}
		
		CompiledInputs.Add(TextureCodeIndex);
	}
	
	// Compile Params
	for (const FPCAScalarParam& ScalarParam : PCATextureAsset->Parameters)
	{
		int32 ParamIndex = INDEX_NONE;
		if (ScalarParam.Dimensions == 1)
			ParamIndex = Compiler->Constant(ScalarParam.Value.X);
		else if (ScalarParam.Dimensions == 2)
			ParamIndex = Compiler->Constant2(ScalarParam.Value.X, ScalarParam.Value.Y);
		else if (ScalarParam.Dimensions == 3)
			ParamIndex = Compiler->Constant3(ScalarParam.Value.X, ScalarParam.Value.Y, ScalarParam.Value.Z);
		else
			ParamIndex = Compiler->Constant4(ScalarParam.Value.X, ScalarParam.Value.Y, ScalarParam.Value.Z, ScalarParam.Value.W);
			
		CompiledInputs.Add(ParamIndex);
	}

	// 3. Delegate to Internal Custom Expression
	SyncInternalCustomExpression();
	return Compiler->CustomExpression(InternalCustomExpression, OutputIndex, CompiledInputs);
}

void UMaterialExpressionPCACompressedTextureSample::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("PCA Texture Decoder"));
	if (PCATextureAsset)
	{
		OutCaptions.Add(FString::Printf(TEXT("Asset: %s"), *PCATextureAsset->GetName()));
	}
}

TArrayView<FExpressionInput*> UMaterialExpressionPCACompressedTextureSample::GetInputsView()
{
	static FExpressionInput* Inputs[] = { &Coordinates };
	return Inputs;
}

FExpressionInput* UMaterialExpressionPCACompressedTextureSample::GetInput(int32 InputIndex)
{
	if (InputIndex == 0) return &Coordinates;
	return nullptr;
}

FName UMaterialExpressionPCACompressedTextureSample::GetInputName(int32 InputIndex) const
{
	if (InputIndex == 0) return TEXT("UVs");
	return NAME_None;
}

TArray<FExpressionOutput>& UMaterialExpressionPCACompressedTextureSample::GetOutputs()
{
	if (Outputs.Num() == 0)
	{
		RebuildOutputsFromAsset();
	}
	return Outputs;
}

uint32 UMaterialExpressionPCACompressedTextureSample::GetOutputType(int32 OutputIndex)
{
	return MCT_Float4; // We output Float4 for all custom outputs
}

UObject* UMaterialExpressionPCACompressedTextureSample::GetReferencedTexture() const
{
	if (PCATextureAsset && PCATextureAsset->Textures.Num() > 0)
	{
		return PCATextureAsset->Textures[0].Texture;
	}
	return nullptr;
}

void UMaterialExpressionPCACompressedTextureSample::GetReferencedTextureCollection(TArray<UObject*>& OutTextures)
{
	if (PCATextureAsset)
	{
		for (const FPCATextureParam& Tex : PCATextureAsset->Textures)
		{
			if (Tex.Texture)
			{
				OutTextures.Add(Tex.Texture);
			}
		}
	}
}

#endif // WITH_EDITOR
