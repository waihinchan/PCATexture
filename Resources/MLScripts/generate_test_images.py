import torch
import torchvision.transforms.functional as F
import os

os.makedirs('Engine/Plugins/Runtime/PCACompressed/Resources/MLScripts/TestOutput/Inputs', exist_ok=True)

# Generate dummy BaseColor (RGB), Normal (RGB, but only RG used usually), and MRE (RGB, but G is Roughness)
base_color = torch.rand(3, 256, 256)
normal = torch.rand(3, 256, 256)
mre = torch.rand(3, 256, 256)

F.to_pil_image(base_color).save('Engine/Plugins/Runtime/PCACompressed/Resources/MLScripts/TestOutput/Inputs/test_B.png')
F.to_pil_image(normal).save('Engine/Plugins/Runtime/PCACompressed/Resources/MLScripts/TestOutput/Inputs/test_NOR.png')
F.to_pil_image(mre).save('Engine/Plugins/Runtime/PCACompressed/Resources/MLScripts/TestOutput/Inputs/test_MRE.png')
print("Test images generated.")
