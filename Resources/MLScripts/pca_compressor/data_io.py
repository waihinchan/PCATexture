import torch
import numpy as np
import json
from PIL import Image


def load_tensor_from_bin(filepath, shape, dtype=np.float32):
    """Loads a flat binary dump from UE into a PyTorch tensor."""
    data = np.fromfile(filepath, dtype=dtype)
    tensor = torch.from_numpy(data.copy()).view(*shape)
    return tensor


def save_tensor_to_bin(tensor, filepath):
    """Saves the latent maps to a raw binary file for UE to import."""
    array = tensor.detach().cpu().numpy().astype(np.float32)
    array.tofile(filepath)


def save_params_to_json(params_dict, filepath):
    """Saves the scalar/vector parameters to JSON for UE Asset metadata."""
    with open(filepath, "w") as f:
        json.dump(params_dict, f, indent=4)


def load_tensor_from_image(filepath, size=None):
    """Helper for testing without UE: Loads an image directly into PyTorch."""
    img = Image.open(filepath).convert("RGB")
    if size:
        img = img.resize(size, Image.Resampling.BILINEAR)
    img_np = np.array(img).astype(np.float32) / 255.0
    # Convert HWC to CHW
    tensor = torch.from_numpy(img_np).permute(2, 0, 1).unsqueeze(0)
    return tensor


def save_tensor_to_image(tensor, filepath):
    """Helper for testing: Saves a CHW tensor as an image."""
    tensor = torch.clamp(tensor.squeeze(0), 0.0, 1.0)
    img_np = (tensor.permute(1, 2, 0).detach().cpu().numpy() * 255).astype(np.uint8)
    if img_np.shape[2] == 1:
        img_np = img_np.squeeze(2)
        img = Image.fromarray(img_np, mode="L")
    elif img_np.shape[2] == 2:
        # Padded to 3 channels for RGB saving
        padded = np.zeros((img_np.shape[0], img_np.shape[1], 3), dtype=np.uint8)
        padded[:, :, :2] = img_np
        img = Image.fromarray(padded, mode="RGB")
    elif img_np.shape[2] == 4:
        img = Image.fromarray(img_np, mode="RGBA")
    else:
        # If >4 or ==3, just grab the first 3 channels for visualization
        img = Image.fromarray(img_np[:, :, :3], mode="RGB")
    img.save(filepath)
