// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCACompressedModule.h"

#define LOCTEXT_NAMESPACE "FPCACompressedModule"

void FPCACompressedModule::StartupModule()
{
	// This code will execute after your module is loaded into memory
}

void FPCACompressedModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPCACompressedModule, PCACompressed)
