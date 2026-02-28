// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCACompressedTextureFactory.h"
#include "PCACompressedTextureAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PCACompressedTextureFactory)

UPCACompressedTextureFactory::UPCACompressedTextureFactory()
{
	SupportedClass = UPCACompressedTextureAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

bool UPCACompressedTextureFactory::ShouldShowInNewMenu() const
{
	return true;
}

UObject* UPCACompressedTextureFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	if (!InClass || !InClass->IsChildOf(UPCACompressedTextureAsset::StaticClass()))
	{
		return nullptr;
	}

	return NewObject<UPCACompressedTextureAsset>(InParent, InClass, InName, Flags);
}
