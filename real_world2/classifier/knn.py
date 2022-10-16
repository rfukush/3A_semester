import numpy as np

class KNNClassifier():

    def __init__(self, k = 5):
        self.k = k

    def fit(self, feat_train, label_train):
        self.feat_train = feat_train
        self.label_train = label_train

    def predict(self, feat_test):
        #dist = np.zeros((feat_test.shape[0], self.feat_train.shape[0]), dtype=np.float32)
        # for tr in range(self.feat_train.shape[0]):
        #     for te in range(feat_test.shape[0]):
        #         dist[te,tr] = (np.sum(self.feat_train[tr] ** 2) + np.sum(feat_test[te] ** 2) - 2 * np.dot(self.feat_train[tr],feat_test[te]))**0.5
        # solution for TODO2.3
        # for te in range(feat_test.shape[0]):
        #     dist[te,:] = np.sum(self.feat_train ** 2, axis=1, keepdims=True).T - 2 * np.dot(feat_test[te], self.feat_train.T)
        # another solution for TODO2.3
        #     dist[te, :] = np.sum(self.feat_train ** 2, axis=1) - 2 * np.dot(self.feat_train, feat_test[te])
        dist = np.sum(self.feat_train ** 2, axis=1, keepdims=True).T - 2 * np.dot(feat_test, self.feat_train.T)
        ind = np.argsort(dist)[:, :self.k]
        # another solution for TODO2.1
        # ind = np.argpartition(dist, kth=self.k, axis=1)[:, :self.k]
        label_k_nearest = self.label_train[ind]
        pred = 2 * np.mean(label_k_nearest, axis=1) - 1
        return pred
