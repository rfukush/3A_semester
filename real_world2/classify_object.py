from __future__ import print_function
import cv2 as cv
import numpy as np

from dataset import train_dataset, test_dataset

from classifier.random_guess import RandomGuess
#from classifier.knn import KNNClassifier
#from classifier.ncm import NCMClassifier
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
    # if true, images in datasets/test_data/ will be recognized
    # otherwise, images captured by web camera will be recognized
    is_cam = False
    # input image size
    img_size = (72, 104)


    print('construct dataset')
    tr_img, tr_label = train_dataset(obj_class, img_size)
    print('extract feature')
    tr_feat = extract_features(tr_img, image_feature)
    print('%d sample, %dfeature dimension'%tr_feat.shape)

    print('train classifier')
    mlc = machine_learning(**param_dict)
    mlc.fit(tr_feat, tr_label)

    print('test model')
    if is_cam:
        classify_cam(mlc, image_feature, img_size)
    else:
        test_img, test_label = test_dataset(obj_class, img_size)
        test_feat = extract_features(test_img, image_feature)
        test_predict = mlc.predict(test_feat)
        ap = averageprecision(test_label, test_predict)
        print('ap: %f'%ap)

        
def extract_features(img, feature):
    nimg = img.shape[0]
    nfeat = feature(img[0]).size
    feat = np.zeros((nimg, nfeat), dtype=np.float32)
    for i in range(nimg):
        feat[i,:] = np.reshape(feature(img[i]),(1,nfeat))
    return feat


def averageprecision(label, pred):
    ndata = label.size
    npos = np.sum(label).astype(np.float32)

    # reorder the label from higher score
    label = label[np.argsort(-pred)]

    # calculate precision, recall
    clabel = np.cumsum(label)
    recall = clabel / npos
    precision = clabel / (np.array(range(ndata)).astype(np.float32) + 1)

    # calculate ap
    ap = 0
    for i in range(11):
        ap += np.max(precision[np.where(recall >= i * 0.1)]) / 11.0
    return ap


def classify_cam(model, feat, size, threshold = 0.5):

    cap = cv.VideoCapture(0)

    while True:
        ret, frame = cap.read()
        cv.imshow('camera capture', frame)
        test_img = cv.resize(frame, (size[1],size[0]))
        test_feat = feat(test_img)
        test_feat = np.reshape(test_feat, (1, test_feat.size))
        test_predict = model.predict(test_feat)
        if test_predict > threshold:
            print('detected!')

        # press ESC to quit
        k = cv.waitKey(1)
        if k == 27:
            break

    cap.release()
    cv.destroyAllWindows()
    

if __name__ == '__main__':
    main()
