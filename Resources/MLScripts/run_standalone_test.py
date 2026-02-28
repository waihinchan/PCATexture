import os
import torch
from pca_compressor.models import DoubleGreyDecoder
from pca_compressor.trainer import PCATrainer
from pca_compressor.data_io import save_tensor_to_bin, save_params_to_json, save_tensor_to_image

def main():
    print("--- Starting Standalone PCA Compressor Test ---")
    device = 'cuda' if torch.cuda.is_available() else 'cpu'
    print(f"Using device: {device}")

    # 1. Create a dummy target tensor to act as our "Albedo" map
    # A simple gradient patch to prove the network learns
    H, W = 128, 128
    y = torch.linspace(0, 1, H).view(-1, 1).repeat(1, W)
    x = torch.linspace(0, 1, W).view(1, -1).repeat(H, 1)
    
    # Target image is mostly red and green gradients
    target = torch.stack([x, y, x*y], dim=0).unsqueeze(0).to(device)
    
    # Optional: save target to see what we are trying to fit
    save_tensor_to_image(target, "target_dummy.png")
    
    # 2. Initialize our specialized DoubleGreyDecoder 
    # (Extracts 2 compressed grey maps and tries to reconstruct using mulA, mulB)
    model = DoubleGreyDecoder(height=H, width=W)
    
    # 3. Initialize Trainer
    trainer = PCATrainer(model, target, lr=0.05, device=device)
    
    # 4. Run Optimization
    print("Beginning optimization...")
    final_loss = trainer.train(max_iters=500, min_loss=0.005, log_interval=50, ue_progress=True)
    print(f"Optimization finished. Final Loss: {final_loss:.6f}")
    
    # 5. Export results
    # Dump the learned latents as binary for UE
    latents = model.get_latents()
    save_tensor_to_bin(latents, "Optimized_Latents.bin")
    
    # Dump the learned vectors to json for UE
    params = model.get_scalar_params()
    save_params_to_json(params, "Optimized_Params.json")
    
    # Save the reconstructed image to prove it works visually
    reconstructed = model()
    save_tensor_to_image(reconstructed, "reconstructed_dummy.png")
    
    print("[UE_TRAIN_SUCCESS]")
    print("Exported Optimized_Latents.bin and Optimized_Params.json")

if __name__ == "__main__":
    main()
