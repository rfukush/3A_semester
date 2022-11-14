#include <stdio.h>
#include <ctype.h>
#include "cv.h"
#include "highgui.h"

void match_template(const IplImage* current,
    const CvPoint2D32f directed_position, CvSize halfsize, int step,
    int *number_p, int *sad_p);
void on_mouse(int event, int x, int y, int flags, void* parameters);

#define SELECTING 0
#define SELECTED 1
#define MATCHING 2
#define TEMPLATE_NUMBER 5
#define WINDOW_STEP 5
#define WINDOW_HALFSIZE 8
#define THRESHOLD 5000

IplImage* gray = 0;
int mode = SELECTING;
CvPoint mouse_position;
IplImage* template_gray_images[TEMPLATE_NUMBER];
CvPoint2D32f template_points[TEMPLATE_NUMBER];
int count;

int main(int argc, char** argv) {
  CvCapture* capture = 0;
  IplImage* input = 0;
  IplImage* gray = 0;
  CvPoint2D32f current_point;
  int window_halfsize;
  int window_step;
  int sad;
  int number;
  int p1x, p1y, p2x, p2y;
  int tick = 0, previous_tick = 0;
  double now = 0.0;
  CvFont font;
  char buffer[256];
  int i;

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
  for (i = 0; i < TEMPLATE_NUMBER; i++) {
    template_gray_images[i] = cvCreateImage(cvGetSize(input), 8, 1);
  }

  for (i = 0; i < TEMPLATE_NUMBER; i++) {
    sprintf(buffer, "Template%2d", i);
    cvNamedWindow(buffer, 0);
    cvMoveWindow(buffer, 40 + 50 * i, 20);
    cvResizeWindow(buffer, 160, 120);
  }

  cvNamedWindow("Match", CV_WINDOW_AUTOSIZE);
  cvSetMouseCallback("Match", on_mouse, 0);
  cvMoveWindow("Match", 400, 200);
  cvResizeWindow("Match", 640, 480);

  cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1.0, 1.0, 0.0, 1, 0);

  window_halfsize = WINDOW_HALFSIZE;
  window_step = WINDOW_STEP;

  count = 0;
  while (1) {
    input = cvQueryFrame(capture);
    if (!input) {
      fprintf(stderr, "Could not query frame...\n");
      break;
    }

    cvCvtColor(input, gray, CV_BGR2GRAY);

    if (mode == MATCHING) {
      current_point = cvPointTo32f(cvPoint(mouse_position.x, mouse_position.y));
      match_template(gray, current_point,
          cvSize(window_halfsize, window_halfsize), window_step, &number, &sad);
      sprintf(buffer, "NUM=%d, SAD=%6d", number, sad);
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
      for (i = 0; i < TEMPLATE_NUMBER; i++) {
        p1x = (int) template_points[i].x - window_halfsize * window_step;
        p1y = (int) template_points[i].y - window_halfsize * window_step;
        p2x = (int) template_points[i].x + window_halfsize * window_step - 1;
        p2y = (int) template_points[i].y + window_halfsize * window_step - 1;
        cvRectangle(template_gray_images[i], cvPoint(p1x - 1, p1y - 1),
            cvPoint(p2x + 1, p2y + 1), CV_RGB(0, 255, 0), 1, // thickness, 1
            8, // line_type, 8=8-connected
            0);
      }
    } else if (mode == SELECTED) {
      cvCvtColor(input, template_gray_images[count], CV_BGR2GRAY);
      template_points[count] = cvPointTo32f(
          cvPoint(mouse_position.x, mouse_position.y));
      count++;
      if (count >= TEMPLATE_NUMBER) {
        mode = MATCHING;
      } else {
        mode = SELECTING;
      }
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
    for (i = 0; i < TEMPLATE_NUMBER; i++) {
      if (template_gray_images[i]) {
        sprintf(buffer, "Template%2d", i);
        cvShowImage(buffer, template_gray_images[i]);
      }
    }

    //If a certain key pressed
    if (cvWaitKey(10) >= 0) {
      break;
    }

    tick = cvGetTickCount();
    now = (tick - previous_tick) / cvGetTickFrequency();
    previous_tick = tick;
  }

  for (i = 0; i < TEMPLATE_NUMBER; i++) {
    cvReleaseImage(&template_gray_images[i]);
  }
  cvReleaseImage(&gray);

  cvReleaseCapture(&capture);
  for (i = 0; i < TEMPLATE_NUMBER; i++) {
    sprintf(buffer, "Template%2d", i);
    cvDestroyWindow(buffer);
  }
  cvDestroyWindow("Match");

  return 0;
}

void match_template(const IplImage* current,
    const CvPoint2D32f directed_position, CvSize halfsize, int step,
    int *number_p, int *sad_p) {
  uchar *dataR, *dataS;
  int stepR, stepS;
  CvSize sizeR, sizeS;
  int u, v;
  int uu, vv;
  unsigned int sum[TEMPLATE_NUMBER];
  unsigned int absdiff;
  uchar *pR, *pS;
  uchar r, s;
  int i;

  for (i = 0; i < TEMPLATE_NUMBER; i++) {
    cvGetRawData(template_gray_images[i], &dataR, &stepR, &sizeR);
  }
  cvGetRawData(current, &dataS, &stepS, &sizeS);

  u = directed_position.x;
  v = directed_position.y;

  *sad_p = 255 * (halfsize.height * 2) * (halfsize.width * 2) + 1;
  for (i = 0; i < TEMPLATE_NUMBER; i++) {
    sum[i] = 0;
    for (vv = -halfsize.height * step; vv < halfsize.height * step;
        vv += step) {
      for (uu = -halfsize.width * step; uu < halfsize.width * step;
          uu += step) {
        pR = dataR + (int) template_points[i].x + uu
            + stepR * ((int) template_points[i].y + vv);
        r = *pR;
        pS = dataS + u + uu + stepS * (v + vv);
        s = *pS;
        if (s > r) {
          absdiff = (int) (s - r);
        } else {
          absdiff = (int) (r - s);
        }
        sum[i] = sum[i] + absdiff;
      }
    }
    // fprintf(stderr, "sum[%d]=%d\n", i, sum[i]);
    if (sum[i] < *sad_p) {
      *sad_p = sum[i];
      *number_p = i;
    }
  }
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
    if (count < TEMPLATE_NUMBER) {
      mode = SELECTED;
    }
    break;
  }
}

