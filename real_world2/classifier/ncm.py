import numpy as np

class NCMClassifier():

    def __init__(self):
        pass

    def fit(self, feat_train, label_train):
        pos_mean = np.mean(feat_train[np.where(label_train == 1)[0], :], axis=0)
        neg_mean = np.mean(feat_train[np.where(label_train == 0)[0], :], axis=0)

        # another solution: 
        # pos_mean = np.mean(feat_train[label_train == 1], axis=0)
        # neg_mean = np.mean(feat_train[label_train == 0], axis=0)
        
        self.weight = 2 * (pos_mean - neg_mean)
        self.bias = np.sum(neg_mean ** 2) - np.sum(pos_mean ** 2)

    def predict(self, feat_test):
        pred = np.dot(feat_test, self.weight) + self.bias
        return pred
