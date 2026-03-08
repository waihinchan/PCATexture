// Copyright Epic Games, Inc. All Rights Reserved.

#include "MaterialExpressionPCACompressedTextureSample.h"
#include "MaterialCompiler.h"

#if WITH_EDITOR
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/Material.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "MaterialGraph/MaterialGraph.h"
#include "EdGraph/EdGraph.h"
#include "GraphEditAction.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "TimerManager.h"
#endif

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

	Outputs.Reset();
	Outputs.Add(FExpressionOutput(TEXT("BaseColor"), 1, 1, 1, 1, 0));
	Outputs.Add(FExpressionOutput(TEXT("Normal"), 1, 1, 1, 1, 0));
	Outputs.Add(FExpressionOutput(TEXT("Metallic"), 1, 1, 0, 0, 0));
	Outputs.Add(FExpressionOutput(TEXT("Roughness"), 1, 1, 0, 0, 0));
	Outputs.Add(FExpressionOutput(TEXT("Emissive"), 1, 1, 1, 1, 0));
	Outputs.Add(FExpressionOutput(TEXT("AO"), 1, 1, 0, 0, 0));

	InternalCustomExpression = CreateDefaultSubobject<UMaterialExpressionCustom>(TEXT("InternalCustomNode"));
#endif
}

#if WITH_EDITOR

void UMaterialExpressionPCACompressedTextureSample::GenerateParameterNodes()
{
	if (!PCATextureAsset || !Material || !Material->MaterialGraph) return;

	FScopedTransaction Transaction(FText::FromString("Generate Parameter Nodes"));
	Material->Modify();
	Modify();
	if (GraphNode)
	{
		GraphNode->Modify();
		// Reconstruct our node first to ensure all dynamic pins exist in the UI graph
		GraphNode->ReconstructNode();
	}

	int32 NodeX = MaterialExpressionEditorX - 300;
	int32 NodeY = MaterialExpressionEditorY;
	int32 DynInputIndex = 0;

	bool bCreatedAny = false;

	// Helper for common parameter node initialization
	auto InitParamNode = [&](UMaterialExpression* NewNode)
	{
		NewNode->Material = Material;
		NewNode->MaterialExpressionEditorX = NodeX;
		NewNode->MaterialExpressionEditorY = NodeY;
		NewNode->UpdateMaterialExpressionGuid(true, true);
		NewNode->UpdateParameterGuid(true, true);
		
		Material->GetExpressionCollection().AddExpression(NewNode);
		Material->AddExpressionParameter(NewNode, Material->EditorParameters);
		Material->MaterialGraph->AddExpression(NewNode, true);
	};

	for (const FPCATextureParam& TexParam : PCATextureAsset->FeatureMaps)
	{
		if (DynamicInputs.IsValidIndex(DynInputIndex) && DynamicInputs[DynInputIndex].Input.Expression == nullptr)
		{
			UMaterialExpressionTextureObjectParameter* TexParamNode = NewObject<UMaterialExpressionTextureObjectParameter>(Material, NAME_None, RF_Transactional);
			
			FName ExposeName = ParameterGroupPrefix.IsEmpty() ? TexParam.Name : FName(*(ParameterGroupPrefix + TEXT("_") + TexParam.Name.ToString()));
			TexParamNode->ParameterName = ExposeName;
			TexParamNode->Texture = TexParam.Texture;
			TexParamNode->SamplerType = SAMPLERTYPE_LinearColor;
			if (!ParameterGroupPrefix.IsEmpty()) TexParamNode->Group = FName(*ParameterGroupPrefix);
			
			InitParamNode(TexParamNode);
			
			DynamicInputs[DynInputIndex].Input.Expression = TexParamNode;
			DynamicInputs[DynInputIndex].Input.Mask = 0;
			DynamicInputs[DynInputIndex].Input.MaskR = 0;
			DynamicInputs[DynInputIndex].Input.MaskG = 0;
			DynamicInputs[DynInputIndex].Input.MaskB = 0;
			DynamicInputs[DynInputIndex].Input.MaskA = 0;
			
			// Manually link the Graph pins to avoid needing a global graph rebuild
			if (TexParamNode->GraphNode && GraphNode)
			{
				UEdGraphPin* OutPin = nullptr;
				for (UEdGraphPin* Pin : TexParamNode->GraphNode->Pins)
				{
					if (Pin->Direction == EGPD_Output) { OutPin = Pin; break; }
				}
				UEdGraphPin* InPin = nullptr;
				for (UEdGraphPin* Pin : GraphNode->Pins)
				{
					if (Pin->Direction == EGPD_Input && Pin->PinName == DynamicInputs[DynInputIndex].InputName) { InPin = Pin; break; }
				}
				if (OutPin && InPin)
				{
					OutPin->MakeLinkTo(InPin);
				}
			}
			
			bCreatedAny = true;
		}
		NodeY += 150;
		DynInputIndex++;
	}

	for (const FPCAScalarParam& ScalarParam : PCATextureAsset->Parameters)
	{
		if (DynamicInputs.IsValidIndex(DynInputIndex) && DynamicInputs[DynInputIndex].Input.Expression == nullptr)
		{
			FName ExposeName = ParameterGroupPrefix.IsEmpty() ? ScalarParam.Name : FName(*(ParameterGroupPrefix + TEXT("_") + ScalarParam.Name.ToString()));
			UMaterialExpression* NewParamNode = nullptr;

			if (ScalarParam.Dimensions == 1)
			{
				UMaterialExpressionScalarParameter* ScalarParamNode = NewObject<UMaterialExpressionScalarParameter>(Material, NAME_None, RF_Transactional);
				ScalarParamNode->ParameterName = ExposeName;
				ScalarParamNode->DefaultValue = ScalarParam.Value.X;
				if (!ParameterGroupPrefix.IsEmpty()) ScalarParamNode->Group = FName(*ParameterGroupPrefix);
				NewParamNode = ScalarParamNode;
			}
			else
			{
				UMaterialExpressionVectorParameter* VectorParamNode = NewObject<UMaterialExpressionVectorParameter>(Material, NAME_None, RF_Transactional);
				VectorParamNode->ParameterName = ExposeName;
				VectorParamNode->DefaultValue = FLinearColor(ScalarParam.Value.X, ScalarParam.Value.Y, ScalarParam.Value.Z, ScalarParam.Value.W);
				if (!ParameterGroupPrefix.IsEmpty()) VectorParamNode->Group = FName(*ParameterGroupPrefix);
				NewParamNode = VectorParamNode;
			}

			if (NewParamNode)
			{
				InitParamNode(NewParamNode);
				
				DynamicInputs[DynInputIndex].Input.Expression = NewParamNode;
				DynamicInputs[DynInputIndex].Input.Mask = 0;
				
				if (NewParamNode->GraphNode && GraphNode)
				{
					UEdGraphPin* OutPin = nullptr;
					if (ScalarParam.Dimensions == 4)
					{
						for (UEdGraphPin* Pin : NewParamNode->GraphNode->Pins)
						{
							if (Pin->Direction == EGPD_Output && Pin->PinName == TEXT("RGBA"))
							{
								OutPin = Pin;
								break;
							}
						}
					}
					
					if (!OutPin)
					{
						for (UEdGraphPin* Pin : NewParamNode->GraphNode->Pins)
						{
							if (Pin->Direction == EGPD_Output) { OutPin = Pin; break; }
						}
					}
					
					UEdGraphPin* InPin = nullptr;
					for (UEdGraphPin* Pin : GraphNode->Pins)
					{
						if (Pin->Direction == EGPD_Input && Pin->PinName == DynamicInputs[DynInputIndex].InputName) { InPin = Pin; break; }
					}
					if (OutPin && InPin)
					{
						OutPin->MakeLinkTo(InPin);
					}
				}

				bCreatedAny = true;
			}
		}
		NodeY += 150;
		DynInputIndex++;
	}

	if (bCreatedAny && GraphNode)
	{
		GraphNode->GetGraph()->NotifyGraphChanged();
		Material->PostEditChange();
	}
}

void UMaterialExpressionPCACompressedTextureSample::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	// Check if the property being changed is the PCATextureAsset
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UMaterialExpressionPCACompressedTextureSample, PCATextureAsset))
	{
		RebuildOutputsFromAsset();
		
		// Optional: We can automatically generate parameter nodes when a new asset is set, 
		// but since we had stability issues with auto-generation, it's safer to let the user
		// click the "Generate Parameter Nodes" button manually after changing the asset.
		// However, we MUST rebuild the node's UI to reflect the new dynamic pins.
		if (GraphNode)
		{
			// Safe deferred update for the UI node
			GraphNode->ReconstructNode();
			GraphNode->GetGraph()->NotifyGraphChanged();
		}
	}
}

void UMaterialExpressionPCACompressedTextureSample::RebuildOutputsFromAsset()
{
	Outputs.Reset();
	
	if (PCATextureAsset && PCATextureAsset->Decoder && PCATextureAsset->Decoder->OutputPins.Num() > 0)
	{
		for (const FPCADecoderPin& Pin : PCATextureAsset->Decoder->OutputPins)
		{
			int32 MaskR = 1, MaskG = 1, MaskB = 1, MaskA = 1;
			if (Pin.Type == EPCAPinType::Float1) { MaskG = 0; MaskB = 0; MaskA = 0; }
			else if (Pin.Type == EPCAPinType::Float2) { MaskB = 0; MaskA = 0; }
			else if (Pin.Type == EPCAPinType::Float3) { MaskA = 0; }

			Outputs.Add(FExpressionOutput(Pin.Name, 1, MaskR, MaskG, MaskB, MaskA));
		}
	}
	else
	{
		// Fallback to default PBR pins if no asset is assigned or if decoder is invalid
		Outputs.Add(FExpressionOutput(TEXT("BaseColor"), 1, 1, 1, 1, 0));
		Outputs.Add(FExpressionOutput(TEXT("Normal"), 1, 1, 1, 1, 0));
		Outputs.Add(FExpressionOutput(TEXT("Metallic"), 1, 1, 0, 0, 0));
		Outputs.Add(FExpressionOutput(TEXT("Roughness"), 1, 1, 0, 0, 0));
		Outputs.Add(FExpressionOutput(TEXT("Emissive"), 1, 1, 1, 1, 0));
		Outputs.Add(FExpressionOutput(TEXT("AO"), 1, 1, 0, 0, 0));
	}
	
	// Sync Dynamic Inputs
	TArray<FPCANodeInput> OldInputs = DynamicInputs;
	DynamicInputs.Reset();
	
	if (PCATextureAsset)
	{
		for (const FPCATextureParam& TexParam : PCATextureAsset->FeatureMaps)
		{
			FPCANodeInput NewInput;
			NewInput.InputName = TexParam.Name;
			FPCANodeInput* FoundOld = OldInputs.FindByPredicate([&](const FPCANodeInput& Old) { return Old.InputName == TexParam.Name; });
			if (FoundOld) NewInput.Input = FoundOld->Input;
			DynamicInputs.Add(NewInput);
		}
		for (const FPCAScalarParam& ScalarParam : PCATextureAsset->Parameters)
		{
			FPCANodeInput NewInput;
			NewInput.InputName = ScalarParam.Name;
			FPCANodeInput* FoundOld = OldInputs.FindByPredicate([&](const FPCANodeInput& Old) { return Old.InputName == ScalarParam.Name; });
			if (FoundOld) NewInput.Input = FoundOld->Input;
			DynamicInputs.Add(NewInput);
		}
	}
	
	SyncInternalCustomExpression();
}

void UMaterialExpressionPCACompressedTextureSample::SyncInternalCustomExpression()
{
	if (!InternalCustomExpression || !PCATextureAsset || !PCATextureAsset->Decoder)
	{
		return;
	}

	// Wrapper to force all output pins to be 'out' variables, so the user doesn't have to return the first one.
	FString WrappedHLSL = PCATextureAsset->Decoder->CustomHLSL;
	
	// Create a dummy return so the compiler is happy with the main return type
	WrappedHLSL += TEXT("\n\n// Auto-generated wrapper to satisfy Custom Node return signature\nreturn 0;");

	InternalCustomExpression->Code = WrappedHLSL;
	InternalCustomExpression->Description = TEXT("Internal PCA Code");
	
	// Configure Outputs
	InternalCustomExpression->AdditionalOutputs.Empty();
	if (PCATextureAsset->Decoder->OutputPins.Num() > 0)
	{
		// Force the main output to be a dummy. The actual first pin will be created as an AdditionalOutput.
		// However, Unreal Custom node UI always expects the first output to be the return value.
		// To trick it and make *all* user pins be 'out' variables in the function signature:
		InternalCustomExpression->OutputType = ECustomMaterialOutputType::CMOT_Float1;
		
		// All user defined pins go to AdditionalOutputs
		for (int32 i = 0; i < PCATextureAsset->Decoder->OutputPins.Num(); ++i)
		{
			FCustomOutput NewOutput;
			NewOutput.OutputName = PCATextureAsset->Decoder->OutputPins[i].Name;
			
			EPCAPinType PinType = PCATextureAsset->Decoder->OutputPins[i].Type;
			if (PinType == EPCAPinType::Float1) NewOutput.OutputType = ECustomMaterialOutputType::CMOT_Float1;
			else if (PinType == EPCAPinType::Float2) NewOutput.OutputType = ECustomMaterialOutputType::CMOT_Float2;
			else if (PinType == EPCAPinType::Float3) NewOutput.OutputType = ECustomMaterialOutputType::CMOT_Float3;
			else NewOutput.OutputType = ECustomMaterialOutputType::CMOT_Float4;
			
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
	for (const FPCATextureParam& TexParam : PCATextureAsset->FeatureMaps)
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
	
	for (const FPCATextureParam& TexParam : PCATextureAsset->FeatureMaps)
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
	int32 DynInputIndex = 0;
	for (const FPCATextureParam& TexParam : PCATextureAsset->FeatureMaps)
	{
		int32 TextureCodeIndex = INDEX_NONE;
		
		if (DynamicInputs.IsValidIndex(DynInputIndex) && DynamicInputs[DynInputIndex].Input.GetTracedInput().Expression)
		{
			TextureCodeIndex = DynamicInputs[DynInputIndex].Input.Compile(Compiler);
		}
		else
		{
			if (!TexParam.Texture)
			{
				return Compiler->Errorf(TEXT("Missing texture for parameter '%s'"), *TexParam.Name.ToString());
			}
			
			int32 TextureReferenceIndex = INDEX_NONE;
			// Since we generate parameters automatically, if no pin is connected we fallback to standard texture sampling
			TextureCodeIndex = Compiler->Texture(TexParam.Texture, TextureReferenceIndex, SAMPLERTYPE_LinearColor, SSM_FromTextureAsset, TMVM_None);
		}
		
		if (TextureCodeIndex == INDEX_NONE)
		{
			return INDEX_NONE;
		}
		
		CompiledInputs.Add(TextureCodeIndex);
		DynInputIndex++;
	}
	
	// Compile Params
	for (const FPCAScalarParam& ScalarParam : PCATextureAsset->Parameters)
	{
		int32 ParamIndex = INDEX_NONE;
		
		if (DynamicInputs.IsValidIndex(DynInputIndex) && DynamicInputs[DynInputIndex].Input.GetTracedInput().Expression)
		{
			ParamIndex = DynamicInputs[DynInputIndex].Input.Compile(Compiler);
		}
		else
		{
			if (ScalarParam.Dimensions == 1)
				ParamIndex = Compiler->Constant(ScalarParam.Value.X);
			else if (ScalarParam.Dimensions == 2)
				ParamIndex = Compiler->Constant2(ScalarParam.Value.X, ScalarParam.Value.Y);
			else if (ScalarParam.Dimensions == 3)
				ParamIndex = Compiler->Constant3(ScalarParam.Value.X, ScalarParam.Value.Y, ScalarParam.Value.Z);
			else
				ParamIndex = Compiler->Constant4(ScalarParam.Value.X, ScalarParam.Value.Y, ScalarParam.Value.Z, ScalarParam.Value.W);
		}
			
		CompiledInputs.Add(ParamIndex);
		DynInputIndex++;
	}

	// 3. Delegate to Internal Custom Expression
	SyncInternalCustomExpression();
	
	// Because we added a dummy return value as the first output of the CustomNode (to force 'out' on everything else),
	// the OutputIndex we request from the InternalCustomExpression needs to be shifted by +1.
	int32 InternalOutputIndex = OutputIndex + 1;
	
	return Compiler->CustomExpression(InternalCustomExpression, InternalOutputIndex, CompiledInputs);
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
	InputsView.Reset();
	InputsView.Add(&Coordinates);
	for (FPCANodeInput& DynInput : DynamicInputs)
	{
		InputsView.Add(&DynInput.Input);
	}
	return InputsView;
}

FExpressionInput* UMaterialExpressionPCACompressedTextureSample::GetInput(int32 InputIndex)
{
	if (InputIndex == 0) return &Coordinates;
	int32 DynIndex = InputIndex - 1;
	if (DynamicInputs.IsValidIndex(DynIndex))
	{
		return &DynamicInputs[DynIndex].Input;
	}
	return nullptr;
}

FName UMaterialExpressionPCACompressedTextureSample::GetInputName(int32 InputIndex) const
{
	if (InputIndex == 0) return TEXT("UVs");
	int32 DynIndex = InputIndex - 1;
	if (DynamicInputs.IsValidIndex(DynIndex))
	{
		return DynamicInputs[DynIndex].InputName;
	}
	return NAME_None;
}

UObject* UMaterialExpressionPCACompressedTextureSample::GetReferencedTexture() const
{
	if (PCATextureAsset && PCATextureAsset->FeatureMaps.Num() > 0)
	{
		return PCATextureAsset->FeatureMaps[0].Texture;
	}
	return nullptr;
}



#endif // WITH_EDITOR
