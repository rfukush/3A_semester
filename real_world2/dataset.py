import cv2 as cv
import numpy as np
import os

def train_dataset(category, size):

    # extract positive image
    pospath = os.listdir('datasets/images/' + category)
    npos = len(pospath)
    posimages = np.zeros((npos, size[0], size[1], 3), dtype=np.uint8)
    for i in range(npos):
        img = cv.imread('datasets/images/' + category + '/' + pospath[i])
        posimages[i] = cv.resize(img, (size[1],size[0]))

    # extract negative image
    negpath = os.listdir('datasets/images/background')
    negimages = []
    for p in negpath:
        img = cv.imread('datasets/images/background/' + p)
        # skip too small image
        if img.shape[0] < size[0] or img.shape[1] < size[1]:
            continue
        # crop random subimages
        ncrop = 10
        subimages = np.zeros((ncrop, size[0], size[1], 3), dtype=np.uint8)
        for j in range(ncrop):
            s0 = np.random.randint(img.shape[0] - size[0])
            s1 = np.random.randint(img.shape[1] - size[1])
            subimages[j] = img[s0:s0+size[0], s1:s1+size[1], :]
        negimages.append(subimages)
    negimages = np.concatenate(negimages)
    nneg = negimages.shape[0]

    # concatenate
    images = np.concatenate((posimages, negimages))
    labels = np.concatenate((np.ones(npos, dtype=np.int32), np.zeros(nneg, dtype=np.int32)))
    return images, labels

def test_dataset(category, size):
    testpath = open('datasets/test_data/ground_truth_%s.txt'%category,'r').readlines()
    ntest = len(testpath)
    images = np.zeros((ntest, size[0], size[1], 3), dtype=np.uint8)
    labels = np.zeros(ntest, dtype=np.int32)
    for i,str in enumerate(testpath):
        p, l = str.split()
        img = cv.imread('datasets/test_data/%s/%s.jpg'%(category, p))
        images[i] = cv.resize(img, (size[1],size[0]))
        labels[i] = np.int32(l)
    return images, labels
            
    
    
    
    
