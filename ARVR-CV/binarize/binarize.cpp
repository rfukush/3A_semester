#include <stdio.h>
#include <ctype.h>
#include "cv.h"
#include "highgui.h"

int bytes_per_pixel(const IplImage* image);
void process(IplImage *source, IplImage *destination);

const double threshold = 80.0;

int main(int argc, char** argv) {
  CvCapture* capture = 0;
  IplImage* image = 0;
  IplImage* result = 0;
  const char* windowName = "binarize 2";

  if (argc == 1 || (argc == 2 && strlen(argv[1]) == 1 && isdigit(argv[1][0]))) {
    capture = cvCreateCameraCapture(argc == 2 ? argv[1][0] - '0' : 0);
  } else if (argc == 2) {
    capture = cvCreateFileCapture(argv[1]);
  }

  if (!capture) {
    fprintf(stderr, "ERROR: capture is NULL \n");
    return -1;
  }

  image = cvQueryFrame(capture);
  if (!image) {
    fprintf(stderr, "Could not query frame...\n");
    return -1;
  }

  cvNamedWindow(windowName, CV_WINDOW_AUTOSIZE);

  result = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);

  while (1) {
    image = cvQueryFrame(capture);
    if (!image) {
      fprintf(stderr, "Could not query frame...\n");
      break;
    }

    process(image, result);
    cvShowImage(windowName, result);

    if (cvWaitKey(10) >= 0) {
      break;
    }
  }

  cvReleaseImage(&result);
  cvReleaseCapture(&capture);
  cvDestroyWindow(windowName);

  return 0;
}

int bytes_per_pixel(const IplImage* image) {
  return ((((image)->depth & 255) / 8) * (image)->nChannels);
}

void process(IplImage *source, IplImage *destination) {
  int bppS, bppD;
  uchar *pS, *pD;
  uchar *dataS, *dataD;
  int stepS, stepD;
  CvSize sizeS;
  int x, y;
  double intensity;
  int intensity_d;

  bppS = bytes_per_pixel(source);
  bppD = bytes_per_pixel(destination);

  cvGetRawData(source, &dataS, &stepS, &sizeS);
  cvGetRawData(destination, &dataD, &stepD, NULL);

  pS = dataS;
  pD = dataD;

  for (y = 0; y < sizeS.height; y++) {
    for (x = 0; x < sizeS.width; x++) {
      intensity = (0.114 * pS[0] + 0.587 * pS[1] + 0.299 * pS[2]) / 255.0;
      intensity_d = (int) (219.0 * intensity) + 16;
      if (intensity_d > threshold) {
        *pD = 255;
      } else {
        *pD = 0;
      }
      pS += bppS;
      pD += bppD;
    }
  }
}

