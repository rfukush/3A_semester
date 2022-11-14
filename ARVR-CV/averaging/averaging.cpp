#include <stdio.h>
#include <ctype.h>
#include "cv.h"
#include "highgui.h"

void process(IplImage *source, IplImage *destination);

int main(int argc, char** argv) {
  CvCapture* capture = 0;
  IplImage* input = 0;
  IplImage* output = 0;
  int tick = 0, prev_tick = 0;
  double now = 0.0;
  CvFont font;
  char buffer[256];
  const char* windowName = "averaging";

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

    process(input, output);

    sprintf(buffer, "%3.1lfms", now / 1000);
    cvPutText(output, buffer, cvPoint(50, 150), &font, CV_RGB(255, 0, 0));

    cvShowImage(windowName, output);

    //If a certain key pressed
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

void process(IplImage *source, IplImage *destination) {
  uchar *dataS;
  uchar *dataD;
  //uchar* pD;
  int bpp;
  int step;
  CvSize size;
  int x;
  int y;
  int row;
  int col;

  int bankSize = 15;
  int offset = (bankSize - 1) / 2;

  double** filter = (double**) malloc(sizeof(double*) * bankSize);

  for (row = 0; row < bankSize; row++) {
    filter[row] = (double*) malloc(sizeof(double) * bankSize);
    for (col = 0; col < bankSize; col++) {
      filter[row][col] = 1.0 / bankSize / bankSize;
    }
  }

  cvCopy(source, destination, NULL);

  bpp = 3;

  cvGetRawData(source, &dataS, &step, &size);
  cvGetRawData(destination, &dataD, NULL, NULL);

  for (y = size.height / 4; y < (size.height * 3 / 4); y++) {
    uchar* pD = dataD + step * y + bpp * size.width / 4;
    for (x = size.width / 4; x < (size.width * 3 / 4); x++) {
      double b = 0;
      double g = 0;
      double r = 0;
      int i;
      int j;
      //printf("%d\n", *pD);
      for (j = 0; j < bankSize; j++) {
        for (i = 0; i < bankSize; i++) {
          uchar* pN = dataS + step * (y + j - offset) + bpp * (x + i - offset);
          b += filter[j][i] * *pN;
          g += filter[j][i] * *(pN + 1);
          r += filter[j][i] * *(pN + 2);
        }
      }

      *pD = (uchar) b;
      *(pD + 1) = (uchar) g;
      *(pD + 2) = (uchar) r;
      pD += 3;
    }
  }

  free(filter);
}

