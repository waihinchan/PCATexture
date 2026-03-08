# Changelog

## [1.1.0] - Recent Updates

### Added
- **PCA Material Configurator**: Native C++ Editor UI panel (`PCAMaterialConfiguratorTab`) to streamline mapping PCA textures and scalar/vector parameters to Material Instances.
- **Auto-Sync functionality**: Enabling artists to instantly swap PCA assets on Material Instances without clicking an "Apply" button.
- **Parameter Prefixing**: Support for parameter prefixes when generating properties to accommodate Master Materials with grouped variables.

### Fixed / Improved (C++)
- **Sampler Type Issue**: Fixed texture sampler parameter generation. Latent maps are now properly sampled as `LinearColor` (`SAMPLERTYPE_LinearColor`) preventing `sRGB` conversion from corrupting numeric data.
- **Texture Compression Profile**: Generated latent textures now default to `TC_Default` with `sRGB` disabled, automatically utilizing robust BC1/BC3 block compression.
- **Missing Vector Alpha**: Fixed material node generator discarding the Alpha channel (4th PCA feature dimension) from `VectorParameter`. `RGBA` outputs are now correctly linked.
- **HLSL Normal Reconstruction**: Corrected math in the generated PCA Decoder HLSL string. Texture features `(0.0-1.0)` are now safely expanded to `(-1.0-1.0)` vectors before calculating the Z channel, fixing normal rendering.
- **Bias Mapping**: Fixed dimensionality mismatches where a Scalar Bias was mistakenly converted to a Vector parameter, leading to HLSL type mismatch errors during dot-product computations.

### Fixed / Improved (Python ML)
- **SVD Alignment & Learning Rate**: Tuned Adam optimizer learning rate down to `0.001` to respect high-quality SVD initializations rather than aggressively discarding them.
- **Loss Function Tuning**: Replaced `L1Loss` with `MSELoss` (Mean Squared Error), mathematically aligning the neural network fine-tuning goal with the Principal Component Analysis (SVD) target to preserve high-frequencies.
- **Clamping Logic**: Removed redundant `relu` activation before `.clamp(0.0, 1.0)`, smoothing gradient back-propagation during QAT (Quantization Aware Training).
