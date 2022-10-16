import numpy as np

class NCMClassifier():

    def __init__(self):
        pass

    def fit(self, feat_train, label_train):
        #print(feat_train[label_train == 0, :].shape)
        #print(label_train.shape)
        pos_mean = np.mean(feat_train[label_train == 1, :], axis = 0) #TODO3.1
        neg_mean = np.mean(feat_train[label_train == 0, :], axis = 0) #TODO3.2
        #print(pos_mean.shape)
        #print(neg_mean.shape)
        self.weight = 2 * (pos_mean - neg_mean) #TODO3.3
        #print(self.weight.shape)
        self.bias = -np.sum(pos_mean**2) + np.sum(neg_mean**2) #TODO3.4
        #print(self.bias.shape)

    def predict(self, feat_test):
        pred = np.dot(feat_test, self.weight) + self.bias
        return pred
