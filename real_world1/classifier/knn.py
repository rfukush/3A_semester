import numpy as np

class KNNClassifier():

    def __init__(self, k = 5):
        self.k = k

    def fit(self, feat_train, label_train):
        self.feat_train = feat_train
        self.label_train = label_train

    def predict(self, feat_test):
        #x = np.broadcast_to(np.sum(self.feat_train**2, axis = 1), (feat_test.shape[0], self.feat_train.shape[0]))
        dist = np.sum(self.feat_train**2, axis = 1, keepdims = True).T - 2 * np.dot(feat_test, self.feat_train.T)
        #print((np.sum(self.feat_train**2, axis = 1 , keepdims = True)).shape)
        #print((np.dot(feat_test, self.feat_train.T)).shape)
        #print(dist.shape)
        #TODO2.3
        """
        dist = np.zeros((feat_test.shape[0], self.feat_train.shape[0]), dtype=np.float32)
        for te in range(feat_test.shape[0]):
            dist[te, :] = np.sum(self.feat_train**2, axis = 1) - 2 * np.dot(feat_test[te], self.feat_train.T)
        """
        ind = np.argsort(dist, axis = 1)[:, :self.k] #TODO2.1
        label_k_nearest = self.label_train[ind] #TODO2.2
        pred = 2 * np.mean(label_k_nearest, axis=1) - 1
        return pred
