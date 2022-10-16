from __future__ import print_function
from layer import *
import util
import sys
import time
import random
import numpy as np
import sys

def main(arch, epoch):
    if arch == 'linear':
        model = Sequential()
        model.addlayer(FlattenLayer())
        model.addlayer(LinearLayer(3072, 784))
        model.addlayer(ReLULayer())
        model.addlayer(LinearLayer(784, 784))
        model.addlayer(ReLULayer())
        model.addlayer(LinearLayer(784, 10))
        classifier = Classifier(model)
    elif arch == 'cnn':
        model = Sequential()
        model.addlayer(Convsize3Layer(3, 32))
        model.addlayer(ReLULayer())
        model.addlayer(Convsize3Layer(32, 32))
        model.addlayer(ReLULayer())
        model.addlayer(MaxPoolingLayer(2))
        model.addlayer(Convsize3Layer(32, 64))
        model.addlayer(ReLULayer())
        model.addlayer(Convsize3Layer(64, 64))
        model.addlayer(ReLULayer())
        model.addlayer(MaxPoolingLayer(2))
        model.addlayer(Convsize3Layer(64, 128))
        model.addlayer(ReLULayer())
        model.addlayer(Convsize3Layer(128, 128))
        model.addlayer(ReLULayer())
        model.addlayer(MaxPoolingLayer(2))
        model.addlayer(Convsize3Layer(128, 256))
        model.addlayer(ReLULayer())
        model.addlayer(Convsize3Layer(256, 256))
        model.addlayer(ReLULayer())
        model.addlayer(AvgPoolingLayer(4))
        model.addlayer(FlattenLayer())
        model.addlayer(LinearLayer(256, 10))
        classifier = Classifier(model)

    tr, te = util.get_cifar10()

    batchsize = 100
    ntrain = 50000
    ntest = 10000
    
    for e in range(epoch):
        print('epoch %d'%e)
        randinds = np.random.permutation(ntrain)
        for it in range(0, ntrain, batchsize):
            ind = randinds[it:it+batchsize]
            x = tr[0][ind]
            t = tr[1][ind]
            start = time.time()
            loss, acc = classifier.update(x, t)
            end = time.time()
            print('train iteration %d, elapsed time %f, loss %f, acc %f'%(it//batchsize, end-start, loss, acc))

        start = time.time()
        acctest = 0
        losstest = 0
        for it in range(0, ntest, batchsize):
            x = te[0][it:it+batchsize]
            t = te[1][it:it+batchsize]
            loss, acc = classifier.predict(x, t)
            acctest += int(acc * batchsize)
            losstest += loss
        acctest /= (1.0 * ntest)
        losstest /= (ntest // batchsize)
        end = time.time()
        print('test, elapsed time %f, loss %f, acc %f'%(end-start, loss, acc))

if __name__ == '__main__':
    arch = sys.argv[1]
    if arch == 'cnn' or arch == 'linear':
        main(arch, 40)
    else:
        print('arch should be cnn or linear')
