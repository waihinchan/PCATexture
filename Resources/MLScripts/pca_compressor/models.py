import torch
import torch.nn as nn


class BaseDecoder(nn.Module):
    def __init__(self, num_latent_channels, height, width, init_latents=None):
        super().__init__()
        if init_latents is not None:
            self.latent_maps = nn.Parameter(init_latents.clone())
        else:
            self.latent_maps = nn.Parameter(
                torch.rand(1, num_latent_channels, height, width) * 0.5 + 0.25
            )

    def get_latents(self):
        return torch.clamp(self.latent_maps, 0.0, 1.0)


class PCADotDecoder(BaseDecoder):
    """
    A highly generalized, multi-channel dot-product decoder.
    """

    def __init__(
        self,
        height,
        width,
        num_target_channels=7,
        num_latent_channels=4,
        channel_names=None,
        init_latents=None,
        init_weights=None,
        init_biases=None,
    ):
        super().__init__(
            num_latent_channels=num_latent_channels,
            height=height,
            width=width,
            init_latents=init_latents,
        )

        self.num_target_channels = num_target_channels
        self.num_latent_channels = num_latent_channels
        self.channel_names = channel_names

        if init_weights is not None:
            self.channel_weights = nn.Parameter(
                init_weights.clone().view(
                    num_target_channels, num_latent_channels, 1, 1
                )
            )
        else:
            self.channel_weights = nn.Parameter(
                torch.randn(num_target_channels, num_latent_channels, 1, 1) * 0.1 + 0.5
            )

        if init_biases is not None:
            self.channel_biases = nn.Parameter(
                init_biases.clone().view(num_target_channels)
            )
        else:
            self.channel_biases = nn.Parameter(torch.zeros(num_target_channels))

    def forward(self):
        latents = self.get_latents()

        latents_8bit_exact = torch.round(latents * 255.0) / 255.0
        latents_qat = (latents_8bit_exact - latents).detach() + latents

        result = torch.nn.functional.conv2d(
            latents_qat, self.channel_weights, bias=self.channel_biases
        )

        result = torch.clamp(result, 0.0, 1.0)

        return result

    def get_scalar_params(self):
        weights = self.channel_weights.detach().cpu().squeeze().numpy()
        if weights.ndim == 1:
            weights = weights[None, :]

        params_dict = {}

        if self.channel_names and len(self.channel_names) == self.num_target_channels:
            names_to_use = self.channel_names
        else:
            names_to_use = [
                "BaseColor_R",
                "BaseColor_G",
                "BaseColor_B",
                "Normal_X",
                "Normal_Y",
                "Roughness_R",
                "AO",
            ]

        for i in range(self.num_target_channels):
            name = names_to_use[i] if i < len(names_to_use) else f"TargetChannel_{i}"
            vector4 = weights[i].tolist()

            while len(vector4) < 4:
                vector4.append(0.0)

            params_dict[name] = vector4
            bias_val = float(self.channel_biases[i].item())
            params_dict[f"{name}_Bias"] = bias_val

        return params_dict
