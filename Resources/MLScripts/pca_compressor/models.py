import torch
import torch.nn as nn
from . import shader_ops

class BaseDecoder(nn.Module):
    def __init__(self, num_latent_channels, height, width):
        super().__init__()
        # Direct generation of latent texture feature maps instead of filtering from original image.
        # This is a huge optimization over the demo script, acting as a true Neural Encoder bottleneck.
        self.latent_maps = nn.Parameter(torch.rand(1, num_latent_channels, height, width) * 0.5 + 0.25)

    def get_latents(self):
        return torch.clamp(self.latent_maps, 0.0, 1.0)


class DoubleGreyDecoder(BaseDecoder):
    """
    Refactored version of albedoSimulator's forward pass.
    Instead of calculating two grey maps from the source via linear layer,
    it directly optimizes 2 latent grey channels and reconstructs using mulA/mulB.
    """
    def __init__(self, height, width):
        super().__init__(num_latent_channels=2, height=height, width=width)
        # Vector parameters representing dominant colors (mulA and mulB)
        self.mulA = nn.Parameter(torch.tensor([1.0, 1.0, 1.0]))
        self.mulB = nn.Parameter(torch.tensor([0.5, 0.5, 0.5]))

    def forward(self):
        latents = self.get_latents()
        grey = latents[:, 0:1, :, :]
        grey_extra = latents[:, 1:2, :, :]

        # Broadcast vector params to image dimensions: (1, 3, 1, 1)
        mA = self.mulA.view(1, 3, 1, 1)
        mB = self.mulB.view(1, 3, 1, 1)

        result = grey * mA + grey_extra * mB
        return torch.clamp(result, 0.0, 1.0)

    def get_scalar_params(self):
        return {
            "mulA": self.mulA.detach().cpu().tolist(),
            "mulB": self.mulB.detach().cpu().tolist()
        }


class RoughnessLevelDecoder(BaseDecoder):
    """
    Refactored version of roughness.py's genrateLevelsParamForRoughness.
    Optimizes a single channel latent map alongside Photoshop level parameters.
    """
    def __init__(self, height, width):
        super().__init__(num_latent_channels=1, height=height, width=width)
        
        # Level Parameters
        self.inblack = nn.Parameter(torch.tensor([0.0]))
        self.inwhite = nn.Parameter(torch.tensor([1.0]))
        self.ingamma = nn.Parameter(torch.tensor([1.0]))
        self.outblack = nn.Parameter(torch.tensor([0.0]))
        self.outwhite = nn.Parameter(torch.tensor([1.0]))

    def forward(self):
        latents = self.get_latents()
        
        result = shader_ops.pslevel(
            latents,
            self.inblack, self.inwhite, self.ingamma,
            self.outblack, self.outwhite
        )
        return result

    def get_scalar_params(self):
        return {
            "inblack": self.inblack.detach().cpu().tolist(),
            "inwhite": self.inwhite.detach().cpu().tolist(),
            "ingamma": self.ingamma.detach().cpu().tolist(),
            "outblack": self.outblack.detach().cpu().tolist(),
            "outwhite": self.outwhite.detach().cpu().tolist()
        }

class PCADotDecoder(BaseDecoder):
    """
    A highly generalized, multi-channel dot-product decoder.
    
    Architecture:
    - Inputs: A 4-channel Latent Texture (RGBA).
    - Params: N distinct float4 vectors (where N is the number of target output channels).
    - Forward: For each target channel `i`, Output_i = max(0, dot(Latent_RGBA, Param_i_float4))
    
    This effectively acts as a 1x1 Convolution mapping 4 latent channels to N output channels, 
    followed by a ReLU activation.
    """
    def __init__(self, height, width, num_target_channels=7, num_latent_channels=4):
        super().__init__(num_latent_channels=num_latent_channels, height=height, width=width)
        
        self.num_target_channels = num_target_channels
        self.num_latent_channels = num_latent_channels
        
        # We declare a single weight matrix of shape (num_target_channels, num_latent_channels).
        # For a 7-channel output (BaseColor(3) + NormalRG(2) + Roughness(1) + AO(1)) 
        # and 4 latent channels, this is a 7x4 matrix.
        # This is mathematically identical to 7 individual float4 parameters, but heavily 
        # optimized for PyTorch's Conv2d/MatMul tensor operations.
        # Initialize with random values (e.g., Xavier init approximation)
        self.channel_weights = nn.Parameter(
            torch.randn(num_target_channels, num_latent_channels, 1, 1) * 0.1 + 0.5
        )

    def forward(self):
        latents = self.get_latents()  # Shape: (1, 4, H, W)
        
        # In PyTorch, a 1x1 convolution is exactly the dot product over the channel dimension.
        # output_channel[i] = sum(latents[j] * weight[i, j]) = dot(latents_rgba, weight_float4_i)
        result = torch.nn.functional.conv2d(latents, self.channel_weights)
        
        # Apply the max(0, x) which is a standard ReLU activation
        result = torch.relu(result)
        
        # Optional: clamp to 1.0 to ensure strict valid PBR ranges
        result = torch.clamp(result, 0.0, 1.0)
        
        return result

    def get_scalar_params(self):
        """
        Exports the learned weights as individual float4 vectors 
        ready to be plugged into UE's FPCAScalarParam.
        """
        weights = self.channel_weights.detach().cpu().squeeze().numpy() # Shape: (N, 4)
        
        params_dict = {}
        # Naming convention matches the typical PBR packing order
        channel_names = ["BaseColor_R", "BaseColor_G", "BaseColor_B", "Normal_X", "Normal_Y", "Roughness", "AO"]
        
        for i in range(self.num_target_channels):
            # If the user requested more channels than the standard 7, fallback to generic names
            name = channel_names[i] if i < len(channel_names) else f"TargetChannel_{i}"
            
            # Convert the (4,) numpy array row to a list [x, y, z, w]
            vector4 = weights[i].tolist()
            
            # If latent channels < 4, pad with 0s to make it a valid float4 for UE
            while len(vector4) < 4:
                vector4.append(0.0)
                
            params_dict[name] = vector4
            
        return params_dict
