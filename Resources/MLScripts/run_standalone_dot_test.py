import os
import torch
from pca_compressor.models import PCADotDecoder
from pca_compressor.trainer import PCATrainer
from pca_compressor.data_io import save_tensor_to_bin, save_params_to_json

def main():
    print("--- Starting PCADotDecoder Standalone Test ---")
    device = 'cuda' if torch.cuda.is_available() else 'cpu'
    print(f"Using device: {device}")

    # 1. Create a dummy target tensor to act as our 7-channel "PBR" maps
    # BaseColor(3) + Normal(2) + Roughness(1) + AO(1) = 7 channels
    H, W = 64, 64
    num_targets = 7
    # Just random noise as target for stress testing the dot product fitter
    target = torch.rand(1, num_targets, H, W).to(device)
    
    # 2. Initialize our PCADotDecoder 
    # (Extracts 4 latent channels and reconstructs 7 channels via max(0, dot))
    model = PCADotDecoder(height=H, width=W, num_target_channels=num_targets, num_latent_channels=4)
    
    # 3. Initialize Trainer
    trainer = PCATrainer(model, target, lr=0.05, device=device)
    
    # 4. Run Optimization
    print("Beginning optimization for Dot Product Decoder...")
    final_loss = trainer.train(max_iters=1000, min_loss=0.01, log_interval=100, ue_progress=True)
    print(f"Optimization finished. Final Loss: {final_loss:.6f}")
    
    # 5. Export results
    latents = model.get_latents()
    save_tensor_to_bin(latents, "Optimized_Latents_Dot.bin")
    
    params = model.get_scalar_params()
    save_params_to_json(params, "Optimized_Params_Dot.json")
    
    print("[UE_TRAIN_SUCCESS]")
    print(f"Exported latent tensor shape: {latents.shape}")
    print("Exported params:", list(params.keys()))

if __name__ == "__main__":
    main()
