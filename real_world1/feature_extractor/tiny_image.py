import cv2 as cv
import numpy as np

def tiny_image(img):
    feat = cv.resize(img, dsize=(33, 19)) #TODO1
    feat = feat[1:-1,1:-1,:].astype(np.float32)
    return feat
