

#include <iostream>
#include "cv.h"
#include "highgui.h"
#include "SiftFeature.h"
#include "SiftFeatureManager.h"
#include <vector>

using ics::har::vision::SiftFeature;
using ics::har::vision::SiftFeatureManager;

CvScalar colors[6] = { CV_RGB(255, 0, 0), CV_RGB(0, 255, 0), CV_RGB(0, 0, 255),
    CV_RGB(255, 0, 255), CV_RGB(255, 255, 0), CV_RGB(0, 255, 255) };

void deleteAllElements(std::vector<SiftFeature*>& f) {

  for (int i = 0; i < f.size(); i++) {
    delete f[i];
  }
  f.clear();
}

void copyElements(std::vector<SiftFeature*>& src,
    std::vector<SiftFeature*>& dest) {

  for (int i = 0; i < src.size(); i++) {
    dest.push_back(src[i]);
  }
}

void swapBuffer(std::vector<SiftFeature*>& src,
    std::vector<SiftFeature*>& dest) {
  deleteAllElements(dest);
  copyElements(src, dest);
  src.clear();
}

void match(IplImage* image, std::vector<SiftFeature*>& f1,
    std::vector<SiftFeature*>& f2) {
  int idx = 0;
  for (int i = 0; i < f1.size(); i++) {
    if (f2.size() == 0) {
      return;
    }

    double min = f1[i]->distance(f2[0]);
    double secondMin = min;
    int minId = 0;
    for (int j = 1; j < f2.size(); j++) {
      double diffX = abs(f1[i]->getX() - f2[j]->getX());
      double diffY = abs(f1[i]->getY() - f2[j]->getY());
      if (diffX * diffX + diffY * diffY > 25 * 25) {
        continue;
      }

      double dis = f1[i]->distance(f2[j]);
      if (min > dis) {
        min = dis;
        minId = j;
      } else if (secondMin > dis) {
        secondMin = dis;
      }
    }

    if (min * 2 < secondMin && min < 3000) {
      CvPoint pt1 = cvPoint(cvRound(f1[i]->getX()), cvRound(f1[i]->getY()));
      CvPoint pt2 = cvPoint(cvRound(f2[minId]->getX()),
          cvRound(f2[minId]->getY()));
      cvLine(image, pt1, pt2, colors[idx % 6], 2, 8, 0);
      idx++;
    }
  }
  std::cout << idx << " points matched." << std::endl;
}

int main(int argc, char* argv[]) {
  CvCapture* capture = cvCreateCameraCapture(0);

  if (!capture) {
    std::cerr << "could not create capture device: 0" << std::endl;
  }

  IplImage* image = cvQueryFrame(capture);
  if (!image) {
    std::cerr << "could not query image" << std::endl;
    exit(0);
  }

  image = cvQueryFrame(capture);

  IplImage* renderedImage = cvCreateImage(cvSize(240, 180), IPL_DEPTH_8U, 3);
  IplImage* imgGray = cvCreateImage(cvGetSize(renderedImage), IPL_DEPTH_8U, 1);

  cvNamedWindow("sift tracking", CV_WINDOW_AUTOSIZE);

  SiftFeatureManager manager;
  manager.setImageDoublyMultiplyMode(false);

  bool initial = true;

  std::vector<SiftFeature*> feature1;
  std::vector<SiftFeature*> feature2;

  while (true) {

    image = cvQueryFrame(capture);

    if (!image) {
      std::cerr << "could not query image" << std::endl;
      exit(1);
    }

    cvResize(image, renderedImage, CV_INTER_NN);
    renderedImage->origin = image->origin;

    cvCvtColor(renderedImage, imgGray, CV_RGB2GRAY);
    imgGray->origin = renderedImage->origin;

    manager.computeFeature(imgGray);

    while (manager.hasNextFeature()) {
      feature1.push_back(manager.nextFeature());
    }

    match(renderedImage, feature1, feature2);

    cvShowImage("sift tracking", renderedImage);

    if (cvWaitKey(4) > 0) {
      break;
    }

    swapBuffer(feature1, feature2);
  }

  cvReleaseImage(&imgGray);
  cvReleaseImage(&renderedImage);

  cvReleaseCapture(&capture);
  cvDestroyAllWindows();

  return 0;
}
