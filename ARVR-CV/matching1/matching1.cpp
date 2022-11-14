#include <stdio.h>
#include <ctype.h>
#include "cv.h"
#include "highgui.h"

void match_template(const CvArr* tmp, const CvArr* current,
    const CvPoint2D32f template_position, const CvPoint2D32f directed_position,
    CvSize halfsize, int step, int *sad_p);
void on_mouse(int event, int x, int y, int flags, void* parameters);

#define SELECTING 0
#define SELECTED 1
#define MATCHING 2
#define WINDOW_STEP 5
#define WINDOW_HALFSIZE 8
#define THRESHOLD 5000

IplImage* gray = 0;
int mode = SELECTING;
CvPoint mouse_position;

int main(int argc, char** argv) {
  CvCapture* capture = 0;
  IplImage* input = 0;
  IplImage* gray = 0;
  IplImage* template_gray = 0;
  int window_halfsize;
  int window_step;
  CvPoint2D32f template_point;
  CvPoint2D32f current_point;
  int sad;
  int p1x, p1y, p2x, p2y;
  int tick = 0, previous_tick = 0;
  double now = 0.0;
  CvFont font;
  char buffer[256];

  if (argc == 1 || (argc == 2 && strlen(argv[1]) == 1 && isdigit(argv[1][0]))) {
    capture = cvCreateCameraCapture(argc == 2 ? argv[1][0] - '0' : 0);
  } else if (argc == 2) {
    capture = cvCreateFileCapture(argv[1]);
  }
  if (!capture) {
    fprintf(stderr, "ERROR: capture is NULL \n");
    return -1;
  }

  input = cvQueryFrame(capture);
  if (!input) {
    fprintf(stderr, "Could not query frame...\n");
    return -1;
  }

  gray = cvCreateImage(cvGetSize(input), IPL_DEPTH_8U, 1);
  gray->origin = input->origin;
  cvCvtColor(input, gray, CV_BGR2GRAY);
  template_gray = cvCreateImage(cvGetSize(input), 8, 1);

  cvNamedWindow("Template", CV_WINDOW_AUTOSIZE);
  cvMoveWindow("Template", 40, 20);
  cvResizeWindow("Template", 160, 120);

  cvNamedWindow("Match", CV_WINDOW_AUTOSIZE);
  cvSetMouseCallback("Match", on_mouse, 0);
  cvMoveWindow("Match", 400, 200);
  cvResizeWindow("Match", 640, 480);

  cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1.0, 1.0, 0.0, 1, 0);

  window_halfsize = WINDOW_HALFSIZE;
  window_step = WINDOW_STEP;

  while (1) {
    input = cvQueryFrame(capture);
    if (!input) {
      fprintf(stderr, "Could not query frame...\n");
      break;
    }

    cvCvtColor(input, gray, CV_BGR2GRAY);

    if (mode == MATCHING) {
      current_point = cvPointTo32f(cvPoint(mouse_position.x, mouse_position.y));
      match_template(template_gray, gray, template_point, current_point,
          cvSize(window_halfsize, window_halfsize), window_step, &sad);
      sprintf(buffer, "SAD=%6d", sad);
      cvPutText(gray, buffer, cvPoint(50, 250), &font, CV_RGB(255, 0, 0));
      p1x = (int) current_point.x - window_halfsize * window_step;
      p1y = (int) current_point.y - window_halfsize * window_step;
      p2x = (int) current_point.x + window_halfsize * window_step - 1;
      p2y = (int) current_point.y + window_halfsize * window_step - 1;
      if (sad < THRESHOLD) {
        cvRectangle(gray, cvPoint(p1x, p1y), cvPoint(p2x, p2y),
            CV_RGB(0, 255, 0), 1, // thickness, 1
            8, // line_type, 8=8-connected
            0);
      } else {
        cvRectangle(gray, cvPoint(p1x, p1y), cvPoint(p2x, p2y),
            CV_RGB(255, 0, 0), -1, // thickness, -1 = filled
            8, // line_type, 8=8-connected
            0);
      }
      p1x = (int) template_point.x - window_halfsize * window_step;
      p1y = (int) template_point.y - window_halfsize * window_step;
      p2x = (int) template_point.x + window_halfsize * window_step - 1;
      p2y = (int) template_point.y + window_halfsize * window_step - 1;
      cvRectangle(template_gray, cvPoint(p1x - 1, p1y - 1),
          cvPoint(p2x + 1, p2y + 1), CV_RGB(0, 255, 0), 1, // thickness, 1
          8, // line_type, 8=8-connected
          0);
    } else if (mode == SELECTED) {
      cvCvtColor(input, template_gray, CV_BGR2GRAY);
      template_point = cvPointTo32f(
          cvPoint(mouse_position.x, mouse_position.y));
      mode = MATCHING;
    } else {
      // draw rectangle
      p1x = (int) mouse_position.x - window_halfsize * window_step;
      p1y = (int) mouse_position.y - window_halfsize * window_step;
      p2x = (int) mouse_position.x + window_halfsize * window_step - 1;
      p2y = (int) mouse_position.y + window_halfsize * window_step - 1;
      cvRectangle(gray, cvPoint(p1x, p1y), cvPoint(p2x, p2y), CV_RGB(0, 0, 255),
          1, // thickness, 1
          8, // line_type, 8=8-connected
          0);
      sprintf(buffer, "Select a region!");
      cvPutText(gray, buffer, cvPoint(150, 250), &font, CV_RGB(0, 255, 0));
    }

    // draw cross
    cvLine(gray, cvPoint(mouse_position.x, 0),
        cvPoint(mouse_position.x, gray->height - 1), CV_RGB(0, 255, 255), 1, 8,
        0);
    cvLine(gray, cvPoint(0, mouse_position.y),
        cvPoint(gray->width - 1, mouse_position.y), CV_RGB(0, 255, 255), 1, 8,
        0);

    sprintf(buffer, "%3.1lfms", now / 1000);
    cvPutText(gray, buffer, cvPoint(50, 150), &font, CV_RGB(255, 0, 0));

    cvShowImage("Match", gray);
    if (template_gray) {
      cvShowImage("Template", template_gray);
    }

    //If a certain key pressed
    if (cvWaitKey(10) >= 0) {
      break;
    }

    tick = cvGetTickCount();
    now = (tick - previous_tick) / cvGetTickFrequency();
    previous_tick = tick;
  }

  cvReleaseImage(&template_gray);
  cvReleaseImage(&gray);

  cvReleaseCapture(&capture);
  cvDestroyWindow("Match");

  return 0;
}

void match_template(const CvArr* tmp, const CvArr* current,
    const CvPoint2D32f template_position, const CvPoint2D32f directed_position,
    CvSize halfsize, int step, int *sad_p) {
  uchar *dataR, *dataS;
  int stepR, stepS;
  CvSize sizeR, sizeS;
  int u, v;
  int uu, vv;
  unsigned int sum;
  unsigned int absdiff;
  uchar *pR, *pS;
  uchar r, s;

  cvGetRawData(tmp, &dataR, &stepR, &sizeR);
  cvGetRawData(current, &dataS, &stepS, &sizeS);

  u = directed_position.x;
  v = directed_position.y;

  sum = 0;
  for (vv = -halfsize.height * step; vv < halfsize.height * step; vv += step) {
    for (uu = -halfsize.width * step; uu < halfsize.width * step; uu += step) {
      pR = dataR + (int) template_position.x + uu
          + stepR * ((int) template_position.y + vv);
      r = *pR;
      pS = dataS + u + uu + stepS * (v + vv);
      s = *pS;
      if (s > r) {
        absdiff = (int) (s - r);
      } else {
        absdiff = (int) (r - s);
      }
      sum = sum + absdiff;
    }
  }

  // fprintf(stderr, "sum=%d\n", sum);
  *sad_p = sum;
}

void on_mouse(int event, int x, int y, int flags, void* parameters) {
  switch (event) {
  case CV_EVENT_MOUSEMOVE:
    mouse_position = cvPoint(x, y);
    break;
  case CV_EVENT_LBUTTONDOWN:
    mouse_position = cvPoint(x, y);
    break;
  case CV_EVENT_LBUTTONUP:
    mode = SELECTED;
    break;
  }
}

