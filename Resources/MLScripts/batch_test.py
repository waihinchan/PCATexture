import os
import sys
import re
from collections import defaultdict
import torch
import numpy as np

# Ensure we can import pca_compressor
script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
    sys.path.append(script_dir)

from pca_compressor.data_io import (
    load_tensor_from_image,
    save_tensor_to_image,
    save_params_to_json,
    save_tensor_to_bin,
)
from pca_compressor.models import PCADotDecoder
from pca_compressor.trainer import PCATrainer

DATASET_DIR = (
    r"C:\Users\Administrator\Desktop\optimizeroughness\optimizeroughness\dataset"
)
OUTPUT_DIR = os.path.join(script_dir, "test_output")


def main():
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

    files = [f for f in os.listdir(DATASET_DIR) if f.lower().endswith(".tga")]
    print(f"Total TGA files found: {len(files)}")

    # Group by material prefix
    groups = defaultdict(dict)
    for f in files:
        match = re.search(r"^(.*?)_?(B|MRE|NOR|Nor|nor|mre|b)\.tga$", f, re.IGNORECASE)
        if match:
            prefix = match.group(1)
            # Remove trailing '_B' if it got baked into the prefix for MRE/NOR
            if prefix.upper().endswith("_B"):
                prefix = prefix[:-2]

            suffix = match.group(2).upper()
            groups[prefix][suffix] = f

    # Keep only materials that have BaseColor, Normal, and MRE
    valid_groups = {}
    for prefix, textures in groups.items():
        if "B" in textures and "NOR" in textures and "MRE" in textures:
            valid_groups[prefix] = textures

    print(f"Total valid materials (has B, NOR, MRE): {len(valid_groups)}")

    # Select groups until we reach ~50 materials to test
    target_material_count = 50
    selected_groups = dict(list(valid_groups.items())[:target_material_count])

    print(f"Selected {len(selected_groups)} materials for testing.")

    device = "cuda" if torch.cuda.is_available() else "cpu"
    print(f"Using device: {device}")

    for prefix, textures in selected_groups.items():
        print(f"\nProcessing material: {prefix}")

        # Load the 3 images: BaseColor (RGB), Normal (RG), MRE (Roughness is usually R or G in MRE, let's assume M(R) R(G) E(B) standard Unreal packing)
        # However, you asked to keep BaseColor (RGB), Normal (RG), and Roughness (G channel of MRE usually, but let's just grab the whole MRE for now and extract what we need)

        tensors = []

        # 1. BaseColor (B)
        img_b = os.path.join(DATASET_DIR, textures["B"])
        tensor_b = load_tensor_from_image(
            img_b, size=(512, 512)
        )  # Shape: (1, 3, 512, 512)
        # Ignore Alpha channel for all textures as per your instructions. load_tensor_from_image already converts to RGB (3 channels).
        tensors.append(tensor_b)

        # 2. Normal (NOR) - We usually only need RG or XYZ, assuming XYZ here for standard packing
        img_nor = os.path.join(DATASET_DIR, textures["NOR"])
        tensor_nor = load_tensor_from_image(img_nor, size=(512, 512))
        # Keep just X and Y (first two channels) for Normal map compression to save space
        tensor_nor_rg = tensor_nor[:, :2, :, :]
        tensors.append(tensor_nor_rg)

        # 3. Roughness (from MRE)
        # Assuming MRE packing is: R=Metallic, G=Roughness, B=Emissive or similar.
        # We need to extract just Roughness. If G channel is roughness:
        img_mre = os.path.join(DATASET_DIR, textures["MRE"])
        tensor_mre = load_tensor_from_image(img_mre, size=(512, 512))
        # Let's extract Roughness (assumed to be Green channel, index 1)
        tensor_r = tensor_mre[:, 1:2, :, :]
        tensors.append(tensor_r)

        # Concatenate: BaseColor (3) + Normal (2) + Roughness (1) = 6 channels total target
        target_tensor = torch.cat(tensors, dim=1)
        num_target_channels = target_tensor.shape[1]

        print(f"Target tensor shape (RGB + NorRG + Roughness): {target_tensor.shape}")

        # Model
        model = PCADotDecoder(
            height=512,
            width=512,
            num_target_channels=num_target_channels,
            num_latent_channels=4,  # Compress 6 channels into 4 latent channels
        )

        # Train
        trainer = PCATrainer(model, target_tensor, lr=0.01, device=device)
        print("Starting training...")
        final_loss = trainer.train(max_iters=500, log_interval=100)
        print(f"Finished training. Final Loss: {final_loss:.6f}")

        # Export
        group_out_dir = os.path.join(OUTPUT_DIR, prefix)
        if not os.path.exists(group_out_dir):
            os.makedirs(group_out_dir)

        # Save Latent Map (4 channels -> saved as RGBA PNG)
        latent_map = model.get_latents()
        save_tensor_to_image(
            latent_map, os.path.join(group_out_dir, f"{prefix}_latent.png")
        )
        # Removed bin saving as requested

        # Save Parameters (kept JSON for reference)
        params = model.get_scalar_params()
        save_params_to_json(
            params, os.path.join(group_out_dir, f"{prefix}_params.json")
        )

        # Reconstruct and save each logical channel to verify quality visually
        with torch.no_grad():
            reconstructed = model()

        # Reconstruct BaseColor (channels 0, 1, 2)
        recon_b = reconstructed[:, 0:3, :, :]
        save_tensor_to_image(
            recon_b, os.path.join(group_out_dir, f"recon_{prefix}_B.png")
        )

        # Reconstruct Normal (channels 3, 4) - We pad with B=1.0 for visualization
        recon_nor_rg = reconstructed[:, 3:5, :, :]
        # Create a Z channel (1.0) and concatenate for a valid RGB normal map visualization
        b_channel = torch.ones_like(recon_nor_rg[:, 0:1, :, :])
        recon_nor_rgb = torch.cat([recon_nor_rg, b_channel], dim=1)
        save_tensor_to_image(
            recon_nor_rgb, os.path.join(group_out_dir, f"recon_{prefix}_NOR.png")
        )

        # Reconstruct Roughness (channel 5)
        recon_r = reconstructed[:, 5:6, :, :]
        save_tensor_to_image(
            recon_r, os.path.join(group_out_dir, f"recon_{prefix}_Roughness.png")
        )

        # Copy the originals for easy comparison side-by-side
        save_tensor_to_image(
            tensor_b, os.path.join(group_out_dir, f"orig_{prefix}_B.png")
        )
        save_tensor_to_image(
            tensor_nor_rg, os.path.join(group_out_dir, f"orig_{prefix}_NOR_RG.png")
        )
        save_tensor_to_image(
            tensor_r, os.path.join(group_out_dir, f"orig_{prefix}_Roughness.png")
        )
