#include <stdio.h>
#include <ctype.h>
#include "cv.h"
#include "highgui.h"

int _brightness = 100;
int _contrast = 100;
int histogram_size = 64;
CvHistogram *histogram;
uchar lut[256];
CvMat* lut_matrix;
IplImage *source = 0;
IplImage *destination = 0;
IplImage *histogram_image = 0;

float range_0[] = { 0, 256 };
float* ranges[] = { range_0 };

void update_bc(int arg);

int main(int argc, char* argv[]) {
  CvCapture* capture = 0;
  IplImage *image = 0;

  if (argc == 1 || (argc == 2 && strlen(argv[1]) == 1 && isdigit(argv[1][0]))) {
    capture = cvCreateCameraCapture(argc == 2 ? argv[1][0] - '0' : 0);
  } else if (argc == 2) {
    capture = cvCreateFileCapture(argv[1]);
  }

  if (!capture) {
    fprintf(stderr, "Could not initialize capturing...\n");
    return (-1);
  }

  image = cvQueryFrame(capture);
  if (!image) {
    fprintf(stderr, "Could not query frame...\n");
    return (-1);
  }

  source = cvCreateImage(cvSize(image->width, image->height), IPL_DEPTH_8U, 1);
  cvCvtColor(image, source, CV_BGR2GRAY);

  destination = cvCloneImage(source);

  histogram_image = cvCreateImage(cvSize(320, 200), 8, 1);

  histogram = cvCreateHist(1, &histogram_size, CV_HIST_ARRAY, ranges, 1);

  lut_matrix = cvCreateMatHeader(1, 256, CV_8UC1);
  cvSetData(lut_matrix, lut, 0);

  cvNamedWindow("Image", 0);
  cvNamedWindow("Histogram", 0);

  cvCreateTrackbar("brightness", "Image", &_brightness, 200, update_bc);
  cvCreateTrackbar("contrast", "Image", &_contrast, 200, update_bc);

  update_bc(0);

  for (;;) {
    image = cvQueryFrame(capture);
    if (!image) {
      break;
    }

    cvCvtColor(image, source, CV_BGR2GRAY);
    cvSetData(lut_matrix, lut, 0);
    update_bc(0);

    if (cvWaitKey(10) >= 0) {
      break;
    }
  }

  cvReleaseImage(&source);
  cvReleaseImage(&destination);

  cvReleaseHist(&histogram);

  cvReleaseCapture(&capture);
  cvDestroyWindow("Histogram");
  cvDestroyWindow("Image");

  return (0);
}

void update_bc(int arg) {
  int brightness = _brightness - 100;
  int contrast = _contrast - 100;
  float maximum_value = 0;
  int bin_width;
  int i;
  double delta;
  double a, b;
  int v;

  /*
   * The algorithm is by Werner D. Streidt
   * (http://visca.com/ffactory/archives/5-99/msg00021.html)
   */
  if (contrast > 0) {
    delta = 127.0 * contrast / 100;
    a = 255.0 / (255.0 - delta * 2);
    b = a * (brightness - delta);
    for (i = 0; i < 256; i++) {
      v = cvRound(a * i + b);
      if (v < 0) {
        v = 0;
      }
      if (v > 255) {
        v = 255;
      }
      lut[i] = (uchar) v;
    }
  } else {
    delta = -128.0 * contrast / 100;
    a = (256.0 - delta * 2) / 255.0;
    b = a * brightness + delta;
    for (i = 0; i < 256; i++) {
      v = cvRound(a * i + b);
      if (v < 0) {
        v = 0;
      }
      if (v > 255) {
        v = 255;
      }
      lut[i] = (uchar) v;
    }
  }

  cvLUT(source, destination, lut_matrix);

  cvShowImage("Image", destination);

  cvCalcHist(&destination, histogram, 0, NULL);
  cvZero(destination);
  cvGetMinMaxHistValue(histogram, 0, &maximum_value, 0, 0);
  cvScale(histogram->bins, histogram->bins,
      ((double) histogram_image->height) / maximum_value, 0);

  cvSet(histogram_image, cvScalarAll(255), 0);
  bin_width = cvRound((double) histogram_image->width / histogram_size);

  for (i = 0; i < histogram_size; i++) {
    cvRectangle(
        histogram_image,
        cvPoint(i * bin_width, histogram_image->height),
        cvPoint((i + 1) * bin_width,
            histogram_image->height - cvRound(cvGetReal1D(histogram->bins, i))),
        cvScalarAll(0), -1, 8, 0);
  }

  cvShowImage("Histogram", histogram_image);
}

