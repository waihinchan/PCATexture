import torch

def lerp(invalue, A, B):
    """Linear interpolation"""
    return A + invalue * (B - A)

def smoothstep(x, edge0, edge1):
    """HLSL smoothstep equivalent"""
    t = torch.clamp((x - edge0) / (edge1 - edge0 + 1e-6), 0.0, 1.0)
    return t * t * (3 - 2 * t)

def pslevel(invalue, inblack, inwhite, ingamma, outblack, outwhite, doClamp=True):
    """
    Simulates Photoshop's Levels adjustment.
    Engineered with epsilon values to prevent zero-division and negative power NaNs during backprop.
    """
    # 1. Subtract inblack
    result = torch.subtract(invalue, inblack)
    
    # 2. Divide by (inwhite - inblack) securely
    safe_range = torch.clamp(torch.subtract(inwhite, inblack), min=1e-5)
    result = torch.divide(result, safe_range)
    
    # 3. Apply Gamma. Clamp to prevent nan when base < 0 and exponent is fractional.
    result = torch.clamp(result, min=0.0)
    safe_gamma = torch.clamp(ingamma, min=1e-3)
    result = torch.pow(result, safe_gamma)
    
    # 4. Map to output range
    result = torch.mul(result, torch.subtract(outwhite, outblack))
    result = torch.add(result, outblack)
    
    if doClamp:
        result = torch.clamp(result, 0.0, 1.0)
        
    return result
