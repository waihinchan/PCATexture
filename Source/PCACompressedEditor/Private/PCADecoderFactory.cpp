// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCADecoderFactory.h"
#include "PCADecoderAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PCADecoderFactory)

UPCADecoderFactory::UPCADecoderFactory()
{
	SupportedClass = UPCADecoderAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

bool UPCADecoderFactory::ShouldShowInNewMenu() const
{
	return true;
}

UObject* UPCADecoderFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	if (!InClass || !InClass->IsChildOf(UPCADecoderAsset::StaticClass()))
	{
		return nullptr;
	}

	return NewObject<UPCADecoderAsset>(InParent, InClass, InName, Flags);
}
