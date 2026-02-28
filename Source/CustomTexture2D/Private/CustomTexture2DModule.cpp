// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomTexture2DModule.h"

#define LOCTEXT_NAMESPACE "FCustomTexture2DModule"

void FCustomTexture2DModule::StartupModule()
{
	// This code will execute after your module is loaded into memory
}

void FCustomTexture2DModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCustomTexture2DModule, CustomTexture2D)
