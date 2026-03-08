// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCADecoderAsset.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(PCADecoderAsset)

UPCADecoderAsset::UPCADecoderAsset()
{
	CustomHLSL = TEXT(
		"// Auto-generated default PCA Decoder HLSL\n"
		"float4 PCA_Latent = Texture2DSample(LatentMap, LatentMapSampler, UV);\n\n"
		"// Reconstruct BaseColor\n"
		"BaseColor.r = max(0.0, dot(PCA_Latent, BaseColor_R) + BaseColor_R_Bias);\n"
		"BaseColor.g = max(0.0, dot(PCA_Latent, BaseColor_G) + BaseColor_G_Bias);\n"
		"BaseColor.b = max(0.0, dot(PCA_Latent, BaseColor_B) + BaseColor_B_Bias);\n"
		"BaseColor = pow(BaseColor,2.2);\n"
		"// Reconstruct Normal\n"
		"float nx = max(0.0, dot(PCA_Latent, Normal_X) + Normal_X_Bias) * 2.0 - 1.0;\n"
		"float ny = max(0.0, dot(PCA_Latent, Normal_Y) + Normal_Y_Bias) * 2.0 - 1.0;\n"
		"Normal.x = nx;\n"
		"Normal.y = ny;\n"
		"Normal.z = sqrt(max(0.001, 1.0 - nx * nx - ny * ny));\n\n"
		"// Reconstruct Roughness\n"
		"Roughness = max(0.0, dot(PCA_Latent, Roughness_R) + Roughness_R_Bias);\n\n"
		"// Default fallbacks for currently uncompressed channels\n"
		"Metallic = 0.0;\n"
		"Emissive = float3(0.0, 0.0, 0.0);\n"
		"AO = max(0.0, dot(PCA_Latent, 0));\n"
	);

	OutputPins.Empty();
	OutputPins.Add({TEXT("BaseColor"), EPCAPinType::Float3});
	OutputPins.Add({TEXT("Normal"), EPCAPinType::Float3});
	OutputPins.Add({TEXT("Metallic"), EPCAPinType::Float1});
	OutputPins.Add({TEXT("Roughness"), EPCAPinType::Float1});
	OutputPins.Add({TEXT("Emissive"), EPCAPinType::Float3});
	OutputPins.Add({TEXT("AO"), EPCAPinType::Float1});
}

void UPCADecoderAsset::PostLoad()
{
	Super::PostLoad();

	// If the asset was created before the PBR pin updates and still has only 1 pin (e.g. "Result"), upgrade it
	if (OutputPins.Num() < 6)
	{
		OutputPins.Empty();
		OutputPins.Add({TEXT("BaseColor"), EPCAPinType::Float3});
		OutputPins.Add({TEXT("Normal"), EPCAPinType::Float3});
		OutputPins.Add({TEXT("Metallic"), EPCAPinType::Float1});
		OutputPins.Add({TEXT("Roughness"), EPCAPinType::Float1});
		OutputPins.Add({TEXT("Emissive"), EPCAPinType::Float3});
		OutputPins.Add({TEXT("AO"), EPCAPinType::Float1});
	}
}
