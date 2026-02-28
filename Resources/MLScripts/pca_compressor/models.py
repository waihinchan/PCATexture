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
