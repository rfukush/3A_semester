import numpy as np

class SVMClassifier():

    def __init__(self, lam=0.1, iter_num=100, learning_rate=1e-2):
        self.lam = lam
        self.iter_num = iter_num
        self.learning_rate = learning_rate

    def fit(self, feat_train, label_train):
        self.weight = np.zeros(feat_train.shape[1] + 1, dtype=np.float32)

        for it in range(self.iter_num):
            for i in np.random.permutation(feat_train.shape[0]):
                x = np.concatenate((feat_train[i],np.ones(1, dtype=np.float32)))
                y = label_train[i] * 2 - 1

                
                hinge = np.maximum(0, 1 - y * np.dot(self.weight, x))
                self.weight[:-1] = (1 - self.learning_rate * self.lam) * self.weight[:-1]

                if hinge > 0:
                    self.weight = self.weight + self.learning_rate * y * x

    def predict(self, feat_test):
        return np.dot(np.concatenate((feat_test, np.ones((feat_test.shape[0], 1), dtype=np.float32)), axis=1), self.weight)
