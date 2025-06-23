from pearl.agent import RLAgent
import torch

class TorchDQN(RLAgent):
    """
    Simple DQN agent using PyTorch.
    """
    def __init__(self, model_path: str, module, device):
        self.m_agent = module.to(device)
        self.m_agent.load_state_dict(torch.load(model_path, map_location=device))
        self.m_agent.eval()
        self.q_net = self.m_agent.net

    def predict(self, observation):
        self.q_net.eval()

        with torch.no_grad():
            q_vals = self.m_agent(observation)
            action = q_vals.argmax(1).item()
            return action

    def get_q_net(self):
        return self.q_net