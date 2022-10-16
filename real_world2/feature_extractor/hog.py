import cv2 as cv
import numpy as np
import sys
sys.path.append('feature_extractor/build/lib.linux-x86_64-2.7')
import hog_desc

def hog(img):
    return hog_desc.hog_desc(img.astype(np.float64)[:,:,::-1])
