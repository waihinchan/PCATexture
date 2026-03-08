# PCA Texture Compression Plugin for Unreal Engine 5

This Unreal Engine 5 plugin provides an end-to-end pipeline for compressing multiple PBR texture channels (BaseColor, Normal, Roughness, etc.) into a highly optimized latent space representation using Principal Component Analysis (PCA) and Neural Network optimization via PyTorch.

## Features
- **Editor Integration**: Select multiple textures in the Content Browser to directly generate a `PCA Compressed Texture Asset`.
- **Machine Learning Compression**: Automates a PyTorch backend to decompose PBR textures, preserving high-frequency details (using SVD) and refining the latent representation.
- **Material Node**: Custom Material Expression (`PCA Compressed Texture Sample`) that dynamically generates shader code (HLSL) for reconstructing the textures.
- **PCA Material Configurator**: A Nomad Editor Tab to easily apply generated PCA parameters and texture maps to your Material Instances, featuring Auto-Sync capabilities.

## Requirements
- Unreal Engine 5
- Python Environment with PyTorch (`torch`, `torchvision`, `numpy`, `Pillow`)

## Usage
1. Select your target textures in the UE Content Browser.
2. Right-click and choose **Create PCA Compressed Texture**.
3. Wait for the Python ML pipeline to finish optimization.
4. Open your Material and place a `PCA Compressed Texture Sample` node. Assign the newly created PCA Asset.
5. Click **Generate Parameter Nodes** to automatically wire the inputs.
6. Open the **PCA Material Configurator** to sync PCA assets dynamically to material instances.

## Architecture
- **C++**: Editor extensions, asset factories, custom material nodes, HLSL generation, UI configurator.
- **Python**: SVD initialization, PyTorch neural network training, quantization-aware training.
