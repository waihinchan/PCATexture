// Copyright Epic Games, Inc. All Rights Reserved.

#include "../Public/PCAMaterialConfigurator.h"
#include "Materials/MaterialInstanceConstant.h"
#include "PCACompressedTextureAsset.h"
#include "Misc/MessageDialog.h"
#include "Materials/MaterialParameterCollection.h"

void UPCAMaterialConfigurator::ApplyPCAAssetToMaterial()
{
	ApplyPCAAssetToMaterial_Internal(false);
}

void UPCAMaterialConfigurator::ApplyPCAAssetToMaterial_Internal(bool bSilent)
{
	if (!TargetMaterialInstance)
	{
		if (!bSilent) FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Please select a Target Material Instance!")));
		return;
	}

	if (!PCAAsset)
	{
		if (!bSilent) FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Please select a PCA Asset!")));
		return;
	}

	TargetMaterialInstance->PreEditChange(nullptr);

	int32 AppliedParams = 0;

	// Apply Texture Parameters
	for (const FPCATextureParam& TexParam : PCAAsset->FeatureMaps)
	{
		if (TexParam.Texture)
		{
			FName ParamName = ParameterPrefix.IsEmpty() ? TexParam.Name : FName(*(ParameterPrefix + TEXT("_") + TexParam.Name.ToString()));
			FMaterialParameterInfo ParamInfo(ParamName);
			TargetMaterialInstance->SetTextureParameterValueEditorOnly(ParamInfo, TexParam.Texture);
			AppliedParams++;
		}
	}

	// Apply Scalar/Vector Parameters
	for (const FPCAScalarParam& ScalParam : PCAAsset->Parameters)
	{
		FName ParamName = ParameterPrefix.IsEmpty() ? ScalParam.Name : FName(*(ParameterPrefix + TEXT("_") + ScalParam.Name.ToString()));
		FMaterialParameterInfo ParamInfo(ParamName);
		
		if (ScalParam.Dimensions == 1)
		{
			TargetMaterialInstance->SetScalarParameterValueEditorOnly(ParamInfo, ScalParam.Value.X);
			AppliedParams++;
		}
		else
		{
			FLinearColor VecValue(ScalParam.Value.X, ScalParam.Value.Y, ScalParam.Value.Z, ScalParam.Value.W);
			TargetMaterialInstance->SetVectorParameterValueEditorOnly(ParamInfo, VecValue);
			AppliedParams++;
		}
	}

	TargetMaterialInstance->PostEditChange();
	TargetMaterialInstance->MarkPackageDirty();

	if (!bSilent)
	{
		FString SuccessMsg = FString::Printf(TEXT("Successfully applied %d parameters from PCA Asset to Material Instance!"), AppliedParams);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(SuccessMsg));
	}
}

#if WITH_EDITOR
void UPCAMaterialConfigurator::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (bAutoSync)
	{
		FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UPCAMaterialConfigurator, PCAAsset) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UPCAMaterialConfigurator, TargetMaterialInstance) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UPCAMaterialConfigurator, ParameterPrefix) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UPCAMaterialConfigurator, bAutoSync))
		{
			if (TargetMaterialInstance && PCAAsset)
			{
				ApplyPCAAssetToMaterial_Internal(true);
			}
		}
	}
}
#endif
