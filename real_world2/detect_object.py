from __future__ import print_function
import cv2 as cv
import numpy as np

from dataset import train_dataset, test_dataset

from classifier.random_guess import RandomGuess
from classifier.knn import KNNClassifier
from classifier.ncm import NCMClassifier
from classifier.svm import SVMClassifier

from feature_extractor.tiny_image import tiny_image
from feature_extractor.hog import hog


def main():
    
    np.random.seed(77777) # fix the seed for reproducibility
    # object to be recognized. first you can choose 'bicycle', 'motorbike', or 'horse'
    obj_class = 'bicycle' 
    # handle for feature_extractor/*.py. first you must complete tiny_image
    image_feature = hog
    # handle for classifier/*.py. first you can choose only RandomGuess
    machine_learning= SVMClassifier
    #param_dict = {'k':5}
    param_dict = {}
    # input image size
    img_size = (72, 104)


    print('construct dataset')
    tr_img, tr_label = train_dataset(obj_class, img_size)
    print('extract feature')
    tr_feat = extract_features(tr_img, image_feature)
    print('%d sample, %dfeature dimsneion'%tr_feat.shape)

    print('train classifier')
    mlc = machine_learning(**param_dict)
    mlc.fit(tr_feat, tr_label)

    print('test model')
    detect_cam(mlc, image_feature, image_feature(tr_img[0]).shape)

        
def extract_features(img, feature):
    nimg = img.shape[0]
    nfeat = feature(img[0]).size
    feat = np.zeros((nimg, nfeat), dtype=np.float32)
    for i in range(nimg):
        feat[i,:] = np.reshape(feature(img[i]),(1,nfeat))
    return feat


def detect_cam(model, feature_extractor, filter_size, threshold = 0):

    if isinstance(model, NCMClassifier):
        filt = np.reshape(model.weight, filter_size)
        bias = model.bias
    elif isinstance(model, SVMClassifier):
        filt = np.reshape(model.weight[:-1], filter_size)
        bias = model.weight[-1]

    sbin = 8
    cap = cv.VideoCapture(0)

    while True:
        
        ret, frame = cap.read()
        feats, scales = featpyramid(frame, feature_extractor, sbin)
        score_max = np.NINF

        lefts = []
        uppers = []
        rights = []
        downs = []
        scores = []
        
        for i in range(len(feats)):
            
            scoremap = myconv(feats[i], filt)
            if scoremap.size == 0:
                continue
            pos0, pos1 = np.where(scoremap + bias > threshold)
            score_max = np.maximum(score_max, np.max(scoremap))
            
            if pos0.size == 0:
                continue
            
            left = pos1 * sbin / scales[i]
            upper = pos0 * sbin / scales[i]
            right = left + filt.shape[1] * sbin / scales[i]
            down = upper + filt.shape[0] * sbin / scales[i]
            score = scoremap[pos0,pos1]

            lefts.append(left)
            uppers.append(upper)
            rights.append(right)
            downs.append(down)
            scores.append(score)

        if lefts == []:
            lefts = np.array([])
            uppers = np.array([])
            rights = np.array([])
            downs = np.array([])
            scores = np.array([])
        else:
            lefts = np.concatenate(lefts)
            uppers = np.concatenate(uppers)
            rights = np.concatenate(rights)
            downs = np.concatenate(downs)
            scores = np.concatenate(scores)
            lefts, uppers, rights, downs = nms(lefts, uppers, rights, downs, scores)

        show_result(frame, lefts, uppers, rights, downs, scores)
        if score_max + bias > threshold:
            print('detected!')

        # press ESC to quit
        k = cv.waitKey(1)
        if k == 27:
            break

    cap.release()
    cv.destroyAllWindows()


def featpyramid(frame, feature_extractor, sbin):

    interval = 5
    sc = 2 ** (1.0/interval)
    max_scale = 1 + np.floor(np.log(np.min(frame[:,:,0].shape)/(5.0*sbin))/np.log(sc)).astype(np.int32)
    feats = []
    scales = []

    for i in range(max_scale):
        scale = 1.0/(sc ** i)
        feats.append(feature_extractor(cv.resize(frame, dsize=None, fx=scale, fy=scale)))
        scales.append(scale)

    return feats, scales


def nms(lefts, uppers, rights, downs, scores, overlap = 0.5):
    ind = np.argsort(-scores)
    lefts = lefts[ind]
    uppers = uppers[ind]
    rights = rights[ind]
    downs = downs[ind]
    areas = (rights - lefts + 1) * (downs - uppers + 1)

    candinds = range(lefts.size)
    pickinds = []

    while not(candinds == []):
        pi = candinds.pop(0)
        pickinds.append(pi)
        
        for j in reversed(range(len(candinds))):
            ci = candinds[j]
            l = np.maximum(lefts[pi], lefts[ci])
            u = np.maximum(uppers[pi], uppers[ci])
            r = np.minimum(rights[pi], rights[ci])
            d = np.minimum(downs[pi], downs[ci])
            w = r - l + 1
            h = d - u + 1

            if w > 0 and h > 0:
                o = w * h / areas[ci] #TODO1 change the right side
                if o > overlap:
                    candinds.pop(j) #TODO2 change this line
            
    return lefts[pickinds], uppers[pickinds], rights[pickinds], downs[pickinds]
    

def show_result(frame, lefts, uppers, rights, downs, scores):
    
    for i in range(lefts.size):
        l = np.maximum(0, lefts[i].astype(np.int32))
        u = np.maximum(0, uppers[i].astype(np.int32))
        r = np.minimum(frame.shape[1]-1, rights[i].astype(np.int32))
        d = np.minimum(frame.shape[0]-1, downs[i].astype(np.int32))
        
        frame = cv.rectangle(frame, (l, u), (r, d), (0, 255, 0), 3)
    
    cv.imshow('camera capture', frame)
    

def myconv(feat, filt):
    if feat.shape[0] < filt.shape[0] or feat.shape[1] < filt.shape[1]:
        return np.array([])
    col = np.zeros((feat.shape[0] - (filt.shape[0] - 1), feat.shape[1] - (filt.shape[1] - 1), filt.shape[0], filt.shape[1], filt.shape[2]), dtype=np.float32)
    for fw in range(filt.shape[0]):
        for fh in range(filt.shape[1]):
            col[:,:,fw,fh,:] = feat[fw:fw+col.shape[0],fh:fh+col.shape[1],:]
    return np.sum(col * filt[np.newaxis,np.newaxis,:,:,:], axis=(2,3,4))

if __name__ == '__main__':
    main()
