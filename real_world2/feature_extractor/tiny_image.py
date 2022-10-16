import cv2 as cv
import numpy as np

def tiny_image(img):
    feat = cv.resize(img, (img.shape[1]//8, img.shape[0]//8)) # or cv.resize(img, dsize=None, fx=1.0/8, fy=1.0/8)
    feat = feat[1:-1,1:-1,:].astype(np.float32)
    return feat
