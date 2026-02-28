import torch
import torch.nn.functional as F

class PCATrainer:
    """
    Generalized training loop that decouples the optimization algorithm 
    from the data and the model architecture.
    """
    def __init__(self, model, target_tensor, lr=0.01, device=None):
        if device is None:
            self.device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        else:
            self.device = torch.device(device)
            
        self.model = model.to(self.device)
        self.target = target_tensor.to(self.device)
        self.optimizer = torch.optim.Adam(self.model.parameters(), lr=lr)

    def train(self, max_iters=5000, min_loss=0.01, log_interval=100, ue_progress=False):
        """
        Runs the fitting process.
        :param ue_progress: If true, prints progress in a format easily hookable by UE's regex.
        """
        self.model.train()
        best_loss = float('inf')
        
        for i in range(max_iters):
            self.optimizer.zero_grad()
            output = self.model()
            
            # Using L1 Loss as specified in original codebase for perceptual closeness
            loss = F.l1_loss(output, self.target)
            
            loss.backward()
            self.optimizer.step()
            
            current_loss = loss.item()
            best_loss = min(best_loss, current_loss)
            
            # Parameter safety constraints applied after gradient step
            self._apply_constraints()

            if i % log_interval == 0:
                if ue_progress:
                    # Specific format tag: [UE_PROGRESS] Current/Max - Loss: 0.0xx
                    print(f"[UE_PROGRESS] {i}/{max_iters} - Loss: {current_loss:.6f}")
                else:
                    print(f"Iteration {i:04d} | Loss: {current_loss:.6f}")
                    
            if current_loss <= min_loss:
                if ue_progress:
                    print(f"[UE_PROGRESS] {i}/{max_iters} - Loss: {current_loss:.6f} (Target Reached)")
                break
                
        return best_loss

    def _apply_constraints(self):
        """
        Keep parameters within mathematically safe bounds 
        without breaking the computation graph.
        """
        with torch.no_grad():
            if hasattr(self.model, 'inwhite') and hasattr(self.model, 'inblack'):
                # inwhite must always be slightly larger than inblack
                self.model.inwhite.copy_(torch.clamp(self.model.inwhite, min=self.model.inblack.item() + 0.001))
            if hasattr(self.model, 'ingamma'):
                self.model.ingamma.copy_(torch.clamp(self.model.ingamma, min=0.001, max=10.0))
            if hasattr(self.model, 'latent_maps'):
                self.model.latent_maps.copy_(torch.clamp(self.model.latent_maps, min=0.0, max=1.0))
