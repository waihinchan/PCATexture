// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PCAMaterialConfigurator.generated.h"

class UMaterialInstanceConstant;
class UPCACompressedTextureAsset;

UCLASS(BlueprintType)
class PCACOMPRESSEDEDITOR_API UPCAMaterialConfigurator : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="PCA Configuration", meta=(DisplayName="Target Material Instance"))
	TObjectPtr<UMaterialInstanceConstant> TargetMaterialInstance;

	UPROPERTY(EditAnywhere, Category="PCA Configuration", meta=(DisplayName="PCA Asset"))
	TObjectPtr<UPCACompressedTextureAsset> PCAAsset;

	UPROPERTY(EditAnywhere, Category="PCA Configuration", meta=(DisplayName="Parameter Prefix", ToolTip="If you used a Parameter Group Prefix when generating the nodes in the Master Material, enter it here so the names match."))
	FString ParameterPrefix;

	UPROPERTY(EditAnywhere, Category="PCA Configuration", meta=(DisplayName="Auto Sync on Change"))
	bool bAutoSync = true;

	UFUNCTION(CallInEditor, Category="PCA Actions", meta=(DisplayName="Apply PCA Data To Material"))
	void ApplyPCAAssetToMaterial();

	void ApplyPCAAssetToMaterial_Internal(bool bSilent = false);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
