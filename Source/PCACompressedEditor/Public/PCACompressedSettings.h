#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PCACompressedSettings.generated.h"

UCLASS(config = EditorPerProjectUserSettings, defaultconfig, meta = (DisplayName = "PCA Compressed Texture"))
class PCACOMPRESSEDEDITOR_API UPCACompressedSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPCACompressedSettings()
	{
		PythonExecutablePath.FilePath = TEXT("python");
		TrainingIterations = 500;
	}

	/** 
	 * The absolute path to the Python executable used for PCA compression. 
	 * If left as 'python', it will use the system's default Python environment.
	 * To use the plugin's venv, point this to the venv's python.exe (e.g., C:/.../venv/Scripts/python.exe).
	 */
	UPROPERTY(config, EditAnywhere, Category = "Python Environment", meta = (FilePathFilter = "exe", RelativeToGameDir))
	FFilePath PythonExecutablePath;

	/** Number of iterations for the PCA model training. Higher means better quality but takes longer. */
	UPROPERTY(config, EditAnywhere, Category = "Training Settings", meta = (ClampMin = "1", ClampMax = "10000"))
	int32 TrainingIterations;

	// Helper function to get the actual executable string
	FString GetPythonExecutable() const
	{
		return PythonExecutablePath.FilePath.IsEmpty() ? TEXT("python") : PythonExecutablePath.FilePath;
	}
};
