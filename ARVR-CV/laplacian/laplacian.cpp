#include <stdio.h>
#include <ctype.h>
#include "cv.h"
#include "highgui.h"

void laplacian(IplImage *source, IplImage *destination);

int main(int argc, char** argv) {
  CvCapture* capture = 0;
  IplImage* input = 0;
  IplImage* output = 0;
  int tick = 0;
  int prev_tick = 0;
  double now = 0.0;
  CvFont font;
  char buffer[256];
  const char* windowName = "laplacian";

  if (argc == 1 || (argc == 2 && strlen(argv[1]) == 1 && isdigit(argv[1][0]))) {
    capture = cvCreateCameraCapture(argc == 2 ? argv[1][0] - '0' : 0);
  } else if (argc == 2) {
    capture = cvCreateFileCapture(argv[1]);
  }
  if (!capture) {
    fprintf(stderr, "ERROR: capture is NULL \n");
    return -1;
  }

  cvNamedWindow(windowName, CV_WINDOW_AUTOSIZE);
  cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1.0, 1.0, 0.0, 1, 0);
  input = cvQueryFrame(capture);
  if (!input) {
    fprintf(stderr, "Could not query frame...\n");
    return -1;
  }
  output = cvCreateImage(cvSize(input->width, input->height), IPL_DEPTH_8U, 3);

  while (1) {
    input = cvQueryFrame(capture);
    if (!input) {
      fprintf(stderr, "Could not query frame...\n");
      break;
    }

    laplacian(input, output);

    sprintf(buffer, "%3.1lfms", now / 1000);
    cvPutText(output, buffer, cvPoint(50, 150), &font, CV_RGB(255, 0, 0));

    cvShowImage(windowName, output);

    if (cvWaitKey(10) >= 0) {
      break;
    }

    tick = cvGetTickCount();
    now = (tick - prev_tick) / cvGetTickFrequency();
    prev_tick = tick;
  }

  cvReleaseImage(&output);

  cvReleaseCapture(&capture);
  cvDestroyWindow(windowName);

  return 0;
}

void averaging(IplImage *source, IplImage *destination) {
  uchar *dataS, *dataD;
  int bpp = 3;
  int step;
  CvSize size;
  int x, y;

  double filter[][3] = { { 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0 }, { 1.0 / 9.0, 1.0
      / 9.0, 1.0 / 9.0 }, { 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0 } };

  /* for outer region */
  cvCopy(source, destination, NULL);

  cvGetRawData(source, &dataS, &step, &size);
  cvGetRawData(destination, &dataD, NULL, NULL);

  for (y = size.height / 4; y < (size.height * 3 / 4); y++) {
    for (x = size.width / 4; x < (size.width * 3 / 4); x++) {
      double b = 0;
      double g = 0;
      double r = 0;
      int i;
      int j;
      uchar* pD;
      for (j = 0; j < 3; j++) {
        for (i = 0; i < 3; i++) {
          uchar* pN = dataS + step * (y + j - 1) + bpp * (x + i - 1);
          double blueS = *pN;
          double greenS = *(pN + 1);
          double redS = *(pN + 2);
          b += blueS * filter[j][i];
          g += greenS * filter[j][i];
          r += redS * filter[j][i];
        }
      }
      pD = dataD + step * y + bpp * x;
      *pD = b;
      *(pD + 1) = g;
      *(pD + 2) = r;
    }
  }
}

void laplacian(IplImage *source, IplImage *destination) {
  uchar *dataS, *dataD;
  int bpp = 3;
  int step;
  CvSize size;
  int x, y;

  double filter[][3] = { { 1., 1., 1. }, { 1., -8., 1. }, { 1., 1., 1. } };

  cvCopy(source, destination, NULL);

  cvGetRawData(source, &dataS, &step, &size);
  cvGetRawData(destination, &dataD, NULL, NULL);

  for (y = size.height / 4; y < (size.height * 3 / 4); y++) {
    uchar* pD = dataD + step * y + bpp * size.width / 4;
    for (x = size.width / 4; x < (size.width * 3 / 4); x++) {
      double intensityD_d = 0;
      int i;
      int j;

      uchar value;
      for (j = 0; j < 3; j++) {
        for (i = 0; i < 3; i++) {
          uchar* pN = dataS + step * (y + j - 1) + bpp * (x + i - 1);
          uchar blueS = *pN;
          uchar greenS = *(pN + 1);
          uchar redS = *(pN + 2);
          double intensityS = (0.114 * blueS + 0.587 * greenS + 0.299 * redS)
              / 255.0;
          double intensityS_d = (219.0 * intensityS) + 16;
          intensityD_d += intensityS_d * filter[j][i];
        }
      }

      value = (uchar) abs(intensityD_d * 10);
      *pD = value;
      *(pD + 1) = value;
      *(pD + 2) = value;
      pD += 3;
    }
  }
}

