import sys
import numpy as np
import pickle

def softmax(y):
    y = y - np.max(y, axis=1, keepdims=True)
    return np.exp(y) / np.sum(np.exp(y), axis=1, keepdims=True)

def cross_entropy(prob, t):
    return -np.mean(np.log(prob[np.arange(prob.shape[0]), t] + 1e-7))

def softmax_cross_entropy(y, t):
    return cross_entropy(softmax(y), t)

def get_cifar10():
    train_image = np.zeros((50000, 3 * 32 * 32), dtype=np.float32)
    train_label = np.zeros(50000, dtype=np.int32)
    test_image = np.zeros((10000, 3 * 32 * 32), dtype=np.float32)
    test_label = np.zeros(10000, dtype=np.int32)

    for i in range(5):
        if sys.version_info.major == 2:
            data = pickle.load(open('cifar-10-batches-py/data_batch_%d'%(i+1)))
        else:
            data = pickle.load(open('cifar-10-batches-py/data_batch_%d'%(i+1),'rb'),encoding='latin-1')
        train_image[i*10000:(i+1)*10000] = np.asarray(data['data']).astype(np.float32)/255.0 - 0.5
        train_label[i*10000:(i+1)*10000] = np.array(data['labels']).astype(np.int32)

    if sys.version_info.major == 2:
        data = pickle.load(open('cifar-10-batches-py/test_batch'))
    else:
        data = pickle.load(open('cifar-10-batches-py/test_batch','rb'),encoding='latin-1')
    test_image = np.asarray(data['data']).astype(np.float32)/255.0 - 0.5
    test_label = np.array(data['labels']).astype(np.int32)

    train_image = np.reshape(train_image, (-1, 3, 32, 32))
    test_image = np.reshape(test_image, (-1, 3, 32, 32))
    return (train_image, train_label), (test_image, test_label)
