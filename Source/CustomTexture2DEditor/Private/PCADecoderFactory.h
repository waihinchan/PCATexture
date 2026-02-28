// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "PCADecoderFactory.generated.h"

UCLASS()
class UPCADecoderFactory : public UFactory
{
	GENERATED_BODY()

public:
	UPCADecoderFactory();

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override;
	//~ End UFactory Interface
};
