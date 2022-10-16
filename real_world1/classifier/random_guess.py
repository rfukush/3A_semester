import numpy as np

class RandomGuess():
    
    def __init__(self):
        pass

    def fit(self, feat, label):
        pass

    def predict(self, feat):
        ndata = feat.shape[0]
        return np.random.rand(ndata).astype(np.float32)
        
