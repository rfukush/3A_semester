#include <stdio.h>
#include <ctype.h>
#include "cv.h"
#include "highgui.h"

int main(int argc, char* argv[]) {
  CvCapture* capture;
  IplImage* frame;
  const char* windowName = "capture test";

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

  while (1) {
    frame = cvQueryFrame(capture);
    if (!frame) {
      fprintf(stderr, "ERROR: frame is null...\n");
      break;
    }

    cvShowImage(windowName, frame);

    if ((cvWaitKey(10) & 255) == 27) {
      break;
    }
  }

  cvReleaseCapture(&capture);
  cvDestroyWindow(windowName);

  return 0;
}

