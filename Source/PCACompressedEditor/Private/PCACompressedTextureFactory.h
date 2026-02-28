// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "PCACompressedTextureFactory.generated.h"

UCLASS()
class UPCACompressedTextureFactory : public UFactory
{
	GENERATED_BODY()

public:
	UPCACompressedTextureFactory();

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override;
	//~ End UFactory Interface
};
