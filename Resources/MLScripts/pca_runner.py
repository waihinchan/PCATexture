import argparse
import sys
import os
import json
import torch

script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
    sys.path.append(script_dir)

from pca_compressor.data_io import (
    load_tensor_from_image,
    save_tensor_to_image,
    save_params_to_json,
)
from pca_compressor.models import PCADotDecoder
from pca_compressor.trainer import PCATrainer


def main():
    parser = argparse.ArgumentParser(description="UE PCA Texture Compressor")
    parser.add_argument("--out_dir", type=str, required=True, help="Output directory")
    parser.add_argument(
        "--inputs", type=str, nargs="+", required=True, help="Input texture paths"
    )
    parser.add_argument("--name", type=str, required=True, help="Base name for outputs")
    parser.add_argument(
        "--iters", type=int, default=500, help="Number of training iterations"
    )

    args = parser.parse_args()

    if not os.path.exists(args.out_dir):
        os.makedirs(args.out_dir)

    print(f"[UE_PROGRESS] 0/{args.iters} - Loading textures...")

    tensors = []

    channel_names = []

    # Simple heuristic to extract channels based on naming
    for img_path in args.inputs:
        filename = os.path.basename(img_path).upper()
        tensor = load_tensor_from_image(img_path)  # native size

        if "_B." in filename:
            # BaseColor -> Keep RGB
            tensors.append(tensor[:, :3, :, :])
            channel_names.extend(["BaseColor_R", "BaseColor_G", "BaseColor_B"])
        elif "_NOR." in filename:
            # Normal -> Keep RG
            tensors.append(tensor[:, :2, :, :])
            channel_names.extend(["Normal_X", "Normal_Y"])
        elif "_MRE." in filename:
            # MRE -> Keep Roughness (Green channel)
            tensors.append(tensor[:, 1:2, :, :])
            channel_names.extend(["Roughness_R"])
        else:
            # Unknown -> Keep RGB just in case
            tensors.append(tensor[:, :3, :, :])
            channel_names.extend(
                [
                    f"Unknown_{len(channel_names)}_R",
                    f"Unknown_{len(channel_names)}_G",
                    f"Unknown_{len(channel_names)}_B",
                ]
            )

    target_tensor = torch.cat(tensors, dim=1)
    num_target_channels = target_tensor.shape[1]

    print(
        f"[UE_PROGRESS] 10/{args.iters} - Target tensor built with {num_target_channels} channels. Starting training..."
    )

    device = "cuda" if torch.cuda.is_available() else "cpu"

    # -------------------------------------------------------------
    # PERFORM SVD/PCA INITIALIZATION TO PRESERVE HIGH-FREQUENCIES
    # -------------------------------------------------------------
    B, C, H, W = target_tensor.shape
    # Flatten spatial dimensions: shape (C, H*W)
    flat_target = target_tensor.view(C, -1)

    # Mean centering
    mean_val = flat_target.mean(dim=1, keepdim=True)
    centered_target = flat_target - mean_val

    # Compute SVD: centered_target = U * S * V^T
    # U: (C, C), S: (C,), V: (H*W, C)
    U, S, Vh = torch.linalg.svd(centered_target, full_matrices=False)

    num_latent_channels = 4

    # Take top components
    U_k = U[:, :num_latent_channels]  # (C, 4)
    S_k = S[:num_latent_channels]  # (4,)
    Vh_k = Vh[:num_latent_channels, :]  # (4, H*W)

    # Raw latent features
    # shape: (4, H*W)
    raw_latents = Vh_k

    # We must normalize latents to [0, 1] range to be stored as 8-bit texture
    # Min/Max per channel
    latent_min = raw_latents.min(dim=1, keepdim=True)[0]
    latent_max = raw_latents.max(dim=1, keepdim=True)[0]
    latent_range = torch.clamp(latent_max - latent_min, min=1e-5)

    # Normalized latents [0, 1]
    norm_latents = (raw_latents - latent_min) / latent_range
    init_latents = norm_latents.view(1, num_latent_channels, H, W)

    # Calculate the corresponding weights to perfectly reverse the normalization
    # Reconstruction: U_k * S_k * raw_latents + mean
    # = U_k * S_k * (norm_latents * latent_range + latent_min) + mean
    # = (U_k * S_k * latent_range) * norm_latents + (mean + U_k * S_k * latent_min)

    # Weights: shape (C, 4)
    init_weights = U_k * S_k.unsqueeze(0) * latent_range.t()

    # Bias: shape (C,)
    init_biases = mean_val.squeeze(1) + torch.matmul(
        U_k * S_k.unsqueeze(0), latent_min
    ).squeeze(1)

    model = PCADotDecoder(
        height=H,
        width=W,
        num_target_channels=num_target_channels,
        num_latent_channels=num_latent_channels,
        channel_names=channel_names,
        init_latents=init_latents,
        init_weights=init_weights,
        init_biases=init_biases,
    )

    trainer = PCATrainer(model, target_tensor, lr=0.001, device=device)
    log_interval = max(1, args.iters // 10)
    # Set min_loss to extremely low so it is forced to do neural network fine-tuning for BC compression resistance
    final_loss = trainer.train(
        max_iters=args.iters,
        min_loss=0.0001,
        log_interval=log_interval,
        ue_progress=True,
    )

    # Save Latent
    latent_path = os.path.join(args.out_dir, f"{args.name}_latent.png")
    save_tensor_to_image(model.get_latents(), latent_path)

    # Save Params
    params_path = os.path.join(args.out_dir, f"{args.name}_params.json")
    save_params_to_json(model.get_scalar_params(), params_path)

    # Calculate and Save Reconstructed Images for Visual Inspection
    model.eval()
    with torch.no_grad():
        reconstructed = model()  # Forward pass

        current_idx = 0
        for channel_name in channel_names:
            if "BaseColor_R" in channel_name:
                # Save next 3 channels as BaseColor
                if current_idx + 3 <= reconstructed.shape[1]:
                    bc_tensor = reconstructed[:, current_idx : current_idx + 3, :, :]
                    save_tensor_to_image(
                        bc_tensor,
                        os.path.join(args.out_dir, f"{args.name}_Recon_BaseColor.png"),
                    )
                current_idx += 3
            elif "Normal_X" in channel_name:
                # Save next 2 channels as Normal (padded with Blue)
                if current_idx + 2 <= reconstructed.shape[1]:
                    nor_tensor = reconstructed[:, current_idx : current_idx + 2, :, :]
                    # Compute Z channel to make it a proper normal map for visualization
                    x = (
                        nor_tensor[:, 0:1, :, :] * 2.0 - 1.0
                    )  # Assuming normals are 0-1, actually our target is 0-1.
                    # Wait, our target normal is directly from image 0-1 or -1 to 1?
                    # Real dataset images are usually 0-1 mapped.
                    # Let's just save RG channels directly padded to RGB (data_io handles 2 channels by padding with 0s)
                    # We will reconstruct Z manually to make it look blueish like a real normal map
                    z = torch.sqrt(
                        torch.clamp(
                            1.0
                            - (nor_tensor[:, 0:1, :, :] * 2 - 1) ** 2
                            - (nor_tensor[:, 1:2, :, :] * 2 - 1) ** 2,
                            min=0.001,
                        )
                    )
                    z = z * 0.5 + 0.5  # Map back to 0-1
                    nor_rgb = torch.cat([nor_tensor, z], dim=1)
                    save_tensor_to_image(
                        nor_rgb,
                        os.path.join(args.out_dir, f"{args.name}_Recon_Normal.png"),
                    )
                current_idx += 2
            elif "Roughness_R" in channel_name:
                # Save next 1 channel as Roughness
                if current_idx + 1 <= reconstructed.shape[1]:
                    r_tensor = reconstructed[:, current_idx : current_idx + 1, :, :]
                    save_tensor_to_image(
                        r_tensor,
                        os.path.join(args.out_dir, f"{args.name}_Recon_Roughness.png"),
                    )
                current_idx += 1
            elif channel_name in ["BaseColor_G", "BaseColor_B", "Normal_Y"]:
                pass  # Already handled by the X/R channel triggers
            else:
                current_idx += 1

    print(f"[UE_PROGRESS] {args.iters}/{args.iters} - Done. Loss: {final_loss:.6f}")


if __name__ == "__main__":
    main()
